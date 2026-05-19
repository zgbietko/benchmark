#Choose to use generic mmpd_adapter (TRUE) or previous mmpd_prism (FALSE)
set(MODFEM_NEW_MPI TRUE)

# Choose TRUE to buid static libraries (archives) and
# statically linked executables (slower); FALSE otherwise (faster)
set(MODFEM_USE_STATIC TRUE)

# Choose solver library used by executables
# Valid options are:
# - sil_pardiso
# - sil_krylow_bliter
# - sil_lapack
# Note: For sid_pardiso option MODFEM_USE_MKL must be set to TRUE
# Note: Due to cyclic dependency in sid_krylow_bliter MODFEM_USE_STATIC
# 	must be set to TRUE for that solver
set(MODFEM_ITER_SOLVER_MODULE sil_krylow_bliter)
set(MODFEM_DIRECT_SOLVER_MODULE sil_pardiso)

# Set to MKL, ACML or GENERIC
set(CMAKE_BUILD_TYPE "Release")
set(MODFEM_BLASLAPACK MKL)
set(CMAKE_CXX_FLAGS     "${CMAKE_CXX_FLAGS} -m64 -std=c++11")
set(CMAKE_C_FLAGS       "${CMAKE_C_FLAGS} -m64")
#----------------------------------------------#
#   EXTERNAL LIBRARIES CONFIGURATION SECTION   #
#----------------------------------------------#

set(MODFEM_USER_LIBS "-lpthread /usr/lib/x86_64-linux-gnu/libm.a /usr/lib/x86_64-linux-gnu/libdl.a" )

#----------------------
# MKL paths and names
# (only relevent if MODFEM_USE_MKL set to TRUE)
#
# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
set(MKL_INCLUDE_DIRS /opt/intel/mkl/include /opt/intel/mkl)
# Paths to look for MKL library files (you can provide more then one) and Intel Compiler library files (provide two paths)
set(MKL_LIBRARY_DIRS /opt/intel/mkl/lib/intel64 /opt/intel/lib/intel64 )
# MKL library names (no extension) -
# - consult: http://software.intel.com/en-us/articles/intel-mkl-link-line-advisor/
set(MKL_INTEL_LIB_NAME mkl_intel_lp64)  #never use 'lib' prefix
set(MKL_LAPACK_LIB_NAME mkl_lapack95_lp64) #usually not needed  #never use 'lib' prefix
set(MKL_SOLVER_LIB_NAME mkl_solver)  #never use 'lib' prefix
set(MKL_THREAD_LIB_NAME mkl_intel_thread) #never use 'lib' prefix
set(MKL_CORE_LIB_NAME mkl_core ) #never use 'lib' prefix
set(MKL_IOMP5_LIB_NAME iomp5 )   #never use 'lib' prefix
#----------------------
# Blas and Lapack paths and names
# (only relevant if MODFEM_USE_MKL set to FALSE)
# (when MKL in use blas/lapack from MKL will be used)
#
# Paths to look for Blas,Lapack libraries
# (usually can be left empty)
set(LAPACK_DIRS /usr/lib/lapack/)
set(BLAS_DIRS /usr/lib/libblas/)
# Blas,Lapack library names (no extension)
set(BLAS_LIB_NAME gfortran blas ) #never use 'lib' prefix
set(LAPACK_LIB_NAME gfortran lapack ) #never use 'lib' prefix

#----------------------
# Libconfig paths and names
# (only important when building targets that use Libconfig)
set(LIBCONFIG_INCLUDE_DIRS /usr/include)
set(LIBCONFIG_LIBRARY_DIRS /usr/lib /usr/local/lib/)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix

#----------------------
# Boost paths and names
# (only important when building targets that use Boost)
set(BOOST_ROOT /usr/local/include/boost_1.46.1)
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} /usr/local/include/boost_1.46.1)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/local/lib64/boost_1.46.1)
set(BOOST_VER_NO "1.46.1") #put the whole version number in ""

#----------------------
# Voro++ paths and names
# (only important when building targets that use Voro++)
set(VOROPP_INCLUDE_DIRS /usr/local/include/voro++)
set(VOROPP_LIBRARY_DIRS /usr/local/bin)
set(VOROPP_LIB_NAME voro++) #never use 'lib' prefix

# TODO: ACML, PARMETIS

# TODO: PARALLEL BUILD (MPI)

# ------------------ FreeGLUT library configuration ------------------- #
# FreeGLUT paths and names
set(GLUT_Xmu_LIBRARY /usr/lib/x86_64-linux-gnu/libXmu.so.6) #Library: libXmu.so.{x}
                                                   #{x} - version  

# ------------------------- MOD_FEM_VIWER ------------------------------#
set(CREATE_MOD_FEM_VIEWER FALSE)     #Set TRUE or FALSE
set(MOD_FEM_VIEWER_USE_AS_LIB FALSE) #Set TRUE or FALSE
set(MOD_FEM_VIEWER_GUI_LIN "WX") #St 'WX' gui support 

#-----------------------------------#
#   TARGETS CONFIGURATION SECTION   #
#-----------------------------------#

# Specify which exe targets should be created
# (usage: "make target_name", "make" will try to build all of them )
# use TRUE or FALSE. Do _not_ comment them out.

# HMT
set(CREATE_MOD_FEM_HMT_PRISM_STD FALSE)
set(CREATE_MOD_FEM_HMT_HYBRID_STD FALSE)
#set(CREATE_MOD_FEM_HMT_PRISM2D_STD TRUE)
# CBS_LIB
set(CREATE_MOD_FEM_CBS_LIB FALSE)
# NS_SUPG_NEW
set(CREATE_MOD_FEM_NS_SUPG_NEW_PRISM_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_NEW_PRISM_STD_TURBULENT FALSE)
set(CREATE_MOD_FEM_NS_SUPG_NEW_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_NEW_HYBRID_STD_TURBULENT FALSE)
#set(CREATE_MOD_FEM_NS_SUPG_NEW_PRISM2D_STD TRUE)
# NS_SUPG
set(CREATE_MOD_FEM_NS_SUPG_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HYBRID_STD TRUE)
#set(CREATE_MOD_FEM_NS_SUPG_PRISM2D_STD TRUE)
#set(CREATE_MOD_FEM_NS_SUPG_FCPM_STD TRUE)
# NS_SUPG_HEAT
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_REMESH_STD TRUE)
#set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD TRUE)
# NS_SUPG_HEAT_VOF
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM2D_STD FALSE)
# CONV_DIFF
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_DG FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_DG FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_DG FALSE)
# LAPLACE
set(CREATE_MOD_FEM_LAPLACE_PRISM_STD FALSE)
set(CREATE_MOD_FEM_LAPLACE_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_LAPLACE_PRISM_DG FALSE)
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


