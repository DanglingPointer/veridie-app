#include "jni/uiproxy.hpp"
#include "jni/exec.hpp"
#include "jni/uiinvoker.hpp"
#include "core/logging.hpp"
#include "core/exec.hpp"
#include <vector>

namespace {
using namespace std::string_literals;

void MainExecutor(std::function<void()> cb)
{
   main::Exec([cb = std::move(cb)](auto) { cb(); });
}

std::vector<std::string> DevicesToNames(const std::vector<bt::Device> & candidatePlayers)
{
   std::vector<std::string> names;
   names.reserve(candidatePlayers.size());
   std::transform(std::cbegin(candidatePlayers), std::cend(candidatePlayers),
                  std::back_inserter(names), [](const bt::Device & device) { return device.name; });
   return names;
}

void FreeObjectArray(jobjectArray arr, jint size, JNIEnv * env)
{
   for (size_t i = 0; i < size; ++i) {
      env->DeleteLocalRef(env->GetObjectArrayElement(arr, i));
   }
   env->DeleteLocalRef(arr);
}

jobjectArray CreateStringArray(const std::vector<std::string> & data, JNIEnv * env)
{
   jclass stringClass = env->FindClass("java/lang/String");
   jobjectArray arr = env->NewObjectArray(data.size(), stringClass, nullptr);
   for (size_t i = 0; i < data.size(); ++i) {
      env->SetObjectArrayElement(arr, i, env->NewStringUTF(data[i].c_str()));
   }
   return arr;
}

struct ResponseToIntArray
{
   JNIEnv * env;

   template <typename D>
   jintArray operator()(const std::vector<D> & cast)
   {
      jintArray arr = env->NewIntArray(cast.size());
      jint * elems = env->GetIntArrayElements(arr, nullptr);
      jint * it = elems;
      for (const auto & dice : cast) {
         *it++ = dice;
      }
      env->ReleaseIntArrayElements(arr, elems, 0);
      return arr;
   }
};

using Executor = std::function<void(std::function<void()>)>;

class UiProxy : public ui::IProxy
{
public:
   template <typename F>
   explicit UiProxy(F && cbExecutor)
      : m_cbExecutor(std::forward<F>(cbExecutor))
   {}
   void ShowToast(std::string message, std::chrono::seconds duration, Callback && cb) override;
   void ShowCandidates(const std::vector<bt::Device> & candidatePlayers, Callback && cb) override;
   void ShowConnections(const std::vector<bt::Device> & connectedPlayers, Callback && cb) override;
   void ShowCastResponse(dice::Response generated, bool external, Callback && cb) override;
   void ShowLocalName(std::string ownName, Callback && cb) override;

private:
   template <typename F>
   void MarshalToJNI(F && f, Callback && cb)
   {
      jni::Exec([func = std::forward<F>(f), cb = std::move(cb),
                 exec = m_cbExecutor](jni::ServiceLocator * sl) mutable {
         if (cb.Cancelled())
            return;
         auto * invoker = sl->GetUiInvoker();
         auto * env = sl->GetJNIEnv();
         Error e;
         if (!invoker || !env) {
            sl->GetLogger().Write(LogPriority::ERROR,
                                  "UiProxy error: no "s + (env ? "jni::UiInvoker" : "JNIEnv"));
            e = ui::IProxy::Error::ILLEGAL_STATE;
         } else {
            e = func(invoker, env);
         }
         async::Schedule(std::move(exec), std::move(cb), e);
      });
   }

   const Executor m_cbExecutor;
};

void UiProxy::ShowToast(std::string message, std::chrono::seconds duration, Callback && cb)
{
   MarshalToJNI(
      [msg = std::move(message), sec = duration.count()](jni::UiInvoker * invoker, JNIEnv * env) {
         jstring jmsg = env->NewStringUTF(msg.c_str());
         jint error = invoker->ShowToast(jmsg, static_cast<jint>(sec));
         env->DeleteLocalRef(jmsg);
         return static_cast<ui::IProxy::Error>(error);
      },
      std::move(cb));
}
void UiProxy::ShowCandidates(const std::vector<bt::Device> & candidatePlayers, Callback && cb)
{
   return MarshalToJNI(
      [names = DevicesToNames(candidatePlayers)](jni::UiInvoker * invoker, JNIEnv * env) {
         jobjectArray jnames = CreateStringArray(names, env);
         jint error = invoker->ShowCandidates(jnames);
         FreeObjectArray(jnames, names.size(), env);
         return static_cast<ui::IProxy::Error>(error);
      },
      std::move(cb));
}
void UiProxy::ShowConnections(const std::vector<bt::Device> & connectedPlayers, Callback && cb)
{
   MarshalToJNI(
      [names = DevicesToNames(connectedPlayers)](jni::UiInvoker * invoker, JNIEnv * env) {
         jobjectArray jnames = CreateStringArray(names, env);
         jint error = invoker->ShowConnections(jnames);
         FreeObjectArray(jnames, names.size(), env);
         return static_cast<ui::IProxy::Error>(error);
      },
      std::move(cb));
}
void UiProxy::ShowCastResponse(dice::Response response, bool external, Callback && cb)
{
   MarshalToJNI(
      [cast = std::move(response), external](jni::UiInvoker * invoker, JNIEnv * env) {
         jintArray data = std::visit(ResponseToIntArray{env}, cast.cast);
         jint successCount = cast.successCount.value_or(-1);
         jboolean isExternal = external ? JNI_TRUE : JNI_FALSE;

         jint error = invoker->ShowCastResponse(data, successCount, isExternal);
         env->DeleteLocalRef(data);
         return static_cast<ui::IProxy::Error>(error);
      },
      std::move(cb));
}
void UiProxy::ShowLocalName(std::string ownName, Callback && cb)
{
   MarshalToJNI(
      [name = std::move(ownName)](jni::UiInvoker * invoker, JNIEnv * env) {
         jstring jname = env->NewStringUTF(name.c_str());
         jint error = invoker->ShowLocalName(jname);
         env->DeleteLocalRef(jname);
         return static_cast<ui::IProxy::Error>(error);
      },
      std::move(cb));
}

} // namespace

namespace jni {

std::unique_ptr<ui::IProxy> CreateUiProxy()
{
   return std::make_unique<UiProxy>(MainExecutor);
}

} // namespace jni