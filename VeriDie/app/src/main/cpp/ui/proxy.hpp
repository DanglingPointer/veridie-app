#ifndef UI_PROXY_HPP
#define UI_PROXY_HPP

#include <chrono>
#include <vector>
#include <string>
#include "dice/serializer.hpp"
#include "bt/device.hpp"
#include "utils/future.hpp"

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
   using Handle = async::Future<Error>;
   virtual ~IProxy() = default;
   virtual Handle ShowToast(std::string message, std::chrono::seconds duration) = 0;
   virtual Handle ShowCandidates(const std::vector<bt::Device> & candidatePlayers) = 0;
   virtual Handle ShowConnections(const std::vector<bt::Device> & connectedPlayers) = 0;
   virtual Handle ShowCastResponse(dice::Response generated, bool external) = 0;
   virtual Handle ShowLocalName(std::string ownName) = 0;
};

} // namespace ui

#endif // UI_PROXY_HPP
