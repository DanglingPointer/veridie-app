#include <gtest/gtest.h>
#include "utils/canceller.hpp"
#include "sign/commands.hpp"
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


TEST_F(CmdFixture, response_code_subset_maps_values_correctly)
{
   StopListeningResponse r1(ResponseCode::OK::value);
   r1.Handle(
      [](ResponseCode::OK) {
         // OK
      },
      [](ResponseCode::INVALID_STATE) {
         ADD_FAILURE();
      });
   EXPECT_EQ(ResponseCode::OK::value, r1.Value());

   StopListeningResponse r2(ResponseCode::INVALID_STATE::value);
   r2.Handle(
      [](ResponseCode::OK) {
         ADD_FAILURE();
      },
      [](ResponseCode::INVALID_STATE) {
         // OK
      });
   EXPECT_EQ(ResponseCode::INVALID_STATE::value, r2.Value());

   EXPECT_THROW({ StopListeningResponse r4(42); }, std::invalid_argument);
}

TEST_F(CmdFixture, common_base_stores_arguments_and_responds_correctly)
{
   auto cast = dice::MakeCast("D6", 4);

   const int64_t expectedResponse = 0LL;
   std::optional<int64_t> responseValue;
   const std::string player1 = "Player 1";

   ShowResponse cmd(MakeCb([&](ShowResponseResponse r) {
                       responseValue = r.Value();
                    }),
                    cast,
                    "D100",
                    2,
                    player1);

   EXPECT_EQ(ShowResponse::ID, cmd.GetId());
   EXPECT_STREQ("ShowResponse", cmd.GetName().data());
   EXPECT_EQ(4U, cmd.GetArgsCount());
   EXPECT_STREQ("0;0;0;0;", cmd.GetArgAt(0).data());
   EXPECT_STREQ("D100", cmd.GetArgAt(1).data());
   EXPECT_STREQ("2", cmd.GetArgAt(2).data());
   EXPECT_STREQ("Player 1", cmd.GetArgAt(3).data());

   cmd.Respond(expectedResponse);
   EXPECT_TRUE(responseValue);
   EXPECT_EQ(expectedResponse, *responseValue);
}

TEST_F(CmdFixture, invalid_response_throws_exception)
{
   NegotiationStart cmd(MakeCb([&](NegotiationStartResponse r) {
      ADD_FAILURE() << "Responded to: " << r.Value();
   }));
   EXPECT_EQ(NegotiationStart::ID, cmd.GetId());
   EXPECT_STREQ("NegotiationStart", cmd.GetName().data());
   EXPECT_EQ(0U, cmd.GetArgsCount());

   EXPECT_THROW(cmd.Respond(123456789), std::invalid_argument);
}

} // namespace