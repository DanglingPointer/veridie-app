#ifndef BTINVOKER_HPP
#define BTINVOKER_HPP

#include <jni.h>

namespace jni {

class BtInvoker
{
public:
   BtInvoker(JNIEnv * env, jclass clazz);
   jboolean IsBluetoothEnabled();
   jint RequestPairedDevices();
   jint StartDiscovery();
   jint CancelDiscovery();
   jint RequestDiscoverability();
   jint StartListening(jstring name, jlong uuidLsl, jlong uuidMsl);
   jint StopListening();
   jint Connect(jstring remoteMac, jlong uuidLsl, jlong uuidMsl);
   jint CloseConnection(jstring remoteMac);
   jint SendMessage(jstring remoteMac, jbyteArray msg);

private:
   struct {
      jmethodID isBluetoothEnabled;
      jmethodID requestPairedDevices;
      jmethodID startDiscovery;
      jmethodID cancelDiscovery;
      jmethodID requestDiscoverability;
      jmethodID startListening;
      jmethodID stopListening;
      jmethodID connect;
      jmethodID closeConnection;
      jmethodID sendMessage;
   } m_methods;
   JNIEnv * m_env;
   jclass m_class;
};

} // namespace jni

#endif // BTINVOKER_HPP
