cmake_minimum_required(VERSION 3.25)
project(RiddickVR VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bridge
)

# Common IPC Bridge Library
add_library(riddick_vr_bridge STATIC
    src/bridge/ipc_bridge.cpp
    src/bridge/ipc_bridge.h
)
target_include_directories(riddick_vr_bridge PUBLIC
    include
    src/bridge
)

# ---- Build Options (Follows F.E.A.R. VR multi-arch build pattern) ------------
option(RIDDICKVR_BUILD_HOST  "Build x64 OpenXR companion host (riddickvr-host.exe)" ON)
option(RIDDICKVR_BUILD_PROXY "Build x86 opengl32.dll proxy/bridge"                  ON)

# Architecture-specific targets
if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND RIDDICKVR_BUILD_PROXY)
    message(STATUS "Configuring 32-bit Hook target: opengl32.dll...")
    
    add_library(opengl32 SHARED
        src/hook/dllmain.cpp
        src/hook/opengl_proxy.cpp
        src/hook/opengl_proxy.h
        src/hook/opengl_forwarders.c
        src/hook/opengl_forwarders.h
        src/hook/iat_hook.cpp
        src/hook/iat_hook.h
        src/hook/stereo_renderer.cpp
        src/hook/stereo_renderer.h
        src/hook/camera_hook.cpp
        src/hook/camera_hook.h
        src/hook/logging.cpp
        src/hook/logging.h
        src/hook/opengl32.def
    )
    
    target_link_libraries(opengl32 PRIVATE
        riddick_vr_bridge
        d3d11
        dxgi
    )
    
    set_target_properties(opengl32 PROPERTIES
        OUTPUT_NAME "opengl32"
        PREFIX ""
    )

elseif(CMAKE_SIZEOF_VOID_P EQUAL 8 AND RIDDICKVR_BUILD_HOST)
    message(STATUS "Configuring 64-bit Companion Host target: riddickvr-host.exe...")
    
    add_executable(riddickvr-host
        src/host/main.cpp
        src/host/openxr_manager.cpp
        src/host/openxr_manager.h
    )
    
    target_link_libraries(riddickvr-host PRIVATE
        riddick_vr_bridge
        d3d11
        dxgi
    )
endif()
