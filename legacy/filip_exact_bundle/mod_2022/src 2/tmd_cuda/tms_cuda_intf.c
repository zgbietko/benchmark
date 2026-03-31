/****************************************************************
File tms_cuda_intf.c - implementation of OpenMP/OpenCL/CUDA multithreading
                      version for thread management

Contains definitions of interface routines:
  tmr_init_multithreading_opencl - to initialize (multi)thread management
  tmr_init_multithreading_cuda
  tmr_ocl_create_command_queues - for a selected platform and device
  tmr_ocl_create_kernel - for a selected platform, device and kernel index

  tmr_ocl_get_current_platform_index() - to return the index of current OpenCL platform
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
	10.2015 - Michał Grabarczyk, initial cuda version
	01.2016 - Filip Krużel, cuda version
****************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

#include"tmh_intf.h"

// interface of opencl implementation
#include"tmh_cuda.h"

struct __device_builtin__ cudaDeviceProp;

//define PRISM
//#define TETRA
//#define QSS
//#define SSQ
//#define SQS
/*---------------------------------------------------------
  tmr_init_multithreading - to initialize (multi)thread management
---------------------------------------------------------*/
extern int tmr_init_multithreading_cuda(
  char* Work_dir,
  int Argc,
  char **Argv,
  FILE *Interactive_input,
  FILE *Interactive_output,
  int Control,   // not used !!!
  int Monitor
 )
{

  cudaSetDeviceFlags(cudaDeviceMapHost);
  cudaDeviceSetCacheConfig( cudaFuncCachePreferL1 );
  tmr_cuda_init(Work_dir,Interactive_input,Interactive_output,Control,Monitor);

  return(1);
}

/*---------------------------------------------------------
  tmr_cuda_init - to initialize CUDA thread management
---------------------------------------------------------*/
int tmr_cuda_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output,
  int Control,   // TMC_MTH_SEQUENTIAL, TMC_MTH_OPENMP or TMC_MTH_OPENCL_GPU
  int Monitor
)
{
  // MONITOR SHOULD BE PASSED AS PARAMETER !!!!!!!!!!!!!!!!!!!!!!
  // for all operations indicate explicit info messages
  //int Monitor = TMC_PRINT_INFO + 1;

#ifdef TIME_TEST
  double t_begin, t_end;
  t_begin = time_clock();
#endif

  int err = tmr_cuda_create_content(Interactive_output);


#ifdef TIME_TEST
  t_end = time_clock();
  fprintf(Interactive_output,"EXECUTION TIME: creating cuda content for devices: %lf\n", t_end-t_begin);
#endif


  /*-------------- KERNEL CREATION PHASE--------------------*/

  //int device_tmc_type=Control;
  //int kernel_index = TMC_OCL_KERNEL_NUM_INT_GENERIC;
  int device_index = tmr_cuda_select_device();

#ifdef TIME_TEST
  t_begin = time_clock();
#endif


  if(Monitor>TMC_PRINT_INFO)
  {
      fprintf(Interactive_output,
	      "\nChoosing CUDA device index %d\n",
	      device_index);
  }


    // HARDCODED DEFAULT
    tmv_cuda_struct.list_of_devices[device_index].num_int_kernel_version = 0;
    int kernel_version_hw = 0;
    int kernel_version_alg = 0;

#ifdef TUNING
    tmr_cuda_create_kernel(Work_dir,Interactive_output,Monitor);
#endif

}

int tmr_cuda_select_device()
{
  int device_index = -1;

  // in a loop over all devices
  int idev;
  for(idev=0; idev<tmv_cuda_struct.number_of_devices; idev++){

    // TODO check device type and choose the fastest
      device_index = idev;
      break;

  }

  return(device_index);
}

/*---------------------------------------------------------
  DisplayDeviceInfo - utility local procedure
---------------------------------------------------------*/
void DisplayDeviceInfoCuda(
		       FILE *Interactive_output, /* file or stdout to write messages */
                       int deviceId)
{
    int numDevices;
    cudaGetDeviceCount(&numDevices);

    if( deviceId > numDevices || deviceId < 0)
    {
        fprintf(Interactive_output,"Device %d not found",deviceId);
        return;
    }

    //cudaGetDeviceProperties(&prop,deviceId);

    fprintf(Interactive_output, "\n Device Number: %d", deviceId);
    fprintf(Interactive_output, "\n\t Device name: %s",
            tmv_cuda_struct.list_of_devices[deviceId].name);
    fprintf(Interactive_output, "\n\t Compute capability: %d.%d",
            tmv_cuda_struct.list_of_devices[deviceId].major,
            tmv_cuda_struct.list_of_devices[deviceId].minor);
    fprintf(Interactive_output, "\n\t device global memory size (MB) = %f",
            (double) tmv_cuda_struct.list_of_devices[deviceId].global_mem_bytes/1024/1024);
    fprintf(Interactive_output, "\n\t device global max alloc size (MB) = %f",
	    (double) tmv_cuda_struct.list_of_devices[deviceId].global_max_alloc/1024/1024);
    fprintf(Interactive_output, "\n\t device shared memory size (kB) = %f",
            (double) tmv_cuda_struct.list_of_devices[deviceId].shared_mem_bytes/1024);
    fprintf(Interactive_output, "\n\t device constant memory size (kB) = %f",
            (double) tmv_cuda_struct.list_of_devices[deviceId].shared_mem_bytes/1024);
  //  fprintf(Interactive_output, "\n\t device cache memory size (kB) = 0.000000",);
  //  fprintf(Interactive_output, "\n\t device cache line size (B) = 0",);
    fprintf(Interactive_output, "\n\t device maximal number of comptme units = %d",
            tmv_cuda_struct.list_of_devices[deviceId].max_num_comp_units);
    fprintf(Interactive_output, "\n\t device maximal number of work units in work group = %d",
            tmv_cuda_struct.list_of_devices[deviceId].max_work_group_size);

}

/*---------------------------------------------------------
  tmr_cuda_create_content - fills cuda structures content
---------------------------------------------------------*/
int tmr_cuda_create_content(FILE *Interactive_output)
{
    int numDevices, deviceId;

    cudaGetDeviceCount(&numDevices);
    tmv_cuda_struct.number_of_devices = numDevices;
    tmv_cuda_struct.list_of_devices =
            (tmt_cuda_device_struct*)malloc(numDevices*sizeof(tmt_cuda_device_struct));

    fprintf(Interactive_output, "\n Number of CUDA devices: %d", numDevices);

    //Iterate trough every device and display properties
    for( deviceId = 0 ; deviceId < numDevices ; ++deviceId)
    {
        struct cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop,deviceId);

        strcpy(tmv_cuda_struct.list_of_devices[deviceId].name,prop.name);

        tmv_cuda_struct.list_of_devices[deviceId].major
                = prop.major;
        tmv_cuda_struct.list_of_devices[deviceId].minor
                = prop.minor;
        tmv_cuda_struct.list_of_devices[deviceId].id = deviceId;
        tmv_cuda_struct.list_of_devices[deviceId].global_mem_bytes
                = prop.totalGlobalMem;
        tmv_cuda_struct.list_of_devices[deviceId].global_max_alloc
                = prop.totalGlobalMem/4;
	tmv_cuda_struct.list_of_devices[deviceId].shared_mem_bytes
                = prop.sharedMemPerBlock;
        tmv_cuda_struct.list_of_devices[deviceId].constant_mem_bytes
                = prop.totalConstMem;
        tmv_cuda_struct.list_of_devices[deviceId].max_num_comp_units
                = prop.multiProcessorCount;
        tmv_cuda_struct.list_of_devices[deviceId].max_work_group_size
                = prop.maxThreadsPerBlock ;

        DisplayDeviceInfoCuda(Interactive_output,deviceId);
    }
}

/*---------------------------------------------------------
  tmr_ocl_create_kernel - for a selected platform, device and kernel index
---------------------------------------------------------*/
int tmr_cuda_create_kernel(
		char* Work_dir,
		FILE *Interactive_output, /* file or stdout to write messages */
		int Monitor
)
{

  //printf("Wlazłem ;%s\n",Work_dir);

  //cudaError_t err;
#ifdef TUNING
  //CUmodule module;
  //CUfunction function;

  resf = fopen("result.csv", "a+");
  if(!resf) {
	 printf("Could not open results file!\n");
	 exit(-1);
  }

  unsigned long line_count = 0;
  int c;

  while ( (c=fgetc(resf)) != EOF ) {
	 if ( c == '\n' )
			line_count++;
  }

  printf("Result file has %u lines\n", line_count);
  if(line_count==0)
  {
	  headf = fopen("header.csv", "w");
		  if(!headf) {
			 printf("Could not open header file!\n");
			 exit(-1);
	  }
	  system("touch num");
  }

  char buffer[200];

#ifdef PRISM
	#ifdef LAPLACE
		#ifdef QSS
		  sprintf(buffer,"nvcc -DLAPLACE -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_prism_QSS.cu -o apr_cuda_num_int_el_reference_prism_QSS_%d.ptx\n",line_count);
		#elif defined SSQ
		  sprintf(buffer,"nvcc -DLAPLACE -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_prism_SSQ.cu -o apr_cuda_num_int_el_reference_prism_SSQ_%d.ptx\n",line_count);
		#elif defined SQS
		  sprintf(buffer,"nvcc -DLAPLACE -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_prism_SQS.cu -o apr_cuda_num_int_el_reference_prism_SQS_%d.ptx\n",line_count);
		#endif
	#elif defined TEST_SCALAR
		#ifdef QSS
		  sprintf(buffer,"nvcc -DTEST_NUMINT -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_prism_QSS.cu -o apr_cuda_num_int_el_reference_prism_QSS_%d.ptx\n",line_count);
		#elif defined SSQ
		  sprintf(buffer,"nvcc -DTEST_NUMINT -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_prism_SSQ.cu -o apr_cuda_num_int_el_reference_prism_SSQ_%d.ptx\n",line_count);
		#elif defined SQS
		  sprintf(buffer,"nvcc -DTEST_NUMINT -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_prism_SQS.cu -o apr_cuda_num_int_el_reference_prism_SQS_%d.ptx\n",line_count);
		#endif
	#endif
#elif defined TETRA
	#ifdef LAPLACE
		#ifdef QSS
		  sprintf(buffer,"nvcc -DLAPLACE -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_tetra_QSS.cu -o apr_cuda_num_int_el_reference_tetra_QSS_%d.ptx\n",line_count);
		#elif defined SSQ
		  sprintf(buffer,"nvcc -DLAPLACE -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_tetra_SSQ.cu -o apr_cuda_num_int_el_reference_tetra_SSQ_%d.ptx\n",line_count);
		#elif defined SQS
		  sprintf(buffer,"nvcc -DLAPLACE -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_tetra_SQS.cu -o apr_cuda_num_int_el_reference_tetra_SQS_%d.ptx\n",line_count);
		#endif
	#elif defined TEST_SCALAR
		#ifdef QSS
		  sprintf(buffer,"nvcc -DTEST_NUMINT -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_tetra_QSS.cu -o apr_cuda_num_int_el_reference_tetra_QSS_%d.ptx\n",line_count);
		#elif defined SSQ
		  sprintf(buffer,"nvcc -DTEST_NUMINT -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_tetra_SSQ.cu -o apr_cuda_num_int_el_reference_tetra_SSQ_%d.ptx\n",line_count);
		#elif defined SQS
		  sprintf(buffer,"nvcc -DTEST_NUMINT -ptx ../../src/pdd_conv_diff/linear_solver_interface/cuda/std_lin/apr_cuda_num_int_el_reference_tetra_SQS.cu -o apr_cuda_num_int_el_reference_tetra_SQS_%d.ptx\n",line_count);
		#endif
	#endif
#endif
//  system(buffer);
#endif

  //contex!!

  //const char* module_file = (char*) "apr_cuda_num_int_el_reference_prism_QSS.ptx";
  //const char* kernel_name = (char*) "tmr_cuda_num_int_el";

//  err = cuModuleLoad(&module, module_file);
//
//  if (err != cudaSuccess)
//  {
//  	printf("Module load error : %d, %s \n", err,  cudaGetErrorString(err));
//  	exit(0);
//  }
//
//  err = cuModuleGetFunction(&function, module, kernel_name);
//
//  if (err != cudaSuccess)
//  {
//	printf("Function load error : %d, %s \n", err,  cudaGetErrorString(err));
//	exit(0);
//  }



  return(1);
}


