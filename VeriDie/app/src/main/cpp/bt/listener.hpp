#ifndef BT_LISTENER_HPP
#define BT_LISTENER_HPP

#include <string>
#include "bt/device.hpp"

namespace bt {

class IListener
{
public:
   virtual ~IListener() = default;
   virtual void OnBluetoothOn() = 0;
   virtual void OnBluetoothOff() = 0;
   virtual void OnDeviceFound(const bt::Device & remote, bool paired) = 0;
   virtual void OnDiscoverabilityConfirmed() = 0;
   virtual void OnDiscoverabilityRejected() = 0;
   virtual void OnScanModeChanged(bool discoverable, bool connectable) = 0;
   virtual void OnDeviceConnected(const bt::Device & remote) = 0;
   virtual void OnDeviceDisconnected(const bt::Device & remote) = 0;
   virtual void OnMessageReceived(const bt::Device & remote, std::string message) = 0;
};

} // namespace bt

#endif // BT_LISTENER_HPP
