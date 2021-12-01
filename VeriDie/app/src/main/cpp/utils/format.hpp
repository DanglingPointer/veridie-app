#ifndef FORMAT_HPP
#define FORMAT_HPP

#include <cassert>
#include <charconv>
#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>

namespace fmt {
namespace internal {

template <typename T>
concept Arithmetic =
   std::is_arithmetic_v<std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, bool>;

template <Arithmetic T>
std::span<char> WriteAsText(T arg, std::span<char> dest)
{
   assert(!dest.empty());
   const auto [last, _] = std::to_chars(&dest[0], &dest[dest.size()], arg);
   return dest.subspan(std::distance(dest.data(), last));
}
inline std::span<char> WriteAsText(char arg, std::span<char> dest)
{
   assert(!dest.empty());
   dest[0] = arg;
   return dest.subspan(1);
}
inline std::span<char> WriteAsText(std::string_view arg, std::span<char> dest)
{
   assert(!dest.empty());
   const size_t length = std::min(arg.size(), dest.size());
   std::copy_n(arg.cbegin(), length, dest.begin());
   return dest.subspan(length);
}
inline std::span<char> WriteAsText(void * arg, std::span<char> dest)
{
   assert(!dest.empty());
   static constexpr std::string_view prefix = "0x";
   dest = WriteAsText(prefix, dest);
   const auto [last, _] =
      std::to_chars(&dest[0], &dest[dest.size()], reinterpret_cast<uintptr_t>(arg), 16);
   return dest.subspan(std::distance(dest.data(), last));
}
inline std::span<char> WriteAsText(bool arg, std::span<char> dest)
{
   assert(!dest.empty());
   static constexpr std::string_view t = "true";
   static constexpr std::string_view f = "false";
   return arg ? WriteAsText(t, dest) : WriteAsText(f, dest);
}
inline std::span<char> WriteAsText(const char * arg, std::span<char> dest)
{
   assert(!dest.empty());
   return WriteAsText(std::string_view(arg), dest);
}
inline std::span<char> WriteAsText(const std::string & arg, std::span<char> dest)
{
   assert(!dest.empty());
   return WriteAsText(std::string_view(arg), dest);
}
// clang-format off
template <typename T>
concept Writable = requires(T && arg, std::span<char> buf) {
   { WriteAsText(std::forward<T>(arg), buf) } -> std::same_as<std::span<char>>;
};
// clang-format on

inline size_t ParsePlaceholder(std::string_view from)
{
   static constexpr std::string_view placeholder = "{}";
   return from.starts_with(placeholder) ? placeholder.size() : 0;
}
inline size_t CountPlaceholders(std::string_view fmt)
{
   size_t count = 0;
   for (size_t i = 0; i < fmt.size(); ++i)
      count += ParsePlaceholder(fmt.substr(i)) ? 1 : 0;
   return count;
}
inline std::tuple<std::string_view, std::span<char>> CopyUntilPlaceholder(std::string_view src,
                                                                          std::span<char> dest)
{
   size_t srcPos = 0;
   size_t destPos = 0;
   while (srcPos < src.size() && destPos < dest.size()) {
      const size_t placeholderLength = ParsePlaceholder(src.substr(srcPos));
      if (placeholderLength > 0) {
         srcPos += placeholderLength;
         break;
      }
      dest[destPos++] = src[srcPos++];
   }
   return {src.substr(srcPos), dest.subspan(destPos)};
}

} // namespace internal

template <typename T>
concept Formattable = internal::Writable<T>;

template <Formattable... Ts>
std::span<char> Format(std::span<char> buffer, std::string_view fmt, Ts &&... args)
{
   using namespace internal;
   assert(CountPlaceholders(fmt) == sizeof...(Ts));
   auto ProcessArg = [&](auto && arg) {
      std::tie(fmt, buffer) = CopyUntilPlaceholder(fmt, buffer);
      if (!buffer.empty())
         buffer = WriteAsText(std::forward<decltype(arg)>(arg), buffer);
      return !buffer.empty();
   };
   (... && ProcessArg(std::forward<Ts>(args)));
   std::tie(fmt, buffer) = CopyUntilPlaceholder(fmt, buffer);
   return buffer;
}

} // namespace fmt

#endif
