cmake_minimum_required(VERSION 3.22)
project(SwitchUIAndroid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT ANDROID)
    message(FATAL_ERROR "This CMake project is intended for Android NDK builds.")
endif()

if(NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "SwitchUI Android build supports only arm64-v8a.")
endif()

add_library(
    switchui_native
    SHARED
    Native/Android/jit_memory_android.cpp
    Native/Android/switchui_android_jni.cpp
    Native/Renderer/vulkan_backend_android.cpp
)

target_include_directories(
    switchui_native
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Native
    ${CMAKE_CURRENT_SOURCE_DIR}/Native/Android
    ${CMAKE_CURRENT_SOURCE_DIR}/Native/Renderer
)

find_library(log-lib log)
find_library(android-lib android)
find_library(vulkan-lib vulkan)

target_link_libraries(
    switchui_native
    PRIVATE
    ${log-lib}
    ${android-lib}
    ${vulkan-lib}
)
