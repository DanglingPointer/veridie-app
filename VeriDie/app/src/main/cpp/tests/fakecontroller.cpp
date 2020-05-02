#include <vector>
#include <sstream>

#include "core/controller.hpp"
#include "core/logging.hpp"
#include "core/timerengine.hpp"
#include "sign/commands.hpp"
#include "core/proxy.hpp"
#include "utils/canceller.hpp"

namespace dice {
std::unique_ptr<IEngine> CreateUniformEngine()
{
   return nullptr;
}
} // namespace dice
namespace core {
std::unique_ptr<ITimerEngine> CreateTimerEngine(async::Executor)
{
   return nullptr;
}
} // namespace core
namespace dice {
std::unique_ptr<dice::ISerializer> CreateXmlSerializer()
{
   return nullptr;
}
} // namespace dice

namespace {

#define ASSERT(cond)  \
   if (!(cond)) {     \
      assert((cond)); \
      std::abort();   \
   }

struct TestCommand : public cmd::ICommand
{
   TestCommand(int32_t id, std::vector<std::string> args)
      : id(id)
      , args(std::move(args))
      , responded(false)
   {}
   ~TestCommand() { ASSERT(responded); }
   int32_t GetId() const override { return id; }
   size_t GetArgsCount() const override { return args.size(); }
   std::string_view GetArgAt(size_t index) const override { return args[index]; }
   void Respond(int64_t response) override
   {
      responded = true;
      ASSERT(response == 0);
   }

   int32_t id;
   std::vector<std::string> args;
   bool responded;
};

class EchoController
   : public core::IController
   , private async::Canceller<>
{
public:
   EchoController(ILogger & logger, std::unique_ptr<core::Proxy> proxy)
      : m_logger(logger)
      , m_proxy(std::move(proxy))
   {}
   void OnEvent(int32_t eventId, const std::vector<std::string> & args) override
   {
      std::ostringstream ss;
      for (const auto & s : args)
         ss << "[" << s << "] ";
      m_logger.Write<LogPriority::DEBUG>("EchoController received event Id:",
                                         eventId,
                                         "Args:",
                                         ss.str());
      m_proxy->ForwardToUi<TestCommand>((eventId << 8), args);
   }

private:
   ILogger & m_logger;
   std::unique_ptr<core::Proxy> m_proxy;
};

} // namespace

namespace core {

std::unique_ptr<IController> CreateController(std::unique_ptr<core::Proxy> proxy,
                                              std::unique_ptr<dice::IEngine> /*engine*/,
                                              std::unique_ptr<core::ITimerEngine> /*timer*/,
                                              std::unique_ptr<dice::ISerializer> /*serializer*/,
                                              ILogger & logger)
{
   return std::make_unique<EchoController>(logger, std::move(proxy));
}
} // namespace core
