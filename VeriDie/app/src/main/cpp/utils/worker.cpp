#include <memory>
#include <stdexcept>
#include <thread>
#include "utils/worker.hpp"

Worker::Worker(void * data, ILogger & log)
   : m_queue(new Queue(Queue::BLOCK_SIZE * 2))
   , m_stop(new bool(false))
   , m_log(log)
{
   Launch(data);
}

Worker::~Worker()
{
   ScheduleTask([stop = m_stop](void *) { *stop = true; });
}

void Worker::ScheduleTask(Worker::Task item)
{
   if (!m_queue->enqueue(std::move(item))) {
      m_log.Write(LogPriority::FATAL, "Internal worker error: failed to enqueue task");
      std::abort();
   }
}

void Worker::Launch(void * arg)
{
   std::thread([queue = std::unique_ptr<Queue>(m_queue), stop = std::unique_ptr<bool>(m_stop), arg,
                &log = m_log] {
      log.Write(LogPriority::INFO, "Hello world!");
      Worker::Task t;
      while (!*stop) {
         try {
            queue->wait_dequeue(t);
            t(arg);
         }
         catch (const std::exception & e) {
            log.Write(LogPriority::WARN, std::string("Uncaught exception: ") + e.what());
         }
         catch (...) {
            log.Write(LogPriority::WARN, "Uncaught exception: UNKNOWN");
         }
      }
      log.Write(LogPriority::INFO, "Worker shut down");
   }).detach();
}