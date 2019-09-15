#include <chrono>

#include "fsm/states.hpp"
#include "bt/proxy.hpp"
#include "core/logging.hpp"

namespace fsm {
using namespace std::chrono_literals;

StateIdle::StateIdle(const Context & ctx)
   : m_ctx(ctx)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   CheckBtState();
}

void StateIdle::OnBluetoothOn()
{
   if (m_toastRepeater.IsActive())
      m_toastRepeater.Cancel();
}

void StateIdle::OnBluetoothOff()
{
   OnBtStateError();
}

void StateIdle::OnNewGame()
{
   m_btQuery = m_ctx.bluetooth->IsBluetoothEnabled().Then([this](std::optional<bool> on) {
      if (!on || !*on)
         OnBtStateError();
      else
         m_ctx.state->emplace<StateConnecting>(m_ctx);
   });
}

void StateIdle::CheckBtState()
{
   m_btQuery = m_ctx.bluetooth->IsBluetoothEnabled().Then([this](std::optional<bool> on) {
      if (!on || !*on)
         OnBtStateError();
   });
}

void StateIdle::OnBtStateError()
{
   m_toastHandle = m_ctx.gui->ShowToast("Please, turn Bluetooth on", 3s);
   m_toastRepeater = m_ctx.timer->ScheduleTimer(15s).Then([this](auto) { CheckBtState(); });
}

} // namespace fsm