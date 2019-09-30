#include "core/controller.hpp"
#include "core/logging.hpp"
#include "core/timerengine.hpp"
#include "bt/proxy.hpp"
#include "ui/proxy.hpp"
#include "dice/engine.hpp"
#include "fsm/states.hpp"

namespace {

class Controller : public main::IController
{
public:
   Controller(ILogger & logger, bt::IProxy & btProxy, ui::IProxy & uiProxy, dice::IEngine & engine,
              main::ITimerEngine & timer)
      : m_logger(logger)
      , m_generator(engine)
      , m_timer(timer)
      , m_bluetooth(btProxy)
      , m_gui(uiProxy)
      , m_serializer(dice::CreateXmlSerializer())
   {
      m_state.emplace<fsm::StateIdle>(fsm::Context{&m_logger, &m_generator, m_serializer.get(),
                                                   &m_timer, &m_bluetooth, &m_gui, &m_state});
   }

private: /*bt::IListener*/
   void OnBluetoothOn() override
   {
      StateInvoke([](auto & s) { s.OnBluetoothOn(); });
   };
   void OnBluetoothOff() override
   {
      StateInvoke([](auto & s) { s.OnBluetoothOff(); });
   };
   void OnDeviceFound(const bt::Device & remote, bool paired) override
   {
      StateInvoke([&](auto & s) { s.OnDeviceFound(remote, paired); });
   };
   void OnDiscoverabilityConfirmed() override
   {
      StateInvoke([](auto & s) { s.OnDiscoverabilityConfirmed(); });
   };
   void OnDiscoverabilityRejected() override
   {
      StateInvoke([](auto & s) { s.OnDiscoverabilityRejected(); });
   };
   void OnScanModeChanged(bool discoverable, bool connectable) override
   {
      StateInvoke([=](auto & s) { s.OnScanModeChanged(discoverable, connectable); });
   };
   void OnDeviceConnected(const bt::Device & remote) override
   {
      StateInvoke([&](auto & s) { s.OnDeviceConnected(remote); });
   };
   void OnDeviceDisconnected(const bt::Device & remote) override
   {
      StateInvoke([&](auto & s) { s.OnDeviceDisconnected(remote); });
   };
   void OnMessageReceived(const bt::Device & remote, std::string message) override
   {
      StateInvoke([&](auto & s) mutable { s.OnMessageReceived(remote, std::move(message)); });
   };

private: /*ui::IListener*/
   void OnDevicesQuery(bool connected, bool discovered) override
   {
      StateInvoke([=](auto & s) { s.OnDevicesQuery(connected, discovered); });
   };
   void OnNameSet(std::string name) override
   {
      StateInvoke([&](auto & s) mutable { s.OnNameSet(name); });
   };
   void OnLocalNameQuery() override
   {
      StateInvoke([](auto & s) { s.OnLocalNameQuery(); });
   };
   void OnCastRequest(dice::Request localRequest) override
   {
      StateInvoke([&](auto & s) mutable { s.OnCastRequest(std::move(localRequest)); });
   };
   void OnCandidateApproved(bt::Device candidatePlayer) override
   {
      StateInvoke([&](auto & s) { s.OnCandidateApproved(candidatePlayer); });
   };
   void OnNewGame() override
   {
      StateInvoke([](auto & s) { s.OnNewGame(); });
   };
   void OnRestoringState() override{
      // TODO: create m_state from string (after reading it from a file)
   };
   void OnSavingState() override{
      // TODO: save state to a string a write to file
   };

private:
   template <typename F>
   void StateInvoke(F && f)
   {
      struct Workaround : F
      {
         using F::operator();
         void operator()(std::monostate) {}
      };
      std::visit(Workaround{std::forward<F>(f)}, m_state);
   }

   ILogger & m_logger;
   dice::IEngine & m_generator;
   main::ITimerEngine & m_timer;
   bt::IProxy & m_bluetooth;
   ui::IProxy & m_gui;
   std::unique_ptr<dice::ISerializer> m_serializer;

   fsm::StateHolder m_state;
};

} // namespace

namespace main {

std::unique_ptr<IController> CreateController(ILogger & logger, bt::IProxy & btProxy,
                                              ui::IProxy & uiProxy, dice::IEngine & engine,
                                              main::ITimerEngine & timer)
{
   return std::make_unique<Controller>(logger, btProxy, uiProxy, engine, timer);
}

} // namespace main
