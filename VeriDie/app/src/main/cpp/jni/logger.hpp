#ifndef JNI_LOGGER_HPP
#define JNI_LOGGER_HPP

#include <string>
#include <memory>
#include "core/logging.hpp"

namespace jni {

std::unique_ptr<ILogger> CreateLogger(std::string tag);
}

#endif // JNI_LOGGER_HPP
