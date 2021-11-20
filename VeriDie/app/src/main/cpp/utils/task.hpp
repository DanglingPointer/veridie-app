#ifndef TASK_HPP
#define TASK_HPP

#include "utils/coroutine.hpp"

#include <concepts>
#include <exception>
#include <type_traits>
#include <variant>

namespace cr {

template <typename T>
concept TaskResult = std::is_move_constructible_v<T> || std::is_same_v<T, void>;

namespace internal {
template <typename T>
struct ValidAwaitSuspendReturnType : std::false_type
{};
template <>
struct ValidAwaitSuspendReturnType<bool> : std::true_type
{};
template <>
struct ValidAwaitSuspendReturnType<void> : std::true_type
{};
template <typename P>
struct ValidAwaitSuspendReturnType<stdcr::coroutine_handle<P>> : std::true_type
{};
template <typename T>
concept AwaitSuspendReturnType = ValidAwaitSuspendReturnType<T>::value;

struct DetachedPromise;

template <TaskResult T>
struct Promise;
} // namespace internal


// shamelessly copied from ref impl
template <typename T>
concept Awaiter = requires(T && awaiter, stdcr::coroutine_handle<void> h) {
   static_cast<bool>(awaiter.await_ready());
   awaiter.await_resume();
   requires internal::AwaitSuspendReturnType<decltype(awaiter.await_suspend(h))>;
};


struct DetachedHandle
{
   using promise_type = internal::DetachedPromise;
};

struct CanceledException : std::exception
{
   const char * what() const noexcept override { return "Coroutine canceled"; }
};

template <TaskResult T>
struct TaskHandle
{
   using promise_type = internal::Promise<T>;
   using handle_type = stdcr::coroutine_handle<promise_type>;

   TaskHandle() noexcept;
   TaskHandle(TaskHandle && other) noexcept;
   explicit TaskHandle(promise_type & promise);
   ~TaskHandle();
   TaskHandle & operator=(TaskHandle && other) noexcept;
   explicit operator bool() const noexcept;

   bool await_ready() const noexcept;
   void await_suspend(stdcr::coroutine_handle<> h) noexcept;
   T await_resume();

   void Swap(TaskHandle & other) noexcept;

private:
   handle_type m_handle;
};

namespace internal {

struct DetachedPromise
{
   DetachedHandle get_return_object() const noexcept { return {}; }
   stdcr::suspend_never initial_suspend() const noexcept { return {}; }
   stdcr::suspend_never final_suspend() const noexcept { return {}; }
   void unhandled_exception() const noexcept { std::abort(); }
   void return_void() noexcept {}
};

template <TaskResult T>
struct ValueHolder
{
   template <typename U>
   void return_value(U && val)
   {
      value.template emplace<T>(std::forward<U>(val));
   }
   T RetrieveValue() { return std::get<T>(std::move(value)); }

   std::variant<std::monostate, T, std::exception_ptr> value;
};

template <>
struct ValueHolder<void>
{
   void return_void() const noexcept {}
   void RetrieveValue() const noexcept {}

   std::variant<std::monostate, std::exception_ptr> value;
};

template <TaskResult T>
struct Promise : ValueHolder<T>
{
   bool canceled = false;
   stdcr::coroutine_handle<> outerHandle = nullptr;

   void unhandled_exception() noexcept
   {
      this->value.template emplace<std::exception_ptr>(std::current_exception());
   }

   TaskHandle<T> get_return_object() { return TaskHandle<T>{*this}; }

   template <typename A>
   auto await_transform(A && awaiter)
   {
      static_assert(Awaiter<A>, "Can't wrap non-awaiter types");
      struct CancelingAwaitable
      {
         std::remove_reference_t<A> a;
         const Promise & p;

         bool await_ready() { return a.await_ready(); }
         void await_suspend(stdcr::coroutine_handle<> h) { a.await_suspend(h); }
         decltype(auto) await_resume()
         {
            if (p.canceled)
               throw CanceledException{};
            return a.await_resume();
         }
      };
      return CancelingAwaitable{std::forward<A>(awaiter), *this};
   }

   auto initial_suspend() noexcept { return stdcr::suspend_never{}; }

   auto final_suspend() noexcept
   {
      struct ResumingAwaitable
      {
         Promise & p;

         bool await_ready() const noexcept { return p.canceled; }
         void await_suspend(stdcr::coroutine_handle<>) noexcept
         {
            if (p.outerHandle)
               p.outerHandle.resume();
         }
         void await_resume() const noexcept {}
      };
      return ResumingAwaitable{*this};
   }
};

} // namespace internal

template <TaskResult T>
TaskHandle<T>::TaskHandle() noexcept
   : m_handle(nullptr)
{}

template <TaskResult T>
TaskHandle<T>::TaskHandle(TaskHandle && other) noexcept
   : TaskHandle()
{
   Swap(other);
}

template <TaskResult T>
TaskHandle<T>::TaskHandle(promise_type & promise)
   : m_handle(handle_type::from_promise(promise))
{}

template <TaskResult T>
TaskHandle<T>::~TaskHandle()
{
   if (!m_handle)
      return;

   if (m_handle.done()) {
      m_handle.destroy();
      return;
   }

   m_handle.promise().canceled = true;
}

template <TaskResult T>
TaskHandle<T> & TaskHandle<T>::operator=(TaskHandle && other) noexcept
{
   TaskHandle(std::move(other)).Swap(*this);
   return *this;
}

template <TaskResult T>
TaskHandle<T>::operator bool() const noexcept
{
   return m_handle && !m_handle.done();
}

template <TaskResult T>
bool TaskHandle<T>::await_ready() const noexcept
{
   return false;
}

template <TaskResult T>
void TaskHandle<T>::await_suspend(stdcr::coroutine_handle<> h) noexcept
{
   m_handle.promise().outerHandle = h;
}

template <TaskResult T>
T TaskHandle<T>::await_resume()
{
   if (std::holds_alternative<std::exception_ptr>(m_handle.promise().value))
      std::rethrow_exception(std::get<std::exception_ptr>(m_handle.promise().value));
   return m_handle.promise().RetrieveValue();
}

template <TaskResult T>
void TaskHandle<T>::Swap(TaskHandle & other) noexcept
{
   std::swap(m_handle, other.m_handle);
}

} // namespace cr

#endif
