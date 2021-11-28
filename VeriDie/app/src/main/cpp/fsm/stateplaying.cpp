#include <functional>
#include <vector>

#include "bt/device.hpp"
#include "core/log.hpp"
#include "fsm/states.hpp"
#include "fsm/stateswitcher.hpp"
#include "utils/timer.hpp"
#include "dice/serializer.hpp"
#include "dice/engine.hpp"
#include "sign/commands.hpp"
#include "sign/commandpool.hpp"

using namespace std::chrono_literals;
namespace {

constexpr auto TAG = "FSM";
constexpr uint32_t RETRY_COUNT = 5U;
constexpr uint32_t REQUEST_ATTEMPTS = 3U;
constexpr uint32_t ROUNDS_PER_GENERATOR = 10U;
constexpr auto IGNORE_OFFERS_DURATION = 10s;

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
class StatePlaying::RemotePeerManager : private cr::TaskOwner<>
{
public:
   RemotePeerManager(const bt::Device & remote,
                     core::CommandAdapter & proxy,
                     async::Timer & timer,
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
         m_proxy.FireAndForget<cmd::CloseConnection>("Connection has been lost", m_remote.mac);
   }
   const bt::Device & GetDevice() const { return m_remote; }
   bool IsConnected() const { return m_connected; }
   bool IsGenerator() const { return m_isGenerator; }
   void SendRequest(const std::string & request)
   {
      m_pendingRequest = true;
      StartTask(m_isGenerator ? SendRequestToGenerator(request) : Send(request));
   }
   void SendResponse(const std::string & response) { StartTask(Send(response)); }
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
   [[nodiscard]] cr::TaskHandle<void> SendRequestToGenerator(std::string request)
   {
      for (unsigned attempt = REQUEST_ATTEMPTS; attempt > 0; --attempt) {
         co_await StartNestedTask(Send(request));
         co_await m_timer.WaitFor(1s);
         if (!m_pendingRequest)
            co_return;
      }
      m_renegotiate();
   }
   [[nodiscard]] cr::TaskHandle<void> Send(std::string message)
   {
      if (message.size() > cmd::SendLongMessage::MAX_BUFFER_SIZE) {
         m_proxy.FireAndForget<cmd::ShowToast>("Cannot send too long message, try fewer dices", 7s);
         co_return;
      }

      unsigned retriesLeft = RETRY_COUNT;

      do {
         cmd::SendMessageResponse response;
         if (message.size() <= cmd::SendMessage::MAX_BUFFER_SIZE)
            response = co_await m_proxy.Command<cmd::SendMessage>(message, m_remote.mac);
         else
            response = co_await m_proxy.Command<cmd::SendLongMessage>(message, m_remote.mac);

         switch (response) {
         case cmd::SendMessageResponse::INVALID_STATE:
         case cmd::SendMessageResponse::INTEROP_FAILURE:
            break;
         case cmd::SendMessageResponse::OK:
            m_connected = true;
            if (m_queuedMessages.empty())
               co_return;

            message = std::move(m_queuedMessages.back());
            m_queuedMessages.pop_back();
            retriesLeft = RETRY_COUNT + 1;
            break;
         default:
            m_connected = false;
            m_queuedMessages.emplace_back(std::move(message));
            if (m_isGenerator)
               m_renegotiate();
            co_return;
         }
      } while (--retriesLeft > 0);
   }

   bt::Device m_remote;
   core::CommandAdapter & m_proxy;
   async::Timer & m_timer;
   std::function<void()> m_renegotiate;
   const bool m_isGenerator;

   bool m_pendingRequest;
   bool m_connected;
   std::vector<std::string> m_queuedMessages;
};


StatePlaying::StatePlaying(const Context & ctx,
                           std::unordered_set<bt::Device> && peers,
                           std::string && localMac,
                           std::string && generatorMac)
   : m_ctx(ctx)
   , m_localMac(std::move(localMac))
   , m_localGenerator(m_localMac == generatorMac)
   , m_ignoreOffers(m_ctx.timer->WaitFor(IGNORE_OFFERS_DURATION))
   , m_responseCount(0U)
{
   Log::Info(TAG, "New state: {}", __func__);
   m_ignoreOffers.Run(Executor());

   for (const auto & peer : peers) {
      bool isGenerator = !m_localGenerator && peer.mac == generatorMac;
      m_managers.emplace(std::piecewise_construct,
                         std::forward_as_tuple(peer.mac),
                         std::forward_as_tuple(peer,
                                               std::ref(m_ctx.proxy),
                                               std::ref(*m_ctx.timer),
                                               isGenerator,
                                               [this] {
                                                  StartNegotiation();
                                               }));
   }
}

StatePlaying::~StatePlaying() = default;

void StatePlaying::OnBluetoothOff()
{
   m_ctx.proxy.FireAndForget<cmd::ResetConnections>();
   m_ctx.proxy.FireAndForget<cmd::ResetGame>();
   SwitchToState<StateIdle>(m_ctx);
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

   try {
      auto parsed = m_ctx.serializer->Deserialize(message);

      if (std::holds_alternative<dice::Offer>(parsed)) {
         mgr->second.OnReceptionSuccess(m_pendingRequest == nullptr);
         if (m_ignoreOffers)
            return;
         StateNegotiating & s = StartImmediateNegotiation();
         s.OnMessageReceived(sender, message);
         return;
      }

      if (auto * response = std::get_if<dice::Response>(&parsed)) {
         if (!mgr->second.IsGenerator())
            return;
         if (Matches(*response, m_pendingRequest.get()))
            m_pendingRequest = nullptr;
         mgr->second.OnReceptionSuccess(m_pendingRequest == nullptr);
         StartTask(ShowResponse(*response, mgr->second.GetDevice().name));
         return;
      }

      if (auto * request = std::get_if<dice::Request>(&parsed)) {
         mgr->second.OnReceptionSuccess(m_pendingRequest == nullptr);
         StartTask(ShowRequest(*request, mgr->second.GetDevice().name));
         if (m_localGenerator) {
            dice::Response response = GenerateResponse(*m_ctx.generator, std::move(*request));
            std::string encoded = m_ctx.serializer->Serialize(response);
            for (auto & [_, peer] : m_managers)
               peer.SendResponse(encoded);
            StartTask(ShowResponse(response, "You"));
         }
         return;
      }
   }
   catch (const std::exception & e) {
      Log::Error(TAG, "StateConnecting::{}(): {}", __func__, e.what());
   }
}

void StatePlaying::OnCastRequest(dice::Request && localRequest)
{
   StartTask(ShowRequest(localRequest, "You"));

   std::string encodedRequest = m_ctx.serializer->Serialize(localRequest);
   for (auto & [_, mgr] : m_managers)
      mgr.SendRequest(encodedRequest);

   if (m_localGenerator) {
      dice::Response response = GenerateResponse(*m_ctx.generator, std::move(localRequest));
      std::string encodedResponse = m_ctx.serializer->Serialize(response);
      for (auto & [_, mgr] : m_managers)
         mgr.SendResponse(encodedResponse);
      StartTask(ShowResponse(response, "You"));
   } else {
      m_pendingRequest = std::make_unique<dice::Request>(std::move(localRequest));
   }
}

void StatePlaying::OnGameStopped()
{
   m_ctx.proxy.FireAndForget<cmd::ResetConnections>();
   m_ctx.proxy.FireAndForget<cmd::ResetGame>();
   SwitchToState<StateIdle>(m_ctx);
}

void StatePlaying::OnSocketReadFailure(const bt::Device & transmitter)
{
   if (auto mgr = m_managers.find(transmitter.mac); mgr != std::cend(m_managers))
      mgr->second.OnReceptionFailure();
}

void StatePlaying::StartNegotiation()
{
   std::unordered_set<bt::Device> peers;
   for (const auto & [_, mgr] : m_managers)
      if (mgr.IsConnected())
         peers.emplace(mgr.GetDevice());

   SwitchToState<StateNegotiating>(m_ctx, std::move(peers), m_localMac);
}

StateNegotiating & StatePlaying::StartImmediateNegotiation()
{
   std::unordered_set<bt::Device> peers;
   for (const auto & [_, mgr] : m_managers)
      if (mgr.IsConnected())
         peers.emplace(mgr.GetDevice());
   m_managers.clear();
   fsm::Context ctx{m_ctx};
   std::string localMac(m_localMac);
   return ctx.state->emplace<StateNegotiating>(ctx, std::move(peers), std::move(localMac));
}

cr::TaskHandle<void> StatePlaying::ShowRequest(const dice::Request & request,
                                               const std::string & from)
{
   const auto response =
      co_await m_ctx.proxy.Command<cmd::ShowRequest>(dice::TypeToString(request.cast),
                                                     std::visit(
                                                        [](const auto & vec) {
                                                           return vec.size();
                                                        },
                                                        request.cast),
                                                     request.threshold.value_or(0U),
                                                     from);

   if (response != cmd::ShowRequestResponse::OK)
      OnGameStopped();
}

cr::TaskHandle<void> StatePlaying::ShowResponse(const dice::Response & response,
                                                const std::string & from)
{
   const size_t responseSize = std::visit(
      [](const auto & vec) {
         return vec.size();
      },
      response.cast);

   if (responseSize > cmd::ShowLongResponse::MAX_BUFFER_SIZE / 3) {
      m_ctx.proxy.FireAndForget<cmd::ShowToast>("Request is too big, cannot proceed", 7s);
      co_return;
   }

   cmd::ShowResponseResponse responseCode;

   if (responseSize <= cmd::ShowResponse::MAX_BUFFER_SIZE / 3) {
      responseCode =
         co_await m_ctx.proxy.Command<cmd::ShowResponse>(response.cast,
                                                         dice::TypeToString(response.cast),
                                                         response.successCount.value_or(-1),
                                                         from);
   } else {
      responseCode =
         co_await m_ctx.proxy.Command<cmd::ShowLongResponse>(response.cast,
                                                             dice::TypeToString(response.cast),
                                                             response.successCount.value_or(-1),
                                                             from);
   }

   if (responseCode != cmd::ShowResponseResponse::OK)
      OnGameStopped();
   else if (++m_responseCount >= ROUNDS_PER_GENERATOR)
      StartNegotiation();
}

} // namespace fsm