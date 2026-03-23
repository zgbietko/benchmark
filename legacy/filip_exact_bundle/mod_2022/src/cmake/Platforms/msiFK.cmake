#########################################################################
#                                ModFEM                                 #
#                  computerJB-OLD-MPI-OpenCL.cmake                      #
#########################################################################

# ------------------------ User compiler flags ------------------------ #
if(CMAKE_BUILD_TYPE STREQUAL "Release") #Flags for release mode
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -std=c++11 -Wextra")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64 -Wextra")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug") #Flags for debug mode
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -ggdb3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -ggdb3")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64 -Wextra -ldl")

  # Address Sanitizer
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -O0 -g3 -ggdb3")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -O0 -g3 -ggdb3")
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -O0 -g3 -ggdb3")
  
else() #Flags for other modes
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ")
endif()

# ------------------------- Libraries linking  ------------------------ #
# Choose TRUE to buid static libraries (archives) and
# statically linked executables (slower); 
# FALSE otherwise (faster)
set(MODFEM_USE_STATIC TRUE)

# ---------------------- NEW MPI mmpd_adapter ------------------------- #
#Choose to use generic mmpd_adapter (TRUE) or previous mmpd_prism (FALSE)
set(MODFEM_NEW_MPI FALSE)

# --------------------------- ModFEM Solver  -------------------------- #
# Choose solver library used by executables
# Valid options are:
# - sil_pardiso       [direct solver]
# - sil_krylow_bliter [iterative solver]
# - sil_lapack        [iterative solver]
# - sil_viennacl_crs  [iterative solver]
# Note: For sid_pardiso option MODFEM_USE_MKL must be set to TRUE
# Note: Due to cyclic dependency in sid_krylow_bliter MODFEM_USE_STATIC
# 	must be set to TRUE for that solver
set(MODFEM_ITER_SOLVER_MODULE sil_krylow_bliter)
set(MODFEM_DIRECT_SOLVER_MODULE sil_pardiso)

# -------------------------- Algebra library  ------------------------- #
# Select linear algebra libraries BLAS/LAPACK
# Set to MKL, ACML or GENERIC
set(MODFEM_BLASLAPACK MKL)

#-----------------------------------------------------------------------#
#              EXTERNAL LIBRARIES CONFIGURATION SECTION                 #
#-----------------------------------------------------------------------#

# ---------------------- Intel MKL configuration ---------------------- #
# MKL paths and names
# (only relevent if MODFEM_USE_MKL set to TRUE)

# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
set(MKL_INCLUDE_DIRS /usr/local/intel/mkl/include /usr/local/intel/mkl)


# Paths to look for MKL library files (you can provide more then one) 
# and Intel Compiler library files (provide two paths)
# Available intel architecture subdirectories:
# - intel64 -> libraries for 64-bit system
# - ia32    -> libraries for 32-bit system
# - mic     -> libraries for MIC Xeon Phi architecture
set(MKL_LIBRARY_DIRS /opt/intel/mkl/lib/intel64 /opt/intel/lib/intel64) # 64-bit system

# MKL library names (no extension) -
# - consult: http://software.intel.com/en-us/articles/intel-mkl-link-line-advisor/
# Select 32-bit or 64-bit system libraries by comment unused

# 32-bit system
#set(MKL_INTEL_LIB_NAME mkl_intel)  #never use 'lib' prefix
#set(MKL_LAPACK_LIB_NAME mkl_lapack95) #usually not needed  #never use 'lib' prefix
#set(MKL_SOLVER_LIB_NAME mkl_solver)  #never use 'lib' prefix
#set(MKL_IOMP5_LIB_NAME iomp5 libiomp5md)  #never use 'lib' prefix
#set(MKL_THREAD_LIB_NAME mkl_intel_thread) #never use 'lib' prefix
#set(MKL_CORE_LIB_NAME mkl_core) #never use 'lib' prefix

# 64-bit system
set(MKL_INTEL_LIB_NAME mkl_intel_lp64)  #never use 'lib' prefix
set(MKL_LAPACK_LIB_NAME mkl_lapack95_lp64) #usually not needed  #never use 'lib' prefix
set(MKL_SOLVER_LIB_NAME mkl_solver_lp64)  #never use 'lib' prefix
set(MKL_IOMP5_LIB_NAME iomp5 libiomp5md)  #never use 'lib' prefix
set(MKL_THREAD_LIB_NAME mkl_intel_thread) #never use 'lib' prefix
set(MKL_CORE_LIB_NAME mkl_core) #never use 'lib' prefix

# ------------------- BLAS and LAPACK configuration ------------------- #
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

# ---------------------- LIBCONFIG configuration ---------------------- #
# Libconfig paths and names
# (only important when building targets that use Libconfig)
set(LIBCONFIG_INCLUDE_DIRS /usr/include)
set(LIBCONFIG_LIBRARY_DIRS /usr/lib64 /usr/local/lib/)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix

# ------------------------ BOOST configuration ------------------------ #
# Boost paths and names
# (only important when building targets that use Boost)
set(BOOST_ROOT /usr/include/boost)
set(BOOST_INCLUDEDIR /usr/include/boost)
set(BOOST_LIBRARYDIR /usr/lib/x86_64-linux-gnu/l)
set(BOOST_VER_NO "1.56.0") #put the whole version number in ""

# ------------------ FreeGLUT library configuration ------------------- #
# FreeGLUT paths and names
#set(GLUT_Xmu_LIBRARY /usr/lib/x86_64-linux-gnu/libXmu.so.6)
#set(GLUT_Xmu_LIBRARY /usr/lib64/libXmu.so.6)
set(GLUT_Xmu_LIBRARY Xmu)

# ------------------------ OPENCL configuration ----------------------- #
# OpenCL - setting opencl acceleration
# Available machine flag: 
# cpu - central processing unit
# gpu - graphics processing unit
# phi - xeon phi acceleartor
set(OPENCL_INCLUDE_DIRS /usr/include /usr/local/cuda/include)
set(OPENCL_LIBRARY_DIRS   /usr/lib /usr/local/cuda/lib64)
set(OPENCL_MACHINE "gpu")
set(OPENCL_USE_LIBRARY_ONLY TRUE)

# --------------------------- FUTURE changes -------------------------- #
# TODO: ACML, PARMETIS
# TODO: PARALLEL BUILD (MPI)
set(METIS_INCLUDE_DIR /usr/local/include)

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
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD TRUE)
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

# -------------------------- NS_SUPG_HEAT ------------------------------#
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD FALSE)
set(CREATE_MOD_FEM_NS_SUPG_REMESH_STD FALSE)

# ------------------------- MOD_FEM_VIWER ------------------------------#
set(CREATE_MOD_FEM_VIEWER FALSE)
set(MOD_FEM_VIEWER_USE_AS_LIB FALSE)
set(MOD_FEM_VIEWER_GUI_LIN "WX")

