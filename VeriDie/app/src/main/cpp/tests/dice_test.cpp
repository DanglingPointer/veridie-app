#include <gtest/gtest.h>
#include "dice/engine.hpp"

TEST(DiceTest, generate_result)
{
   dice::Cast sequence = dice::D6(10);
   for (const auto & val : std::get<dice::D6>(sequence)) {
      ASSERT_EQ((uint32_t)val, 0U);
   }
   auto engine = dice::CreateUniformEngine();
   engine->GenerateResult(sequence);
   for (const auto & val : std::get<dice::D6>(sequence)) {
      ASSERT_GE((uint32_t)val, 1U);
      ASSERT_LE((uint32_t)val, 6U);
   }
}