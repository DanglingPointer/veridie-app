#ifdef __ANDROID__
#include <android/log.h>
#else
#include <cstdio>
#endif
#include "utils/logger.hpp"

namespace {

class Logger : public ILogger
{
public:
   explicit Logger(std::string tag)
      : m_tag(std::move(tag))
   {}
   void Write(LogPriority prio, std::string msg) override
   {
#ifdef __ANDROID__
      __android_log_write(static_cast<int>(prio), m_tag.c_str(), msg.c_str());
#else
      fprintf(stdout, "Prio(%d): %s\n", static_cast<int>(prio), msg.c_str());
#endif
   }

private:
   std::string m_tag;
};

} // namespace

std::unique_ptr<ILogger> CreateLogger(std::string tag)
{
   return std::make_unique<Logger>(std::move(tag));
}