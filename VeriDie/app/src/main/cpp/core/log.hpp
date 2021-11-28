#ifndef CORE_LOG_HPP
#define CORE_LOG_HPP

#undef NDEBUG
#include <array>
#include <cassert>
#include <charconv>
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace internal {
template <typename T>
concept Arithmetic =
   std::is_arithmetic_v<std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, bool>;

template <typename... Ts>
concept NonEmpty = sizeof...(Ts) > 0;

} // namespace internal

struct Log final
{
   static void Debug(const char * tag, const char * text);
   static void Info(const char * tag, const char * text);
   static void Warning(const char * tag, const char * text);
   static void Error(const char * tag, const char * text);
   [[noreturn]] static void Fatal(const char * tag, const char * text);


   // clang-format off
   template <typename... Ts> requires internal::NonEmpty<Ts...>
   static void Debug(const char * tag, std::string_view fmt, Ts &&... args)
   {
      auto formattedArgs = FormatArgs(fmt, std::forward<Ts>(args)...);
      Debug(tag, std::data(formattedArgs));
   }
   template <typename... Ts> requires internal::NonEmpty<Ts...>
   static void Info(const char * tag, std::string_view fmt, Ts &&... args)
   {
      auto formattedArgs = FormatArgs(fmt, std::forward<Ts>(args)...);
      Info(tag, std::data(formattedArgs));
   }
   template <typename... Ts> requires internal::NonEmpty<Ts...>
   static void Warning(const char * tag, std::string_view fmt, Ts &&... args)
   {
      auto formattedArgs = FormatArgs(fmt, std::forward<Ts>(args)...);
      Warning(tag, std::data(formattedArgs));
   }
   template <typename... Ts> requires internal::NonEmpty<Ts...>
   static void Error(const char * tag, std::string_view fmt, Ts &&... args)
   {
      auto formattedArgs = FormatArgs(fmt, std::forward<Ts>(args)...);
      Error(tag, std::data(formattedArgs));
   }
   template <typename... Ts> requires internal::NonEmpty<Ts...>
   [[noreturn]] static void Fatal(const char * tag, std::string_view fmt, Ts &&... args)
   {
      auto formattedArgs = FormatArgs(fmt, std::forward<Ts>(args)...);
      Fatal(tag, std::data(formattedArgs));
   }
   // clang-format on

   using Handler = void (*)(const char *, const char *);
   static Handler s_debugHandler;
   static Handler s_infoHandler;
   static Handler s_warningHandler;
   static Handler s_errorHandler;
   static Handler s_fatalHandler;

   static constexpr size_t MAX_LINE_LENGTH = 511;

private:
   static_assert((MAX_LINE_LENGTH + 1) % 64 == 0);

   struct ArgBuffer
   {
      constexpr ArgBuffer() noexcept = default;

      constexpr std::string_view View() const noexcept { return m_iface; }

      template <internal::Arithmetic T>
      static ArgBuffer From(T arg)
      {
         ArgBuffer buffer;
         std::memset(std::data(buffer.m_storage), 0, std::size(buffer.m_storage));
         auto [last, _] =
            std::to_chars(std::begin(buffer.m_storage), std::end(buffer.m_storage), arg);
         assert(last <= std::end(buffer.m_storage));
         buffer.m_iface = {std::data(buffer.m_storage),
                           static_cast<size_t>(std::distance(std::begin(buffer.m_storage), last))};
         return buffer;
      }
      static ArgBuffer From(bool arg) noexcept;
      static ArgBuffer From(char arg) noexcept;
      static ArgBuffer From(const char * arg);
      static ArgBuffer From(const std::string & arg);
      static ArgBuffer From(std::string_view arg) noexcept;

   private:
      std::array<char, 32> m_storage;
      std::string_view m_iface;
   };

   template <typename... Ts>
   static auto FormatArgs(std::string_view fmt, Ts &&... args)
   {
      std::array<char, MAX_LINE_LENGTH + 1> retBuf;
      std::array<ArgBuffer, sizeof...(args)> argBufs{ArgBuffer::From(std::forward<Ts>(args))...};

      char * dest = std::begin(retBuf);
      char * const destEnd = std::prev(std::end(retBuf));

      const char * src = std::cbegin(fmt);
      const char * const srcEnd = std::cend(fmt);

      size_t nextArgInd = 0;

      while (dest != destEnd && src != srcEnd) {
         const auto srcSpaceLeft = std::distance(src, srcEnd);
         const auto destSpaceLeft = std::distance(dest, destEnd);

         if (srcSpaceLeft >= 2 && *src == '{' && *std::next(src) == '}') {
            assert(nextArgInd < argBufs.size());
            const std::string_view formattedArg = argBufs[nextArgInd++].View();
            const auto lengthToCopy = std::min<size_t>(destSpaceLeft, formattedArg.size());

            std::memcpy(dest, formattedArg.data(), lengthToCopy);
            dest += lengthToCopy;
            src += 2;
         } else {
            *dest++ = *src++;
         }
      }
      *dest = '\0';
      return retBuf;
   }
};

#endif
