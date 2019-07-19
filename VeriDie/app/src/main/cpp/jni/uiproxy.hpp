#ifndef UIPROXY_HPP
#define UIPROXY_HPP

#include <memory>
#include "ui/proxy.hpp"

namespace jni {

std::unique_ptr<ui::IProxy> CreateUiProxy();
}

#endif // UIPROXY_HPP
