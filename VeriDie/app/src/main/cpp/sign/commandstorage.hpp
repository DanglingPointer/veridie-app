#ifndef SIGN_COMMANDSTORAGE_HPP
#define SIGN_COMMANDSTORAGE_HPP

#include <cstdint>
#include <unordered_map>
#include "utils/poolptr.hpp"

class ILogger;

namespace cmd {
class ICommand;

class Storage
{
public:
   explicit Storage(ILogger & logger);
   ~Storage();
   int32_t Store(mem::pool_ptr<ICommand> && cmd);
   mem::pool_ptr<ICommand> Retrieve(int32_t id);

private:
   ILogger & m_log;
   std::unordered_map<int32_t, mem::pool_ptr<ICommand>> m_content;
};

} // namespace cmd

#endif // SIGN_COMMANDSTORAGE_HPP
