#ifndef JNI_EXEC_HPP
#define JNI_EXEC_HPP

#include <functional>
#include "utils/alwayscopyable.hpp"

namespace jni {
class ICmdManager;

void InternalExec(std::function<void()> task);

template <typename F>
void Exec(F && f)
{
   InternalExec(AlwaysCopyable(std::move(f)));
}

} // namespace jni

#endif // JNI_EXEC_HPP
