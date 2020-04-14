#include <algorithm>

#include "fsm/states.hpp"
#include "core/logging.hpp"

namespace fsm {

uint32_t StateNegotiating::s_round = 0U;

StateNegotiating::StateNegotiating(const fsm::Context & ctx,
                                   std::unordered_set<bt::Device> && peers)
   : m_ctx(ctx)
   , m_peers(std::move(peers))
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   s_round++;

   std::transform(std::cbegin(m_peers),
                  std::cend(m_peers),
                  std::inserter(m_votes, m_votes.end()),
                  [](const bt::Device & device) {
                     return std::make_pair(device, 0U); // everyone has 0 votes initially
                  });
}

void StateNegotiating::OnBluetoothOff()
{

}

void StateNegotiating::OnMessageReceived(const bt::Device & /*sender*/, const std::string & /*message*/)
{

}

void StateNegotiating::OnSocketReadFailure(const bt::Device & /*from*/)
{

}


} // namespace fsm