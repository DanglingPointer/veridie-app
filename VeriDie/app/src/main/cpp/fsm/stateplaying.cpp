#include "fsm/states.hpp"
#include "core/logging.hpp"

namespace fsm {

StatePlaying::StatePlaying(const Context & ctx)
   : m_ctx(ctx)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
}

} // namespace fsm