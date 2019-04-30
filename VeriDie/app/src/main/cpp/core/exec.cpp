#include "core/exec.hpp"
#include "jni/logger.hpp"
#include "dice/engine.hpp"
#include "utils/worker.hpp"

namespace {

Worker & MainWorker()
{
   static main::ServiceLocator svc;
   static Worker w(&svc, svc.GetLogger());
   return w;
}

} // namespace

main::ServiceLocator::ServiceLocator()
   : m_logger(jni::CreateLogger("MAIN"))
   , m_engine(dice::CreateUniformEngine())
{}

void main::Exec(std::function<void(main::ServiceLocator *)> task)
{
   MainWorker().ScheduleTask([t = std::move(task)](void * data) {
      t(static_cast<main::ServiceLocator *>(data));
   });
}