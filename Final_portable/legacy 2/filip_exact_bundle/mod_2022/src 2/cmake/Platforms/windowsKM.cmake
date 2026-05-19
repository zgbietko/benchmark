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
set(MODFEM_DIRECT_SOLVER_MODULE sil_lapack)

# Set to MKL, ACML or GENERIC
set(MODFEM_BLASLAPACK GENERIC)

#----------------------------------------------#
#   EXTERNAL LIBRARIES CONFIGURATION SECTION   #
#----------------------------------------------#

#----------------------
# MKL paths and names
# (only relevent if MODFEM_USE_MKL set to TRUE)
#
# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
set(MKL_INCLUDE_DIRS /opt/intel/mkl/include /opt/intel/mkl)
# Paths to look for MKL library files (you can provide more then one) and Intel Compiler library files (provide two paths)
set(MKL_LIBRARY_DIRS /opt/intel/mkl/lib/ia32 /opt/intel/lib/ia32)
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
set(LAPACK_DIRS c:/local/lapack/)
set(BLAS_DIRS c:/local/lapack/)
# Blas,Lapack library names (no extension)
set(BLAS_LIB_NAME "blas.lib") #never use 'lib' prefix in *nix systems
set(LAPACK_LIB_NAME "lapack.lib") #never use 'lib' prefix in *nix systems
set(F2C_LIB_NAME "libf2c.lib")
#----------------------
# Libconfig paths and names
# (only important when building targets that use Libconfig)
set(LIBCONFIG_INCLUDE_DIRS "c:/local/libconfig-1.4.9/32/lib")
set(LIBCONFIG_LIBRARY_DIRS "c:/local/libconfig-1.4.9/32/Release/")
set(LIBCONFIG_LIB_NAME libconfig) #never use 'lib' prefix in *nix systems

#----------------------
# Boost paths and names
# (only important when building targets that use Boost)
set(BOOST_ROOT "C:/local/boost_1_57_0/msvc_12.0(2013)/")
set(BOOST_INCLUDEDIR "C:/local/boost_1_57_0/msvc_12.0(2013)/")
set(BOOST_LIBRARYDIR "C:/local/boost_1_57_0/msvc_12.0(2013)/lib32-msvc-12.0/")
set(BOOST_VER_NO "1.57.0") #put the whole version number in ""

# TODO: ACML, PARMETIS

# TODO: PARALLEL BUILD (MPI)

#-----------------------------------------------------------------------#
#                      TARGETS CONFIGURATION SECTION                    #
#-----------------------------------------------------------------------#

# Specify which exe targets should be created
# (usage: "make target_name", "make" will try to build all of them )
# use TRUE or FALSE. Do _not_ comment them out.
# All available exe targets will be shown by main cmake,
# status of exe targets:
# TRUE      - target will be built
# FALSE     - target will not be built
# UNDEFINED - target is not set in your platform file

# ----------------------------- CONV_DIFF ------------------------------#
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD_QUAD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD_QUAD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD_QUAD FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_DG FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_DG FALSE)
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_DG FALSE)

# -------------------------------- HEAT --------------------------------#
set(CREATE_MOD_FEM_HEAT_PRISM_STD FALSE)
set(CREATE_MOD_FEM_HEAT_PRISM_STD_QUAD FALSE)
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD_QUAD FALSE)
set(CREATE_MOD_FEM_HEAT_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_HEAT_HYBRID_STD_QUAD FALSE)

# NS_SUPG
set(CREATE_MOD_FEM_NS_SUPG_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_PRISM2D_STD FALSE)

# -------------------------- NS_SUPG_HEAT ------------------------------#
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD TRUE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_REMESH_STD FALSE)

# ------------------------- MOD_FEM_VIWER ------------------------------#
set(CREATE_MOD_FEM_VIEWER FALSE)
set(MOD_FEM_VIEWER_USE_AS_LIB FALSE)
set(MOD_FEM_VIEWER_GUI_LIN "WX")

