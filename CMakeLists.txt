cmake_minimum_required(VERSION 3.10.2)
project("switchui_native")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Vulkan search path (standard for NDK)
find_library(log-lib log)
find_library(android-lib android)
find_library(vulkan-lib vulkan)

# Source files
set(SOURCE_FILES
    android/cpp/jni/switchui_android_jni.cpp
    android/cpp/gpu/vulkan_context.cpp
    android/cpp/memory/jit_memory.cpp
)

# Shared library target
add_library(switchui_native SHARED ${SOURCE_FILES})

# Include directories
target_include_directories(switchui_native PRIVATE
    android/cpp/gpu
    android/cpp/memory
    android/cpp/jni
)

# Link libraries
target_link_libraries(switchui_native
    ${log-lib}
    ${android-lib}
    ${vulkan-lib}
)
