#ifndef _TIMERENGINE_HPP
#define _TIMERENGINE_HPP

#include <chrono>
#include <memory>
#include "utils/async.hpp"

namespace main {

struct Timeout
{};

class TimerEngine
{
public:
   explicit TimerEngine(async::Executor callbackExecutor);
   ~TimerEngine();
   async::Future<Timeout> ScheduleTimer(std::chrono::seconds period);

private:
   void Launch();

   struct State;
   std::shared_ptr<State> m_state;
   const async::Executor m_executor;
};

} // namespace main

#endif // TIMERENGINE_HPP
