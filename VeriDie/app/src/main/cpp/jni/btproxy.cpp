#include "jni/btproxy.hpp"
#include "jni/exec.hpp"
#include "jni/btinvoker.hpp"
#include "core/logging.hpp"
#include "core/exec.hpp"

namespace {

class BtProxy : public bt::IProxy
{
public:
   void IsBluetoothEnabled(std::function<void(bool)> resultCb) override;
   void RequestPairedDevices(std::optional<ErrorCallback> onError) override;
   void StartDiscovery(std::optional<ErrorCallback> onError) override;
   void CancelDiscovery(std::optional<ErrorCallback> onError) override;
   void RequestDiscoverability(std::optional<ErrorCallback> onError) override;
   void StartListening(std::string selfName, const bt::Uuid & uuid,
                       std::optional<ErrorCallback> onError) override;
   void StopListening(std::optional<ErrorCallback> onError) override;
   void Connect(const bt::Device & remote, const bt::Uuid & conn,
                std::optional<ErrorCallback> onError) override;
   void CloseConnection(const bt::Device & remote, std::optional<ErrorCallback> onError) override;
   void SendMessage(const bt::Device & remote, std::string msg,
                    std::optional<ErrorCallback> onError) override;
};

#define DEFINE_BTPROXY_SIMPLE_FUNC(name)                                            \
   void BtProxy::name(std::optional<bt::IProxy::ErrorCallback> onError)             \
   {                                                                                \
      jni::Exec([cb = std::move(onError)](jni::ServiceLocator * sl) mutable {       \
         auto * invoker = sl->GetBtInvoker();                                       \
         if (!invoker) {                                                            \
            sl->GetLogger().Write(LogPriority::ERROR, #name " failed: no invoker"); \
            return;                                                                 \
         }                                                                          \
         jint error = invoker->name();                                              \
         if (error && cb) {                                                         \
            main::Exec([cb = std::move(cb).value(), error](auto) {                  \
               cb(static_cast<bt::IProxy::Error>(error));                           \
            });                                                                     \
         }                                                                          \
      });                                                                           \
   }

DEFINE_BTPROXY_SIMPLE_FUNC(RequestPairedDevices)
DEFINE_BTPROXY_SIMPLE_FUNC(RequestDiscoverability)
DEFINE_BTPROXY_SIMPLE_FUNC(StartDiscovery)
DEFINE_BTPROXY_SIMPLE_FUNC(CancelDiscovery)
DEFINE_BTPROXY_SIMPLE_FUNC(StopListening)

#undef DEFINE_BTPROXY_SIMPLE_FUNC

void BtProxy::IsBluetoothEnabled(std::function<void(bool)> resultCb)
{
   jni::Exec([cb = std::move(resultCb)](jni::ServiceLocator * sl) mutable {
      auto * invoker = sl->GetBtInvoker();
      if (!invoker) {
         sl->GetLogger().Write(LogPriority::ERROR, "IsBluetoothEnabled failed: no invoker");
         return;
      }
      bool btEnabled = static_cast<bool>(invoker->IsBluetoothEnabled());
      main::Exec([cb = std::move(cb), btEnabled](auto) { cb(btEnabled); });
   });
}

void BtProxy::StartListening(std::string selfName, const bt::Uuid & uuid,
                             std::optional<bt::IProxy::ErrorCallback> onError)
{
   auto[lsl, msl] = bt::UuidToLong(uuid);
   jni::Exec([cb = std::move(onError), name = std::move(selfName), lsl = lsl,
              msl = msl](jni::ServiceLocator * sl) mutable {
      auto * invoker = sl->GetBtInvoker();
      if (!invoker) {
         sl->GetLogger().Write(LogPriority::ERROR, "StartListening failed: no invoker");
         return;
      }
      jstring jname = sl->GetJNIEnv()->NewStringUTF(name.c_str());
      jint error = invoker->StartListening(jname, lsl, msl);
      if (error && cb) {
         main::Exec([cb = std::move(cb).value(), error](auto) {
            cb(static_cast<bt::IProxy::Error>(error));
         });
      }
      sl->GetJNIEnv()->DeleteLocalRef(jname);
   });
}

void BtProxy::Connect(const bt::Device & remote, const bt::Uuid & conn,
                      std::optional<bt::IProxy::ErrorCallback> onError)
{
   auto[lsl, msl] = bt::UuidToLong(conn);
   jni::Exec(
      [cb = std::move(onError), mac = remote.mac, lsl = lsl, msl = msl](jni::ServiceLocator * sl) mutable {
         auto * invoker = sl->GetBtInvoker();
         if (!invoker) {
            sl->GetLogger().Write(LogPriority::ERROR, "Connect failed: no invoker");
            return;
         }
         jstring jmac = sl->GetJNIEnv()->NewStringUTF(mac.c_str());
         jint error = invoker->Connect(jmac, lsl, msl);
         if (error && cb) {
            main::Exec([cb = std::move(cb).value(), error](auto) {
               cb(static_cast<bt::IProxy::Error>(error));
            });
         }
         sl->GetJNIEnv()->DeleteLocalRef(jmac);
      });
}

void BtProxy::CloseConnection(const bt::Device & remote,
                              std::optional<bt::IProxy::ErrorCallback> onError)
{
   jni::Exec([cb = std::move(onError), remoteMac = remote.mac](jni::ServiceLocator * sl) mutable {
      auto * invoker = sl->GetBtInvoker();
      if (!invoker) {
         sl->GetLogger().Write(LogPriority::ERROR, "CloseConnection failed: no invoker");
         return;
      }
      jstring jmac = sl->GetJNIEnv()->NewStringUTF(remoteMac.c_str());
      jint error = invoker->CloseConnection(jmac);
      if (error && cb) {
         main::Exec([cb = std::move(cb).value(), error](auto) {
            cb(static_cast<bt::IProxy::Error>(error));
         });
      }
      sl->GetJNIEnv()->DeleteLocalRef(jmac);
   });
}

void BtProxy::SendMessage(const bt::Device & remote, std::string msg,
                          std::optional<bt::IProxy::ErrorCallback> onError)
{
   jni::Exec([cb = std::move(onError), remoteMac = remote.mac,
              msg = std::move(msg)](jni::ServiceLocator * sl) mutable {
      auto * invoker = sl->GetBtInvoker();
      if (!invoker) {
         sl->GetLogger().Write(LogPriority::ERROR, "SendMessage failed: no invoker");
         return;
      }
      JNIEnv * env = sl->GetJNIEnv();
      jstring jmac = env->NewStringUTF(remoteMac.c_str());
      jbyteArray jmsg = env->NewByteArray(msg.length());
      env->SetByteArrayRegion(jmsg, 0, msg.size(), reinterpret_cast<jbyte *>(msg.data()));
      jint error = invoker->SendMessage(jmac, jmsg);
      if (error && cb) {
         main::Exec([cb = std::move(cb).value(), error](auto) {
            cb(static_cast<bt::IProxy::Error>(error));
         });
      }
      env->DeleteLocalRef(jmac);
      env->DeleteLocalRef(jmsg);
   });
}

} // namespace

namespace jni {

std::unique_ptr<bt::IProxy> CreateBtProxy()
{
   return std::make_unique<BtProxy>();
}

} // namespace jni