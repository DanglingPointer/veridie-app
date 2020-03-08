#ifndef FSM_STATES_HPP
#define FSM_STATES_HPP

#include <unordered_map>
#include <vector>

#include "fsm/context.hpp"
#include "fsm/statebase.hpp"
#include "utils/future.hpp"
#include "utils/canceller.hpp"
#include "bt/proxy.hpp"
#include "bt/device.hpp"
#include "ui/proxy.hpp"
#include "core/timerengine.hpp"

namespace fsm {

// Makes sure bluetooth is on. Goes to Connecting on new game if bluetooth is on.
class StateIdle : public StateBase, private async::Canceller<1>
{
public:
   explicit StateIdle(const Context & ctx);
   void OnBluetoothOn();
   void OnBluetoothOff();
   void OnNewGame();

private:
   void CheckBtState();

   Context m_ctx;

   async::Future<bool> m_btQuery;
   async::Future<main::Timeout> m_toastRepeater;
   bool m_newGamePending;
};

// Discovers and connects to other devices. Goes to Idle if bluetooth is off or if both discovery
// and discoverability are turned off. Goes to Negotiating after 30 sec of listening/discovering.
class StateConnecting : public StateBase, private async::Canceller<1>
{
public:
   explicit StateConnecting(const Context & ctx);
   void OnBluetoothOff();
   void OnDiscoverabilityConfirmed();
   void OnDiscoverabilityRejected();
   void OnScanModeChanged(bool discoverable, bool connectable);

   void OnDeviceFound(const bt::Device & remote, bool paired);
   void OnDeviceConnected(const bt::Device & remote);
   void OnDeviceDisconnected(const bt::Device & remote);
   void OnCandidateApproved(const bt::Device & remote);
   void OnDevicesQuery(bool connected, bool discovered);

private:
   void TryStartDiscoveryTimer();
   void OnDiscoveryFinished();
   void StartListening();

   Context m_ctx;

   std::optional<bool> m_discoverable;
   std::optional<bool> m_discovering;
   bt::IProxy::Handle m_discoverabilityRequest;
   bt::IProxy::Handle m_listening;
   bt::IProxy::Handle m_discovery;
   async::CombinedFuture m_discoveryShutdown;
   async::Future<main::Timeout> m_discoveryTimer;

   bt::Uuid m_conn;
   std::string m_selfName;
   std::unordered_map<bt::Device, int> m_devices;
   std::vector<bt::IProxy::Handle> m_connectingHandles;
   async::CombinedFuture m_disconnects;
};

class StateNegotiating : public StateBase
{
public:
   StateNegotiating(const Context & ctx, std::vector<bt::Device> && peers);

private:
   Context m_ctx;
};

class StatePlaying : public StateBase
{
public:
   explicit StatePlaying(const Context & ctx);

private:
   Context m_ctx;
};

} // namespace fsm

#endif // FSM_STATES_HPP
