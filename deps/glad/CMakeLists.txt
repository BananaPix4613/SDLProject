cmake_minimum_required(VERSION 3.8)

project(GLAD)

add_subdirectory(src)

set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)