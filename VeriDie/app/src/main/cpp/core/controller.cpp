#include "core/controller.hpp"
#include "core/logging.hpp"
#include "core/timerengine.hpp"
#include "bt/proxy.hpp"
#include "ui/proxy.hpp"
#include "dice/engine.hpp"

namespace {

class Controller : public main::IController
{
public:
   Controller(ILogger & logger, bt::IProxy & btProxy, ui::IProxy & uiProxy, dice::IEngine & engine,
              main::TimerEngine & timer)
      : m_logger(logger)
      , m_generator(engine)
      , m_timer(timer)
      , m_bluetooth(btProxy)
      , m_gui(uiProxy)
   {}

private: /*bt::IListener*/
   void OnBluetoothOn() override{};
   void OnBluetoothOff() override{};
   void OnDeviceFound(const bt::Device & remote, bool paired) override{};
   void OnDiscoverabilityConfirmed() override{};
   void OnDiscoverabilityRejected() override{};
   void OnScanModeChanged(bool discoverable, bool connectable) override{};
   void OnDeviceConnected(const bt::Device & remote) override{};
   void OnDeviceDisconnected(const bt::Device & remote) override{};
   void OnMessageReceived(const bt::Device & remote, std::string message) override{};

private: /*ui::IListener*/
   void OnDevicesQuery(bool connected, bool discovered) override{};
   void OnNameSet(std::string name) override{};
   void OnLocalNameQuery() override{};
   void OnCastRequest(dice::Request localRequest) override{};
   void OnCandidateApproved(bt::Device candidatePlayer) override{};
   void OnNewGame() override{};
   void OnRestoringState() override{};
   void OnSavingState() override{};

private:
   ILogger & m_logger;
   dice::IEngine & m_generator;
   main::TimerEngine & m_timer;
   bt::IProxy & m_bluetooth;
   ui::IProxy & m_gui;
};

} // namespace

namespace main {

std::unique_ptr<IController> CreateController(ILogger & logger, bt::IProxy & btProxy,
                                              ui::IProxy & uiProxy, dice::IEngine & engine,
                                              main::TimerEngine & timer)
{
   return std::make_unique<Controller>(logger, btProxy, uiProxy, engine, timer);
}

} // namespace main
