#ifndef FSM_CONTEXT_HPP
#define FSM_CONTEXT_HPP

#include <variant>

namespace main {
class ITimerEngine;
}
namespace dice {
class IEngine;
class ISerializer;
}
namespace bt {
class IProxy;
}
namespace ui {
class IProxy;
}
class ILogger;

namespace fsm {

class StateIdle;
class StateConnecting;
class StatePlaying;
class StateNegotiating;

using StateHolder =
   std::variant<std::monostate, StateIdle, StateConnecting, StateNegotiating, StatePlaying>;

struct Context
{
   ILogger * const logger;
   dice::IEngine * const generator;
   dice::ISerializer * const serializer;
   main::ITimerEngine * const timer;
   bt::IProxy * const bluetooth;
   ui::IProxy * const gui;

   StateHolder * const state;
};

} // namespace fsm


#endif // FSM_CONTEXT_HPP