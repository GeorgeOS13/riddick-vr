cmake_minimum_required(VERSION 3.25)
project(RiddickVR VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# The project consists of:
# 1. riddick_vr_hook (32-bit x86 DLL injected into DarkAthena.exe)
# 2. riddick_vr_host (64-bit x64 Companion executable interfacing with OpenXR)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Configuring 32-bit target (riddick_vr_hook)...")
    # 32-bit Hook DLL build targets will go here
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Configuring 64-bit target (riddick_vr_host)...")
    # 64-bit Host Executable build targets will go here
endif()
