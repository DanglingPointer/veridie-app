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
