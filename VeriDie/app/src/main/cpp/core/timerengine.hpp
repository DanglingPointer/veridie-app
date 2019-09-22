#ifndef _TIMERENGINE_HPP
#define _TIMERENGINE_HPP

#include <chrono>
#include <memory>
#include "utils/future.hpp"

namespace main {

struct Timeout
{};

class ITimerEngine
{
public:
   virtual ~ITimerEngine() = default;
   virtual async::Future<Timeout> ScheduleTimer(std::chrono::seconds period) = 0;
};

std::unique_ptr<ITimerEngine> CreateTimerEngine(async::Executor callbackExecutor);

} // namespace main

#endif // TIMERENGINE_HPP
