#ifndef TESTS_FAKELOGGER_HPP
#define TESTS_FAKELOGGER_HPP

#include "core/logging.hpp"
#include <vector>

class FakeLogger : ILogger
{
public:
   virtual void Write(LogPriority prio, std::string msg) override
   {
      entries.emplace_back(prio, std::move(msg));
   }

   struct Entry {
      LogPriority prio;
      std::string msg;
   };
   std::vector<Entry> entries;
};

#endif // TESTS_FAKELOGGER_HPP
