cmake_minimum_required(VERSION 3.16...3.25)

project(Alert-Twitch-Fix VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(libobs REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Plugin source files
set(PLUGIN_SOURCES
    src/plugin.cpp
    src/plugin.h
    src/cef_browser.cpp
    src/cef_browser.h
    src/cef_audio.cpp
    src/cef_audio.h
    src/chromium_source.cpp
    src/chromium_source.h
)

# CEF configuration
set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cef")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

# Find CEF
find_package(CEF REQUIRED)

# Add CEF binary distribution includes
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

# Create the plugin library
add_library(Alert-Twitch-Fix MODULE ${PLUGIN_SOURCES})

# Set target properties
set_target_properties(Alert-Twitch-Fix PROPERTIES
    PREFIX ""
    OUTPUT_NAME "Alert-Twitch-Fix"
)

# Include directories
target_include_directories(Alert-Twitch-Fix PRIVATE
    src/
    ${CEF_ROOT}
    ${CEF_ROOT}/include
)

# Link libraries
target_link_libraries(Alert-Twitch-Fix
    libobs
    Qt6::Core
    Qt6::Widgets
    libcef_lib
    libcef_dll_wrapper
    ${CEF_STANDARD_LIBS}
)

# Windows-specific configuration
if(WIN32)
    target_compile_definitions(Alert-Twitch-Fix PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    # Copy CEF binaries to output directory
    add_custom_command(TARGET Alert-Twitch-Fix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CEF_ROOT}/Release/libcef.dll"
        "${CEF_ROOT}/Release/chrome_elf.dll"
        "${CEF_ROOT}/Release/d3dcompiler_47.dll"
        "${CEF_ROOT}/Release/libEGL.dll"
        "${CEF_ROOT}/Release/libGLESv2.dll"
        "${CEF_ROOT}/Release/snapshot_blob.bin"
        "${CEF_ROOT}/Release/v8_context_snapshot.bin"
        "${CEF_ROOT}/Release/vk_swiftshader.dll"
        "${CEF_ROOT}/Release/vulkan-1.dll"
        $<TARGET_FILE_DIR:Alert-Twitch-Fix>
    )
    
    # Copy CEF resources
    add_custom_command(TARGET Alert-Twitch-Fix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CEF_ROOT}/Resources"
        "$<TARGET_FILE_DIR:Alert-Twitch-Fix>/Resources"
    )
endif()

# Installation
install(TARGETS Alert-Twitch-Fix
    LIBRARY DESTINATION obs-plugins/64bit
    RUNTIME DESTINATION obs-plugins/64bit
)

install(DIRECTORY "${CEF_ROOT}/Resources/"
    DESTINATION obs-plugins/64bit/Resources
)

if(WIN32)
    install(FILES
        "${CEF_ROOT}/Release/libcef.dll"
        "${CEF_ROOT}/Release/chrome_elf.dll"
        "${CEF_ROOT}/Release/d3dcompiler_47.dll"
        "${CEF_ROOT}/Release/libEGL.dll"
        "${CEF_ROOT}/Release/libGLESv2.dll"
        "${CEF_ROOT}/Release/snapshot_blob.bin"
        "${CEF_ROOT}/Release/v8_context_snapshot.bin"
        "${CEF_ROOT}/Release/vk_swiftshader.dll"
        "${CEF_ROOT}/Release/vulkan-1.dll"
        DESTINATION obs-plugins/64bit
    )
endif()