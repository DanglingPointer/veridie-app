#include <string>
#include <memory>
#include <vector>
#include "jni/exec.hpp"
#include "jni/interop.hpp"
#include "core/exec.hpp"
#include "jni/logger.hpp"
#include "utils/worker.hpp"

namespace {
std::unique_ptr<Worker> g_thread;
std::unique_ptr<ILogger> g_logger;
} // namespace

namespace jni {

void Exec(std::function<void(JNIEnv *)> task)
{
   if (!g_thread) {
      g_logger->Write(LogPriority::WARN, "Fail: Trying to schedule task after Unload");
      return;
   }
   g_thread->ScheduleTask([t = std::move(task)](void * data) {
      void * env = nullptr;
      auto jvm = static_cast<JavaVM *>(data);
      if (auto err = jvm->GetEnv(&env, JNI_VERSION_1_6) != JNI_OK) {
         g_logger->Write(LogPriority::WARN, "Failed to get JNIEnv: " + ErrorToString(err));
      }
      t(static_cast<JNIEnv *>(env));
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

JNIEXPORT jstring JNICALL Java_com_vasilyev_veridie_MainActivity_stringFromJNI(JNIEnv * env,
                                                                               jobject /* this */)
{
   std::string hello = "Hello from C++";
   return env->NewStringUTF(hello.c_str());
}


JNIEXPORT void JNICALL Java_com_vasilyev_veridie_MainActivity_receiveData(JNIEnv * env, jobject obj,
                                                                          jstring srcDevice,
                                                                          jbyteArray dataArr,
                                                                          jint length)
{
   jbyte * elems = env->GetByteArrayElements(dataArr, nullptr);
   auto * first = reinterpret_cast<uint8_t *>(elems);
   auto * last = first + length;
   std::vector<uint8_t> buf(first, last);

   auto * strChars = env->GetStringUTFChars(srcDevice, nullptr);
   auto strLength = env->GetStringUTFLength(srcDevice);
   std::string deviceName(strChars, (size_t)strLength);

   main::Exec(
      [buf = std::move(buf), deviceName = std::move(deviceName)](main::ServiceLocator * svc){
         // todo
      });
   env->ReleaseByteArrayElements(dataArr, elems, JNI_ABORT);
   env->ReleaseStringUTFChars(srcDevice, strChars);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * vm, void * reserved)
{
   g_logger = jni::CreateLogger("JNI_WORKER");
   g_thread = std::make_unique<Worker>(vm, *g_logger);
   g_thread->ScheduleTask([](void * arg) {
      auto * jvm = static_cast<JavaVM *>(arg);
      JNIEnv * env = nullptr;
      if (auto err = jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
         g_logger->Write(LogPriority::FATAL,
                         "Failed to attach jni thread: " + jni::ErrorToString(err));
         std::abort();
      }
   });
   return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM * vm, void * reserved)
{
   g_thread->ScheduleTask([vm](void *) {
      vm->DetachCurrentThread();
      g_thread.reset();
   });
}