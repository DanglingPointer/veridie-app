#include <string>
#include <memory>
#include <vector>
#include "jni/exec.hpp"
#include "jni/interop.hpp"
#include "jni/logger.hpp"
#include "jni/btinvoker.hpp"
#include "jni/uiinvoker.hpp"
#include "core/exec.hpp"
#include "core/controller.hpp"
#include "utils/worker.hpp"
#include "bt/device.hpp"

namespace {
std::unique_ptr<Worker> g_thread;

class ServiceLocatorImpl : public jni::ServiceLocator
{
public:
   explicit ServiceLocatorImpl(JavaVM * javaVM)
      : m_logger(jni::CreateLogger("JNI_WORKER"))
      , m_javaVM(javaVM)
      , m_jniEnv(nullptr)
   {}
   JavaVM * GetJavaVM() override { return m_javaVM; }
   JNIEnv * GetJNIEnv() override { return m_jniEnv; }
   ILogger & GetLogger() override { return *m_logger; }
   jni::BtInvoker * GetBtInvoker() override { return m_btInvoker.get(); }
   jni::UiInvoker * GetUiInvoker() override { return m_uiInvoker.get(); }
   std::unique_ptr<ILogger> m_logger;
   JavaVM * m_javaVM;
   JNIEnv * m_jniEnv;
   std::unique_ptr<jni::BtInvoker> m_btInvoker;
   std::unique_ptr<jni::UiInvoker> m_uiInvoker;
};

std::string GetString(JNIEnv * env, jstring jstr)
{
   auto * strChars = env->GetStringUTFChars(jstr, nullptr);
   auto strLength = env->GetStringUTFLength(jstr);
   std::string ret(strChars, static_cast<size_t>(strLength));
   env->ReleaseStringUTFChars(jstr, strChars);
   return ret;
}

bool GetBool(jboolean var)
{
   return var == JNI_TRUE;
}

} // namespace

namespace jni {

void Exec(std::function<void(jni::ServiceLocator *)> task)
{
   if (!g_thread) {
      // Trying to schedule task after Unload
      return;
   }
   g_thread->ScheduleTask([t = std::move(task)](void * data) {
      t(static_cast<jni::ServiceLocator *>(data));
   });
}

std::string ErrorToString(jint error)
{
   switch (error) {
      case JNI_ERR: return "Generic error";
      case JNI_EDETACHED: return "Thread detached from the VM";
      case JNI_EVERSION: return "JNI version error";
      case JNI_ENOMEM: return "Out of memory";
      case JNI_EEXIST: return "VM already created";
      case JNI_EINVAL: return "InvalidArgument";
      default: return "Unknown error";
   }
}
} // namespace jni

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * vm, void * reserved)
{
   auto sl = std::make_unique<ServiceLocatorImpl>(vm);
   auto & logger = sl->GetLogger();
   g_thread = std::make_unique<Worker>(sl.release(), logger);
   g_thread->ScheduleTask([](void * arg) {
      auto * sl = static_cast<ServiceLocatorImpl *>(arg);
      JNIEnv * env = nullptr;
      if (auto err = sl->GetJavaVM()->AttachCurrentThread(&env, nullptr) != JNI_OK) {
         sl->GetLogger().Write(LogPriority::FATAL,
                               "Failed to attach jni thread: " + jni::ErrorToString(err));
         std::abort();
      }
      sl->m_jniEnv = env;
   });
   return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM * vm, void * reserved)
{
   g_thread->ScheduleTask([vm](void * arg) {
      auto * sl = static_cast<ServiceLocatorImpl *>(arg);
      sl->m_btInvoker.reset();
      sl->m_uiInvoker.reset();
      sl->m_jniEnv = nullptr;
      sl->m_javaVM = nullptr;
      vm->DetachCurrentThread();
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_bridgeCreated(JNIEnv * env,
                                                                                       jclass clazz)
{
   g_thread->ScheduleTask([env, clazz](void * arg) {
      auto * sl = static_cast<ServiceLocatorImpl *>(arg);
      sl->m_btInvoker = std::make_unique<jni::BtInvoker>(env, clazz);
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_bluetoothOn(JNIEnv * env,
                                                                                     jclass clazz)
{
   main::Exec([](main::ServiceLocator * svc) { svc->GetBtListener().OnBluetoothOn(); });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_bluetoothOff(JNIEnv * env,
                                                                                      jclass clazz)
{
   main::Exec([](main::ServiceLocator * svc) { svc->GetBtListener().OnBluetoothOff(); });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_deviceFound(
   JNIEnv * env, jclass clazz, jstring name, jstring mac, jboolean paired)
{
   main::Exec([
      deviceName = GetString(env, name), deviceMac = GetString(env, mac), paired = GetBool(paired)
   ](main::ServiceLocator * svc) mutable {
      svc->GetBtListener().OnDeviceFound(bt::Device(std::move(deviceName), std::move(deviceMac)),
                                         paired);
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_discoverabilityConfirmed(
   JNIEnv * env, jclass clazz)
{
   main::Exec(
      [](main::ServiceLocator * svc) { svc->GetBtListener().OnDiscoverabilityConfirmed(); });
}


JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_discoverabilityRejected(
   JNIEnv * env, jclass clazz)
{
   main::Exec([](main::ServiceLocator * svc) { svc->GetBtListener().OnDiscoverabilityRejected(); });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_scanModeChanged(
   JNIEnv * env, jclass clazz, jboolean discoverable, jboolean connectable)
{
   main::Exec([discoverable = GetBool(discoverable),
               connectable = GetBool(connectable)](main::ServiceLocator * svc) {
      svc->GetBtListener().OnScanModeChanged(discoverable, connectable);
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_deviceConnected(
   JNIEnv * env, jclass clazz, jstring name, jstring mac)
{
   main::Exec([deviceName = GetString(env, name),
               deviceMac = GetString(env, mac)](main::ServiceLocator * svc) mutable {
      svc->GetBtListener().OnDeviceConnected(
         bt::Device(std::move(deviceName), std::move(deviceMac)));
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_deviceDisonnected(
   JNIEnv * env, jclass clazz, jstring mac)
{
   main::Exec([deviceMac = GetString(env, mac)](main::ServiceLocator * svc) mutable {
      svc->GetBtListener().OnDeviceDisconnected(bt::Device(std::string(), std::move(deviceMac)));
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_BluetoothBridge_messageReceived(
   JNIEnv * env, jclass clazz, jstring srcDevice, jbyteArray dataArr, jint length)
{
   jbyte * elems = env->GetByteArrayElements(dataArr, nullptr);
   auto * first = reinterpret_cast<const char *>(elems);
   std::string buf(first, static_cast<size_t>(length));
   main::Exec([message = std::move(buf),
               deviceMac = GetString(env, srcDevice)](main::ServiceLocator * svc) mutable {
      svc->GetBtListener().OnMessageReceived(bt::Device(std::string(), std::move(deviceMac)),
                                             std::move(message));
   });
   env->ReleaseByteArrayElements(dataArr, elems, JNI_ABORT);
}