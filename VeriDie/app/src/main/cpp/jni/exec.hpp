#ifndef JNI_EXEC_HPP
#define JNI_EXEC_HPP

#include <string>
#include <jni.h>

namespace jni {

void Exec(std::function<void(JNIEnv *)> task);

std::string ErrorToString(jint error);

}

#endif // JNI_EXEC_HPP
