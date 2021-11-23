#ifndef CORE_COMMANDADAPTER_HPP
#define CORE_COMMANDADAPTER_HPP

#include "utils/task.hpp"
#include "utils/poolptr.hpp"
#include "sign/commandpool.hpp"

namespace cmd {
class Manager;
}

namespace core {

class CommandAdapter
{
public:
   explicit CommandAdapter(cmd::Manager & manager)
      : m_manager(manager)
   {}

   template <typename TCmd, typename... TArgs>
   cr::TaskHandle<int64_t> UiCommand(TArgs &&... args)
   {
      auto pcmd = cmd::pool.MakeUnique<TCmd>(std::forward<TArgs>(args)...);
      return ForwardUiCommand(std::move(pcmd));
   }

   template <typename TCmd, typename... TArgs>
   cr::TaskHandle<int64_t> BtCommand(TArgs &&... args)
   {
      auto pcmd = cmd::pool.MakeUnique<TCmd>(std::forward<TArgs>(args)...);
      return ForwardBtCommand(std::move(pcmd));
   }

private:
   cr::TaskHandle<int64_t> ForwardUiCommand(mem::pool_ptr<cmd::ICommand> && cmd);
   cr::TaskHandle<int64_t> ForwardBtCommand(mem::pool_ptr<cmd::ICommand> && cmd);

   cmd::Manager & m_manager;
};

} // namespace core

#endif
