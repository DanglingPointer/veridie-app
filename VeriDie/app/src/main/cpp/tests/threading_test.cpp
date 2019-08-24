#include <gtest/gtest.h>
#include <atomic>
#include "tests/fakelogger.hpp"
#include "core/timerengine.hpp"
#include "utils/worker.hpp"

namespace {
using namespace std::chrono_literals;

TEST(TimerTest, timer_fires_within_1s_of_scheduled_time)
{
   main::TimerEngine engine([](auto task) { task(); });

   std::atomic_bool fired = false;
   std::optional<main::Timeout> timeout;
   auto handle = engine.ScheduleTimer(3s).Then([&](std::optional<main::Timeout> t) {
      fired = true;
      timeout = std::move(t);
   });

   std::this_thread::sleep_for(2s);
   EXPECT_FALSE(fired.load());
   EXPECT_FALSE(timeout);

   std::this_thread::sleep_for(2s);
   EXPECT_TRUE(fired.load());
   EXPECT_TRUE(timeout);
}

class WorkerFixture : public ::testing::Test
{
protected:
   FakeLogger logger;
};

TEST_F(WorkerFixture, worker_executes_scheduled_task_within_1s_without_errors)
{
   Worker w(nullptr, logger);
   std::atomic_bool done = false;

   w.ScheduleTask([&](auto) { done = true; });

   std::this_thread::sleep_for(1s);
   EXPECT_TRUE(logger.entries.empty());
   EXPECT_TRUE(done.load());
}

TEST_F(WorkerFixture, worker_processes_tasks_in_correct_sequence_without_errors)
{
   Worker w(nullptr, logger);
   std::atomic_bool done1 = false;
   std::atomic_bool done2 = false;

   w.ScheduleTask([&](auto) { done1 = true; });
   w.ScheduleTask([&](auto) { done2 = true; });

   while (false == done2.load())
      ;
   EXPECT_TRUE(done1);
   EXPECT_TRUE(logger.entries.empty());
}

TEST_F(WorkerFixture, worker_passes_its_argument_to_task)
{
   int i = 42;
   Worker w(&i, logger);
   std::atomic_bool done = false;
   std::atomic<void *> actual = nullptr;
   w.ScheduleTask([&] (void * ptr) {
      actual = ptr;
      done = true;
   });
   while (false == done.load())
      ;
   EXPECT_EQ(actual.load(), (void *)(&i));
}

} // namespace