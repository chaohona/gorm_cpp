cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED GAMESH_TOOLKIT)
    include(${CMAKE_CURRENT_SOURCE_DIR}/vendor/gamesh/toolkit/gamesh.cmake)
endif()

find_package(protobuf CONFIG REQUIRED)
set(projName gorm-client)
project(${projName} CXX)

gamesh_module(${projName} DYNAMIC)

target_include_directories(
    ${projName}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_dependencies(${projName} gamesh.galog  gamesh.framework gamesh.coroutine)
target_link_libraries(${projName} gamesh.galog  gamesh.framework gamesh.coroutine protobuf::libprotobuf)
