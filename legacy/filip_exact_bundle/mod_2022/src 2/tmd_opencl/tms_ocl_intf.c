/****************************************************************
File tms_ocl_intf.c - implementation of OpenMP/OpenCL/CUDA multithreading
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
****************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

#include"tmh_intf.h"

#include <CL/cl.h>

// interface of opencl implementation
#include"tmh_ocl.h"
//FK sep_jac
//#define SEP_JAC
//
//
#ifdef TUNING
    FILE *optf,*resf;
    FILE *headf;  //header file only for result titles
    //#define COUNT_OPER
#endif

tmt_ocl_struct tmv_ocl_struct;


// DECLARATIONS OF LOCAL FUNCTIONS
void DisplayPlatformInfo(
			 FILE *Interactive_output, /* file or stdout to write messages */
			 cl_platform_id id, 
			 cl_platform_info name,
			 char* str);

void DisplayDeviceInfo(
		       FILE *Interactive_output, /* file or stdout to write messages */
		       cl_device_id id, 
		       cl_device_info name,
		       char* str);

/*---------------------------------------------------------
  tmr_init_multithreading_opencl - to initialize (multi)thread management
---------------------------------------------------------*/
extern int tmr_init_multithreading_opencl(
  char* Work_dir,
  int Argc, 
  char **Argv,
  FILE *Interactive_input,
  FILE *Interactive_output,
  int Control,   // not used !!!
  int Monitor
 )
{

  //Monitor = TMC_PRINT_INFO + 1;

  int ocl_device = -1;
#ifdef OPENCL_PHI
  ocl_device = TMC_OCL_DEVICE_ACCELERATOR; // should be substituted in some way
#endif
#ifdef OPENCL_GPU
  ocl_device = TMC_OCL_DEVICE_GPU;
#endif
#ifdef OPENCL_CPU
  ocl_device = TMC_OCL_DEVICE_CPU; // should be substituted in some way
#endif
#ifdef OPENCL_HSA
  ocl_device = TMC_OCL_DEVICE_GPU;
#endif


  tmr_ocl_init(Work_dir,Interactive_input,Interactive_output,ocl_device,Monitor);


  return(1);
}

/*---------------------------------------------------------
  tmr_ocl_init - to initialize OpenCL thread management
---------------------------------------------------------*/
int tmr_ocl_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output,
  int Control,  // OpenCL device type
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
  
  // Create OpenCL contexts on all available platforms
  // contexts are stored in table with indices: 0-CPU, 1-GPU, 2-ACCELERATOR
  // table entry is NULL if there is no such context for a given platform
  int number_of_platforms = tmr_ocl_create_contexts(Interactive_output,
						    TMC_OCL_ALL_PLATFORMS, Monitor);
  
#ifdef DEBUG_TMM  
  if(number_of_platforms>1){
    fprintf(Interactive_output,
	    "\nMore than one platform in a system! Check whether it's OK with the code.\n");
  }
#endif

  if(number_of_platforms>1){
    if(Interactive_input == stdin){
      fprintf(Interactive_output,
	      "\nSelect platform ID (index): ");
      fscanf(Interactive_input, "%d", &tmv_ocl_struct.current_platform_index);
      //tmv_ocl_struct.current_platform_index = 0;

    }
    else{ // if input from file - first platform selected

//here change between intel or nvidia

      tmv_ocl_struct.current_platform_index = 1;

    }

  }
  else{ // if only one platform - first platform selected
    
    tmv_ocl_struct.current_platform_index = 0;
    
  }
  
  fprintf(Interactive_output,
	  "\nSelected platform ID (index): %d", 
	  tmv_ocl_struct.current_platform_index);


#ifdef TIME_TEST
  t_end = time_clock();
  fprintf(Interactive_output,"EXECUTION TIME: creating contexts: %lf\n", t_end-t_begin);
#endif
  
#ifdef TIME_TEST
  t_begin = time_clock();
#endif
  

  int platform_index = tmv_ocl_struct.current_platform_index;
  // or better
  // int platform_index = tmr_ocl_get_current_platform_index();

  // create command queues on all devices 
  tmr_ocl_create_command_queues(Interactive_output, platform_index, 
				TMC_OCL_ALL_DEVICES, Monitor);
  
#ifdef TIME_TEST
  t_end = time_clock();
  fprintf(Interactive_output,"EXECUTION TIME: creating command queues: %lf\n", 
	  t_end-t_begin);
#endif
  
  //cl_platform_id platform = tmv_ocl_struct.list_of_platforms[platform_index].id;
  tmt_ocl_platform_struct platform_struct = 
    tmv_ocl_struct.list_of_platforms[platform_index];
  
/*-------------- KERNEL CREATION PHASE--------------------*/

  int device_tmc_type=Control;
  tmv_ocl_struct.current_device_type=Control;
  int device_index;
  int kernel_index;

#ifdef TIME_TEST
  t_begin = time_clock();
#endif


  if(Monitor>TMC_PRINT_INFO){
    if(device_tmc_type==0){
      fprintf(Interactive_output, 
	      "\nSelecting OpenCL device type %d (CPU) for platform %d\n", 
	      device_tmc_type, platform_index);
    }
    else if(device_tmc_type==1){
      fprintf(Interactive_output, 
	      "\nSelecting OpenCL device type %d (GPU) for platform %d\n", 
	      device_tmc_type, platform_index);
    }
    else if(device_tmc_type==2){
      fprintf(Interactive_output, 
	      "\nSelecting OpenCL device type %d (ACCELERATOR) for platform %d\n", 
	      device_tmc_type, platform_index);
    }
    else {
      fprintf(Interactive_output, 
	      "\nUnknown OpenCL device type %d for platform %d. Exiting!\n", 
	      device_tmc_type, platform_index);
      exit(-1);
    }
  }

  // choose device_index
  device_index = tmr_ocl_select_device(platform_index, device_tmc_type);

  // if required context exists
  if(device_index >= 0){

    // create the kernel for numerical integration
    kernel_index = TMC_OCL_KERNEL_NUM_INT_INDEX;

    if(Monitor>TMC_PRINT_INFO){
      fprintf(Interactive_output, 
	      "\nCreating OpenCL kernel %d for device type %d and device index %d\n", 
	      kernel_index, device_tmc_type, device_index);
    }

    // !!!!!!!!!!!!!!!!!!!!!!????????????????!!!!!!!!!!!!!!!!!!!!!!!
    // APART FROM DIFFERENT KERNELS FOR DIFFERENT HARDWARE, THERE ARE
    // DIFFERENT KERNEL VERSIONS FOR DIFFERENT ALGORITHMS;
    // FROM tmd_ocl/tmh_ocl.h:
// numerical integration kernel versions depending on hardware
//#define TMC_OCL_KERNEL_NUM_INT_GENERIC   0
//#define TMC_OCL_KERNEL_NUM_INT_CPU_OPT   1
//#define TMC_OCL_KERNEL_NUM_INT_GPU_OPT   2
//#define TMC_OCL_KERNEL_NUM_INT_CELL_OPT  3

// numerical integration kernel versions depending on algorithm
//#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_ONE_THREAD    1
//#define TMC_OCL_KERNEL_NUM_INT_ONE_EL_ONE_WORKGROUP 2

    // HARDCODED DEFAULT
    platform_struct.list_of_devices[device_index].num_int_kernel_version = 0;
    int kernel_version_hw = 0;
    int kernel_version_alg = 0; 

  // BELOW IT IS READ FROM INPUT:
  // IT SHOULD BE BASED ON INPUT PARAMETERS FOR OPENCL !!!!!!!!!!!!!!!
    /* if(Interactive_input == stdin){ */

    /*   printf("Select kernel version (kernel_version_hw + 10*kernel_version_alg): \n"); */
    /*   printf("see tmd_ocl/tmh_ocl.h for details\n"); */
    /*   // HARDCODED DEFAULT */
    /*   int i = 0; */
    /*   fscanf(Interactive_input, "%d", &i); */
    /*   platform_struct.list_of_devices[device_index].num_int_kernel_version = i; */
    /*   int kernel_version_hw = i%10; */
    /*   int kernel_version_alg = i/10;  */

    /* } */

    char arg[255]={0};
    sprintf(arg,"%s/tmr_ocl_num_int_el.cl", Work_dir);

  // BASED ON INPUT PARAMETERS SELECT OPENCL KERNEL FOR NUMERICAL INTEGRATION !!!!!!!
    if(kernel_index == TMC_OCL_KERNEL_NUM_INT_GENERIC){
      tmr_ocl_create_kernel(Interactive_output,platform_index,device_index,kernel_index,
	// kernel name:         , file:
       //  "tmr_ocl_num_int_el", "tmr_ocl_num_int_el.cl", Monitor);
        "tmr_ocl_num_int_el", arg, Monitor);
      //printf("first!\n");
    }
    /* else if(kernel_index == TMC_OCL_KERNEL_NUM_INT_CPU_OPT){ */

    /*   tmr_ocl_create_kernel(Interactive_output,platform_index,device_index,kernel_index, */
    /* 	   // kernel name:         , file: */
    /*     "tmr_ocl_num_int_el", "tmr_ocl_num_int_el_cpu.cl", Monitor); */
    /* } */
    /* else if(kernel_index == TMC_OCL_KERNEL_NUM_INT_GPU_OPT){ */

    /*   tmr_ocl_create_kernel(Interactive_output,platform_index,device_index,kernel_index, */
    /* 	   // kernel name:         , file: */
    /*     "tmr_ocl_num_int_el", "tmr_ocl_num_int_el_gpu.cl", Monitor); */
    /* } */
    /* else if(kernel_index == TMC_OCL_KERNEL_NUM_INT_CELL_OPT){ */

    /*   tmr_ocl_create_kernel(Interactive_output,platform_index,device_index,kernel_index, */
    /* 	   // kernel name:         , file: */
    /*     "tmr_ocl_num_int_el", "tmr_ocl_num_int_el_cell.cl", Monitor); */
    /* } */
//FK sep_jac
#ifdef SEP_JAC

    // create kernel for separate jacobian data computing
     kernel_index = 5;

     if(Monitor>TMC_PRINT_INFO){
       fprintf(Interactive_output,
              "\nCreating OpenCL kernel %d for device type %d and device index %d\n",
              kernel_index, device_tmc_type, device_index);
     }

     // BASED ON INPUT PARAMETERS SELECT OPENCL KERNEL FOR Compute Jacobian data !!!!!!!
     if(kernel_index == 5){
       // specific for finalize the crs creation
    	 tmr_ocl_create_kernel_generic(Interactive_output,platform_index,device_index,kernel_index,
 				    // kernel name:         , file:
 				    //  "tmr_ocl_num_int_el", "tmr_ocl_num_int_el.cl", Monitor);
 				    "tmr_ocl_prepare_jacobian_data", arg, Monitor);
    	 //printf("second!\n");
     }


#endif



#ifdef TIME_TEST
    t_end = time_clock();
    fprintf(Interactive_output,"EXECUTION TIME: creating OpenCL kernel for integration: %lf\n", t_end-t_begin);
#endif
  
  }
  else{
      fprintf(Interactive_output,"Requested device not supported for this platform.\n");
      exit(-1);
  }

  return(1);
}


/*---------------------------------------------------------
  DisplayPlatformInfo - utility local procedure
---------------------------------------------------------*/
void DisplayPlatformInfo(
			 FILE *Interactive_output, /* file or stdout to write messages */
			 cl_platform_id id, 
			 cl_platform_info name,
			 char* str)
{
  cl_int retval;
  size_t paramValueSize;
  
  retval = clGetPlatformInfo(
			     id,
			     name,
			     0,
			     NULL,
			     &paramValueSize);
  if (retval != CL_SUCCESS){
    fprintf(Interactive_output,"Failed to find OpenCL platform %s.\n", str);
    return;
  }
  
  char * info = (char *)malloc(sizeof(char) * paramValueSize);
  retval = clGetPlatformInfo(
			     id,
			     name,
			     paramValueSize,
			     info,
			     NULL);
  if (retval != CL_SUCCESS)  {
    fprintf(Interactive_output,"Failed to find OpenCL platform %s.\n", str);
    return;
  }
  
  fprintf(Interactive_output,"\t%s:\t%s\n", str, info );
  free(info); 
}

/*---------------------------------------------------------
  DisplayDeviceInfo - utility local procedure
---------------------------------------------------------*/
void DisplayDeviceInfo(
		       FILE *Interactive_output, /* file or stdout to write messages */
		       cl_device_id id, 
		       cl_device_info name,
		       char* str)
{
  cl_int retval;
  size_t paramValueSize;
  
  retval = clGetDeviceInfo(
			   id,
			   name,
			   0,
			   NULL,
			   &paramValueSize);
  if (retval != CL_SUCCESS) {
    fprintf(Interactive_output,"Failed to find OpenCL device info %s.\n", str);
    return;
  }
  
  char * info = (char *)malloc(sizeof(char) * paramValueSize);
  retval = clGetDeviceInfo(
			   id,
			   name,
			   paramValueSize,
			   info,
			   NULL);
  
  if (retval != CL_SUCCESS) {
    fprintf(Interactive_output,"Failed to find OpenCL device info %s.\n", str);
    return;
  }

  fprintf(Interactive_output,"\t\t%s:\t%s\n", str, info );
  free(info);
};

/*---------------------------------------------------------
//  tmr_ocl_create_contexts - to create OpenCL contexts on a selected platform
---------------------------------------------------------*/
int tmr_ocl_create_contexts(
  FILE *Interactive_output, /* file or stdout to write messages */
  int Platform_id_in,
  int Monitor
  )
{
  cl_int retval;
  cl_uint numPlatforms;
  cl_platform_id * platformIds;
  cl_context context = NULL;
  cl_uint iplat, jdev, k;
  
  // First, query the total number of platforms
  retval = clGetPlatformIDs(0, (cl_platform_id *) NULL, &numPlatforms);

  // allocate memory for local platform structures
  tmv_ocl_struct.number_of_platforms = numPlatforms;
  tmv_ocl_struct.list_of_platforms = 
    (tmt_ocl_platform_struct *) malloc( sizeof(tmt_ocl_platform_struct)
					* numPlatforms);

  // Next, allocate memory for the installed platforms, and qeury 
  // to get the list.
  platformIds = (cl_platform_id *)malloc(sizeof(cl_platform_id) * numPlatforms);
  retval = clGetPlatformIDs(numPlatforms, platformIds, NULL);

  if(Monitor>=TMC_PRINT_INFO){
    fprintf(Interactive_output,"\nNumber of OpenCL platforms: \t%d\n", numPlatforms); 
  }

  // Iterate through the list of platforms displaying associated information
  for (iplat = 0; iplat < numPlatforms; iplat++) {

    if(Monitor>TMC_PRINT_INFO){
      fprintf(Interactive_output,"\n");
      fprintf(Interactive_output,"Platform %d:\n", iplat); 
    }

    tmv_ocl_struct.list_of_platforms[iplat].id = platformIds[iplat];
    //clGetPlatformInfo(platformIds[iplat], CL_PLATFORM_NAME, size_of_name???, 
    //		      tmv_ocl_struct.list_of_platforms[iplat].name, (size_t *) NULL);

    if(Monitor>TMC_PRINT_INFO){

      // First we display information associated with the platform
      DisplayPlatformInfo(Interactive_output,
			platformIds[iplat], 
			CL_PLATFORM_NAME, 
			"CL_PLATFORM_NAME");
      DisplayPlatformInfo(Interactive_output,
			platformIds[iplat], 
			CL_PLATFORM_PROFILE, 
			"CL_PLATFORM_PROFILE");
      DisplayPlatformInfo(Interactive_output,
			platformIds[iplat], 
			CL_PLATFORM_VERSION, 
			"CL_PLATFORM_VERSION");
      DisplayPlatformInfo(Interactive_output,
			platformIds[iplat], 
			CL_PLATFORM_VENDOR, 
			"CL_PLATFORM_VENDOR");
    }

    // Now query the set of devices associated with the platform
    cl_uint numDevices;
    retval = clGetDeviceIDs(
			    platformIds[iplat],
			    CL_DEVICE_TYPE_ALL,
			    0,
			    NULL,
			    &numDevices);


    tmv_ocl_struct.list_of_platforms[iplat].number_of_devices = numDevices;
    tmv_ocl_struct.list_of_platforms[iplat].list_of_devices = 
      (tmt_ocl_device_struct *) malloc( sizeof(tmt_ocl_device_struct) 
					* numDevices);

    cl_device_id * devices = 
      (cl_device_id *) malloc (sizeof(cl_device_id) * numDevices);

    retval = clGetDeviceIDs(
			    platformIds[iplat],
			    CL_DEVICE_TYPE_ALL,
			    numDevices,
			    devices,
			    NULL);
    
    if(Monitor>=TMC_PRINT_INFO){
      fprintf(Interactive_output,"\n\tNumber of devices: \t%d\n", numDevices); 
    }
    // Iterate through each device, displaying associated information
    for (jdev = 0; jdev < numDevices; jdev++)
      {
	
	if(Monitor>TMC_PRINT_INFO){
	  fprintf(Interactive_output,"\tDevice %d:\n", jdev); 
	}
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].id = 
	  devices[jdev];
	clGetDeviceInfo(devices[jdev], CL_DEVICE_TYPE, sizeof(cl_device_type), 
	  &tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].type, NULL);

	if(tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].type == CL_DEVICE_TYPE_CPU){
	  tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].tmc_type = TMC_OCL_DEVICE_CPU;
	}
	if(tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].type == CL_DEVICE_TYPE_GPU){
	  tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].tmc_type = TMC_OCL_DEVICE_GPU;
	}   
	if(tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].type == CL_DEVICE_TYPE_ACCELERATOR){
	  tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].tmc_type = TMC_OCL_DEVICE_ACCELERATOR;
	}  

	cl_ulong mem_size_ulong = 0;
	int err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_GLOBAL_MEM_SIZE, 
			sizeof(cl_ulong), &mem_size_ulong, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].global_mem_bytes = 
	  (double)mem_size_ulong;

	err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_MAX_MEM_ALLOC_SIZE, 
			sizeof(cl_ulong), &mem_size_ulong, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].global_max_alloc= 
	  (double)mem_size_ulong;

	err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_LOCAL_MEM_SIZE,
			sizeof(cl_ulong), &mem_size_ulong, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].shared_mem_bytes = 
	  (double)mem_size_ulong;

	err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,
			sizeof(cl_ulong), &mem_size_ulong, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].cache_bytes = 
	  (double)mem_size_ulong;

	err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,
			sizeof(cl_ulong), &mem_size_ulong, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].constant_mem_bytes = 
	  (double)mem_size_ulong;

	cl_uint cache_line_size = 0;
	err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,
			sizeof(cl_uint), &cache_line_size, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].cache_line_bytes = 
	  (int) cache_line_size;

	cl_uint max_num_comp_units = 0;
	err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_MAX_COMPUTE_UNITS,
			sizeof(cl_uint), &max_num_comp_units, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].max_num_comp_units = 
	  (int) max_num_comp_units;

	size_t max_work_group_size =0;
	err_num = clGetDeviceInfo(devices[jdev], CL_DEVICE_MAX_WORK_GROUP_SIZE,
				  sizeof(size_t), &max_work_group_size, NULL);
	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].max_work_group_size = 
	  (int) max_work_group_size;

	// possible further inquires:
	//CL_DEVICE_MAX_WORK_GROUP_SIZE, 
	//CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, CL_DEVICE_MAX_WORK_ITEM_SIZES
	//CL_DEVICE_MAX_CONSTANT_ARGS
	//CL_DEVICE_MAX_PARAMETER_SIZE
	//CL_DEVICE_PREFERRED_VECTOR_WIDTH_ - char, int, float, double etc.
	//CL_DEVICE_MEM_BASE_ADDR_ALIGN, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE

	tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].command_queue = 0;
	
	for(k=0;k<TMC_OCL_MAX_NUM_KERNELS;k++){
	  tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].program[k]=0;
	  tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].kernel[k]=0;
	}
	      
	//clGetDeviceInfo(devices[jdev], CL_DEVICE_NAME, sizeof(device_name?), 
	//&tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].name, NULL);

	if(Monitor>TMC_PRINT_INFO){

	  DisplayDeviceInfo(Interactive_output,
			  devices[jdev], 
			  CL_DEVICE_NAME, 
			  "CL_DEVICE_NAME");
	
	  DisplayDeviceInfo(Interactive_output,
			  devices[jdev], 
			  CL_DEVICE_VENDOR, 
			  "CL_DEVICE_VENDOR");
	
	  DisplayDeviceInfo(Interactive_output,
			  devices[jdev], 
			  CL_DEVICE_VERSION, 
			  "CL_DEVICE_VERSION");
	  fprintf(Interactive_output,"\t\tdevice global memory size (MB) = %lf\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].global_mem_bytes/1024/1024);
	  fprintf(Interactive_output,"\t\tdevice global max alloc size (MB) = %lf\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].global_max_alloc/1024/1024);
	  fprintf(Interactive_output,"\t\tdevice local memory size (kB) = %lf\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].shared_mem_bytes/1024);
	  fprintf(Interactive_output,"\t\tdevice constant memory size (kB) = %lf\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].constant_mem_bytes/1024);
	  fprintf(Interactive_output,"\t\tdevice cache memory size (kB) = %lf\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].cache_bytes/1024);
	  fprintf(Interactive_output,"\t\tdevice cache line size (B) = %d\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].cache_line_bytes);
	  fprintf(Interactive_output,"\t\tdevice maximal number of comptme units = %d\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].max_num_comp_units);
	  fprintf(Interactive_output,"\t\tdevice maximal number of work units in work group = %d\n",
		 tmv_ocl_struct.list_of_platforms[iplat].list_of_devices[jdev].max_work_group_size);
	  
	  fprintf(Interactive_output,"\n");
	}
      }

    free(devices);
  
    // Next, create OpenCL contexts on platforms
    cl_context_properties contextProperties[] = {
      CL_CONTEXT_PLATFORM,
      (cl_context_properties)platformIds[iplat],
      0
    };

    if(Platform_id_in == TMC_OCL_ALL_PLATFORMS || Platform_id_in == iplat){

      if(Monitor>TMC_PRINT_INFO){
	fprintf(Interactive_output,"\tCreating CPU context (index=0) on platform %d\n", iplat);
      }

      tmv_ocl_struct.list_of_platforms[iplat].list_of_contexts[0] = 
	clCreateContextFromType(contextProperties, 
				CL_DEVICE_TYPE_CPU, NULL, NULL, &retval);

      if(Monitor>=TMC_PRINT_INFO && retval != CL_SUCCESS){
	fprintf(Interactive_output,"\tCould not create CPU context on platform %d\n", iplat);
      }

      if(Monitor>TMC_PRINT_INFO){
	fprintf(Interactive_output,"\tCreating GPU context (index=1) on platform %d\n", iplat);
      }

      tmv_ocl_struct.list_of_platforms[iplat].list_of_contexts[1] = 
	clCreateContextFromType(contextProperties, 
				CL_DEVICE_TYPE_GPU, NULL, NULL, &retval);

      if(Monitor>=TMC_PRINT_INFO && retval != CL_SUCCESS){
	fprintf(Interactive_output,"\tCould not create GPU context on platform %d\n", iplat);
      }

      if(Monitor>TMC_PRINT_INFO){
	fprintf(Interactive_output,"\tCreating ACCELERATOR context (index=2) on platform %d\n", iplat);
      }

      tmv_ocl_struct.list_of_platforms[iplat].list_of_contexts[2] = 
	clCreateContextFromType(contextProperties, 
				CL_DEVICE_TYPE_ACCELERATOR, NULL, NULL, &retval);
      if(Monitor>=TMC_PRINT_INFO && retval != CL_SUCCESS){
	fprintf(Interactive_output,"\tCould not create ACCELERATOR context on platform %d\n", iplat);
      }

    }
  }
  
  free(platformIds);
  return numPlatforms;
}


/*---------------------------------------------------------
  tmr_ocl_create_command_queues - for a selected platform and device
---------------------------------------------------------*/
int tmr_ocl_create_command_queues(
    FILE *Interactive_output, /* file or stdout to write messages */
    int Platform_index,
    int Device_type,
    int Monitor
  )
{

  // in a loop over all platforms
  int platform_index;
  for(platform_index=0; 
      platform_index<tmv_ocl_struct.number_of_platforms; 
      platform_index++){

    // shortctm for global platform structure
    tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[platform_index];
    
    // if creating contexts for all platforms or just this one 
    if(Platform_index == TMC_OCL_ALL_PLATFORMS || 
       Platform_index == platform_index){
      
      // in a loop over all devices
      int idev;
      for(idev=0; 
	  idev<platform_struct.number_of_devices;
	  idev++){
	
	// variable for storing device_id
	cl_device_id device = 0;

	// select context for the device (CPU context for CPU device, etc.)
	// (contexts are already created!,
	// icon is just the index in the platform structure)	
	int icon;
	
	// check whether this is a CPU device - then context is no 0
	if(platform_struct.list_of_devices[idev].type ==
	   CL_DEVICE_TYPE_CPU){
	  
	  if(Device_type == TMC_OCL_ALL_DEVICES || 
	     Device_type == TMC_OCL_DEVICE_CPU){
	    
	    device = platform_struct.list_of_devices[idev].id;
	    platform_struct.list_of_devices[idev].tmc_type = TMC_OCL_DEVICE_CPU;
	    icon = 0;
	    
	  }
	  else{
	    
	    device = NULL;
	    
	  }
	  
	}
	// check whether this is a GPU device - then context is no 1
	else if(platform_struct.list_of_devices[idev].type ==
		CL_DEVICE_TYPE_GPU){
	  
	  if(Device_type == TMC_OCL_ALL_DEVICES || 
	     Device_type == TMC_OCL_DEVICE_GPU){
	    
	    device = platform_struct.list_of_devices[idev].id;
	    platform_struct.list_of_devices[idev].tmc_type = TMC_OCL_DEVICE_GPU;
	    icon = 1;
	    
	  }
	  else{
	    
	    device = NULL;
	    
	  }
	  
	}
	// check whether this is an ACCELERATOR device - then context is no 2
	else if(platform_struct.list_of_devices[idev].type ==
		CL_DEVICE_TYPE_ACCELERATOR){
	  
	  if(Device_type == TMC_OCL_ALL_DEVICES || 
	     Device_type == TMC_OCL_DEVICE_ACCELERATOR){
	    
	    device = platform_struct.list_of_devices[idev].id;
	    platform_struct.list_of_devices[idev].tmc_type = TMC_OCL_DEVICE_ACCELERATOR;
	    icon = 2;
	    
	  }
	  else{
	    
	    device = NULL;
	    
	  }
	  
	}
	
	if(device != NULL){
	  
	  // choose OpenCL context selected for a device
	  cl_context context = platform_struct.list_of_contexts[icon];
	  platform_struct.list_of_devices[idev].context_index = icon;
	  
	  // if context exist
	  if(context != NULL){
	    
	    if(Monitor>TMC_PRINT_INFO){
	      if(platform_struct.list_of_devices[idev].tmc_type == TMC_OCL_DEVICE_CPU){
		fprintf(Interactive_output,"\nCreating command queue for CPU context %d, device index %d, platform %d\n",
		       icon, idev, platform_index);
	      }
	      if(platform_struct.list_of_devices[idev].tmc_type == TMC_OCL_DEVICE_GPU){
		fprintf(Interactive_output,"\nCreating command queue for GPU context %d, device index %d, platform %d\n",
		       icon, idev, platform_index);
	      }
	      if(platform_struct.list_of_devices[idev].tmc_type == TMC_OCL_DEVICE_ACCELERATOR){
		fprintf(Interactive_output,"\nCreating command queue for ACCELERATOR context %d, device index %d, platform %d\n",
		       icon, idev, platform_index);
	      }
	    }

	    // Create a command-queue on the device for the context
	    cl_command_queue_properties prop = 0;
	    prop |= CL_QUEUE_PROFILING_ENABLE;
	    platform_struct.list_of_devices[idev].command_queue = 
	      clCreateCommandQueue(context, device, prop, NULL);
	    if (platform_struct.list_of_devices[idev].command_queue == NULL)
	      {
		fprintf(Interactive_output,"Failed to create command queue for context %d, device %d, platform %d\n",
		       icon, idev, platform_index);
		exit(-1);
	      }
	    
	  } // end if context exist for a given device
	  
	} // end if device is of specified type
	
      } // end loop over devices
      
    } // end if platform is of specified type
    
  } // end loop over platforms
  
  return(1);
}

/*---------------------------------------------------------
// auxiliary procedure for reading source files
----------------------------------------------------------*/
char* tmr_ocl_readSource(
  FILE *Interactive_output, /* file or stdout to write messages */
  const char* kernelPath
			 ) {

   cl_int status;
   FILE *fp;
   char *source;
   long int size;

   fp = fopen(kernelPath, "rb");
   if(!fp) {
      fprintf(Interactive_output,"Could not open kernel file\n");
      exit(-1);
   }
   status = fseek(fp, 0, SEEK_END);
   if(status != 0) {
      fprintf(Interactive_output,"Error seeking to end of file\n");
      exit(-1);
   }
   size = ftell(fp);
   if(size < 0) {
      fprintf(Interactive_output,"Error getting file position\n");
      exit(-1);
   }

   rewind(fp);

   source = (char *)malloc(size + 1);

   int i;
   for (i = 0; i < size+1; i++) {
      source[i]='\0';
   }

   if(source == NULL) {
      fprintf(Interactive_output,"Error allocating space for the kernel source\n");
      exit(-1);
   }

   fread(source, 1, size, fp);
   source[size] = '\0';

   return source;
}

/*---------------------------------------------------------
// procedure for vendor checking needed for kernel creation
 * 1 - NVIDIA
 * 2 - INTEL
 * 3 - AMD
----------------------------------------------------------*/

int tmr_ocl_check_vendor()
{
    cl_platform_id *platformIds;
    size_t paramValueSize;

    platformIds = (cl_platform_id *)malloc(sizeof(cl_platform_id));
    clGetPlatformIDs(tmv_ocl_struct.number_of_platforms, platformIds, NULL);



    clGetPlatformInfo(
    		platformIds[tmv_ocl_struct.current_platform_index],
    		CL_PLATFORM_VENDOR,
    		0,
    		NULL,
    		&paramValueSize);


    printf("paramvalueSize=%u\n",paramValueSize);

    char * info = (char *)malloc(sizeof(char) * paramValueSize*100);
    clGetPlatformInfo(
    		 platformIds[tmv_ocl_struct.current_platform_index],
    		 CL_PLATFORM_VENDOR,
    		 paramValueSize,
    		 info,
    		 NULL);
    //fprintf(Interactive_output,"%s\n", info );

   int test;
   test = strcmp("NVIDIA Corporation",info);
   if (test==0)
	   return 1;
   test = strcmp("Intel(R) Corporation",info);
   if (test==0)
 	   return 2;
   test = strcmp("Advanced Micro Devices, Inc.",info);
   if (test==0)
	   return 3;

   return 2;
}

void tmr_ocl_device_name(char **info)
{

	  cl_uint jdev, numDevices, cur_id;
	  size_t paramValueSize;

	  cur_id=tmv_ocl_struct.current_platform_index;
	  numDevices=tmv_ocl_struct.list_of_platforms[cur_id].number_of_devices;

	  // Iterate through each device, displaying associated information
	  for (jdev = 0; jdev < numDevices; jdev++)
	  {
		  if(tmv_ocl_struct.list_of_platforms[cur_id].list_of_devices[jdev].tmc_type==TMC_OCL_DEVICE_GPU)
		  {
			  clGetDeviceInfo(tmv_ocl_struct.list_of_platforms[cur_id].list_of_devices[jdev].id, CL_DEVICE_NAME,
			  			  				0, NULL, &paramValueSize);
			  *info = (char *)malloc(sizeof(char) * paramValueSize);

			  clGetDeviceInfo(tmv_ocl_struct.list_of_platforms[cur_id].list_of_devices[jdev].id, CL_DEVICE_NAME,
					  	  paramValueSize, *info, NULL);
		  }
	  }

	  //printf("INFO=%s!!!!\n",*info);

}

int wc(char* file_path, char* word){
    FILE *fp;
    int count = 0;
    int ch, len;

    if(NULL==(fp=fopen(file_path, "r")))
        return -1;
    len = strlen(word);
    for(;;){
        int i;
        if(EOF==(ch=fgetc(fp))) break;
        if((char)ch != *word) continue;
        for(i=1;i<len;++i){
            if(EOF==(ch = fgetc(fp))) goto end;
            if((char)ch != word[i]){
                fseek(fp, 1-i, SEEK_CUR);
                goto next;
            }
        }
        ++count;
        next: ;
    }
    end:
    fclose(fp);
    return count;
}

/*---------------------------------------------------------
  tmr_ocl_create_kernel - for a selected platform, device and kernel index
---------------------------------------------------------*/
int tmr_ocl_create_kernel(
  FILE *Interactive_output, /* file or stdout to write messages */
  int Platform_index,
  int Device_index,
  int Kernel_index,
  char* Kernel_name,
  const char* Kernel_file,
  int Monitor
)
{

  cl_int retval;

  // choose the platform
  tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[Platform_index];

  // check the device !!!!!!!!!!!!!!!!! (or at least its index)
  if(Device_index < 0){
    fprintf(Interactive_output,"Wrong device_index %d passed to tmr_ocl_create_kernel! Exiting.\n",
	   Device_index);
    exit(-1);
  } 

  cl_device_id device = platform_struct.list_of_devices[Device_index].id;
  cl_context context = platform_struct.list_of_contexts[ 
			 platform_struct.list_of_devices[Device_index].context_index
							 ];

  if(Monitor>TMC_PRINT_INFO){
    fprintf(Interactive_output,"Program file is: %s\n", Kernel_file);
  }

#ifdef TUNING
  //for assembler
  system("rm -rf ~/.nv");
#endif

  // read source file into data structure
  const char* source = tmr_ocl_readSource(Interactive_output,Kernel_file);

  cl_program program = clCreateProgramWithSource(context, 1,
				      &source,
				      NULL, NULL);
  if (program == NULL)
    {
      fprintf(Interactive_output,"Failed to create CL program from source.\n");
      exit(-1);
    }

#ifdef TUNING

	  optf = fopen("options.txt", "r");
	  if(!optf) {
		 printf("Could not open options file!\n");
		 exit(-1);
	  }
	  resf = fopen("result.csv", "a+");
	  if(!resf) {
		 printf("Could not open results file!\n");
		 exit(-1);
	  }

	  #define NOPT 9

	  char opt[NOPT][40];
	  char buffer[40*(NOPT+10)];
	  int options[NOPT];
	  char name[50];
	  int word;
	  int tmp;
	  int i;
	  for(i=0;i<40*(NOPT+10);i++)
		  buffer[i]=0;



	  sprintf(&opt[0]," -D COAL_READ");
	  sprintf(&opt[1]," -D COAL_WRITE");
	  sprintf(&opt[2]," -D COMPUTE_ALL_SHAPE_FUN_DER");
	  sprintf(&opt[3]," -D USE_WORKSPACE_FOR_PDE_COEFF");
	  sprintf(&opt[4]," -D USE_WORKSPACE_FOR_GEO_DATA");
	  sprintf(&opt[5]," -D USE_WORKSPACE_FOR_SHAPE_FUN");
	  sprintf(&opt[6]," -D USE_WORKSPACE_FOR_STIFF_MAT");
	  sprintf(&opt[7]," -D WORKSPACE_PADDING=0");
	  sprintf(&opt[8]," -D WORKSPACE_PADDING=1");

//	  sprintf(&opt[0]," -D USE_WORKSPACE");
//	  sprintf(&opt[1]," -D USE_REGISTERS_FOR_COEFF");
//	  sprintf(&opt[2]," -D USE_SHAPE_FUN_WORKSPACE");
//	  sprintf(&opt[3]," -D USE_REGISTERS_FOR_SHAPE_FUN");
//	  sprintf(&opt[4]," -D USE_SHAPE_FUN_REF_DIRECTLY");
//	  sprintf(&opt[5]," -D STIFF_MAT_IN_SHARED");
//	  sprintf(&opt[6]," -D PADDING=0");
//	  sprintf(&opt[7]," -D PADDING=1");

	  //sprintf(&opt[7]," -D COAL_READ");
	  //sprintf(&opt[8]," -D COAL_WRITE");
	  //sprintf(&opt[9]," -D CONSTANT_COEFF");
	  //sprintf(&opt[9]," -D COUNT_OPER");

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

	  for(i=0;i<NOPT*line_count;i++)
		  fscanf(optf,"%d",&tmp);

	  for(i=0;i<NOPT;i++)
		  fscanf(optf,"%d",&options[i]);

	  printf("Options indicators:\n");
	  for(i=0;i<NOPT;i++)
	  {
		  if(line_count==0)
		  {
			  fprintf(headf, "%s,",opt[i]);
		  }
		  printf("%d ,",options[i]);
		  fprintf(resf,"%d ,",options[i]);
	  }
	  //fprintf(resf,";");
	  printf("Options:\n");
	  strcpy(buffer," ");
	  for(i=0;i<NOPT;i++)
	  {
		  if(options[i]==1)
		  {
			  strcat(buffer,opt[i]);
			  if(i==1)
			  {
				  system("touch coal_write");
			  }
		  }
		  else
			  strcat(buffer," ");
	  }

	#ifdef OPENCL_CPU
	 strcat(buffer," -D WORK_GROUP_SIZE=8");
	#elif defined OPENCL_GPU
	 strcat(buffer," -D WORK_GROUP_SIZE=64");
	#elif defined OPENCL_HSA
	 strcat(buffer," -D WORK_GROUP_SIZE=64");
	#elif defined OPENCL_PHI
	 strcat(buffer," -D WORK_GROUP_SIZE=8");
	#endif


	#ifdef LAPLACE
	 strcat(buffer," -D LAPLACE");
	#elif TEST_SCALAR
	 strcat(buffer," -D TEST_NUMINT");
	#elif HEAT
	 strcat(buffer," -D HEAT");
	#endif

	#ifdef COUNT_OPER
	   strcat(buffer," -D COUNT_OPER");
	#endif

#ifdef OPENCL_HSA
	strcat(buffer," -cl-std=CL2.0");
#endif
//debug
	//strcat(buffer," -g -s \"/home/fkruzel/Kod/mod_2015/work/dg_prism/tmr_ocl_num_int_el.cl\"");

	if(tmr_ocl_check_vendor()==1)
		strcat(buffer," -cl-nv-verbose");

#ifdef TUNING

	if(tmr_ocl_check_vendor()==3)
	{
		//strcat(buffer," -fno-bin-source -fno-bin-llvmir -fno-bin-amdil -save-temps=");
		strcat(buffer," -save-temps=");
		system("mkdir kernele");
		FILE* fp;
		for(i=0;i<50;i++)
		  name[i]=0;

		strcat(name,"kernele/");
		for(i=0;i<NOPT;i++)
		{
		  if(options[i]==1)
			  strcat(name,"1");
		  else
			  strcat(name,"0");
		}



#ifdef OPENCL_HSA
	strcat(name,"_HSA");
#endif
#ifdef OPENCL_GPU
	strcat(name,"_GPU");
#endif
#ifdef OPENCL_CPU
	strcat(name,"_CPU");
#endif

		word=wc("tmr_ocl_num_int_el.cl","//QSS");
		if(word==1)
		  strcat(name,"_QSS");
		word=wc("tmr_ocl_num_int_el.cl","//SQS");
		if(word==1)
		  strcat(name,"_SQS");
		word=wc("tmr_ocl_num_int_el.cl","//SSQ");
		if(word==1)
		  strcat(name,"_SSQ");

		strcat(buffer,name);

	}

#endif

	printf("%s\n",buffer);

	retval = clBuildProgram(program, 0, NULL, buffer, NULL, NULL);

	printf("SKOMPILOWANE!");

#else

#ifdef OPENCL_HSA
  retval = clBuildProgram(program, 0, NULL, "-cl-std=CL2.0", NULL, NULL);
#else

  if(tmr_ocl_check_vendor()==1)
  {
	  // TO GET INFO FROM NVIDIA COMPILER
	  retval = clBuildProgram(program, 0, NULL, "-cl-nv-verbose", NULL, NULL);
	  // TO FORCE NVIDIA COMPILER TO LIMIT THE NUMBER OF USED REGISTERS
	  //retval = clBuildProgram(program, 0, NULL, "-cl-nv-verbose -cl-nv-maxrregcount=64", NULL, NULL);
  }
  else
  {
	  // generic call - no compiler options
	  retval = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	  // debugger call
	  //retval = clBuildProgram(program, 0, NULL, "-g -s \"/home/fkruzel/Kod/mod_2015/work/diff_in_box\"", NULL, NULL);
	  //retval = clBuildProgram(program, 0, NULL, "-cl-nv-verbose", NULL, NULL);  
  }

#endif

#endif //TUNING


  char* buildLog; size_t size_of_buildLog; 
  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
			0, NULL, &size_of_buildLog); 
  buildLog = (char *)malloc(size_of_buildLog+1); 
  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
			size_of_buildLog, buildLog, NULL); 
  buildLog[size_of_buildLog]= '\0'; 
  if(Monitor>TMC_PRINT_INFO){
    fprintf(Interactive_output,"Kernel buildLog: %s\n", buildLog);
  }
  if (retval != CL_SUCCESS)
	{
	  fprintf(Interactive_output,"Error building program in tmr_ocl_create_kernel\n");
	  #ifdef TUNING
		  //fprintf(resf,"Kernel buildLog: %s", buildLog);
	  	  fprintf(resf,"Kernel error - too much shared data\n");
		  fclose(resf);
	  #endif
	  exit(-1);
	}


#ifdef TUNING

  if(tmr_ocl_check_vendor()==1)
  {
	  char *s;
	  int t=0;
	  int stack;
	  int stores;
	  int loads;
	  int registers;
	  int smem;
	  int cmem;
	  int cmem2;
	  int nr;

	  s = strstr(buildLog,"stack");
	  if (s != NULL)
	  {
		   t = (int)(s - buildLog);
		   //printf("Found string at index = %d\n", t );
		   sscanf(&buildLog[t-10],"%d",&stack);
		   //printf("stack=%d\n",stack);
	  }
	  else
	  {
		   //printf("String not found\n");
		  stack = 0;
	  }
	  if(line_count==0)
	  {
		  fprintf(headf, "bytes stack frame,");
	  }
	  fprintf(resf,"%d,",stack);
	  s = strstr(buildLog,"bytes spill stores");
	  if (s != NULL)
	  {
		   t = (int)(s - buildLog);
		   //printf("Found string at index = %d\n", t );
		   sscanf(&buildLog[t-10],"%*s %d",&stores);
		   //printf("stores=%d\n",stores);
	  }
	  else
	  {
		   //printf("String not found\n");
		  stores = 0;
	  }
	  if(line_count==0)
	  {
		  fprintf(headf, "bytes spill stores,");
	  }
	  fprintf(resf,"%d,",stores);
	  s = strstr(buildLog,"bytes spill loads");
	  if (s != NULL)
	  {
		  t = (int)(s - buildLog);
		  //printf("Found string at index = %d\n", t );
		  sscanf(&buildLog[t-10],"%*s %d",&loads);
		  //printf("loads=%d\n",loads);
	  }
	  else
	  {
		  //printf("String not found\n");
		  loads = 0;
	  }
	  if(line_count==0)
	  {
		  fprintf(headf, "bytes spill loads,");
	  }
	  fprintf(resf,"%d,",loads);
	  s = strstr(buildLog,"registers");
	  if (s != NULL)
	  {
		  t = (int)(s - buildLog);
		  //printf("Found string at index = %d\n", t );
		  sscanf(&buildLog[t-7],"%*s %d",&registers);
		  //printf("registers=%d\n",registers);
	  }
	  else
	  {
		  //printf("String not found\n");
		  registers = 0;
	  }
	  if(line_count==0)
	  {
		  fprintf(headf, "registers,");
	  }
	  fprintf(resf,"%d,",registers);
	  s = strstr(buildLog,"smem");
	  if (s != NULL)
	  {
		  t = (int)(s - buildLog);
		  //printf("Found string at index = %d\n", t );
		  sscanf(&buildLog[t-20],"%*s %d",&smem);
		  //printf("smem=%d\n",smem);
	  }
	  else
	  {
		  //printf("String not found\n");
		  smem = 0;
	  }
	  if(line_count==0)
	  {
		  fprintf(headf, "bytes smem,");
	  }
	  fprintf(resf,"%d,",smem);
	  s = strstr(buildLog,"cmem[0]");
	  if (s != NULL)
	  {
		  t = (int)(s - buildLog);
		  //printf("Found string at index = %d\n", t );
		  sscanf(&buildLog[t-10],"%d",&cmem);
		  //printf("cmem[0]=%d\n",cmem);
	  }
	  else
	  {
		  //printf("String not found\n");
		  cmem = 0;
	  }
	  if(line_count==0)
	  {
		  fprintf(headf, "bytes cmem[0],");
	  }
	  fprintf(resf,"%d,",cmem);
	  s = strstr(&buildLog[t+9],"cmem[");
	  if (s != NULL)
	  {
		  t = (int)(s - buildLog);
		  //printf("Found string at index = %d\n", t );
		  sscanf(&buildLog[t-10],"%d",&cmem2);
		  sscanf(&buildLog[t+5],"%d",&nr);
		  //printf("cmem[%d]=%d\n",nr,cmem2);
	  }
	  else
	  {
		  //printf("String not found\n");
		  nr=-1;
		  cmem2 = 0;
	  }
	  if(line_count==0)
	  {
		  fprintf(headf, "bytes cmem[%d],",nr);
	  }
	  fprintf(resf,"%d,",cmem2);

  }//end nvidia

#endif

  //printf("TUTAJ_DOSZLO %s?\n", Kernel_name);

  // Create OpenCL kernel
  cl_kernel kernel = clCreateKernel(program, Kernel_name, NULL);

  printf("TUTAJ_DOSZLO2?\n");



  if (kernel == NULL)
    {
      fprintf(Interactive_output,"Failed to create kernel.\n");
      exit(-1);
    }
  
  if(Monitor>TMC_PRINT_INFO){
    fprintf(Interactive_output,"Created kernel for platform %d, device %d, kernel index %d\n",
	   Platform_index, Device_index, Kernel_index);
  }
  
#ifdef DLDLDL

  if(tmr_ocl_check_vendor()==1)
  {
	  system("mkdir kernele");
	  FILE* fp;
	  size_t bin_sz;
	  for(i=0;i<50;i++)
		  name[i]=0;

	  strcat(name,"kernele/");
	  for(i=0;i<NOPT;i++)
	  {
		  if(options[i]==1)
			  strcat(name,"1");
		  else
			  strcat(name,"0");
	  }

	  word=wc("tmr_ocl_num_int_el.cl","//QSS");
	  if(word==1)
		  strcat(name,"_QSS");
	  word=wc("tmr_ocl_num_int_el.cl","//SQS");
	  if(word==1)
		  strcat(name,"_SQS");
	  word=wc("tmr_ocl_num_int_el.cl","//SSQ");
	  if(word==1)
		  strcat(name,"_SSQ");

	  strcat(name,"_kernel.ptx");

	  printf("Kernel: %s\n",name);

	  clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &bin_sz, NULL);

	  // Read binary (PTX file) to memory buffer
	  unsigned char *bin = (unsigned char *)malloc(bin_sz);
	  clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char *), &bin, NULL);

	  // Save PTX to add_vectors_ocl.ptx
	  fp = fopen(name, "wb");
	  fwrite(bin, sizeof(char), bin_sz, fp);
	  fclose(fp);
	  free(bin);


	  word=wc(name,"add.f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "add.f64,");
	  }
	  fprintf(resf,"%d,",word);
	  //printf("add.f64=%d\n",word);
	  word=wc(name,"sub.f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "sub.f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"mul.f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "mul.f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"fma.rn.f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "fma.rn.f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"neg.f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "neg.f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"rcp.rn.f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "rcp.rn.f64,");
	  }
	  fprintf(resf,"%d,",word);

		word=wc(name,"global.f64");
		if(line_count==0)
		{
			fprintf(headf, "global.f64,");
		}
		fprintf(resf,"%d,",word);
		word=wc(name,"shared.f64");
		if(line_count==0)
		{
			fprintf(headf, "shared.f64,");
		}
		fprintf(resf,"%d,",word);
		word=wc(name,"const.f64");
		if(line_count==0)
		{
			fprintf(headf, "const.f64,");
		}
		fprintf(resf,"%d,",word);
		word=wc(name,"local.f64");
		if(line_count==0)
		{
		  fprintf(headf, "local.f64,");
		}
		fprintf(resf,"%d,",word);


  }
  else if(tmr_ocl_check_vendor()==2)
  {
	  system("mkdir kernele");
	  for(i=0;i<50;i++)
		  name[i]=0;

	  strcat(name,"kernele/");
	  for(i=0;i<NOPT;i++)
	  {
		  if(options[i]==1)
			  strcat(name,"1");
		  else
			  strcat(name,"0");
	  }

	  word=wc("tmr_ocl_num_int_el.cl","//QSS");
	  if(word==1)
		  strcat(name,"_QSS");
	  word=wc("tmr_ocl_num_int_el.cl","//SQS");
	  if(word==1)
		  strcat(name,"_SQS");
	  word=wc("tmr_ocl_num_int_el.cl","//SSQ");
	  if(word==1)
		  strcat(name,"_SSQ");

#ifdef OPENCL_CPU
	  strcat(name,"_kernel_CPU.ptx");
#endif
#ifdef OPENCL_PHI
	  strcat(name,"_kernel_PHI.ptx");
#endif


	  printf("Kernel: %s\n",name);

	  char compile[500];

	  for(i=0;i<500;i++)
		  compile[i]=0;
	  strcat(compile,"ioc64 -cmd=build -input=tmr_ocl_num_int_el.cl -device=");
#ifdef OPENCL_CPU
	  strcat(compile,"cpu -asm=");
#endif
#ifdef OPENCL_PHI
	  strcat(compile,"co -asm=");
#endif
	  strcat(compile,name);
	  strcat(compile," -bo=\"");
	  strcat(compile,buffer);
	  strcat(compile,"\"");

	  printf("Compile: %s\n",compile);

	  system(compile);
//PHI
	  word=wc(name,"vsubrpd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vsubrpd,");
	  }
	  fprintf(resf,"%d,",word);
	  //printf("add.f64=%d\n",word);
	  word=wc(name,"vfmadd213pd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vfmadd213pd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"vfnmadd231pd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vfnmadd231pd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"vfmadd231pd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vfmadd231pd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"vfmadd132pd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vfmadd132pd,");
	  }
	  fprintf(resf,"%d,",word);
//common
	  word=wc(name,"vmulpd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vmulpd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"vaddpd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vaddpd,");
	  }
	  fprintf(resf,"%d,",word);

//cpu
	  word=wc(name,"vsubsd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vsubsd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"vmulsd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vmulsd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"vaddsd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vaddsd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"vsubpd");
	  if(line_count==0)
	  {
		  fprintf(headf, "vsubpd,");
	  }
	  fprintf(resf,"%d,",word);

  }
  else if(tmr_ocl_check_vendor()==3)
  {

	  char *device=NULL;

	  tmr_ocl_device_name(&device);

#ifdef OPENCL_HSA
	  strcat(name,"_1_");
	  strcat(name,device);
	  strcat(name,".hsail");
#else
	  strcat(name,"_0_");
	  strcat(name,device);
	  strcat(name,".il");
#endif

	  printf("Name: %s\n",name);

#ifdef OPENCL_GPU

//Tahiti
	  word=wc(name,"uav_raw_load_id");
	  if(line_count==0)
	  {
		  fprintf(headf, "uav_raw_load_id,");
	  }
	  fprintf(resf,"%d,",word);

	  word=wc(name,"uav_raw_store_id");
	  if(line_count==0)
	  {
		  fprintf(headf, "uav_raw_store_id,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"lds_store_vec_id(2)");
	  if(line_count==0)
	  {
		  fprintf(headf, "lds_store_vec_id(2),");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"lds_store64_id(2)");
	  if(line_count==0)
	  {
		  fprintf(headf, "lds_store64_id(2),");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"lds_load64_id(2)");
	  if(line_count==0)
	  {
		  fprintf(headf, "lds_load64_id(2),");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"dadd");
	  if(line_count==0)
	  {
		  fprintf(headf, "dadd,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"dmul");
	  if(line_count==0)
	  {
		  fprintf(headf, "dmul,");
	  }
	  fprintf(resf,"%d,",word);

#elif OPENCL_HSA

	  word=wc(name,"ld_global_align(8)_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "ld_global_align(8)_f64,");
	  }
	  fprintf(resf,"%d,",word);

	  word=wc(name,"st_group_align(8)_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "st_group_align(8)_f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"ld_v2_readonly_align(8)_width(all)_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "ld_v2_readonly_align(8)_width(all)_f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"sub_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "sub_f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"mul_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "mul_f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"add_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "add_f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"ld_group_align(8)_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "ld_group_align(8)_f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"ld_readonly_align(8)_width(all)_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "ld_readonly_align(8)_width(all)_f64,");
	  }
	  fprintf(resf,"%d,",word);
	  word=wc(name,"st_global_align(8)_f64");
	  if(line_count==0)
	  {
		  fprintf(headf, "st_global_align(8)_f64,");
	  }
	  fprintf(resf,"%d,",word);



	  //spectre
	  /*
	  v_add_f64
	  v_mul_f64
	  v_div_scale_f64
	  v_rcp_f64
	  v_fma_f64
	  v_div_fmas_f64
	  v_div_fixup_f64

	  */
#endif

	  free(device);
  }



#endif


  printf("DOSZ!\n");

  platform_struct.list_of_devices[Device_index].program[Kernel_index] = program;
  platform_struct.list_of_devices[Device_index].kernel[Kernel_index] = kernel;
  
  return(1);
}
//FK sep_jac

/*---------------------------------------------------------
  tmr_ocl_create_kernel_generic - copied from new version)
---------------------------------------------------------*/
int tmr_ocl_create_kernel_generic(
  FILE *Interactive_output, /* file or stdout to write messages */
  int Platform_index,
  int Device_index,
  int Kernel_index,
  char* Kernel_name,
  const char* Kernel_file,
  int Monitor
)
{

  cl_int retval;

  // choose the platform
  tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[Platform_index];

  // check the device !!!!!!!!!!!!!!!!! (or at least its index)
  if(Device_index < 0){
    fprintf(Interactive_output,"Wrong device_index %d passed to tmr_ocl_create_kernel! Exiting.\n",
	   Device_index);
    exit(-1);
  }

  cl_device_id device = platform_struct.list_of_devices[Device_index].id;
  cl_context context = platform_struct.list_of_contexts[
			 platform_struct.list_of_devices[Device_index].context_index
							 ];

  if(Monitor>TMC_PRINT_INFO){
    fprintf(Interactive_output,"Program file is: %s\n", Kernel_file);
  }

  // read source file into data structure
  const char* source = tmr_ocl_readSource(Interactive_output,Kernel_file);

  cl_program program = clCreateProgramWithSource(context, 1,
				      &source,
				      NULL, NULL);
  if (program == NULL)
    {
      fprintf(Interactive_output,"Failed to create CL program from source.\n");
      exit(-1);
    }


#ifdef OPENCL_HSA
  retval = clBuildProgram(program, 0, NULL, "-cl-std=CL2.0", NULL, NULL);
#else

  if(tmr_ocl_check_vendor()==1)
  {
	  // TO GET INFO FROM NVIDIA COMPILER
	  retval = clBuildProgram(program, 0, NULL, "-cl-nv-verbose", NULL, NULL);
	  // TO FORCE NVIDIA COMPILER TO LIMIT THE NUMBER OF USED REGISTERS
	  // retval = clBuildProgram(program, 0, NULL, "-cl-nv-maxrregcount=32", NULL, NULL);
  }
  else
  {
	  // generic call - no compiler options
	  retval = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	  //retval = clBuildProgram(program, 0, NULL, "-cl-nv-verbose", NULL, NULL);
  }

#endif


  char* buildLog; size_t size_of_buildLog;
  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
			0, NULL, &size_of_buildLog);
  buildLog = (char *)malloc(size_of_buildLog+1);
  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
			size_of_buildLog, buildLog, NULL);
  buildLog[size_of_buildLog]= '\0';
  if(Monitor>TMC_PRINT_INFO){
    fprintf(Interactive_output,"Kernel buildLog: %s\n", buildLog);
  }
  if (retval != CL_SUCCESS){
    fprintf(Interactive_output,"Error building program in tmr_ocl_create_kernel\n");
    exit(-1);
  }

  // Create OpenCL kernel
  cl_kernel kernel = clCreateKernel(program, Kernel_name, NULL);
  if (kernel == NULL)
    {
      fprintf(Interactive_output,"Failed to create kernel.\n");
      exit(-1);
    }

  if(Monitor>TMC_PRINT_INFO){
    fprintf(Interactive_output,"Created kernel for platform %d, device %d, kernel index %d\n",
	   Platform_index, Device_index, Kernel_index);
  }


  platform_struct.list_of_devices[Device_index].program[Kernel_index] = program;
  platform_struct.list_of_devices[Device_index].kernel[Kernel_index] = kernel;

  return(1);
}

/*---------------------------------------------------------
tmr_ocl_get_current_platform_index() - to return the index of current OpenCL platform
---------------------------------------------------------*/
int tmr_ocl_get_current_platform_index()
{
  return(tmv_ocl_struct.current_platform_index);
}

/*---------------------------------------------------------
  tmr_ocl_get_current_device_type 
//   returns the current OpenCL device type specified based on compiler switches
---------------------------------------------------------*/
int tmr_ocl_get_current_device_type()
{
  return(tmv_ocl_struct.current_device_type);
}

/*---------------------------------------------------------
  tmr_ocl_select_device 
//   returns OpenCL device index (for local data structures) or -1 if device
//   is not available (not existing or not serviced) for the specified platform
---------------------------------------------------------*/
int tmr_ocl_select_device( 
			  int Platform_index,
			  int Device_tmc_type
			   )
{
  int device_index = -1;
  
  // choose the platform
  tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[Platform_index];
  
  // in a loop over all devices
  int idev;
  for(idev=0; idev<platform_struct.number_of_devices; idev++){

    //fprintf(Interactive_output,"platform %d, idev %d, type %d, inptm_type %d\n",
    //	   Platform_index, idev, platform_struct.list_of_devices[idev].tmc_type, Device_tmc_type);
	
    // check device type
    if(platform_struct.list_of_devices[idev].tmc_type == Device_tmc_type) {
      device_index = idev;
      break;
    }
  }

  return(device_index);
}


/*---------------------------------------------------------
  tmr_ocl_device_type - returns LOCAL device type (INTEGER)
---------------------------------------------------------*/
int tmr_ocl_device_type(
  int Platform_index,
  int Device_index
  )
{
  // choose the platform
  tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[Platform_index];
  return(platform_struct.list_of_devices[Device_index].tmc_type);
}


/*---------------------------------------------------------
  tmr_ocl_select_context - selects context for a given device
---------------------------------------------------------*/
cl_context tmr_ocl_select_context(
  int Platform_index,
  int Device_index
)
{
  // choose the platform
  tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[Platform_index];
  int context_index = platform_struct.list_of_devices[Device_index].context_index;
  return(platform_struct.list_of_contexts[context_index]);
}

/*---------------------------------------------------------
  tmr_ocl_select_command_queue - selects command queue for a given device
---------------------------------------------------------*/
cl_command_queue tmr_ocl_select_command_queue(
  int Platform_index,
  int Device_index
)
{
  // choose the platform
  tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[Platform_index];
  return(platform_struct.list_of_devices[Device_index].command_queue);
}

/*---------------------------------------------------------
  tmr_ocl_select_kernel - selects kernel for a given device and kernel index
---------------------------------------------------------*/
cl_kernel tmr_ocl_select_kernel(
  int Platform_index,
  int Device_index,
  int Kernel_index
)
{
  // choose the platform
  tmt_ocl_platform_struct platform_struct = tmv_ocl_struct.list_of_platforms[Platform_index];
  return(platform_struct.list_of_devices[Device_index].kernel[Kernel_index]);
}

/*---------------------------------------------------------
  tmr_ocl_cleanup - discards created OpenCL resources
---------------------------------------------------------*/
void tmr_ocl_cleanup()
{
  int i,j,k;

  for(i=0; i< tmv_ocl_struct.number_of_platforms; i++){
    
    for(j=0; j<tmv_ocl_struct.list_of_platforms[i].number_of_devices; j++){
      
      if(tmv_ocl_struct.list_of_platforms[i].list_of_devices[j].command_queue!=0){
        clReleaseCommandQueue(
		      tmv_ocl_struct.list_of_platforms[i].list_of_devices[j].command_queue);
      }
      
      for(k=0;k<TMC_OCL_MAX_NUM_KERNELS;k++){
	if (tmv_ocl_struct.list_of_platforms[i].list_of_devices[j].kernel[k] != 0){
	  clReleaseKernel(tmv_ocl_struct.list_of_platforms[i].list_of_devices[j].kernel[k]);
	}
      }
      
      for(k=0;k<TMC_OCL_MAX_NUM_KERNELS;k++){
	if (tmv_ocl_struct.list_of_platforms[i].list_of_devices[j].program[k] != 0){
	  clReleaseProgram(tmv_ocl_struct.list_of_platforms[i].list_of_devices[j].program[k]);
	}
      }
      
    }
     
    free(tmv_ocl_struct.list_of_platforms[i].list_of_devices);
    
    if (tmv_ocl_struct.list_of_platforms[i].list_of_contexts[0] != 0)
      clReleaseContext(tmv_ocl_struct.list_of_platforms[i].list_of_contexts[0]);
    
    if (tmv_ocl_struct.list_of_platforms[i].list_of_contexts[1] != 0)
      clReleaseContext(tmv_ocl_struct.list_of_platforms[i].list_of_contexts[1]);
    
    if (tmv_ocl_struct.list_of_platforms[i].list_of_contexts[2] != 0)
      clReleaseContext(tmv_ocl_struct.list_of_platforms[i].list_of_contexts[2]);
    
  }
  
  free(tmv_ocl_struct.list_of_platforms);
    
}
  
  
