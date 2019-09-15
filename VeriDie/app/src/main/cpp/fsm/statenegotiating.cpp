#include "fsm/states.hpp"
#include "core/logging.hpp"

namespace fsm {

StateNegotiating::StateNegotiating(const fsm::Context & ctx, std::vector<bt::Device> && peers)
   : m_ctx(ctx)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
}

} // namespace fsm