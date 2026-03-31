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

# Set to MKL, ACML or GENERIC
set(MODFEM_BLASLAPACK MKL)


### EXTERNAL LIBRARIES CONFIGURATION SECTION ###


# MKL paths and names
# (only relevent if MODFEM_USE_MKL set to TRUE)
#
# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
set(MKL_INCLUDE_DIRS /opt/intel/composer_xe_2013/mkl/include /opt/intel/composer_xe_2013/mkl)
# Paths to look for MKL library files (you can provide more then one) and Intel Compiler library files (provide two paths)
set(MKL_LIBRARY_DIRS /opt/intel/composer_xe_2013/mkl/lib/intel64 /opt/intel/composer_xe_2013/lib/intel64)
# MKL library names (no extension) -
# - consult: http://software.intel.com/en-us/articles/intel-mkl-link-line-advisor/
set(MKL_INTEL_LIB_NAME mkl_intel_lp64)  #never use 'lib' prefix
set(MKL_LAPACK_LIB_NAME mkl_lapack95_lp64) #usually not needed  #never use 'lib' prefix
set(MKL_SOLVER_LIB_NAME )  #never use 'lib' prefix
set(MKL_IOMP5_LIB_NAME iomp5)  #never use 'lib' prefix
set(MKL_THREAD_LIB_NAME mkl_intel_thread) #never use 'lib' prefix
set(MKL_CORE_LIB_NAME mkl_core) #never use 'lib' prefix

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

# Libconfig paths and names
# (only important when building targets that use Libconfig)
set(LIBCONFIG_INCLUDE_DIRS /usr/include /usr/local/include)
set(LIBCONFIG_LIBRARY_DIRS /usr/lib64 /usr/local/lib/)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix

# Boost paths and names
# (only important when building targets that use Boost)
set(BOOST_ROOT /usr/include/boost)
set(BOOST_INCLUDEDIR /usr/include/boost)
set(BOOST_LIBRARYDIR /usr/lib64)
set(BOOST_VER_NO "1.53.0") #put the whole version number in ""


### TARGETS CONFIGURATION SECTION ###


# Specify which pdd targets should be created
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
set(CREATE_MOD_FEM_NS_SUPG_REMESH_STD TRUE)
# NS_SUPG_ALE
set(CREATE_MOD_FEM_NS_SUPG_ALE_REMESH_STD TRUE)
# NS_SUPG_HEAT
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_REMESH_STD TRUE)
# CONV_DIFF
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_DG TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_DG TRUE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_DG TRUE)
# LAPLACE
set(CREATE_MOD_FEM_LAPLACE_PRISM_STD TRUE)
set(CREATE_MOD_FEM_LAPLACE_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD TRUE) 
set(CREATE_MOD_FEM_LAPLACE_PRISM_DG TRUE)
set(CREATE_MOD_FEM_LAPLACE_HYBRID_DG TRUE)
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_DG TRUE)
# ELAST
set(CREATE_MOD_FEM_ELAST_PRISM_STD FALSE)
set(CREATE_MOD_FEM_ELAST_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_ELAST_PRISM2D_STD FALSE)
# CBS
set(CREATE_MOD_FEM_CBS_PRISM FALSE)
set(CREATE_MOD_FEM_CBS_HYBRID FALSE)
# TURBULENT
set(CREATE_MOD_FEM_TURBULENT_LIB FALSE)
# -------------------------------- HEAT --------------------------------#
set(CREATE_MOD_FEM_HEAT_PRISM_STD TRUE)            #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_PRISM_STD_QUAD FALSE)       #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD TRUE)          #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD_QUAD FALSE)     #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_HYBRID_STD TRUE)           #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_HYBRID_STD_QUAD FALSE)      #Set TRUE or FALSE
