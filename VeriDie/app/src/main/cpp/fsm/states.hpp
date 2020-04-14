#ifndef FSM_STATES_HPP
#define FSM_STATES_HPP

#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "fsm/context.hpp"
#include "fsm/statebase.hpp"
#include "bt/device.hpp"
#include "core/timerengine.hpp"

namespace fsm {

// Makes sure bluetooth is on. Goes to Connecting on new game if bluetooth is on.
class StateIdle : public StateBase
{
public:
   explicit StateIdle(const Context & ctx);
   void OnBluetoothOn();
   void OnBluetoothOff();
   void OnNewGame();

private:
   void RequestBluetoothOn();

   Context m_ctx;

   CallbackId m_enableBtCb;
   async::Future<main::Timeout> m_retryHandle;

   bool m_newGamePending;
   bool m_bluetoothOn;
};

// Discovers and connects to other devices. Goes to Idle if bluetooth is off or if both discovery
// and discoverability are turned off. Goes to Negotiating after 30 sec of listening/discovering.
class StateConnecting : public StateBase
{
public:
   explicit StateConnecting(const Context & ctx);
   ~StateConnecting();
   void OnBluetoothOff();
   void OnDeviceConnected(const bt::Device & remote);
   void OnDeviceDisconnected(const bt::Device & remote);
   void OnMessageReceived(const bt::Device & sender, const std::string & message);
   void OnConnectivityEstablished();
   void OnSocketReadFailure(const bt::Device & from);

private:
   void CheckStatus();
   void SendHelloTo(const std::string & mac, uint32_t retriesLeft);
   void DisconnectDevice(const std::string & mac);
   void AttemptNegotiationStart(uint32_t retriesLeft);

   Context m_ctx;

   std::optional<bool> m_discovering;
   std::optional<bool> m_listening;

   std::optional<std::string> m_localMac;
   std::unordered_set<bt::Device> m_peers;
   async::Future<main::Timeout> m_retryStartHandle;
};

class StateNegotiating : public StateBase
{
public:
   StateNegotiating(const Context & ctx, std::unordered_set<bt::Device> && peers);
   void OnBluetoothOff();
   void OnMessageReceived(const bt::Device & sender, const std::string & message);
   void OnSocketReadFailure(const bt::Device & from);

private:
   Context m_ctx;

   std::unordered_set<bt::Device> m_peers;
   std::map<std::string, uint32_t> m_votes;

   static uint32_t s_round;
};

class StatePlaying : public StateBase
{
public:
   explicit StatePlaying(const Context & ctx, std::unordered_set<bt::Device> && peers);
   ~StatePlaying();

private:
   class RemotePeerManager;

   Context m_ctx;
   std::unordered_map<std::string, RemotePeerManager> m_managers;
};

} // namespace fsm

#endif // FSM_STATES_HPP
