#ifndef BTPROXY_HPP
#define BTPROXY_HPP

#include <memory>
#include "bt/proxy.hpp"

namespace jni {

std::unique_ptr<bt::IProxy> CreateBtProxy();
}

#endif // BTPROXY_HPP
