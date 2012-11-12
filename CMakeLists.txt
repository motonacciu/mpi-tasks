cmake_minimum_required(VERSION 2.8)

project(mpits)

add_definitions(-std=c++11 -g)

find_package(Boost REQUIRED serialization context)
find_package(MPI REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

SET(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})

set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

include_directories(${MPI_INCLUDE_PATH})
include_directories(${Boost_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/include )

file(GLOB_RECURSE sources src/*.cpp)

add_library(mpits SHARED ${sources})

add_library(kernels SHARED kernels.cpp)

add_executable(main main.cpp)
target_link_libraries(main mpits ${MPI_LIBRARIES} ${Boost_LIBRARIES})


## Enable testing with Gtest
enable_testing()

include_directories(${GTEST_INCLUDE_DIRS})

macro(add_unit_test case_name)

	# Link against the gtest library 
	target_link_libraries(${case_name} ${GTEST_LIBRARY})
	target_link_libraries(${case_name} ${GTEST_MAIN_LIBRARY})

	add_test(NAME ${case_name} 
			COMMAND valgrind
				--leak-check=full
				--show-reachable=no
				--track-fds=yes
				--error-exitcode=1
				--track-origins=yes
				#--log-file=${CMAKE_CURRENT_BINARY_DIR}/valgrind.log.${case_name}
				${CMAKE_CURRENT_BINARY_DIR}/${case_name}
				WORKING_DIRECTORY
				${CMAKE_CURRENT_BINARY_DIR}
		)
endmacro(add_unit_test)

set ( ut_prefix  ut_mpits_ )
file(GLOB_RECURSE test_cases test/*.cc)
foreach ( case_file ${test_cases})
	get_filename_component( case_name ${case_file} NAME_WE )
	
	set ( case_name ${ut_prefix}${case_name} )
	add_executable(${case_name} ${case_file})
	target_link_libraries(${case_name} mpits ${Threads} ${MPI_LIBRARIES} ${Boost_LIBRARIES})
	
	add_unit_test(${case_name})

endforeach(case_file)

