#include "jni/uiinvoker.hpp"

namespace jni {

UiInvoker::UiInvoker(JNIEnv * env, jclass clazz)
   : m_env(env)
   , m_class(clazz)
{}
/* TODO */
} // namespace jni