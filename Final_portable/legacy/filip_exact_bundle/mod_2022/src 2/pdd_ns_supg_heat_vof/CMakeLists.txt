# -- Add target to global list --
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM_STD")
list(APPEND MOD_FEM_EXEC_LIST "CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_HYBRID_STD")
set(MOD_FEM_EXEC_LIST "${MOD_FEM_EXEC_LIST}" PARENT_SCOPE)

# -- Exec files --
set(pdd_ns_supg_heat_vof_srcdir "${CMAKE_SOURCE_DIR}/pdd_ns_supg_heat_vof")
set(PDD_NS_SUPG_HEAT_VOF_PRISM_STD_EXE "MOD_FEM_ns_supg_heat_vof_prism_std")
set(PDD_NS_SUPG_HEAT_VOF_HYBRID_STD_EXE "MOD_FEM_ns_supg_heat_vof_hybrid_std")
# -- Include directories --
list(APPEND PDD_NS_SUPG_HEAT_VOF_INCLUDE_DIRS
  ${pdd_ns_supg_heat_vof_srcdir}/adaptation
  ${pdd_ns_supg_heat_vof_srcdir}/include
  ${pdd_ns_supg_heat_vof_srcdir}/input_output
  ${pdd_ns_supg_heat_vof_srcdir}/linear_solver_interface
  ${pdd_ns_supg_heat_vof_srcdir}/main
  ${pdd_ns_supg_heat_vof_srcdir}/time_integration
  ${pdd_ns_supg_heat_vof_srcdir}
  ${CMAKE_SOURCE_DIR}/pdd_ns_supg/weak_formulation
  ${CMAKE_SOURCE_DIR}/pdd_ns_supg/input_output
  ${CMAKE_SOURCE_DIR}/pdd_heat/weak_formulation
  ${CMAKE_SOURCE_DIR}/pdd_heat/input_output
)
if(PDD_NS_SUPG_HEAT_VOF_INCLUDE_DIRS)
  set(PDD_NS_SUPG_HEAT_VOF_INCLUDE_DIRS ${PDD_NS_SUPG_HEAT_VOF_INCLUDE_DIRS} PARENT_SCOPE)
endif()
# -- Source files list --
collect_src_from_include_dirs(PDD_NS_SUPG_HEAT_VOF_INCLUDE_DIRS)
# -- Target --
include_directories(${PDD_NS_SUPG_HEAT_VOF_INCLUDE_DIRS} ${PDD_NS_SUPG_HEAT_VOF_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

if(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM_STD)
  if(NOT ${MODFEM_ACCEL} STREQUAL "opencl")
    add_executable(${PDD_NS_SUPG_HEAT_VOF_PRISM_STD_EXE} ${SOURCE_FILES})
    target_link_libraries(${PDD_NS_SUPG_HEAT_VOF_PRISM_STD_EXE} ${PDD_NS_SUPG_HEAT_VOF_EXTERNAL_LIBRARIES} mml_prism apl_std_lin utl_system utl_util pdl_vof ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
    SET_TARGET_PROPERTIES(${PDD_NS_SUPG_HEAT_VOF_PRISM_STD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
    SET_TARGET_PROPERTIES(${PDD_NS_SUPG_HEAT_VOF_PRISM_STD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
    SET_TARGET_PROPERTIES(${PDD_NS_SUPG_HEAT_VOF_PRISM_STD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
  else()
    message(WARNING "OpenCL for PDD_NS_SUPG_HEAT_VOF_PRISM_STD not implemented yet!!!")
  endif()
endif()

if(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_HYBRID_STD)
  if(NOT ${MODFEM_ACCEL} STREQUAL "opencl")
    add_executable(${PDD_NS_SUPG_HEAT_VOF_HYBRID_STD_EXE} ${SOURCE_FILES})
    target_link_libraries(${PDD_NS_SUPG_HEAT_VOF_HYBRID_STD_EXE} ${PDD_NS_SUPG_HEAT_VOF_EXTERNAL_LIBRARIES} mml_t4_prism apl_std_lin utl_system utl_util pdl_vof ${MODFEM_DIRECT_SOLVER_MODULE} ${MODFEM_ITER_SOLVER_MODULE} ${MODFEM_MPI_LIBS_STD})
    SET_TARGET_PROPERTIES(${PDD_NS_SUPG_HEAT_VOF_HYBRID_STD_EXE} PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE} RELEASE_POSTFIX "${RELEASE_POSTFIX}" DEBUG_POSTFIX "${DEBUG_POSTFIX}")
    SET_TARGET_PROPERTIES(${PDD_NS_SUPG_HEAT_VOF_HYBRID_STD_EXE} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
    SET_TARGET_PROPERTIES(${PDD_NS_SUPG_HEAT_VOF_HYBRID_STD_EXE} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
  else()
    message(WARNING "OpenCL for PDD_NS_SUPG_HEAT_VOF_HYBRID_STD not implemented yet!!!")
  endif()
endif()

##############################################################################################################
#enable_testing ()

foreach(folder ${MKL_INCLUDE_DIRS})
    set(KKK "${KKK}${folder}:")
endforeach()

foreach(folder ${MKL_LIBRARY_DIRS})
    set(KKK "${KKK}${folder}:")
endforeach()

set(KKK "${KKK}.")



set(CTEST_BUILD_DIR $ENV{PWD})
set(CTEST_DIR ${CMAKE_SOURCE_DIR}/../ctest)


add_test (ns_supg_heat_vof_Make ${CTEST_DIR}/build.sh ${CTEST_BUILD_DIR}/pdd_ns_supg_heat_vof/ ${ENV_PLATFORM} ${KKK})

set (passRegex "^Test passed")
set (failRegex "Fail")


if(MODFEM_MPI MATCHES "nompi")


# NO MPI

add_test (ns_supg_heat_vof_hybrid_plate_nas_120ppm_Run ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/ns_supg_heat_vof_hybrid_std_run.py ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/ ${CTEST_BUILD_DIR} ${KKK})
add_test (ns_supg_heat_vof_hybrid_plate_nas_120ppm_supg_Diff ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/ns_supg_heat_vof_hybrid_std_supg_field.py ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/)
add_test (ns_supg_heat_vof_hybrid_plate_nas_120ppm_heat_Diff ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/ns_supg_heat_vof_hybrid_std_heat_field.py ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/)
add_test (ns_supg_heat_vof_hybrid_plate_nas_120ppm_vof_Diff ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/ns_supg_heat_vof_hybrid_std_vof_field.py ${CTEST_DIR}/ns_supg_heat_vof/plate/nas/120ppm/)

set_tests_properties(ns_supg_heat_vof_hybrid_plate_nas_120ppm_supg_Diff PROPERTIES DEPENDS ns_supg_heat_vof_hybrid_plate_nas_120ppm_Run)
set_tests_properties(ns_supg_heat_vof_hybrid_plate_nas_120ppm_heat_Diff PROPERTIES DEPENDS ns_supg_heat_vof_hybrid_plate_nas_120ppm_Run)
set_tests_properties(ns_supg_heat_vof_hybrid_plate_nas_120ppm_vof_Diff PROPERTIES DEPENDS ns_supg_heat_vof_hybrid_plate_nas_120ppm_Run)
set_tests_properties(ns_supg_heat_vof_hybrid_plate_nas_120ppm_Run PROPERTIES DEPENDS ns_supg_heat_vof_Make)

set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_Run PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_Run PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_supg_Diff PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_supg_Diff PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_heat_Diff PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_heat_Diff PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")
set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_vof_Diff PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
set_property(TEST ns_supg_heat_vof_hybrid_plate_nas_120ppm_vof_Diff PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")



endif()
