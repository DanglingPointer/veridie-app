#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <string>
#include <memory>
#include "bt/listener.hpp"
#include "ui/listener.hpp"

class ILogger;

namespace bt {
class IProxy;
}
namespace ui {
class IProxy;
}
namespace dice {
class IEngine;
}

namespace main {

class IController
   : public bt::IListener
   , public ui::IListener
{
public:
   virtual ~IController() = default;
};

std::unique_ptr<IController> CreateController(ILogger & logger, bt::IProxy & btProxy,
                                              ui::IProxy & uiProxy, dice::IEngine & engine);

} // namespace main

#endif // CONTROLLER_HPP
