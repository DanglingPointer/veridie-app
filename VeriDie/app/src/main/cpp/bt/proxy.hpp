#ifndef BT_PROXY_HPP
#define BT_PROXY_HPP

#include <functional>
#include <optional>
#include "bt/device.hpp"

namespace bt {

class IProxy
{
public:
   enum class Error
   { // same values as in BluetoothBridge.java
      NO_LISTENER = 1,
      UNHANDLED_EXCEPTION = 2,
      ILLEGAL_STATE = 3
   };
   using ErrorCallback = std::function<void(Error)>;
   virtual ~IProxy() = default;
   virtual void IsBluetoothEnabled(std::function<void(bool)> resultCb) = 0;
   virtual void RequestPairedDevices(std::optional<ErrorCallback> onError) = 0;
   virtual void StartDiscovery(std::optional<ErrorCallback> onError) = 0;
   virtual void CancelDiscovery(std::optional<ErrorCallback> onError) = 0;
   virtual void RequestDiscoverability(std::optional<ErrorCallback> onError) = 0;
   virtual void StartListening(std::string selfName, const bt::Uuid & uuid, std::optional<ErrorCallback> onError) = 0;
   virtual void StopListening(std::optional<ErrorCallback> onError) = 0;
   virtual void Connect(const bt::Device & remote, const bt::Uuid & conn, std::optional<ErrorCallback> onError) = 0;
   virtual void CloseConnection(const bt::Device & remote, std::optional<ErrorCallback> onError) = 0;
   virtual void SendMessage(const bt::Device & remote, std::string msg, std::optional<ErrorCallback> onError) = 0;
};

} // namespace bt

#endif // BT_PROXY_HPP
