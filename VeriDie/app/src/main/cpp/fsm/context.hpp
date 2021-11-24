#ifndef FSM_CONTEXT_HPP
#define FSM_CONTEXT_HPP

#include "core/commandadapter.hpp"
#include <variant>

namespace dice {
class IEngine;
class ISerializer;
}
namespace async {
class Timer;
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
   async::Timer * const timer;
   core::CommandAdapter proxy;

   StateHolder * const state;
};

} // namespace fsm


#endif // FSM_CONTEXT_HPP
