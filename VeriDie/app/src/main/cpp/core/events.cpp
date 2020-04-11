#include "core/events.hpp"
#include "fsm/states.hpp"
#include "dice/serializer.hpp"

namespace event {

template <typename F>
void StateInvoke(F && f, fsm::StateHolder & state)
{
   struct Workaround : F
   {
      using F::operator();
      void operator()(std::monostate) {}
   };
   std::visit(Workaround{std::forward<F>(f)}, state);
}

bool BluetoothStateChanged::Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args)
{
   if (args.size() < 1)
      return false;

   std::string_view state = args.front();

   if (state == "on") {
      StateInvoke([](auto & s) { s.OnBluetoothOn(); }, stateHolder);
      return true;
   }
   if (state == "off") {
      StateInvoke([](auto & s) { s.OnBluetoothOff(); }, stateHolder);
      return true;
   }
   return false;
}

bool RemoteDeviceStateChanged::Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args)
{
   if (args.size() < 3)
      return false;

   bt::Device device(args[2], args[1]);
   if (args[0] == "connected") {
      StateInvoke([&](auto & s) { s.OnDeviceConnected(device); }, stateHolder);
      return true;
   }
   if (args[0] == "disconnected") {
      StateInvoke([&](auto & s) { s.OnDeviceDisconnected(device); }, stateHolder);
      return true;
   }
   return false;
}

bool ConnectivityEstablished::Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> &)
{
   StateInvoke([](auto & s) { s.OnConnectivityEstablished(); }, stateHolder);
   return true;
}

bool NewGameRequested::Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> &)
{
   StateInvoke([](auto & s) { s.OnNewGame(); }, stateHolder);
   return true;
}

bool MessageReceived::Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args)
{
   // "message", "mac", "name"
   if (args.size() < 3)
      return false;

   bt::Device sender(args[2], args[1]);
   StateInvoke([&](auto & s) { s.OnMessageReceived(sender, args[0]); }, stateHolder);
   return true;
}

bool CastRequestIssued::Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args)
{
   // "type", "size", "threshold"
   if (args.size() < 2)
      return false;

   try {
      size_t size = std::stoul(args[1]);
      dice::Request request{dice::MakeCast(args[0], size), std::nullopt};
      if (args.size() == 3)
         request.threshold = std::stoi(args[2]);
      StateInvoke([&](auto & s) { s.OnCastRequest(std::move(request)); }, stateHolder);
   }
   catch (...) {
      return false;
   }
   return true;
}

bool GameStopped::Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> &)
{
   StateInvoke([](auto & s) { s.OnGameStopped(); }, stateHolder);
   return true;
}

} // namespace event