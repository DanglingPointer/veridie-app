#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <memory>
#include "utils/logging.hpp"

namespace jni {

std::unique_ptr<ILogger> CreateLogger(std::string tag);
}

#endif // LOGGER_HPP
