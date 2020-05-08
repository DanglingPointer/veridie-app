#include <string>
#include <memory>
#include <vector>
#include <jni.h>
#include "jni/exec.hpp"
#include "utils/logger.hpp"
#include "jni/cmdmanager.hpp"
#include "core/exec.hpp"
#include "core/controller.hpp"
#include "utils/worker.hpp"
#include "bt/device.hpp"


namespace {

struct Context
{
   ILogger & logger;
   JavaVM * jvm;
   JNIEnv * jenv;
   std::unique_ptr<jni::ICmdManager> cmdMgr;
};

Worker & JniWorker()
{
   static auto s_logger = CreateLogger("JNI_WORKER");
   static Context s_ctx{*s_logger, nullptr, nullptr, nullptr};

   static Worker s_w(&s_ctx, *s_logger);
   return s_w;
}


std::string GetString(JNIEnv * env, jstring jstr)
{
   auto * strChars = env->GetStringUTFChars(jstr, nullptr);
   auto strLength = env->GetStringUTFLength(jstr);
   std::string ret(strChars, static_cast<size_t>(strLength));
   env->ReleaseStringUTFChars(jstr, strChars);
   return ret;
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

} // namespace

namespace jni {

void InternalExec(std::function<void(ICmdManager *)> task)
{
   JniWorker().ScheduleTask([t = std::move(task)](void * arg) {
      t(static_cast<Context *>(arg)->cmdMgr.get());
   });
}

} // namespace jni

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * vm, void * /*reserved*/)
{
   JniWorker().ScheduleTask([vm](void * arg) {
      auto * ctx = static_cast<Context *>(arg);
      ctx->jvm = vm;
      auto res = ctx->jvm->AttachCurrentThread(&ctx->jenv, nullptr);
      if (res != JNI_OK) {
         ctx->logger.Write<LogPriority::FATAL>("Failed to attach jni thread:", ErrorToString(res));
         std::abort();
      }
   });
   return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM * vm, void * /*reserved*/)
{
   JniWorker().ScheduleTask([vm](void * arg) {
      auto * ctx = static_cast<Context *>(arg);
      ctx->cmdMgr = nullptr;
      ctx->jvm = nullptr;
      ctx->jenv = nullptr;
      vm->DetachCurrentThread();
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_Bridge_bridgeReady(JNIEnv * env,
                                                                            jclass localRef)
{
   auto globalRef = static_cast<jclass>(env->NewGlobalRef(localRef));
   JniWorker().ScheduleTask([globalRef](void * arg) {
      auto * ctx = static_cast<Context *>(arg);
      if (!ctx->cmdMgr)
         ctx->cmdMgr = jni::CreateCmdManager(ctx->logger, ctx->jenv, globalRef);
   });
   core::Exec([](auto) {
      // Do nothing; this will create Controller and states
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_Bridge_sendEvent(JNIEnv * env,
                                                                          jclass,
                                                                          jint eventId,
                                                                          jobjectArray args)
{
   std::vector<std::string> arguments;

   size_t size = args ? env->GetArrayLength(args) : 0U;
   for (auto i = 0; i < size; ++i) {
      auto str = static_cast<jstring>(env->GetObjectArrayElement(args, i));
      arguments.emplace_back(GetString(env, str));
   }
   core::Exec([eventId, arguments = std::move(arguments)](core::IController * ctrl) {
      ctrl->OnEvent(eventId, arguments);
   });
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_Bridge_sendResponse(JNIEnv * /*env*/,
                                                                             jclass /*clazz*/,
                                                                             jint cmdId,
                                                                             jlong result)
{
   jni::Exec([cmdId, result](jni::ICmdManager * mgr) {
      mgr->OnCommandResponse(cmdId, result);
   });
}

} // extern C