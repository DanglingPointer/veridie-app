#include <unordered_map>
#include <sstream>

#include "jni/proxy.hpp"
#include "sign/commands.hpp"
#include "core/logging.hpp"
#include "jni/exec.hpp"
#include "jni/cmdmanager.hpp"

namespace {

template <typename... Ts>
const std::unordered_map<int32_t, std::string_view> & Names(cmd::List<Ts...>)
{
   static std::unordered_map<int32_t, std::string_view> mapping{{Ts::ID, cmd::NameOf<Ts>()}...};
   return mapping;
}

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
      LogCommand(Names(cmd::UiDictionary{}), c);
      jni::Exec([cmd = std::move(c)](jni::ICmdManager * mgr) mutable {
         mgr->IssueUiCommand(std::move(cmd));
      });
   }
   void ForwardCommandToBt(std::unique_ptr<cmd::ICommand> c) override
   {
      LogCommand(Names(cmd::BtDictionary{}), c);
      jni::Exec([cmd = std::move(c)](jni::ICmdManager * mgr) mutable {
         mgr->IssueBtCommand(std::move(cmd));
      });
   }

private:
   void LogCommand(const std::unordered_map<int32_t, std::string_view> & idToName,
                   const std::unique_ptr<cmd::ICommand> & cmd)
   {
      std::string_view name = "Unknown";
      if (auto it = idToName.find(cmd->GetId()); it != std::cend(idToName))
         name = it->second;
      std::ostringstream ss;
      ss << ">>>>> " << name;
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