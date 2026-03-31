# Allowed values: none, opencl, cuda
set(MODFEM_ACCEL none)
# Allowed values: none, opencl, cuda
set(MODFEM_MPI nompi)

# Choose TRUE to build static libraries (archives) and
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

# Set TRUE if you want to use Blas,Lapack and Pardiso
# implementation from Intel MKL
# (otherwise standard blas and lapack will be used)
#set(MODFEM_USE_MKL TRUE)
# Set to MKL, ACML or GENERIC
set(MODFEM_BLASLAPACK MKL)

#----------------------------------------------#
#   EXTERNAL LIBRARIES CONFIGURATION SECTION   #
#----------------------------------------------#

#----------------------
# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
set(MKL_INCLUDE_DIRS /opt/intel/mkl/include)
# Paths to look for MKL library files (you can provide more then one) and Intel Compiler library files (provide two paths)
set(MKL_LIBRARY_DIRS /opt/intel/mkl/lib/ia32 /opt/intel/composerxe/lib/ia32)
# MKL library names (no extension) -
# - consult: http://software.intel.com/en-us/articles/intel-mkl-link-line-advisor/
set(MKL_INTEL_LIB_NAME mkl_intel)  #never use 'lib' prefix
set(MKL_LAPACK_LIB_NAME mkl_lapack95) #usually not needed  #never use 'lib' prefix
set(MKL_SOLVER_LIB_NAME mkl_solver)  #never use 'lib' prefix
set(MKL_IOMP5_LIB_NAME iomp5 libiomp5md)  #never use 'lib' prefix
set(MKL_THREAD_LIB_NAME mkl_intel_thread) #never use 'lib' prefix
set(MKL_CORE_LIB_NAME mkl_core) #never use 'lib' prefix

#----------------------
# Blas and Lapack paths and names
# (only relevant if MODFEM_USE_MKL set to FALSE)
# (when MKL in use blas/lapack from MKL will be used)
#
# Paths to look for Blas,Lapack libraries
# (usually can be left empty)
set(LAPACK_DIRS)
set(BLAS_DIRS)
# Blas,Lapack library names (no extension)
set(BLAS_LIB_NAME blas) #never use 'lib' prefix
set(LAPACK_LIB_NAME lapack) #never use 'lib' prefix

#----------------------
# Libconfig paths and names
# (only important when building targets that use Libconfig)
# module add tools/libconfig/1.4.9
set(LIBCONFIG_INCLUDE_DIRS /usr/include)
set(LIBCONFIG_LIBRARY_DIRS /usr/lib)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix

#----------------------
# Voro++ paths and names
# (only important when building targets that use Voro++)
set(VOROPP_INCLUDE_DIRS /usr/local/include/voro++)
set(VOROPP_LIBRARY_DIRS /usr/local/lib)
set(VOROPP_LIB_NAME voro++) #never use 'lib' prefix

#----------------------
# Boost paths and names
# (only important when building targets that use Boost)
# module add libs/boost/1.41.0
set(BOOST_ROOT /usr/local/include/boost)
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} /usr/local/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/local/lib)
#set(BOOST_INCLUDEDIR /usr/include)
#set(BOOST_LIBRARYDIR /usr/lib64)
set(BOOST_VER_NO "1.41.0") #put the whole version number in ""

# TODO: PARMETIS

#-----------------------
# ACML paths
# (only relevant if MODFEM_BLASLAPACK set to ACML)
#
# Paths to look for ACML include file
#set(ACML_INCLUDE_DIRS /opt/acml4.4.0/gfortran64/include)
# Paths to look for ACML library
#set(ACML_LIBRARY_DIRS /opt/acml4.4.0/gfortran64/lib)

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
# NS_SUPG_NEW
set(CREATE_MOD_FEM_NS_SUPG_NEW_PRISM_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_NEW_PRISM_STD_TURBULENT FALSE)
set(CREATE_MOD_FEM_NS_SUPG_NEW_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_NEW_HYBRID_STD_TURBULENT FALSE)
set(CREATE_MOD_FEM_NS_SUPG_NEW_PRISM2D_STD FALSE)
# NS_SUPG
set(CREATE_MOD_FEM_NS_SUPG_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_PRISM2D_STD TRUE)
# NS_SUPG_HEAT
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD TRUE)
# NS_SUPG_HEAT_VOF
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM2D_STD TRUE)
# CONV_DIFF
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD FALSE)
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
