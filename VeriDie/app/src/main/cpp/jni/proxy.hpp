#ifndef JNI_PROXY_HPP
#define JNI_PROXY_HPP

#include <memory>

class ILogger;
namespace cmd {
class ICommand;
}

namespace jni {

class IProxy
{
public:
   virtual ~IProxy() = default;
   virtual void ForwardCommandToUi(std::unique_ptr<cmd::ICommand> c) = 0;
   virtual void ForwardCommandToBt(std::unique_ptr<cmd::ICommand> c) = 0;
   virtual void ForwardCommand(std::unique_ptr<cmd::ICommand> c) = 0;

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
};

std::unique_ptr<IProxy> CreateProxy(ILogger & logger);

} // namespace jni

#endif // JNI_PROXY_HPP
