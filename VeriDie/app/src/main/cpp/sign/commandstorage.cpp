#include "sign/commandstorage.hpp"
#include "sign/commands.hpp"
#include "utils/logger.hpp"

namespace cmd {

Storage::Storage(ILogger & logger)
   : m_log(logger)
{}

Storage::~Storage() = default;

int32_t Storage::Store(mem::pool_ptr<ICommand> && cmd)
{
   int32_t id = cmd->GetId();
   while (m_content.count(id))
      ++id;
   if ((id - cmd->GetId()) >= COMMAND_ID(1)) {
      m_log.Write<LogPriority::ERROR>("cmd::Storage is full for ID =", cmd->GetId());
      return 0;
   }
   m_content.emplace(id, std::move(cmd));
   return id;
}

mem::pool_ptr<ICommand> Storage::Retrieve(int32_t id)
{
   auto it = m_content.find(id);
   if (it == std::end(m_content)) {
      m_log.Write<LogPriority::WARN>("cmd::Storage received orphaned response, ID =", id);
      return nullptr;
   }
   auto cmd = std::move(it->second);
   m_content.erase(it);
   return cmd;
}

} // namespace cmd