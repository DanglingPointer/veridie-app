#ifndef FSM_STATEBASE_HPP
#define FSM_STATEBASE_HPP

#include <string>

namespace bt {
struct Device;
}
namespace dice {
struct Request;
}

namespace fsm {

class StateBase
{
public:
   void OnBluetoothOn(){}
   void OnBluetoothOff(){}
   void OnDeviceConnected(const bt::Device & /*remote*/){}
   void OnDeviceDisconnected(const bt::Device & /*remote*/){}
   void OnConnectivityEstablished(){}
   void OnNewGame(){}
   void OnMessageReceived(const bt::Device & /*sender*/, const std::string & /*message*/){}
   void OnCastRequest(dice::Request && /*localRequest*/){}
   void OnGameStopped(){}
   void OnSocketReadFailure(const bt::Device & /*transmitter*/){}
};

} // namespace fsm

#endif // FSM_STATEBASE_HPP
