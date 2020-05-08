#ifndef SIGN_COMMANDS_HPP
#define SIGN_COMMANDS_HPP

#include <array>
#include <chrono>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include "dice/cast.hpp"
#include "utils/callback.hpp"

namespace cmd {
namespace internal {

template <typename... Ts>
struct CreateVariant
{
   template <typename T, typename... TArgs>
   static std::variant<Ts...> Match(int64_t value, T /*first*/, TArgs... args)
   {
      if (value == T::value)
         return T{};
      return Match(value, args...);
   }
   static std::variant<Ts...> Match(int64_t /*value*/)
   {
      throw std::invalid_argument("Invalid command response");
   }

   static std::variant<Ts...> FromValue(int64_t value) { return Match(value, Ts{}...); }
};

} // namespace internal


class ICommand
{
public:
   virtual ~ICommand() = default;
   virtual int32_t GetId() const = 0;
   virtual std::string_view GetName() const = 0;
   virtual size_t GetArgsCount() const = 0;
   virtual std::string_view GetArgAt(size_t index) const = 0;
   virtual void Respond(int64_t response) = 0;
};


template <typename TTraits>
class Base : public ICommand
{
   static_assert(TTraits::ID >= (100 << 8));
   static_assert(TTraits::LONG_BUFFER_SIZE >= TTraits::SHORT_BUFFER_SIZE);
   using LongBuffer = std::array<char, TTraits::LONG_BUFFER_SIZE>;
   using ShortBuffer = std::array<char, TTraits::SHORT_BUFFER_SIZE>;

public:
   using Response = typename TTraits::Response;
   using ParamTuple = typename TTraits::ParamTuple;

   static constexpr int32_t ID = TTraits::ID;
   static constexpr size_t ARG_SIZE = std::tuple_size_v<ParamTuple>;
   static constexpr size_t MAX_BUFFER_SIZE = TTraits::LONG_BUFFER_SIZE - 1;


   template <typename... Ts>
   Base(async::Callback<Response> && cb, Ts &&... params)
      : Base(std::move(cb), ParamTuple(std::forward<Ts>(params)...))
   {}

   Base(async::Callback<Response> && cb, ParamTuple params);
   int32_t GetId() const noexcept override;
   std::string_view GetName() const noexcept override;
   size_t GetArgsCount() const noexcept override;
   std::string_view GetArgAt(size_t index) const noexcept override;
   void Respond(int64_t response) override;

private:
   async::Callback<Response> m_cb;
   std::array<LongBuffer, (ARG_SIZE != 0)> m_longArgs{};
   std::array<ShortBuffer, (ARG_SIZE > 1) ? (ARG_SIZE - 1) : 0U> m_shortArgs{};
};


template <int32_t Id, typename TResponse, typename... TParams>
struct Traits
{
   static constexpr int32_t ID = Id;
   using Command = Base<Traits<Id, TResponse, TParams...>>;
   using Response = TResponse;
   using ParamTuple = std::tuple<
      std::conditional_t<(!std::is_trivially_copyable_v<TParams> || sizeof(TParams) > 16U),
                         const TParams &,
                         TParams>...>;

   static constexpr size_t LONG_BUFFER_SIZE = 32U;
   static constexpr size_t SHORT_BUFFER_SIZE = 24U;
};

template <int32_t Id, typename TResponse, typename... TParams>
struct LongTraits : Traits<Id, TResponse, TParams...>
{
   static constexpr size_t LONG_BUFFER_SIZE = 256U;
   static constexpr size_t SHORT_BUFFER_SIZE = 32U;
};

template <int32_t Id, typename TResponse, typename... TParams>
struct ExtraLongTraits : Traits<Id, TResponse, TParams...>
{
   static constexpr size_t LONG_BUFFER_SIZE = 1024U;
   static constexpr size_t SHORT_BUFFER_SIZE = 32U;
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

// clang-format off

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
// the largest parameter type must be first

#define COMMON_RESPONSES \
   ResponseCode::OK, ResponseCode::INVALID_STATE

#define ID(id) \
   (id << 8)


using StartListeningResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::BLUETOOTH_OFF,
   ResponseCode::USER_DECLINED,
   ResponseCode::LISTEN_FAILED>;
using StartListeningTraits = LongTraits<
   ID(100),
   StartListeningResponse,
   std::string_view /*uuid*/, std::string_view /*name*/, std::chrono::seconds /*discoverability duration*/>;
using StartListening = Base<StartListeningTraits>;


using StartDiscoveryResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::NO_BT_ADAPTER,
   ResponseCode::BLUETOOTH_OFF>;
using StartDiscoveryTraits = LongTraits<
   ID(101),
   StartDiscoveryResponse,
   std::string_view /*uuid*/, std::string_view /*name*/, bool /*include paired*/>;
using StartDiscovery = Base<StartDiscoveryTraits>;


using StopListeningResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using StopListeningTraits = Traits<
   ID(102),
   StopListeningResponse>;
using StopListening = Base<StopListeningTraits>;


using StopDiscoveryResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using StopDiscoveryTraits = Traits<
   ID(103),
   StopDiscoveryResponse>;
using StopDiscovery = Base<StopDiscoveryTraits>;


using CloseConnectionResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::CONNECTION_NOT_FOUND>;
using CloseConnectionTraits = Traits<
   ID(104),
   CloseConnectionResponse,
   std::string_view/*error msg*/, std::string_view/*remote mac addr*/>;
using CloseConnection = Base<CloseConnectionTraits>;


using EnableBluetoothResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::NO_BT_ADAPTER,
   ResponseCode::USER_DECLINED>;
using EnableBluetoothTraits = Traits<
   ID(105),
   EnableBluetoothResponse>;
using EnableBluetooth = Base<EnableBluetoothTraits>;


using NegotiationStartResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using NegotiationStartTraits = Traits<
   ID(106),
   NegotiationStartResponse>;
using NegotiationStart = Base<NegotiationStartTraits>;


using NegotiationStopResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using NegotiationStopTraits = Traits<
   ID(107),
   NegotiationStopResponse,
   std::string_view/*nominee name*/>;
using NegotiationStop = Base<NegotiationStopTraits>;


using SendMessageResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::CONNECTION_NOT_FOUND,
   ResponseCode::SOCKET_ERROR>;
using SendMessageTraits = LongTraits<
   ID(108),
   SendMessageResponse,
   std::string_view/*message*/, std::string_view/*remote mac addr*/>;
using SendMessage = Base<SendMessageTraits>;


using SendLongMessageTraits = ExtraLongTraits<
   ID(108),
   SendMessageResponse,
   std::string_view/*message*/, std::string_view/*remote mac addr*/>;
using SendLongMessage = Base<SendLongMessageTraits>;


using ShowAndExitResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using ShowAndExitTraits = LongTraits<
   ID(109),
   ShowAndExitResponse,
   std::string_view>;
using ShowAndExit = Base<ShowAndExitTraits>;


using ShowToastResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using ShowToastTraits = Traits<
   ID(110),
   ShowToastResponse,
   std::string_view, std::chrono::seconds>;
using ShowToast = Base<ShowToastTraits>;


using ShowNotificationResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using ShowNotificationTraits = Traits<
   ID(111),
   ShowNotificationResponse,
   std::string_view>;
using ShowNotification = Base<ShowNotificationTraits>;


using ShowRequestResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using ShowRequestTraits = Traits<
   ID(112),
   ShowRequestResponse,
   std::string_view/*type*/, size_t/*size*/, uint32_t/*threshold, 0=not set*/, std::string_view/*name*/>;
using ShowRequest = Base<ShowRequestTraits>;


using ShowResponseResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using ShowResponseTraits = LongTraits<
   ID(113),
   ShowResponseResponse,
   dice::Cast/*numbers*/, std::string_view/*type*/, int32_t/*success count, -1=not set*/, std::string_view/*name*/>;
using ShowResponse = Base<ShowResponseTraits>;


using ShowLongResponseTraits = ExtraLongTraits<
   ID(113),
   ShowResponseResponse,
   dice::Cast/*numbers*/, std::string_view/*type*/, int32_t/*success count, -1=not set*/, std::string_view/*name*/>;
using ShowLongResponse = Base<ShowLongResponseTraits>;


using ResetGameResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using ResetGameTraits = Traits<
   ID(114),
   ResetGameResponse>;
using ResetGame = Base<ResetGameTraits>;


using ResetConnectionsResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;
using ResetConnectionsTraits = Traits<
   ID(115),
   ResetConnectionsResponse>;
using ResetConnections = Base<ResetConnectionsTraits>;


#undef COMMON_RESPONSES
#undef ID


template <typename... T> struct List {};

using BtDictionary = List<
   EnableBluetooth,
   StartListening,
   StartDiscovery,
   StopListening,
   StopDiscovery,
   CloseConnection,
   SendMessage,
   SendLongMessage,
   ResetConnections>;

using UiDictionary = List<
   NegotiationStart,
   NegotiationStop,
   ShowAndExit,
   ShowToast,
   ShowNotification,
   ShowRequest,
   ShowResponse,
   ShowLongResponse,
   ResetGame>;


} // namespace cmd

#endif // SIGN_COMMANDS_HPP
