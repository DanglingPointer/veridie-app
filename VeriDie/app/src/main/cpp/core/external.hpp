#ifndef EXTERNAL_HPP
#define EXTERNAL_HPP

#ifdef __ANDROID__

#include "jni/logger.hpp"
#include "jni/proxy.hpp"

namespace external = jni;

#endif

#endif // EXTERNAL_HPP
