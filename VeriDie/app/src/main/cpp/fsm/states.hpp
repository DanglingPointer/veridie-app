#ifndef FSM_STATES_HPP
#define FSM_STATES_HPP

#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "fsm/context.hpp"
#include "fsm/statebase.hpp"
#include "utils/future.hpp"

namespace bt {
struct Device;
}
namespace dice {
struct Offer;
}
namespace core {
struct Timeout;
}

namespace fsm {

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
   async::Future<core::Timeout> m_retryHandle;

   bool m_newGamePending;
   bool m_bluetoothOn;
};

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
   async::Future<core::Timeout> m_retryStartHandle;
};

class StateNegotiating : public StateBase
{
public:
   StateNegotiating(const Context & ctx,
                    std::unordered_set<bt::Device> && peers,
                    std::string && localMac);
   ~StateNegotiating();
   void OnBluetoothOff();
   void OnMessageReceived(const bt::Device & sender, const std::string & message);
   void OnSocketReadFailure(const bt::Device & from);

private:
   void UpdateAndBroadcastOffer();
   const std::string & GetLocalOfferMac();
   void DisconnectDevice(const std::string & mac);

   static uint32_t s_round;

   Context m_ctx;
   std::string m_localMac;

   std::unordered_set<bt::Device> m_peers;
   std::map<std::string, dice::Offer> m_offers;

   async::Future<core::Timeout> m_retrySendOffer;
};

class StatePlaying : public StateBase
{
public:
   explicit StatePlaying(const Context & ctx,
                         std::unordered_set<bt::Device> && peers,
                         std::string && generatorMac);
   ~StatePlaying();

private:
   class RemotePeerManager;

   Context m_ctx;
   std::unordered_map<std::string, RemotePeerManager> m_managers;
};

} // namespace fsm

#endif // FSM_STATES_HPP
