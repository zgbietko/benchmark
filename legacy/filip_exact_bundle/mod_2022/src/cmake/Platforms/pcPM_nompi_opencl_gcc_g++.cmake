set(MODFEM_NEW_MPI FALSE)
# Configuration type
# ------------------------ User compiler flags ------------------------ #
if(CMAKE_BUILD_TYPE STREQUAL "Release") #Flags for release mode
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -Wextra -fopenmp")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -std=c++11 -Wextra -fopenmp")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64 -Wextra -ldl")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug") #Flags for debug mode
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -ggdb3 -m64 -Wextra -fopenmp")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -ggdb3 -m64 -std=c++11 -Wextra -fopenmp")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64 -Wextra -ldl")
  add_definitions("-DDEBUG")
endif()
  # Address Sanitizer
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -O0 -g3 -ggdb3")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -O0 -g3 -ggdb3")
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -O0 -g3 -ggdb3")
  
#else() #Flags for other modes
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
#  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ")
#endif()

set(ARCHIVE_OUTPUT_PATH ${MODFEM_CONFIGURATION})
set(CMAKE_BUILD_TYPE ${MODFEM_CONFIGURATION})
# Choose TRUE to enable PARALLEL (MPI) 
set(MODFEM_MPI "nompi")
set(MODFEM_ACCEL "openmp")

# Choose TRUE to buid static libraries (archives) and
# statically linked executables (slower); FALSE otherwise (faster)
set(MODFEM_USE_STATIC TRUE)

# Choose solver library used by executables
# Valid options are:
# - sil_pardiso
# - sil_krylow_bliter
# - sil_lapack
# Note: For sid_pardiso option MODFEM_BLASLAPACK must be set to MKL
# Note: Due to cyclic dependency in sid_krylow_bliter MODFEM_USE_STATIC
# 	must be set to TRUE for that solver
set(MODFEM_ITER_SOLVER_MODULE sil_krylow_bliter)
set(MODFEM_DIRECT_SOLVER_MODULE sil_lapack)

# Set to MKL, ACML or GENERIC
set(MODFEM_BLASLAPACK GENERIC)

#----------------------------------------------#
#   EXTERNAL LIBRARIES CONFIGURATION SECTION   #
#----------------------------------------------#

#----------------------
# MKL paths and names
# (only relevent if MODFEM_BLASLAPACK set to MKL)
#
# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
set(MKL_INCLUDE_DIRS /opt/intel/mkl/10.2.5.035/include /opt/intel/mkl/10.2.5.035)
# Paths to look for MKL library files (you can provide more then one) and Intel Compiler library files (provide two paths)
set(MKL_LIBRARY_DIRS /opt/intel/mkl/10.2.5.035/lib/em64t)
# MKL library names (no extension) -
# - consult: http://software.intel.com/en-us/articles/intel-mkl-link-line-advisor/
set(MKL_INTEL_LIB_NAME mkl_intel_lp64)  #never use 'lib' prefix
set(MKL_LAPACK_LIB_NAME mkl_lapack) #usually not needed  #never use 'lib' prefix
set(MKL_SOLVER_LIB_NAME mkl_solver_lp64)  #never use 'lib' prefix
set(MKL_IOMP5_LIB_NAME iomp5 libiomp5md)  #never use 'lib' prefix
set(MKL_THREAD_LIB_NAME mkl_intel_thread) #never use 'lib' prefix
set(MKL_CORE_LIB_NAME mkl_core) #never use 'lib' prefix

#-----------------------
# ACML paths
# (only relevant if MODFEM_BLASLAPACK set to ACML)
#
# Paths to look for ACML include file
set(ACML_INCLUDE_DIRS /opt/acml4.4.0/gfortran64/include)
# Paths to look for ACML library
set(ACML_LIBRARY_DIRS /opt/acml4.4.0/gfortran64/lib)

#----------------------
# Blas and Lapack paths and names
# (only relevant if MODFEM_BLASLAPACK set to GENERIC)
#
# Paths to look for Blas,Lapack libraries
# (usually can be left empty)
set(LAPACK_DIRS C:/Libraries/lib/Win32/Release)
set(BLAS_DIRS C:/Libraries/lib/Win32/Release)
# Blas,Lapack library names (no extension)
set(BLAS_LIB_NAME blas) #never use 'lib' prefix
set(LAPACK_LIB_NAME lapack) #never use 'lib' prefix

#----------------------
# Libconfig paths and names
# (only important when building targets that use Libconfig)
if(WIN32)
set(LIBCONFIG_INCLUDE_DIRS C:/Libraries/include)
set(LIBCONFIG_LIBRARY_DIRS C:/libraries/lib/Win32)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix
elseif(UNIX)
set(LIBCONFIG_INCLUDE_DIRS /usr/include)
set(LIBCONFIG_LIBRARY_DIRS /usr/lib64 /usr/local/lib/)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix
endif()
#----------------------
# Boost paths and names
# (only important when building targets that use Boost)
if(WIN32)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_ADDITIONAL_VERSIONS 1.53.0) 
set(BOOST_ROOT "C:/Libraries/boost_1_53_0")
set(BOOST_INCLUDE_DIR "C:/Libraries/boost_1_53_0")
set(BOOST_LIBRARY_DIR "C:/Libraries/boost_1_53_0/libs")
set(BOOST_VER_NO "1.53.0") #put the whole version number in ""
endif()

#-----------------------
# FreeGLUT paths and names
if(WIN32)
set(GLUT_ROOT_PATH "C:/Libraries")
set(FREEGLUT_LIBRARY "C:/Libraries/lib/Win32/freeglut.lib")
set(FREEGLUT_INCLUDE_DIR "C:/Libraries/include")
elseif(UNIX)
set(GLUT_Xmu_LIBRARY /usr/lib64/libXmu.so.6)
endif()

#-------------------------
# OpeCL 
set(OPENCL_INCLUDE_DIRS /usr/include)
set(OPENCL_LIBRARY_DIRS   /usr/lib64)
set(OPENCL_LIBRARIES  /usr/lib64/libOpenCL.so)
set(OPENCL_MACHINE "gpu")
set(OPENCL_USE_LIBRARY_ONLY TRUE)
#----------------------------
# wxWidgets
set(wxWidgets_CONFIG_EXECUTABLE /usr/bin/wx-config-3.0)
set(wxWidgets_wxrc_EXECUTABLE	/usr/bin/wxrc-3.0)


# TODO: ACML, PARMETIS

# TODO: PARALLEL BUILD (MPI)

#-----------------------------------#
#   TARGETS CONFIGURATION SECTION   #
#-----------------------------------#

# Specify which exe targets should be created
# (usage: "make target_name", "make" will try to build all of them )
# use TRUE or FALSE. Do _not_ comment them out.

# HMT
set(CREATE_MOD_FEM_HMT_PRISM_STD FALSE)
set(CREATE_MOD_FEM_HMT_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_HMT_PRISM2D_STD FALSE)
# CBS_LIB
set(CREATE_MOD_FEM_CBS_LIB FALSE)
# NS_SUPG
set(CREATE_MOD_FEM_NS_SUPG_PRISM_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_PRISM_STD_TURBULENT FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HYBRID_STD_TURBULENT FALSE)
set(CREATE_MOD_FEM_NS_SUPG_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_REMESH_STD FALSE)
# NS_SUPG_HEAT
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD FALSE)
# CONV_DIFF
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_DG FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_DG FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_DG FALSE)
# LAPLACE
set(CREATE_MOD_FEM_LAPLACE_PRISM_STD TRUE)
set(CREATE_MOD_FEM_LAPLACE_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD TRUE)
set(CREATE_MOD_FEM_LAPLACE_PRISM_DG TRUE)
set(CREATE_MOD_FEM_LAPLACE_HYBRID_DG FALSE)
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_DG FALSE)
# ELAST
set(CREATE_MOD_FEM_ELAST_PRISM_STD FALSE)
set(CREATE_MOD_FEM_ELAST_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_ELAST_PRISM2D_STD FALSE)
# CBS
set(CREATE_MOD_FEM_CBS_PRISM FALSE)
set(CREATE_MOD_FEM_CBS_HYBRID FALSE)
# TURBULENT
set(CREATE_MOD_FEM_TURBULENT_LIB FALSE)
# MOD_FEM_VIWER
set(CREATE_MOD_FEM_VIEWER TRUE)
set(MOD_FEM_VIEWER_USE_AS_LIB TRUE)
set(MOD_FEM_VIEWER_GUI_WIN "WX")
set(MOD_FEM_VIEWER_GUI_LIN "WX")
