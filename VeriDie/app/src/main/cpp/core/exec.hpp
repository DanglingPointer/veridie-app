#ifndef CORE_EXEC_HPP
#define CORE_EXEC_HPP

#include <functional>
#include "utils/alwayscopyable.hpp"

namespace main {
class IController;

void InternalExec(std::function<void(IController *)> task);

template <typename F>
void Exec(F && f)
{
   InternalExec(AlwaysCopyable(std::move(f)));
}

} // namespace main

#endif // CORE_EXEC_HPP
