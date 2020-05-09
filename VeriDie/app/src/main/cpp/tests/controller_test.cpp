#include <gtest/gtest.h>
#include <queue>
#include <list>
#include <sstream>
#include <vector>

#include "tests/fakelogger.hpp"
#include "bt/device.hpp"
#include "core/controller.hpp"
#include "sign/commands.hpp"
#include "sign/commandpool.hpp"
#include "core/timerengine.hpp"
#include "core/proxy.hpp"
#include "dice/engine.hpp"
#include "dice/serializer.hpp"

namespace {
using namespace std::chrono_literals;

class MockProxy : public core::Proxy
{
public:
   mem::pool_ptr<cmd::ICommand> PopNextCommand()
   {
      if (m_q.empty())
         return nullptr;
      auto c = std::move(m_q.front());
      m_q.pop();
      return c;
   }
   bool NoCommands() const { return m_q.empty(); }

private:
   void ForwardCommandToUi(mem::pool_ptr<cmd::ICommand> && c) override { m_q.push(std::move(c)); }
   void ForwardCommandToBt(mem::pool_ptr<cmd::ICommand> && c) override { m_q.push(std::move(c)); }

private:
   std::queue<mem::pool_ptr<cmd::ICommand>> m_q;
};

class MockTimerEngine : public core::ITimerEngine
{
public:
   MockTimerEngine()
      : m_processing(false)
   {}

   void FastForwardTime(std::chrono::seconds sec)
   {
      ASSERT_TRUE(sec >= 1s);
      const auto end = m_now + sec;
      do {
         m_now += 1s;
         ProcessTimers();
      } while (m_now < end);
   }

private:
   async::Future<core::Timeout> ScheduleTimer(std::chrono::seconds period) override
   {
      async::Promise<core::Timeout> p([](auto task) {
         task();
      });
      auto future = p.GetFuture();
      const auto timeToFire = m_now + period;
      m_timers.push_back({std::move(p), timeToFire});
      ProcessTimers();
      return future;
   }

private:
   struct Timer
   {
      async::Promise<core::Timeout> promise;
      std::chrono::steady_clock::time_point time;
   };
   void ProcessTimers()
   {
      if (m_processing)
         return;
      m_processing = true;
      for (auto it = std::begin(m_timers); it != std::end(m_timers);) {
         if (it->promise.IsCancelled()) {
            it = m_timers.erase(it);
         } else if (it->time <= m_now) {
            it->promise.Finished(core::Timeout{});
            it = m_timers.erase(it);
         } else {
            ++it;
         }
      }
      m_processing = false;
   }

   std::chrono::steady_clock::time_point m_now;
   std::list<Timer> m_timers;
   bool m_processing;
};

class StubGenerator : public dice::IEngine
{
public:
   uint32_t value = 3;

private:
   void GenerateResult(dice::Cast & cast) override
   {
      std::visit(
         [this](auto & vec) {
            for (auto & e : vec)
               e(value);
         },
         cast);
   }
};

class IdlingFixture : public ::testing::Test
{
protected:
   ~IdlingFixture()
   {
      cmd::pool.ShrinkToFit();
      EXPECT_EQ(0U, cmd::pool.GetBlockCount());
   }
   std::unique_ptr<core::IController> CreateController()
   {
      proxy = new MockProxy;
      timer = new MockTimerEngine;
      generator = new StubGenerator;
      auto ctrl = core::CreateController(std::unique_ptr<dice::IEngine>(generator),
                                         std::unique_ptr<core::ITimerEngine>(timer),
                                         dice::CreateXmlSerializer(),
                                         logger);
      ctrl->Start([=](ILogger &) {
         return std::unique_ptr<core::Proxy>(proxy);
      });
      return ctrl;
   }

   MockProxy * proxy;
   MockTimerEngine * timer;
   StubGenerator * generator;
   FakeLogger logger;
};

#define ID(cmd) (cmd->GetId() >> 8)

TEST_F(IdlingFixture, state_idle_bluetooth_turned_on_successfully)
{
   auto ctrl = CreateController();
   EXPECT_EQ("New state: StateIdle ", logger.GetLastLine());

   logger.Clear();
   auto c = proxy->PopNextCommand();
   ASSERT_TRUE(c);
   EXPECT_EQ(105, ID(c)); // enable bt
   EXPECT_EQ(0U, c->GetArgsCount());

   // bluetooth on
   c->Respond(0);
   ctrl->OnEvent(17, {});
   EXPECT_TRUE(proxy->NoCommands());
   logger.Clear();

   // new game requested
   ctrl->OnEvent(13, {});
   EXPECT_EQ("New state: StateConnecting ", logger.GetLastLine());
}

TEST_F(IdlingFixture, state_idle_bluetooth_fatal_failure)
{
   auto ctrl = CreateController();
   logger.Clear();

   auto c = proxy->PopNextCommand();
   ASSERT_TRUE(c);

   // user declined
   c->Respond(6);
   EXPECT_TRUE(logger.Empty());
   EXPECT_TRUE(proxy->NoCommands());

   // no retries
   timer->FastForwardTime(2s);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // new game requested
   ctrl->OnEvent(13, {});
   c = proxy->PopNextCommand();
   ASSERT_TRUE(c);
   EXPECT_EQ(105, ID(c)); // enable bt
   EXPECT_EQ(0U, c->GetArgsCount());
}

TEST_F(IdlingFixture, state_idle_retries_to_enable_bluetooth)
{
   auto ctrl = CreateController();
   logger.Clear();

   auto c = proxy->PopNextCommand();
   ASSERT_TRUE(c);

   // invalid state
   c->Respond(0xffffffffffffffffLL);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // check retry
   timer->FastForwardTime(1s);
   c = proxy->PopNextCommand();
   ASSERT_TRUE(c);
   EXPECT_EQ(105, ID(c)); // enable bt
   EXPECT_EQ(0U, c->GetArgsCount());

   // no adapter
   c->Respond(5);
   c = proxy->PopNextCommand();
   ASSERT_TRUE(c);
   EXPECT_EQ(109, ID(c)); // text message
   EXPECT_EQ(1U, c->GetArgsCount());
   EXPECT_TRUE(proxy->NoCommands());

   // no retries
   timer->FastForwardTime(2s);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // new game requested
   ctrl->OnEvent(13, {});
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(IdlingFixture, state_idle_retry_after_bluetooth_off_and_user_declined)
{
   auto ctrl = CreateController();
   EXPECT_EQ("New state: StateIdle ", logger.GetLastLine());
   logger.Clear();
   {
      auto enableBt = proxy->PopNextCommand();
      ASSERT_TRUE(enableBt);
      EXPECT_EQ(105, ID(enableBt));
      EXPECT_EQ(0U, enableBt->GetArgsCount());
      enableBt->Respond(0);
   }

   // bluetooth off
   ctrl->OnEvent(18, {});
   {
      auto enableBt = proxy->PopNextCommand();
      ASSERT_TRUE(enableBt);
      EXPECT_EQ(105, ID(enableBt));
      EXPECT_EQ(0U, enableBt->GetArgsCount());
      enableBt->Respond(6); // user declined
   }
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(2s);
   EXPECT_TRUE(proxy->NoCommands());

   // new game
   ctrl->OnEvent(13, {});
   {
      auto enableBt = proxy->PopNextCommand();
      ASSERT_TRUE(enableBt);
      EXPECT_EQ(105, ID(enableBt));
      EXPECT_EQ(0U, enableBt->GetArgsCount());
      enableBt->Respond(0);
   }
   EXPECT_EQ("New state: StateConnecting ", logger.GetLastLine());
}

class ConnectingFixture : public IdlingFixture
{
protected:
   ConnectingFixture()
   {
      ctrl = CreateController();
      auto c = proxy->PopNextCommand();
      c->Respond(0);
      ctrl->OnEvent(13, {});
      EXPECT_EQ("New state: StateConnecting ", logger.GetLastLine());
      EXPECT_TRUE(logger.NoWarningsOrErrors());
      logger.Clear();
   }
   void StartDiscoveryAndListening()
   {
      auto cmdDiscovering = proxy->PopNextCommand();
      ASSERT_TRUE(cmdDiscovering);
      EXPECT_EQ(101, ID(cmdDiscovering));
      auto cmdListening = proxy->PopNextCommand();
      ASSERT_TRUE(cmdListening);
      EXPECT_EQ(100, ID(cmdListening));
      cmdDiscovering->Respond(0);
      cmdListening->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   std::unique_ptr<core::IController> ctrl;
};

TEST_F(ConnectingFixture, discovery_and_listening_started_successfully)
{
   auto cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));
   EXPECT_EQ(3U, cmdDiscovering->GetArgsCount());
   EXPECT_STREQ("false", cmdDiscovering->GetArgAt(2).data());

   auto cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));
   EXPECT_EQ(3U, cmdListening->GetArgsCount());
   EXPECT_STREQ("300", cmdListening->GetArgAt(2).data());

   cmdListening->Respond(0);
   cmdDiscovering->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, retries_to_start_listening_at_least_twice)
{
   auto cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));
   auto cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   cmdListening->Respond(0xff'ff'ff'ff'ff'ff'ff'ffLL);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   cmdListening->Respond(3);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   cmdListening->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, retries_to_start_discovery_at_least_twice)
{
   auto cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));
   auto cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   cmdDiscovering->Respond(0xff'ff'ff'ff'ff'ff'ff'ffLL);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));

   cmdDiscovering->Respond(0xff'ff'ff'ff'ff'ff'ff'ffLL);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));

   cmdDiscovering->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, fatal_failure_when_both_discovery_and_listening_failed)
{
   auto cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));
   auto cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   logger.Clear();
   cmdDiscovering->Respond(5);
   cmdListening->Respond(6);

   auto fatalFailureText = proxy->PopNextCommand();
   ASSERT_TRUE(fatalFailureText);
   EXPECT_EQ(109, ID(fatalFailureText));
   EXPECT_TRUE(logger.Empty());
}

TEST_F(ConnectingFixture, no_fatal_failure_when_only_listening_failed)
{
   auto cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));
   auto cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   cmdDiscovering->Respond(0);
   cmdListening->Respond(3);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, no_fatal_failure_when_only_discovery_failed)
{
   auto cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));
   auto cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   cmdDiscovering->Respond(0xffffffffffffffff);
   cmdListening->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, goes_to_idle_and_back_if_bluetooth_is_off)
{
   auto cmdDiscovering = proxy->PopNextCommand();
   ASSERT_TRUE(cmdDiscovering);
   EXPECT_EQ(101, ID(cmdDiscovering));
   auto cmdListening = proxy->PopNextCommand();
   ASSERT_TRUE(cmdListening);
   EXPECT_EQ(100, ID(cmdListening));

   size_t prevBlockCount = cmd::pool.GetBlockCount();
   cmdDiscovering->Respond(2);
   cmdListening->Respond(2);
   EXPECT_EQ("New state: StateIdle ", logger.GetLastLine());
   EXPECT_TRUE(cmd::pool.GetBlockCount() <= prevBlockCount);
   auto enableBt = proxy->PopNextCommand();
   ASSERT_TRUE(enableBt);
   EXPECT_EQ(105, ID(enableBt));
   EXPECT_TRUE(proxy->NoCommands());
   enableBt->Respond(0);
   EXPECT_EQ("New state: StateConnecting ", logger.GetLastLine());
}

TEST_F(ConnectingFixture, sends_hello_to_connected_device)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto cmdHello = proxy->PopNextCommand();
   ASSERT_TRUE(cmdHello);
   EXPECT_EQ(108, ID(cmdHello));
   EXPECT_EQ(2U, cmdHello->GetArgsCount());
   EXPECT_STREQ(R"(<Hello><Mac>5c:b9:01:f8:b6:49</Mac></Hello>)", cmdHello->GetArgAt(0).data());
   EXPECT_STREQ("5c:b9:01:f8:b6:49", cmdHello->GetArgAt(1).data());
   cmdHello->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, retries_hello_on_invalid_state_and_disconnects_on_socket_error)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto hello1 = proxy->PopNextCommand();
   ASSERT_TRUE(hello1);
   EXPECT_EQ(108, ID(hello1));
   hello1->Respond(0xffffffffffffffff);

   auto hello2 = proxy->PopNextCommand();
   ASSERT_TRUE(hello2);
   EXPECT_EQ(108, ID(hello2));
   hello2->Respond(7);

   auto disconnect = proxy->PopNextCommand();
   ASSERT_TRUE(disconnect);
   EXPECT_EQ(104, ID(disconnect));
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:49", disconnect->GetArgAt(1).data());
   disconnect->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, disconnects_on_read_error_and_does_not_retry_hello)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto hello = proxy->PopNextCommand();
   ASSERT_TRUE(hello);
   EXPECT_EQ(108, ID(hello));
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(19, {"5c:b9:01:f8:b6:49", ""});
   auto disconnect = proxy->PopNextCommand();
   ASSERT_TRUE(disconnect);
   EXPECT_EQ(104, ID(disconnect));
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:49", disconnect->GetArgAt(1).data());
   disconnect->Respond(0);

   hello->Respond(0xffffffffffffffff);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, does_not_start_negotiation_until_received_own_mac)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto cmdHello = proxy->PopNextCommand();
   ASSERT_TRUE(cmdHello);
   EXPECT_EQ(108, ID(cmdHello));
   cmdHello->Respond(0);
   logger.Clear();

   ctrl->OnEvent(12, {}); // start
   auto toast = proxy->PopNextCommand();
   ASSERT_TRUE(toast);
   EXPECT_EQ(110, ID(toast));
   EXPECT_EQ(2U, toast->GetArgsCount());
   EXPECT_STREQ("3", toast->GetArgAt(1).data());
   EXPECT_NE("New state: StateNegotiating ", logger.GetLastLine());
   logger.Clear();

   timer->FastForwardTime(1s);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   ctrl->OnEvent(14, {R"(<Hello><Mac>f4:06:69:7b:4b:e7</Mac></Hello>)", "5c:b9:01:f8:b6:49", ""});
   EXPECT_TRUE(proxy->NoCommands());
   logger.Clear();

   timer->FastForwardTime(1s);
   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());

   auto stopDiscovery = proxy->PopNextCommand();
   ASSERT_TRUE(stopDiscovery);
   EXPECT_EQ(103, ID(stopDiscovery));

   auto stopListening = proxy->PopNextCommand();
   ASSERT_TRUE(stopListening);
   EXPECT_EQ(102, ID(stopListening));
}

TEST_F(ConnectingFixture, does_not_negotiate_with_disconnected)
{
   StartDiscoveryAndListening();

   // 3 well-behaved peers
   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:41", "Chalie Chaplin 1"});
   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:42", "Chalie Chaplin 2"});
   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:43", "Chalie Chaplin 3"});

   for (int i = 0; i < 3; ++i) {
      auto cmdHello = proxy->PopNextCommand();
      ASSERT_TRUE(cmdHello);
      cmdHello->Respond(0);
   }

   // 1 weird peer
   ctrl->OnEvent(
      14,
      {R"(<Hello><Mac>5c:b9:01:f8:b6:40</Mac></Hello>)", "5c:b9:01:f8:b6:44", "Charlie Chaplin 4"});
   auto cmdHello = proxy->PopNextCommand();
   ASSERT_TRUE(cmdHello);
   EXPECT_EQ(108, ID(cmdHello));
   EXPECT_EQ(2U, cmdHello->GetArgsCount());
   EXPECT_STREQ(R"(<Hello><Mac>5c:b9:01:f8:b6:44</Mac></Hello>)", cmdHello->GetArgAt(0).data());
   EXPECT_STREQ("5c:b9:01:f8:b6:44", cmdHello->GetArgAt(1).data());
   cmdHello->Respond(0);

   // 1 disconnects
   ctrl->OnEvent(19, {"5c:b9:01:f8:b6:42", ""});
   auto disconnect = proxy->PopNextCommand();
   ASSERT_TRUE(disconnect);
   EXPECT_EQ(104, ID(disconnect));
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:42", disconnect->GetArgAt(1).data());
   disconnect->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
   logger.Clear();

   // game start
   ctrl->OnEvent(12, {});
   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());

   auto stopDiscovery = proxy->PopNextCommand();
   ASSERT_TRUE(stopDiscovery);
   auto stopListening = proxy->PopNextCommand();
   ASSERT_TRUE(stopListening);

   auto negotiationStart = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStart);
   EXPECT_EQ(106, ID(negotiationStart));
   negotiationStart->Respond(0);

   // sorted list of candidates should be:
   // 5c:b9:01:f8:b6:40
   // 5c:b9:01:f8:b6:41
   // 5c:b9:01:f8:b6:43 <-- round 2
   // 5c:b9:01:f8:b6:44
   const char * expectedOffer = R"(<Offer round="2"><Mac>5c:b9:01:f8:b6:43</Mac></Offer>)";

   // local offer is being broadcast
   {
      auto offer = proxy->PopNextCommand();
      ASSERT_TRUE(offer);
      EXPECT_EQ(108, ID(offer));
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(0).data());
      EXPECT_STREQ("5c:b9:01:f8:b6:44", offer->GetArgAt(1).data());
      offer->Respond(0);
   }
   {
      auto offer = proxy->PopNextCommand();
      ASSERT_TRUE(offer);
      EXPECT_EQ(108, ID(offer));
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(0).data());
      EXPECT_STREQ("5c:b9:01:f8:b6:43", offer->GetArgAt(1).data());
      offer->Respond(0);
   }
   {
      auto offer = proxy->PopNextCommand();
      ASSERT_TRUE(offer);
      EXPECT_EQ(108, ID(offer));
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(0).data());
      EXPECT_STREQ("5c:b9:01:f8:b6:41", offer->GetArgAt(1).data());
      offer->Respond(0);
   }
   EXPECT_TRUE(proxy->NoCommands());
}

template <size_t PEERS_COUNT>
class NegotiatingFixture : public ConnectingFixture
{
protected:
   static constexpr size_t peersCount = PEERS_COUNT;

   NegotiatingFixture()
      : ConnectingFixture()
      , localMac("5c:b9:01:f8:b6:4" + std::to_string(peersCount))
   {
      StartDiscoveryAndListening();

      for (size_t i = 0; i < peersCount; ++i)
         peers.emplace_back("Charlie Chaplin " + std::to_string(i),
                            "5c:b9:01:f8:b6:4" + std::to_string(i));

      for (const auto & peer : peers) {
         ctrl->OnEvent(10, {peer.mac, peer.name});
         auto cmdHello = proxy->PopNextCommand();
         EXPECT_TRUE(cmdHello);
         EXPECT_EQ(108, ID(cmdHello));
         cmdHello->Respond(0);
      }
      for (const auto & peer : peers) {
         ctrl->OnEvent(14,
                       {R"(<Hello><Mac>)" + localMac + R"(</Mac></Hello>)", peer.mac, peer.name});
         EXPECT_TRUE(proxy->NoCommands());
      }
      ctrl->OnEvent(12, {}); // start
      EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());

      auto stopDiscovery = proxy->PopNextCommand();
      EXPECT_TRUE(stopDiscovery);
      EXPECT_EQ(103, ID(stopDiscovery));

      auto stopListening = proxy->PopNextCommand();
      EXPECT_TRUE(stopListening);
      EXPECT_EQ(102, ID(stopListening));
      logger.Clear();
   }
   void CheckLocalOffer(const char * expectedOffer)
   {
      for (auto it = std::crbegin(Peers()); it != std::crend(Peers()); ++it) {
         auto offer = proxy->PopNextCommand();
         ASSERT_TRUE(offer);
         EXPECT_EQ(108, ID(offer));
         EXPECT_EQ(2U, offer->GetArgsCount());
         EXPECT_STREQ(expectedOffer, offer->GetArgAt(0).data());
         EXPECT_STREQ(it->mac.c_str(), offer->GetArgAt(1).data());
         offer->Respond(0);
      }
   }


   const std::vector<bt::Device> & Peers() const { return peers; }
   const std::string localMac;

private:
   std::vector<bt::Device> peers;
};

#define NEG_FIX_ALIAS(num) using NegotiatingFixture##num = NegotiatingFixture<num>

NEG_FIX_ALIAS(2);
NEG_FIX_ALIAS(3);
NEG_FIX_ALIAS(4);
NEG_FIX_ALIAS(5);
NEG_FIX_ALIAS(6);
NEG_FIX_ALIAS(7);
NEG_FIX_ALIAS(8);
NEG_FIX_ALIAS(9);
NEG_FIX_ALIAS(10);

#undef NEG_FIX_ALIAS

// round 3
TEST_F(NegotiatingFixture10, goes_to_negotiation_successfully)
{
   SUCCEED();
}

// round 4
TEST_F(NegotiatingFixture4, increases_round_appropriately)
{
   auto negotiationStart = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStart);
   EXPECT_EQ(106, ID(negotiationStart));
   negotiationStart->Respond(0);

   // sorted list of candidates should be:
   // 5c:b9:01:f8:b6:40 <-- round 5
   // 5c:b9:01:f8:b6:41 <-- round 6
   // 5c:b9:01:f8:b6:42
   // 5c:b9:01:f8:b6:43
   // 5c:b9:01:f8:b6:44 <-- round 4
   CheckLocalOffer(R"(<Offer round="4"><Mac>5c:b9:01:f8:b6:44</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(
      14,
      {R"(<Offer round="5"><Mac>5c:b9:01:f8:b6:40</Mac></Offer>)", "5c:b9:01:f8:b6:42", ""});
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(
      14,
      {R"(<Offer round="3"><Mac>5c:b9:01:f8:b6:43</Mac></Offer>)", "5c:b9:01:f8:b6:41", ""});
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   CheckLocalOffer(R"(<Offer round="5"><Mac>5c:b9:01:f8:b6:40</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(
      14,
      {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:40", ""});
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   CheckLocalOffer(R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(
      14,
      {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:41", ""});
   ctrl->OnEvent(
      14,
      {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:42", ""});
   ctrl->OnEvent(
      14,
      {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:43", ""});

   timer->FastForwardTime(1s);
   auto negotiationStop = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStop);
   EXPECT_EQ(107, ID(negotiationStop));
   EXPECT_EQ(1U, negotiationStop->GetArgsCount());
   EXPECT_STREQ("Charlie Chaplin 1", negotiationStop->GetArgAt(0).data());
   negotiationStop->Respond(0);
   EXPECT_EQ("New state: StatePlaying ", logger.GetLastLine());
   EXPECT_TRUE(proxy->NoCommands());
}

// round 7
TEST_F(NegotiatingFixture2, handles_disconnects_and_disagreements_on_nominees_mac)
{
   auto negotiationStart = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStart);
   EXPECT_EQ(106, ID(negotiationStart));
   negotiationStart->Respond(0);

   // sorted list of candidates should be:
   // 5c:b9:01:f8:b6:40
   // 5c:b9:01:f8:b6:41 <-- round 7
   // 5c:b9:01:f8:b6:42
   CheckLocalOffer(R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(
      14,
      {R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:41", ""});
   // peer 1 disconnected from peer 0
   ctrl->OnEvent(
      14,
      {R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:42</Mac></Offer>)", "5c:b9:01:f8:b6:40", ""});

   timer->FastForwardTime(1s);
   CheckLocalOffer(R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   // peer 1 disconnected from us as well
   ctrl->OnEvent(19, {"5c:b9:01:f8:b6:41", ""});
   auto disconnect = proxy->PopNextCommand();
   ASSERT_TRUE(disconnect);
   EXPECT_EQ(104, ID(disconnect));
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:41", disconnect->GetArgAt(1).data());
   disconnect->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   const char * expectedOffer = R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:42</Mac></Offer>)";
   {
      auto offer = proxy->PopNextCommand();
      ASSERT_TRUE(offer);
      EXPECT_EQ(108, ID(offer));
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(0).data());
      EXPECT_STREQ("5c:b9:01:f8:b6:40", offer->GetArgAt(1).data());
      offer->Respond(0);
   }
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   auto negotiationStop = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStop);
   EXPECT_EQ(107, ID(negotiationStop));
   EXPECT_EQ(1U, negotiationStop->GetArgsCount());
   EXPECT_STREQ("You", negotiationStop->GetArgAt(0).data());
   negotiationStop->Respond(0);
   EXPECT_EQ("New state: StatePlaying ", logger.GetLastLine());
   EXPECT_TRUE(proxy->NoCommands());
}

template <size_t PEERS_COUNT, uint32_t ROUND>
class PlayingFixture : public NegotiatingFixture<PEERS_COUNT>
{
protected:
   static constexpr uint32_t round = ROUND;

   using Base = NegotiatingFixture<PEERS_COUNT>;

   PlayingFixture()
      : Base()
   {
      auto negotiationStart = Base::proxy->PopNextCommand();
      EXPECT_TRUE(negotiationStart);
      EXPECT_EQ(106, ID(negotiationStart));
      negotiationStart->Respond(0);

      int nomineeIndex = round % (Base::Peers().size() + 1);
      nomineeMac =
         nomineeIndex == Base::Peers().size() ? Base::localMac : Base::Peers()[nomineeIndex].mac;
      nomineeName = nomineeIndex == Base::Peers().size() ? "You" : Base::Peers()[nomineeIndex].name;

      std::string offer =
         "<Offer round=\"" + std::to_string(round) + "\"><Mac>" + nomineeMac + "</Mac></Offer>";
      Base::CheckLocalOffer(offer.c_str());
      for (const auto & device : Base::Peers())
         Base::ctrl->OnEvent(14, {offer, device.mac, device.name});

      Base::timer->FastForwardTime(1s);
      auto negotiationStop = Base::proxy->PopNextCommand();
      EXPECT_TRUE(negotiationStop);
      EXPECT_EQ(107, ID(negotiationStop));
      EXPECT_EQ(1U, negotiationStop->GetArgsCount());
      EXPECT_STREQ(nomineeName.c_str(), negotiationStop->GetArgAt(0).data());
      negotiationStop->Respond(0);
      EXPECT_EQ("New state: StatePlaying ", Base::logger.GetLastLine());
      Base::logger.Clear();
      EXPECT_TRUE(Base::proxy->NoCommands());
   }

   std::string nomineeMac;
   std::string nomineeName;
};

using P2R8 = PlayingFixture<2u, 8u>;

TEST_F(P2R8, local_generator_responds_to_remote_and_local_requests)
{
   timer->FastForwardTime(2s);
   EXPECT_TRUE(proxy->NoCommands());

   // remote request
   {
      generator->value = 3;
      ctrl->OnEvent(14, {R"(<Request type="D6" size="4" successFrom="3" />)", Peers()[0].mac, ""});

      auto showRequest = proxy->PopNextCommand();
      ASSERT_TRUE(showRequest);
      EXPECT_EQ(112, ID(showRequest));
      EXPECT_EQ(4U, showRequest->GetArgsCount());
      EXPECT_STREQ("D6", showRequest->GetArgAt(0).data());
      EXPECT_STREQ("4", showRequest->GetArgAt(1).data());
      EXPECT_STREQ("3", showRequest->GetArgAt(2).data());
      EXPECT_STREQ(Peers()[0].name.c_str(), showRequest->GetArgAt(3).data());
      showRequest->Respond(0);

      const char * expectedResponse =
         R"(<Response successCount="4" size="4" type="D6"><Val>3</Val><Val>3</Val><Val>3</Val><Val>3</Val></Response>)";
      for (const auto & peer : Peers()) {
         auto sendResponse = proxy->PopNextCommand();
         ASSERT_TRUE(sendResponse);
         EXPECT_EQ(108, ID(sendResponse));
         EXPECT_EQ(2U, sendResponse->GetArgsCount());
         EXPECT_STREQ(expectedResponse, sendResponse->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendResponse->GetArgAt(1).data());
         sendResponse->Respond(0);
      }

      auto showResponse = proxy->PopNextCommand();
      ASSERT_TRUE(showResponse);
      EXPECT_EQ(113, ID(showResponse));
      EXPECT_EQ(4u, showResponse->GetArgsCount());
      EXPECT_STREQ("3;3;3;3;", showResponse->GetArgAt(0).data());
      EXPECT_STREQ("D6", showResponse->GetArgAt(1).data());
      EXPECT_STREQ("4", showResponse->GetArgAt(2).data());
      EXPECT_STREQ("You", showResponse->GetArgAt(3).data());
      showResponse->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // local request w threshold
   {
      generator->value = 42;
      ctrl->OnEvent(15, {"D100", "2", "43"});

      auto showRequest = proxy->PopNextCommand();
      ASSERT_TRUE(showRequest);
      EXPECT_EQ(112, ID(showRequest));
      EXPECT_EQ(4U, showRequest->GetArgsCount());
      EXPECT_STREQ("D100", showRequest->GetArgAt(0).data());
      EXPECT_STREQ("2", showRequest->GetArgAt(1).data());
      EXPECT_STREQ("43", showRequest->GetArgAt(2).data());
      EXPECT_STREQ("You", showRequest->GetArgAt(3).data());
      showRequest->Respond(0);

      const char * expectedRequest = R"(<Request successFrom="43" size="2" type="D100" />)";
      for (const auto & peer : Peers()) {
         auto sendRequest = proxy->PopNextCommand();
         ASSERT_TRUE(sendRequest);
         EXPECT_EQ(108, ID(sendRequest));
         EXPECT_EQ(2U, sendRequest->GetArgsCount());
         EXPECT_STREQ(expectedRequest, sendRequest->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendRequest->GetArgAt(1).data());
         sendRequest->Respond(0);
      }

      const char * expectedResponse =
         R"(<Response successCount="0" size="2" type="D100"><Val>42</Val><Val>42</Val></Response>)";
      for (const auto & peer : Peers()) {
         auto sendResponse = proxy->PopNextCommand();
         ASSERT_TRUE(sendResponse);
         EXPECT_EQ(108, ID(sendResponse));
         EXPECT_EQ(2U, sendResponse->GetArgsCount());
         EXPECT_STREQ(expectedResponse, sendResponse->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendResponse->GetArgAt(1).data());
         sendResponse->Respond(0);
      }

      auto showResponse = proxy->PopNextCommand();
      ASSERT_TRUE(showResponse);
      EXPECT_EQ(113, ID(showResponse));
      EXPECT_EQ(4u, showResponse->GetArgsCount());
      EXPECT_STREQ("42;42;", showResponse->GetArgAt(0).data());
      EXPECT_STREQ("D100", showResponse->GetArgAt(1).data());
      EXPECT_STREQ("0", showResponse->GetArgAt(2).data());
      EXPECT_STREQ("You", showResponse->GetArgAt(3).data());
      showResponse->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // local request w/o threshold
   {
      generator->value = 42;
      ctrl->OnEvent(15, {"D100", "2"});

      auto showRequest = proxy->PopNextCommand();
      ASSERT_TRUE(showRequest);
      EXPECT_EQ(112, ID(showRequest));
      EXPECT_EQ(4U, showRequest->GetArgsCount());
      EXPECT_STREQ("D100", showRequest->GetArgAt(0).data());
      EXPECT_STREQ("2", showRequest->GetArgAt(1).data());
      EXPECT_STREQ("0", showRequest->GetArgAt(2).data());
      EXPECT_STREQ("You", showRequest->GetArgAt(3).data());
      showRequest->Respond(0);

      const char * expectedRequest = R"(<Request size="2" type="D100" />)";
      for (const auto & peer : Peers()) {
         auto sendRequest = proxy->PopNextCommand();
         ASSERT_TRUE(sendRequest);
         EXPECT_EQ(108, ID(sendRequest));
         EXPECT_EQ(2U, sendRequest->GetArgsCount());
         EXPECT_STREQ(expectedRequest, sendRequest->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendRequest->GetArgAt(1).data());
         sendRequest->Respond(0);
      }

      const char * expectedResponse =
         R"(<Response size="2" type="D100"><Val>42</Val><Val>42</Val></Response>)";
      for (const auto & peer : Peers()) {
         auto sendResponse = proxy->PopNextCommand();
         ASSERT_TRUE(sendResponse);
         EXPECT_EQ(108, ID(sendResponse));
         EXPECT_EQ(2U, sendResponse->GetArgsCount());
         EXPECT_STREQ(expectedResponse, sendResponse->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendResponse->GetArgAt(1).data());
         sendResponse->Respond(0);
      }

      auto showResponse = proxy->PopNextCommand();
      ASSERT_TRUE(showResponse);
      EXPECT_EQ(113, ID(showResponse));
      EXPECT_EQ(4u, showResponse->GetArgsCount());
      EXPECT_STREQ("42;42;", showResponse->GetArgAt(0).data());
      EXPECT_STREQ("D100", showResponse->GetArgAt(1).data());
      EXPECT_STREQ("-1", showResponse->GetArgAt(2).data());
      EXPECT_STREQ("You", showResponse->GetArgAt(3).data());
      showResponse->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // BIG local request
   {
      generator->value = 6;
      ctrl->OnEvent(15, {"D6", "70", "3"});

      auto showRequest = proxy->PopNextCommand();
      ASSERT_TRUE(showRequest);
      EXPECT_EQ(112, ID(showRequest));
      EXPECT_EQ(4U, showRequest->GetArgsCount());
      EXPECT_STREQ("D6", showRequest->GetArgAt(0).data());
      EXPECT_STREQ("70", showRequest->GetArgAt(1).data());
      EXPECT_STREQ("3", showRequest->GetArgAt(2).data());
      EXPECT_STREQ("You", showRequest->GetArgAt(3).data());
      showRequest->Respond(0);

      const char * expectedRequest = R"(<Request successFrom="3" size="70" type="D6" />)";
      for (const auto & peer : Peers()) {
         auto sendRequest = proxy->PopNextCommand();
         ASSERT_TRUE(sendRequest);
         EXPECT_EQ(108, ID(sendRequest));
         EXPECT_EQ(2U, sendRequest->GetArgsCount());
         EXPECT_STREQ(expectedRequest, sendRequest->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendRequest->GetArgAt(1).data());
         sendRequest->Respond(0);
      }

      std::ostringstream expectedResponse;
      expectedResponse << R"(<Response successCount="70" size="70" type="D6">)";
      for (int i = 0; i < 70; ++i)
         expectedResponse << "<Val>6</Val>";
      expectedResponse << R"(</Response>)";

      for (const auto & peer : Peers()) {
         auto sendResponse = proxy->PopNextCommand();
         ASSERT_TRUE(sendResponse);
         EXPECT_EQ(108, ID(sendResponse));
         EXPECT_EQ(2U, sendResponse->GetArgsCount());
         EXPECT_STREQ(expectedResponse.str().c_str(), sendResponse->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendResponse->GetArgAt(1).data());
         sendResponse->Respond(0);
      }

      expectedResponse.str("");
      for (int i = 0; i < 70; ++i)
         expectedResponse << "6;";

      auto showResponse = proxy->PopNextCommand();
      ASSERT_TRUE(showResponse);
      EXPECT_EQ(113, ID(showResponse));
      EXPECT_EQ(4u, showResponse->GetArgsCount());
      EXPECT_STREQ(expectedResponse.str().c_str(), showResponse->GetArgAt(0).data());
      EXPECT_STREQ("D6", showResponse->GetArgAt(1).data());
      EXPECT_STREQ("70", showResponse->GetArgAt(2).data());
      EXPECT_STREQ("You", showResponse->GetArgAt(3).data());
      showResponse->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // Peer 1 wants to re-negotiate before 5 sec
   std::string offer = R"(<Offer round="12"><Mac>)" + Peers()[0].mac + "</Mac></Offer>";
   ctrl->OnEvent(14, {offer, Peers()[1].mac, ""});
   EXPECT_TRUE(proxy->NoCommands());

   // Peer 1 tries again 3 sec later
   timer->FastForwardTime(3s);
   EXPECT_TRUE(proxy->NoCommands());
   ctrl->OnEvent(14, {offer, Peers()[1].mac, ""});
   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());

   auto negotiationStart = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStart);
   EXPECT_EQ(106, ID(negotiationStart));
   negotiationStart->Respond(0);

   for (auto it = std::crbegin(Peers()); it != std::crend(Peers()); ++it) {
      auto sendOffer = proxy->PopNextCommand();
      ASSERT_TRUE(sendOffer);
      EXPECT_EQ(108, ID(sendOffer));
      EXPECT_EQ(2U, sendOffer->GetArgsCount());
      EXPECT_STREQ(offer.c_str(), sendOffer->GetArgAt(0).data());
      EXPECT_STREQ(it->mac.c_str(), sendOffer->GetArgAt(1).data());
      sendOffer->Respond(0);
   }
   EXPECT_TRUE(proxy->NoCommands());
}

using P2R13 = PlayingFixture<2u, 13u>;

TEST_F(P2R13, remote_generator_is_respected)
{
   // remote request
   {
      // peer 0 makes a request
      ctrl->OnEvent(14, {R"(<Request type="D8" size="1" />)", Peers()[0].mac, ""});
      auto showRequest = proxy->PopNextCommand();
      ASSERT_TRUE(showRequest);
      EXPECT_EQ(112, ID(showRequest));
      EXPECT_EQ(4U, showRequest->GetArgsCount());
      EXPECT_STREQ("D8", showRequest->GetArgAt(0).data());
      EXPECT_STREQ("1", showRequest->GetArgAt(1).data());
      EXPECT_STREQ("0", showRequest->GetArgAt(2).data());
      EXPECT_STREQ(Peers()[0].name.c_str(), showRequest->GetArgAt(3).data());
      showRequest->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());

      timer->FastForwardTime(1s);
      EXPECT_TRUE(proxy->NoCommands());

      // peer 0 answers its own request even though it's not generator
      ctrl->OnEvent(
         14,
         {R"(<Response size="1" type="D8"><Val>8</Val></Response>)", Peers()[0].mac, ""});
      EXPECT_TRUE(proxy->NoCommands());

      // generator answers the request
      ctrl->OnEvent(
         14,
         {R"(<Response size="1" type="D8"><Val>1</Val></Response> )", Peers()[1].mac, ""});
      auto showResponse = proxy->PopNextCommand();
      ASSERT_TRUE(showResponse);
      EXPECT_EQ(113, ID(showResponse));
      EXPECT_EQ(4u, showResponse->GetArgsCount());
      EXPECT_STREQ("1;", showResponse->GetArgAt(0).data());
      EXPECT_STREQ("D8", showResponse->GetArgAt(1).data());
      EXPECT_STREQ("-1", showResponse->GetArgAt(2).data());
      EXPECT_STREQ(Peers()[1].name.c_str(), showResponse->GetArgAt(3).data());
      showResponse->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // repeating local request
   {
      ctrl->OnEvent(15, {"D4", "1", "3"});
      auto showRequest = proxy->PopNextCommand();
      ASSERT_TRUE(showRequest);
      EXPECT_EQ(112, ID(showRequest));
      EXPECT_EQ(4U, showRequest->GetArgsCount());
      EXPECT_STREQ("D4", showRequest->GetArgAt(0).data());
      EXPECT_STREQ("1", showRequest->GetArgAt(1).data());
      EXPECT_STREQ("3", showRequest->GetArgAt(2).data());
      EXPECT_STREQ("You", showRequest->GetArgAt(3).data());
      showRequest->Respond(0);

      const char * expectedRequest = R"(<Request successFrom="3" size="1" type="D4" />)";
      for (const auto & peer : Peers()) {
         auto sendRequest = proxy->PopNextCommand();
         ASSERT_TRUE(sendRequest);
         EXPECT_EQ(108, ID(sendRequest));
         EXPECT_EQ(2U, sendRequest->GetArgsCount());
         EXPECT_STREQ(expectedRequest, sendRequest->GetArgAt(0).data());
         EXPECT_STREQ(peer.mac.c_str(), sendRequest->GetArgAt(1).data());
         sendRequest->Respond(0);
      }
      EXPECT_TRUE(proxy->NoCommands());

      timer->FastForwardTime(1s);

      auto sendRequest = proxy->PopNextCommand();
      ASSERT_TRUE(sendRequest);
      EXPECT_EQ(108, ID(sendRequest));
      EXPECT_EQ(2U, sendRequest->GetArgsCount());
      EXPECT_STREQ(expectedRequest, sendRequest->GetArgAt(0).data());
      EXPECT_STREQ(Peers()[1].mac.c_str(), sendRequest->GetArgAt(1).data());
      sendRequest->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());

      ctrl->OnEvent(14,
                    {R"(<Response successCount="1" size="1" type="D4"><Val>4</Val></Response>)",
                     Peers()[1].mac,
                     ""});
      auto showResponse = proxy->PopNextCommand();
      ASSERT_TRUE(showResponse);
      EXPECT_EQ(113, ID(showResponse));
      EXPECT_EQ(4u, showResponse->GetArgsCount());
      EXPECT_STREQ("4;", showResponse->GetArgAt(0).data());
      EXPECT_STREQ("D4", showResponse->GetArgAt(1).data());
      EXPECT_STREQ("1", showResponse->GetArgAt(2).data());
      EXPECT_STREQ(Peers()[1].name.c_str(), showResponse->GetArgAt(3).data());
      showResponse->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());

      timer->FastForwardTime(1s);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // 7 more requests and responses
   for (int i = 0; i < 7; ++i) {
      ctrl->OnEvent(14, {R"(<Request successFrom="3" type="D4" size="1" />)", Peers()[0].mac, ""});
      auto showRequest = proxy->PopNextCommand();
      ASSERT_TRUE(showRequest);
      EXPECT_EQ(112, ID(showRequest));
      EXPECT_EQ(4U, showRequest->GetArgsCount());
      EXPECT_STREQ("D4", showRequest->GetArgAt(0).data());
      EXPECT_STREQ("1", showRequest->GetArgAt(1).data());
      EXPECT_STREQ("3", showRequest->GetArgAt(2).data());
      EXPECT_STREQ(Peers()[0].name.c_str(), showRequest->GetArgAt(3).data());
      showRequest->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());

      ctrl->OnEvent(14,
                    {R"(<Response successCount="0" size="1" type="D4"><Val>2</Val></Response>)",
                     Peers()[1].mac,
                     ""});
      auto showResponse = proxy->PopNextCommand();
      ASSERT_TRUE(showResponse);
      EXPECT_EQ(113, ID(showResponse));
      EXPECT_EQ(4u, showResponse->GetArgsCount());
      EXPECT_STREQ("2;", showResponse->GetArgAt(0).data());
      EXPECT_STREQ("D4", showResponse->GetArgAt(1).data());
      EXPECT_STREQ("0", showResponse->GetArgAt(2).data());
      EXPECT_STREQ(Peers()[1].name.c_str(), showResponse->GetArgAt(3).data());
      showResponse->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // one response from unauthorized peer
   ctrl->OnEvent(14,
                 {R"(<Response successCount="1" size="1" type="D4"><Val>4</Val></Response>)",
                  Peers()[0].mac,
                  ""});
   EXPECT_TRUE(proxy->NoCommands());

   // one authorized response without previous request
   ctrl->OnEvent(14,
                 {R"(<Response successCount="1" size="1" type="D6"><Val>5</Val></Response>)",
                  Peers()[1].mac,
                  ""});
   auto showResponse = proxy->PopNextCommand();
   ASSERT_TRUE(showResponse);
   EXPECT_EQ(113, ID(showResponse));
   EXPECT_EQ(4u, showResponse->GetArgsCount());
   EXPECT_STREQ("5;", showResponse->GetArgAt(0).data());
   EXPECT_STREQ("D6", showResponse->GetArgAt(1).data());
   EXPECT_STREQ("1", showResponse->GetArgAt(2).data());
   EXPECT_STREQ(Peers()[1].name.c_str(), showResponse->GetArgAt(3).data());
   showResponse->Respond(0);

   // starting negotiation
   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());
   auto negotiationStart = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStart);
   EXPECT_EQ(106, ID(negotiationStart));
   negotiationStart->Respond(0);

   std::string offer = R"(<Offer round="14"><Mac>)" + localMac + "</Mac></Offer>";
   for (auto it = std::crbegin(Peers()); it != std::crend(Peers()); ++it) {
      auto sendOffer = proxy->PopNextCommand();
      ASSERT_TRUE(sendOffer);
      EXPECT_EQ(108, ID(sendOffer));
      EXPECT_EQ(2U, sendOffer->GetArgsCount());
      EXPECT_STREQ(offer.c_str(), sendOffer->GetArgAt(0).data());
      EXPECT_STREQ(it->mac.c_str(), sendOffer->GetArgAt(1).data());
      sendOffer->Respond(0);
   }
   EXPECT_TRUE(proxy->NoCommands());
}

using P2R15 = PlayingFixture<2u, 15u>;

TEST_F(P2R15, renegotiates_when_generator_doesnt_answer_requests)
{
   ctrl->OnEvent(15, {"D4", "1", "3"});
   auto showRequest = proxy->PopNextCommand();
   ASSERT_TRUE(showRequest);
   EXPECT_EQ(112, ID(showRequest));
   showRequest->Respond(0);

   // sends request
   const char * expectedRequest = R"(<Request successFrom="3" size="1" type="D4" />)";
   for (const auto & peer : Peers()) {
      auto sendRequest = proxy->PopNextCommand();
      ASSERT_TRUE(sendRequest);
      EXPECT_EQ(108, ID(sendRequest));
      EXPECT_EQ(2U, sendRequest->GetArgsCount());
      EXPECT_STREQ(expectedRequest, sendRequest->GetArgAt(0).data());
      EXPECT_STREQ(peer.mac.c_str(), sendRequest->GetArgAt(1).data());
      sendRequest->Respond(0);
   }
   EXPECT_TRUE(proxy->NoCommands());

   // received response to other request
   ctrl->OnEvent(14,
                 {R"(<Response successCount="1" size="1" type="D6"><Val>5</Val></Response>)",
                  Peers()[0].mac,
                  ""});
   auto showResponse = proxy->PopNextCommand();
   ASSERT_TRUE(showResponse);
   EXPECT_EQ(113, ID(showResponse));
   showResponse->Respond(0);

   // retries request
   for (int i = 0; i < 2; ++i) {
      timer->FastForwardTime(1s);
      auto sendRequest = proxy->PopNextCommand();
      ASSERT_TRUE(sendRequest);
      EXPECT_EQ(108, ID(sendRequest));
      EXPECT_EQ(2U, sendRequest->GetArgsCount());
      EXPECT_STREQ(expectedRequest, sendRequest->GetArgAt(0).data());
      EXPECT_STREQ(Peers()[0].mac.c_str(), sendRequest->GetArgAt(1).data());
      sendRequest->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   // no success => goes to Negotiation
   timer->FastForwardTime(1s);
   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());
   auto negotiationStart = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStart);
   EXPECT_EQ(106, ID(negotiationStart));
   negotiationStart->Respond(0);

   std::string offer = R"(<Offer round="16"><Mac>)" + Peers()[1].mac + "</Mac></Offer>";
   for (auto it = std::crbegin(Peers()); it != std::crend(Peers()); ++it) {
      auto sendOffer = proxy->PopNextCommand();
      ASSERT_TRUE(sendOffer);
      EXPECT_EQ(108, ID(sendOffer));
      EXPECT_EQ(2U, sendOffer->GetArgsCount());
      EXPECT_STREQ(offer.c_str(), sendOffer->GetArgAt(0).data());
      EXPECT_STREQ(it->mac.c_str(), sendOffer->GetArgAt(1).data());
      sendOffer->Respond(0);
   }
   EXPECT_TRUE(proxy->NoCommands());
}

using P2R17 = PlayingFixture<2u, 17u>;

TEST_F(P2R17, disconnects_peers_that_are_in_error_state_at_the_end)
{
   timer->FastForwardTime(5s);

   // both peers report read errors...
   ctrl->OnEvent(19, {Peers()[0].mac, ""});
   EXPECT_TRUE(proxy->NoCommands());
   ctrl->OnEvent(19, {Peers()[1].mac, ""});
   EXPECT_TRUE(proxy->NoCommands());
   timer->FastForwardTime(1s);
   EXPECT_TRUE(proxy->NoCommands());

   // ...but peer 0 comes back with an offer
   std::string offer = R"(<Offer round="19"><Mac>)" + Peers()[1].mac + "</Mac></Offer>";
   ctrl->OnEvent(14, {offer, Peers()[0].mac, ""});

   // so we should disconnect peer 1 and start negotiation...
   auto disconnect = proxy->PopNextCommand();
   ASSERT_TRUE(disconnect);
   EXPECT_EQ(104, ID(disconnect));
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ(Peers()[1].mac.c_str(), disconnect->GetArgAt(1).data());

   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());
   auto negotiationStart = proxy->PopNextCommand();
   ASSERT_TRUE(negotiationStart);
   EXPECT_EQ(106, ID(negotiationStart));

   EXPECT_TRUE(proxy->NoCommands());
   disconnect->Respond(0);
   negotiationStart->Respond(0);

   // ...with only one remote peer
   std::string expectedOffer = R"(<Offer round="19"><Mac>)" + localMac + "</Mac></Offer>";
   auto sendOffer = proxy->PopNextCommand();
   ASSERT_TRUE(sendOffer);
   EXPECT_EQ(108, ID(sendOffer));
   EXPECT_EQ(2U, sendOffer->GetArgsCount());
   EXPECT_STREQ(expectedOffer.c_str(), sendOffer->GetArgAt(0).data());
   EXPECT_STREQ(Peers()[0].mac.c_str(), sendOffer->GetArgAt(1).data());
   sendOffer->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
}

using P2R20 = PlayingFixture<2u, 20u>;

TEST_F(P2R20, resets_and_goes_to_idle_on_game_stop)
{
   size_t prevBlockCount = cmd::pool.GetBlockCount();
   ctrl->OnEvent(16, {}); // game stopped

   {
      auto reset = proxy->PopNextCommand();
      ASSERT_TRUE(reset);
      EXPECT_EQ(115, ID(reset));
      EXPECT_EQ(0U, reset->GetArgsCount());
      reset->Respond(0);
   }
   {
      auto reset = proxy->PopNextCommand();
      ASSERT_TRUE(reset);
      EXPECT_EQ(114, ID(reset));
      EXPECT_EQ(0U, reset->GetArgsCount());
      reset->Respond(0);
   }

   EXPECT_EQ("New state: StateIdle ", logger.GetLastLine());
   EXPECT_TRUE(cmd::pool.GetBlockCount() <= prevBlockCount);
   auto btOn = proxy->PopNextCommand();
   ASSERT_TRUE(btOn);
   EXPECT_EQ(105, ID(btOn));
   EXPECT_EQ(0U, btOn->GetArgsCount());
   btOn->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
}

} // namespace