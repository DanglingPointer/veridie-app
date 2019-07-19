#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <bitset>
#include <functional>
#include <string>

namespace bt {

using Uuid = std::bitset<128>;

Uuid UuidFromLong(uint64_t lsl, uint64_t msl) noexcept;

std::pair<uint64_t/*lsl*/, uint64_t/*msl*/> UuidToLong(const Uuid & uuid) noexcept;

struct Device
{
   Device(std::string name, std::string mac)
      : name(std::move(name))
      , mac(std::move(mac))
   {}
   Device(const Device&) = default;
   Device(Device&&) noexcept = default;
   std::string name;
   std::string mac;

   bool operator==(const Device & rhs) const noexcept { return mac == rhs.mac; }
   bool operator!=(const Device & rhs) const noexcept { return !(*this == rhs); }
};
} // namespace bt

namespace std {

template <>
struct hash<bt::Device>
{
   using argument_type = bt::Device;
   using result_type = size_t;
   result_type operator()(const argument_type & device) const noexcept
   {
      return std::hash<std::string>{}(device.mac);
   }
};
} // namespace std

#endif // DEVICE_HPP
