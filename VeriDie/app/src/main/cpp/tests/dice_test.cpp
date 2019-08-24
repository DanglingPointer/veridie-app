#include <gtest/gtest.h>
#include "dice/engine.hpp"
#include "dice/serializer.hpp"

namespace {

TEST(DiceTest, generate_result)
{
   dice::Cast sequence = dice::D6(100);
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

TEST(DiceTest, count_success)
{
   dice::D8 sequence(10);
   size_t i = 0;
   for (auto & val : sequence) {
      val(i++);
   }
   const uint32_t threshold = 6;
   ASSERT_EQ(dice::GetSuccessCount(sequence, threshold), 4U);
}

TEST(DiceTest, deserialize_request)
{
   auto slzr = dice::CreateXmlSerializer();

   {
      std::string msg = R"(<Request type="D4" size="10" successFrom="3" />)";
      dice::Request r = slzr->ParseRequest(msg);
      auto * cast = std::get_if<dice::D4>(&r.cast);
      ASSERT_TRUE(cast);
      ASSERT_EQ(10U, cast->size());
      for (const auto & val : *cast) {
         ASSERT_EQ((uint32_t)val, 0U);
      }
      ASSERT_TRUE(r.threshold);
      ASSERT_EQ(3U, *r.threshold);
   }
   {
      std::string msg = R"(<Request type="D4" size="10" />)";
      dice::Request r = slzr->ParseRequest(msg);
      auto * cast = std::get_if<dice::D4>(&r.cast);
      ASSERT_TRUE(cast);
      ASSERT_EQ(10U, cast->size());
      for (const auto & val : *cast) {
         ASSERT_EQ((uint32_t)val, 0U);
      }
      ASSERT_FALSE(r.threshold);
   }
}

TEST(DiceTest, deserialize_response)
{
   auto slzr = dice::CreateXmlSerializer();

   {
      std::string msg = R"(<Response type="D12" size="5" successCount="3">
                           <Val>1</Val>
                           <Val>2</Val>
                           <Val>3</Val>
                           <Val>4</Val>
                           <Val>5</Val>
                        </Response>)";
      dice::Response r = slzr->ParseResponse(msg);
      auto * cast = std::get_if<dice::D12>(&r.cast);
      ASSERT_TRUE(cast);
      ASSERT_EQ(5U, cast->size());
      for (int i = 0; i < 5; ++i) {
         ASSERT_EQ(i + 1, cast->at(i));
      }
      ASSERT_TRUE(r.successCount);
      ASSERT_EQ(3U, *r.successCount);
   }
   {
      std::string msg = R"(<Response type="D12" size="5">
                           <Val>1</Val>
                           <Val>2</Val>
                           <Val>3</Val>
                           <Val>4</Val>
                           <Val>5</Val>
                        </Response>)";
      dice::Response r = slzr->ParseResponse(msg);
      auto * cast = std::get_if<dice::D12>(&r.cast);
      ASSERT_TRUE(cast);
      ASSERT_EQ(5U, cast->size());
      for (int i = 0; i < 5; ++i) {
         ASSERT_EQ(i + 1, cast->at(i));
      }
      ASSERT_FALSE(r.successCount);
   }
}

TEST(DiceTest, serialize_and_deserialize_request)
{
   auto slzr = dice::CreateXmlSerializer();

   dice::D20 d(15);
   uint32_t successFrom = 5U;
   dice::Request r{d, successFrom};

   std::string serialized = slzr->CreateRequest(r);

   dice::Request r1 = slzr->ParseRequest(serialized);
   auto * cast = std::get_if<dice::D20>(&r1.cast);
   ASSERT_TRUE(cast);
   ASSERT_EQ(d, *cast);
   ASSERT_TRUE(r1.threshold);
   ASSERT_EQ(successFrom, *r1.threshold);
}

TEST(DiceTest, serialize_and_deserialize_response)
{
   auto slzr = dice::CreateXmlSerializer();

   dice::D100 d(6);
   for (int i = 0; i < 6; ++i) {
      d[i](7 - i);
   }
   uint32_t successCount = 1U;
   dice::Response r{d, successCount};

   std::string serialized = slzr->CreateResponse(r);

   dice::Response r1 = slzr->ParseResponse(serialized);
   auto * cast = std::get_if<dice::D100>(&r1.cast);
   ASSERT_TRUE(cast);
   ASSERT_EQ(d, *cast);
   ASSERT_TRUE(r1.successCount);
   ASSERT_EQ(successCount, *r1.successCount);
}

} // namespace