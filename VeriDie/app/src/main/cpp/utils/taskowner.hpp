#ifndef TASK_OWNER_HPP
#define TASK_OWNER_HPP

#include "utils/task.hpp"

#include <vector>

namespace cr {

template <Executor E = InlineExecutor>
class TaskOwner
{
public:
   explicit TaskOwner(E executor = {})
      : m_executor(executor)
   {}

   void StartTask(TaskHandle<void, E> && task)
   {
      std::erase_if(m_tasks, [](const TaskHandle<void, E> & t) {
         return !t;
      });
      m_tasks.template emplace_back(std::move(task));
      m_tasks.back().Run(m_executor);
   }

private:
   E m_executor;
   std::vector<TaskHandle<void, E>> m_tasks;
};

} // namespace cr

#endif
