#include "core/controller.hpp"
#include "core/logging.hpp"
#include "bt/proxy.hpp"
#include "ui/proxy.hpp"

namespace {
using namespace std::chrono_literals;

const std::string & ToString(const std::string & s) { return s; }
std::string ToString(const char * s) { return s; }
template <typename T>
std::string ToString(T s) { return std::to_string(s); }

class FakeController : public main::IController
{
public:
   FakeController(ILogger & logger, bt::IProxy & /*btProxy*/, ui::IProxy & uiProxy)
      : m_logger(logger)
      , m_gui(uiProxy)
   {}

   std::vector<ui::IProxy::Handle> handles;

   template <typename... TArgs>
   void Toast(const char * fun, TArgs... args)
   {
      auto handle = m_gui.ShowToast((fun + ... + ToString(args)), 2s);
      handles.push_back(std::move(handle));
   }

private: /*bt::IListener*/
   void OnBluetoothOn() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnBluetoothOff() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnDeviceFound(const bt::Device & /*remote*/, bool /*paired*/) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnDiscoverabilityConfirmed() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnDiscoverabilityRejected() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnScanModeChanged(bool discoverable, bool connectable) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__, discoverable ? "true" : "false", connectable ? "true" : "false");
   };
   void OnDeviceConnected(const bt::Device & /*remote*/) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnDeviceDisconnected(const bt::Device & /*remote*/) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnMessageReceived(const bt::Device & /*remote*/, std::string /*message*/) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };

private: /*ui::IListener*/
   void OnDevicesQuery(bool connected, bool discovered) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__, connected ? "true" : "false", discovered ? "true" : "false");
   };
   void OnNameSet(std::string name) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__, name);
   };
   void OnLocalNameQuery() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnCastRequest(dice::Request localRequest) override
   {
      std::string d = "invalid";
      size_t size = 0;
      if (auto cast = std::get_if<dice::D6>(&localRequest.cast)) {
         d = "D6";
         size = cast->size();
      }
      m_logger.Write<LogPriority::DEBUG>(__PRETTY_FUNCTION__, d, size,
                                         localRequest.threshold.value_or(0));
      Toast(__func__, d, size, localRequest.threshold.value_or(0));
   };
   void OnCandidateApproved(bt::Device candidatePlayer) override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__, candidatePlayer.mac);
   };
   void OnNewGame() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnRestoringState() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };
   void OnSavingState() override
   {
      m_logger.Write(LogPriority::DEBUG, __PRETTY_FUNCTION__);
      Toast(__func__);
   };

private:
   ILogger & m_logger;
   ui::IProxy & m_gui;
};

} // namespace

namespace main {

std::unique_ptr<IController> CreateController(ILogger & logger, bt::IProxy & btProxy,
                                              ui::IProxy & uiProxy, dice::IEngine &,
                                              main::ITimerEngine &)
{
   return std::make_unique<FakeController>(logger, btProxy, uiProxy);
}

} // namespace main
