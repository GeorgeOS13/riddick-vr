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

# Architecture-specific targets
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Configuring 32-bit Hook target: opengl32.dll...")
    
    add_library(opengl32 SHARED
        src/hook/dllmain.cpp
        src/hook/opengl_proxy.cpp
        src/hook/opengl_proxy.h
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
    )
    
    set_target_properties(opengl32 PROPERTIES
        OUTPUT_NAME "opengl32"
        PREFIX ""
    )

elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Configuring 64-bit Companion Host target: RiddickVRHost.exe...")
    
    add_executable(RiddickVRHost
        src/host/main.cpp
        src/host/openxr_manager.cpp
        src/host/openxr_manager.h
    )
    
    target_link_libraries(RiddickVRHost PRIVATE
        riddick_vr_bridge
    )
endif()
