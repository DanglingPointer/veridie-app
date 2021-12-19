# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

project(VeriDie)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wpedantic -Wimplicit-fallthrough -Wno-sign-compare")
if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts")
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
endif()

include_directories(${PROJECT_SOURCE_DIR})


include(FetchContent)

FetchContent_Declare(googletest
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           release-1.11.0
  SOURCE_DIR        "${CMAKE_CURRENT_BINARY_DIR}/googletest-src"
  BINARY_DIR        "${CMAKE_CURRENT_BINARY_DIR}/googletest-build"
)

# Don't build gmock
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


# cross-platform main logic
add_library(veridiecore
        STATIC
        core/commandadapter.cpp
        core/controller.cpp
        core/log.cpp
        core/timer.cpp
        dice/engine.cpp
        dice/serializer.cpp
        fsm/stateconnecting.cpp
        fsm/stateidle.cpp
        fsm/statenegotiating.cpp
        fsm/stateplaying.cpp
        sign/commands.cpp
        sign/commandmanager.cpp
        sign/events.cpp
        utils/format.cpp
        )
target_compile_features(veridiecore
        PUBLIC cxx_std_20)

if(ANDROID)

    find_library(log-lib
            log)

    # veridie for android
    add_library(veridie
            SHARED
            core/exec.cpp
            jni/exec.cpp
            jni/javainvoker.cpp
            utils/worker.cpp
            )
    target_link_libraries(veridie
            ${log-lib}
            veridiecore)


    # echo controller for testing jni interop via instrumented tests
    add_library(jni_only_veridie
            SHARED
            tests/fakecontroller.cpp
            core/exec.cpp
            core/log.cpp
            jni/exec.cpp
            jni/javainvoker.cpp
            sign/commandmanager.cpp
            utils/format.cpp
            utils/worker.cpp
            )
    target_link_libraries(jni_only_veridie
            ${log-lib})

endif()


## google test static lib
#set(GOOGLETEST_ROOT ${ANDROID_NDK}/sources/third_party/googletest)
#add_library(gtest STATIC
#        ${GOOGLETEST_ROOT}/src/gtest_main.cc
#        ${GOOGLETEST_ROOT}/src/gtest-all.cc)
#target_include_directories(gtest PRIVATE ${GOOGLETEST_ROOT})
#target_include_directories(gtest PUBLIC ${GOOGLETEST_ROOT}/include)


# unit tests executable
add_executable(tests
        utils/worker.cpp
        tests/test_dice.cpp
        tests/test_task.cpp
        tests/test_threading.cpp
        tests/test_cmd.cpp
        tests/test_controller.cpp
        tests/test_log.cpp
        tests/test_mempool.cpp)
target_link_libraries(tests
        gtest_main
        veridiecore)


if(ANDROID)
    # push and run unit tests
    find_program(ADB ${ANDROID_NDK}/../../platform-tools/adb)
    if(NOT ADB)
        message(FATAL_ERROR "Could not find adb")
    endif()
    add_custom_command(TARGET tests POST_BUILD
            COMMAND ${ADB} shell mkdir -p /data/local/tmp/${ANDROID_ABI}
            COMMAND ${ADB} push $<TARGET_FILE:tests> /data/local/tmp/${ANDROID_ABI}/
            COMMAND ${ADB} shell \"export LD_LIBRARY_PATH=/data/local/tmp/${ANDROID_ABI}\; /data/local/tmp/${ANDROID_ABI}/tests\")
endif()

#
#
# adb gdbserver :5039 shell \"export LD_LIBRARY_PATH=/data/local/tmp/armeabi-v7a\; /data/local/tmp/armeabi-v7a/tests\"
# export LD_LIBRARY_PATH=/data/local/tmp/armeabi-v7a; /data/local/tmp/armeabi-v7a/tests
