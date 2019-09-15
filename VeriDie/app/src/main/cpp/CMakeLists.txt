# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR})

find_library(log-lib
        log)


# main app logic:
add_library(veridie
        SHARED
        dice/engine.cpp
        dice/serializer.cpp
        jni/exec.cpp
        jni/logger.cpp
        jni/btproxy.cpp
        jni/btinvoker.cpp
        jni/uiproxy.cpp
        jni/uiinvoker.cpp
        bt/device.cpp
        core/exec.cpp
        core/controller.cpp
        core/timerengine.cpp
        fsm/stateidle.cpp
        fsm/stateplaying.cpp
        fsm/stateconnecting.cpp
        fsm/statenegotiating.cpp
        utils/worker.cpp)
target_link_libraries(veridie
        ${log-lib})


# fake library with empty controller for testing jni interop:
add_library(jni_only_veridie
        SHARED
        dice/engine.cpp
        dice/serializer.cpp
        jni/exec.cpp
        jni/logger.cpp
        jni/btproxy.cpp
        jni/btinvoker.cpp
        jni/uiproxy.cpp
        jni/uiinvoker.cpp
        bt/device.cpp
        core/exec.cpp
        tests/fakecontroller.cpp
        core/timerengine.cpp
        utils/worker.cpp)
target_link_libraries(jni_only_veridie
        ${log-lib})


# google test library:
set(GOOGLETEST_ROOT ${ANDROID_NDK}/sources/third_party/googletest)
add_library(gtest STATIC
        ${GOOGLETEST_ROOT}/src/gtest_main.cc
        ${GOOGLETEST_ROOT}/src/gtest-all.cc)
target_include_directories(gtest PRIVATE ${GOOGLETEST_ROOT})
target_include_directories(gtest PUBLIC ${GOOGLETEST_ROOT}/include)


# google test executable:
add_executable(tests
        tests/dice_test.cpp
        tests/bt_test.cpp
        tests/async_test.cpp
        tests/threading_test.cpp)
target_link_libraries(tests
        gtest
        veridie)


# run gtest executable
find_program(ADB ${ANDROID_NDK}/../platform-tools/adb)
if(NOT ADB)
    message(FATAL_ERROR "Could not find adb")
endif()
add_custom_command(TARGET tests POST_BUILD
        COMMAND ${ADB} shell mkdir -p /data/local/tmp/${ANDROID_ABI}
        COMMAND ${ADB} push $<TARGET_FILE:veridie> /data/local/tmp/${ANDROID_ABI}/
        COMMAND ${ADB} push $<TARGET_FILE:tests> /data/local/tmp/${ANDROID_ABI}/
        COMMAND ${ADB} shell \"export LD_LIBRARY_PATH=/data/local/tmp/${ANDROID_ABI}\; /data/local/tmp/${ANDROID_ABI}/tests || true\")
