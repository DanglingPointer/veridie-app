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

   void ForwardCommandToUi(std::unique_ptr<cmd::ICommand> c) override
   {
      jni::Exec([cmd = std::move(c)] (jni::ICmdManager * mgr) mutable {
         mgr->IssueUiCommand(std::move(cmd));
      });
   }
   void ForwardCommandToBt(std::unique_ptr<cmd::ICommand> c) override
   {
      jni::Exec([cmd = std::move(c)] (jni::ICmdManager * mgr) mutable {
         mgr->IssueBtCommand(std::move(cmd));
      });
   }

private:
   ILogger & m_logger;
};


} // namespace


namespace jni {

std::unique_ptr<core::Proxy> CreateProxy(ILogger & logger)
{
   return std::make_unique<Proxy>(logger);
}

} // namespace jni