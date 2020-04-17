#include <functional>

#include "fsm/states.hpp"
#include "core/logging.hpp"
#include "dice/serializer.hpp"
#include "dice/engine.hpp"
#include "jni/proxy.hpp"

namespace fsm {

// handles connection errors, retries, reconnections, buffering etc
class StatePlaying::RemotePeerManager : private async::Canceller<32>
{
//public:
//   RemotePeerManager(const bt::Device & remote,
//                     std::function<void(dice::Request)> onRequestReceived,
//                     std::function<void(dice::Response)> onResponseReceived,
//                     jni::IProxy & proxy,
//                     dice::ISerializer & serializer)
//      : m_remote(remote)
//      , m_onRequestCb(std::move(onRequestReceived))
//      , m_onResponseCb(std::move(onResponseReceived))
//      , m_proxy(proxy)
//      , m_serializer(serializer)
//   {}
//
//
//private:
//   bt::Device m_remote;
//   std::function<void(dice::Request)> m_onRequestCb;
//   std::function<void(dice::Response)> m_onResponseCb;
//   jni::IProxy & m_proxy;
//   dice::ISerializer & m_serializer; // todo: replace by context
};



StatePlaying::StatePlaying(const Context & ctx, std::unordered_set<bt::Device> && /*peers*/, std::string && /*generatorMac*/)
   : m_ctx(ctx)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);

   // TODO: initialize managers using std::transform from peers
}

StatePlaying::~StatePlaying() = default;

} // namespace fsm