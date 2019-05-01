#ifndef INTEROP_HPP
#define INTEROP_HPP

#include <jni.h>

extern "C" {

JNIEXPORT jstring JNICALL Java_com_vasilyev_veridie_MainActivity_stringFromJNI(JNIEnv * env,
                                                                               jobject /* this */);


/*
 * Class:     com_vasilyev_veridie_MainActivity
 * Method:    receiveData
 * Signature: (Ljava/lang/String;[BI)V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_MainActivity_receiveData(JNIEnv *, jobject,
                                                                          jstring, jbyteArray,
                                                                          jint);


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * vm, void * reserved);

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM * vm, void * reserved);
}

#endif // INTEROP_HPP
