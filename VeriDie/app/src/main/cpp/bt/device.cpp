#include "bt/device.hpp"

using namespace bt;

Uuid bt::UuidFromLong(uint64_t lsl, uint64_t msl) noexcept
{
   Uuid data(msl);
   data <<= 64;
   data |= lsl;
   return data;
}

std::pair<uint64_t, uint64_t> bt::UuidToLong(const Uuid & uuid) noexcept
{
   uint64_t lsl = (uuid & Uuid(ULLONG_MAX)).to_ullong();
   uint64_t msl = (uuid >> 64).to_ullong();
   return std::make_pair(lsl, msl);
}