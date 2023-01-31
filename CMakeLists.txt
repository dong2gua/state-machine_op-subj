# project specific information
cmake_minimum_required(VERSION 3.19)

project(state-machine-example VERSION 0.1 LANGUAGES C CXX)

set(NOT_USING_HEAP_DATA FALSE CACHE BOOL "Not using head to allocate the signal data.")

set(EXECUTABLE_OUTPUT_PATH bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRCS
	"*.h"
	"*.cpp"
	"sm/*.h"
	"sm/*.cpp"
)

add_executable(${PROJECT_NAME} ${SRCS})

if(NOT_USING_HEAP_DATA)
	message("Not using heap to allocate the signal data.")
	target_compile_definitions(${PROJECT_NAME} PRIVATE EXTERNAL_EVENT_NO_HEAP_DATA)
endif()

string(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE")
set_target_properties(${PROJECT_NAME} PROPERTIES
	COMPILE_FLAGS "-pthread"
	LINK_FLAGS "-pthread")

target_include_directories(${PROJECT_NAME}
	PUBLIC
	"${CMAKE_SOURCE_DIR}/"
	"${CMAKE_SOURCE_DIR}/sm/"
)
