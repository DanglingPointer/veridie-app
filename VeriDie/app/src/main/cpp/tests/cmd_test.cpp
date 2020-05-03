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

using TestResponse =
   ResponseCodeSubset<ResponseCode::OK, ResponseCode::INVALID_STATE, ResponseCode::BLUETOOTH_OFF>;

using TestCommand = CommonBase<(199 << 8), TestResponse, int, dice::Cast, std::string>;


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

   const int64_t expectedResponse = 0LL;
   std::optional<int64_t> responseValue;

   TestCommand cmd(MakeCb([&](TestResponse r) {
                      responseValue = r.Value();
                   }),
                   42,
                   cast,
                   "<Wow>No comments</Wow>");

   EXPECT_EQ((199 << 8), cmd.GetId());
   EXPECT_EQ(3U, cmd.GetArgsCount());
   EXPECT_EQ("42", cmd.GetArgAt(0));
   EXPECT_EQ("0;0;0;0;", cmd.GetArgAt(1));
   EXPECT_EQ("<Wow>No comments</Wow>", cmd.GetArgAt(2));

   cmd.Respond(expectedResponse);
   EXPECT_TRUE(responseValue);
   EXPECT_EQ(expectedResponse, *responseValue);
}

TEST_F(CmdFixture, invalid_response_throws_exception)
{
   auto cast = dice::MakeCast("D6", 4);
   TestCommand cmd(MakeCb([&](TestResponse r) {
                      ADD_FAILURE() << "Responded to: " << r.Value();
                   }),
                   42,
                   cast,
                   "<Wow>No comments</Wow>");

   EXPECT_THROW(cmd.Respond(123456789), std::invalid_argument);
}

TEST_F(CmdFixture, command_names_printed_correctly)
{
   EXPECT_STREQ("StartListening", cmd::NameOf<cmd::StartListening>().data());
   EXPECT_STREQ("CloseConnection", cmd::NameOf<cmd::CloseConnection>().data());
   EXPECT_STREQ("SendMessage", cmd::NameOf<cmd::SendMessage>().data());
   EXPECT_STREQ("ShowAndExit", cmd::NameOf<cmd::ShowAndExit>().data());
   EXPECT_STREQ("ShowRequest", cmd::NameOf<cmd::ShowRequest>().data());
   EXPECT_STREQ("ShowResponse", cmd::NameOf<cmd::ShowResponse>().data());
}


} // namespace