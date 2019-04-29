#include <jni.h>
#include <string>
#include "dice/cast.hpp"

extern "C" JNIEXPORT jstring JNICALL Java_com_vasilyev_veridie_MainActivity_stringFromJNI(JNIEnv * env,
                                                                                          jobject /* this */)
{
   std::string hello = "Hello from C++";
   return env->NewStringUTF(hello.c_str());
}
