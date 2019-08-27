#ifndef JNI_UIINVOKER_HPP
#define JNI_UIINVOKER_HPP

#include <jni.h>

namespace jni {

class UiInvoker
{
public:
   UiInvoker(JNIEnv * env, jclass clazz);
   jint ShowToast(jstring message, jint seconds);
   jint ShowCandidates(jobjectArray names);
   jint ShowConnections(jobjectArray names);
   jint ShowCastResponse(jintArray result, jint successCount, jboolean external);
   jint ShowLocalName(jstring name);

private:
   struct {
      jmethodID showToast;
      jmethodID showCandidates;
      jmethodID showConnections;
      jmethodID showCastResponse;
      jmethodID showLocalName;
   } m_methods;
   JNIEnv * m_env;
   jclass m_class;
};

} // namespace jni


#endif // JNI_UIINVOKER_HPP
