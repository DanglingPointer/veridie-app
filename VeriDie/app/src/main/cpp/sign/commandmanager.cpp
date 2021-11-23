#include "sign/commandmanager.hpp"
#include "sign/commands.hpp"
#include "utils/logger.hpp"

#undef NDEBUG
#include <cassert>

namespace cmd {
namespace {
constexpr int32_t INVALID_CMD_ID = 0;
}

struct Manager::CommandData
{
   stdcr::coroutine_handle<> callback = nullptr;
   int64_t response = ResponseCode::INVALID_STATE();
};

Manager::FutureResponse::FutureResponse(Manager & mgr, int32_t id) noexcept
   : m_mgr(mgr)
   , m_id(id)
{}

bool Manager::FutureResponse::await_ready() const noexcept
{
   return m_id == INVALID_CMD_ID;
}

void Manager::FutureResponse::await_suspend(stdcr::coroutine_handle<> h) const
{
   assert(m_mgr.m_pendingCmds.count(m_id) == 0);
   m_mgr.m_pendingCmds[m_id].callback = h;
}

int64_t Manager::FutureResponse::await_resume() const
{
   const auto it = m_mgr.m_pendingCmds.find(m_id);
   if (it == std::end(m_mgr.m_pendingCmds))
      return cmd::ResponseCode::INVALID_STATE();

   const int64_t response = it->second.response;
   m_mgr.m_pendingCmds.erase(it);
   return response;
}

Manager::Manager(ILogger & logger,
                 std::unique_ptr<IExternalInvoker> uiInvoker,
                 std::unique_ptr<IExternalInvoker> btInvoker)
   : m_log(logger)
   , m_uiInvoker(std::move(uiInvoker))
   , m_btInvoker(std::move(btInvoker))
{
   assert(m_uiInvoker);
   assert(m_btInvoker);
}

Manager::~Manager()
{
   for (auto it = m_pendingCmds.begin(); it != std::end(m_pendingCmds); it = m_pendingCmds.begin())
      if (auto & [_, data] = *it; data.callback)
         data.callback();
}

Manager::FutureResponse Manager::IssueUiCommand(mem::pool_ptr<ICommand> && cmd)
{
   return IssueCommand(std::move(cmd), *m_uiInvoker);
}

Manager::FutureResponse Manager::IssueBtCommand(mem::pool_ptr<ICommand> && cmd)
{
   return IssueCommand(std::move(cmd), *m_btInvoker);
}

Manager::FutureResponse Manager::IssueCommand(mem::pool_ptr<ICommand> && cmd,
                                              IExternalInvoker & invoker)
{
   int32_t id = cmd->GetId();
   while (m_pendingCmds.count(id))
      ++id;

   if ((id - cmd->GetId()) >= COMMAND_ID(1)) {
      m_log.Write<LogPriority::ERROR>("Command storage is full for ", cmd->GetName());
      return FutureResponse(*this, INVALID_CMD_ID);
   }

   if (!invoker.Invoke(std::move(cmd), id)) {
      m_log.Write<LogPriority::ERROR>("External Invoker failed");
      return FutureResponse(*this, INVALID_CMD_ID);
   }

   return FutureResponse(*this, id);
}

void Manager::SubmitResponse(int32_t cmdId, int64_t response)
{
   const auto it = m_pendingCmds.find(cmdId);
   if (it == std::end(m_pendingCmds)) {
      m_log.Write<LogPriority::WARN>("CommandManager received an orphaned response, ID =", cmdId);
      return;
   }
   if (!it->second.callback) {
      m_pendingCmds.erase(it);
      m_log.Write<LogPriority::WARN>("CommandManager received response to a detached command, ID =",
                                     cmdId);
      return;
   }
   it->second.response = response;
   it->second.callback();
}


} // namespace cmd
