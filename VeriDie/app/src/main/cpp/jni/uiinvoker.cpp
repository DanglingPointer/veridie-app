#include <stdexcept>
#include <string>
#include "jni/uiinvoker.hpp"

namespace jni {

UiInvoker::UiInvoker(JNIEnv * env, jclass clazz)
   : m_env(env)
   , m_class(clazz)
{
#define GET_METHOD(name, signature) m_methods.name = env->GetStaticMethodID(clazz, #name, signature)

   GET_METHOD(showToast, "(Ljava/lang/String;I)I");
   GET_METHOD(showCandidates, "([Ljava/lang/String;)I");
   GET_METHOD(showConnections, "([Ljava/lang/String;)I");
   GET_METHOD(showCastResponse, "([IIZ)I");
   GET_METHOD(showLocalName, "(Ljava/lang/String;)I");

#undef GET_METHOD
   const jmethodID * const begin = &m_methods.showToast;
   const jmethodID * const end = &m_methods.showLocalName + 1;
   for (const jmethodID * p = begin; p != end; ++p) {
      if (*p == nullptr) {
         throw std::runtime_error("Failed to obtain UiBridge method " + std::to_string(p - begin));
      }
   }
}
UiInvoker::~UiInvoker()
{
   m_env->DeleteGlobalRef(m_class);
}
jint UiInvoker::ShowToast(jstring message, jint seconds)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.showToast, message, seconds);
}
jint UiInvoker::ShowCandidates(jobjectArray names)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.showCandidates, names);
}
jint UiInvoker::ShowConnections(jobjectArray names)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.showConnections, names);
}
jint UiInvoker::ShowCastResponse(jintArray result, jint successCount, jboolean external)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.showCastResponse, result, successCount,
                                     external);
}
jint UiInvoker::ShowLocalName(jstring name)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.showLocalName, name);
}

} // namespace jni