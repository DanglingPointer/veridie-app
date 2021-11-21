#include <chrono>

#include "bt/device.hpp"
#include "fsm/states.hpp"
#include "fsm/stateswitcher.hpp"
#include "core/proxy.hpp"
#include "dice/serializer.hpp"
#include "sign/commands.hpp"
#include "sign/commandpool.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"

using namespace std::chrono_literals;
using cmd::Make;

namespace {

constexpr auto APP_UUID = "76445157-4f39-42e9-a62e-877390cbb4bb";
constexpr auto APP_NAME = "VeriDie";
constexpr uint32_t MAX_SEND_RETRY_COUNT = 10U;
constexpr uint32_t MAX_GAME_START_RETRY_COUNT = 30U;
constexpr uint32_t MAX_DISCOVERY_RETRY_COUNT = 2U;
constexpr uint32_t MAX_LISTENING_RETRY_COUNT = 2U;
constexpr auto DISCOVERABILITY_DURATION = 5min;

} // namespace


namespace fsm {

StateConnecting::StateConnecting(const Context & ctx)
   : m_ctx(ctx)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   KickOffDiscovery(MAX_DISCOVERY_RETRY_COUNT);
   KickOffListening(MAX_LISTENING_RETRY_COUNT);
}

StateConnecting::~StateConnecting()
{
   if (m_discovering.value_or(false))
      *m_ctx.proxy << Make<cmd::StopDiscovery>(DetachedCb<cmd::StopDiscoveryResponse>());
   if (m_listening.value_or(false))
      *m_ctx.proxy << Make<cmd::StopListening>(DetachedCb<cmd::StopListeningResponse>());
}

void StateConnecting::OnBluetoothOff()
{
   SwitchToState<StateIdle>(m_ctx, true);
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
   if (m_peers.count(from)) {
      m_peers.erase(from);
      DisconnectDevice(from.mac);
   }
}

void StateConnecting::OnConnectivityEstablished()
{
   if (m_retryStartHandle)
      return;
   m_retryStartHandle = AttemptNegotiationStart();
   m_retryStartHandle.Run();
}

void StateConnecting::OnGameStopped()
{
   *m_ctx.proxy << Make<cmd::ResetConnections>(DetachedCb<cmd::ResetConnectionsResponse>());
   SwitchToState<StateIdle>(m_ctx);
}

void StateConnecting::CheckStatus()
{
   if (m_listening.has_value() && !*m_listening && m_discovering.has_value() && !*m_discovering) {
      *m_ctx.proxy << Make<cmd::ShowAndExit>(DetachedCb<cmd::ShowAndExitResponse>(),
                                             "Cannot proceed due to a fatal failure.");
      SwitchToState<std::monostate>(m_ctx);
   }
}

void StateConnecting::SendHelloTo(const std::string & mac, uint32_t retriesLeft)
{
   if (retriesLeft == 0)
      return;
   if (m_peers.count(bt::Device{"", mac}) == 0)
      return;

   std::string hello = m_ctx.serializer->Serialize(dice::Hello{mac});
   *m_ctx.proxy << Make<cmd::SendMessage>(MakeCb([=](cmd::SendMessageResponse result) {
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
                                          std::move(hello),
                                          mac);
}

void StateConnecting::DisconnectDevice(const std::string & mac)
{
   *m_ctx.proxy << Make<cmd::CloseConnection>(
      MakeCb([this, mac](cmd::CloseConnectionResponse result) {
         result.Handle(
            [&](cmd::ResponseCode::INVALID_STATE) {
               DisconnectDevice(mac);
            },
            [&](auto) {});
      }),
      "",
      mac);
}

cr::TaskHandle<void> StateConnecting::AttemptNegotiationStart()
{
   for (unsigned retriesLeft = MAX_GAME_START_RETRY_COUNT; retriesLeft > 0; --retriesLeft) {
      if (m_localMac.has_value()) {
         cmd::pool.Resize(m_peers.size());
         SwitchToState<StateNegotiating>(m_ctx, std::move(m_peers), *std::move(m_localMac));
         co_return;
      }
      if (retriesLeft % 3 == 0) {
         *m_ctx.proxy << Make<cmd::ShowToast>(DetachedCb<cmd::ShowToastResponse>(),
                                              "Getting ready...",
                                              3s);
      }
      co_await m_ctx.timer->WaitFor(1s);
   }

   *m_ctx.proxy << Make<cmd::ResetGame>(DetachedCb<cmd::ResetGameResponse>());
   *m_ctx.proxy << Make<cmd::ResetConnections>(DetachedCb<cmd::ResetConnectionsResponse>());
   SwitchToState<StateIdle>(m_ctx);
   co_return;
}

void StateConnecting::KickOffDiscovery(uint32_t retriesLeft)
{
   const bool includePaired = true;
   *m_ctx.proxy << Make<cmd::StartDiscovery>(
      MakeCb([=](cmd::StartDiscoveryResponse result) {
         result.Handle(
            [this](cmd::ResponseCode::OK) {
               m_discovering = true;
            },
            [this](cmd::ResponseCode::BLUETOOTH_OFF) {
               OnBluetoothOff();
            },
            [=](cmd::ResponseCode::INVALID_STATE) {
               if (retriesLeft == 0) {
                  m_discovering = false;
                  CheckStatus();
               } else {
                  StartTask([=] () -> cr::TaskHandle<void> {
                     co_await m_ctx.timer->WaitFor(1s);
                     KickOffDiscovery(retriesLeft - 1);
                  }());
               }
            },
            [this](auto) {
               m_discovering = false;
               CheckStatus();
            });
      }),
      APP_UUID,
      APP_NAME,
      includePaired);
}

void StateConnecting::KickOffListening(uint32_t retriesLeft)
{
   *m_ctx.proxy << Make<cmd::StartListening>(
      MakeCb([=](cmd::StartListeningResponse result) {
         result.Handle(
            [this](cmd::ResponseCode::OK) {
               m_listening = true;
            },
            [this](cmd::ResponseCode::BLUETOOTH_OFF) {
               OnBluetoothOff();
            },
            [this](cmd::ResponseCode::USER_DECLINED) {
               m_listening = false;
               CheckStatus();
            },
            [=](auto) {
               if (retriesLeft == 0) {
                  m_listening = false;
                  CheckStatus();
               } else {
                  StartTask([=] () -> cr::TaskHandle<void> {
                     co_await m_ctx.timer->WaitFor(1s);
                     KickOffListening(retriesLeft - 1);
                  }());
               }
            });
      }),
      APP_UUID,
      APP_NAME,
      DISCOVERABILITY_DURATION);
}

} // namespace fsm
