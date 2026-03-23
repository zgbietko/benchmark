set(sid_amg_srcdir "${CMAKE_SOURCE_DIR}/sid_amg")

# -- Include directories --
list(APPEND SID_AMG_INCLUDE_DIRS
  ${sid_amg_srcdir}
)

if(SID_AMG_INCLUDE_DIRS)
  set(SID_AMG_INCLUDE_DIRS ${SID_AMG_INCLUDE_DIRS} PARENT_SCOPE)
endif()

# -- Source files list --
collect_src_from_include_dirs(SID_AMG_INCLUDE_DIRS)

# -- Target --
include_directories( ${SID_AMG_INCLUDE_DIRS} ${SID_AMG_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
add_library(sil_amg ${SOURCE_FILES} )

#target_link_libraries(sil_krylow_bliter ${SID_KRYLOW_BLITER_EXTERNAL_LIBRARIES}  lsl_mkb )
target_link_libraries(sil_amg ${SID_AMG_EXTERNAL_LIBRARIES}  amg_mkb )

SET_TARGET_PROPERTIES(sil_amg PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(sil_amg PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(sil_amg PROPERTIES LINK_FLAGS "${LINK_FLAGS}")