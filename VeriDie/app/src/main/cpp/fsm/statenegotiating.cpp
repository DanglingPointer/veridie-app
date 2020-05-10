#include <algorithm>
#include <iterator>

#include "bt/device.hpp"
#include "dice/serializer.hpp"
#include "core/proxy.hpp"
#include "fsm/states.hpp"
#include "sign/commands.hpp"
#include "sign/commandpool.hpp"
#include "utils/logger.hpp"
#include "core/timerengine.hpp"

namespace fsm {
using namespace std::chrono_literals;
using cmd::Make;

uint32_t g_negotiationRound = 0U;

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
   auto [localOffer, _] = m_offers.insert_or_assign(m_localMac, dice::Offer{"", ++g_negotiationRound});
   localOffer->second.mac = GetLocalOfferMac();

   *m_ctx.proxy << Make<cmd::NegotiationStart>(MakeCb([this](cmd::NegotiationStartResponse result) {
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
   *m_ctx.proxy << Make<cmd::ResetConnections>(DetachedCb<cmd::ResetConnectionsResponse>());
   *m_ctx.proxy << Make<cmd::ResetGame>(DetachedCb<cmd::ResetGameResponse>());
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

void StateNegotiating::OnGameStopped()
{
   *m_ctx.proxy << Make<cmd::ResetConnections>(DetachedCb<cmd::ResetConnectionsResponse>());
   Context ctx{m_ctx};
   m_ctx.state->emplace<StateIdle>(ctx);
}

void StateNegotiating::OnSocketReadFailure(const bt::Device & from)
{
   if (m_peers.count(from)) {
      DisconnectDevice(from.mac);
      m_peers.erase(from);
      m_offers.erase(from.mac);
   }
}

const std::string & StateNegotiating::GetLocalOfferMac()
{
   size_t index = g_negotiationRound % m_offers.size();
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
      std::string_view nomineeName("You");
      if (auto it = m_peers.find(bt::Device{"", localOffer->second.mac}); it != std::cend(m_peers))
         nomineeName = it->name;
      *m_ctx.proxy << Make<cmd::NegotiationStop>(DetachedCb<cmd::NegotiationStopResponse>(),
                                                 nomineeName);
      fsm::Context ctx{m_ctx};
      std::unordered_set<bt::Device> peers(std::move(m_peers));
      std::string localMac = std::move(m_localMac);
      std::string nomineeMac = std::move(localOffer->second.mac);
      ctx.state->emplace<StatePlaying>(ctx,
                                       std::move(peers),
                                       std::move(localMac),
                                       std::move(nomineeMac));
      return;
   }

   uint32_t maxRound = g_negotiationRound;
   for (const auto & [_, offer] : m_offers) {
      if (offer.round > maxRound)
         maxRound = offer.round;
   }

   // update local offer
   if (maxRound > g_negotiationRound)
      g_negotiationRound = maxRound;
   localOffer->second.round = g_negotiationRound;
   localOffer->second.mac = GetLocalOfferMac();

   // broadcast local offer
   std::string message = m_ctx.serializer->Serialize(localOffer->second);
   for (const auto & remote : m_peers) {
      *m_ctx.proxy << Make<cmd::SendMessage>(
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
         message,
         remote.mac);
   }

   m_retrySendOffer = m_ctx.timer->ScheduleTimer(1s).Then([this](auto) {
      UpdateAndBroadcastOffer();
   });
}

void StateNegotiating::DisconnectDevice(const std::string & mac)
{
   *m_ctx.proxy << Make<cmd::CloseConnection>(
      MakeCb([this, mac](cmd::CloseConnectionResponse result) {
         result.Handle(
            [&](cmd::ResponseCode::INVALID_STATE) {
               DisconnectDevice(mac);
            },
            [](auto) {});
      }),
      "",
      mac);
}

} // namespace fsm