#ifndef UI_PROXY_HPP
#define UI_PROXY_HPP

namespace ui {

class IProxy
{
public:
   virtual ~IProxy() = default;
   virtual void ShowToast(const std::string & message, int duration);
};

} // namespace ui

#endif // UI_PROXY_HPP
