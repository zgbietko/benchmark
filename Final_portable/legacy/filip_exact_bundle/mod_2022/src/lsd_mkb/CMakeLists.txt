set(lsd_mkb_srcdir "${CMAKE_SOURCE_DIR}/lsd_mkb")

# -- Include directories --
list(APPEND LSD_MKB_INCLUDE_DIRS
  ${lsd_mkb_srcdir}
  ${lsd_mkb_srcdir}/mkb
  ${lsd_mkb_srcdir}/lad_block
)

if(LSD_MKB_INCLUDE_DIRS)
  set(LSD_MKB_INCLUDE_DIRS ${LSD_MKB_INCLUDE_DIRS} PARENT_SCOPE)
endif()

# -- Source files list --
collect_src_from_include_dirs(LSD_MKB_INCLUDE_DIRS)

# -- Target --
include_directories( ${LSD_MKB_INCLUDE_DIRS} ${LSD_MKB_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
add_library(lsl_mkb ${SOURCE_FILES} )

#target_link_libraries(lsl_mkb ${LSD_MKB_EXTERNAL_LIBRARIES} sil_krylow_bliter )
target_link_libraries(lsl_mkb ${LSD_MKB_EXTERNAL_LIBRARIES}  sil_krylow_bliter utl_util)

SET_TARGET_PROPERTIES(lsl_mkb PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(lsl_mkb PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(lsl_mkb PROPERTIES LINK_FLAGS "${LINK_FLAGS}")




#add_library(lsd_mkb STATIC lsh_mkb_fem_intf.h lsh_mkb_intf.h
#lss_mkb_fem_intf.c lss_mkb_intf.c 
#mkb/it_bliter mkb/it_bliter_blocks mkb/it_bliter_core mkb/it_bliter_util mkb/it_gmres)

#IF (WIN32)
#target_link_libraries(lsd_mkb sid_krylow_bliter blas lapack pthreadVC2)
#ELSE (WIN32)
#target_link_libraries(lsd_mkb sid_krylow_bliter )
#ENDIF (WIN32)
