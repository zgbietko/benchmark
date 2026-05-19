/************************************************************************
File tmh_intf.h - generic(?) interface to thread management routines

Contains declarations of constants, types and interface routines:
  tmr_ocl_create_command_queues - for a selected platform and device
  tmr_ocl_create_kernel - for a selected platform, device and kernel index
  tmr_ocl_get_current_device_type
//   returns the current OpenCL device type specified based on compiler switches
  tmr_ocl_select_device
//   returns OpenCL device index (for local data structures) or -1 if device
//   is not available (not existing or not serviced) for the specified platform
  tmr_ocl_device_type - returns LOCAL device type (INTEGER)
  tmr_ocl_select_context - selects context for a given device
  tmr_ocl_select_command_queue - selects command queue for a given device
  tmr_ocl_select_kernel - selects kernel for a given device and kernel index
  tmr_ocl_cleanup - discards created OpenCL resources


------------------------------
History:
	10.2015 - Michał Grabarczyk, initial cuda version
	01.2016 - Filip Krużel, initial version

*************************************************************************/

#ifndef TMH_MTH_CUDA_H
#define TMH_MTH_CUDA_H

#include<stdlib.h>
#include<stdio.h>
#include<cuda.h>

#include <cuda_runtime_api.h>

/* Constants */

//#define TMC_OCL_MAX_NUM_KERNELS 10
//
//#define TMC_OCL_ALL_PLATFORMS -1 // for future use
//
//#define TMC_OCL_ALL_DEVICES -1 // for future use
//#define TMC_OCL_DEVICE_CPU 0 // (for OPENCL_CPU compile time switch)
//#define TMC_OCL_DEVICE_GPU 1 // (for OPENCL_GPU compile time switch)
//#define TMC_OCL_DEVICE_ACCELERATOR 2 // (for OPENCL_PHI compile time switch)
//
//// index for storing numerical integration kernel in data structure
#define TMC_OCL_KERNEL_NUM_INT_INDEX  0
//// place for further kernels
//
//// numerical integration kernel versions depending on hardware
//#define TMC_OCL_KERNEL_NUM_INT_GENERIC   0
//#define TMC_OCL_KERNEL_NUM_INT_CPU_OPT   1
//#define TMC_OCL_KERNEL_NUM_INT_GPU_OPT   2
//#define TMC_OCL_KERNEL_NUM_INT_CELL_OPT  3
//#define TMC_OCL_KERNEL_NUM_INT_PHI_OPT   4
//
// numerical integration kernel versions depending on algorithm
#define TMC_OCL_KERNEL_NUM_INT_DEFAULT     0 // which one is default?
#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_ONE_THREAD    1
#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_TWO_THREADS	2
#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_ONE_WORKGROUP 3

#ifdef TUNING
    FILE *resf;
    FILE *headf;  //header file only for result titles
    //#define COUNT_OPER
#endif

typedef struct {
  char name[256];
  int id;
  int major; //compute capability
  int minor;
//  int tmc_type;
  double global_mem_bytes; // in B
  double global_max_alloc; // in B
  double shared_mem_bytes; // in B
  double constant_mem_bytes; // in B
//  double cache_bytes; // in B
//  int cache_line_bytes; // in B
  int max_num_comp_units;
  int max_work_group_size;
  int number_of_kernels;
  int num_int_kernel_version; // computed as kernel_version_hw + 10*kernel_version_alg
} tmt_cuda_device_struct;

typedef struct {
  //cl_uint preferred_alignment = 16;
  //cl_uint number_of_platforms;
  int number_of_devices;
  tmt_cuda_device_struct* list_of_devices;
  int current_device_index;
} tmt_cuda_struct;

/* A single global variable */
tmt_cuda_struct tmv_cuda_struct;

/* Declarations of interface routines: */
/**--------------------------------------------------------
  tmr_cuda_init - to initialize CUDA thread management
---------------------------------------------------------*/

int tmr_cuda_init(char* Work_dir,
		  FILE *Interactive_input,
		  FILE *Interactive_output,
		  int Control,   // TMC_MTH_SEQUENTIAL, TMC_MTH_OPENMP or TMC_MTH_OPENCL_GPU
		  int Monitor);

int tmr_cuda_select_device();
int tmr_cuda_create_content(FILE *Interactive_output);

#endif
