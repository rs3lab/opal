cmake_minimum_required(VERSION 3.11)
project(omcs)
include(FetchContent)

# No in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
  message("-- No build mode chosen - using Debug by default")
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors -fPIC -march=native -std=c++20") #-pedantic
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfatal-errors -fPIC -march=native -std=c++20") #-pedantic
set(CMAKE_ENABLE_COMPILE_COMMANDS "ON")


# local dependencies
include_directories("~/local/include")
link_directories("~/local/lib")
set(glog_DIR /home/vishal/project/glog-0.7.1/build/)
#target_link_libraries(target_name glog)

find_package (glog 0.7.1 REQUIRED)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  message("-- Debug mode, sanitizers enabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
elseif (${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
  string(REPLACE "-O2" "-O3 -Ofast" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  string(REPLACE "-O2" "-O3 -Ofast" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  message("-- Release mode, all optimization enabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Ofast -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Ofast -g")
  find_package(PkgConfig REQUIRED)
  set(CMAKE_PREFIX_PATH $ENV{HOME}/local)
  
  #pkg_check_modules(JEMALLOC jemalloc)
  #find_package (TBB REQUIRED)
  pkg_search_module(JEMALLOC REQUIRED jemalloc)
  include_directories(${JEMALLOC_INCLUDE_DIRS})
  message(STATUS "found jemalloc in ${JEMALLOC_INCLUDE_DIRS}")
  message(STATUS "found jemalloc libs in ${JEMALLOC_LIBRARIES}")
endif ()

enable_testing()

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/common)

list(APPEND omcs_impls OMCS_LOCK)
list(APPEND omcs_impl_suffixes "")

add_subdirectory(indexes)
add_subdirectory(common)
add_subdirectory(latches)
add_subdirectory(wrappers)
add_subdirectory(fds)
