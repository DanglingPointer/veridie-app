#ifndef EXEC_HPP
#define EXEC_HPP

#include <memory>
#include <functional>

namespace dice {
class IEngine;
}
class ILogger;

namespace main {

class ServiceLocator
{
public:
   ServiceLocator();
   ILogger & GetLogger() { return *m_logger; }
   dice::IEngine & GetDiceEngine() { return *m_engine; }

private:
   std::unique_ptr<ILogger> m_logger;
   std::unique_ptr<dice::IEngine> m_engine;
};

void Exec(std::function<void(ServiceLocator *)> task);

} // namespace main


#endif // EXEC_HPP
