#include <atomic>
#include <list>
#include <mutex>
#include <thread>
#include "core/timerengine.hpp"

namespace main {
using namespace std::chrono_literals;

namespace {

struct Timer
{
   async::Promise<Timeout> promise;
   std::chrono::steady_clock::time_point time;
};

} // namespace

struct TimerEngine::State
{
   std::atomic_bool stop;
   std::list<Timer> timers;
   std::mutex mutex;
};


TimerEngine::TimerEngine(async::Executor callbackExecutor)
   : m_state(std::make_shared<TimerEngine::State>())
   , m_executor(std::move(callbackExecutor))
{
   m_state->stop.store(false);
   Launch();
}

TimerEngine::~TimerEngine()
{
   m_state->stop.store(true);
}

async::Future<Timeout> TimerEngine::ScheduleTimer(std::chrono::seconds period)
{
   async::Promise<Timeout> p(m_executor);
   auto future = p.GetFuture();
   const auto timeToFire = std::chrono::steady_clock::now() + period;
   {
      std::lock_guard lock(m_state->mutex);
      m_state->timers.push_back({std::move(p), timeToFire});
   }
   return future;
}

void TimerEngine::Launch()
{
   std::thread([state = m_state] {
      while (false == state->stop.load()) {
         std::this_thread::sleep_for(1s);
         auto now = std::chrono::steady_clock::now();

         std::lock_guard lock(state->mutex);
         for (auto it = std::begin(state->timers); it != std::end(state->timers);) {
            if (it->promise.IsCancelled()) {
               it = state->timers.erase(it);
            } else if (it->time <= now) {
               it->promise.Finished(Timeout{});
               it = state->timers.erase(it);
            } else {
               ++it;
            }
         }
      }
   }).detach();
}

} // namespace main