#ifndef CORE_PROXY_HPP
#define CORE_PROXY_HPP

#include <cassert>
#include <memory>
#include "sign/commands.hpp"

namespace core {

class Proxy
{
public:
   virtual ~Proxy() = default;
   virtual void ForwardCommandToUi(std::unique_ptr<cmd::ICommand> c) = 0;
   virtual void ForwardCommandToBt(std::unique_ptr<cmd::ICommand> c) = 0;
   void ForwardCommand(std::unique_ptr<cmd::ICommand> c)
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

   template <typename TCmd, typename... TArgs>
   void ForwardToUi(TArgs &&... args)
   {
      ForwardCommandToUi(std::make_unique<TCmd>(std::forward<TArgs>(args)...));
   }
   template <typename TCmd, typename... TArgs>
   void ForwardToBt(TArgs &&... args)
   {
      ForwardCommandToBt(std::make_unique<TCmd>(std::forward<TArgs>(args)...));
   }
   template <typename TCmd, typename... TArgs>
   void Forward(TArgs &&... args)
   {
      ForwardCommand(std::make_unique<TCmd>(std::forward<TArgs>(args)...));
   }

private:
   template <typename... Ts>
   static bool Contains(cmd::List<Ts...>, int32_t value)
   {
      return ((Ts::ID == value) || ...);
   }
};

}

#endif // CORE_PROXY_HPP
