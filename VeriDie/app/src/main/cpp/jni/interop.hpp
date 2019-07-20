#ifndef JNI_INTEROP_HPP
#define JNI_INTEROP_HPP

#include <jni.h>

extern "C" {

#undef com_vasilyev_veridie_interop_BluetoothBridge_NO_ERROR
#define com_vasilyev_veridie_interop_BluetoothBridge_NO_ERROR 0L
#undef com_vasilyev_veridie_interop_BluetoothBridge_ERROR_NO_LISTENER
#define com_vasilyev_veridie_interop_BluetoothBridge_ERROR_NO_LISTENER 1L
#undef com_vasilyev_veridie_interop_BluetoothBridge_ERROR_UNHANDLED_EXCEPTION
#define com_vasilyev_veridie_interop_BluetoothBridge_ERROR_UNHANDLED_EXCEPTION 2L
#undef com_vasilyev_veridie_interop_BluetoothBridge_ERROR_WRONG_STATE
#define com_vasilyev_veridie_interop_BluetoothBridge_ERROR_WRONG_STATE 3L

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    bridgeCreated
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_bridgeCreated
    (JNIEnv *env, jclass clazz);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    bluetoothOn
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_bluetoothOn(JNIEnv * env,
                                                                                     jclass clazz);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    bluetoothOff
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_bluetoothOff(JNIEnv * env,
                                                                                      jclass clazz);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    deviceFound
 * Signature: (Ljava/lang/String;Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_deviceFound(
   JNIEnv * env, jclass clazz, jstring name, jstring mac, jboolean paired);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    discoverabilityConfirmed
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_discoverabilityConfirmed(
   JNIEnv * env, jclass clazz);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    discoverabilityRejected
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_discoverabilityRejected(
   JNIEnv * env, jclass clazz);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    scanModeChanged
 * Signature: (ZZ)V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_scanModeChanged(
   JNIEnv * env, jclass clazz, jboolean discoverable, jboolean connectable);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    deviceConnected
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_deviceConnected(
   JNIEnv * env, jclass clazz, jstring name, jstring mac);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    deviceDisonnected
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_deviceDisonnected(
   JNIEnv * env, jclass clazz, jstring mac);

/*
 * Class:     com_vasilyev_veridie_interop_BluetoothBridge
 * Method:    messageReceived
 * Signature: (Ljava/lang/String;[BI)V
 */
JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_messageReceived(
   JNIEnv * env, jclass clazz, jstring srcMac, jbyteArray dataArr, jint length);


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * vm, void * reserved);

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM * vm, void * reserved);
}

#endif // JNI_INTEROP_HPP
