#ifndef CORE_LOGGING_HPP
#define CORE_LOGGING_HPP

#include <string>
#include <string_view>
#include <type_traits>

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

   template <LogPriority prio, typename... TArgs>
   void Write(TArgs &&... args)
   {
      Write(prio, (ToString(std::forward<TArgs>(args)) + ...));
   }

private:
   static std::string ToString(const std::string & s) { return s + ' '; }
   static std::string ToString(const char * s) { return std::string(s) + ' '; }
   static std::string ToString(std::string_view s) { return std::string(s) + ' '; }
   template <typename T, typename = std::enable_if_t<
       std::is_arithmetic_v<std::decay_t<T>>> >
   static std::string ToString(T s)
   {
      return std::to_string(s) + ' ';
   }
};

#endif // CORE_LOGGING_HPP
