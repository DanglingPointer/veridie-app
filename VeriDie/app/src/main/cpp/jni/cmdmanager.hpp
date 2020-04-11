#ifndef JNI_CMD_MANAGER_HPP
#define JNI_CMD_MANAGER_HPP

#include <memory>
#include <cstdint>
#include <jni.h>

class ILogger;

namespace cmd {
class ICommand;
}

namespace jni {

class ICmdManager
{
public:
   virtual ~ICmdManager() = default;
   virtual void IssueUiCommand(std::unique_ptr<cmd::ICommand> c) = 0;
   virtual void IssueBtCommand(std::unique_ptr<cmd::ICommand> c) = 0;
   virtual void OnCommandResponse(int32_t cmdId, int64_t response) = 0;
};

std::unique_ptr<ICmdManager> CreateCmdManager(ILogger & logger, JNIEnv * env, jclass globalRef);

} // namespace jni

#endif // JNI_CMD_MANAGER_HPP
