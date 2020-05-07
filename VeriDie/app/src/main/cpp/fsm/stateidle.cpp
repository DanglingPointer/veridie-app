#include <chrono>

#include "sign/commandpool.hpp"
#include "fsm/states.hpp"
#include "core/logging.hpp"
#include "core/timerengine.hpp"
#include "sign/commands.hpp"
#include "core/proxy.hpp"

namespace fsm {
using namespace std::chrono_literals;
using cmd::Make;

StateIdle::StateIdle(const Context & ctx)
   : m_ctx(ctx)
   , m_newGamePending(false)
   , m_bluetoothOn(false)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   RequestBluetoothOn();
   cmd::pool.ShrinkToFit();
}

void StateIdle::OnBluetoothOn()
{
   m_bluetoothOn = true;
   if (IsActive(m_enableBtCb))
      CancelCallback(m_enableBtCb);
   if (m_newGamePending) {
      fsm::Context ctx{m_ctx};
      m_ctx.state->emplace<StateConnecting>(ctx);
   }
}

void StateIdle::OnBluetoothOff()
{
   m_bluetoothOn = false;
   if (!IsActive(m_enableBtCb)) {
      RequestBluetoothOn();
   }
}

void StateIdle::OnNewGame()
{
   m_newGamePending = true;
   if (m_bluetoothOn) {
      fsm::Context ctx{m_ctx};
      m_ctx.state->emplace<StateConnecting>(ctx);
   } else if (!IsActive(m_enableBtCb)) {
      RequestBluetoothOn();
   }
}

void StateIdle::RequestBluetoothOn()
{
   *m_ctx.proxy << Make<cmd::EnableBluetooth>(MakeCb(
      [this](cmd::EnableBluetoothResponse result) {
         result.Handle(
            [this](cmd::ResponseCode::OK) {
               OnBluetoothOn();
            },
            [this](cmd::ResponseCode::INVALID_STATE) {
               m_retryHandle = m_ctx.timer->ScheduleTimer(1s).Then([this](auto) {
                  RequestBluetoothOn();
               });
            },
            [this](cmd::ResponseCode::NO_BT_ADAPTER) {
               *m_ctx.proxy << Make<cmd::ShowAndExit>(DetachedCb<cmd::ShowAndExitResponse>(),
                                                      "Cannot proceed due to a fatal failure.");
               m_ctx.state->emplace<std::monostate>();
            },
            [](cmd::ResponseCode::USER_DECLINED) {});
      },
      &m_enableBtCb));
}

} // namespace fsm