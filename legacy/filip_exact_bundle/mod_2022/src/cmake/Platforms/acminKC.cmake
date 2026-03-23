#Choose to use generic mmpd_adapter (TRUE) or previous mmpd_prism (FALSE)
set(MODFEM_NEW_MPI FALSE)

# Choose TRUE to enable PARALLEL (MPI) 
set(MODFEM_MPI nompi)
# Choose TRUE to build static libraries (archives) and
# statically linked executables (slower); FALSE otherwise (faster)
set(MODFEM_USE_STATIC TRUE)

# Choose solver library used by executables
# Valid options are:
# - sil_pardiso
# - sil_krylow_bliter
# - sil_lapack
# - sil_viennacl_crs
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
#
# ZEUS module extras
# module add mkl/10.2.2.025
# module add libs/boost/1.52.0-intel-13.0.1
# module add tools/cmake/2.8.2
# module add tools/libconfig/1.4.9

#----------------------
# MKL paths and names
# (only relevent if MODFEM_USE_MKL set to TRUE)
# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
# module add mkl/10.2.2.025
set(MKL_INCLUDE_DIRS /opt/intel/mkl/include /opt/intel/mkl)
# Paths to look for MKL library files (you can provide more then one) and Intel Compiler library files (provide two paths)
set(MKL_LIBRARY_DIRS /opt/intel/mkl/lib/intel64 /opt/intel/composerxe/lib/intel64)
#set(MKL_LIBRARY_DIRS /software/local/Intel/compiler12.1/lib/intel64)
# MKL library names (no extension) -
# - consult: http://software.intel.com/en-us/articles/intel-mkl-link-line-advisor/

set(MKL_INTEL_LIB_NAME mkl_intel_lp64)  #never use 'lib' prefix
set(MKL_LAPACK_LIB_NAME mkl_lapack95_lp64) #usually not needed  #never use 'lib' prefix
set(MKL_SOLVER_LIB_NAME mkl_solver_lp64)  #never use 'lib' prefix
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
set(LIBCONFIG_INCLUDE_DIRS /usr/local/include)
set(LIBCONFIG_LIBRARY_DIRS /usr/local/lib)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix

#----------------------
# Voro++ paths and names
# (only important when building targets that use Voro++)
set(VOROPP_INCLUDE_DIRS /usr/local/include/voro++)
set(VOROPP_LIBRARY_DIRS /usr/local/lib)
set(VOROPP_LIB_NAME voro++) #never use 'lib' prefix

#set(Subversion_SVN_EXECUTABLE /usr/bin/svn)
#set(SVNCOMMAND "svn up")

set(VIENNACL_INCLUDE_DIRS /home/kchlon/ViennaCL-1.6.2)
set(VIENNACL_LIBRARY_DIRS /home/kchlon/ViennaCL-1.6.2)
set(VIENNACL_LIB_NAME viennacl) #never use 'lib' prefix

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


#----------------------
# OpenCL - setting opencl acceleration
# Available machine flag: 
# cpu - central processing unit
# gpu - graphics processing unit
# phi - xeon phi acceleartor
# apu - amd apu unit ???
set(OPENCL_INCLUDE_DIRS /usr/include /usr/local/cuda-5.0/include)
set(OPENCL_LIBRARY_DIRS /usr/lib64 /usr/local/cuda-5.0/lib64)
set(OPENCL_LIBRARIES /usr/lib64/libOpenCL.so)
set(OPENCL_MACHINE "gpu")


# TODO: ACML, PARMETIS

# TODO: PARALLEL BUILD (MPI)

#-----------------------------------#
#   TARGETS CONFIGURATION SECTION   #
#-----------------------------------#

# Specify which exe targets should be created
# (usage: "make target_name", "make" will try to build all of them )
# use TRUE or FALSE. Do _not_ comment them out.

# HEAT
set(CREATE_MOD_FEM_HEAT_PRISM_STD TRUE)
set(CREATE_MOD_FEM_HEAT_PRISM_STD_QUAD FALSE)
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD_QUAD FALSE)
set(CREATE_MOD_FEM_HEAT_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_HEAT_HYBRID_STD_QUAD FALSE)

# NS_SUPG
set(CREATE_MOD_FEM_NS_SUPG_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_PRISM2D_STD FALSE)
# NS_SUPG_HEAT
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD FALSE)
# NS_SUPG_HEAT_REMESH
set(CREATE_MOD_FEM_NS_SUPG_HEAT_REMESH_STD TRUE)
# NS_SUPG_HEAT_VOF
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM2D_STD FALSE)
# CONV_DIFF
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_DG TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_DG TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_DG FALSE)
# LAPLACE
set(CREATE_MOD_FEM_LAPLACE_PRISM_STD TRUE)
set(CREATE_MOD_FEM_LAPLACE_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD FALSE) 
set(CREATE_MOD_FEM_LAPLACE_PRISM_DG TRUE)
set(CREATE_MOD_FEM_LAPLACE_HYBRID_DG TRUE)
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_DG FALSE)
# ELAST
set(CREATE_MOD_FEM_ELAST_PRISM_STD FALSE)
set(CREATE_MOD_FEM_ELAST_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_ELAST_PRISM2D_STD FALSE)
