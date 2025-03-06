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

# Add source to this project's executable.
add_executable (SDLProject
	src/SDLProject.cpp
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SDLProject PROPERTY CXX_STANDARD 20)
endif()

# Define paths to the pre-built libraries
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/deps/SDL")
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/deps/GLFW")
set(GLEW_DIR "${CMAKE_SOURCE_DIR}/deps/GLEW")

# Include directories
include_directories("${CMAKE_SOURCE_DIR}/include" "${SDL3_DIR}/include" "${GLFW_DIR}/include" "${GLEW_DIR}/include")

# Link libraries
target_link_libraries(SDLProject "${GLEW_DIR}/lib/Release/x64/glew32.lib" "${SDL3_DIR}/lib/x64/SDL3.lib")

# TODO: Add tests and install targets if needed.
