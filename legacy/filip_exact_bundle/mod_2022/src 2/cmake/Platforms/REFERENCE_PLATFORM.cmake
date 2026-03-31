#########################################################################
#                                ModFEM                                 #
#                        REFERENCE PLATFORM FILE                        #
#########################################################################

# ------------------------ User compiler flags ------------------------ #
if(CMAKE_BUILD_TYPE STREQUAL "Release") #Flags for release mode
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} PUT_YOUR_FLAGS_HERE") #C compiler flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} PUT_YOUR_FLAGS_HERE") #C++ compiler flags
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} PUT_YOUR_FLAGS_HERE") #Linker flags
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug") #Flags for debug mode
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} PUT_YOUR_FLAGS_HERE") #C compiler flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} PUT_YOUR_FLAGS_HERE") #C++ compiler flags
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} PUT_YOUR_FLAGS_HERE") #Linker flags
else() #Flags for other modes
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} PUT_YOUR_FLAGS_HERE") #C compiler flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} PUT_YOUR_FLAGS_HERE") #C++ compiler flags
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} PUT_YOUR_FLAGS_HERE") #Linker flags
endif()

# ------------------------- Libraries linking  ------------------------ #
# Choose TRUE to buid static libraries (archives) and
# statically linked executables (slower); 
# FALSE otherwise (faster)
set(MODFEM_USE_STATIC PUT_YOUR_CHOICE) #Set TRUE or FALSE

# ---------------------- NEW MPI mmpd_adapter ------------------------- #
#Choose to use generic mmpd_adapter (TRUE) or previous mmpd_prism (FALSE)
set(MODFEM_NEW_MPI PUT_YOUR_CHOICE) #Set TRUE or FALSE

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
set(MODFEM_ITER_SOLVER_MODULE PUT_YOUR_CHOICE) #Iterative solver
set(MODFEM_DIRECT_SOLVER_MODULE PUT_YOUR_CHOICE) #Direct solver

# -------------------------- Algebra library  ------------------------- #
# Select linear algebra libraries BLAS/LAPACK
# Set to MKL, ACML or GENERIC
set(MODFEM_BLASLAPACK PUT_YOUR_CHOICE) # Set MKL or ACML or GENERIC

#-----------------------------------------------------------------------#
#              EXTERNAL LIBRARIES CONFIGURATION SECTION                 #
#-----------------------------------------------------------------------#

# ---------------------- Intel MKL configuration ---------------------- #
# MKL paths and names
# (only relevent if MODFEM_USE_MKL set to TRUE)

# Paths to look for MKL include file
# (you can provide more then one - whitespace separated)
set(MKL_INCLUDE_DIRS PUT_YOUR_PATH_TO_MKL_INCLUDE_DIRECTORIES)

# Paths to look for MKL library files (you can provide more then one) 
# and Intel Compiler library files (provide two paths)
# Available intel architecture subdirectories:
# - intel64 -> libraries for 64-bit system
# - ia32    -> libraries for 32-bit system
# - mic     -> libraries for MIC Xeon Phi architecture
set(MKL_LIBRARY_DIRS PUT_YOUR_PATH_TO_MKL_LIBRARY_DIRECTORIES)

# MKL library names (no extension) -
# - consult: http://software.intel.com/en-us/articles/intel-mkl-link-line-advisor/
# Select 32-bit or 64-bit system libraries by comment unused

# 32-bit system
set(MKL_INTEL_LIB_NAME mkl_intel)  #never use 'lib' prefix
set(MKL_LAPACK_LIB_NAME mkl_lapack95) #usually not needed  #never use 'lib' prefix
set(MKL_SOLVER_LIB_NAME mkl_solver)  #never use 'lib' prefix
set(MKL_IOMP5_LIB_NAME iomp5 libiomp5md)  #never use 'lib' prefix
set(MKL_THREAD_LIB_NAME mkl_intel_thread) #never use 'lib' prefix
set(MKL_CORE_LIB_NAME mkl_core) #never use 'lib' prefix

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
set(LAPACK_DIRS PUT_YOUR_LAPACK_DIRECTORY_OR_NOTHING)
set(BLAS_DIRS PUT_YOUR_BLAS_DIRECTORY_OR_NOTHING)

# Blas,Lapack library names (no extension)
set(BLAS_LIB_NAME blas) #never use 'lib' prefix
set(LAPACK_LIB_NAME lapack) #never use 'lib' prefix

# ---------------------- LIBCONFIG configuration ---------------------- #
# Libconfig paths and names
# (only important when building targets that use Libconfig)
set(LIBCONFIG_INCLUDE_DIRS PUT_YOUR_PATH_TO_INCLUDE_DIRECTORY) 
set(LIBCONFIG_LIBRARY_DIRS PUT_YOUR_PATH_TO_LIBRARY_DIRECTORY)
set(LIBCONFIG_LIB_NAME config) #never use 'lib' prefix

# ------------------------ BOOST configuration ------------------------ #
# Boost paths and names
# (only important when building targets that use Boost)
set(BOOST_ROOT PUT_YOUR_PATH_TO_BOOST_ROOT_INCLUDE_DIRECTORY)
set(BOOST_INCLUDEDIR PUT_YOUR_PATH_TO_BOOST_INCLUDE_DIRECTORY)
set(BOOST_LIBRARYDIR PUT_YOUR_PATH_TO_BOOST_LIBRARY_DIRECTORY)
set(BOOST_VER_NO "PUT_YOUR_BOOST_VERSION") #put the whole version number in ""

# ---------------------------- VORO++ --------------------------------- # 
# Voro++ paths and names
# (only important when building targets that use Voro++)
set(VOROPP_INCLUDE_DIRS PUT_YOUR_PATH_TO_INCLUDE_DIRECTORY)
set(VOROPP_LIBRARY_DIRS PUT_YOUR_PATH_TO_BOOST_LIBRARY_DIRECTORY)
set(VOROPP_LIB_NAME voro++) #never use 'lib' prefix

# ------------------ FreeGLUT library configuration ------------------- #
# FreeGLUT paths and names
set(GLUT_Xmu_LIBRARY PUT_YOUR_PATH_TO_XMU_LIBRARY) #Library: libXmu.so.{x}
                                                    #{x} - version  

# ------------------------ OPENCL configuration ----------------------- #
# OpenCL - setting opencl acceleration
# Available machine flag: 
# cpu - central processing unit
# gpu - graphics processing unit
# phi - xeon phi acceleartor
# hsa - heterogeneous system architecture
set(OPENCL_INCLUDE_DIRS YOUR_PATH_TO_OPENCL_INCLUDE_DIRECTORIES) #Path to directories where is CL/OpenCL folder
set(OPENCL_LIBRARY_DIRS YOUR_PATH_TO_OPENCL_LIBRARY_DIRECTORIES) #Path to directories where is OpenCL library
set(OPENCL_MACHINE "YOUR_OPENCL_MACHINE") #Used machine cpu/gpu/phi/hsa
set(OPENCL_USE_LIBRARY_ONLY PUT_YOUR_CHOICE) #Set TRUE or FALSE for use OpenCL library
set(OPENCL_AUTO_TUNING PUT_YOUR_CHOICE) #Set TRUE or FALSE for use auto-tuning

# --------------------------- FUTURE changes -------------------------- #
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

# ------------------------------ LAPLACE -------------------------------#
set(CREATE_MOD_FEM_LAPLACE_PRISM_STD FALSE)        #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_PRISM_STD_QUAD FALSE)   #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_HYBRID_STD FALSE)       #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_HYBRID_STD_QUAD FALSE)  #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD FALSE)      #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_STD_QUAD FALSE) #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_PRISM_DG FALSE)         #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_HYBRID_DG FALSE)        #Set TRUE or FALSE
set(CREATE_MOD_FEM_LAPLACE_PRISM2D_DG FALSE)       #Set TRUE or FALSE

# ----------------------------- CONV_DIFF ------------------------------#
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD FALSE)        #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_STD_QUAD FALSE)   #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD FALSE)       #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_STD_QUAD FALSE)  #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD FALSE)      #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_STD_QUAD FALSE) #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_PRISM_DG FALSE)         #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_HYBRID_DG FALSE)        #Set TRUE or FALSE
set(CREATE_MOD_FEM_CONV_DIFF_PRISM2D_DG FALSE)       #Set TRUE or FALSE

# -------------------------------- HEAT --------------------------------#
set(CREATE_MOD_FEM_HEAT_PRISM_STD FALSE)            #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_PRISM_STD_QUAD FALSE)       #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD FALSE)          #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_PRISM2D_STD_QUAD FALSE)     #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_HYBRID_STD FALSE)           #Set TRUE or FALSE
set(CREATE_MOD_FEM_HEAT_HYBRID_STD_QUAD FALSE)      #Set TRUE or FALSE

# ----------------------------- NS_SUPG --------------------------------#
set(CREATE_MOD_FEM_NS_SUPG_PRISM_STD FALSE)     #Set TRUE or FALSE
set(CREATE_MOD_FEM_NS_SUPG_HYBRID_STD FALSE)    #Set TRUE or FALSE
set(CREATE_MOD_FEM_NS_SUPG_PRISM2D_STD FALSE)   #Set TRUE or FALSE
set(CREATE_MOD_FEM_NS_SUPG_REMESH_STD FALSE)    #Set TRUE or FALSE

# -------------------------- NS_SUPG_HEAT ------------------------------#
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM_STD FALSE)    #Set TRUE or FALSE
set(CREATE_MOD_FEM_NS_SUPG_HEAT_HYBRID_STD FALSE)   #Set TRUE or FALSE
set(CREATE_MOD_FEM_NS_SUPG_HEAT_PRISM2D_STD FALSE)  #Set TRUE or FALSE

# ------------------------ NS_SUPG_HEAT_ALE ----------------------------#
set(CREATE_MOD_FEM_NS_SUPG_ALE_REMESH_STD FALSE)    #Set TRUE or FALSE

# ------------------------ NS_SUPG_HEAT_VOF ----------------------------#
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_PRISM_STD FALSE)    #Set TRUE or FALSE
set(CREATE_MOD_FEM_NS_SUPG_HEAT_VOF_HYBRID_STD FALSE)   #Set TRUE or FALSE

# ------------------------------ ELAST ---------------------------------#
set(CREATE_MOD_FEM_ELAST_PRISM_STD FALSE)        #Set TRUE or FALSE
set(CREATE_MOD_FEM_ELAST_HYBRID_STD FALSE)       #Set TRUE or FALSE
set(CREATE_MOD_FEM_ELAST_PRISM2D_STD FALSE)      #Set TRUE or FALSE

# ------------------------- MOD_FEM_VIWER ------------------------------#
set(CREATE_MOD_FEM_VIEWER FALSE)     #Set TRUE or FALSE
set(MOD_FEM_VIEWER_USE_AS_LIB FALSE) #Set TRUE or FALSE
set(MOD_FEM_VIEWER_GUI_LIN "WX") #Set 'WX' gui support 

