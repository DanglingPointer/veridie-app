#include "core/log.hpp"

#include <cstdio>
#include <ctime>

namespace {

void StdLog(FILE * file, char what, const char * tag, const char * text)
{
   char timeBuf[64];
   const std::time_t t = std::time(nullptr);
   [[maybe_unused]] const int ret1 =
      std::strftime(std::data(timeBuf), std::size(timeBuf), "%D %T", std::gmtime(&t));
   assert(ret1 > 0);

   fprintf(file, "%s %c/%s: %s\n", std::data(timeBuf), what, tag, text);
}

size_t ParsePlaceholder(std::string_view from)
{
   static constexpr std::string_view placeholder = "{}";
   return from.starts_with(placeholder) ? placeholder.size() : 0;
}

} // namespace


Log::ArgBuffer Log::ArgBuffer::From(bool arg) noexcept
{
   static constexpr char t[] = "true";
   static constexpr char f[] = "false";
   ArgBuffer buffer;
   if (arg)
      buffer.m_iface = {std::data(t), std::size(t) - 1};
   else
      buffer.m_iface = {std::data(f), std::size(f) - 1};
   return buffer;
}

Log::ArgBuffer Log::ArgBuffer::From(char arg) noexcept
{
   ArgBuffer buffer;
   buffer.m_storage[0] = arg;
   buffer.m_iface = {std::data(buffer.m_storage), 1};
   return buffer;
}

Log::ArgBuffer Log::ArgBuffer::From(const char * arg)
{
   ArgBuffer buffer;
   buffer.m_iface = {arg};
   return buffer;
}

Log::ArgBuffer Log::ArgBuffer::From(const std::string & arg)
{
   ArgBuffer buffer;
   buffer.m_iface = arg;
   return buffer;
}

Log::ArgBuffer Log::ArgBuffer::From(std::string_view arg) noexcept
{
   ArgBuffer buffer;
   buffer.m_iface = arg;
   return buffer;
}

Log::Handler Log::s_debugHandler = nullptr;
Log::Handler Log::s_infoHandler = nullptr;
Log::Handler Log::s_warningHandler = nullptr;
Log::Handler Log::s_errorHandler = nullptr;
Log::Handler Log::s_fatalHandler = nullptr;

void Log::Debug(const char * tag, const char * text)
{
   if (s_debugHandler)
      s_debugHandler(tag, text);
   else
      StdLog(stdout, 'D', tag, text);
}

void Log::Info(const char * tag, const char * text)
{
   if (s_infoHandler)
      s_infoHandler(tag, text);
   else
      StdLog(stdout, 'I', tag, text);
}

void Log::Warning(const char * tag, const char * text)
{
   if (s_warningHandler)
      s_warningHandler(tag, text);
   else
      StdLog(stderr, 'W', tag, text);
}

void Log::Error(const char * tag, const char * text)
{
   if (s_errorHandler)
      s_errorHandler(tag, text);
   else
      StdLog(stderr, 'E', tag, text);
}

void Log::Fatal(const char * tag, const char * text)
{
   if (s_fatalHandler)
      s_fatalHandler(tag, text);
   else
      StdLog(stderr, 'F', tag, text);
   std::abort();
}

size_t Log::WriteBuffer(const ArgBuffer & src, std::span<char> dest)
{
   const std::string_view formattedArg = src.View();
   const size_t lengthToCopy = std::min(formattedArg.size(), dest.size());
   std::memcpy(dest.data(), formattedArg.data(), lengthToCopy);
   return lengthToCopy;
}

std::array<char, Log::MAX_LINE_LENGTH + 1> Log::FormatArgs(std::span<const ArgBuffer> args,
                                                           std::string_view fmt)
{
   std::array<char, MAX_LINE_LENGTH + 1> retBuf;
   std::span<char> logLine{retBuf.begin(), MAX_LINE_LENGTH};

   size_t srcPos = 0;
   size_t destPos = 0;
   size_t nextArgInd = 0;

   while (srcPos < fmt.size() && destPos < logLine.size()) {
      const size_t placeholderLength = ParsePlaceholder(fmt.substr(srcPos));
      if (placeholderLength > 0) {
         if (nextArgInd >= args.size())
            throw std::out_of_range(__PRETTY_FUNCTION__);
         const size_t written = WriteBuffer(args[nextArgInd++], logLine.subspan(destPos));
         srcPos += placeholderLength;
         destPos += written;
      } else {
         logLine[destPos++] = fmt[srcPos++];
      }
   }
   retBuf[destPos] = '\0';
   return retBuf;
}
