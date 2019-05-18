#include <stdexcept>
#include <string>
#include "jni/btinvoker.hpp"

namespace jni {

BtInvoker::BtInvoker(JNIEnv * env, jclass clazz)
   : m_env(env)
   , m_class(clazz)
{
   m_methods.isBluetoothEnabled = env->GetStaticMethodID(clazz, "isBluetoothEnabled", "()Z");
   m_methods.requestPairedDevices = env->GetStaticMethodID(clazz, "requestPairedDevices", "()I");
   m_methods.startDiscovery = env->GetStaticMethodID(clazz, "startDiscovery", "()I");
   m_methods.cancelDiscovery = env->GetStaticMethodID(clazz, "cancelDiscovery", "()I");
   m_methods.requestDiscoverability =
      env->GetStaticMethodID(clazz, "requestDiscoverability", "(I)I");
   m_methods.startListening =
      env->GetStaticMethodID(clazz, "startListening", "(Ljava/lang/String;JJ)I");
   m_methods.stopListening = env->GetStaticMethodID(clazz, "stopListening", "()I");
   m_methods.connect = env->GetStaticMethodID(clazz, "connect", "(Ljava/lang/String;JJ)I");
   m_methods.closeConnection =
      env->GetStaticMethodID(clazz, "closeConnection", "(Ljava/lang/String;)I");
   m_methods.sendMessage = env->GetStaticMethodID(clazz, "sendMessage", "(Ljava/lang/String;[B)I");

   const jmethodID * const begin = &m_methods.isBluetoothEnabled;
   const jmethodID * const end = &m_methods.sendMessage + 1;
   for (const jmethodID * p = begin; p != end; ++p) {
      if (*p == nullptr) {
         throw std::runtime_error("Failed to obtain BluetoothBridge method " + std::to_string(p - begin));
      }
   }
}
jboolean BtInvoker::IsBluetoothEnabled()
{
   return m_env->CallStaticBooleanMethod(m_class, m_methods.isBluetoothEnabled);
}
jint BtInvoker::RequestPairedDevices()
{
   return m_env->CallStaticIntMethod(m_class, m_methods.requestPairedDevices);
}
jint BtInvoker::StartDiscovery()
{
   return m_env->CallStaticIntMethod(m_class, m_methods.startDiscovery);
}
jint BtInvoker::CancelDiscovery()
{
   return m_env->CallStaticIntMethod(m_class, m_methods.cancelDiscovery);
}
jint BtInvoker::RequestDiscoverability()
{
   return m_env->CallStaticIntMethod(m_class, m_methods.requestDiscoverability);
}
jint BtInvoker::StartListening(jstring name, jlong uuidLsl, jlong uuidMsl)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.startListening, name, uuidLsl, uuidMsl);
}
jint BtInvoker::StopListening()
{
   return m_env->CallStaticIntMethod(m_class, m_methods.stopListening);
}
jint BtInvoker::Connect(jstring remoteMac, jlong uuidLsl, jlong uuidMsl)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.connect, remoteMac, uuidLsl, uuidMsl);
}
jint BtInvoker::CloseConnection(jstring remoteMac)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.closeConnection, remoteMac);
}
jint BtInvoker::SendMessage(jstring remoteMac, jbyteArray msg)
{
   return m_env->CallStaticIntMethod(m_class, m_methods.sendMessage, remoteMac, msg);
}

} // namespace jni