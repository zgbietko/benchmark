set(sid_pardiso_srcdir "${CMAKE_SOURCE_DIR}/sid_pardiso")

# -- Include directories --
list(APPEND SID_PARDISO_INCLUDE_DIRS
  ${sid_pardiso_srcdir}
)

if(SID_PARDISO_INCLUDE_DIRS)
  set(SID_PARDISO_INCLUDE_DIRS ${SID_PARDISO_INCLUDE_DIRS} PARENT_SCOPE)
endif()

# -- Source files list --
collect_src_from_include_dirs(SID_PARDISO_INCLUDE_DIRS)

# -- Target --
include_directories( ${SID_PARDISO_INCLUDE_DIRS} ${SID_PARDISO_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
add_library( sil_pardiso ${SOURCE_FILES} )

target_link_libraries(sil_pardiso ${SID_PARDISO_EXTERNAL_LIBRARIES})

SET_TARGET_PROPERTIES(sil_pardiso PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(sil_pardiso PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(sil_pardiso PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
