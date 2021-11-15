#ifndef JNI_EXEC_HPP
#define JNI_EXEC_HPP

#include <experimental/coroutine>
#include <functional>
#include "utils/alwayscopyable.hpp"

namespace jni {
class ICmdManager;

void InternalExec(std::function<void(ICmdManager *)> task);

template <typename F>
void Exec(F && f)
{
   InternalExec(AlwaysCopyable(std::move(f)));
}

struct Scheduler
{
   bool await_ready() { return false; }
   void await_suspend(std::experimental::coroutine_handle<> h);
   ICmdManager * await_resume();

private:
   ICmdManager * mgr_ = nullptr;
};

} // namespace jni

#endif // JNI_EXEC_HPP
