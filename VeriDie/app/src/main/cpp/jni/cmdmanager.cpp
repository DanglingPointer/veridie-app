#include <unordered_map>

#include "sign/commands.hpp"
#include "jni/cmdmanager.hpp"
#include "core/logging.hpp"
#include "core/exec.hpp"

namespace {

class CmdManager : public jni::ICmdManager
{
public:
   CmdManager(ILogger & logger, JNIEnv * env, jclass globalRef)
      : m_logger(logger)
      , m_env(env)
      , m_class(globalRef)
   {
      m_receiveUiCommand =
         m_env->GetStaticMethodID(m_class, "receiveUiCommand", "(I[Ljava/lang/String;)V");
      if (!m_receiveUiCommand) {
         m_logger.Write<LogPriority::FATAL>(
            "CmdManager could not obtain 'receiveUiCommand' method id");
         std::abort();
      }
      m_receiveBtCommand =
         m_env->GetStaticMethodID(m_class, "receiveBtCommand", "(I[Ljava/lang/String;)V");
      if (!m_receiveBtCommand) {
         m_logger.Write<LogPriority::FATAL>(
            "CmdManager could not obtain 'receiveBtCommand' method id");
         std::abort();
      }
   }
   ~CmdManager() { m_env->DeleteGlobalRef(m_class); }

   void IssueUiCommand(mem::pool_ptr<cmd::ICommand> c) override
   {
      IssueCommand(std::move(c), m_receiveUiCommand);
   }
   void IssueBtCommand(mem::pool_ptr<cmd::ICommand> c) override
   {
      IssueCommand(std::move(c), m_receiveBtCommand);
   }
   void OnCommandResponse(int32_t cmdId, int64_t response) override
   {
      auto it = m_waitingCmds.find(cmdId);
      if (it == std::end(m_waitingCmds)) {
         m_logger.Write<LogPriority::WARN>("CmdManager received orphaned response, cmdId =", cmdId);
         return;
      }
      core::Exec([cmd = std::move(it->second), response](auto) {
         cmd->Respond(response);
      });
      m_waitingCmds.erase(it);
   }

private:
   void IssueCommand(mem::pool_ptr<cmd::ICommand> c, jmethodID method)
   {
      jobjectArray argsArray = nullptr;
      const size_t argCount = c->GetArgsCount();
      if (argCount > 0) {
         jclass stringClass = m_env->FindClass("java/lang/String");
         argsArray = m_env->NewObjectArray(argCount, stringClass, nullptr);
         for (size_t i = 0; i < argCount; ++i)
            m_env->SetObjectArrayElement(argsArray, i, m_env->NewStringUTF(c->GetArgAt(i).data()));
      }

      jint argId = c->GetId();
      while (m_waitingCmds.count(argId))
         ++argId;
      m_waitingCmds.emplace(argId, std::move(c));

      m_env->CallStaticVoidMethod(m_class, method, argId, argsArray);

      if (m_env->ExceptionCheck() == JNI_TRUE) {
         m_logger.Write<LogPriority::WARN>("CmdManager caught an exception!");
         m_env->ExceptionDescribe();
         m_env->ExceptionClear();
      }

      if (argsArray) {
         for (size_t i = 0; i < argCount; ++i)
            m_env->DeleteLocalRef(m_env->GetObjectArrayElement(argsArray, i));
         m_env->DeleteLocalRef(argsArray);
      }
   }

   ILogger & m_logger;
   JNIEnv * m_env;
   jclass m_class;

   jmethodID m_receiveUiCommand;
   jmethodID m_receiveBtCommand;

   std::unordered_map<int32_t, mem::pool_ptr<cmd::ICommand>> m_waitingCmds;
};

} // namespace

namespace jni {

std::unique_ptr<ICmdManager> CreateCmdManager(ILogger & logger, JNIEnv * env, jclass globalRef)
{
   return std::make_unique<CmdManager>(logger, env, globalRef);
}

} // namespace jni