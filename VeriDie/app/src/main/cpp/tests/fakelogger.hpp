#ifndef TESTS_FAKELOGGER_HPP
#define TESTS_FAKELOGGER_HPP

#include <vector>
#include <mutex>
#include "core/logging.hpp"

class FakeLogger : public ILogger
{
public:
   struct Entry {
      LogPriority prio;
      std::string msg;
   };

   void Write(LogPriority prio, std::string msg) override
   {
      std::lock_guard lg(mutex);
      entries.push_back({prio, std::move(msg)});
   }
   std::vector<Entry> GetEntries() const
   {
      std::lock_guard lg(mutex);
      return entries;
   }
   const std::string & GetLastLine() const
   {
      std::lock_guard lg(mutex);
      return entries.back().msg;
   }
   bool Empty() const
   {
      std::lock_guard lg(mutex);
      return entries.empty();
   }
   void Clear()
   {
      std::lock_guard lg(mutex);
      entries.clear();
   }
   bool NoWarningsOrErrors() const
   {
      std::lock_guard lg(mutex);
      for (const auto & entry : entries) {
         switch (entry.prio) {
            case LogPriority::ERROR:
            case LogPriority::WARN:
            case LogPriority::FATAL:
               return false;
            default:
               break;
         }
      }
      return true;
   }
   void DumpLines() const
   {
      for (const auto & entry : entries) {
         fprintf(stderr, "Prio(%d): %s\n", static_cast<int>(entry.prio), entry.msg.c_str());
      }
   }

private:
   mutable std::mutex mutex;
   std::vector<Entry> entries;
};

#endif // TESTS_FAKELOGGER_HPP
