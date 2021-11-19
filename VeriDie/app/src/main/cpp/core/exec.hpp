#ifndef CORE_EXEC_HPP
#define CORE_EXEC_HPP

#include <experimental/coroutine>
#include <functional>
#include "utils/alwayscopyable.hpp"

namespace core {
class IController;

void InternalExec(std::function<void(IController *)> task);

template <typename F>
void Exec(F && f)
{
   InternalExec(AlwaysCopyable(std::move(f)));
}

struct Scheduler
{
   bool await_ready() const noexcept { return false; }
   void await_suspend(std::experimental::coroutine_handle<> h);
   core::IController * await_resume() const noexcept;

private:
   core::IController * m_ctrl = nullptr;
};

} // namespace core

#endif // CORE_EXEC_HPP
