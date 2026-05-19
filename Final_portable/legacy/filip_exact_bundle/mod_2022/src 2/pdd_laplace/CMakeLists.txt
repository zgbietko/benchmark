# Uses:
#
# PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS
# PDD_LAPLACE_EXTERNAL_LIBRARIES
#
# CREATE_MOD_FEM_LAPLACE_PRISM_STD
# CREATE_MOD_FEM_LAPLACE_PRISM_STD_QUAD
# CREATE_MOD_FEM_LAPLACE_HYBRID_STD
# CREATE_MOD_FEM_LAPLACE_HYBRID_STD_QUAD
# CREATE_MOD_FEM_LAPLACE_PRISM_DG
# CREATE_MOD_FEM_LAPLACE_HYBRID_DG

# -- Add target to global list --
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_PRISM_STD")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_PRISM_STD_QUAD")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_HYBRID_STD")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_HYBRID_STD_QUAD")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_PRISM2D_STD")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_PRISM2D_STD_QUAD") 
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_PRISM_DG")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_HYBRID_DG")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_LAPLACE_PRISM2D_DG")
set(MOD_FEM_EXEC_LIST "${MOD_FEM_EXEC_LIST}" PARENT_SCOPE)

# -- Exec files --
set(pdd_laplace_srcdir "${CMAKE_SOURCE_DIR}/pdd_laplace")
set(PDD_LAPLACE_PRISM_STD_EXE "MOD_FEM_laplace_prism_std")
set(PDD_LAPLACE_PRISM_STD_QUAD_EXE "MOD_FEM_laplace_prism_std_quad")
set(PDD_LAPLACE_PRISM_DG_EXE "MOD_FEM_laplace_prism_dg")
set(PDD_LAPLACE_HYBRID_STD_EXE "MOD_FEM_laplace_hybrid_std")
set(PDD_LAPLACE_HYBRID_STD_QUAD_EXE "MOD_FEM_laplace_hybrid_std_quad")
set(PDD_LAPLACE_HYBRID_DG_EXE "MOD_FEM_laplace_hybrid_dg")
set(PDD_LAPLACE_PRISM2D_STD_EXE "MOD_FEM_laplace_prism2d_std")
set(PDD_LAPLACE_PRISM2D_STD_QUAD_EXE "MOD_FEM_laplace_prism2d_std_quad")
set(PDD_LAPLACE_PRISM2D_DG_EXE "MOD_FEM_laplace_prism2d_dg")

# -- Include directories --
list(APPEND PDD_LAPLACE_STD_INCLUDE_DIRS
  ${pdd_laplace_srcdir}/approx_std
  ${pdd_laplace_srcdir}
)
# -- Include directories --
list(APPEND PDD_LAPLACE_DG_INCLUDE_DIRS
  ${pdd_laplace_srcdir}/approx_dg
  ${pdd_laplace_srcdir}
)


if(PDD_LAPLACE_STD_INCLUDE_DIRS)
  set(PDD_LAPLACE_STD_INCLUDE_DIRS ${PDD_LAPLACE_STD_INCLUDE_DIRS} PARENT_SCOPE)
endif()

if(PDD_LAPLACE_DG_INCLUDE_DIRS)
  set(PDD_LAPLACE_DG_INCLUDE_DIRS ${PDD_LAPLACE_DG_INCLUDE_DIRS} PARENT_SCOPE)
endif()


if(CREATE_MOD_FEM_LAPLACE_PRISM_STD)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_STD_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_STD_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_PRISM_STD_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_PRISM_STD_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_prism apl_std_lin utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_STD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_STD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_STD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

#message ("VARIABLES FOR LAPLACE_PRISM_STD")
#message (${SOURCE_FILES})
unset(SOURCE_FILES)
#message (${PDD_LAPLACE_PRISM_STD_EXE})
#message (${PDD_LAPLACE_EXTERNAL_LIBRARIES})
#message (${MODFEM_SOLVER_MODULE})
#message (${MODFEM_MPI_LIBS_STD})

if(CREATE_MOD_FEM_LAPLACE_PRISM_STD_QUAD)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_STD_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_STD_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_PRISM_STD_QUAD_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_PRISM_STD_QUAD_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_prism apl_std_quad utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_STD_QUAD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_STD_QUAD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_STD_QAUD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

unset(SOURCE_FILES)

if(CREATE_MOD_FEM_LAPLACE_PRISM_DG)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_DG_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_DG_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_PRISM_DG_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_PRISM_DG_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_prism apl_dg_prism utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_DG})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_DG_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_DG_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM_DG_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

unset(SOURCE_FILES)

if(CREATE_MOD_FEM_LAPLACE_HYBRID_STD)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_STD_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_STD_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_HYBRID_STD_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_HYBRID_STD_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_t4_prism apl_std_lin utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_STD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_STD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_STD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

unset(SOURCE_FILES)

if(CREATE_MOD_FEM_LAPLACE_HYBRID_STD_QUAD)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_STD_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_STD_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_HYBRID_STD_QUAD_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_HYBRID_STD_QUAD_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_t4_prism apl_std_quad utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_STD_QUAD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_STD_QUAD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_STD_QUAD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

unset(SOURCE_FILES)

if(CREATE_MOD_FEM_LAPLACE_HYBRID_DG)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_DG_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_DG_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_HYBRID_DG_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_HYBRID_DG_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_t4_prism apl_dg_prism utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_DG})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_DG_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_DG_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_HYBRID_DG_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

unset(SOURCE_FILES)

if(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_STD_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_STD_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_PRISM2D_STD_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_PRISM2D_STD_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_prism_2D apl_std_lin utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_STD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_STD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_STD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

unset(SOURCE_FILES)

if(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD_QUAD)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_STD_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_STD_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_PRISM2D_STD_QUAD_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_PRISM2D_STD_QUAD_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_prism_2D apl_std_quad utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_STD_QUAD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_STD_QUAD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_STD_QUAD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

unset(SOURCE_FILES)

if(CREATE_MOD_FEM_LAPLACE_PRISM2D_DG)
  # -- Source files list --
  collect_src_from_include_dirs(PDD_LAPLACE_DG_INCLUDE_DIRS)
  # -- Target --
  include_directories(${PDD_LAPLACE_DG_INCLUDE_DIRS} ${PDD_LAPLACE_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

  add_executable(${PDD_LAPLACE_PRISM2D_DG_EXE} ${SOURCE_FILES})
  target_link_libraries(${PDD_LAPLACE_PRISM2D_DG_EXE} ${PDD_LAPLACE_EXTERNAL_LIBRARIES} mml_prism_2D apl_dg_prism utl_system utl_util ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_DG})
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_DG_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_DG_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
  SET_TARGET_PROPERTIES(${PDD_LAPLACE_PRISM2D_DG_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
endif()

enable_testing ()








foreach(folder ${MKL_INCLUDE_DIRS})
    set(KKK "${KKK}${folder}:")
endforeach()

foreach(folder ${MKL_LIBRARY_DIRS})
    set(KKK "${KKK}${folder}:")
endforeach()


set(KKK "${KKK}.")

#message(${KKK})

set(CTEST_BUILD_DIR $ENV{PWD})
set(CTEST_DIR ${CMAKE_SOURCE_DIR}/../ctest)

#message(${ENV_PLATFORM})

#STRING(REGEX REPLACE " " ":" KKK ${MKL_INCLUDE_DIRS})

message(${KKK})
#add_test (laplace_test ${KKK})

add_test (laplace_Make ${CTEST_DIR}/build.sh ${CTEST_BUILD_DIR}/pdd_laplace/ ${ENV_PLATFORM} ${KKK})

set (passRegex "^Test passed")
set (failRegex "Fail")

if(MODFEM_MPI MATCHES "nompi")

message("NO MPI")
message(${MODFEM_MPI})

# NO MPI

# test Laplace_prism_dg
  add_test (laplace_prism_dg_Run ${CTEST_DIR}/laplace_dg/lap_run.py ${CTEST_DIR}/laplace_dg/ ${CTEST_BUILD_DIR} ${KKK})
  add_test (laplace_prism_dg_Diff ${CTEST_DIR}/laplace_dg/lap_field.py ${CTEST_DIR}/laplace_dg/)
  add_test (laplace_prism_dg_L2 ${CTEST_DIR}/laplace_dg/lap_L2.py ${CTEST_DIR}/laplace_dg/)
  add_test (laplace_prism_dg_H1 ${CTEST_DIR}/laplace_dg/lap_H1.py ${CTEST_DIR}/laplace_dg/)

  set_tests_properties(laplace_prism_dg_Run PROPERTIES DEPENDS laplace_Make)
  set_tests_properties(laplace_prism_dg_Diff PROPERTIES DEPENDS laplace_prism_dg_Run)
  set_tests_properties(laplace_prism_dg_L2 PROPERTIES DEPENDS laplace_prism_dg_Run)
  set_tests_properties(laplace_prism_dg_H1 PROPERTIES DEPENDS laplace_prism_dg_Run)

  set_property(TEST laplace_prism_dg_Run PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_dg_Run PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST laplace_prism_dg_L2 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_dg_L2 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST laplace_prism_dg_H1 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_dg_H1 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST laplace_prism_dg_Diff PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_dg_Diff PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")



# test Laplace_prism_std
  add_test (laplace_prism_std_Run ${CTEST_DIR}/laplace_std/lap_run.py ${CTEST_DIR}/laplace_std/ ${CTEST_BUILD_DIR} ${KKK})
  add_test (laplace_prism_std_Diff ${CTEST_DIR}/laplace_std/lap_field.py ${CTEST_DIR}/laplace_std/)
  add_test (laplace_prism_std_L2 ${CTEST_DIR}/laplace_std/lap_L2.py ${CTEST_DIR}/laplace_std/)
  add_test (laplace_prism_std_H1 ${CTEST_DIR}/laplace_std/lap_H1.py ${CTEST_DIR}/laplace_std/)
  add_test (laplace_prism_std_ZZ ${CTEST_DIR}/laplace_std/lap_ZZ.py ${CTEST_DIR}/laplace_std/)
  add_test (laplace_prism_std_ref ${CTEST_DIR}/laplace_std/lap_ref.py ${CTEST_DIR}/laplace_std/ ${CTEST_BUILD_DIR} ${KKK})

  set_tests_properties(laplace_prism_std_ref PROPERTIES DEPENDS laplace_Make)
  set_tests_properties(laplace_prism_std_Diff PROPERTIES DEPENDS laplace_prism_std_Run)
  set_tests_properties(laplace_prism_std_L2 PROPERTIES DEPENDS laplace_prism_std_Run)
  set_tests_properties(laplace_prism_std_H1 PROPERTIES DEPENDS laplace_prism_std_Run)
  set_tests_properties(laplace_prism_std_ZZ PROPERTIES DEPENDS laplace_prism_std_Run)
  set_tests_properties(laplace_prism_std_Run PROPERTIES DEPENDS laplace_prism_std_ref)

  set_property(TEST laplace_prism_std_Run PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_std_Run PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST laplace_prism_std_L2 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_std_L2 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST laplace_prism_std_H1 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_std_H1 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST laplace_prism_std_ref PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_std_ref PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST laplace_prism_std_Diff PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST laplace_prism_std_Diff PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")

else()

#MPI

message("MPI")
message(${MODFEM_MPI})


# test Laplace_prism_dg
  add_test (mpi_laplace_prism_dg_Run ${CTEST_DIR}/mpi_laplace_dg_in_box/lap_run.py ${CTEST_DIR}/mpi_laplace_dg_in_box/ ${CTEST_BUILD_DIR} ${KKK})
  add_test (mpi_laplace_prism_dg_L2 ${CTEST_DIR}/mpi_laplace_dg_in_box/lap_L2.py ${CTEST_DIR}/mpi_laplace_dg_in_box/)
  add_test (mpi_laplace_prism_dg_H1 ${CTEST_DIR}/mpi_laplace_dg_in_box/lap_H1.py ${CTEST_DIR}/mpi_laplace_dg_in_box/)
  

  set_tests_properties(mpi_laplace_prism_dg_Run PROPERTIES DEPENDS laplace_Make)
  set_tests_properties(mpi_laplace_prism_dg_L2 PROPERTIES DEPENDS mpi_laplace_prism_dg_Run)
  set_tests_properties(mpi_laplace_prism_dg_H1 PROPERTIES DEPENDS mpi_laplace_prism_dg_Run)
 
  set_property(TEST mpi_laplace_prism_dg_Run PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST mpi_laplace_prism_dg_Run PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST mpi_laplace_prism_dg_L2 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST mpi_laplace_prism_dg_L2 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST mpi_laplace_prism_dg_H1 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST mpi_laplace_prism_dg_H1 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")



# test Laplace_prism_std
  add_test (mpi_laplace_prism_std_Run ${CTEST_DIR}/mpi_laplace_std_in_box/lap_run.py ${CTEST_DIR}/mpi_laplace_std_in_box/ ${CTEST_BUILD_DIR} ${KKK})
  add_test (mpi_laplace_prism_std_L2 ${CTEST_DIR}/mpi_laplace_std_in_box/lap_L2.py ${CTEST_DIR}/mpi_laplace_std_in_box/)
  add_test (mpi_laplace_prism_std_H1 ${CTEST_DIR}/mpi_laplace_std_in_box/lap_H1.py ${CTEST_DIR}/mpi_laplace_std_in_box/)
  add_test (mpi_laplace_prism_std_ZZ ${CTEST_DIR}/mpi_laplace_std_in_box/lap_ZZ.py ${CTEST_DIR}/mpi_laplace_std_in_box/)

  set_tests_properties(mpi_laplace_prism_std_Run PROPERTIES DEPENDS laplace_Make)
  set_tests_properties(mpi_laplace_prism_std_L2 PROPERTIES DEPENDS mpi_laplace_prism_std_Run)
  set_tests_properties(mpi_laplace_prism_std_H1 PROPERTIES DEPENDS mpi_laplace_prism_std_Run)
  set_tests_properties(mpi_laplace_prism_std_ZZ PROPERTIES DEPENDS mpi_laplace_prism_std_Run)
 
  set_property(TEST mpi_laplace_prism_std_Run PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST mpi_laplace_prism_std_Run PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST mpi_laplace_prism_std_L2 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST mpi_laplace_prism_std_L2 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
  set_property(TEST mpi_laplace_prism_std_H1 PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
  set_property(TEST mpi_laplace_prism_std_H1 PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")



endif()

