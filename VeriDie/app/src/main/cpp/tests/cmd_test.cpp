#include <gtest/gtest.h>
#include "utils/canceller.hpp"
#include "core/commands.hpp"
#include "dice/serializer.hpp"

namespace {
using namespace cmd;

class CmdFixture
   : public ::testing::Test
   , protected async::Canceller<>
{
protected:
   CmdFixture() = default;
};

using TestResponse =
   ResponseCodeSubset<ResponseCode::OK, ResponseCode::INVALID_STATE, ResponseCode::BLUETOOTH_OFF>;

using TestCommand = CommonBase<199, TestResponse, bt::Uuid, dice::Cast, std::string>;


TEST_F(CmdFixture, response_code_subset_maps_values_correctly)
{
   TestResponse r1(ResponseCode::OK::value);
   r1.Handle(
      [](ResponseCode::OK) {
         // OK
      },
      [](ResponseCode::INVALID_STATE) {
         ADD_FAILURE();
      },
      [](ResponseCode::BLUETOOTH_OFF) {
         ADD_FAILURE();
      });
   EXPECT_EQ(ResponseCode::OK::value, r1.Value());

   TestResponse r2(ResponseCode::INVALID_STATE::value);
   r2.Handle(
      [](ResponseCode::OK) {
         ADD_FAILURE();
      },
      [](ResponseCode::INVALID_STATE) {
         // OK
      },
      [](ResponseCode::BLUETOOTH_OFF) {
         ADD_FAILURE();
      });
   EXPECT_EQ(ResponseCode::INVALID_STATE::value, r2.Value());

   TestResponse r3(ResponseCode::BLUETOOTH_OFF::value);
   r3.Handle(
      [](ResponseCode::OK) {
         ADD_FAILURE();
      },
      [](ResponseCode::INVALID_STATE) {
         ADD_FAILURE();
      },
      [](ResponseCode::BLUETOOTH_OFF) {
         // OK
      });
   r3.Handle(
      [](ResponseCode::BLUETOOTH_OFF) {
         // OK
      },
      [](auto) {
         ADD_FAILURE();
      });
   EXPECT_EQ(ResponseCode::BLUETOOTH_OFF::value, r3.Value());

   EXPECT_THROW({ TestResponse r4(42); }, std::invalid_argument);
}

TEST_F(CmdFixture, common_base_stores_arguments_and_responds_correctly)
{
   auto cast = dice::MakeCast("D6", 4);
   auto uuid = bt::UuidFromLong(123, 456);

   const int64_t expectedResponse = 0LL;
   std::optional<int64_t> responseValue;

   TestCommand cmd(MakeCb([&](TestResponse r) {
                      responseValue = r.Value();
                   }),
                   uuid,
                   cast,
                   "<Wow>No comments</Wow>");

   EXPECT_EQ(199, cmd.GetId());
   EXPECT_EQ(3U, cmd.GetArgsCount());
   EXPECT_EQ("456;123", cmd.GetArgAt(0));
   EXPECT_EQ("0;0;0;0;", cmd.GetArgAt(1));
   EXPECT_EQ("<Wow>No comments</Wow>", cmd.GetArgAt(2));

   cmd.Respond(expectedResponse);
   EXPECT_TRUE(responseValue);
   EXPECT_EQ(expectedResponse, *responseValue);
}


} // namespace