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

add_subdirectory(deps/glfw)

set (SOURCES
	src/SDLProject.cpp
	deps/glad/src/glad.c
 )

include_directories(SYSTEM
	include
	deps/glm/
	deps/glad/include
)

# Copy necessary resources to build
file (COPY
	assets/container.jpg
	assets/awesomeface.png
	DESTINATION "${CMAKE_BINARY_DIR}/assets")

file (COPY
	shaders/FragmentShader.glsl
	shaders/VertexShader.glsl
	DESTINATION "${CMAKE_BINARY_DIR}/shaders")

add_executable(SDLProject ${SOURCES})
target_link_libraries(SDLProject ${OPENGL_LIBRARIES} glfw)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SDLProject PROPERTY CXX_STANDARD 20)
endif()
