#include "jni/btproxy.hpp"
#include "jni/exec.hpp"
#include "jni/btinvoker.hpp"
#include "core/logging.hpp"
#include "core/exec.hpp"

namespace {
using namespace std::string_literals;

void MainExecutor(std::function<void()> cb)
{
   main::Exec([cb = std::move(cb)](auto) { cb(); });
}

class BtProxy : public bt::IProxy
{
public:
   template <typename F>
   explicit BtProxy(F && cbExecutor)
      : m_cbExecutor(std::forward<F>(cbExecutor))
   {}
   async::Future<bool> IsBluetoothEnabled() override;
   bt::IProxy::Handle RequestPairedDevices() override;
   bt::IProxy::Handle StartDiscovery() override;
   bt::IProxy::Handle CancelDiscovery() override;
   bt::IProxy::Handle RequestDiscoverability() override;
   bt::IProxy::Handle StartListening(std::string selfName, const bt::Uuid & uuid) override;
   bt::IProxy::Handle StopListening() override;
   bt::IProxy::Handle Connect(const bt::Device & remote, const bt::Uuid & conn) override;
   bt::IProxy::Handle CloseConnection(const bt::Device & remote) override;
   bt::IProxy::Handle SendMessage(const bt::Device & remote, std::string msg) override;

private:
   template <typename F>
   bt::IProxy::Handle MarshalToJNI(F && f)
   {
      async::Promise<bt::IProxy::Error> p(m_cbExecutor);
      auto handle = p.GetFuture();
      jni::Exec(async::EmbedPromiseIntoTask(
         std::move(p), [func = std::forward<F>(f)](jni::ServiceLocator * sl) {
            auto * invoker = sl->GetBtInvoker();
            auto * env = sl->GetJNIEnv();
            if (!invoker || !env) {
               sl->GetLogger().Write(LogPriority::ERROR,
                                     "BtProxy error: no "s + (env ? "jni::BtInvoker" : "JNIEnv"));
               return bt::IProxy::Error::ILLEGAL_STATE;
            }
            return func(invoker, env);
         }));
      return handle;
   }

   const async::Executor m_cbExecutor;
};

#define DEFINE_BTPROXY_SIMPLE_FUNC(name)                               \
   bt::IProxy::Handle BtProxy::name()                                  \
   {                                                                   \
      return MarshalToJNI([](jni::BtInvoker * invoker, JNIEnv * env) { \
         jint error = invoker->name();                                 \
         return static_cast<bt::IProxy::Error>(error);                 \
      });                                                              \
   }

DEFINE_BTPROXY_SIMPLE_FUNC(RequestPairedDevices)
DEFINE_BTPROXY_SIMPLE_FUNC(RequestDiscoverability)
DEFINE_BTPROXY_SIMPLE_FUNC(StartDiscovery)
DEFINE_BTPROXY_SIMPLE_FUNC(CancelDiscovery)
DEFINE_BTPROXY_SIMPLE_FUNC(StopListening)

#undef DEFINE_BTPROXY_SIMPLE_FUNC

async::Future<bool> BtProxy::IsBluetoothEnabled()
{
   async::Promise<bool> p(m_cbExecutor);
   auto handle = p.GetFuture();

   jni::Exec(async::EmbedPromiseIntoTask(std::move(p), [](jni::ServiceLocator * sl) {
      if (auto * invoker = sl->GetBtInvoker()) {
         jboolean btEnabled = invoker->IsBluetoothEnabled();
         return static_cast<bool>(btEnabled);
      }
      sl->GetLogger().Write(LogPriority::ERROR, "IsBluetoothEnabled failed: no invoker");
      return false;
   }));
   return handle;
}

bt::IProxy::Handle BtProxy::StartListening(std::string selfName, const bt::Uuid & uuid)
{
   auto[lsl, msl] = bt::UuidToLong(uuid);
   return MarshalToJNI(
      [name = std::move(selfName), lsl = lsl, msl = msl](jni::BtInvoker * invoker, JNIEnv * env) {
         jstring jname = env->NewStringUTF(name.c_str());
         jint error = invoker->StartListening(jname, lsl, msl);
         env->DeleteLocalRef(jname);
         return static_cast<bt::IProxy::Error>(error);
      });
}

bt::IProxy::Handle BtProxy::Connect(const bt::Device & remote, const bt::Uuid & conn)
{
   auto[lsl, msl] = bt::UuidToLong(conn);
   return MarshalToJNI([mac = remote.mac, lsl = lsl, msl = msl](jni::BtInvoker * invoker, JNIEnv * env) {
      jstring jmac = env->NewStringUTF(mac.c_str());
      jint error = invoker->Connect(jmac, lsl, msl);
      env->DeleteLocalRef(jmac);
      return static_cast<bt::IProxy::Error>(error);
   });
}

bt::IProxy::Handle BtProxy::CloseConnection(const bt::Device & remote)
{
   return MarshalToJNI([remoteMac = remote.mac](jni::BtInvoker * invoker, JNIEnv * env) {
      jstring jmac = env->NewStringUTF(remoteMac.c_str());
      jint error = invoker->CloseConnection(jmac);
      env->DeleteLocalRef(jmac);
      return static_cast<bt::IProxy::Error>(error);
   });
}

bt::IProxy::Handle BtProxy::SendMessage(const bt::Device & remote, std::string msg)
{
   return MarshalToJNI([remoteMac = remote.mac, msg = std::move(msg)](jni::BtInvoker * invoker, JNIEnv * env) {
      jstring jmac = env->NewStringUTF(remoteMac.c_str());
      jbyteArray jmsg = env->NewByteArray(msg.length());
      env->SetByteArrayRegion(jmsg, 0, msg.size(), reinterpret_cast<const jbyte *>(msg.data()));
      jint error = invoker->SendMessage(jmac, jmsg);
      env->DeleteLocalRef(jmac);
      env->DeleteLocalRef(jmsg);
      return static_cast<bt::IProxy::Error>(error);
   });
}

} // namespace

namespace jni {

std::unique_ptr<bt::IProxy> CreateBtProxy()
{
   return std::make_unique<BtProxy>(MainExecutor);
}

} // namespace jni