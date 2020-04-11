#ifndef CORE_COMMANDS_HPP
#define CORE_COMMANDS_HPP

#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include "dice/cast.hpp"
#include "bt/device.hpp"
#include "utils/callback.hpp"

namespace cmd {
namespace details {

inline std::string ToString(const std::string & s)
{
   return s;
}
inline std::string ToString(const char * s)
{
   return std::string(s);
}
inline std::string ToString(std::string_view s)
{
   return std::string(s);
}
template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>>>
std::string ToString(T s)
{
   return std::to_string(s);
}
inline std::string ToString(const dice::Cast & cast)
{
   std::ostringstream ss;
   std::visit([&](const auto & vec) {
      for (const auto & e : vec)
         ss << e << ';';
   }, cast);
   return ss.str();
}
inline std::string ToString(const bt::Uuid & uuid)
{
   auto[lsl, msl] = bt::UuidToLong(uuid);
   return std::to_string(msl) + ";" + std::to_string(lsl);
}

} // namespace details


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
      , m_args({details::ToString(params)...})
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

enum class ResponseCode : int64_t
{
   OK = 0,
   INVALID_STATE = 0xff'ff'ff'ff'ff'ff'ff'ffLL,
   BLUETOOTH_OFF = 2,
   LISTEN_FAILED = 3,
   CONNECTION_NOT_FOUND = 4,
   NO_BT_ADAPTER = 5,
   USER_DECLINED = 6,
   SOCKET_ERROR = 7
};

template <ResponseCode... Cs>
struct ResponseCodeSubset
{
public:
   ResponseCodeSubset(int64_t code) noexcept
      : m_code(code)
   {}
   int64_t GetRaw() const noexcept { return m_code; }
   std::optional<ResponseCode> GetCode() const
   {
      bool found = ((m_code == static_cast<int64_t>(Cs)) || ...);
      return found ? std::make_optional(ResponseCode(m_code)) : std::nullopt;
   }

private:
   int64_t m_code;
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
   bt::Uuid, std::string /*name*/>;


using StartDiscoveryResponse = ResponseCodeSubset<
   COMMON_RESPONSES,
   ResponseCode::BLUETOOTH_OFF>;

using StartDiscovery = CommonBase<
   ID(101),
   StartDiscoveryResponse,
   bt::Uuid, std::string /*name*/>;


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


using ShowTextResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ShowText = CommonBase<
   ID(109),
   ShowTextResponse,
   std::string>;


using ShowToastResponse = ResponseCodeSubset<
   COMMON_RESPONSES>;

using ShowToast = CommonBase<
   ID(110),
   ShowToastResponse,
   std::string, int>;


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


#undef COMMON_RESPONSES
#undef ID


template <typename... T> struct List {};

using BtDictionary = List<
   StartListening,
   StartDiscovery,
   StopListening,
   StopDiscovery,
   CloseConnection,
   EnableBluetooth,
   SendMessage>;

using UiDictionary = List<
   NegotiationStart,
   NegotiationStop,
   ShowText,
   ShowToast,
   ShowNotification,
   ShowRequest,
   ShowResponse>;


} // namespace cmd

#endif // CORE_COMMANDS_HPP
