#ifndef CORE_CONTROLLER_HPP
#define CORE_CONTROLLER_HPP

#include <string>
#include <memory>

class ILogger;

namespace dice {
class IEngine;
class ISerializer;
}

namespace core {
class ITimerEngine;
class Proxy;

class IController
{
public:
   virtual ~IController() = default;
   virtual void OnEvent(int32_t eventId, const std::vector<std::string> & args) = 0;
};

std::unique_ptr<IController> CreateController(std::unique_ptr<core::Proxy> proxy,
                                              std::unique_ptr<dice::IEngine> engine,
                                              std::unique_ptr<core::ITimerEngine> timer,
                                              std::unique_ptr<dice::ISerializer> serializer,
                                              ILogger & logger);

} // namespace core

#endif // CORE_CONTROLLER_HPP
