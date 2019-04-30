#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <string_view>

enum class LogPriority
{
   DEFAULT = 1,
   VERBOSE,
   DEBUG,
   INFO,
   WARN,
   ERROR,
   FATAL
};

class ILogger
{
public:
   virtual ~ILogger() = default;

   virtual void Write(LogPriority prio, std::string_view msg) = 0;
};

#endif // LOGGING_HPP
