add_library(glad ${GLAD_LIBRARY_TYPE}
		 "${GLAD_SOURCE_DIR}/include/glad/glad.h")

target_include_directories(glad PUBLIC
			   "${GLAD_SOURCE_DIR}/include")
target_include_directories(glad PRIVATE
			   "${GLAD_SOURCE_DIR}/src")