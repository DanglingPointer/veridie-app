#include <gtest/gtest.h>
#include "dice/cast.hpp"

TEST(DiceTest, generate_result)
{
   dice::Cast sequence = dice::D6(10);
   for (const auto & val : std::get<dice::D6>(sequence)) {
      ASSERT_EQ((uint32_t)val, 0U);
   }

   auto engine = dice::CreateUniformEngine();
   engine->GenerateResult(sequence);
   for (const auto & val : std::get<dice::D6>(sequence)) {
      ASSERT_NE((uint32_t)val, 0U);
   }
}