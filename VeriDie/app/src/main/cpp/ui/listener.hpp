#ifndef UI_LISTENER_HPP
#define UI_LISTENER_HPP

#include <string>
#include "dice/serializer.hpp"

namespace ui {

class IListener
{
public:
   virtual ~IListener() = default;
   virtual void OnDevicesQuery(bool connected) = 0;
   virtual void OnNameSet(std::string name) = 0;
   virtual void OnLocalNameQuery() = 0;
   virtual void OnCastRequest(dice::Request localRequest) = 0;
   virtual void OnCandidateApproved(const bt::Device & candidatePlayer) = 0;
   virtual void OnNewGame() = 0;
   virtual void OnRestoringState() = 0;
   virtual void OnSavingState() = 0;
};

} // namespace ui

#endif // UI_LISTENER_HPP
