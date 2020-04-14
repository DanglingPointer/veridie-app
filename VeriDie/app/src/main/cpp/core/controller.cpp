#include <unordered_map>

#include "core/controller.hpp"
#include "core/logging.hpp"
#include "jni/proxy.hpp"
#include "dice/engine.hpp"
#include "core/timerengine.hpp"
#include "dice/serializer.hpp"
#include "sign/events.hpp"
#include "fsm/states.hpp"

namespace {

class Controller : public main::IController
{
public:
   Controller(ILogger & logger,
              std::unique_ptr<jni::IProxy> proxy,
              std::unique_ptr<dice::IEngine> engine,
              std::unique_ptr<main::ITimerEngine> timer,
              std::unique_ptr<dice::ISerializer> serializer)
      : m_logger(logger)
      , m_proxy(std::move(proxy))
      , m_generator(std::move(engine))
      , m_timer(std::move(timer))
      , m_serializer(std::move(serializer))
   {
      InitializeEventHandlers(event::Dictionary{});
      m_state.emplace<fsm::StateIdle>(fsm::Context{&m_logger,
                                                   m_generator.get(),
                                                   m_serializer.get(),
                                                   m_timer.get(),
                                                   m_proxy.get(),
                                                   &m_state});
   }

private:
   void OnEvent(int32_t eventId, const std::vector<std::string> & args) override
   {
      auto it = m_eventHandlers.find(eventId);
      if (it == std::cend(m_eventHandlers)) {
         m_logger.Write<LogPriority::ERROR>("Event handler not found, id=", eventId);
         return;
      }
      bool success = (*it->second)(m_state, args);
      if (!success) {
         m_logger.Write<LogPriority::ERROR>("Could not parse event args, id=", eventId);
      }
   }

   template <typename... Event>
   void InitializeEventHandlers(event::List<Event...>)
   {
      (..., m_eventHandlers.insert({Event::ID, &Event::Handle}));
      assert(m_eventHandlers.size() == sizeof...(Event));
   }

   ILogger & m_logger;
   std::unique_ptr<jni::IProxy> m_proxy;
   std::unique_ptr<dice::IEngine> m_generator;
   std::unique_ptr<main::ITimerEngine> m_timer;
   std::unique_ptr<dice::ISerializer> m_serializer;

   std::unordered_map<int32_t, event::Handler> m_eventHandlers;
   fsm::StateHolder m_state;
};

} // namespace

namespace main {

std::unique_ptr<IController> CreateController(std::unique_ptr<jni::IProxy> proxy,
                                              std::unique_ptr<dice::IEngine> engine,
                                              std::unique_ptr<main::ITimerEngine> timer,
                                              std::unique_ptr<dice::ISerializer> serializer,
                                              ILogger & logger)
{
   return std::make_unique<Controller>(logger,
                                       std::move(proxy),
                                       std::move(engine),
                                       std::move(timer),
                                       std::move(serializer));
}

} // namespace main
