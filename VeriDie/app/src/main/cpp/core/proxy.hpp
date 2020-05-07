#ifndef CORE_PROXY_HPP
#define CORE_PROXY_HPP

#include <cassert>
#include <memory>
#include "sign/commands.hpp"
#include "utils/poolptr.hpp"

namespace core {

class Proxy
{
public:
   virtual ~Proxy() = default;
   virtual void ForwardCommandToUi(mem::pool_ptr<cmd::ICommand> c) = 0;
   virtual void ForwardCommandToBt(mem::pool_ptr<cmd::ICommand> c) = 0;
   void ForwardCommand(mem::pool_ptr<cmd::ICommand> c)
   {
      if (Contains(cmd::UiDictionary{}, c->GetId())) {
         ForwardCommandToUi(std::move(c));
         return;
      }
      if (Contains(cmd::BtDictionary{}, c->GetId())) {
         ForwardCommandToBt(std::move(c));
         return;
      }
      assert(false);
   }

   Proxy & operator<<(mem::pool_ptr<cmd::ICommand> c)
   {
      ForwardCommand(std::move(c));
      return *this;
   }

private:
   template <typename... Ts>
   static bool Contains(cmd::List<Ts...>, int32_t value)
   {
      return ((Ts::ID == value) || ...);
   }
};

} // namespace core

#endif // CORE_PROXY_HPP
