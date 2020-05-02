#ifndef JNI_PROXY_HPP
#define JNI_PROXY_HPP

#include <memory>
#include "core/proxy.hpp"

class ILogger;

namespace jni {

std::unique_ptr<core::Proxy> CreateProxy(ILogger & logger);

} // namespace jni

#endif // JNI_PROXY_HPP
