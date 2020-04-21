#include <algorithm>
#include <iterator>

#include "bt/device.hpp"
#include "dice/serializer.hpp"
#include "jni/proxy.hpp"
#include "fsm/states.hpp"
#include "sign/commands.hpp"
#include "core/logging.hpp"
#include "core/timerengine.hpp"

namespace fsm {
using namespace std::chrono_literals;

uint32_t StateNegotiating::s_round = 0U;

StateNegotiating::StateNegotiating(const fsm::Context & ctx,
                                   std::unordered_set<bt::Device> && peers,
                                   std::string && localMac)
   : m_ctx(ctx)
   , m_localMac(std::move(localMac))
   , m_peers(std::move(peers))
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   std::transform(std::cbegin(m_peers),
                  std::cend(m_peers),
                  std::inserter(m_offers, m_offers.end()),
                  [](const bt::Device & device) {
                     return std::make_pair(device.mac, dice::Offer{"", 0U});
                  });
   auto [localOffer, _] = m_offers.insert_or_assign(m_localMac, dice::Offer{"", ++s_round});
   localOffer->second.mac = GetLocalOfferMac();

   m_ctx.proxy->Forward<cmd::NegotiationStart>(MakeCb([this](cmd::NegotiationStartResponse result) {
      result.Handle(
         [this](cmd::ResponseCode::INVALID_STATE) {
            m_ctx.logger->Write<LogPriority::FATAL>("Cannot start negotiation in invalid state");
         },
         [this](cmd::ResponseCode::OK) {
            UpdateAndBroadcastOffer();
         });
   }));
}

StateNegotiating::~StateNegotiating() = default;

void StateNegotiating::OnBluetoothOff()
{
   m_ctx.proxy->Forward<cmd::ResetGame>(DetachedCb<cmd::ResetGameResponse>());
   fsm::Context ctx{m_ctx};
   ctx.state->emplace<StateIdle>(ctx);
}

void StateNegotiating::OnMessageReceived(const bt::Device & sender, const std::string & message)
{
   if (m_peers.count(sender) == 0)
      return;

   auto decoded = m_ctx.serializer->Deserialize(message);
   auto & offer = std::get<dice::Offer>(decoded);
   m_offers[sender.mac] = std::move(offer);
}

void StateNegotiating::OnSocketReadFailure(const bt::Device & from)
{
   DisconnectDevice(from.mac);
   m_peers.erase(from);
   m_offers.erase(from.mac);
}

const std::string & StateNegotiating::GetLocalOfferMac()
{
   size_t index = s_round % m_offers.size();
   auto it = std::next(m_offers.cbegin(), index);
   return it->first;
}

void StateNegotiating::UpdateAndBroadcastOffer()
{
   auto localOffer = m_offers.find(m_localMac);

   bool allOffersEqual =
      std::all_of(cbegin(m_offers), std::cend(m_offers), [localOffer](const auto & e) {
         return e.second.round == localOffer->second.round &&
                e.second.mac == localOffer->second.mac;
      });

   if (allOffersEqual) {
      std::string nomineeName("You");
      if (auto it = m_peers.find(bt::Device{"", localOffer->second.mac}); it != std::cend(m_peers))
         nomineeName = it->name;
      m_ctx.proxy->Forward<cmd::NegotiationStop>(DetachedCb<cmd::NegotiationStopResponse>(),
                                                 nomineeName);
      fsm::Context ctx{m_ctx};
      std::unordered_set<bt::Device> peers(std::move(m_peers));
      std::string nomineeMac = std::move(localOffer->second.mac);
      ctx.state->emplace<StatePlaying>(ctx, std::move(peers), std::move(nomineeMac));
      return;
   }

   uint32_t maxRound = s_round;
   for (const auto & e : m_offers) {
      if (e.second.round > maxRound)
         maxRound = e.second.round;
   }

   // update local offer
   if (maxRound > s_round)
      s_round = maxRound;
   localOffer->second.round = s_round;
   localOffer->second.mac = GetLocalOfferMac();

   // broadcast local offer
   std::string message = m_ctx.serializer->Serialize(localOffer->second);
   for (const auto & remote : m_peers) {
      m_ctx.proxy->Forward<cmd::SendMessage>(
         MakeCb([this, remote](cmd::SendMessageResponse result) {
            result.Handle(
               [&](cmd::ResponseCode::CONNECTION_NOT_FOUND) {
                  m_peers.erase(remote);
                  m_offers.erase(remote.mac);
               },
               [&](cmd::ResponseCode::SOCKET_ERROR) {
                  OnSocketReadFailure(remote);
               },
               [](auto) {});
         }),
         remote.mac,
         message);
   }

   m_retrySendOffer = m_ctx.timer->ScheduleTimer(1s).Then([this](auto) {
      UpdateAndBroadcastOffer();
   });
}

void StateNegotiating::DisconnectDevice(const std::string & mac)
{
   m_ctx.proxy->Forward<cmd::CloseConnection>(
      MakeCb([this, mac](cmd::CloseConnectionResponse result) {
         result.Handle(
            [&](cmd::ResponseCode::INVALID_STATE) {
               DisconnectDevice(mac);
            },
            [](auto) {});
      }),
      mac,
      "");
}

} // namespace fsm