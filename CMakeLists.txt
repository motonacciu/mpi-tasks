cmake_minimum_required(VERSION 2.8)

project(mpits)

add_definitions(-std=c++11)

find_package(Boost)
find_package(MPI REQUIRED)

set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})

set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

include_directories(MPI_INCLUDE_PATH)

include_directories(${CMAKE_SOURCE_DIR}/include )

add_executable(mpits src/main.cpp src/comm/init.cpp)
target_link_libraries(mpits ${MPI_LIBRARIES})
