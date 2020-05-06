# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wpedantic -Wimplicit-fallthrough -Wno-sign-compare -Wno-error=c++11-narrowing")

include_directories(${PROJECT_SOURCE_DIR})

find_library(log-lib
        log)


# main app logic:
add_library(veridie
        SHARED

        sign/commands.cpp
        sign/events.cpp

        core/controller.cpp
        core/exec.cpp
        core/timerengine.cpp

        dice/engine.cpp
        dice/serializer.cpp

        fsm/stateconnecting.cpp
        fsm/stateidle.cpp
        fsm/statenegotiating.cpp
        fsm/stateplaying.cpp

        jni/cmdmanager.cpp
        jni/exec.cpp
        jni/logger.cpp
        jni/proxy.cpp

        utils/async.cpp
        utils/worker.cpp
        )
target_link_libraries(veridie
        ${log-lib})


# fake library with empty controller for testing jni interop:
add_library(jni_only_veridie
        SHARED

        sign/commands.cpp

        tests/fakecontroller.cpp
        core/exec.cpp

        # no dice/

        # no fsm/

        jni/cmdmanager.cpp
        jni/exec.cpp
        jni/logger.cpp
        jni/proxy.cpp

        utils/async.cpp
        utils/worker.cpp
        )
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
        tests/async_test.cpp
        tests/threading_test.cpp
        tests/cmd_test.cpp
        tests/controller_test.cpp)
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
        COMMAND ${ADB} shell \"export LD_LIBRARY_PATH=/data/local/tmp/${ANDROID_ABI}\; /data/local/tmp/${ANDROID_ABI}/tests\")


#
#
# adb gdbserver :5039 shell \"export LD_LIBRARY_PATH=/data/local/tmp/armeabi-v7a\; /data/local/tmp/armeabi-v7a/tests\"
# export LD_LIBRARY_PATH=/data/local/tmp/armeabi-v7a; /data/local/tmp/armeabi-v7a/tests
