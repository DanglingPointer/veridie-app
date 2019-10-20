#ifndef UI_PROXY_HPP
#define UI_PROXY_HPP

#include <chrono>
#include <vector>
#include <string>
#include <string_view>
#include "dice/serializer.hpp"
#include "bt/device.hpp"
#include "utils/callback.hpp"

namespace ui {

class IProxy
{
public:
   enum class Error
   { // same values as in UiBridge.java
      NO_ERROR = 0,
      NO_LISTENER = 1,
      UNHANDLED_EXCEPTION = 2,
      ILLEGAL_STATE = 3
   };
   using Callback = async::Callback<Error>;
   virtual ~IProxy() = default;
   virtual void ShowToast(std::string message, std::chrono::seconds duration, Callback && cb) = 0;
   virtual void ShowCandidates(const std::vector<bt::Device> & candidatePlayers,
                               Callback && cb) = 0;
   virtual void ShowConnections(const std::vector<bt::Device> & connectedPlayers,
                                Callback && cb) = 0;
   virtual void ShowCastResponse(dice::Response generated, bool external, Callback && cb) = 0;
   virtual void ShowLocalName(std::string ownName, Callback && cb) = 0;
};

inline std::string_view ToString(IProxy::Error e)
{
   switch (e) {
      case IProxy::Error::NO_LISTENER: return "No listener";
      case IProxy::Error::UNHANDLED_EXCEPTION: return "Unhandled exceptino";
      case IProxy::Error::ILLEGAL_STATE: return "Illegal state";
      default: return "";
   }
}

} // namespace ui

#endif // UI_PROXY_HPP
