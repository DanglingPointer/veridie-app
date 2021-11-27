#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <jni.h>
#include "jni/exec.hpp"
#include "jni/javainvoker.hpp"
#include "core/exec.hpp"
#include "core/controller.hpp"
#include "sign/commandmanager.hpp"
#include "utils/task.hpp"
#include "utils/logger.hpp"
#include "utils/worker.hpp"
#include "bt/device.hpp"

using namespace std::chrono_literals;

namespace {

struct Context
{
   ILogger & logger;
   JavaVM * jvm;
   JNIEnv * jenv;
   std::shared_ptr<jni::JavaInvoker> uiInvoker;
   std::shared_ptr<jni::JavaInvoker> btInvoker;
};

void ScheduleOnJniWorker(std::function<void(Context &)> && task,
                         std::chrono::milliseconds delay = 0ms)
{
   static auto onException = [](std::string_view /*worker*/, std::string_view /*exception*/) {
      std::abort(); // TODO
   };
   static async::Worker s_worker(async::Worker::Config{
      .name = "JNI_WORKER",
      .capacity = std::numeric_limits<size_t>::max(),
      .exceptionHandler = onException,
   });
   static auto s_logger = CreateLogger("JNI_WORKER");
   static Context s_ctx{*s_logger, nullptr, nullptr, nullptr, nullptr};

   s_worker.Schedule(delay, [task = std::move(task)] {
      task(s_ctx);
   });
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

struct JniWorkerScheduler
{
   Context * ctx_ = nullptr;

   bool await_ready() { return false; }
   void await_suspend(stdcr::coroutine_handle<> h)
   {
      ScheduleOnJniWorker([h, this](Context & ctx) mutable {
         ctx_ = &ctx;
         h.resume();
      });
   }
   Context * await_resume()
   {
      assert(ctx_);
      return ctx_;
   }
};

cr::DetachedHandle OnLoad(JavaVM * vm)
{
   Context * ctx = co_await JniWorkerScheduler{};
   ctx->jvm = vm;
   auto res = ctx->jvm->AttachCurrentThread(&ctx->jenv, nullptr);
   if (res != JNI_OK) {
      ctx->logger.Write<LogPriority::FATAL>("Failed to attach jni thread:", ErrorToString(res));
      std::abort();
   }
}

cr::DetachedHandle OnUnload(JavaVM * vm)
{
   Context * ctx = co_await JniWorkerScheduler{};
   ctx->uiInvoker.reset();
   ctx->btInvoker.reset();
   ctx->jvm = nullptr;
   ctx->jenv = nullptr;
   vm->DetachCurrentThread();
}

cr::DetachedHandle BridgeReady(JNIEnv * env, jclass localRef)
{
   auto globalRef1 = static_cast<jclass>(env->NewGlobalRef(localRef));
   auto globalRef2 = static_cast<jclass>(env->NewGlobalRef(localRef));

   Context * ctx = co_await JniWorkerScheduler{};

   ctx->uiInvoker = std::make_shared<jni::JavaInvoker>(*ctx->jenv, globalRef1, "receiveUiCommand");
   auto uii = ctx->uiInvoker->GetExternalInvoker();

   ctx->btInvoker = std::make_shared<jni::JavaInvoker>(*ctx->jenv, globalRef2, "receiveBtCommand");
   auto bti = ctx->btInvoker->GetExternalInvoker();

   core::IController * ctrl = co_await core::Scheduler{};

   ctrl->Start(std::move(uii), std::move(bti));
}

cr::DetachedHandle SendEvent(JNIEnv * env, jint eventId, jobjectArray args)
{
   std::vector<std::string> arguments;

   size_t size = args ? env->GetArrayLength(args) : 0U;
   for (auto i = 0; i < size; ++i) {
      auto str = static_cast<jstring>(env->GetObjectArrayElement(args, i));
      arguments.emplace_back(GetString(env, str));
   }

   core::IController * ctrl = co_await core::Scheduler{};
   ctrl->OnEvent(eventId, arguments);
}

cr::DetachedHandle SendResponse(jint cmdId, jlong result)
{
   core::IController * ctrl = co_await core::Scheduler{};
   ctrl->OnCommandResponse(cmdId, result);
}

} // namespace

namespace jni {

void InternalExec(std::function<void()> task)
{
   ScheduleOnJniWorker([t = std::move(task)](Context &) {
      t();
   });
}

} // namespace jni

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * vm, void * /*reserved*/)
{
   OnLoad(vm);
   return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM * vm, void * /*reserved*/)
{
   OnUnload(vm);
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_Bridge_bridgeReady(JNIEnv * env,
                                                                            jclass localRef)
{
   BridgeReady(env, localRef);
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_Bridge_sendEvent(JNIEnv * env,
                                                                          jclass,
                                                                          jint eventId,
                                                                          jobjectArray args)
{
   SendEvent(env, eventId, args);
}

JNIEXPORT void JNICALL Java_com_vasilyev_veridie_interop_Bridge_sendResponse(JNIEnv * /*env*/,
                                                                             jclass /*clazz*/,
                                                                             jint cmdId,
                                                                             jlong result)
{
   SendResponse(cmdId, result);
}

} // extern C