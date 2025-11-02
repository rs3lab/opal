add_definitions(-DGLOG_USE_GLOG_EXPORT)

#add_executable(btreeolc_tests_tcl tests.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../../latches/komb.c)
#target_compile_definitions(btreeolc_tests_tcl PUBLIC TC_LOCK BTREE_RWLOCK RWLOCK BTREE_PAGE_SIZE=256)
#target_link_libraries(btreeolc_tests_tcl tbb glog::glog numa)
#
#add_executable(btreeolc_tests_tclrw tests.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../../latches/kombrw.c)
#target_compile_definitions(btreeolc_tests_tclrw PUBLIC TC_LOCK_RW BTREE_RWLOCK RWLOCK BTREE_PAGE_SIZE=256)
#target_link_libraries(btreeolc_tests_tclrw tbb glog::glog numa)
#
#add_executable(btreeolc_tests_stdrw tests.cpp)
#target_compile_definitions(btreeolc_tests_stdrw PUBLIC STDRW_LOCK BTREE_RWLOCK RWLOCK BTREE_PAGE_SIZE=256)
#target_link_libraries(btreeolc_tests_stdrw tbb glog::glog numa)
#
#add_executable(btreeolc_tests_leaf_op_read tests.cpp)
#target_compile_definitions(btreeolc_tests_leaf_op_read PUBLIC OMCS_LOCK BTREE_OMCS_LEAF_ONLY OMCS_OP_READ OMCS_OFFSET OMCS_OFFSET_NUMA_QNODE BTREE_PAGE_SIZE=256)
#target_link_libraries(btreeolc_tests_leaf_op_read tbb glog::glog numa)
