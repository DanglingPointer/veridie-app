#ifndef WORKER_HPP
#define WORKER_HPP

#include <functional>
#include <memory>
#include <atomic>
#include "utils/blockingconcurrentqueue.h"
#include "core/logging.hpp"

class Worker
{
public:
   using Task = std::function<void(void *)>;

   // starts executing and never stops, even after Worker is destructed
   Worker(void * data, ILogger & log);
   ~Worker();
   void ScheduleTask(Task item);

private:
   void Launch(void * arg);
   std::shared_ptr<moodycamel::BlockingConcurrentQueue<Task>> m_queue;
   std::shared_ptr<std::atomic_bool> m_stop;
   ILogger & m_log;
};

#endif // WORKER_HPP
