#ifndef JNI_EXEC_HPP
#define JNI_EXEC_HPP

#include <string>
#include <jni.h>

class ILogger;

namespace jni {

class BtInvoker;
class UiInvoker;

class ServiceLocator
{
public:
   virtual ~ServiceLocator() = default;
   virtual JavaVM * GetJavaVM() = 0;
   virtual JNIEnv * GetJNIEnv() = 0;
   virtual ILogger & GetLogger() = 0;
   virtual BtInvoker * GetBtInvoker() = 0;
   virtual UiInvoker * GetUiInvoker() = 0;
};

void Exec(std::function<void(jni::ServiceLocator *)> task);

std::string ErrorToString(jint error);

}

#endif // JNI_EXEC_HPP
