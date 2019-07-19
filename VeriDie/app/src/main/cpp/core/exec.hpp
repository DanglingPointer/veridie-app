#ifndef MAIN_EXEC_HPP
#define MAIN_EXEC_HPP

#include <memory>
#include <functional>

namespace dice {
class IEngine;
}
namespace bt {
class IProxy;
class IListener;
}
namespace ui {
class IListener;
class IProxy;
}
class ILogger;

namespace main {
class IController;

class ServiceLocator
{
public:
   ServiceLocator();
   ILogger & GetLogger() { return *m_logger; }
   bt::IListener & GetBtListener();
   ui::IListener & GetUiListener();

private:
   std::unique_ptr<ILogger> m_logger;
   std::unique_ptr<dice::IEngine> m_engine;
   std::unique_ptr<bt::IProxy> m_btProxy;
   std::unique_ptr<ui::IProxy> m_uiProxy;
   std::unique_ptr<main::IController> m_ctrl;
};

void Exec(std::function<void(ServiceLocator *)> task);

} // namespace main


#endif // MAIN_EXEC_HPP
