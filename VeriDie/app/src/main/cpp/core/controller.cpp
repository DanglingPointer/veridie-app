#include "core/controller.hpp"
#include "core/logging.hpp"
#include "bt/proxy.hpp"
#include "ui/proxy.hpp"
#include "dice/engine.hpp"

namespace {

class Controller : public main::IController
{
public:
   Controller(ILogger & logger, bt::IProxy & btProxy, ui::IProxy & uiProxy, dice::IEngine & engine)
      : m_logger(logger)
      , m_generator(engine)
      , m_bluetooth(btProxy)
      , m_gui(uiProxy)
   {}

private: /*bt::IListener*/
   void OnBluetoothOn() override{};
   void OnBluetoothOff() override{};
   void OnDeviceFound(std::string name, std::string mac, bool paired) override{};
   void OnDiscoverabilityConfirmed() override{};
   void OnDiscoverabilityRejected() override{};
   void OnScanModeChanged(bool discoverable, bool connectable) override{};
   void OnDeviceConnected(std::string name, std::string mac) override{};
   void OnDeviceDisconnected(std::string mac) override{};
   void OnMessageReceived(std::string mac, std::string message) override{};

private: /*ui::IListener*/
   // TODO

private:
   ILogger & m_logger;
   dice::IEngine & m_generator;
   bt::IProxy & m_bluetooth;
   ui::IProxy & m_gui;
};

} // namespace

namespace main {

std::unique_ptr<IController> CreateController(ILogger & logger, bt::IProxy & btProxy, ui::IProxy & uiProxy, dice::IEngine & engine)
{
   return std::make_unique<Controller>(logger, btProxy, uiProxy, engine);
}

} // namespace main
