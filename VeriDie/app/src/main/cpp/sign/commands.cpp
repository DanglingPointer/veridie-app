#include <sstream>
#include "sign/commands.hpp"

namespace cmd::internal {

std::string ToString(const std::string & s)
{
   return s;
}

std::string ToString(const char * s)
{
   return std::string(s);
}

std::string ToString(bool b)
{
   return b ? "true" : "false";
}

std::string ToString(std::string_view s)
{
   return std::string(s);
}

std::string ToString(const dice::Cast & cast)
{
   std::ostringstream ss;
   std::visit(
      [&](const auto & vec) {
         for (const auto & e : vec)
            ss << e << ';';
      },
      cast);
   return ss.str();
}

std::string ToString(std::chrono::seconds sec)
{
   return ToString(sec.count());
}

} // namespace cmd::internal

namespace cmd {

#define DEFINE_NAMEOF(command)                 \
   template <>                                 \
   std::string_view NameOf<command>() noexcept \
   {                                           \
      return #command;                         \
   }

DEFINE_NAMEOF(EnableBluetooth)
DEFINE_NAMEOF(StartListening)
DEFINE_NAMEOF(StartDiscovery)
DEFINE_NAMEOF(StopListening)
DEFINE_NAMEOF(StopDiscovery)
DEFINE_NAMEOF(CloseConnection)
DEFINE_NAMEOF(SendMessage)
DEFINE_NAMEOF(ResetConnections)
DEFINE_NAMEOF(NegotiationStart)
DEFINE_NAMEOF(NegotiationStop)
DEFINE_NAMEOF(ShowAndExit)
DEFINE_NAMEOF(ShowToast)
DEFINE_NAMEOF(ShowNotification)
DEFINE_NAMEOF(ShowRequest)
DEFINE_NAMEOF(ShowResponse)
DEFINE_NAMEOF(ResetGame)

} // namespace cmd