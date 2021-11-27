#include "jni/javainvoker.hpp"
#include "jni/exec.hpp"
#include "sign/cmd.hpp"
#include "sign/commandmanager.hpp"

namespace jni {

JavaInvoker::JavaInvoker(JNIEnv & env, jclass localRef, std::string_view methodName)
   : m_env(env)
   , m_class(localRef)
{
   m_method = m_env.GetStaticMethodID(m_class, methodName.data(), "(I[Ljava/lang/String;)V");
   if (!m_method) {
      // Log::Fatal("JavaInvoker could not obtain %s method id", methodName.data()); // TODO
      std::abort();
   }
}

JavaInvoker::~JavaInvoker()
{
   m_env.DeleteGlobalRef(m_class);
}

std::unique_ptr<cmd::IExternalInvoker> JavaInvoker::GetExternalInvoker()
{
   struct ExternalInvoker : cmd::IExternalInvoker
   {
      ExternalInvoker(const std::shared_ptr<JavaInvoker> & parent)
         : parent(parent)
      {}
      bool Invoke(mem::pool_ptr<cmd::ICommand> && cmd, int32_t argId) override
      {
         auto javaInvoker = parent.lock();
         if (!javaInvoker)
            return false;

         Exec([=, cmd = std::move(cmd)]() mutable {
            javaInvoker->PassCommand(std::move(cmd), argId);
         });
         return true;
      }
      std::weak_ptr<JavaInvoker> parent;
   };
   return std::make_unique<ExternalInvoker>(shared_from_this());
}

void JavaInvoker::PassCommand(mem::pool_ptr<cmd::ICommand> && cmd, int32_t argId)
{
   jobjectArray argsArray = nullptr;
   const size_t argCount = cmd->GetArgsCount();
   if (argCount > 0) {
      jclass stringClass = m_env.FindClass("java/lang/String");
      argsArray = m_env.NewObjectArray(argCount, stringClass, nullptr);
      for (size_t i = 0; i < argCount; ++i) {
         jstring arg = m_env.NewStringUTF(cmd->GetArgAt(i).data());
         m_env.SetObjectArrayElement(argsArray, i, arg);
         m_env.DeleteLocalRef(arg);
      }
      m_env.DeleteLocalRef(stringClass);
   }

   m_env.CallStaticVoidMethod(m_class, m_method, argId, argsArray);

   if (m_env.ExceptionCheck() == JNI_TRUE) {
      // Log::Warning("JavaInvoker caught an exception!"); // TODO
      m_env.ExceptionDescribe();
      m_env.ExceptionClear();
   }

   if (argsArray)
      m_env.DeleteLocalRef(argsArray);
}

} // namespace jni