set(mmpd_adapter_srcdir "${CMAKE_SOURCE_DIR}/mmpd_adapter")

# -- Include directories --
list(APPEND mmpd_ADAPTER_INCLUDE_DIRS
  ${mmpd_adapter_srcdir}
  ${mmpd_adapter_srcdir}/ddd_metis_adapter
)

if(mmpd_ADAPTER_INCLUDE_DIRS)
  set(mmpd__INCLUDE_DIRS ${mmpd_ADAPTER_INCLUDE_DIRS} PARENT_SCOPE)
endif()

list(APPEND mmpd_ADAPTER_EXTERNAL_LIBRARIES "-Wl,--start-group ${METIS_LIBRARY} ${PARMETIS_LIBRARY} -Wl,--end-group")

# -- Source files list --
collect_src_from_include_dirs(mmpd_ADAPTER_INCLUDE_DIRS)

# -- Target --
include_directories(${mmpd_ADAPTER_INCLUDE_DIRS} ${mmpd_ADAPTER_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

add_library(mmpl_adapter ${SOURCE_FILES})
target_link_libraries(mmpl_adapter ${mmpd_ADAPTER_EXTERNAL_LIBRARIES})
SET_TARGET_PROPERTIES(mmpl_adapter PROPERTIES COMPILE_DEFINITIONS "${MODFEM_MPI_CODE_DEFINE}")
SET_TARGET_PROPERTIES(mmpl_adapter PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(mmpl_adapter PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

enable_testing()

add_executable(MOD_FEM_test_mmpl_adapter "mmps_adapter_test.cpp")
target_link_libraries(MOD_FEM_test_mmpl_adapter utl_log pcl_mpi_safe mmpl_adapter)


add_test(NAME MOD_FEM_test_mmpl_adapter COMMAND MOD_FEM_test_mmpl_adapter )
