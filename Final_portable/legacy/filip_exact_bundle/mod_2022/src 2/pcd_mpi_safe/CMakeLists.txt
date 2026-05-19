add_library(pcl_mpi_safe pcs_mpi_safe_intf.cpp ../include/pch_intf.h)

enable_testing()

add_executable(MOD_FEM_test_pcl "pcs_test.cpp")
target_link_libraries(MOD_FEM_test_pcl utl_log pcl_mpi_safe)


add_test(NAME MOD_FEM_test_pcl COMMAND MOD_FEM_test_pcl )
