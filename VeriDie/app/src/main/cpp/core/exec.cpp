#include "core/exec.hpp"
#include "core/controller.hpp"
#include "jni/logger.hpp"
#include "jni/btproxy.hpp"
#include "jni/uiproxy.hpp"
#include "dice/engine.hpp"
#include "utils/worker.hpp"

namespace {

Worker & MainWorker()
{
   static main::ServiceLocator s_svc;
   static Worker s_w(&s_svc, s_svc.GetLogger());
   return s_w;
}

} // namespace

namespace main {

ServiceLocator::ServiceLocator()
   : m_logger(jni::CreateLogger("MAIN_WORKER"))
   , m_engine(dice::CreateUniformEngine())
   , m_btProxy(jni::CreateBtProxy())
   , m_uiProxy(jni::CreateUiProxy())
   , m_ctrl(main::CreateController(*m_logger, *m_btProxy, *m_uiProxy, *m_engine))
{}

bt::IListener & ServiceLocator::GetBtListener()
{
   return *m_ctrl;
}

ui::IListener & ServiceLocator::GetUiListener()
{
   return *m_ctrl;
}

void Exec(std::function<void(main::ServiceLocator *)> task)
{
   MainWorker().ScheduleTask([t = std::move(task)](void * data) {
      t(static_cast<main::ServiceLocator *>(data));
   });
}

} // namespace main