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
struct Request;
struct Response;
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
   void KickOffDiscovery(uint32_t retriesLeft);
   void KickOffListening(uint32_t retriesLeft);

   Context m_ctx;

   std::optional<bool> m_discovering;
   std::optional<bool> m_listening;

   std::optional<std::string> m_localMac;
   std::unordered_set<bt::Device> m_peers;
   async::Future<core::Timeout> m_retryStartHandle;
   async::Future<core::Timeout> m_retryDiscoveryHandle;
   async::Future<core::Timeout> m_retryListeningHandle;
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
   StatePlaying(const Context & ctx,
                std::unordered_set<bt::Device> && peers,
                std::string && localMac,
                std::string && generatorMac);
   ~StatePlaying();
   void OnBluetoothOff();
   void OnDeviceConnected(const bt::Device & remote);
   void OnMessageReceived(const bt::Device & sender, const std::string & message);
   void OnCastRequest(dice::Request && localRequest);
   void OnGameStopped();
   void OnSocketReadFailure(const bt::Device & transmitter);

private:
   class RemotePeerManager;

   [[maybe_unused]] StateNegotiating * StartNegotiation();
   void ShowRequest(const dice::Request & request, const std::string & from);
   void ShowResponse(const dice::Response & response, const std::string & from);

   Context m_ctx;
   std::string m_localMac;
   bool m_localGenerator;

   async::Future<core::Timeout> m_ignoreOffers;
   std::unique_ptr<dice::Request> m_pendingRequest;

   std::unordered_map<std::string, RemotePeerManager> m_managers;
   uint32_t m_responseCount;
};

} // namespace fsm

#endif // FSM_STATES_HPP
