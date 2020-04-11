#include "jni/proxy.hpp"
#include "core/commands.hpp"
#include "core/logging.hpp"
#include "jni/exec.hpp"
#include "jni/cmdmanager.hpp"

namespace {

template <typename... Ts>
bool Contains(cmd::List<Ts...>, int32_t value)
{
   return ((Ts::ID == value) || ...);
}

class Proxy : public jni::IProxy
{
public:
   explicit Proxy(ILogger & logger)
      : m_logger(logger)
   {}

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
   void ForwardCommand(std::unique_ptr<cmd::ICommand> c) override
   {
      if (Contains(cmd::UiDictionary{}, c->GetId())) {
         ForwardCommandToUi(std::move(c));
         return;
      }
      if (Contains(cmd::BtDictionary{}, c->GetId())) {
         ForwardCommandToBt(std::move(c));
         return;
      }
      m_logger.Write<LogPriority::ERROR>("Proxy unable to auto-forward cmd:", c->GetId());
   }


private:
   ILogger & m_logger;
};


} // namespace


namespace jni {

std::unique_ptr<IProxy> CreateProxy(ILogger & logger)
{
   return std::make_unique<Proxy>(logger);
}

} // namespace jni