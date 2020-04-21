#include <gtest/gtest.h>
#include <queue>
#include <list>
#include <vector>

#include "tests/fakelogger.hpp"
#include "bt/device.hpp"
#include "core/controller.hpp"
#include "sign/commands.hpp"
#include "core/timerengine.hpp"
#include "jni/proxy.hpp"
#include "dice/engine.hpp"
#include "dice/serializer.hpp"

namespace {
using namespace std::chrono_literals;

class MockProxy : public jni::IProxy
{
public:
   std::unique_ptr<cmd::ICommand> PopNextCommand()
   {
      if (m_q.empty())
         return nullptr;
      auto c = std::move(m_q.front());
      m_q.pop();
      return c;
   }
   bool NoCommands() const { return m_q.empty(); }

private:
   void ForwardCommandToUi(std::unique_ptr<cmd::ICommand> c) override { m_q.push(std::move(c)); }
   void ForwardCommandToBt(std::unique_ptr<cmd::ICommand> c) override { m_q.push(std::move(c)); }
   void ForwardCommand(std::unique_ptr<cmd::ICommand> c) override { m_q.push(std::move(c)); }

private:
   std::queue<std::unique_ptr<cmd::ICommand>> m_q;
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
   void GenerateResult(dice::Cast &) override {}
};

class IdlingFixture : public ::testing::Test
{
protected:
   std::unique_ptr<core::IController> CreateController()
   {
      proxy = new MockProxy;
      timer = new MockTimerEngine;
      generator = new StubGenerator;
      return core::CreateController(std::unique_ptr<jni::IProxy>(proxy),
                                    std::unique_ptr<dice::IEngine>(generator),
                                    std::unique_ptr<core::ITimerEngine>(timer),
                                    dice::CreateXmlSerializer(),
                                    logger);
   }

   MockProxy * proxy;
   MockTimerEngine * timer;
   StubGenerator * generator;
   FakeLogger logger;
};

#define ID(id) (id << 8)

TEST_F(IdlingFixture, state_idle_bluetooth_turned_on_successfully)
{
   auto ctrl = CreateController();
   EXPECT_EQ("New state: StateIdle ", logger.GetLastLine());

   logger.Clear();
   auto c = proxy->PopNextCommand();
   EXPECT_TRUE(c);
   EXPECT_EQ(ID(105), c->GetId()); // enable bt
   EXPECT_EQ(0U, c->GetArgsCount());

   // bluetooth on
   c->Respond(0);
   ctrl->OnEvent(17, {});
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // new game requested
   ctrl->OnEvent(13, {});
   EXPECT_EQ("New state: StateConnecting ", logger.GetLastLine());
}

TEST_F(IdlingFixture, state_idle_bluetooth_fatal_failure)
{
   auto ctrl = CreateController();
   logger.Clear();

   auto c = proxy->PopNextCommand();
   EXPECT_TRUE(c);

   // user declined
   c->Respond(6);
   EXPECT_TRUE(logger.Empty());
   c = proxy->PopNextCommand();
   EXPECT_TRUE(c);
   EXPECT_EQ(ID(109), c->GetId()); // text message
   EXPECT_EQ(1U, c->GetArgsCount());
   EXPECT_TRUE(proxy->NoCommands());

   // no retries
   timer->FastForwardTime(2s);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // new game requested
   ctrl->OnEvent(13, {});
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());
}

TEST_F(IdlingFixture, state_idle_retries_to_enable_bluetooth)
{
   auto ctrl = CreateController();
   logger.Clear();

   auto c = proxy->PopNextCommand();
   EXPECT_TRUE(c);

   // invalid state
   c->Respond(0xffffffffffffffffLL);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // check retry
   timer->FastForwardTime(1s);
   c = proxy->PopNextCommand();
   EXPECT_TRUE(c);
   EXPECT_EQ(ID(105), c->GetId()); // enable bt
   EXPECT_EQ(0U, c->GetArgsCount());

   // no adapter
   c->Respond(5);
   c = proxy->PopNextCommand();
   EXPECT_TRUE(c);
   EXPECT_EQ(ID(109), c->GetId()); // text message
   EXPECT_EQ(1U, c->GetArgsCount());
   EXPECT_TRUE(proxy->NoCommands());

   // no retries
   timer->FastForwardTime(2s);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // new game requested
   ctrl->OnEvent(13, {});
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());
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
      auto cmdListening = proxy->PopNextCommand();
      EXPECT_TRUE(cmdListening);
      EXPECT_EQ(ID(100), cmdListening->GetId());
      auto cmdDiscovering = proxy->PopNextCommand();
      EXPECT_TRUE(cmdDiscovering);
      EXPECT_EQ(ID(101), cmdDiscovering->GetId());
      cmdDiscovering->Respond(0);
      cmdListening->Respond(0);
      EXPECT_TRUE(proxy->NoCommands());
   }

   std::unique_ptr<core::IController> ctrl;
};

TEST_F(ConnectingFixture, discovery_and_listening_started_successfully)
{
   auto cmdListening = proxy->PopNextCommand();
   EXPECT_TRUE(cmdListening);
   EXPECT_EQ(ID(100), cmdListening->GetId());
   EXPECT_EQ(3U, cmdListening->GetArgsCount());
   EXPECT_EQ("60", cmdListening->GetArgAt(2));

   auto cmdDiscovering = proxy->PopNextCommand();
   EXPECT_TRUE(cmdDiscovering);
   EXPECT_EQ(ID(101), cmdDiscovering->GetId());
   EXPECT_EQ(3U, cmdDiscovering->GetArgsCount());
   EXPECT_STREQ("true", cmdDiscovering->GetArgAt(2).data());

   cmdListening->Respond(0);
   cmdDiscovering->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, fatal_failure_when_both_discovery_and_listening_failed)
{
   auto cmdListening = proxy->PopNextCommand();
   EXPECT_TRUE(cmdListening);
   auto cmdDiscovering = proxy->PopNextCommand();
   EXPECT_TRUE(cmdDiscovering);

   logger.Clear();
   cmdDiscovering->Respond(0xffffffffffffffff);
   cmdListening->Respond(3);

   auto fatalFailureText = proxy->PopNextCommand();
   EXPECT_TRUE(fatalFailureText);
   EXPECT_EQ(ID(109), fatalFailureText->GetId());
   EXPECT_TRUE(logger.Empty());
}

TEST_F(ConnectingFixture, no_fatal_failure_when_only_listening_failed)
{
   auto cmdListening = proxy->PopNextCommand();
   EXPECT_TRUE(cmdListening);
   auto cmdDiscovering = proxy->PopNextCommand();
   EXPECT_TRUE(cmdDiscovering);

   cmdDiscovering->Respond(0);
   cmdListening->Respond(3);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, no_fatal_failure_when_only_discovery_failed)
{
   auto cmdListening = proxy->PopNextCommand();
   EXPECT_TRUE(cmdListening);
   auto cmdDiscovering = proxy->PopNextCommand();
   EXPECT_TRUE(cmdDiscovering);

   cmdDiscovering->Respond(0xffffffffffffffff);
   cmdListening->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, goes_to_idle_and_back_if_bluetooth_is_off)
{
   auto cmdListening = proxy->PopNextCommand();
   EXPECT_TRUE(cmdListening);
   auto cmdDiscovering = proxy->PopNextCommand();
   EXPECT_TRUE(cmdDiscovering);

   cmdDiscovering->Respond(2);
   cmdListening->Respond(2);
   EXPECT_EQ("New state: StateIdle ", logger.GetLastLine());
   auto enableBt = proxy->PopNextCommand();
   EXPECT_TRUE(enableBt);
   EXPECT_EQ(ID(105), enableBt->GetId());
   EXPECT_TRUE(proxy->NoCommands());
   enableBt->Respond(0);
   EXPECT_EQ("New state: StateConnecting ", logger.GetLastLine());
}

TEST_F(ConnectingFixture, sends_hello_to_connected_device)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto cmdHello = proxy->PopNextCommand();
   EXPECT_TRUE(cmdHello);
   EXPECT_EQ(ID(108), cmdHello->GetId());
   EXPECT_EQ(2U, cmdHello->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:49", cmdHello->GetArgAt(0).data());
   EXPECT_STREQ(R"(<Hello><Mac>5c:b9:01:f8:b6:49</Mac></Hello>)", cmdHello->GetArgAt(1).data());
   cmdHello->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, retries_hello_on_invalid_state_and_disconnects_on_socket_error)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto hello1 = proxy->PopNextCommand();
   EXPECT_TRUE(hello1);
   EXPECT_EQ(ID(108), hello1->GetId());
   hello1->Respond(0xffffffffffffffff);

   auto hello2 = proxy->PopNextCommand();
   EXPECT_TRUE(hello2);
   EXPECT_EQ(ID(108), hello2->GetId());
   hello2->Respond(7);

   auto disconnect = proxy->PopNextCommand();
   EXPECT_TRUE(disconnect);
   EXPECT_EQ(ID(104), disconnect->GetId());
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:49", disconnect->GetArgAt(0).data());
   disconnect->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, disconnects_on_read_error_and_does_not_retry_hello)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto hello = proxy->PopNextCommand();
   EXPECT_TRUE(hello);
   EXPECT_EQ(ID(108), hello->GetId());
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(19, {"5c:b9:01:f8:b6:49", ""});
   auto disconnect = proxy->PopNextCommand();
   EXPECT_TRUE(disconnect);
   EXPECT_EQ(ID(104), disconnect->GetId());
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:49", disconnect->GetArgAt(0).data());
   disconnect->Respond(0);

   hello->Respond(0xffffffffffffffff);
   EXPECT_TRUE(proxy->NoCommands());
}

TEST_F(ConnectingFixture, does_not_start_negotiation_until_received_own_mac)
{
   StartDiscoveryAndListening();

   ctrl->OnEvent(10, {"5c:b9:01:f8:b6:49", "Chalie Chaplin"});

   auto cmdHello = proxy->PopNextCommand();
   EXPECT_TRUE(cmdHello);
   EXPECT_EQ(ID(108), cmdHello->GetId());
   cmdHello->Respond(0);
   logger.Clear();

   ctrl->OnEvent(12, {}); // start
   auto toast = proxy->PopNextCommand();
   EXPECT_TRUE(toast);
   EXPECT_EQ(ID(110), toast->GetId());
   EXPECT_EQ(2U, toast->GetArgsCount());
   EXPECT_STREQ("3", toast->GetArgAt(1).data());
   EXPECT_TRUE(logger.Empty());

   timer->FastForwardTime(1s);
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   ctrl->OnEvent(14, {R"(<Hello><Mac>f4:06:69:7b:4b:e7</Mac></Hello>)", "5c:b9:01:f8:b6:49", ""});
   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   timer->FastForwardTime(1s);
   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());

   auto stopDiscovery = proxy->PopNextCommand();
   EXPECT_TRUE(stopDiscovery);
   EXPECT_EQ(ID(103), stopDiscovery->GetId());

   auto stopListening = proxy->PopNextCommand();
   EXPECT_TRUE(stopListening);
   EXPECT_EQ(ID(102), stopListening->GetId());
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
      EXPECT_TRUE(cmdHello);
      cmdHello->Respond(0);
   }

   // 1 weird peer
   ctrl->OnEvent(14, {R"(<Hello><Mac>5c:b9:01:f8:b6:40</Mac></Hello>)", "5c:b9:01:f8:b6:44", "Charlie Chaplin 4"});
   auto cmdHello = proxy->PopNextCommand();
   EXPECT_TRUE(cmdHello);
   EXPECT_EQ(ID(108), cmdHello->GetId());
   EXPECT_EQ(2U, cmdHello->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:44", cmdHello->GetArgAt(0).data());
   EXPECT_STREQ(R"(<Hello><Mac>5c:b9:01:f8:b6:44</Mac></Hello>)", cmdHello->GetArgAt(1).data());
   cmdHello->Respond(0);

   // 1 disconnects
   ctrl->OnEvent(19, {"5c:b9:01:f8:b6:42", ""});
   auto disconnect = proxy->PopNextCommand();
   EXPECT_TRUE(disconnect);
   EXPECT_EQ(ID(104), disconnect->GetId());
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:42", disconnect->GetArgAt(0).data());
   disconnect->Respond(0);

   EXPECT_TRUE(proxy->NoCommands());
   EXPECT_TRUE(logger.Empty());

   // game start
   ctrl->OnEvent(12, {});
   EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());

   auto stopDiscovery = proxy->PopNextCommand();
   EXPECT_TRUE(stopDiscovery);
   auto stopListening = proxy->PopNextCommand();
   EXPECT_TRUE(stopListening);

   auto negotiationStart = proxy->PopNextCommand();
   EXPECT_TRUE(negotiationStart);
   EXPECT_EQ(ID(106), negotiationStart->GetId());
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
      EXPECT_TRUE(offer);
      EXPECT_EQ(ID(108), offer->GetId());
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ("5c:b9:01:f8:b6:44", offer->GetArgAt(0).data());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(1).data());
      offer->Respond(0);
   }
   {
      auto offer = proxy->PopNextCommand();
      EXPECT_TRUE(offer);
      EXPECT_EQ(ID(108), offer->GetId());
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ("5c:b9:01:f8:b6:43", offer->GetArgAt(0).data());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(1).data());
      offer->Respond(0);
   }
   {
      auto offer = proxy->PopNextCommand();
      EXPECT_TRUE(offer);
      EXPECT_EQ(ID(108), offer->GetId());
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ("5c:b9:01:f8:b6:41", offer->GetArgAt(0).data());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(1).data());
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
         peers.emplace_back("Charlie Chaplin " + std::to_string(i), "5c:b9:01:f8:b6:4" + std::to_string(i));

      for (const auto & peer : peers) {
         ctrl->OnEvent(10, {peer.mac, peer.name});
         auto cmdHello = proxy->PopNextCommand();
         EXPECT_TRUE(cmdHello);
         EXPECT_EQ(ID(108), cmdHello->GetId());
         cmdHello->Respond(0);
      }
      for (const auto & peer : peers) {
         ctrl->OnEvent(14, {R"(<Hello><Mac>)" + localMac + R"(</Mac></Hello>)", peer.mac, peer.name});
         EXPECT_TRUE(proxy->NoCommands());
      }
      ctrl->OnEvent(12, {}); // start
      EXPECT_EQ("New state: StateNegotiating ", logger.GetLastLine());

      auto stopDiscovery = proxy->PopNextCommand();
      EXPECT_TRUE(stopDiscovery);
      EXPECT_EQ(ID(103), stopDiscovery->GetId());

      auto stopListening = proxy->PopNextCommand();
      EXPECT_TRUE(stopListening);
      EXPECT_EQ(ID(102), stopListening->GetId());
      logger.Clear();
   }
   void CheckLocalOffer(const char * expectedOffer)
   {
      for (auto it = std::crbegin(Peers()); it != std::crend(Peers()); ++it) {
         auto offer = proxy->PopNextCommand();
         EXPECT_TRUE(offer);
         EXPECT_EQ(ID(108), offer->GetId());
         EXPECT_EQ(2U, offer->GetArgsCount());
         EXPECT_STREQ(it->mac.c_str(), offer->GetArgAt(0).data());
         EXPECT_STREQ(expectedOffer, offer->GetArgAt(1).data());
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
   EXPECT_TRUE(negotiationStart);
   EXPECT_EQ(ID(106), negotiationStart->GetId());
   negotiationStart->Respond(0);

   // sorted list of candidates should be:
   // 5c:b9:01:f8:b6:40 <-- round 5
   // 5c:b9:01:f8:b6:41 <-- round 6
   // 5c:b9:01:f8:b6:42
   // 5c:b9:01:f8:b6:43
   // 5c:b9:01:f8:b6:44 <-- round 4
   CheckLocalOffer(R"(<Offer round="4"><Mac>5c:b9:01:f8:b6:44</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(14, {R"(<Offer round="5"><Mac>5c:b9:01:f8:b6:40</Mac></Offer>)", "5c:b9:01:f8:b6:42", ""});
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(14, {R"(<Offer round="3"><Mac>5c:b9:01:f8:b6:43</Mac></Offer>)", "5c:b9:01:f8:b6:41", ""});
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   CheckLocalOffer(R"(<Offer round="5"><Mac>5c:b9:01:f8:b6:40</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(14, {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:40", ""});
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   CheckLocalOffer(R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(14, {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:41", ""});
   ctrl->OnEvent(14, {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:42", ""});
   ctrl->OnEvent(14, {R"(<Offer round="6"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:43", ""});

   timer->FastForwardTime(1s);
   auto negotiationStop = proxy->PopNextCommand();
   EXPECT_TRUE(negotiationStop);
   EXPECT_EQ(ID(107), negotiationStop->GetId());
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
   EXPECT_TRUE(negotiationStart);
   EXPECT_EQ(ID(106), negotiationStart->GetId());
   negotiationStart->Respond(0);

   // sorted list of candidates should be:
   // 5c:b9:01:f8:b6:40
   // 5c:b9:01:f8:b6:41 <-- round 7
   // 5c:b9:01:f8:b6:42
   CheckLocalOffer(R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   ctrl->OnEvent(14, {R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)", "5c:b9:01:f8:b6:41", ""});
   // peer 1 disconnected from peer 0
   ctrl->OnEvent(14, {R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:42</Mac></Offer>)", "5c:b9:01:f8:b6:40", ""});

   timer->FastForwardTime(1s);
   CheckLocalOffer(R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:41</Mac></Offer>)");
   EXPECT_TRUE(proxy->NoCommands());

   // peer 1 disconnected from us as well
   ctrl->OnEvent(19, {"5c:b9:01:f8:b6:41", ""});
   auto disconnect = proxy->PopNextCommand();
   EXPECT_TRUE(disconnect);
   EXPECT_EQ(ID(104), disconnect->GetId());
   EXPECT_EQ(2U, disconnect->GetArgsCount());
   EXPECT_STREQ("5c:b9:01:f8:b6:41", disconnect->GetArgAt(0).data());
   disconnect->Respond(0);
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   const char * expectedOffer = R"(<Offer round="7"><Mac>5c:b9:01:f8:b6:42</Mac></Offer>)";
   {
      auto offer = proxy->PopNextCommand();
      EXPECT_TRUE(offer);
      EXPECT_EQ(ID(108), offer->GetId());
      EXPECT_EQ(2U, offer->GetArgsCount());
      EXPECT_STREQ("5c:b9:01:f8:b6:40", offer->GetArgAt(0).data());
      EXPECT_STREQ(expectedOffer, offer->GetArgAt(1).data());
      offer->Respond(0);
   }
   EXPECT_TRUE(proxy->NoCommands());

   timer->FastForwardTime(1s);
   auto negotiationStop = proxy->PopNextCommand();
   EXPECT_TRUE(negotiationStop);
   EXPECT_EQ(ID(107), negotiationStop->GetId());
   EXPECT_EQ(1U, negotiationStop->GetArgsCount());
   EXPECT_STREQ("You", negotiationStop->GetArgAt(0).data());
   negotiationStop->Respond(0);
   EXPECT_EQ("New state: StatePlaying ", logger.GetLastLine());
   EXPECT_TRUE(proxy->NoCommands());
}

} // namespace