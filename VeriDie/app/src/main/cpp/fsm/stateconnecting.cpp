#include "fsm/states.hpp"
#include "core/logging.hpp"

//namespace {
//
//constexpr int CONNECTED = 0x01;
//constexpr int APPROVED = 0x02;
//constexpr int DISCOVERED = 0x04;
//constexpr int PAIRED = 0x08;
//
//} // namespace

#define FIRE_AND_FORGET DetachedCb<ui::IProxy::Error>()

namespace fsm {
using namespace std::chrono_literals;

StateConnecting::StateConnecting(const Context & ctx)
   : m_ctx(ctx)
{
   // TODO: set m_conn and m_selfName

//   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
//
//   m_discoverabilityRequest =
//      m_ctx.bluetooth->RequestDiscoverability().Then([this](std::optional<bt::IProxy::Error> e) {
//         if (!e || *e != bt::IProxy::Error::NO_ERROR) {
//            m_discoverable = false;
//            m_ctx.logger->Write<LogPriority::WARN>(
//               "Discoverability request failed:",
//               std::to_string(static_cast<int>(e.value_or(bt::IProxy::Error::NO_ERROR))));
//            TryStartDiscoveryTimer();
//         } else {
//            StartListening();
//         }
//      });
//   m_discovery = m_ctx.bluetooth->StartDiscovery().Then([this](std::optional<bt::IProxy::Error> e) {
//      m_discovering = (e && *e == bt::IProxy::Error::NO_ERROR);
//      TryStartDiscoveryTimer();
//   });
}

void StateConnecting::OnConnectivityEstablished()
{
   // TODO
}

void StateConnecting::StartListening()
{
//   m_listening =
//      m_ctx.bluetooth->StartListening(m_selfName, m_conn)
//         .Then([this](std::optional<bt::IProxy::Error> e) {
//            m_discoverable = (e && *e == bt::IProxy::Error::NO_ERROR);
//            if (!*m_discoverable) {
//               m_ctx.logger->Write<LogPriority::WARN>(
//                  "Listening start failed:",
//                  std::to_string(static_cast<int>(e.value_or(bt::IProxy::Error::NO_ERROR))));
//            }
//            TryStartDiscoveryTimer();
//         });
}

void StateConnecting::OnBluetoothOff()
{
   m_ctx.logger->Write<LogPriority::ERROR>("Connection failed: bluetooth turned off unexpectedly");
   m_ctx.state->emplace<StateIdle>(m_ctx);
}

void StateConnecting::TryStartDiscoveryTimer()
{
//   if (m_discoveryTimer.IsActive() || m_disconnects.IsActive())
//      return;
//
//   if (!m_discoverable || !m_discovering)
//      return;
//
//   if (!*m_discoverable && !*m_discovering) {
//      m_ctx.logger->Write<LogPriority::ERROR>(
//         "Connection failed: not discoverable, not discovering");
//      m_ctx.gui->ShowToast("Connection failed", 3s, MakeCb([this](ui::IProxy::Error) {
//                              m_ctx.state->emplace<StateIdle>(m_ctx);
//                           }));
//      return;
//   }
//   m_discoveryTimer = m_ctx.timer->ScheduleTimer(30s).Then([this](auto) {
//      m_discovery = m_ctx.bluetooth->CancelDiscovery();
//      m_listening = m_ctx.bluetooth->StopListening();
//      // TODO: turn off discoverability?
//      m_discoveryShutdown =
//         (std::move(m_discovery) && std::move(m_listening)).Then([this](auto) {
//            OnDiscoveryFinished();
//         });
//   });
}

void StateConnecting::OnDiscoveryFinished()
{
//   m_connectingHandles.clear();
//
//   // remove non-connected devices, disconnect and remove non-approved devices
//   std::vector<bt::IProxy::Handle> disconnectRequests;
//   for (auto it = std::begin(m_devices); it != std::end(m_devices);) {
//      bool connected = (it->second & CONNECTED) != 0;
//      bool approved = (it->second & APPROVED) != 0;
//      if (!connected || !approved) {
//         if (connected)
//            disconnectRequests.emplace_back(m_ctx.bluetooth->CloseConnection(it->first));
//         it = m_devices.erase(it);
//      } else {
//         ++it;
//      }
//   }
//
//   // put all others in a vector
//   std::vector<bt::Device> validDevices;
//   validDevices.reserve(m_devices.size());
//   std::transform(std::begin(m_devices), std::end(m_devices), std::back_inserter(validDevices),
//                  [](auto & keyVal) { return keyVal.first; });
//
//   // wait for disconnects to finish, then go to Negotiating
//   if (disconnectRequests.empty()) {
//      m_ctx.state->emplace<StateNegotiating>(m_ctx, std::move(validDevices));
//   } else {
//      for (auto & handle : disconnectRequests)
//         m_disconnects = std::move(handle) && std::move(m_disconnects);
//
//      m_disconnects.Then([this, devices = std::move(validDevices)](auto) mutable {
//         m_ctx.state->emplace<StateNegotiating>(m_ctx, std::move(devices));
//      });
//   }
}

void StateConnecting::OnDeviceConnected(const bt::Device & /*remote*/)
{
//   auto [it, _] = m_devices.try_emplace(remote, 0);
//   it->second |= CONNECTED;
//
//   if ((it->second & APPROVED) == 0) {
//      m_ctx.gui->ShowCandidates({remote}, FIRE_AND_FORGET);
//   }
}

void StateConnecting::OnDeviceDisconnected(const bt::Device & /*remote*/)
{
//   auto [it, _] = m_devices.try_emplace(remote, 0);
//   it->second &= ~CONNECTED;
}

} // namespace fsm
