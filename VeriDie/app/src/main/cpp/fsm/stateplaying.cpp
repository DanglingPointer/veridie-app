#include <functional>
#include <vector>

#include "bt/device.hpp"
#include "fsm/states.hpp"
#include "core/logging.hpp"
#include "core/timerengine.hpp"
#include "dice/serializer.hpp"
#include "dice/engine.hpp"
#include "core/proxy.hpp"
#include "sign/commands.hpp"

using namespace std::chrono_literals;
namespace {

constexpr uint32_t RETRY_COUNT = 5U;
constexpr uint32_t REQUEST_ATTEMPTS = 3U;
constexpr uint32_t ROUNDS_PER_GENERATOR = 10U;
constexpr auto IGNORE_OFFERS_DURATION = 5s;

bool Matches(const dice::Response & response, const dice::Request * request)
{
   if (!request)
      return false;

   if (response.cast.index() != request->cast.index())
      return false;

   auto getSize = [](const auto & vec) {
      return vec.size();
   };
   size_t responseSize = std::visit(getSize, response.cast);
   size_t requestSize = std::visit(getSize, request->cast);

   if (responseSize != requestSize)
      return false;

   return response.successCount.has_value() == request->threshold.has_value();
}

dice::Response GenerateResponse(dice::IEngine & engine, dice::Request && request)
{
   engine.GenerateResult(request.cast);
   std::optional<size_t> successCount;
   if (request.threshold)
      successCount = dice::GetSuccessCount(request.cast, *request.threshold);
   return dice::Response{std::move(request.cast), successCount};
}

} // namespace

namespace fsm {

// Handles connection errors, retries, reconnections(?), buffering etc
// We assume no new requests until the last one has been answered
class StatePlaying::RemotePeerManager : private async::Canceller<32>
{
public:
   RemotePeerManager(const bt::Device & remote,
                     core::Proxy & proxy,
                     core::ITimerEngine & timer,
                     bool isGenerator,
                     std::function<void()> renegotiate)
      : m_remote(remote)
      , m_proxy(proxy)
      , m_timer(timer)
      , m_renegotiate(std::move(renegotiate))
      , m_isGenerator(isGenerator)
      , m_pendingRequest(false)
      , m_connected(true)
   {}
   ~RemotePeerManager()
   {
      if (!m_connected)
         m_proxy.Forward<cmd::CloseConnection>(DetachedCb<cmd::CloseConnectionResponse>(),
                                               m_remote.mac,
                                               "Connection has been lost");
   }
   const bt::Device & GetDevice() const { return m_remote; }
   bool IsConnected() const { return m_connected; }
   bool IsGenerator() const { return m_isGenerator; }
   void SendRequest(const std::string & request, uint32_t attempt = REQUEST_ATTEMPTS)
   {
      if (attempt == 0) {
         m_renegotiate();
         return;
      }
      m_pendingRequest = true;
      Send(request, RETRY_COUNT);
      if (m_isGenerator) {
         m_retryRequest = m_timer.ScheduleTimer(1s).Then([=](auto) {
            if (m_pendingRequest)
               SendRequest(request, attempt - 1);
         });
      }
   }
   void SendResponse(const std::string & response)
   {
      Send(response, RETRY_COUNT);
   }
   void OnReceptionSuccess(bool answeredRequest)
   {
      m_connected = true;
      if (answeredRequest)
         m_pendingRequest = false;
   }
   void OnReceptionFailure()
   {
      m_connected = false;
      if (m_isGenerator)
         m_renegotiate();
   }

private:
   void Send(const std::string & message, uint32_t retriesLeft)
   {
      if (retriesLeft == 0)
         return;

      m_proxy.Forward<cmd::SendMessage>(
         MakeCb([=](cmd::SendMessageResponse result) {
            result.Handle(
               [&](cmd::ResponseCode::INVALID_STATE) {
                  Send(message, retriesLeft - 1);
               },
               [&](cmd::ResponseCode::OK) {
                  m_connected = true;
                  if (!m_queuedMessages.empty()) {
                     std::string lastQueuedMsg = std::move(m_queuedMessages.back());
                     m_queuedMessages.pop_back();
                     Send(lastQueuedMsg, RETRY_COUNT);
                  }
               },
               [&](auto) {
                  m_connected = false;
                  m_queuedMessages.emplace_back(message);
                  if (m_isGenerator)
                     m_renegotiate();
               });
         }),
         m_remote.mac,
         message);
   }

   bt::Device m_remote;
   core::Proxy & m_proxy;
   core::ITimerEngine & m_timer;
   std::function<void()> m_renegotiate;
   const bool m_isGenerator;

   bool m_pendingRequest;
   bool m_connected;
   async::Future<core::Timeout> m_retryRequest;
   std::vector<std::string> m_queuedMessages;
};


StatePlaying::StatePlaying(const Context & ctx,
                           std::unordered_set<bt::Device> && peers,
                           std::string && localMac,
                           std::string && generatorMac)
   : m_ctx(ctx)
   , m_localMac(std::move(localMac))
   , m_localGenerator(m_localMac == generatorMac)
   , m_ignoreOffers(ctx.timer->ScheduleTimer(IGNORE_OFFERS_DURATION))
   , m_responseCount(0U)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);

   for (const auto & peer : peers) {
      bool isGenerator = !m_localGenerator && peer.mac == generatorMac;
      m_managers.emplace(
         std::piecewise_construct,
         std::forward_as_tuple(peer.mac),
         std::forward_as_tuple(peer, *m_ctx.proxy, *m_ctx.timer, isGenerator, [this] {
            StartNegotiation();
         }));
   }
}

StatePlaying::~StatePlaying() = default;

void StatePlaying::OnBluetoothOff()
{
   m_ctx.proxy->Forward<cmd::ResetConnections>(DetachedCb<cmd::ResetConnectionsResponse>());
   m_ctx.proxy->Forward<cmd::ResetGame>(DetachedCb<cmd::ResetGameResponse>());
   fsm::Context ctx{m_ctx};
   m_ctx.state->emplace<StateIdle>(ctx);
}

void StatePlaying::OnDeviceConnected(const bt::Device & remote)
{
   // enable listening for this to be useful?
   if (auto mgr = m_managers.find(remote.mac); mgr != std::cend(m_managers))
      mgr->second.OnReceptionSuccess(true);
}

void StatePlaying::OnMessageReceived(const bt::Device & sender, const std::string & message)
{
   auto mgr = m_managers.find(sender.mac);
   if (mgr == std::cend(m_managers))
      return;

   auto parsed = m_ctx.serializer->Deserialize(message);

   if (std::holds_alternative<dice::Offer>(parsed)) {
      mgr->second.OnReceptionSuccess(m_pendingRequest == nullptr);
      if (m_ignoreOffers.IsActive())
         return;
      StateNegotiating * s = StartNegotiation();
      s->OnMessageReceived(sender, message);
      return;
   }

   if (auto * response = std::get_if<dice::Response>(&parsed)) {
      if (!mgr->second.IsGenerator())
         return;
      if (Matches(*response, m_pendingRequest.get()))
         m_pendingRequest = nullptr;
      mgr->second.OnReceptionSuccess(m_pendingRequest == nullptr);
      ShowResponse(*response, mgr->second.GetDevice().name);
      return;
   }

   if (auto * request = std::get_if<dice::Request>(&parsed)) {
      mgr->second.OnReceptionSuccess(m_pendingRequest == nullptr);
      ShowRequest(*request, mgr->second.GetDevice().name);
      if (m_localGenerator) {
         dice::Response response = GenerateResponse(*m_ctx.generator, std::move(*request));
         std::string encoded = m_ctx.serializer->Serialize(response);
         for (auto & e : m_managers)
            e.second.SendResponse(encoded);
         ShowResponse(response, "You");
      }
      return;
   }
}

void StatePlaying::OnCastRequest(dice::Request && localRequest)
{
   ShowRequest(localRequest, "You");

   std::string encodedRequest = m_ctx.serializer->Serialize(localRequest);
   for (auto & e : m_managers)
      e.second.SendRequest(encodedRequest);

   if (m_localGenerator) {
      dice::Response response = GenerateResponse(*m_ctx.generator, std::move(localRequest));
      std::string encodedResponse = m_ctx.serializer->Serialize(response);
      for (auto & e : m_managers)
         e.second.SendResponse(encodedResponse);
      ShowResponse(response, "You");
   } else {
      m_pendingRequest = std::make_unique<dice::Request>(std::move(localRequest));
   }
}

void StatePlaying::OnGameStopped()
{
   m_ctx.proxy->Forward<cmd::ResetConnections>(DetachedCb<cmd::ResetConnectionsResponse>());
   m_ctx.proxy->Forward<cmd::ResetGame>(DetachedCb<cmd::ResetGameResponse>());
   fsm::Context ctx{m_ctx};
   m_ctx.state->emplace<StateIdle>(ctx);
}

void StatePlaying::OnSocketReadFailure(const bt::Device & transmitter)
{
   if (auto mgr = m_managers.find(transmitter.mac); mgr != std::cend(m_managers))
      mgr->second.OnReceptionFailure();
}

StateNegotiating * StatePlaying::StartNegotiation()
{
   std::unordered_set<bt::Device> peers;
   for (const auto & e : m_managers)
      if (e.second.IsConnected())
         peers.emplace(e.second.GetDevice());
   m_managers.clear();
   fsm::Context ctx{m_ctx};
   std::string localMac(std::move(m_localMac));
   ctx.state->emplace<StateNegotiating>(ctx, std::move(peers), std::move(localMac));
   return std::get_if<StateNegotiating>(ctx.state);
}

void StatePlaying::ShowRequest(const dice::Request & request, const std::string & from)
{
   m_ctx.proxy->Forward<cmd::ShowRequest>(MakeCb(
      [=](cmd::ShowRequestResponse result) {
         if (result.Value() != cmd::ResponseCode::OK::value)
            ShowRequest(request, from);
      }),
      dice::TypeToString(request.cast),
      std::visit([](const auto & vec) {
                    return vec.size();
                 },
                 request.cast),
      request.threshold.value_or(0U),
      from);
}

void StatePlaying::ShowResponse(const dice::Response & response, const std::string & from)
{
   m_ctx.proxy->Forward<cmd::ShowResponse>(MakeCb(
      [=](cmd::ShowResponseResponse result) {
         result.Handle(
            [&](cmd::ResponseCode::OK) {
               if (++m_responseCount >= ROUNDS_PER_GENERATOR)
                  StartNegotiation();
            },
            [&](auto) {
               ShowResponse(response, from);
            });
      }),
      dice::TypeToString(response.cast),
      response.cast,
      response.successCount.value_or(-1),
      from);
}

} // namespace fsm