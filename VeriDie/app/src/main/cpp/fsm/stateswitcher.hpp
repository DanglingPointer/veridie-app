#ifndef FSM_STATESWITCHER_HPP
#define FSM_STATESWITCHER_HPP

#include "fsm/context.hpp"
#include "utils/task.hpp"
#include "utils/timer.hpp"

namespace fsm {

template <typename S, typename... Args>
inline cr::DetachedHandle SwitchToState(Context ctx, Args... args)
{
   co_await ctx.timer->Start(std::chrono::milliseconds(0));
   if (!std::holds_alternative<S>(*ctx.state))
      ctx.state->template emplace<S>(ctx, std::move(args)...);
}

template <>
inline cr::DetachedHandle SwitchToState<std::monostate>(Context ctx)
{
   co_await ctx.timer->Start(std::chrono::milliseconds(0));
   ctx.state->template emplace<std::monostate>();
}

} // namespace fsm

#endif
