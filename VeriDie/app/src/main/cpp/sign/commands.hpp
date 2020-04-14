#ifndef CORE_COMMANDS_HPP
#define CORE_COMMANDS_HPP

#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include "dice/cast.hpp"
#include "utils/callback.hpp"

namespace cmd {
namespace internal {

std::string ToString(const std::string & s);
std::string ToString(const char * s);
std::string ToString(bool b);
std::string ToString(std::string_view s);
std::string ToString(const dice::Cast & cast);
std::string ToString(std::chrono::seconds sec);
template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>>>
std::string ToString(T s)
{
   return std::to_string(s);
}

template<typename... Ts>
struct CreateVariant
{
   template<typename T, typename... TArgs>
   static std::variant<Ts...> Match(int64_t value, T /*first*/, TArgs... args)
   {
      if (value == T::value)
         return T{};
      return Match(value, args...);
   }
   static std::variant<Ts...> Match(int64_t value)
   {
      throw std::invalid_argument(std::to_string(value));
   }

   static std::variant<Ts...> FromValue(int64_t value)
   {
      return Match(value, Ts{}...);
   }
};

} // namespace internal


class ICommand
{
public:
   virtual ~ICommand() = default;
   virtual int32_t GetId() const = 0;
   virtual size_t GetArgsCount() const = 0;
   virtual std::string_view GetArgAt(size_t index) const = 0;
   virtual void Respond(int64_t response) = 0;
};

template <int32_t Id, typename Response, typename... Params>
class CommonBase : public ICommand
{
   static_assert(Id >= 100);

public:
   static constexpr int32_t ID = Id;
   using Cb = async::Callback<Response>;

   CommonBase(async::Callback<Response> && cb, Params... params)
      : m_cb(std::move(cb))
      , m_args({internal::ToString(params)...})
   {}
   int32_t GetId() const override { return ID; }
   size_t GetArgsCount() const override { return m_args.size(); }
   std::string_view GetArgAt(size_t index) const override { return m_args[index]; }
   void Respond(int64_t response) override { m_cb.InvokeOneShot(Response(response)); }

private:
   async::Callback<Response> m_cb;
   std::array<std::string, sizeof...(Params)> m_args;
};


// response codes must be in sync with interop/Command.java

struct ResponseCode final
{
   template <int64_t V>
   using Code = std::integral_constant<int64_t, V>;

   using OK = Code<0>;
   using INVALID_STATE = Code<0xff'ff'ff'ff'ff'ff'ff'ffLL>;
   using BLUETOOTH_OFF = Code<2>;
   using LISTEN_FAILED = Code<3>;
   using CONNECTION_NOT_FOUND = Code<4>;
   using NO_BT_ADAPTER = Code<5>;
   using USER_DECLINED = Code<6>;
   using SOCKET_ERROR = Code<7>;
};

template <typename... Cs>
class ResponseCodeSubset
{
public:
   ResponseCodeSubset(int64_t code)
      : m_code(internal::CreateVariant<Cs...>::FromValue(code))
   {}
   template <typename... Fs>
   void Handle(Fs... funcs)
   {
      struct Overload : Fs... { using Fs::operator()...; };
      std::visit(Overload{funcs...}, m_code);
   }
   int64_t Value() const
   {
      return std::visit([](auto code) { return code(); }, m_code);
   }

private:
   std::variant<Cs...> m_code;
};


// command IDs must be in sync with interop/Command.java

#define COMMON_RESPONSES \
   ResponseCode::OK, ResponseCode::INVALID_STATE

#define ID(id) \
   (id << 8)


using StartListeningResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::BLUETOOTH_OFF,
   ResponseCode::LISTEN_FAILED>;

using StartListening = CommonBase<
   ID(100),
   StartListeningResponse,
   std::string /*uuid*/, std::string /*name*/, std::chrono::seconds /*discoverability duration*/>;


using StartDiscoveryResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::BLUETOOTH_OFF>;

using StartDiscovery = CommonBase<
   ID(101),
   StartDiscoveryResponse,
   std::string /*uuid*/, std::string /*name*/, bool /*include paired*/>;


using StopListeningResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using StopListening = CommonBase<
   ID(102),
   StopListeningResponse>;


using StopDiscoveryResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using StopDiscovery = CommonBase<
   ID(103),
   StopDiscoveryResponse>;


using CloseConnectionResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::CONNECTION_NOT_FOUND>;

using CloseConnection = CommonBase<
   ID(104),
   CloseConnectionResponse,
   std::string/*remote mac addr*/, std::string/*error msg*/>;


using EnableBluetoothResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::NO_BT_ADAPTER,
   ResponseCode::USER_DECLINED>;

using EnableBluetooth = CommonBase<
   ID(105),
   EnableBluetoothResponse>;


using NegotiationStartResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using NegotiationStart = CommonBase<
   ID(106),
   NegotiationStartResponse>;


using NegotiationStopResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using NegotiationStop = CommonBase<
   ID(107),
   NegotiationStopResponse>;


using SendMessageResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::CONNECTION_NOT_FOUND,
   ResponseCode::SOCKET_ERROR>;

using SendMessage = CommonBase<
   ID(108),
   SendMessageResponse,
   std::string/*remote mac addr*/, std::string/*message*/>;


using ShowAndExitResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ShowAndExit = CommonBase<
   ID(109),
   ShowAndExitResponse,
   std::string>;


using ShowToastResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ShowToast = CommonBase<
   ID(110),
   ShowToastResponse,
   std::string, std::chrono::seconds>;


using ShowNotificationResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ShowNotification = CommonBase<
   ID(111),
   ShowNotificationResponse,
   std::string>;


using ShowRequestResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ShowRequest = CommonBase<
   ID(112),
   ShowRequestResponse,
   std::string/*type*/, size_t/*size*/, uint32_t/*threshold*/>;


using ShowResponseResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ShowResponse = CommonBase<
   ID(113),
   ShowResponseResponse,
   std::string/*type*/, dice::Cast/*numbers*/, uint32_t/*threshold*/>;


using ResetGameResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ResetGame = CommonBase<
   ID(114),
   ResetGameResponse>;


#undef COMMON_RESPONSES
#undef ID


template <typename... T> struct List {};

using BtDictionary = List<
   StartListening,
   StartDiscovery,
   StopListening,
   StopDiscovery,
   CloseConnection,
   SendMessage>;

using UiDictionary = List<
   EnableBluetooth,
   NegotiationStart,
   NegotiationStop,
   ShowAndExit,
   ShowToast,
   ShowNotification,
   ShowRequest,
   ShowResponse,
   ResetGame>;


} // namespace cmd

#endif // CORE_COMMANDS_HPP
