#ifndef CORE_EXEC_HPP
#define CORE_EXEC_HPP

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

} // namespace core

#endif // CORE_EXEC_HPP
