#ifndef WORKER_HPP
#define WORKER_HPP

#include <functional>
#include <memory>
#include "utils/blockingconcurrentqueue.h"
#include "utils/logging.hpp"

class Worker
{
public:
   using Task = std::function<void(void *)>;

   // starts executing and never stops, even after Worker is destructed
   Worker(void * data, ILogger & log);

   void ScheduleTask(Task item);

private:
   void Launch(void * arg);
   std::shared_ptr<moodycamel::BlockingConcurrentQueue<Task>> m_queue;
   ILogger & m_log;
};

#endif // WORKER_HPP
