#ifndef BT_LISTENER_HPP
#define BT_LISTENER_HPP

#include <string>

namespace bt {

class IListener
{
public:
   virtual ~IListener() = default;
   virtual void OnBluetoothOn() = 0;
   virtual void OnBluetoothOff() = 0;
   virtual void OnDeviceFound(std::string name, std::string mac, bool paired) = 0;
   virtual void OnDiscoverabilityConfirmed() = 0;
   virtual void OnDiscoverabilityRejected() = 0;
   virtual void OnScanModeChanged(bool discoverable, bool connectable) = 0;
   virtual void OnDeviceConnected(std::string name, std::string mac) = 0;
   virtual void OnDeviceDisconnected(std::string mac) = 0;
   virtual void OnMessageReceived(std::string mac, std::string message) = 0;
};

} // namespace bt

#endif // BT_LISTENER_HPP
