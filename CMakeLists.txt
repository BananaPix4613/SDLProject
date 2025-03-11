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

set(GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only")
set(GLAD_GL "")

add_subdirectory(deps/glfw)
add_subdirectory(DEPS/SDL EXCLUDE_FROM_ALL)

option(SDLPROJECT-USE-GLFW-GLAD "Use GLAD from GLFW" ON)

if (SDLPROJECT-USE-GLFW-GLAD)
    include_directories("${GLFW_SOURCE_DIR}/deps")
    set(GLAD_GL "${GLFW_SOURCE_DIR}/deps/glad/gl.h")
endif()

set (SOURCES
	src/SDLProject.cpp
)

add_executable(SDLProject ${SOURCES} ${GLAD_GL})
target_link_libraries(SDLProject ${OPENGL_LIBRARIES} glfw SDL3::SDL3)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SDLProject PROPERTY CXX_STANDARD 20)
endif()
