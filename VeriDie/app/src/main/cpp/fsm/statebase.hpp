#ifndef FSM_STATEBASE_HPP
#define FSM_STATEBASE_HPP

#include <string>

namespace bt {
class Device;
}
namespace dice {
class Request;
}

namespace fsm {

class StateBase
{
public:
   void OnBluetoothOn(){};
   void OnBluetoothOff(){};
   void OnDeviceFound(const bt::Device & remote, bool paired){};
   void OnDiscoverabilityConfirmed(){};
   void OnDiscoverabilityRejected(){};
   void OnScanModeChanged(bool discoverable, bool connectable){};
   void OnDeviceConnected(const bt::Device & remote){};
   void OnDeviceDisconnected(const bt::Device & remote){};
   void OnMessageReceived(const bt::Device & remote, std::string && message){};

   void OnDevicesQuery(bool connected, bool discovered){};
   void OnNameSet(const std::string & name){};
   void OnLocalNameQuery(){};
   void OnCastRequest(dice::Request && localRequest){};
   void OnCandidateApproved(const bt::Device & candidatePlayer){};
   void OnNewGame(){};
};

} // namespace fsm

#endif // FSM_STATEBASE_HPP
