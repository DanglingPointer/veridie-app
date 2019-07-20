#ifndef JNI_UIPROXY_HPP
#define JNI_UIPROXY_HPP

#include <memory>
#include "ui/proxy.hpp"

namespace jni {

std::unique_ptr<ui::IProxy> CreateUiProxy();
}

#endif // JNI_UIPROXY_HPP
