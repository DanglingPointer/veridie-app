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

namespace async {
class Timer;
}

namespace cmd {
class IExternalInvoker;
}

namespace core {

class IController
{
public:
   virtual ~IController() = default;
   virtual void Start(std::unique_ptr<cmd::IExternalInvoker> uiInvoker,
                      std::unique_ptr<cmd::IExternalInvoker> btInvoker) = 0;
   virtual void OnEvent(int32_t eventId, const std::vector<std::string> & args) = 0;
   virtual void OnCommandResponse(int32_t cmdId, int64_t response) = 0;
};

std::unique_ptr<IController> CreateController(std::unique_ptr<dice::IEngine> engine,
                                              std::unique_ptr<async::Timer> timer,
                                              std::unique_ptr<dice::ISerializer> serializer,
                                              ILogger & logger);

} // namespace core

#endif // CORE_CONTROLLER_HPP
