#include <chrono>

#include "bt/device.hpp"
#include "fsm/states.hpp"
#include "core/proxy.hpp"
#include "dice/serializer.hpp"
#include "sign/commands.hpp"
#include "core/logging.hpp"
#include "core/timerengine.hpp"

using namespace std::chrono_literals;

namespace {

constexpr auto APP_UUID = "76445157-4f39-42e9-a62e-877390cbb4bb";
constexpr auto APP_NAME = "VeriDie";
constexpr uint32_t MAX_SEND_RETRY_COUNT = 10U;
constexpr uint32_t MAX_GAME_START_RETRY_COUNT = 30U;
constexpr auto DISCOVERABILITY_DURATION = 60s;

} // namespace


namespace fsm {

StateConnecting::StateConnecting(const Context & ctx)
   : m_ctx(ctx)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   const bool includePaired = true;
   m_ctx.proxy->Forward<cmd::StartDiscovery>(MakeCb(
      [this](cmd::StartDiscoveryResponse result) {
         result.Handle(
            [this](cmd::ResponseCode::OK) {
               m_discovering = true;
            },
            [this](cmd::ResponseCode::BLUETOOTH_OFF) {
               OnBluetoothOff();
            },
            [this](auto) {
               m_discovering = false;
               CheckStatus();
            });
      }),
      APP_UUID,
      APP_NAME,
      includePaired);
   m_ctx.proxy->Forward<cmd::StartListening>(MakeCb(
      [this](cmd::StartListeningResponse result) {
         result.Handle(
            [this](cmd::ResponseCode::OK) {
               m_listening = true;
            },
            [this](cmd::ResponseCode::BLUETOOTH_OFF) {
               OnBluetoothOff();
            },
            [this](auto) {
               m_listening = false;
               CheckStatus();
            });
      }),
      APP_UUID,
      APP_NAME,
      DISCOVERABILITY_DURATION);
}

StateConnecting::~StateConnecting()
{
   if (m_discovering.value_or(false))
      m_ctx.proxy->Forward<cmd::StopDiscovery>(DetachedCb<cmd::StopDiscoveryResponse>());
   if (m_listening.value_or(false))
      m_ctx.proxy->Forward<cmd::StopListening>(DetachedCb<cmd::StopListeningResponse>());
}

void StateConnecting::OnBluetoothOff()
{
   fsm::Context ctx{m_ctx};
   m_ctx.state->emplace<StateIdle>(ctx);
   std::get_if<StateIdle>(ctx.state)->OnNewGame();
}

void StateConnecting::OnDeviceConnected(const bt::Device & remote)
{
   m_peers.insert(remote);
   SendHelloTo(remote.mac, MAX_SEND_RETRY_COUNT);
}

void StateConnecting::OnDeviceDisconnected(const bt::Device & remote)
{
   m_peers.erase(remote);
}

void StateConnecting::OnMessageReceived(const bt::Device & sender, const std::string & message)
{
   if (m_peers.count(sender) == 0)
      OnDeviceConnected(sender);

   if (m_localMac.has_value())
      return;

   // Code below might throw, but it will be caught in Worker and we don't care about the call stack
   auto decoded = m_ctx.serializer->Deserialize(message);
   auto & hello = std::get<dice::Hello>(decoded);
   m_localMac = std::move(hello.mac);
}

void StateConnecting::OnSocketReadFailure(const bt::Device & from)
{
   m_peers.erase(from);
   DisconnectDevice(from.mac);
}

void StateConnecting::OnConnectivityEstablished()
{
   if (m_retryStartHandle.IsActive())
      return;
   AttemptNegotiationStart(MAX_GAME_START_RETRY_COUNT);
}

void StateConnecting::CheckStatus()
{
   if (m_listening.has_value() && !*m_listening && m_discovering.has_value() && !*m_discovering) {
      m_ctx.proxy->Forward<cmd::ShowAndExit>(DetachedCb<cmd::ShowAndExitResponse>(),
                                             "Cannot proceed due to a fatal failure.");
      m_ctx.state->emplace<std::monostate>();
   }
}

void StateConnecting::SendHelloTo(const std::string & mac, uint32_t retriesLeft)
{
   if (retriesLeft == 0)
      return;
   if (m_peers.count(bt::Device{"", mac}) == 0)
      return;

   std::string hello = m_ctx.serializer->Serialize(dice::Hello{ mac });
   m_ctx.proxy->Forward<cmd::SendMessage>(MakeCb(
      [=](cmd::SendMessageResponse result) {
         result.Handle(
            [&](cmd::ResponseCode::CONNECTION_NOT_FOUND) {
               OnDeviceDisconnected(bt::Device{"", mac});
            },
            [&](cmd::ResponseCode::SOCKET_ERROR) {
               m_peers.erase(bt::Device{"", mac});
               DisconnectDevice(mac);
            },
            [&](cmd::ResponseCode::INVALID_STATE) {
               SendHelloTo(mac, retriesLeft - 1);
            },
            [](cmd::ResponseCode::OK) { /*enjoy*/ });
      }),
      mac,
      std::move(hello));
}

void StateConnecting::DisconnectDevice(const std::string & mac)
{
   m_ctx.proxy->Forward<cmd::CloseConnection>(MakeCb(
      [this, mac](cmd::CloseConnectionResponse result) {
         result.Handle(
            [&](cmd::ResponseCode::INVALID_STATE) {
               DisconnectDevice(mac);
            },
            [&](auto) {});
      }),
      mac,
      "");
}

void StateConnecting::AttemptNegotiationStart(uint32_t retriesLeft)
{
   if (retriesLeft == 0U) {
      m_ctx.proxy->Forward<cmd::ResetGame>(DetachedCb<cmd::ResetGameResponse>());
      m_ctx.proxy->Forward<cmd::ResetConnections>(DetachedCb<cmd::ResetConnectionsResponse>());
      fsm::Context ctx{m_ctx};
      m_ctx.state->emplace<StateIdle>(ctx);
      return;
   }
   if (!m_localMac.has_value()) {
      if (retriesLeft % 3 == 0) {
         m_ctx.proxy->Forward<cmd::ShowToast>(DetachedCb<cmd::ShowToastResponse>(),
                                              "Getting ready...", 3s);
      }
      m_retryStartHandle = m_ctx.timer->ScheduleTimer(1s).Then([=](auto) {
         AttemptNegotiationStart(retriesLeft - 1);
      });
   } else {
      fsm::Context ctx{m_ctx};
      std::string localMac(*std::move(m_localMac));
      std::unordered_set<bt::Device> peers(std::move(m_peers));
      m_ctx.state->emplace<StateNegotiating>(ctx, std::move(peers), std::move(localMac));
   }
}

} // namespace fsm
