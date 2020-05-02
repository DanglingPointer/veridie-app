#ifndef FSM_CONTEXT_HPP
#define FSM_CONTEXT_HPP

#include <variant>

namespace core {
class ITimerEngine;
class Proxy;
}
namespace dice {
class IEngine;
class ISerializer;
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
   core::ITimerEngine * const timer;
   core::Proxy * const proxy;

   StateHolder * const state;
};

} // namespace fsm


#endif // FSM_CONTEXT_HPP
