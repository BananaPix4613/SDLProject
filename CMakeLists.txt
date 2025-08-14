# =========================================================================
# PixelCraft Engine - CMakeLists.txt (Windows Configuration)
# =========================================================================

cmake_minimum_required (VERSION 3.16)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Clear any cached compiler settings that might be invalid
unset(CMAKE_CXX_COMPILER CACHE)
unset(CMAKE_C_COMPILER CACHE)

# Let CMake find the compilers automatically
if(WIN32 AND MSVC)
    # Force CMake to re-detect Visual Studio compilers
    set(CMAKE_GENERATOR_TOOLSET "")
endif()

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows"
      CACHE STRING "")
endif()

# Project Configuration
project (PixelCraftEngine
    VERSION 0.1.0
    DESCRIPTION "PixelCraft 3D Isometric Game Engine"
    LANGUAGES CXX C
)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type" FORCE)
endif()

# Windows-specific settings
if(WIN32)
    # Enable Unicode support
    add_definitions(-DUNICODE -D_UNICODE)

    # Disable specific warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)

    # Set startup project for Visual Studio
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT PixelCraftEngine)

    # Use folders in Visual Studio
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Compiler-specific settings
if(MSVC)
    # MSVC specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")

    # Enable multi-processor compilation
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    # Set runtime library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC/Clang specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Build Configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

# vcpkg integration debugging
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/deps)

# Source files
file(GLOB_RECURSE PIXELCRAFT_SOURCES
    "src/*.cpp"
    "src/*.c"
    "include/*.h"
    "include/*.hpp"
)

# Group source files for Visual Studio
if(MSVC)
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${PIXELCRAFT_SOURCES})
endif()

# Create executable
add_executable(PixelCraftEngine ${PIXELCRAFT_SOURCES})

# Set target properties
set_target_properties(PixelCraftEngine PROPERTIES
    OUTPUT_NAME "PixelCraftEngine"
    DEBUG_POSTFIX "_d"
)

# Copy assets directory to output directory after build
add_custom_command(TARGET PixelCraftEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:PixelCraftEngine>/assets
    COMMENT "Copying assets directory to output directory"
)

# Link libraries
target_link_libraries(PixelCraftEngine
    PRIVATE
        OpenGL::GL
        glfw
        glm::glm
        glad::glad
        OpenSSL::Crypto
        OpenSSL::SSL
        yaml-cpp::yaml-cpp
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(PixelCraftEngine PRIVATE
        winmm
        gdi32
        user32
        kernel32
        shell32
    )
endif()

# Preprocessor definitions
target_compile_definitions(PixelCraftEngine PRIVATE
    $<$<CONFIG:Debug>:PIXELCRAFT_DEBUG=1>
    $<$<CONFIG:Debug>:PIXELCRAFT_ENABLE_LOGGING=1>
    $<$<CONFIG:Debug>:PIXELCRAFT_ENABLE_ASSERTS=1>
    $<$<CONFIG:Release>:PIXELCRAFT_RELEASE=1>
    $<$<CONFIG:Release>:PIXELCRAFT_ENABLE_LOGGING=0>
    $<$<CONFIG:Release>:PIXELCRAFT_ENABLE_ASSERTS=0>
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)

# Windows-specific definitions
if(WIN32)
    target_compile_definitions(PixelCraftEngine PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Testing support
option(PIXELCRAFT_BUILD_TESTS "Build unit tests" ON)

if(PIXELCRAFT_BUILD_TESTS)
    # Enable testing
    enable_testing()

    # Add test definitions to the main executable for now
    target_compile_definitions(PixelCraftEngine PRIVATE
        PIXELCRAFT_ENABLE_UNIT_TESTS=1
    )

    # Note: Tests are integrated into main executable via command line args
    # This allows for easy debugging and reduces build complexity
endif()

# Documentation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    option(PIXELCRAFT_BUILD_DOCS "Build documentation" OFF)

    if(PIXELCRAFT_BUILD_DOCS)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Installation (optional)
option(PIXELCRAFT_INSTALL "Enable installation" OFF)

if(PIXELCRAFT_INSTALL)
    install(TARGETS PixelCraftEngine
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )

    install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()

# Package configuration
option(PIXELCRAFT_PACKAGE "Enable packaging" OFF)

if(PIXELCRAFT_PACKAGE)
    set(CPACK_PACKAGE_NAME "PixelCraftEngine")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VENDOR "PixelCraft Team")

    if(WIN32)
        set(CPACK_GENERATOR "ZIP;NSIS")
        set(CPACK_NSIS_DISPLAY_NAME "PixelCraft Engine")
        set(CPACK_NSIS_PACKAGE_NAME "PixelCraft Engine")
    endif()

    include(CPack)
endif()

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "PixelCraft Engine - Debug Build Configuration")
    message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
    message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    if(WIN32)
        message(STATUS "  Platform: Windows")
        if(MSVC)
            message(STATUS "  Toolset: MSVC")
        else()
            message(STATUS "  Toolset: MinGW/GCC")
        endif()
    endif()

    if(PIXELCRAFT_BUILD_TESTS)
        message(STATUS "  Unit Tests: Enabled")
    endif()
endif()

# Custom targets for development
add_custom_target(run
    COMMAND $<TARGET_FILE:PixelCraftEngine>
    DEPENDS PixelCraftEngine
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running PixelCraft Engine"
)

add_custom_target(test-quick
    COMMAND $<TARGET_FILE:PixelCraftEngine> --test-quick
    DEPENDS PixelCraftEngine
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running quick tests"
)

add_custom_target(test-all
    COMMAND $<TARGET_FILE:PixelCraftEngine> --test
    DEPENDS PixelCraftEngine
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running all tests"
)
