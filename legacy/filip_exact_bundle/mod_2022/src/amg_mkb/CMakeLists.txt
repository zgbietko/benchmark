set(amg_mkb_srcdir "${CMAKE_SOURCE_DIR}/amg_mkb")

# -- Include directories --
list(APPEND AMG_MKB_INCLUDE_DIRS
  ${amg_mkb_srcdir}
  ${amg_mkb_srcdir}/amg
  ${amg_mkb_srcdir}/mkb
  ${amg_mkb_srcdir}/lad_block
  ${amg_mkb_srcdir}/lad_petsc
)

#test
#list(APPEND AMG_MKB_INCLUDE_DIRS
#  ${amg_mkb_srcdir}/test
#)

if(AMG_MKB_INCLUDE_DIRS)
  set(AMG_MKB_INCLUDE_DIRS ${AMG_MKB_INCLUDE_DIRS} PARENT_SCOPE)
endif()

# -- Source files list --
collect_src_from_include_dirs(AMG_MKB_INCLUDE_DIRS)

# -- Target --
include_directories( ${AMG_MKB_INCLUDE_DIRS} ${AMG_MKB_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
set(PETSC_HOME "/home/damian/petsc-3.5.3")
set(OPEN_MPI_HOME "/usr/lib/openmpi")
include_directories(${PETSC_HOME}/include ${PETSC_HOME}/arch-linux2-c-debug/include ${OPEN_MPI_HOME}/include ${OPEN_MPI_HOME}/include/openmpi)
add_library(amg_mkb ${SOURCE_FILES} )

link_directories(${PETSC_HOME}/arch-linux2-c-debug/lib)
target_link_libraries(amg_mkb ${AMG_MKB_EXTERNAL_LIBRARIES} sil_amg ${PETSC_HOME}/arch-linux2-c-debug/lib/libpetsc.so openblas lapack mpi)

set(AMG_MKB_LINK_FLAGS "-Wl,-rpath,${PETSC_HOME}/arch-linux2-c-debug/lib -L${PETSC_HOME}/arch-linux2-c-debug/lib -lX11 -lssl -lcrypto -Wl,-rpath,/usr/lib/openmpi/lib -L/usr/lib/openmpi/lib -Wl,-rpath,/usr/lib/gcc/x86_64-linux-gnu/4.9 -L/usr/lib/gcc/x86_64-linux-gnu/4.9 -Wl,-rpath,/usr/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -Wl,-rpath,/lib/x86_64-linux-gnu -L/lib/x86_64-linux-gnu -lmpi_f90 -lmpi_f77 -lgfortran -lm -lgfortran -lm -lquadmath -lm -lmpi_cxx -lstdc++ -Wl,-rpath,/usr/lib/openmpi/lib -L/usr/lib/openmpi/lib -Wl,-rpath,/usr/lib/gcc/x86_64-linux-gnu/4.9 -L/usr/lib/gcc/x86_64-linux-gnu/4.9 -Wl,-rpath,/usr/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -Wl,-rpath,/lib/x86_64-linux-gnu -L/lib/x86_64-linux-gnu -Wl,-rpath,/usr/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -ldl -lmpi -lhwloc -lgcc_s -lpthread -ldl")

SET_TARGET_PROPERTIES(amg_mkb PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(amg_mkb PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(amg_mkb PROPERTIES LINK_FLAGS "${AMG_MKB_LINK_FLAGS}")


# Test
add_executable(test_jacobi_method ${amg_mkb_srcdir}/test/amg/JacobiMethodTest.cpp)
target_link_libraries(test_jacobi_method amg_mkb)
SET_TARGET_PROPERTIES(test_jacobi_method PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(test_jacobi_method PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
add_test(amg_test test_jacobi_method) 
