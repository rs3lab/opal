##################### PiBench #########################

# FetchContent_Declare(
#         pibench 
#         GIT_REPOSITORY https://github.com/sfu-dis/pibench.git
#         GIT_TAG optiql
# )

FetchContent_Declare(
        pibench 
        GIT_REPOSITORY https://github.com/rs3lab/pibench
        GIT_TAG origin/asplos
)
FetchContent_GetProperties(pibench)
if (NOT pibench_POPULATED)
  FetchContent_Populate(pibench)
  include_directories(${pibench_SOURCE_DIR}/include)
  add_subdirectory(${pibench_SOURCE_DIR} ${pibench_BINARY_DIR})
endif ()

macro(add_wrapper)
  cmake_parse_arguments(WRAPPER "" "NAME" "SOURCE;DEFINITIONS;LIBRARIES;INCLUDE_DIRS" ${ARGN})
  add_library(${WRAPPER_NAME}_wrapper SHARED ${WRAPPER_SOURCE})
  target_link_libraries(${WRAPPER_NAME}_wrapper glog::glog ${WRAPPER_LIBRARIES})
  target_compile_definitions(${WRAPPER_NAME}_wrapper PUBLIC ${WRAPPER_DEFINITIONS})

  if (WRAPPER_INCLUDE_DIRS)
  target_include_directories(${WRAPPER_NAME}_wrapper PRIVATE ${WRAPPER_INCLUDE_DIRS})
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      set_target_properties(${WRAPPER_NAME}_wrapper PROPERTIES COMPILE_FLAGS "-g")
      add_compile_options(-g)
    endif()
  endif()
  
  if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_link_libraries(${WRAPPER_NAME}_wrapper ${JEMALLOC_LIBRARIES})
  endif ()
endmacro()

#list(APPEND page_sizes 256 512 1024 2048 4096 8192 16384)
#list(APPEND page_size_suffixes "" "_512" "_1K" "_2K" "_4K" "_8K" "_16K")
list(APPEND page_sizes 256)
list(APPEND page_size_suffixes "")

foreach(page_size page_size_suffix IN ZIP_LISTS page_sizes page_size_suffixes)

# B+ tree
  add_wrapper(
    NAME btreeomcs_leaf_noop${page_size_suffix}
    SOURCE btreeolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
    DEFINITIONS OMCS_LOCK BTREE_OMCS_LEAF_ONLY OMCS_OP_READ OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE BTREE_PAGE_SIZE=${page_size} #LOCK_MEASURE_TIME
    LIBRARIES numa
  )

  add_wrapper(
    NAME btreeomcs_leaf_op_read${page_size_suffix}
    SOURCE btreeolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
    DEFINITIONS OMCS_LOCK BTREE_OMCS_LEAF_ONLY OMCS_OP_READ OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE BTREE_PAGE_SIZE=${page_size} #LOCK_MEASURE_TIME
    LIBRARIES numa
  )

  add_wrapper(
    NAME btreelc_std${page_size_suffix}
    SOURCE btreeolc_wrapper.cpp
    DEFINITIONS RWLOCK STD_LOCK BTREE_RWLOCK BTREE_PAGE_SIZE=${page_size}
  )

  add_wrapper(
    NAME btreelc_stdrw${page_size_suffix}
    SOURCE btreeolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
    DEFINITIONS RWLOCK STDRW_LOCK BTREE_RWLOCK BTREE_PAGE_SIZE=${page_size}
  )

  add_wrapper(
    NAME btreeofp_numa${page_size_suffix}
    SOURCE btreeolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
    DEFINITIONS OFP_LOCK BTREE_OFP OP_READ OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE BTREE_PAGE_SIZE=${page_size} #LOCK_MEASURE_TIME
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../common/ ${CMAKE_CURRENT_SOURCE_DIR}/../wrappers/
    LIBRARIES numa
  )

  add_wrapper(
    NAME btreeofp_noop_numa${page_size_suffix}
    SOURCE btreeolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
    DEFINITIONS OFP_LOCK BTREE_OFP OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE BTREE_PAGE_SIZE=${page_size} #LOCK_MEASURE_TIME
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../common/ ${CMAKE_CURRENT_SOURCE_DIR}/../wrappers/
    LIBRARIES numa
  )

endforeach()


# ART

add_wrapper(
  NAME artomcs_noop
  SOURCE artolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
  DEFINITIONS OMCS_LOCK IS_CONTEXTFUL OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE #LOCK_MEASURE_TIME
  LIBRARIES artomcs_offset tbb numa
)

add_wrapper(
  NAME artomcs_op_read
  SOURCE artolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
  DEFINITIONS OMCS_LOCK IS_CONTEXTFUL OMCS_OP_READ OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE #LOCK_MEASURE_TIME
  LIBRARIES artomcs_op_read tbb numa
)

 add_wrapper(
   NAME artlc_stdrw
   SOURCE artolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
   DEFINITIONS RWLOCK STDRW_LOCK
   LIBRARIES artlc_stdrw tbb numa
 )

add_wrapper(
  NAME artofp_numa
  SOURCE artolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
  DEFINITIONS OFP_LOCK_OP ART_OFP_LOCK OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE OP_READ _GNU_SOURCE #LOCK_MEASURE_TIME
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../common/ ${CMAKE_CURRENT_SOURCE_DIR}/../wrappers/
  LIBRARIES artofp_numa tbb numa
)

add_wrapper(
  NAME artofp_noop_numa
  SOURCE artolc_wrapper.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fds/stats.c
  DEFINITIONS OFP_LOCK_OP ART_OFP_LOCK OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE _GNU_SOURCE #LOCK_MEASURE_TIME
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../common/ ${CMAKE_CURRENT_SOURCE_DIR}/../wrappers/
  LIBRARIES artofp_noop_numa tbb numa
)
