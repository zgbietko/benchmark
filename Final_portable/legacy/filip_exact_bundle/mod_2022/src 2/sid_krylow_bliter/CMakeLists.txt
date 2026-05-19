set(sid_krylow_bliter_srcdir "${CMAKE_SOURCE_DIR}/sid_krylow_bliter")

# -- Include directories --
list(APPEND SID_KRYLOW_BLITER_INCLUDE_DIRS
  ${sid_krylow_bliter_srcdir}
)

if(SID_KRYLOW_BLITER_INCLUDE_DIRS)
  set(SID_KRYLOW_BLITER_INCLUDE_DIRS ${SID_KRYLOW_BLITER_INCLUDE_DIRS} PARENT_SCOPE)
endif()

# -- Source files list --
collect_src_from_include_dirs(SID_KRYLOW_BLITER_INCLUDE_DIRS)

# -- Target --
include_directories( ${SID_KRYLOW_BLITER_INCLUDE_DIRS} ${SID_KRYLOW_BLITER_EXTERNAL_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
add_library(sil_krylow_bliter ${SOURCE_FILES} )

#target_link_libraries(sil_krylow_bliter ${SID_KRYLOW_BLITER_EXTERNAL_LIBRARIES}  lsl_mkb )
target_link_libraries(sil_krylow_bliter ${SID_KRYLOW_BLITER_EXTERNAL_LIBRARIES}  lsl_mkb )

SET_TARGET_PROPERTIES(sil_krylow_bliter PROPERTIES COMPILE_DEFINITIONS ${MODFEM_MPI_CODE_DEFINE})
SET_TARGET_PROPERTIES(sil_krylow_bliter PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(sil_krylow_bliter PROPERTIES LINK_FLAGS "${LINK_FLAGS}")



# Windows
#set(MKL_INCLUDE_PATH_WINDOWS "c:\\ProgramFiles\\Intel\\MKL\\10.2.6.037\\include")
# Linux
#set(MKL_INCLUDE_PATH_LINUX $MKLROOT/include)

# Windows
#set(MKL_LINK_PATH_WINDOWS "c:\\ProgramFiles\\Intel\\MKL\\10.2.6.037\\ia32\\lib")
# Linux
#set(MKL_LINK_PATH_LINUX $MKLROOT/lib/em64t)

# Windows
#set(MKL_LIBRARIES_WINDOWS mkl_solver.lib mkl_intel_c_dll.lib mkl_intel_thread_dll.lib mkl_core_dll.lib -Qopenmp)
# Linux
#set(MKL_LIBRARIES_LINUX  -L/opt/intel/mkl/10.2.5.035/lib/em64t/ -Wl,--start-group -lmkl_lapack -lmkl_core -lmkl_intel_lp64 -lmkl_intel_thread  -lguide -lpthread -lm -Wl,--end-group )

#if(WIN32)
#        include_directories(../include ${MKL_INCLUDE_PATH_WINDOWS})
#        link_directories(${MKL_LINK_PATH_WINDOWS})
#elseif(UNIX)
#        include_directories(../include ${MKL_INCLUDE_PATH_LINUX})
#        link_directories(${MKL_LINK_PATH_LINUX})
#endif(WIN32)

#add_library(sid_krylow_bliter 
#STATIC 
#sis_krylow_bliter_fem_intf.c sis_krylow_bliter_intf.c)

#target_link_libraries(sid_krylow_bliter lsd_mkb )

#if(WIN32)
#        target_link_libraries(sid_krylow_bliter ${MKL_LIBRARIES_WINDOWS})
#elseif(UNIX)
#        target_link_libraries(sid_krylow_bliter lsd_mkb ${MKL_LIBRARIES_LINUX})
#endif(WIN32)

