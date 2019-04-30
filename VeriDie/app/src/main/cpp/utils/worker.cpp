#include <thread>
#include "utils/worker.hpp"

Worker::Worker(void * data, ILogger & log)
   : m_queue(std::make_shared<moodycamel::BlockingConcurrentQueue<Worker::Task>>())
   , m_log(log)
{
   Launch(data);
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
   std::thread([queue = m_queue, arg] {
      for (;;) {
         Worker::Task t;
         queue->wait_dequeue(t);
         t(arg);
      }
   }).detach();
}