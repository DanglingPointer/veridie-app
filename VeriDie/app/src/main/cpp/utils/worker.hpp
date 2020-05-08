#ifndef WORKER_HPP
#define WORKER_HPP

#include <functional>
#include "utils/blockingconcurrentqueue.h"
#include "utils/logger.hpp"

class Worker
{
public:
   using Task = std::function<void(void *)>;

   Worker(void * data, ILogger & log);
   ~Worker();
   void ScheduleTask(Task item);

private:
   void Launch(void * arg);

   using Queue = moodycamel::BlockingConcurrentQueue<Task>;
   Queue * const m_queue;
   bool * const m_stop;
   ILogger & m_log;
};

#endif // WORKER_HPP
