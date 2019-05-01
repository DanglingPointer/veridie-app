#include <android/log.h>
#include "jni/logger.hpp"

using namespace jni;

namespace {

class Logger : public ILogger
{
public:
   explicit Logger(std::string tag)
      : m_tag(std::move(tag))
   {}
   void Write(LogPriority prio, std::string msg) override
   {
      __android_log_write(static_cast<int>(prio), m_tag.c_str(), msg.c_str());
   }

private:
   std::string m_tag;
};

} // namespace

namespace jni {

std::unique_ptr<ILogger> CreateLogger(std::string tag)
{
   return std::make_unique<Logger>(std::move(tag));
}

} // namespace jni