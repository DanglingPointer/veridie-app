#include <gtest/gtest.h>
#include "bt/device.hpp"

namespace {

TEST(BtTest, uuid_conversions_are_correct)
{
   const uint64_t msl = 0xfedcba4353928745ULL;
   const uint64_t lsl = 0x12957bc987ef45a3ULL;

   bt::Uuid uuid = bt::UuidFromLong(lsl, msl);
   auto[retLsl, retMsl] = bt::UuidToLong(uuid);

   ASSERT_EQ(msl, retMsl);
   ASSERT_EQ(lsl, retLsl);
}

} // namespace