#ifndef SIGN_COMMANDMANAGER_HPP
#define SIGN_COMMANDMANAGER_HPP

#include "utils/coroutine.hpp"
#include "utils/poolptr.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

class ILogger;

namespace cmd {
class ICommand;

class IExternalInvoker
{
public:
   virtual ~IExternalInvoker() = default;
   virtual bool Invoke(mem::pool_ptr<ICommand> && data, int32_t id) = 0;
};

class Manager
{
public:
   struct FutureResponse
   {
      FutureResponse(Manager & mgr, int32_t id) noexcept;
      bool await_ready() const noexcept;
      void await_suspend(stdcr::coroutine_handle<> h) const;
      int64_t await_resume() const;

   private:
      Manager & m_mgr;
      const int32_t m_id;
   };

   Manager(ILogger & logger,
           std::unique_ptr<IExternalInvoker> uiInvoker,
           std::unique_ptr<IExternalInvoker> btInvoker);
   ~Manager();

   FutureResponse IssueUiCommand(mem::pool_ptr<ICommand> && cmd);
   FutureResponse IssueBtCommand(mem::pool_ptr<ICommand> && cmd);

   void SubmitResponse(int32_t cmdId, int64_t response);

private:
   FutureResponse IssueCommand(mem::pool_ptr<ICommand> && cmd, IExternalInvoker & invoker);

   ILogger & m_log;
   const std::unique_ptr<IExternalInvoker> m_uiInvoker;
   const std::unique_ptr<IExternalInvoker> m_btInvoker;

   struct CommandData;
   std::unordered_map<int32_t, CommandData> m_pendingCmds;
};

} // namespace cmd

#endif // SIGN_COMMANDMANAGER_HPP
