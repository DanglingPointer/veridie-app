#ifndef TASK_HPP
#define TASK_HPP

#include "utils/coroutine.hpp"

namespace cr {

struct EagerPromise;

struct DetachedHandle
{
   using promise_type = EagerPromise;
};

struct EagerPromise
{
   DetachedHandle get_return_object() const noexcept { return {}; }
   stdcr::suspend_never initial_suspend() const noexcept { return {}; }
   stdcr::suspend_never final_suspend() const noexcept { return {}; }
   void unhandled_exception() const noexcept { std::abort(); }
   void return_void() noexcept {}
};

} // namespace cr

#endif
