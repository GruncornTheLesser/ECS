cmake_minimum_required(VERSION 3.20)

project(ECS VERSION 1.0 LANGUAGES CXX)

add_library(ecs_lib INTERFACE)
target_compile_features(ecs_lib INTERFACE cxx_std_23)
target_include_directories(ecs_lib INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
	<INSTALL_INTERFACE:include>
)

if (EXISTS "${CMAKE_SOURCE_DIR}/main.cpp") 
	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

	add_executable(ecs ${CMAKE_SOURCE_DIR}/main.cpp)
	target_compile_options(ecs PRIVATE -ftemplate-depth=32)

	target_link_libraries(ecs PRIVATE ecs_lib)
endif()