#include <sstream>
#include <unordered_map>
#include <utility>

#include "core/controller.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "core/proxy.hpp"
#include "dice/engine.hpp"
#include "dice/serializer.hpp"
#include "sign/events.hpp"
#include "fsm/states.hpp"

namespace {

using EventHandlerMap = std::unordered_map<int32_t, std::pair<std::string_view, event::Handler>>;

template <typename... Events>
EventHandlerMap CreateEventHandlers(event::List<Events...>)
{
   return EventHandlerMap{{Events::ID, {Events::NAME, &Events::Handle}}...};
}

class Controller : public core::IController
{
public:
   Controller(ILogger & logger,
              std::unique_ptr<dice::IEngine> engine,
              std::unique_ptr<async::Timer> timer,
              std::unique_ptr<dice::ISerializer> serializer)
      : m_logger(logger)
      , m_proxy(nullptr)
      , m_generator(std::move(engine))
      , m_timer(std::move(timer))
      , m_serializer(std::move(serializer))
      , m_eventHandlers(CreateEventHandlers(event::Dictionary{}))
   {}

private:
   void Start(std::function<std::unique_ptr<core::Proxy>(ILogger &)> proxyBuilder) override
   {
      if (m_proxy)
         return;
      m_proxy = proxyBuilder(m_logger);
      m_state.emplace<fsm::StateIdle>(fsm::Context{&m_logger,
                                                   m_generator.get(),
                                                   m_serializer.get(),
                                                   m_timer.get(),
                                                   m_proxy.get(),
                                                   &m_state});
   }
   void OnEvent(int32_t eventId, const std::vector<std::string> & args) override
   {
      auto it = m_eventHandlers.find(eventId);
      if (it == std::cend(m_eventHandlers)) {
         m_logger.Write<LogPriority::FATAL>("Event handler not found, id=", eventId);
         return;
      }
      std::string_view name = it->second.first;
      event::Handler handler = it->second.second;

      std::ostringstream ss;
      ss << "<<<<< " << name;
      for (const auto & s : args)
         ss << " [" << s << "]";
      m_logger.Write(LogPriority::INFO, ss.str());

      bool success = (*handler)(m_state, args);
      if (!success) {
         m_logger.Write<LogPriority::ERROR>("Could not parse event args");
      }
   }

   ILogger & m_logger;
   std::unique_ptr<core::Proxy> m_proxy;
   std::unique_ptr<dice::IEngine> m_generator;
   std::unique_ptr<async::Timer> m_timer;
   std::unique_ptr<dice::ISerializer> m_serializer;

   EventHandlerMap m_eventHandlers;
   fsm::StateHolder m_state;
};

} // namespace

namespace core {

std::unique_ptr<IController> CreateController(std::unique_ptr<dice::IEngine> engine,
                                              std::unique_ptr<async::Timer> timer,
                                              std::unique_ptr<dice::ISerializer> serializer,
                                              ILogger & logger)
{
   return std::make_unique<Controller>(logger,
                                       std::move(engine),
                                       std::move(timer),
                                       std::move(serializer));
}

} // namespace core
