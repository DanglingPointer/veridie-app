#ifndef UI_PROXY_HPP
#define UI_PROXY_HPP

#include <chrono>
#include <vector>
#include <string>
#include "dice/serializer.hpp"
#include "bt/device.hpp"

namespace ui {

class IProxy
{
public:
   virtual ~IProxy() = default;
   virtual void ShowToast(const std::string & message, std::chrono::seconds duration) = 0;
   virtual void ShowCandidates(const std::vector<bt::Device> & candidatePlayers) = 0;
   virtual void ShowConnections(const std::vector<bt::Device> & connectedPlayers) = 0;
   virtual void ShowCastResponse(const dice::Response & generated, bool external) = 0;
   virtual void ShowLocalName(const std::string & ownName) = 0;
};

} // namespace ui

#endif // UI_PROXY_HPP
