# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR})
# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        veridie

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        dice/engine.cpp
        dice/serializer.cpp
        jni/interop.cpp
        jni/logger.cpp
        jni/btinvoker.cpp
        jni/uiinvoker.cpp
        jni/btproxy.cpp
        bt/device.cpp
        utils/worker.cpp
        core/exec.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        veridie

        # Links the target library to the log library
        # included in the NDK.
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
        tests/bt_test.cpp)
target_link_libraries(tests
        gtest
        veridie)

## run tests
#find_program(ADB adb)
#add_custom_command(TARGET tests POST_BUILD
#        COMMAND ${ADB} shell mkdir -p /data/local/tmp/${ANDROID_ABI}
#        COMMAND ${ADB} push $<TARGET_FILE:veridie> /data/local/tmp/${ANDROID_ABI}/
#        COMMAND ${ADB} push $<TARGET_FILE:tests> /data/local/tmp/${ANDROID_ABI}/
#        COMMAND ${ADB} shell \"export LD_LIBRARY_PATH=/data/local/tmp/${ANDROID_ABI}\; /data/local/tmp/${ANDROID_ABI}/tests\")
