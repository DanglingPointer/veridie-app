#include "core/commandadapter.hpp"
#include "sign/commandmanager.hpp"

#include <sstream>

namespace core {
namespace {

void LogCommand(const mem::pool_ptr<cmd::ICommand> & cmd)
{
   std::ostringstream ss;
   ss << ">>>>> " << cmd->GetName();
   for (size_t i = 0; i < cmd->GetArgsCount(); ++i)
      ss << " [" << cmd->GetArgAt(i) << "]";
   // Log::Info("%s", ss.str().c_str()); // TODO
}

} // namespace

cr::TaskHandle<int64_t> CommandAdapter::ForwardUiCommand(mem::pool_ptr<cmd::ICommand> && cmd)
{
   LogCommand(cmd);
   co_return co_await m_manager.IssueUiCommand(std::move(cmd));
}

cr::TaskHandle<int64_t> CommandAdapter::ForwardBtCommand(mem::pool_ptr<cmd::ICommand> && cmd)
{
   LogCommand(cmd);
   co_return co_await m_manager.IssueBtCommand(std::move(cmd));
}

} // namespace core
