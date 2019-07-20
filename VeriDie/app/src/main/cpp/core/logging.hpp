#ifndef CORE_LOGGING_HPP
#define CORE_LOGGING_HPP

#include <string>

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

   virtual void Write(LogPriority prio, std::string msg) = 0;
};

#endif // CORE_LOGGING_HPP
