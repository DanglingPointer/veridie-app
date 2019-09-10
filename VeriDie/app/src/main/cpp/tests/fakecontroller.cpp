#include "core/controller.hpp"
#include "core/logging.hpp"
#include "bt/proxy.hpp"
#include "ui/proxy.hpp"

namespace {
using namespace std::chrono_literals;

class FakeController : public main::IController
{
public:
   FakeController(ILogger & logger, bt::IProxy & btProxy, ui::IProxy & uiProxy)
      : m_logger(logger)
      , m_bluetooth(btProxy)
      , m_gui(uiProxy)
   {}

   std::vector<ui::IProxy::Handle> handles;

private: /*bt::IListener*/
   void OnBluetoothOn() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnBluetoothOff() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnDeviceFound(const bt::Device & remote, bool paired) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnDiscoverabilityConfirmed() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnDiscoverabilityRejected() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnScanModeChanged(bool discoverable, bool connectable) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnDeviceConnected(const bt::Device & remote) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnDeviceDisconnected(const bt::Device & remote) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnMessageReceived(const bt::Device & remote, std::string message) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };

private: /*ui::IListener*/
   void OnDevicesQuery(bool connected, bool discovered) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      std::string con = connected ? "true" : "false";
      std::string disc = discovered ? "true" : "false";
      handles.push_back(m_gui.ShowToast(__func__ + con + disc, 2s));
   };
   void OnNameSet(std::string name) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__ + name, 2s));
   };
   void OnLocalNameQuery() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnCastRequest(dice::Request localRequest) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);

      std::string d = "invalid";
      size_t size = 0;
      if (auto cast = std::get_if<dice::D6>(&localRequest.cast)) {
         d = "D6";
         size = cast->size();
      }
      handles.push_back(m_gui.ShowToast(__func__ + d + std::to_string(size) +
                                           std::to_string(localRequest.threshold.value_or(0)),
                                        2s));
   };
   void OnCandidateApproved(bt::Device candidatePlayer) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__ + candidatePlayer.mac, 2s));
   };
   void OnNewGame() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnRestoringState() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };
   void OnSavingState() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      handles.push_back(m_gui.ShowToast(__func__, 2s));
   };

private:
   ILogger & m_logger;
   bt::IProxy & m_bluetooth;
   ui::IProxy & m_gui;
};

} // namespace

namespace main {

std::unique_ptr<IController> CreateController(ILogger & logger, bt::IProxy & btProxy,
                                              ui::IProxy & uiProxy, dice::IEngine & engine,
                                              main::TimerEngine & timer)
{
   return std::make_unique<FakeController>(logger, btProxy, uiProxy);
}

} // namespace main
