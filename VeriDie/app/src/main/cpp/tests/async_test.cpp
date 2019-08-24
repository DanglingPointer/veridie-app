#include <gtest/gtest.h>
#include <deque>
#include <functional>
#include <limits>
#include <cassert>
#include "utils/async.hpp"

namespace {
using namespace async;

class AsyncFixture : public ::testing::Test
{
protected:
   AsyncFixture() {}

   std::function<void(std::function<void()>)> GetExecutor()
   {
      return [this](std::function<void()> task) { queue.emplace_back(std::move(task)); };
   }
   void EnqueueTask(std::function<void()> task) { queue.emplace_back(std::move(task)); }
   size_t ProcessTasks(size_t count = std::numeric_limits<size_t>::max())
   {
      size_t processed = 0;
      while (processed < count && !queue.empty()) {
         auto & task = queue.front();
         task();
         queue.pop_front();
         ++processed;
      }
      return processed;
   }

   std::deque<std::function<void()>> queue;
};

TEST_F(AsyncFixture, future_is_active_before_execution_and_inactive_after)
{
   bool done = false;
   Promise<bool> promise(GetExecutor());
   Future<bool> future = promise.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(promise), [&] {
      done = true;
      return done;
   }));
   ASSERT_TRUE(future.IsActive());
   ProcessTasks();

   assert(done);
   ASSERT_FALSE(future.IsActive());
}

TEST_F(AsyncFixture, task_is_not_executed_if_canceled)
{
   bool done = false;
   Promise<bool> promise(GetExecutor());
   Future<bool> future = promise.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(promise), [&] {
      done = true;
      return done;
   }));
   future.Cancel();
   ProcessTasks();

   ASSERT_FALSE(done);
}

TEST_F(AsyncFixture, future_is_inactive_if_promise_died_before_execution)
{
   Promise<bool> promise(GetExecutor());
   Future<bool> future = promise.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(promise), [&] { return true; }));
   ASSERT_TRUE(future.IsActive());

   queue.clear();
   ASSERT_FALSE(future.IsActive());
}

TEST_F(AsyncFixture, callback_is_called_after_completion_using_executor)
{
   bool done = false;
   std::optional<bool> result;
   Promise<bool> promise(GetExecutor());
   Future<bool> future = promise.GetFuture().Then([&](std::optional<bool> r) { result = r; });

   EnqueueTask(EmbedPromiseIntoTask(std::move(promise), [&] {
      done = true;
      return done;
   }));
   ProcessTasks(1U);
   assert(done);
   ASSERT_FALSE(result.has_value());

   ProcessTasks();
   ASSERT_TRUE(result.has_value());
   ASSERT_EQ(*result, true);
}

TEST_F(AsyncFixture, callback_is_not_called_if_canceled_before_execution)
{
   bool callbackCalled = false;
   Promise<bool> promise(GetExecutor());
   Future<bool> future =
      promise.GetFuture().Then([&](std::optional<bool>) { callbackCalled = true; });

   EnqueueTask(EmbedPromiseIntoTask(std::move(promise), [&] { return true; }));
   future.Cancel();
   ProcessTasks();

   ASSERT_FALSE(callbackCalled);
}

TEST_F(AsyncFixture, callback_is_not_called_if_canceled_after_execution)
{
   bool done = false;
   bool callbackCalled = false;
   Promise<bool> promise(GetExecutor());
   Future<bool> future =
      promise.GetFuture().Then([&](std::optional<bool>) { callbackCalled = true; });

   EnqueueTask(EmbedPromiseIntoTask(std::move(promise), [&] {
      done = true;
      return done;
   }));
   ProcessTasks(1U);
   assert(done);
   ASSERT_FALSE(callbackCalled);

   future.Cancel();
   ProcessTasks();
   ASSERT_FALSE(callbackCalled);
}

TEST_F(AsyncFixture, callback_is_called_without_result_if_promise_died_before_execution)
{
   std::optional<bool> result;
   bool callbackCalled = false;
   Promise<bool> promise(GetExecutor());
   Future<bool> future = promise.GetFuture().Then([&](std::optional<bool> r) {
      result = r;
      callbackCalled = true;
   });

   EnqueueTask(EmbedPromiseIntoTask(std::move(promise), [&] { return true; }));

   queue.clear();
   ProcessTasks();
   ASSERT_TRUE(callbackCalled);
   ASSERT_FALSE(result);
}

TEST_F(AsyncFixture, operator_AND_future_becomes_inactive_iff_both_tasks_have_finished)
{
   bool done1 = false;
   bool done2 = false;

   Promise<bool> p1(GetExecutor());
   Promise<bool> p2(GetExecutor());

   Future<bool> f1 = p1.GetFuture();
   Future<bool> f2 = p2.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(p1), [&] {
      done1 = true;
      return done1;
   }));
   EnqueueTask(EmbedPromiseIntoTask(std::move(p2), [&] {
      done2 = true;
      return done2;
   }));

   Future<Empty> future = std::move(f1) && std::move(f2);
   ASSERT_TRUE(future.IsActive());

   ProcessTasks(1U);
   assert(done1);
   ASSERT_TRUE(future.IsActive());

   ProcessTasks(1U);
   assert(done2);
   ASSERT_FALSE(future.IsActive());
}

TEST_F(AsyncFixture, operator_OR_futures_become_inactive_once_one_of_the_tasks_has_finished)
{
   bool done1 = false;

   Promise<bool> p1(GetExecutor());
   Promise<bool> p2(GetExecutor());

   Future<bool> f1 = p1.GetFuture();
   Future<bool> f2 = p2.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(p1), [&] {
      done1 = true;
      return done1;
   }));

   Future<Empty> future = std::move(f1) || std::move(f2);
   ASSERT_TRUE(future.IsActive());

   ProcessTasks();
   assert(done1);
   ASSERT_FALSE(future.IsActive());
}

TEST_F(AsyncFixture, operator_AND_callback_is_executed_iff_both_tasks_have_finished)
{
   bool done1 = false;
   bool done2 = false;

   std::optional<Empty> result;
   bool done = false;

   Promise<bool> p1(GetExecutor());
   Promise<bool> p2(GetExecutor());

   Future<bool> f1 = p1.GetFuture();
   Future<bool> f2 = p2.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(p1), [&] {
      done1 = true;
      return done1;
   }));
   EnqueueTask(EmbedPromiseIntoTask(std::move(p2), [&] {
      done2 = true;
      return done2;
   }));

   Future<Empty> future = (std::move(f1) && std::move(f2)).Then([&](std::optional<Empty> r) {
      result = r;
      done = true;
   });

   ProcessTasks(1U);
   assert(done1);
   ASSERT_FALSE(done);
   ASSERT_FALSE(result);

   ProcessTasks();
   assert(done2);
   ASSERT_TRUE(done);
   ASSERT_TRUE(result);
}

TEST_F(AsyncFixture, operator_OR_callback_is_executed_once_one_of_the_tasks_has_finished)
{
   bool done1 = false;

   std::optional<Empty> result;
   bool done = false;

   Promise<bool> p1(GetExecutor());
   Promise<bool> p2(GetExecutor());

   Future<bool> f1 = p1.GetFuture();
   Future<bool> f2 = p2.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(p1), [&] {
      done1 = true;
      return done1;
   }));

   Future<Empty> future = (std::move(f1) || std::move(f2)).Then([&](std::optional<Empty> r) {
      result = r;
      done = true;
   });

   ProcessTasks();
   assert(done1);
   ASSERT_TRUE(done);
   ASSERT_TRUE(result);
}

TEST_F(AsyncFixture, operator_OR_cancels_the_last_task)
{
   bool done1 = false;
   bool done2 = false;

   bool done = false;

   Promise<bool> p1(GetExecutor());
   Promise<bool> p2(GetExecutor());

   Future<bool> f1 = p1.GetFuture();
   Future<bool> f2 = p2.GetFuture();

   EnqueueTask(EmbedPromiseIntoTask(std::move(p1), [&] {
      done1 = true;
      return done1;
   }));

   Future<Empty> future =
      (std::move(f1) || std::move(f2)).Then([&](std::optional<Empty> r) { done = true; });

   ProcessTasks();
   assert(done1);
   assert(done);

   EnqueueTask(EmbedPromiseIntoTask(std::move(p2), [&] {
      done2 = true;
      return done2;
   }));
   ProcessTasks();
   ASSERT_FALSE(done2);
}

} // namespace