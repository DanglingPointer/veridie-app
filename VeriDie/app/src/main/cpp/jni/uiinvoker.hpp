#ifndef JNI_UIINVOKER_HPP
#define JNI_UIINVOKER_HPP

#include <jni.h>

namespace jni {

class UiInvoker
{
public:
   UiInvoker(JNIEnv * env, jclass clazz);


private:
   struct
   {
      /* TODO */
   } m_methods;
   JNIEnv * m_env;
   jclass m_class;
};

} // namespace jni


#endif // JNI_UIINVOKER_HPP
