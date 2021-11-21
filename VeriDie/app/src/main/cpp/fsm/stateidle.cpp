#include <chrono>

#include "sign/commandpool.hpp"
#include "fsm/states.hpp"
#include "fsm/stateswitcher.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "sign/commands.hpp"
#include "core/proxy.hpp"

namespace fsm {
using namespace std::chrono_literals;
using cmd::Make;

StateIdle::StateIdle(const Context & ctx, bool startNewGame)
   : m_ctx(ctx)
   , m_newGamePending(false)
   , m_bluetoothOn(false)
{
   m_ctx.logger->Write<LogPriority::INFO>("New state:", __func__);
   RequestBluetoothOn();
   cmd::pool.ShrinkToFit();

   if (startNewGame)
      OnNewGame();
}

void StateIdle::OnBluetoothOn()
{
   m_bluetoothOn = true;
   if (IsActive(m_enableBtCb))
      CancelCallback(m_enableBtCb);
   if (m_newGamePending) {
      SwitchToState<StateConnecting>(m_ctx);
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
      SwitchToState<StateConnecting>(m_ctx);
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
               StartTask([this] () -> cr::TaskHandle<void> {
                  co_await m_ctx.timer->Start(1s);
                  RequestBluetoothOn();
               }());
            },
            [this](cmd::ResponseCode::NO_BT_ADAPTER) {
               *m_ctx.proxy << Make<cmd::ShowAndExit>(DetachedCb<cmd::ShowAndExitResponse>(),
                                                      "Cannot proceed due to a fatal failure.");
               SwitchToState<std::monostate>(m_ctx);
            },
            [](cmd::ResponseCode::USER_DECLINED) {});
      },
      &m_enableBtCb));
}

} // namespace fsm