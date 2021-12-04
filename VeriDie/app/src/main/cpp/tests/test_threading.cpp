#include <gtest/gtest.h>

#include "utils/task.hpp"
#include "core/timer.hpp"
#include "utils/worker.hpp"

#include <atomic>
#include <future>

namespace {
using namespace std::chrono_literals;
using async::Worker;
using core::Timer;

TEST(TimerTest, timer_schedules_delayed_task_correctly)
{
   std::function<void()> pendingTask;
   std::chrono::milliseconds requestedDelay;
   bool taskFinished = false;

   Timer timer([&](auto task, std::chrono::milliseconds delay) {
      pendingTask = std::move(task);
      requestedDelay = delay;
   });

   auto StartTimer = [&]() -> cr::DetachedHandle {
      co_await timer.WaitFor(3s);
      taskFinished = true;
   };

   StartTimer();
   EXPECT_FALSE(taskFinished);
   EXPECT_EQ(3s, requestedDelay);
   EXPECT_TRUE(pendingTask);

   pendingTask();
   EXPECT_TRUE(taskFinished);
}

TEST(TimerTest, timer_schedules_immediate_task)
{
   std::function<void()> pendingTask;
   std::chrono::milliseconds requestedDelay = 123ms;
   bool task1Finished = false;
   bool task2Finished = false;

   Timer timer([&](auto task, std::chrono::milliseconds delay) {
      pendingTask = std::move(task);
      requestedDelay = delay;
   });

   auto StartTimer = [&timer](bool & finished,
                              std::chrono::milliseconds timeout) -> cr::DetachedHandle {
      co_await timer.WaitFor(timeout);
      finished = true;
   };

   StartTimer(task1Finished, 0s);
   EXPECT_FALSE(task1Finished);
   EXPECT_EQ(0ms, requestedDelay);
   EXPECT_TRUE(pendingTask);

   pendingTask();
   EXPECT_TRUE(task1Finished);

   StartTimer(task2Finished, -42s);
   EXPECT_FALSE(task2Finished);
   EXPECT_EQ(0ms, requestedDelay);
   EXPECT_TRUE(pendingTask);

   pendingTask();
   EXPECT_TRUE(task2Finished);
}

TEST(WorkerTest, worker_executes_instantaneous_task_within_100ms)
{
   Worker w({"", 1, nullptr});
   std::this_thread::sleep_for(500ms);

   std::promise<void> done;
   auto future = done.get_future();
   w.Schedule([&] { done.set_value(); });

   auto status = future.wait_for(100ms);
   EXPECT_EQ(std::future_status::ready, status);
}

TEST(WorkerTest, worker_executes_delayed_task_within_100ms)
{
   Worker w({"", 1, nullptr});
   std::this_thread::sleep_for(500ms);

   std::atomic_bool done = false;
   w.Schedule(1s, [&] {
      done = true;
   });

   std::this_thread::sleep_for(900ms);
   EXPECT_EQ(false, done);

   std::this_thread::sleep_for(200ms);
   EXPECT_EQ(true, done);
}

std::unique_ptr<Worker> CreateReadyWorker(size_t capacity)
{
   auto w = std::make_unique<Worker>(Worker::Config{"", capacity, nullptr});

   std::promise<void> ready;
   std::future<void> f = ready.get_future();
   w->Schedule([&] { ready.set_value(); });

   auto status = f.wait_for(1s);
   EXPECT_EQ(std::future_status::ready, status);
   if (status != std::future_status::ready)
      std::abort();
   return w;
}

TEST(WorkerTest, worker_executes_in_correct_order)
{
   auto w = CreateReadyWorker(3);

   bool done1 = false, done2 = false;
   std::promise<void> finished;
   auto future = finished.get_future();

   w->Schedule(1ms, [&] {
      EXPECT_TRUE(done1);
      EXPECT_TRUE(done2);
      finished.set_value();
   });
   w->Schedule([&] { done1 = true; });
   w->Schedule([&] {
      EXPECT_TRUE(done1);
      done2 = true;
   });
   auto status = future.wait_for(100ms);
   EXPECT_EQ(std::future_status::ready, status);
}

TEST(WorkerTest, worker_respects_max_capacity)
{
   auto w = CreateReadyWorker(1);

   std::promise<void> unblocker;
   auto f = unblocker.get_future();
   w->Schedule([&] {
      f.wait();
   });
   std::this_thread::sleep_for(100ms);
   EXPECT_TRUE(w->TrySchedule([] {}));
   EXPECT_FALSE(w->TrySchedule([] {}));

   unblocker.set_value();
   std::this_thread::sleep_for(100ms);
   EXPECT_TRUE(w->TrySchedule([] {}));
}

TEST(WorkerTest, worker_handles_uncaught_exceptions)
{
   std::string workerName;
   std::string exceptionWhat;

   std::promise<void> done;
   auto future = done.get_future();

   Worker w({"test worker", 1, [&] (std::string_view name, std::string_view what) {
      workerName = name;
      exceptionWhat = what;
      done.set_value();
   }});
   w.Schedule([] {
      throw std::runtime_error("test exception");
   });
   auto status = future.wait_for(1s);
   EXPECT_EQ(std::future_status::ready, status);

   EXPECT_STREQ("test worker", workerName.c_str());
   EXPECT_STREQ("test exception", exceptionWhat.c_str());
}

} // namespace
