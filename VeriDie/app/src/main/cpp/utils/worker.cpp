#include <thread>
#include "utils/worker.hpp"

Worker::Worker(void * data, ILogger & log)
   : m_queue(std::make_shared<Queue>(Queue::BLOCK_SIZE * 2))
   , m_stop(std::make_shared<bool>(false))
   , m_log(log)
{
   Launch(data);
}

Worker::~Worker()
{
   ScheduleTask([stop = std::move(m_stop)] (void *) {
      *stop = true;
   });
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
   std::thread([queue = m_queue, stop = m_stop, arg] {
      while (!*stop) {
         Worker::Task t;
         queue->wait_dequeue(t);
         t(arg);
      }
   }).detach();
}