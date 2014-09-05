cmake_minimum_required (VERSION 2.6)
project (Tutorial)
# The version number.
#set (Tutorial_VERSION_MAJOR 1)
#set (Tutorial_VERSION_MINOR 0)
 
# configure a header file to pass some of the CMake settings
# to the source code
#configure_file (
#  "${PROJECT_SOURCE_DIR}/TutorialConfig.h.in"
#  "${PROJECT_BINARY_DIR}/TutorialConfig.h"
#  )
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
#include_directories("${PROJECT_BINARY_DIR}")
 
# add the executable

set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -g")

find_package (Threads REQUIRED)
#find_package (Ev)
#find_package (Cocaine-framework)
#find_package (Rt)
#find_package (Elliptics_cpp)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 


find_package(Boost 1.45.0 COMPONENTS thread system) 

if (Boost_FOUND)
	add_executable(worker worker.cpp)
	target_link_libraries(worker ${Boost_LIBRARIES} pthread ev rt elliptics_cpp cocaine-framework msgpack)

# -lboost_program_options -lboost_thread-mt worker.cpp -o worker -lboost_system -lmsgpack -lev -lcocaine-framework -lpthread -lrt -lelliptics_cpp

endif()