#ifdef cl_khr_fp64
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#else
    #error "Double precision floating point not supported by OpenCL implementation."
#endif

#define SCALAR double

#define NR_EXEC_PARAMS 32  // size of array with execution parameters
    // here: the smallest work-group for reading data is selected
    // exec_params are read from global to shared memory and used when needed
    // if shared memory resources are scarce this can be reduced

kernel void tmr_ocl_num_int_el(
  __global int* execution_parameters,
  __global SCALAR* gauss_dat, // integration points data of elements having given p
  __global SCALAR* shpfun_ref, // shape functions on a reference element
  __global SCALAR* el_data_in, // data for integration of NR_ELEMS_PER_KERNEL elements
  __global SCALAR* stiff_mat_out, // result of integration of NR_ELEMS_PER_KERNEL elements
  __local SCALAR *part_of_stiff_mat,
  __local SCALAR *num_shape_workspace,
  __local SCALAR *pde_coeff_workspace
){

  int i,j,k;
  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);


  __local SCALAR exec_params[NR_EXEC_PARAMS]; // shared memory copy of execution parameters

  if(thread_id < NR_EXEC_PARAMS){

    exec_params[thread_id] = execution_parameters[thread_id];

  }


  if(thread_id==0 && group_id==0){
    for(i=0;i<NR_EXEC_PARAMS;i++){

      stiff_mat_out[i] = exec_params[i];

    }

  }

  /* if(thread_id < NR_EXEC_PARAMS){ */
    
  /*   stiff_mat_out[thread_id] = exec_params[thread_id]; */
    
  /* } */


};
