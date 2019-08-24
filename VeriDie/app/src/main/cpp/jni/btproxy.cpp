#include "jni/btproxy.hpp"
#include "jni/exec.hpp"
#include "jni/btinvoker.hpp"
#include "core/logging.hpp"
#include "core/exec.hpp"

namespace {

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
   const async::Executor m_cbExecutor;
};

#define DEFINE_BTPROXY_SIMPLE_FUNC(name)                                                 \
   bt::IProxy::Handle BtProxy::name()                                                    \
   {                                                                                     \
      async::Promise<bt::IProxy::Error> p(m_cbExecutor);                                 \
      auto handle = p.GetFuture();                                                       \
      jni::Exec(async::EmbedPromiseIntoTask(std::move(p), [](jni::ServiceLocator * sl) { \
         if (auto * invoker = sl->GetBtInvoker()) {                                      \
            jint error = invoker->name();                                                \
            return static_cast<bt::IProxy::Error>(error);                                \
         }                                                                               \
         sl->GetLogger().Write(LogPriority::ERROR, #name " failed: no invoker");         \
         return bt::IProxy::Error::ILLEGAL_STATE;                                        \
      }));                                                                               \
      return handle;                                                                     \
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
   async::Promise<bt::IProxy::Error> p(m_cbExecutor);
   auto handle = p.GetFuture();

   auto[lsl, msl] = bt::UuidToLong(uuid);
   jni::Exec(async::EmbedPromiseIntoTask(
      std::move(p), [name = std::move(selfName), lsl = lsl, msl = msl](jni::ServiceLocator * sl) {
         if (auto * invoker = sl->GetBtInvoker()) {
            jstring jname = sl->GetJNIEnv()->NewStringUTF(name.c_str());
            jint error = invoker->StartListening(jname, lsl, msl);
            sl->GetJNIEnv()->DeleteLocalRef(jname);
            return static_cast<bt::IProxy::Error>(error);
         }
         sl->GetLogger().Write(LogPriority::ERROR, "StartListening failed: no invoker");
         return bt::IProxy::Error::ILLEGAL_STATE;
      }));
   return handle;
}

bt::IProxy::Handle BtProxy::Connect(const bt::Device & remote, const bt::Uuid & conn)
{
   async::Promise<bt::IProxy::Error> p(m_cbExecutor);
   auto handle = p.GetFuture();

   auto[lsl, msl] = bt::UuidToLong(conn);
   jni::Exec(async::EmbedPromiseIntoTask(
      std::move(p), [mac = remote.mac, lsl = lsl, msl = msl](jni::ServiceLocator * sl) {
         if (auto * invoker = sl->GetBtInvoker()) {
            jstring jmac = sl->GetJNIEnv()->NewStringUTF(mac.c_str());
            jint error = invoker->Connect(jmac, lsl, msl);
            sl->GetJNIEnv()->DeleteLocalRef(jmac);
            return static_cast<bt::IProxy::Error>(error);
         }
         sl->GetLogger().Write(LogPriority::ERROR, "Connect failed: no invoker");
         return bt::IProxy::Error::ILLEGAL_STATE;
      }));
   return handle;
}

bt::IProxy::Handle BtProxy::CloseConnection(const bt::Device & remote)
{
   async::Promise<bt::IProxy::Error> p(m_cbExecutor);
   auto handle = p.GetFuture();

   jni::Exec(
      async::EmbedPromiseIntoTask(std::move(p), [remoteMac = remote.mac](jni::ServiceLocator * sl) {
         if (auto * invoker = sl->GetBtInvoker()) {
            jstring jmac = sl->GetJNIEnv()->NewStringUTF(remoteMac.c_str());
            jint error = invoker->CloseConnection(jmac);
            sl->GetJNIEnv()->DeleteLocalRef(jmac);
            return static_cast<bt::IProxy::Error>(error);
         }
         sl->GetLogger().Write(LogPriority::ERROR, "CloseConnection failed: no invoker");
         return bt::IProxy::Error::ILLEGAL_STATE;
      }));
   return handle;
}

bt::IProxy::Handle BtProxy::SendMessage(const bt::Device & remote, std::string msg)
{
   async::Promise<bt::IProxy::Error> p(m_cbExecutor);
   auto handle = p.GetFuture();

   jni::Exec([remoteMac = remote.mac, msg = std::move(msg)](jni::ServiceLocator * sl) {
      if (auto * invoker = sl->GetBtInvoker()) {
         JNIEnv * env = sl->GetJNIEnv();
         jstring jmac = env->NewStringUTF(remoteMac.c_str());
         jbyteArray jmsg = env->NewByteArray(msg.length());
         env->SetByteArrayRegion(jmsg, 0, msg.size(), reinterpret_cast<const jbyte *>(msg.data()));
         jint error = invoker->SendMessage(jmac, jmsg);
         env->DeleteLocalRef(jmac);
         env->DeleteLocalRef(jmsg);
         return static_cast<bt::IProxy::Error>(error);
      }
      sl->GetLogger().Write(LogPriority::ERROR, "SendMessage failed: no invoker");
      return bt::IProxy::Error::ILLEGAL_STATE;
   });
   return handle;
}

} // namespace

namespace jni {

std::unique_ptr<bt::IProxy> CreateBtProxy()
{
   return std::make_unique<BtProxy>(MainExecutor);
}

} // namespace jni