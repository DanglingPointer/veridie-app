#include <chrono>

#include "fsm/states.hpp"
#include "core/logging.hpp"

namespace fsm {
using namespace std::chrono_literals;

StateIdle::StateIdle(const Context & ctx)
   : m_ctx(ctx)
   , m_newGamePending(false)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   CheckBtState();
}

void StateIdle::OnBluetoothOn()
{
   m_toastRepeater.Cancel();
   if (m_newGamePending)
      m_ctx.state->emplace<StateConnecting>(m_ctx);
}

void StateIdle::OnBluetoothOff()
{
   if (m_toastRepeater.IsActive())
      return;
//   m_ctx.gui->ShowToast("Please, turn Bluetooth on", 3s, MakeCb([this](ui::IProxy::Error e) {
//      if (e != ui::IProxy::Error::NO_ERROR)
//         m_ctx.logger->Write<LogPriority::ERROR>(
//             "StateIdle::OnBluetoothOff(): Toast failed,", ui::ToString(e));
//   }));
   m_toastRepeater = m_ctx.timer->ScheduleTimer(10s).Then([this](auto) { CheckBtState(); });
}

void StateIdle::OnNewGame()
{
   m_newGamePending = true;
//   CheckBtState();
}

void StateIdle::CheckBtState()
{
//   m_btQuery = m_ctx.bluetooth->IsBluetoothEnabled().Then([this](std::optional<bool> on) {
//      if (!on || !*on)
//         OnBluetoothOff();
//      else
//         OnBluetoothOn();
//   });
}

} // namespace fsm