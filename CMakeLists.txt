# CMakeList.txt : CMake project for GameProject, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("SDLProject")

find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")

add_subdirectory(deps/glfw)
add_subdirectory(deps/assimp)

# FMOD Setup
set(FMOD_ROOT_DIR "${CMAKE_SOURCE_DIR}/deps/fmod" CACHE PATH "FMOD root directory")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FMOD_LIBRARY_PATH "${FMOD_ROOT_DIR}/lib/x64")
else()
    set(FMOD_LIBRARY_PATH "${FMOD_ROOT_DIR}/lib/x86")
endif()
set(FMOD_LIBRARIES "${FMOD_LIBRARY_PATH}/fmod_vc.lib")
set(FMOD_DLL_DIR "${FMOD_LIBRARY_PATH}")

file (GLOB SOURCES
	src/*.cpp
	src/components/*.cpp
	src/editor/*.cpp
	deps/glad/src/glad.c
	deps/imgui/*.cpp
	deps/imgui/backends/imgui_impl_opengl3.cpp
	deps/imgui/backends/imgui_impl_glfw.cpp
 )

include_directories(SYSTEM
	include
	deps/glm
	deps/glad/include
	deps/gl3w/include
	deps/assimp/include
	deps/imgui
	deps/imgui/backends/imgui_impl_opengl3.h
	deps/imgui/backends/imgui_impl_opengl3_loader.h
	deps/imgui/backends/imgui_impl_glfw.h
	deps/rapidjson/include
	deps/fmod/api/core/inc
)

# Copy necessary resources to build
file (COPY assets DESTINATION "${CMAKE_BINARY_DIR}")
file (COPY shaders DESTINATION "${CMAKE_BINARY_DIR}")

# Copy FMOD DLLs to build directory
file(GLOB FMOD_DLLS "${FMOD_DLL_DIR}/*.dll")
foreach(FMOD_DLL ${FMOD_DLLS})
    file(COPY ${FMOD_DLL} DESTINATION "${CMAKE_BINARY_DIR}")
endforeach()

add_executable(SDLProject ${SOURCES})
target_link_libraries(SDLProject ${OPENGL_LIBRARIES} glfw assimp ${FMOD_LIBRARIES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SDLProject PROPERTY CXX_STANDARD 20)
endif()
