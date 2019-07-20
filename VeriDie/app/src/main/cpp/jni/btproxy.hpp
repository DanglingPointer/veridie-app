#ifndef JNI_BTPROXY_HPP
#define JNI_BTPROXY_HPP

#include <memory>
#include "bt/proxy.hpp"

namespace jni {

std::unique_ptr<bt::IProxy> CreateBtProxy();
}

#endif // JNI_BTPROXY_HPP
