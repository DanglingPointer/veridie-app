#include <gtest/gtest.h>
#include "utils/task.hpp"

namespace {

struct Counter
{
   int & count;

   Counter(int & counter)
      : count(counter)
   {
      ++count;
   }
   ~Counter() { --count; }
};

struct DetachedTaskFixture : public ::testing::Test
{
   struct State
   {
      bool beforeSuspend = false;
      bool afterSuspend = false;
      stdcr::coroutine_handle<> handle = nullptr;
      int count = 0;
   };

   struct Awaitable
   {
      State & state;
      bool await_ready() { return false; }
      void await_suspend(stdcr::coroutine_handle<> h) { state.handle = h; }
      void await_resume() {}
   };

   cr::DetachedHandle StartDetachedOperation(State & state)
   {
      Counter c(state.count);
      state.beforeSuspend = true;
      co_await Awaitable{state};
      state.afterSuspend = true;
   }
};

TEST_F(DetachedTaskFixture, detached_task_runs_eagerly)
{
   State state;

   [[maybe_unused]] auto handle = StartDetachedOperation(state);

   EXPECT_TRUE(state.beforeSuspend);
   EXPECT_FALSE(state.afterSuspend);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(1, state.count);

   state.handle.resume();
   EXPECT_TRUE(state.afterSuspend);
   EXPECT_EQ(0, state.count);
}

TEST_F(DetachedTaskFixture, detached_task_runs_without_handle)
{
   State state;

   StartDetachedOperation(state);

   EXPECT_TRUE(state.beforeSuspend);
   EXPECT_FALSE(state.afterSuspend);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(1, state.count);

   state.handle.resume();
   EXPECT_TRUE(state.afterSuspend);
   EXPECT_EQ(0, state.count);
}

struct TaskHandleFixture : public ::testing::Test
{
   template <typename S>
   struct Awaitable
   {
      S & state;
      bool await_ready() { return false; }
      void await_suspend(stdcr::coroutine_handle<> h) { state.handle = h; }
      void await_resume() {}
   };
};

TEST_F(TaskHandleFixture, task_runs_if_handle_is_alive)
{
   struct State
   {
      bool beforeSuspend = false;
      bool afterSuspend = false;
      stdcr::coroutine_handle<> handle = nullptr;
      int count = 0;
   } state;

   auto StartVoidOperation = [](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      state.beforeSuspend = true;
      co_await Awaitable<State>{state};
      state.afterSuspend = true;
   };

   cr::TaskHandle<void> task = StartVoidOperation(state);
   EXPECT_TRUE(task);
   EXPECT_TRUE(state.beforeSuspend);
   EXPECT_FALSE(state.afterSuspend);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(1, state.count);

   state.handle.resume();
   EXPECT_FALSE(task);
   EXPECT_TRUE(state.afterSuspend);
   EXPECT_EQ(0, state.count);
}

TEST_F(TaskHandleFixture, task_is_canceled_when_handle_dies)
{
   struct State
   {
      bool beforeSuspend = false;
      bool afterSuspend = false;
      stdcr::coroutine_handle<> handle = nullptr;
      int count = 0;
   } state;

   auto StartVoidOperation = [](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      state.beforeSuspend = true;
      co_await Awaitable<State>{state};
      state.afterSuspend = true;
   };

   cr::TaskHandle<void> task = StartVoidOperation(state);
   EXPECT_TRUE(task);
   EXPECT_TRUE(state.beforeSuspend);
   EXPECT_FALSE(state.afterSuspend);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(1, state.count);

   task = {};
   EXPECT_FALSE(task);
   EXPECT_TRUE(state.beforeSuspend);
   EXPECT_FALSE(state.afterSuspend);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(1, state.count);

   state.handle.resume();
   EXPECT_FALSE(state.afterSuspend);
   EXPECT_EQ(0, state.count);
}

TEST_F(TaskHandleFixture, task_resumes_outer_task)
{
   struct State
   {
      bool beforeInnerSuspend = false;
      bool afterInnerSuspend = false;
      bool beforeOuterSuspend = false;
      bool afterOuterSuspend = false;
      stdcr::coroutine_handle<> handle = nullptr;
      int count = 0;
   } state;

   auto StartInnerVoidOperation = [](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      state.beforeInnerSuspend = true;
      co_await Awaitable<State>{state};
      state.afterInnerSuspend = true;
   };

   auto StartOuterVoidOperation = [=](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      state.beforeOuterSuspend = true;
      co_await StartInnerVoidOperation(state);
      state.afterOuterSuspend = true;
   };

   auto task = StartOuterVoidOperation(state);
   EXPECT_TRUE(state.beforeOuterSuspend);
   EXPECT_TRUE(state.beforeInnerSuspend);
   EXPECT_FALSE(state.afterOuterSuspend);
   EXPECT_FALSE(state.afterInnerSuspend);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(2, state.count);

   state.handle.resume();
   EXPECT_TRUE(state.afterOuterSuspend);
   EXPECT_TRUE(state.afterInnerSuspend);
   EXPECT_EQ(0, state.count);
}

TEST_F(TaskHandleFixture, canceled_task_doesnt_run_once_resumed)
{
   struct State
   {
      bool beforeInnerSuspend = false;
      bool afterInnerSuspend = false;
      bool beforeOuterSuspend = false;
      bool afterOuterSuspend = false;
      stdcr::coroutine_handle<> handle = nullptr;
      int count = 0;
   } state;

   auto StartInnerVoidOperation = [](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      state.beforeInnerSuspend = true;
      co_await Awaitable<State>{state};
      state.afterInnerSuspend = true;
   };

   auto StartOuterVoidOperation = [=](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      state.beforeOuterSuspend = true;
      co_await StartInnerVoidOperation(state);
      state.afterOuterSuspend = true;
   };

   auto task = StartOuterVoidOperation(state);
   EXPECT_TRUE(state.beforeOuterSuspend);
   EXPECT_TRUE(state.beforeInnerSuspend);
   EXPECT_FALSE(state.afterOuterSuspend);
   EXPECT_FALSE(state.afterInnerSuspend);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(2, state.count);

   task = {};
   state.handle.resume();
   EXPECT_FALSE(state.afterOuterSuspend);
   EXPECT_TRUE(state.afterInnerSuspend);
   EXPECT_EQ(0, state.count);
}

TEST_F(TaskHandleFixture, task_returns_value_to_outer_task)
{
   struct State
   {
      stdcr::coroutine_handle<> handle = nullptr;
      std::string value;
      int count = 0;
   } state;

   auto StartInnerStringOperation = [](State & state) -> cr::TaskHandle<std::string> {
      Counter c(state.count);
      co_await Awaitable<State>{state};
      co_return "Hello World!";
   };

   auto StartOuterVoidOperation = [=](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      std::string result = co_await StartInnerStringOperation(state);
      state.value = std::move(result);
   };

   auto task = StartOuterVoidOperation(state);
   EXPECT_TRUE(state.value.empty());
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(2, state.count);

   state.handle.resume();
   EXPECT_STREQ("Hello World!", state.value.c_str());
   EXPECT_EQ(0, state.count);
}

TEST_F(TaskHandleFixture, canceled_task_doesnt_receive_value_from_inner_task)
{
   struct State
   {
      stdcr::coroutine_handle<> handle = nullptr;
      std::string value;
      int count = 0;
   } state;

   auto StartInnerStringOperation = [](State & state) -> cr::TaskHandle<std::string> {
      Counter c(state.count);
      co_await Awaitable<State>{state};
      co_return "Hello World!";
   };

   auto StartOuterVoidOperation = [=](State & state) -> cr::TaskHandle<void> {
      Counter c(state.count);
      std::string result = co_await StartInnerStringOperation(state);
      state.value = std::move(result);
   };

   auto task = StartOuterVoidOperation(state);
   EXPECT_TRUE(state.value.empty());
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(2, state.count);

   task = {};
   state.handle.resume();
   EXPECT_TRUE(state.value.empty());
   EXPECT_EQ(0, state.count);
}

TEST_F(TaskHandleFixture, three_nested_tasks_resume_each_other)
{
   struct State
   {
      stdcr::coroutine_handle<> handle = nullptr;
      std::string middleValue;
      int innerValue = 0;
   } state;

   auto StartInnerIntOperation = [](State & state) -> cr::TaskHandle<int> {
      co_await Awaitable<State>{state};
      co_return 42;
   };

   auto StartMiddleStringOperation = [=](State & state) -> cr::TaskHandle<std::string> {
      int result = co_await StartInnerIntOperation(state);
      state.innerValue = result;
      co_return std::to_string(result);
   };

   auto StartOuterVoidOperation = [=](State & state) -> cr::TaskHandle<void> {
      std::string result = co_await StartMiddleStringOperation(state);
      state.middleValue = result;
   };

   auto task = StartOuterVoidOperation(state);
   EXPECT_TRUE(state.handle);
   EXPECT_EQ(0, state.innerValue);
   EXPECT_TRUE(state.middleValue.empty());

   state.handle.resume();
   EXPECT_EQ(42, state.innerValue);
   EXPECT_STREQ("42", state.middleValue.c_str());
}

} // namespace
