#ifndef TASK_HPP
#define TASK_HPP

#include "utils/coroutine.hpp"

#include <concepts>
#include <exception>
#include <functional>
#include <type_traits>
#include <variant>

namespace cr {

struct InlineExecutor
{
   template <class F>
   void Execute(F && f) const noexcept
   {
      std::invoke(std::forward<F>(f));
   }
};

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

struct Callable
{
   void operator()() {}
};
} // namespace internal


// shamelessly copied from ref impl
template <typename T>
concept Awaiter = requires(T && awaiter, stdcr::coroutine_handle<void> h) {
   static_cast<bool>(awaiter.await_ready());
   awaiter.await_resume();
   requires internal::AwaitSuspendReturnType<decltype(awaiter.await_suspend(h))>;
};

template <typename E>
concept Executor =
   std::is_default_constructible_v<E> &&
   std::is_copy_constructible_v<E> &&
   requires (E && e, internal::Callable f) {
      std::forward<E>(e).Execute(f);
      std::forward<E>(e).Execute(std::move(f));
   };


namespace internal {

struct DetachedPromise;

template <TaskResult T, Executor E>
struct Promise;
} // namespace internal


struct DetachedHandle
{
   using promise_type = internal::DetachedPromise;
};

struct CanceledException : std::exception
{
   const char * what() const noexcept override { return "Coroutine canceled"; }
};

template <TaskResult T, Executor E = InlineExecutor>
struct TaskHandle
{
   using promise_type = internal::Promise<T, E>;
   using handle_type = stdcr::coroutine_handle<promise_type>;

   TaskHandle() noexcept;
   TaskHandle(TaskHandle && other) noexcept;
   explicit TaskHandle(promise_type & promise);
   ~TaskHandle();
   TaskHandle & operator=(TaskHandle && other) noexcept;

   explicit operator bool() const noexcept;
   auto Run(E executor = {});
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
   template <Awaiter A>
   decltype(auto) await_transform(A && a) { return std::forward<A>(a); }
   template <TaskResult T, Executor E>
   auto await_transform(TaskHandle<T, E> && handle) { return handle.Run(); }
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

template <TaskResult T, Executor E>
struct Promise : public ValueHolder<T>, public E
{
   template <Awaiter A>
   struct CancelingAwaiter : A
   {
      const Promise & p;
      decltype(auto) await_resume()
      {
         if (p.canceled)
            throw CanceledException{};
         return A::await_resume();
      }
   };

   bool canceled = false;
   stdcr::coroutine_handle<> outerHandle = nullptr;

   const E & GetExecutor() const noexcept { return static_cast<const E &>(*this); }
   E & GetExecutor() noexcept { return static_cast<E &>(*this); }

   void unhandled_exception() noexcept
   {
      this->value.template emplace<std::exception_ptr>(std::current_exception());
   }

   TaskHandle<T, E> get_return_object() { return TaskHandle<T, E>{*this}; }

   template <Awaiter A>
   auto await_transform(A && awaiter) const
   {
      return CancelingAwaiter<std::remove_reference_t<A>>{std::forward<A>(awaiter), *this};
   }

   template <TaskResult R>
   auto await_transform(TaskHandle<R, E> && innerTask) const
   {
      using InnerAwaiter = std::remove_reference_t<decltype(innerTask.Run())>;
      return CancelingAwaiter<InnerAwaiter>{innerTask.Run(GetExecutor()), *this};
   }

   auto initial_suspend() noexcept
   {
      struct CancelingSuspendAlways
      {
         const Promise & p;
         bool await_ready() const noexcept { return false; }
         void await_suspend(stdcr::coroutine_handle<>) const noexcept {}
         void await_resume() const
         {
            if (p.canceled)
               throw CanceledException{};
         }
      };
      return CancelingSuspendAlways{*this};
   }

   auto final_suspend() noexcept
   {
      struct ResumingAwaitable
      {
         Promise & p;

         bool await_ready() const noexcept { return p.canceled; }
         void await_suspend(stdcr::coroutine_handle<>) noexcept
         {
            if (p.outerHandle)
               p.Execute(p.outerHandle);
         }
         void await_resume() const noexcept {}
      };
      return ResumingAwaitable{*this};
   }
};

} // namespace internal

template <TaskResult T, Executor E>
TaskHandle<T, E>::TaskHandle() noexcept
   : m_handle(nullptr)
{}

template <TaskResult T, Executor E>
TaskHandle<T, E>::TaskHandle(TaskHandle && other) noexcept
   : TaskHandle()
{
   Swap(other);
}

template <TaskResult T, Executor E>
TaskHandle<T, E>::TaskHandle(promise_type & promise)
   : m_handle(handle_type::from_promise(promise))
{}

template <TaskResult T, Executor E>
TaskHandle<T, E>::~TaskHandle()
{
   if (!m_handle)
      return;

   if (m_handle.done()) {
      m_handle.destroy();
      return;
   }

   m_handle.promise().canceled = true;
}

template <TaskResult T, Executor E>
TaskHandle<T, E> & TaskHandle<T, E>::operator=(TaskHandle && other) noexcept
{
   TaskHandle(std::move(other)).Swap(*this);
   return *this;
}

template <TaskResult T, Executor E>
TaskHandle<T, E>::operator bool() const noexcept
{
   return m_handle && !m_handle.done();
}

template <TaskResult T, Executor E>
auto TaskHandle<T, E>::Run(E executor)
{
   m_handle.promise().GetExecutor() = executor;
   m_handle.promise().GetExecutor().Execute(m_handle);

   struct Awaiter
   {
      handle_type handle;

      bool await_ready() const noexcept { return handle.done(); }
      void await_suspend(stdcr::coroutine_handle<> h) { handle.promise().outerHandle = h; }
      T await_resume()
      {
         if (std::holds_alternative<std::exception_ptr>(handle.promise().value))
            std::rethrow_exception(std::get<std::exception_ptr>(handle.promise().value));
         return handle.promise().RetrieveValue();
      }
   };
   return Awaiter{m_handle};
}

template <TaskResult T, Executor E>
void TaskHandle<T, E>::Swap(TaskHandle & other) noexcept
{
   std::swap(m_handle, other.m_handle);
}

} // namespace cr

#endif
