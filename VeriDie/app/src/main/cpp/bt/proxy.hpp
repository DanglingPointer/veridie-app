#ifndef BT_PROXY_HPP
#define BT_PROXY_HPP

#include <functional>
#include <optional>
#include "utils/future.hpp"
#include "bt/device.hpp"

namespace bt {

class IProxy
{
public:
   enum class Error
   { // same values as in BluetoothBridge.java
      NO_ERROR = 0,
      NO_LISTENER = 1,
      UNHANDLED_EXCEPTION = 2,
      ILLEGAL_STATE = 3
   };
   using Handle = async::Future<Error>;
   virtual ~IProxy() = default;
   virtual async::Future<bool> IsBluetoothEnabled() = 0;
   virtual Handle RequestPairedDevices() = 0;
   virtual Handle StartDiscovery() = 0;
   virtual Handle CancelDiscovery() = 0;
   virtual Handle RequestDiscoverability() = 0;
   virtual Handle StartListening(std::string selfName, const bt::Uuid & uuid) = 0;
   virtual Handle StopListening() = 0;
   virtual Handle Connect(const bt::Device & remote, const bt::Uuid & conn) = 0;
   virtual Handle CloseConnection(const bt::Device & remote) = 0;
   virtual Handle SendMessage(const bt::Device & remote, std::string msg) = 0;
};

} // namespace bt

#endif // BT_PROXY_HPP
