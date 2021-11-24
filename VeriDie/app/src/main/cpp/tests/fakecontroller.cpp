#undef NDEBUG
#include <cassert>
#include <vector>
#include <sstream>

#include "core/controller.hpp"
#include "utils/logger.hpp"
#include "utils/task.hpp"
#include "sign/cmd.hpp"
#include "sign/commandmanager.hpp"
#include "sign/commandpool.hpp"

namespace dice {
std::unique_ptr<IEngine> CreateUniformEngine()
{
   return nullptr;
}
} // namespace dice
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
   {}
   int32_t GetId() const override { return id; }
   std::string_view GetName() const override { return "TestCommand"; }
   size_t GetArgsCount() const override { return args.size(); }
   std::string_view GetArgAt(size_t index) const override { return args[index]; }

   int32_t id;
   std::vector<std::string> args;
};

class EchoController
   : public core::IController
{
public:
   EchoController(ILogger & logger)
      : m_logger(logger)
      , m_cmdManager(nullptr)
   {}
   void Start(std::unique_ptr<cmd::IExternalInvoker> uiInvoker,
              std::unique_ptr<cmd::IExternalInvoker> btInvoker) override
   {
      m_cmdManager =
         std::make_unique<cmd::Manager>(m_logger, std::move(uiInvoker), std::move(btInvoker));
   }
   void OnEvent(int32_t eventId, const std::vector<std::string> & args) override
   {
      std::ostringstream ss;
      for (const auto & s : args)
         ss << "[" << s << "] ";
      m_logger.Write<LogPriority::DEBUG>("EchoController received event Id:",
                                         eventId,
                                         "Args:",
                                         ss.str());
      SendCommmandAndVerifyResponse(cmd::pool.MakeUnique<TestCommand>((eventId << 8), args));
   }
   void OnCommandResponse(int32_t cmdId, int64_t response) override
   {
      ASSERT(m_cmdManager);
      m_cmdManager->SubmitResponse(cmdId, response);
   }

private:
   cr::DetachedHandle SendCommmandAndVerifyResponse(mem::pool_ptr<TestCommand> cmd)
   {
      const int64_t response = co_await m_cmdManager->IssueUiCommand(std::move(cmd));
      ASSERT(response == cmd::ICommand::OK);
   }
   ILogger & m_logger;
   std::unique_ptr<cmd::Manager> m_cmdManager;
};

} // namespace

namespace core {

std::unique_ptr<IController> CreateController(std::unique_ptr<dice::IEngine> /*engine*/,
                                              std::unique_ptr<async::Timer> /*timer*/,
                                              std::unique_ptr<dice::ISerializer> /*serializer*/,
                                              ILogger & logger)
{
   return std::make_unique<EchoController>(logger);
}
} // namespace core
