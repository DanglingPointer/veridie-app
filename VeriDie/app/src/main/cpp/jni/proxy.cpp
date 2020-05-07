#include <unordered_map>
#include <sstream>

#include "jni/proxy.hpp"
#include "sign/commands.hpp"
#include "core/logging.hpp"
#include "jni/exec.hpp"
#include "jni/cmdmanager.hpp"

namespace {

class Proxy : public core::Proxy
{
public:
   explicit Proxy(ILogger & logger)
      : m_logger(logger)
   {
      (void)m_logger;
   }

   void ForwardCommandToUi(mem::pool_ptr<cmd::ICommand> c) override
   {
      LogCommand(c);
      jni::Exec([cmd = std::move(c)](jni::ICmdManager * mgr) mutable {
         mgr->IssueUiCommand(std::move(cmd));
      });
   }
   void ForwardCommandToBt(mem::pool_ptr<cmd::ICommand> c) override
   {
      LogCommand(c);
      jni::Exec([cmd = std::move(c)](jni::ICmdManager * mgr) mutable {
         mgr->IssueBtCommand(std::move(cmd));
      });
   }

private:
   void LogCommand(const mem::pool_ptr<cmd::ICommand> & cmd)
   {
      std::ostringstream ss;
      ss << ">>>>> " << cmd->GetName();
      for (size_t i = 0; i < cmd->GetArgsCount(); ++i)
         ss << " [" << cmd->GetArgAt(i) << "]";
      m_logger.Write(LogPriority::INFO, ss.str());
   }

   ILogger & m_logger;
};


} // namespace


namespace jni {

std::unique_ptr<core::Proxy> CreateProxy(ILogger & logger)
{
   return std::make_unique<Proxy>(logger);
}

} // namespace jni