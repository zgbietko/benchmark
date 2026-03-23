set(utd_util_srcdir "${CMAKE_SOURCE_DIR}/utd_util")

# -- Include directories --
list(APPEND UTD_UTIL_INCLUDE_DIRS
  ${utd_util_srcdir}
)

if(UTD_UTIL_INCLUDE_DIRS)
  set(UTD_UTIL_INCLUDE_DIRS ${UTD_UTIL_INCLUDE_DIRS} PARENT_SCOPE)
endif()

# -- Source files list --
#collect_src_from_include_dirs(UTD_UTIL_INCLUDE_DIRS)



# -- Target --
include_directories( ${UTD_UTIL_INCLUDE_DIRS} ${UTD_UTIL_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

add_library(utl_log "uts_log")
SET_TARGET_PROPERTIES(utl_log PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(utl_log PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

add_library(utl_mat "uts_mat")
target_link_libraries(utl_mat utl_log ${UTD_UTIL_EXTERNAL_LIBRARIES})
SET_TARGET_PROPERTIES(utl_mat PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(utl_mat PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(utl_mat PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

add_library(utl_io_compression "uts_io_compression" "miniz")
SET_TARGET_PROPERTIES(utl_io_compression PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(utl_io_compression PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(utl_io_compression PROPERTIES LINK_FLAGS "${LINK_FLAGS}")


set(SOURCE_FILES
"uts_adapt"
"uts_bc"
"uts_coloring"
"uts_io_intf"
"uts_ls_intf"
"uts_mesh"
"uts_util"
"uts_write_paraview")

add_library(utl_util ${SOURCE_FILES} )
target_link_libraries(utl_util utl_mat utl_mesh_remesh utl_io_compression ${UTD_UTIL_EXTERNAL_LIBRARIES})
SET_TARGET_PROPERTIES(utl_util PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(utl_util PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(utl_util PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

enable_testing()

add_executable(MOD_FEM_test_util "uts_test.cpp")
target_link_libraries(MOD_FEM_test_util utl_mat utl_util mml_t4_prism utl_system  utl_io_compression apl_std_lin ${UTD_UTIL_EXTERNAL_LIBRARIES})

add_test(NAME MOD_FEM_test_util COMMAND MOD_FEM_test_util )
