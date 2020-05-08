#ifndef CORE_CONTROLLER_HPP
#define CORE_CONTROLLER_HPP

#include <memory>
#include <functional>
#include <string>
#include <vector>

class ILogger;

namespace dice {
class IEngine;
class ISerializer;
} // namespace dice

namespace core {
class ITimerEngine;
class Proxy;

class IController
{
public:
   virtual ~IController() = default;
   virtual void Start(std::function<std::unique_ptr<core::Proxy>(ILogger &)> proxyBuilder) = 0;
   virtual void OnEvent(int32_t eventId, const std::vector<std::string> & args) = 0;
};

std::unique_ptr<IController> CreateController(std::unique_ptr<dice::IEngine> engine,
                                              std::unique_ptr<core::ITimerEngine> timer,
                                              std::unique_ptr<dice::ISerializer> serializer,
                                              ILogger & logger);

} // namespace core

#endif // CORE_CONTROLLER_HPP
