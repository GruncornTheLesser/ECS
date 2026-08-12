cmake_minimum_required(VERSION 3.20)

execute_process(
	COMMAND git describe --tags --abbrev=0
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE ECS_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET
)

math(EXPR ECS_VERSION_VARIANT "1")
math(EXPR ECS_VERSION_MAJOR   "0")
math(EXPR ECS_VERSION_MINOR   "0")
math(EXPR ECS_VERSION_PATCH   "0")

if(ECS_VERSION MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    math(EXPR ECS_VERSION_MAJOR   "${CMAKE_MATCH_1}")
    math(EXPR ECS_VERSION_MINOR   "${CMAKE_MATCH_2}")
    math(EXPR ECS_VERSION_PATCH   "${CMAKE_MATCH_3}")
endif()

set(ECS_VERSION "${ECS_VERSION_VARIANT}.${ECS_VERSION_MAJOR}.${ECS_VERSION_MINOR}.${ECS_VERSION_PATCH}")

message(STATUS "ECS Version: ${ECS_VERSION}")

project(ecs VERSION ${ECS_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ecs INTERFACE)
target_compile_features(ecs INTERFACE cxx_std_23)

include(FetchContent)
FetchContent_Declare(util GIT_REPOSITORY https://github.com/GruncornTheLesser/tuple_util.git GIT_TAG master)
FetchContent_MakeAvailable(util)

target_compile_options(ecs INTERFACE
	"-fdebug-prefix-map=${CMAKE_SOURCE_DIR}/="
	"-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/="
)

target_include_directories(ecs INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

target_link_libraries(ecs INTERFACE util)

file(GLOB_RECURSE HEADERS "inc/*.h")
target_sources(ecs INTERFACE ${HEADERS})