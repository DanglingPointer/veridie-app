#include <stdexcept>
#include <string>
#include "jni/btinvoker.hpp"

namespace jni {

BtInvoker::BtInvoker(JNIEnv * env, jclass clazz)
   : m_env(env)
   , m_class(clazz)
{
#define GET_METHOD(name, signature) m_methods.name = env->GetStaticMethodID(clazz, #name, signature)

   GET_METHOD(isBluetoothEnabled, "()Z");
   GET_METHOD(requestPairedDevices, "()I");
   GET_METHOD(startDiscovery, "()I");
   GET_METHOD(cancelDiscovery, "()I");
   GET_METHOD(requestDiscoverability, "(I)I");
   GET_METHOD(startListening, "(Ljava/lang/String;JJ)I");
   GET_METHOD(stopListening, "()I");
   GET_METHOD(connect, "(Ljava/lang/String;JJ)I");
   GET_METHOD(closeConnection, "(Ljava/lang/String;)I");
   GET_METHOD(sendMessage, "(Ljava/lang/String;[B)I");

#undef GET_METHOD
   const jmethodID * const begin = &m_methods.isBluetoothEnabled;
   const jmethodID * const end = &m_methods.sendMessage + 1;
   for (const jmethodID * p = begin; p != end; ++p) {
      if (*p == nullptr) {
         throw std::runtime_error("Failed to obtain BluetoothBridge method " + std::to_string(p - begin));
      }
   }
}
BtInvoker::~BtInvoker()
{
   m_env->DeleteGlobalRef(m_class);
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