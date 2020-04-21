#ifndef _TIMERENGINE_HPP
#define _TIMERENGINE_HPP

#include <chrono>
#include <memory>
#include "utils/future.hpp"

namespace core {

struct Timeout
{};

class ITimerEngine
{
public:
   virtual ~ITimerEngine() = default;
   virtual async::Future<Timeout> ScheduleTimer(std::chrono::seconds period) = 0;
};

std::unique_ptr<ITimerEngine> CreateTimerEngine(async::Executor callbackExecutor);

} // namespace core

#endif // TIMERENGINE_HPP
