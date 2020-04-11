#include "core/exec.hpp"
#include "core/controller.hpp"
#include "core/timerengine.hpp"
#include "core/events.hpp"
#include "jni/logger.hpp"
#include "jni/proxy.hpp"
#include "dice/engine.hpp"
#include "dice/serializer.hpp"
#include "utils/worker.hpp"

namespace {

void MainExecutor(std::function<void()> task);

Worker & MainWorker()
{
   static auto s_logger = jni::CreateLogger("MAIN_WORKER");
   static auto s_ctrl = main::CreateController(
      jni::CreateProxy(*s_logger), dice::CreateUniformEngine(),
      main::CreateTimerEngine(MainExecutor), dice::CreateXmlSerializer(), *s_logger);

   static Worker s_w(s_ctrl.get(), *s_logger);
   return s_w;
}

void MainExecutor(std::function<void()> task)
{
   MainWorker().ScheduleTask([t = std::move(task)](void *) {
      t();
   });
}

} // namespace

namespace main {

void InternalExec(std::function<void(IController *)> task)
{
   MainWorker().ScheduleTask([t = std::move(task)](void * data) {
      t(static_cast<IController *>(data));
   });
}

} // namespace main