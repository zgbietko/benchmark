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
	02.2013 - Krzysztof Banas, initial version


*************************************************************************/

#ifndef TMH_MTH_OPENCL_H
#define TMH_MTH_OPENCL_H

#include<stdlib.h>
#include<stdio.h>

#include <CL/cl.h>

/* Constants */

#define TMC_OCL_MAX_NUM_KERNELS 10

#define TMC_OCL_ALL_PLATFORMS -1 // for future use

#define TMC_OCL_ALL_DEVICES -1 // for future use
#define TMC_OCL_DEVICE_CPU 0 // (for OPENCL_CPU compile time switch)
#define TMC_OCL_DEVICE_GPU 1 // (for OPENCL_GPU compile time switch)
#define TMC_OCL_DEVICE_ACCELERATOR 2 // (for OPENCL_PHI compile time switch)

// index for storing numerical integration kernel in data structure
#define TMC_OCL_KERNEL_NUM_INT_INDEX  0
// place for further kernels

// numerical integration kernel versions depending on hardware
#define TMC_OCL_KERNEL_NUM_INT_GENERIC   0
#define TMC_OCL_KERNEL_NUM_INT_CPU_OPT   1
#define TMC_OCL_KERNEL_NUM_INT_GPU_OPT   2
#define TMC_OCL_KERNEL_NUM_INT_CELL_OPT  3
#define TMC_OCL_KERNEL_NUM_INT_PHI_OPT   4
#define TMC_OCL_KERNEL_NUM_INT_SEP_JAC	 5

// numerical integration kernel versions depending on algorithm
#define TMC_OCL_KERNEL_NUM_INT_DEFAULT     0 // which one is default?
#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_ONE_THREAD    1
#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_TWO_THREADS	2
#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_ONE_WORKGROUP 4

#ifdef TUNING
    extern FILE *optf,*resf;
    extern FILE *headf;  //header file only for result titles
    //#define COUNT_OPER
#endif

/* Datatypes */
typedef struct {
  //  char name[128];
  cl_device_id id;
  int context_index;
  int tmc_type;
  cl_device_type type;
  double global_mem_bytes; // in B
  double global_max_alloc; // in B
  double shared_mem_bytes; // in B
  double constant_mem_bytes; // in B
  double cache_bytes; // in B
  int cache_line_bytes; // in B
  int max_num_comp_units;
  int max_work_group_size;
  cl_command_queue command_queue;
  int number_of_kernels;
  cl_program program[TMC_OCL_MAX_NUM_KERNELS];
  cl_kernel kernel[TMC_OCL_MAX_NUM_KERNELS];
  int num_int_kernel_version; // computed as kernel_version_hw + 10*kernel_version_alg
} tmt_ocl_device_struct;

typedef struct {
  //  char name[128];
  cl_platform_id id;
  // cl_uint number_of_devices;
  int number_of_devices;
  tmt_ocl_device_struct *list_of_devices;
  cl_context list_of_contexts[3]; // always: [0]-CPU, [1]-GPU, [2]-accel
} tmt_ocl_platform_struct;

typedef struct {
  //cl_uint preferred_alignment = 16;    
  //cl_uint number_of_platforms;
  int number_of_platforms;
  tmt_ocl_platform_struct* list_of_platforms;
  int current_platform_index; // we always choose the first platform?
  int current_device_type;
} tmt_ocl_struct;

/* A single global variable */
extern tmt_ocl_struct tmv_ocl_struct;

/* Declarations of interface routines: */
/**--------------------------------------------------------
  tmr_ocl_init - to initialize OpenCL thread management
---------------------------------------------------------*/
int tmr_ocl_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output,
  int Control,   // not used
  int Monitor
);

/**--------------------------------------------------------
//  tmr_ocl_create_contexts - to create OpenCL contexts on a selected platform
---------------------------------------------------------*/
int tmr_ocl_create_contexts(
  FILE *Interactive_output, /* file or stdout to write messages */
  int Chosen_platform_id,
  int Monitor
  );

/**--------------------------------------------------------
  tmr_ocl_create_command_queues - for a selected platform and device
---------------------------------------------------------*/
int tmr_ocl_create_command_queues(
    FILE *Interactive_output, /* file or stdout to write messages */
    int Chosen_platform_index,
    int Chosen_device_type,
    int Monitor
  );

/**--------------------------------------------------------
  tmr_ocl_create_kernel - for a selected platform, device and kernel index
---------------------------------------------------------*/
int tmr_ocl_create_kernel(
  FILE *Interactive_output, /* file or stdout to write messages */
  int Platform_index,
  int Device_index,
  int Kernel_index,
  char* Kernel_name,
  const char* FileName,
  int Monitor
);

int tmr_ocl_create_kernel_generic(
  FILE *Interactive_output, /* file or stdout to write messages */
  int Platform_index,
  int Device_index,
  int Kernel_index,
  char* Kernel_name,
  const char* Kernel_file,
  int Monitor
);


/**--------------------------------------------------------
tmr_ocl_get_current_platform_index() - to return the index of current OpenCL platform
---------------------------------------------------------*/
int tmr_ocl_get_current_platform_index();

/**--------------------------------------------------------
tmr_ocl_get_current_device_type() - to return the type of current OpenCL device
(selected based on compiler switches)
---------------------------------------------------------*/
int tmr_ocl_get_current_device_type();


/**--------------------------------------------------------
  tmr_ocl_select_device 
//   returns OpenCL device index (for local data structures) or -1 if device
//   is not available (not existing or not serviced) for the specified platform
---------------------------------------------------------*/
int tmr_ocl_select_device( 
			  int Platform_index,
			  int Device_tmc_type
			   );

/**--------------------------------------------------------
  tmr_ocl_device_type - returns LOCAL device type (INTEGER)
---------------------------------------------------------*/
int tmr_ocl_device_type(
  int Platform_index,
  int Device_index
);

/**--------------------------------------------------------
  tmr_ocl_select_context - selects context for a given device
---------------------------------------------------------*/
cl_context tmr_ocl_select_context(
  int Platform_index,
  int Device_index
);

/**--------------------------------------------------------
  tmr_ocl_select_command_queue - selects command queue for a given device
---------------------------------------------------------*/
cl_command_queue tmr_ocl_select_command_queue(
  int Platform_index,
  int Device_index
);

/**--------------------------------------------------------
  tmr_ocl_select_kernel - selects kernel for a given device and kernel index
---------------------------------------------------------*/
cl_kernel tmr_ocl_select_kernel(
  int Platform_index,
  int Device_index,
  int Kernel_index
);


/**--------------------------------------------------------
  tmr_ocl_cleanup - discards created OpenCL resources
---------------------------------------------------------*/
void tmr_ocl_cleanup();

#endif
