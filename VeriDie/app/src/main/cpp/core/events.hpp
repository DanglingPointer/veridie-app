#ifndef CORE_EVENTS_HPP
#define CORE_EVENTS_HPP

#include <string>
#include <string_view>
#include <vector>
#include "fsm/context.hpp"

namespace event {

using Handler = bool(*)(fsm::StateHolder &, const std::vector<std::string> &);

// event IDs must be in sync with interop/Event.java

struct BluetoothStateChanged
{
   static constexpr int32_t ID = 10;
   static bool Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args);
};

struct RemoteDeviceStateChanged
{
   static constexpr int32_t ID = 11;
   static bool Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args);
};

struct ConnectivityEstablished
{
   static constexpr int32_t ID = 12;
   static bool Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args);
};

struct NewGameRequested
{
   static constexpr int32_t ID = 13;
   static bool Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args);
};

struct MessageReceived
{
   static constexpr int32_t ID = 14;
   static bool Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args);
};

struct CastRequestIssued
{
   static constexpr int32_t ID = 15;
   static bool Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args);
};

struct GameStopped
{
   static constexpr int32_t ID = 16;
   static bool Handle(fsm::StateHolder & stateHolder, const std::vector<std::string> & args);
};


template<typename... T> struct List {};

using Dictionary = List<
   BluetoothStateChanged,
   RemoteDeviceStateChanged,
   ConnectivityEstablished,
   NewGameRequested,
   MessageReceived,
   CastRequestIssued,
   GameStopped>;

}

#endif //CORE_EVENTS_HPP
