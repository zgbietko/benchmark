//QSS
//!!!!!!!!!!! TODO: SEPARATE GEO_DATA AND PDE_COEFF !!!!!!!!!!!!!!!!

//!!!!!!!!!!! TEST DIFFERENT CONFIGURATIONS OF SHARED / L1 MEMORY

//!!!!!!!!!!! LOOP UNROLLING = REGISTER BLOCKING FOR INNERMOST LOOPS ??????????

#if defined(cl_amd_fp64)
  #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#elif defined(cl_khr_fp64)
  #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#else
  #error "Double precision floating point not supported by OpenCL implementation."
#endif

//----------------------------------------------------
// THREE MASTER SWITCHES (float<->double, work_group_size, nr_threads_per_elem)
//#define FLOAT
#ifdef FLOAT
  #define SCALAR float
  #define zero 0.0f
  #define one 1.0f
  #define two 2.0f
  #define half 0.5f
  #define one_fourth 0.25f
  #define one_sixth (0.16666666667f)
#else
  #define SCALAR double
  #define zero 0.0
  #define one 1.0
  #define two 2.0
  #define half 0.5
  #define one_fourth 0.25
  #define one_sixth (0.166666666666666667)
#endif

//#define WORK_GROUP_SIZE 16 // for XEON_PHI
#define WORK_GROUP_SIZE 64 // for GPUs
//#define WORK_GROUP_SIZE 8 // for CPUs

//#define ONE_EL_ONE_THREAD
#define ONE_EL_TWO_THREADS

// NR_ELEMS_PER_WORKGROUP - number of elements SIMULTANEOUSLY processed by one workgroup !!!
#ifdef ONE_EL_ONE_THREAD
  // for 1 element per thread
  #define NR_ELEMS_PER_WORKGROUP WORK_GROUP_SIZE 
#elif defined(ONE_EL_TWO_THREADS)
  // for 1 element per 2 threads
  #define NR_ELEMS_PER_WORKGROUP WORK_GROUP_SIZE/2
#endif

//----------------------------------------------------


//----------------------------------------------------
// SEVERAL PROBLEM, ELEMENT AND APPROXIMATION DEPENDENT SWITCHES 
// (nr_exec_params, nreq, num_shap, num_gauss, num_geo_dofs
#define NR_EXEC_PARAMS 16  // size of array with execution parameters
// here: the smallest work-group for reading data is selected
// exec_params are read from global to shared memory and used when needed
// if shared memory resources are scarce this can be reduced

// FOR SCALAR PROBLEMS !!!!!!!!!!!!!!!!!!!!!
#define nreq 1
// FOR NS_SUPG PROBLEM !!!!!!!!!!!!!!!!!!!!!
//#define nreq 4

// FOR LINEAR PRISMS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define num_shap 6
#define num_gauss 6
#define num_geo_dofs 6
#define weight_linear_prism (one_sixth)
#define weight_gauss weight_linear_prism

// FOR LINEAR TETRAHEDRA !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//#define num_shap 4
//#define num_gauss 4
//#define num_geo_dofs 4
//#define weight_linear_tetra (one_fourth*one_sixth)
//#define weight_gauss weight_linear_tetra

#define num_dofs (num_shap*nreq)
#define EL_GEO_DAT_SIZE (3*num_geo_dofs)

// J_AND_DETJ_SIZE=10 - for NOJAC variants
//#define J_AND_DETJ_SIZE 10

// the number of coefficients sent for elements
// either coefficients constant for the whole element
// or different for every integration point
//#define LAPLACE
#define TEST_NUMINT
//#define HEAT
#ifdef LAPLACE
  #define NR_COEFFS_SENT_PER_ELEMENT 0
  #define NR_COEFFS_SENT_PER_INT_POINT 1
  #define NR_COEFFS_IN_SM_CALCULATIONS num_gauss
#elif defined(TEST_NUMINT)
  #define NR_COEFFS_SENT_PER_ELEMENT 20
  #define NR_COEFFS_SENT_PER_INT_POINT 0
  #define NR_COEFFS_IN_SM_CALCULATIONS 20
#elif defined(HEAT)
  #define NR_COEFFS_SENT_PER_ELEMENT (num_dofs+1)
  #define NR_COEFFS_SENT_PER_INT_POINT 5
  #define NR_COEFFS_IN_SM_CALCULATIONS 20
#endif

#define NR_PDE_COEFFS_SENT (NR_COEFFS_SENT_PER_ELEMENT + NR_COEFFS_SENT_PER_INT_POINT*num_gauss)
//----------------------------------------------------


//----------------------------------------------------
// SWITCHES FOR DIFFERENT OPTIMIZATION OPTIONS !!!!!!!!!!!!!!!!!!!!!!!!

//load vector computing - not defined only for some tests
#define LOAD_VEC_COMP

// COAL_READ - both PDE_COEFF and GEO_DATA are read in a coalesced way (using a single workspace)
// coalesced reading may be good for GPUs (requires large workspace and several barriers)
// (the workspace can be further used by GEO_DAT or PDE_COEFF or SHAPE_FUN or STIFF_MAT !!!)
#define COAL_READ

// coalesced writing requires host code to adapt to the order of data!!!
// coalesced writing should be switched on for GPUs
#define COAL_WRITE

// COMPUTE_ALL_SHAPE_FUN_DER - to compute all shape functions and their derivatives
//                         before entering the loops over shape functions
#define COMPUTE_ALL_SHAPE_FUN_DER

//#define COUNT_OPER

// ********** THE SET OF OPTIONS FOR USING THE WORKSPACE IN SHARED MEMORY
// ********** AT MOST 1 SHOULD BE DEFINED !!!!!!!!!!!!!!!
// USE_WORKSPACE_FOR_PDE_COEFF - to use shared memory for pde_coeff during SM calculations
// otherwise - registers are used
//#define USE_WORKSPACE_FOR_PDE_COEFF

// USE_WORKSPACE_FOR_GEO_DATA - to use shared memory for geo_dat during SM calculations
// otherwise - registers are used
//#define USE_WORKSPACE_FOR_GEO_DATA


// USE_WORKSPACE_FOR_SHAPE_FUN - to use shared memory for shape functions 
//                               and their derivatives during SM calculations
// otherwise - registers are used
//#define USE_WORKSPACE_FOR_SHAPE_FUN


// USE_WORKSPACE_FOR_STIFF_MAT - to use shared memory for SM during SM calculations
// otherwise - registers are used
//#define USE_WORKSPACE_FOR_STIFF_MAT

// *** ### !!! FOR ONE ELEMENT TWO THREADS SOME SWITCHES ARE MANDATORY *** ### !!!
#ifdef ONE_EL_TWO_THREADS
  #define COAL_READ
  #define COAL_WRITE
  #define COMPUTE_ALL_SHAPE_FUN_DER // reader threads compute the values of shape functions
  #define USE_WORKSPACE_FOR_SHAPE_FUN // each pair of threads share workspace (saving registers)
  #undef USE_WORKSPACE_FOR_STIFF_MAT
#endif

// FOR EACH ARCHITECTURE PADDING SHOULD BE TESTED TO DETECT SHARED MEMORY BANK CONFLICTS
#define WORKSPACE_PADDING 0
#define PADDING WORKSPACE_PADDING


// THE END OF: SWITCHES FOR DIFFERENT OPTIMIZATION OPTIONS
//----------------------------------------------------


kernel void tmr_ocl_num_int_el(
  // execution_parameters can be read directly from constant memory, assuming it is cached and
  // further accesses are realized from cache
  __constant int* execution_parameters,
  //__global int* execution_parameters,
  // gauss data can be read directly from constant memory, assuming it is cached and
  // further accesses are realized from cache
  __constant SCALAR* gauss_dat, // integration points data of elements having given p
  //__global SCALAR* gauss_dat, // integration points data of elements having given p
  // shape function values can be read directly from constant memory, assuming it is cached and
  // further accesses are realized from cache
  __constant SCALAR* shpfun_ref, // shape functions on a reference element
  //__global SCALAR* shpfun_ref, // shape functions on a reference element
  __global SCALAR* el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
  // TODO!!!!!!!!!!!!!!!!!!
  //__global SCALAR* el_data_in_geo, // geo data for integration of NR_ELEMS_THIS_KERCALL elements
  //__global SCALAR* el_data_in_coeff, // coeff data for integration of elements
  __global SCALAR* stiff_mat_out // result of integration of NR_ELEMS_THIS_KERCALL elements
){

#ifdef COUNT_OPER
  SCALAR nr_oper=0.0;
  SCALAR nr_access_shared=0.0;
  SCALAR nr_global_access=0.0;
#endif

  //const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);
  //const int work_group_size = get_local_size(0);
  //const int nr_work_groups = get_num_groups(0);

#ifdef ONE_EL_ONE_THREAD // all threads considered readers, all read data
  const int reader_thread = 1;
  const int my_elem_num = get_local_id(0);
#elif defined(ONE_EL_TWO_THREADS) // threads divided based on thread_id, readers read data
  int reader_thread = 1;
  if(get_local_id(0) >= (WORK_GROUP_SIZE/2))
  {
	  reader_thread = 0;
  }
  const int is_even = 1-get_local_id(0)%2;
  const int my_elem_num = get_local_id(0)/2;
#endif

//  	if(reader_thread==1)
//	{
//	  printf("rThread_id=%d, ",get_local_id(0));
//	  printf("rmyelem_num=%d\n",my_elem_num);
//	}
//  	else
//  	{
//  		  printf("Thread_id=%d, ",get_local_id(0));
//  		  printf("myelem_num=%d\n",my_elem_num);
//  		}

//----------------------------------------------------
// DEFINITIONS DEPENDENT ON OPTIMIZATION OPTIONS

#ifdef USE_WORKSPACE_FOR_PDE_COEFF
  #define WORKSPACE_SIZE_FOR_PDE_COEFF (NR_COEFFS_IN_SM_CALCULATIONS)
#else
  #define WORKSPACE_SIZE_FOR_PDE_COEFF 0
#endif

#ifdef USE_WORKSPACE_FOR_GEO_DATA
  #define WORKSPACE_SIZE_FOR_GEO_DATA (3*num_geo_dofs)
#else
  #define WORKSPACE_SIZE_FOR_GEO_DATA 0
  SCALAR geo_dat[3*num_geo_dofs];
#endif

#ifdef USE_WORKSPACE_FOR_SHAPE_FUN
  #define WORKSPACE_SIZE_FOR_SHAPE_FUN (3*num_shap)
#else
  #define WORKSPACE_SIZE_FOR_SHAPE_FUN 0
  #ifdef COMPUTE_ALL_SHAPE_FUN_DER
    SCALAR tab_fun_u_derx[num_shap];
    SCALAR tab_fun_u_dery[num_shap];
    SCALAR tab_fun_u_derz[num_shap];
  #endif
#endif

#ifdef USE_WORKSPACE_FOR_STIFF_MAT
  #define WORKSPACE_SIZE_FOR_STIFF_MAT (num_dofs*(num_dofs+1))
#else

  #define WORKSPACE_SIZE_FOR_STIFF_MAT 0

  #ifdef ONE_EL_ONE_THREAD
  // for 1 element per thread
    SCALAR stiff_mat[num_dofs*num_dofs];
    SCALAR load_vec[num_dofs];
  #elif defined(ONE_EL_TWO_THREADS)
    // each thread has half of SM and LV
    SCALAR stiff_mat[num_dofs*num_dofs/2];
    SCALAR load_vec[num_dofs/2];
  #endif

#endif

#ifdef COAL_READ
  #if (NR_PDE_COEFFS_SENT) > (3*num_geo_dofs)
    #define WORKSPACE_SIZE_FOR_READING NR_PDE_COEFFS_SENT
  #else
    #define WORKSPACE_SIZE_FOR_READING (3*num_geo_dofs)
  #endif
#else
  #define WORKSPACE_SIZE_FOR_READING 0
#endif

#define TMP_SIZE (WORKSPACE_SIZE_FOR_PDE_COEFF+WORKSPACE_SIZE_FOR_GEO_DATA+WORKSPACE_SIZE_FOR_SHAPE_FUN+WORKSPACE_SIZE_FOR_STIFF_MAT)

#if (TMP_SIZE) > (WORKSPACE_SIZE_FOR_READING)
  #define WORKSPACE_SIZE ((TMP_SIZE+WORKSPACE_PADDING)*NR_ELEMS_PER_WORKGROUP)
#else
  #define WORKSPACE_SIZE ((WORKSPACE_SIZE_FOR_READING+WORKSPACE_PADDING)*NR_ELEMS_PER_WORKGROUP)
#endif

#if (WORKSPACE_SIZE) > 0
  __local SCALAR workspace[WORKSPACE_SIZE]; //
#endif


#ifndef USE_WORKSPACE_FOR_SHAPE_FUN
  __local SCALAR sender[(3*num_shap)*NR_ELEMS_PER_WORKGROUP];
#endif

  __local SCALAR temporary_vol[NR_ELEMS_PER_WORKGROUP];
// THE END OF: DEFINITIONS DEPENDENT ON OPTIMIZATION OPTIONS
//----------------------------------------------------


  // ASSUMPTION: one element = one thread 
  // OR ASSUMPTION: one element = two threads

  int nr_elems_per_thread = execution_parameters[0];
  int nr_elems_this_kercall = execution_parameters[1];

  int ielem;
  int offset;

  //printf("nr_elems_per_thread=%d\n",nr_elems_per_thread);


//-------------------------------------------------------------
//******************* loop over elements processed by a thread *********************
  for(ielem = 0; ielem < nr_elems_per_thread; ielem++){

    int element_index = get_group_id(0) * nr_elems_per_thread * NR_ELEMS_PER_WORKGROUP +
                                                        ielem * NR_ELEMS_PER_WORKGROUP +
                                                                my_elem_num ;
//    int element_index = get_group_id(0) * nr_elems_per_thread * WORK_GROUP_SIZE +
//                                                     ielem * WORK_GROUP_SIZE +
//                                                                   thread_id ;
    int i;

    //if(ielem==nr_elems_per_thread-1)

//-------------------------------------------------------------
// ******************* READING INPUT DATA *********************

#ifdef COAL_READ

  #ifdef USE_WORKSPACE_FOR_GEO_DATA // workspace is used for GEO_DATA hence we read PDE_COEFF
                                    // and immediately rewrite them to registers

    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
      // TODO !!!!!!!!!!!!!!
      //offset= 
                       (element_index - my_elem_num) * NR_PDE_COEFFS_SENT;

    barrier(CLK_LOCAL_MEM_FENCE); // I don't know why but without barrier here, one per ten runs gives bad results

    if(reader_thread){

      // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
      for(i=0;i<NR_PDE_COEFFS_SENT;i++) {
	
	workspace[i*NR_ELEMS_PER_WORKGROUP+my_elem_num] = 
                       	  el_data_in[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];
	// TODO!!!!!!!!!!!!!!
	//el_data_in_coeff[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];
	
      }

    }

    #ifdef COUNT_OPER
    nr_global_access += NR_PDE_COEFFS_SENT; // we neglect shared memory accesses at this stage
    #endif

    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

    offset=my_elem_num*(NR_PDE_COEFFS_SENT);

    #ifdef LAPLACE

    SCALAR coeff0=workspace[offset+0];
    SCALAR coeff1=workspace[offset+1];
    SCALAR coeff2=workspace[offset+2];
    SCALAR coeff3=workspace[offset+3];
    SCALAR coeff4=workspace[offset+4];
    SCALAR coeff5=workspace[offset+5];

    #elif defined(TEST_NUMINT)

    SCALAR coeff00=workspace[offset+0];
    SCALAR coeff01=workspace[offset+1];
    SCALAR coeff02=workspace[offset+2];
    SCALAR coeff10=workspace[offset+3];
    SCALAR coeff11=workspace[offset+4];
    SCALAR coeff12=workspace[offset+5];
    SCALAR coeff20=workspace[offset+6];
    SCALAR coeff21=workspace[offset+7];
    SCALAR coeff22=workspace[offset+8];
    SCALAR coeff30=workspace[offset+9];
    SCALAR coeff31=workspace[offset+10];
    SCALAR coeff32=workspace[offset+11];
    SCALAR coeff03=workspace[offset+12];
    SCALAR coeff13=workspace[offset+13];
    SCALAR coeff23=workspace[offset+14];
    SCALAR coeff33=workspace[offset+15];
    SCALAR coeff04=workspace[offset+16];
    SCALAR coeff14=workspace[offset+17];
    SCALAR coeff24=workspace[offset+18];
    SCALAR coeff34=workspace[offset+19];

    #elif defined(HEAT)

    #endif

    #ifdef COUNT_OPER
    nr_access_shared += NR_PDE_COEFFS_SENT; // we count accesses because of the barriers
    #endif

    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

    // after rewriting PDE_COEFF to registers we read GEO_DATA to workspace
    offset = (element_index-my_elem_num)*(EL_GEO_DAT_SIZE);

    if(reader_thread){

      // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
      for(i = 0; i < EL_GEO_DAT_SIZE; i++){
	
	workspace[i*NR_ELEMS_PER_WORKGROUP+my_elem_num] = 
	                         el_data_in[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];
	// TODO !!!!
	// el_data_in_geo[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];
      }

    }

    #ifdef COUNT_OPER
    nr_global_access += EL_GEO_DAT_SIZE; // we neglect shared memory accesses at this stage
    #endif


  #else // if workspace not used for geo_data
        // (hence used for pde_coeff or something else)

    // first we read geo data to workspace 
    offset = (element_index-my_elem_num)*(EL_GEO_DAT_SIZE);

    //offset = (element_index-thread_id)*(EL_GEO_DAT_SIZE);
    barrier(CLK_LOCAL_MEM_FENCE); // I don't know why but without barrier here, one per ten runs gives bad results

    if(reader_thread){

      // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
      for(i = 0; i < EL_GEO_DAT_SIZE; i++){
      
	workspace[i*NR_ELEMS_PER_WORKGROUP+thread_id] =
                              el_data_in[offset+i*NR_ELEMS_PER_WORKGROUP+thread_id];
	// TODO !!!!
	// el_data_in_geo[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];

      }
    }


    #ifdef COUNT_OPER
    nr_global_access += EL_GEO_DAT_SIZE; // we neglect shared memory accesses at this stage
    #endif

    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

//    if(get_global_id(0)==0)
//    {
//    	for(i = 0; i < EL_GEO_DAT_SIZE*NR_ELEMS_PER_WORKGROUP; i++){
//
//    		//printf("thread=%d\n",thread_id);
//    		//printf("Element=%d\n",my_elem_num);
//    		printf("geo_dat[%d]=%lf\n",i,workspace[i]);
//    	}
//    }

    // we rewrite geo_data to registers
    offset=my_elem_num*EL_GEO_DAT_SIZE;  //ok
    //offset=thread_id*EL_GEO_DAT_SIZE;
    
    for(i=0;i<num_geo_dofs;i++){  
      
      geo_dat[3*i] = workspace[offset+3*i];  //node coor
      geo_dat[3*i+1] = workspace[offset+3*i+1];
      geo_dat[3*i+2] = workspace[offset+3*i+2];

//      if(get_group_id(0)==0)
//          printf("element=%d,my-elem_num=%d,thread=%d,geo_dat[%d]=%lf\n",element_index,my_elem_num,thread_id,3*i,geo_dat[3*i]);

    }
    
    #ifdef COUNT_OPER
    nr_access_shared += 3*num_geo_dofs; // we count accesses because of the barriers
    #endif

    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

    // after rewriting GEO_DATA to registers we read  PDE_COEFF to workspace
    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
      // TODO !!!!!!!!!!!!!!
      //offset= 
                    (element_index-my_elem_num) * NR_PDE_COEFFS_SENT;

    if(reader_thread){

      // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
      for(i=0;i<NR_PDE_COEFFS_SENT;i++) {

	workspace[i*NR_ELEMS_PER_WORKGROUP+thread_id] =
	  el_data_in[offset+i*NR_ELEMS_PER_WORKGROUP+thread_id];
	// TODO !!!!
	//el_data_in_coeff[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];

      }

    }

    #ifdef COUNT_OPER
    nr_global_access += NR_PDE_COEFFS_SENT; // we neglect shared memory accesses at this stage
    #endif

    // if we do not leave pde coeffs in workspace we rewrite them to registers
    #ifndef USE_WORKSPACE_FOR_PDE_COEFF

    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

    offset=my_elem_num*(NR_PDE_COEFFS_SENT);

      #ifdef LAPLACE

    SCALAR coeff0=workspace[offset+0];
    SCALAR coeff1=workspace[offset+1];
    SCALAR coeff2=workspace[offset+2];
    SCALAR coeff3=workspace[offset+3];
    SCALAR coeff4=workspace[offset+4];
    SCALAR coeff5=workspace[offset+5];

      #elif defined(TEST_NUMINT)

    SCALAR coeff00=workspace[offset+0];
    SCALAR coeff01=workspace[offset+1];
    SCALAR coeff02=workspace[offset+2];
    SCALAR coeff10=workspace[offset+3];
    SCALAR coeff11=workspace[offset+4];
    SCALAR coeff12=workspace[offset+5];
    SCALAR coeff20=workspace[offset+6];
    SCALAR coeff21=workspace[offset+7];
    SCALAR coeff22=workspace[offset+8];
    SCALAR coeff30=workspace[offset+9];
    SCALAR coeff31=workspace[offset+10];
    SCALAR coeff32=workspace[offset+11];
    SCALAR coeff03=workspace[offset+12];
    SCALAR coeff13=workspace[offset+13];
    SCALAR coeff23=workspace[offset+14];
    SCALAR coeff33=workspace[offset+15];
    SCALAR coeff04=workspace[offset+16];
    SCALAR coeff14=workspace[offset+17];
    SCALAR coeff24=workspace[offset+18];
    SCALAR coeff34=workspace[offset+19];

      #elif defined(HEAT)

      #endif

      #ifdef COUNT_OPER
    nr_access_shared += NR_PDE_COEFFS_SENT; // we count accesses because of the barriers
      #endif

    #endif // end if we do not leave pde coeffs in workspace and rewrite them to registers

  #endif // end if workspace not used for geo_data (hence used for pde coeffs or something else)

#else // if not COAL_READ (i.e. only one or none of PDE_COEFF and GEO_DAT read in a coalsced way)


  #ifdef USE_WORKSPACE_FOR_PDE_COEFF

    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
      // TODO !!!!!!!!!!!!!!
      //offset= 
                       (element_index - my_elem_num) * NR_PDE_COEFFS_SENT;


    if(reader_thread){

      // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
      for(i=0;i<NR_PDE_COEFFS_SENT;i++) {

        workspace[i*NR_ELEMS_PER_WORKGROUP+my_elem_num] = 
	                           el_data_in[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];
        // TODO!!!!!!!!!!!!!!
        //el_data_in_coeff[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];

      }

    }

    #ifdef COUNT_OPER
    nr_global_access += NR_PDE_COEFFS_SENT; // we neglect shared memory accesses at this stage
    #endif

  #else // if not USE_WORKSPACE_FOR_PDE_COEFF

    offset = nr_elems_this_kercall * EL_GEO_DAT_SIZE +
      // TODO !!!!!!!!!!!!!!
      //offset= 
	  			element_index * (NR_PDE_COEFFS_SENT);

    #ifdef LAPLACE

    SCALAR coeff0=el_data_in[offset+0];
    SCALAR coeff1=el_data_in[offset+1];
    SCALAR coeff2=el_data_in[offset+2];
    SCALAR coeff3=el_data_in[offset+3];
    SCALAR coeff4=el_data_in[offset+4];
    SCALAR coeff5=el_data_in[offset+5];

    #elif defined(TEST_NUMINT)

    SCALAR coeff00=el_data_in[offset+0];
    SCALAR coeff01=el_data_in[offset+1];
    SCALAR coeff02=el_data_in[offset+2];
    SCALAR coeff10=el_data_in[offset+3];
    SCALAR coeff11=el_data_in[offset+4];
    SCALAR coeff12=el_data_in[offset+5];
    SCALAR coeff20=el_data_in[offset+6];
    SCALAR coeff21=el_data_in[offset+7];
    SCALAR coeff22=el_data_in[offset+8];
    SCALAR coeff30=el_data_in[offset+9];
    SCALAR coeff31=el_data_in[offset+10];
    SCALAR coeff32=el_data_in[offset+11];
    SCALAR coeff03=el_data_in[offset+12];
    SCALAR coeff13=el_data_in[offset+13];
    SCALAR coeff23=el_data_in[offset+14];
    SCALAR coeff33=el_data_in[offset+15];
    SCALAR coeff04=el_data_in[offset+16];
    SCALAR coeff14=el_data_in[offset+17];
    SCALAR coeff24=el_data_in[offset+18];
    SCALAR coeff34=el_data_in[offset+19];


    #elif defined(HEAT)

    #endif

    #ifdef COUNT_OPER
    nr_global_access += NR_PDE_COEFFS_SENT;
    #endif

  #endif // end if not USE_WORKSPACE_FOR_PDE_COEFF

  #ifdef USE_WORKSPACE_FOR_GEO_DATA

    offset = (element_index-my_elem_num)*(EL_GEO_DAT_SIZE);

    barrier(CLK_LOCAL_MEM_FENCE); // I don't know why but without barrier here, one per ten runs gives bad results

    if(reader_thread){

      // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
      for(i = 0; i < EL_GEO_DAT_SIZE; i++){

        workspace[i*NR_ELEMS_PER_WORKGROUP+my_elem_num] = 
                    el_data_in[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];
        // TODO !!!!!!!!!
        // el_data_in_geo[offset+i*NR_ELEMS_PER_WORKGROUP+my_elem_num];

      }

    }

  #else // if not USE_WORKSPACE_FOR_GEO_DATA

    // we read geo data to registers
    offset = (element_index)*(EL_GEO_DAT_SIZE);

    // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
    for(i = 0; i < EL_GEO_DAT_SIZE; i++){
      
      geo_dat[i] = el_data_in[offset+i];
      // TODO !!!!!!!!!
      // geo_dat[i] = el_data_in_geo[offset+i];
      
    }

  #endif  // if not USE_WORKSPACE_FOR_GEO_DATA

  #ifdef COUNT_OPER
    nr_global_access += EL_GEO_DAT_SIZE; // we neglect shared memory accesses at this stage
  #endif

#endif // end if not COAL_READ

// ******* THE END OF: READING INPUT DATA *********************
//-------------------------------------------------------------

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@ DONE @@@@@@@@@@@@@@@@@@@@@@@@@@

#if (WORKSPACE_SIZE) > 0
    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
#endif

//-------------------------------------------------------------
//******************** INITIALIZING SM AND LV ******************//

#ifdef USE_WORKSPACE_FOR_STIFF_MAT

    if(reader_thread){

      // stiff_mat_workspace holds SM and LV
      for(i = 0; i < num_dofs*(num_dofs+1); i++) {
	workspace[my_elem_num*(num_dofs*(num_dofs+1)+PADDING)+i] = zero;
      }

    }

#ifdef COUNT_OPER
    nr_access_shared += num_dofs*(num_dofs+1);
#endif

#else // if not  USE_WORKSPACE_FOR_STIFF_MAT

  #ifdef ONE_EL_ONE_THREAD
    // for 1 element per thread

      for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = zero;

    #ifdef LOAD_VEC_COMP
      for(i = 0; i < num_dofs; i++) load_vec[i] = zero;
    #endif

  #elif defined(ONE_EL_TWO_THREADS)
    // for 1 element per 2 threads

      for(i = 0; i < num_dofs*num_dofs/2; i++) stiff_mat[i] = zero;

    #ifdef LOAD_VEC_COMP
      for(i = 0; i < num_dofs/2; i++) load_vec[i] = zero;
    #endif

  #endif // end if one element two threads


#endif // end if not  USE_WORKSPACE_FOR_STIFF_MAT

//******************** END OF: INITIALIZING SM AND LV ******************//
//-------------------------------------------------------------

//      barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
//      printf("wg=%d\t",get_group_id(0));
//      printf("thr=%d\t",get_local_id(0));
//      printf("Element index=%d\n",element_index);

//-------------------------------------------------------------
//************************* LOOP OVER INTEGRATION POINTS ************************//

    // in a loop over gauss points
    int igauss;
    int idof, jdof;
    for(igauss = 0; igauss < num_gauss; igauss++){


      // integration data read from cached constant or shared  memory
      SCALAR daux = gauss_dat[4*igauss];
      SCALAR faux = gauss_dat[4*igauss+1];
      SCALAR eaux = gauss_dat[4*igauss+2];
      //SCALAR vol = gauss_dat[4*igauss+3]; // vol = weight
      SCALAR vol = weight_gauss;	// vol = weight CONSTANT FOR LINEAR PRISM!!!
//      if(get_global_id(0)==0)
//    	  printf("igauss=%d,daux=%lf,faux=%lf,eaux=%lf,vol=%lf\n",igauss,daux,faux,eaux,vol);

#ifdef COUNT_OPER
    nr_access_shared += 4;
#endif


//-------------------------------------------------------------
//************************* JACOBIAN TERMS CALCULATIONS *************************//

      // when geometrical shape functions are not necessary 
      // (only derivatives are used for Jacobian calculations)
      SCALAR temp1 = zero;
      SCALAR temp2 = zero;
      SCALAR temp3 = zero;
      SCALAR temp4 = zero;
      SCALAR temp5 = zero;
      SCALAR temp6 = zero;
      SCALAR temp7 = zero;
      SCALAR temp8 = zero;
      SCALAR temp9 = zero;


#ifdef COMPUTE_ALL_SHAPE_FUN_DER
      if(is_even) // part executed by even threads (for one_el_one_thread all are even!)
      { // block also indicates the scope of jac_x registers
#endif

      SCALAR jac_0 = zero;
      SCALAR jac_1 = zero;
      SCALAR jac_2 = zero;
      SCALAR jac_3 = zero;
      SCALAR jac_4 = zero;
      SCALAR jac_5 = zero;
      SCALAR jac_6 = zero;
      SCALAR jac_7 = zero;
      SCALAR jac_8 = zero;

      // derivatives of geometrical shape functions
      { // block to indicate the scope of jac_data

        // derivatives of geometrical shape functions are stored in jac_data
	SCALAR jac_data[3*num_geo_dofs];
	jac_data[0] = -(one-eaux)*half;
	jac_data[1] =  (one-eaux)*half;
	jac_data[2] =  zero;
	jac_data[3] = -(one+eaux)*half;
	jac_data[4] =  (one+eaux)*half;
	jac_data[5] =  zero;
	jac_data[6] = -(one-eaux)*half;
	jac_data[7] =  zero;
	jac_data[8] =  (one-eaux)*half;
	jac_data[9] = -(one+eaux)*half;
	jac_data[10] =  zero;
	jac_data[11] =  (one+eaux)*half;
	jac_data[12] = -(one-daux-faux)*half;
	jac_data[13] = -daux*half;
	jac_data[14] = -faux*half;
	jac_data[15] =  (one-daux-faux)*half;
	jac_data[16] =  daux*half;
	jac_data[17] =  faux*half;


#ifdef COUNT_OPER
	nr_oper += 14; // after optimization
#endif

	/* Jacobian matrix J */
#ifdef USE_WORKSPACE_FOR_GEO_DATA
	offset=my_elem_num*EL_GEO_DAT_SIZE;
#endif

	for(i=0;i<num_geo_dofs;i++){

	  jac_1 = jac_data[i];
	  jac_2 = jac_data[num_geo_dofs+i];
	  jac_3 = jac_data[2*num_geo_dofs+i];

#ifdef USE_WORKSPACE_FOR_GEO_DATA

	  jac_4 = workspace[offset+3*i];  //node coor
	  jac_5 = workspace[offset+3*i+1];
	  jac_6 = workspace[offset+3*i+2];

#ifdef COUNT_OPER
	  nr_access_shared += 3;
#endif

#else // if not USE_WORKSPACE_FOR_GEO_DATA (geo data in registers)

	  jac_4 = geo_dat[3*i];  //node coor
	  jac_5 = geo_dat[3*i+1];
	  jac_6 = geo_dat[3*i+2];

//	  if(get_global_id(0)==0)
//	  {
//		 printf("igauss=%d,jac_4=%lf,jac_5=%lf,jac_6=%lf\n",igauss,jac_4,jac_5,jac_6,vol);
//	  }

#endif // end if not USE_GEO_DAT_WORKSPACE

	  temp1 += jac_4 * jac_1;
	  temp2 += jac_4 * jac_2;
	  temp3 += jac_4 * jac_3;
	  temp4 += jac_5 * jac_1;
	  temp5 += jac_5 * jac_2;
	  temp6 += jac_5 * jac_3;
	  temp7 += jac_6 * jac_1;
	  temp8 += jac_6 * jac_2;
	  temp9 += jac_6 * jac_3;

	}

      } // the end of scope for jac_data

#ifdef COUNT_OPER
      nr_oper += 18*num_geo_dofs; // after optimization
#endif

      jac_0 = (temp5*temp9 - temp8*temp6);
      jac_1 = (temp8*temp3 - temp2*temp9);
      jac_2 = (temp2*temp6 - temp3*temp5);

      daux = temp1*jac_0 + temp4*jac_1 + temp7*jac_2;

      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
      vol *= daux; // vol = weight * det J

      //printf("igauss=%d,vol=%lf\n",igauss,vol);

      faux = one/daux;

      jac_0 *= faux;
      jac_1 *= faux;
      jac_2 *= faux;

      jac_3 = (temp6*temp7 - temp4*temp9)*faux;
      jac_4 = (temp1*temp9 - temp7*temp3)*faux;
      jac_5 = (temp3*temp4 - temp1*temp6)*faux;

      jac_6 = (temp4*temp8 - temp5*temp7)*faux;
      jac_7 = (temp2*temp7 - temp1*temp8)*faux;
      jac_8 = (temp1*temp5 - temp2*temp4)*faux;

#ifdef COUNT_OPER
 nr_oper += 43; // after optimization, includes 1 inverse and 6 sign changes
 // total: 14+18*num_geo_dofs+43 = 165 (for prisms)
#endif

//************* THE END OF: JACOBIAN TERMS CALCULATIONS *************************//
//-------------------------------------------------------------

//-------------------------------------------------------------
//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//

#ifdef COMPUTE_ALL_SHAPE_FUN_DER

 //************ loop for computing ALL shape function values at integration point **********//
      for(idof = 0; idof < num_shap; idof++){

	// read proper values of shape functions and their derivatives
	temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];

  #ifdef COUNT_OPER
	nr_access_shared += 3; // 3 reads from constant cache
  #endif

	// compute derivatives wrt global coordinates
	// 15 operations

  #ifdef USE_WORKSPACE_FOR_SHAPE_FUN

	workspace[my_elem_num*(3*num_shap+PADDING)+3*idof]   = temp1*jac_0+temp2*jac_3+temp3*jac_6;
	workspace[my_elem_num*(3*num_shap+PADDING)+3*idof+1] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
	workspace[my_elem_num*(3*num_shap+PADDING)+3*idof+2] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

#ifdef COUNT_OPER
	nr_access_shared += 3; //  3 writes to shared memory
#endif

  #else  // if not USE_WORKSPACE_FOR_SHAPE_FUN

	tab_fun_u_derx[idof] = temp1*jac_0+temp2*jac_3+temp3*jac_6;
	tab_fun_u_dery[idof] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
	tab_fun_u_derz[idof] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

  #endif // end if not USE_WORKSPACE_FOR_SHAPE_FUN

      } // end loop over shape functions for which global derivatives were computed

#ifdef COUNT_OPER
 nr_oper += 15*num_shap; 
#endif

    } // the end of part executed by reader threads (for one_le_one_thread all are readers!)
      // also the end of block to indicate the scope of jac_x registers


      if(is_even)
      {
//    	  if(get_global_id(0)==0)
//    		  printf("thread=%d,tv[%d]=%lf\n",thread_id,my_elem_num,vol);
    	  temporary_vol[my_elem_num]=vol;

#ifndef USE_WORKSPACE_FOR_SHAPE_FUN

    	  for(idof = 0; idof < num_shap; idof++){
    		 sender[my_elem_num*(3*num_shap+PADDING)+3*idof]=tab_fun_u_derx[idof];
    		 sender[my_elem_num*(3*num_shap+PADDING)+3*idof+1]=tab_fun_u_dery[idof];
    		 sender[my_elem_num*(3*num_shap+PADDING)+3*idof+2]=tab_fun_u_derz[idof];
    	  }

#endif

      }
      //barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
      if(!is_even)
      {
    	  vol=temporary_vol[my_elem_num];
//    	  if(get_global_id(0)==0)
//    		  printf("thread=%d,vol[%d]=%lf\n",thread_id,my_elem_num,vol);

#ifndef USE_WORKSPACE_FOR_SHAPE_FUN

    	  for(idof = 0; idof < num_shap; idof++){
    		 tab_fun_u_derx[idof]=sender[my_elem_num*(3*num_shap+PADDING)+3*idof];
    		 tab_fun_u_dery[idof]=sender[my_elem_num*(3*num_shap+PADDING)+3*idof+1];
    		 tab_fun_u_derz[idof]=sender[my_elem_num*(3*num_shap+PADDING)+3*idof+2];
    	  }

#endif



      }
      //barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

#endif // end if COMPUTE_ALL_SHAPE_FUN_DER

//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
//-------------------------------------------------------------


//-------------------------------------------------------------
//***** SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS *****//

#ifdef HEAT

      // for non-constant, non-linear coefficients a place for call to problem dependent
      // function calculating actual PDE coefficients based on data in coeff 
      // workspace or registers and  storing data back in workspace or in registers

#endif


#ifdef USE_WORKSPACE_FOR_PDE_COEFF


  #ifdef LAPLACE

      // offset for reading data
      offset=my_elem_num*(NR_PDE_COEFFS_SENT);

      SCALAR coeff00 = workspace[offset+igauss];

    #ifdef COUNT_OPER
    nr_access_shared += 1;
    #endif

  #endif // end if LAPLACE

      // offset for computations
      offset=my_elem_num*(NR_COEFFS_IN_SM_CALCULATIONS);

#else // if not USE_WORKSPACE_FOR_PDE_COEFF

  #ifdef LAPLACE

      offset = nr_elems_this_kercall * EL_GEO_DAT_SIZE +
      	//TODO !!!!!!!!!!!!!!
      	//offset=
      	  			element_index * (NR_PDE_COEFFS_SENT);
      SCALAR coeff00=el_data_in[offset+igauss];

//	  barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
//	  if(get_group_id(0)==0)
//	  {
//		  printf("thread=%d,igauss=%d,coeff00=%lf\n",thread_id,igauss,coeff00);
//	  }

      /* // BELOW IS THE PROPER VERSION BUT NOT WORKING FOR AMD !!! */
      /* SCALAR coeff00=zero; */
      /* switch(igauss){ */
      /* case 0: */
      /* 	coeff00 = coeff0; */
      /* 	break; */
      /* case 1: */
      /* 	coeff00 = coeff1; */
      /* 	break; */
      /* case 2: */
      /* 	coeff00 = coeff2; */
      /* 	break; */
      /* case 3: */
      /* 	coeff00 = coeff3; */
      /* 	break; */
      /* case 4: */
      /* 	coeff00 = coeff4; */
      /* 	break; */
      /* case 5: */
      /* 	coeff00 = coeff5; */
      /* 	break; */
      /* } */

  #endif // end if LAPLACE

#endif // end if not USE_WORKSPACE_FOR_PDE_COEFF

//*** THE END OF: SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS ***//
//-------------------------------------------------------------


//-------------------------------------------------------------
//********************* first loop over shape functions ***********************//

#ifdef ONE_EL_ONE_THREAD
  // for 1 element per thread
      for(idof = 0; idof < num_shap; idof++){
	int idof2=idof;

#elif defined(ONE_EL_TWO_THREADS)
  // for 1 element per 2 threads
      int idof2;
      for(idof2 = 0; idof2 < num_shap/2; idof2++){
	// idof2 indicates the position in half of SM
	if(is_even){ // even threads (0, 2, 4, ...) compute upper half of SM
	  idof = idof2; // idof remains the number of shape function
	} else{
	  idof = idof2 + num_shap/2; // odd threads compute lower half of SM
	}
#endif
	
	//{ // beginning of using registers for u  (shp_fun_u, fun_u_der.)
	  

//-------------------------------------------------------------
//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ******//

#ifdef COMPUTE_ALL_SHAPE_FUN_DER
	  
  #ifdef USE_WORKSPACE_FOR_SHAPE_FUN
	  
//	  SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
//	  SCALAR fun_u_derx = workspace[my_elem_num*(3*num_shap+PADDING)+3*idof];
//	  SCALAR fun_u_dery = workspace[my_elem_num*(3*num_shap+PADDING)+3*idof+1];
//	  SCALAR fun_u_derz = workspace[my_elem_num*(3*num_shap+PADDING)+3*idof+2];
//register reuse
	temp9 = shpfun_ref[igauss*4*num_shap+4*idof];
	temp1 = workspace[my_elem_num*(3*num_shap+PADDING)+3*idof];
	temp2 = workspace[my_elem_num*(3*num_shap+PADDING)+3*idof+1];
	temp3 = workspace[my_elem_num*(3*num_shap+PADDING)+3*idof+2];



//	  barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
//	  if(get_group_id(0)==0)
//	  {
//		  printf("thread=%d,idof=%d,shp_fun_u=%lf\n",thread_id,idof,shp_fun_u);
//	  }

    #ifdef COUNT_OPER
          nr_access_shared += 4; // including 1 for constant cache
    #endif

  #else // if not USE_WORKSPACE_FOR_SHAPE_FUN

	  // read proper values of shape functions and their derivatives
          //register reuse
          temp9 = shpfun_ref[igauss*4*num_shap+4*idof];
          temp1 = tab_fun_u_derx[idof];
          temp2 = tab_fun_u_dery[idof];
          temp3 = tab_fun_u_derz[idof];

//          SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
//          SCALAR fun_u_derx = tab_fun_u_derx[idof];
//          SCALAR fun_u_dery = tab_fun_u_dery[idof];
//          SCALAR fun_u_derz = tab_fun_u_derz[idof];

    #ifdef COUNT_OPER
          nr_access_shared += 1;
    #endif
	  
  #endif // end if not USE_SHAPE_FUN_WORKSPACE

#else // if not COMPUTE_ALL_SHAPE_FUN_DER

	  // read proper values of shape functions and their derivatives
	  //SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
          //register reuse
      temp9 = shpfun_ref[igauss*4*num_shap+4*idof];
	  temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	  temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	  temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];
	  
	  
	  // compute derivatives wrt global coordinates
	  // 15 operations
	  
	  //register reuse

//	  SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
//	  SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
//	  SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
//
	  temp7 = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	  temp8 = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	  temp3 = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

	  temp1=temp7;
	  temp2=temp8;

  #ifdef COUNT_OPER
	  nr_access_shared += 4; // constant cache accesses
	  nr_oper += 15; // after optimization
	  // total: 13+5+18*num_geo_dofs+15+36+15*num_shap = 177+90 = 267 (for prisms)
  #endif

#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//*** ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//

#ifdef LAPLACE

	  temp4=temp1;
	  temp5=temp2;
	  temp6=temp3;

#elif defined(TEST_NUMINT)

  #ifdef USE_WORKSPACE_FOR_PDE_COEFF

	  temp4 = workspace[offset+0]  * temp1 +
	          workspace[offset+1]  * temp2 +
	          workspace[offset+2]  * temp3 +
	          workspace[offset+12] * temp9 ;
	  
	  temp5 = workspace[offset+3]  * temp1 +
	          workspace[offset+4]  * temp2 +
	          workspace[offset+5]  * temp3 +
	          workspace[offset+13] * temp9;
	  
	  temp6 = workspace[offset+6]  * temp1 +
	          workspace[offset+7]  * temp2 +
	          workspace[offset+8]  * temp3 +
	          workspace[offset+14] * temp9;
	  
	  temp7 = workspace[offset+9]  * temp1 +
	          workspace[offset+10] * temp2 +
	          workspace[offset+11] * temp3 +
	          workspace[offset+15] * temp9;
	  
    #ifdef COUNT_OPER
	  nr_access_shared += 16;
    #endif

  #else // if not USE_WORKSPACE_FOR_PDE_COEFF

	  temp4 = coeff00*temp1 + coeff01*temp2 + coeff02*temp3 + coeff03*temp9;
	  temp5 = coeff10*temp1 + coeff11*temp2 + coeff12*temp3 + coeff13*temp9;
	  temp6 = coeff20*temp1 + coeff21*temp2 + coeff22*temp3 + coeff23*temp9;
	  temp7 = coeff30*temp1 + coeff31*temp2 + coeff32*temp3 + coeff33*temp9;

  #endif // if not USE_WORKSPACE_FOR_PDE_COEFF
	
  #ifdef COUNT_OPER
	  nr_oper += 7*4;
  #endif

#elif defined(HEAT)

#endif

//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

#ifdef LOAD_VEC_COMP

  #ifdef USE_WORKSPACE_FOR_STIFF_MAT

    #ifdef COUNT_OPER
	  nr_access_shared += 2;
    #endif

	  workspace[my_elem_num*(num_dofs*(num_dofs+1)+PADDING)+num_dofs*num_dofs+idof] += (

  #else

	  load_vec[idof2] += (

  #endif

  #ifdef LAPLACE
			  //coeff00 * shp_fun_u
			     coeff00 * temp9

  #elif defined(TEST_NUMINT)

    #ifdef USE_WORKSPACE_FOR_PDE_COEFF
			     
//			     workspace[offset+16] * fun_u_derx +
//				 workspace[offset+17] * fun_u_dery +
//				 workspace[offset+18] * fun_u_derz +
//				 workspace[offset+19] * shp_fun_u

			     workspace[offset+16] * temp1 +
			     workspace[offset+17] * temp2 +
			     workspace[offset+18] * temp3 +
			     workspace[offset+19] * temp9

    #else // if not USE_WORKSPACE_FOR_PDE_COEFF

//			     coeff04 * fun_u_derx +
//				 coeff14 * fun_u_dery +
//				 coeff24 * fun_u_derz +
//				 coeff34 * shp_fun_u

			     coeff04 * temp1 +
			     coeff14 * temp2 +
			     coeff24 * temp3 +
			     coeff34 * temp9


    #endif // end if not USE_WORKSPACE_FOR_PDE_COEFF

  #elif defined(HEAT)

  #endif
			     
			     ) * vol;
	  
//	  barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
//	  if(get_group_id(0)==0)
//	  {
//	  	  printf("thread=%d,igauss=%d,idof2=%d,load_vec=%lf=coeff00=%lf x shp_fun_u=%lf, vol=%lf\n",thread_id,igauss,idof2,load_vec[idof2],coeff00,shp_fun_u,vol);
//	  }

  #ifdef COUNT_OPER
    #ifdef LAPLACE

	  nr_oper += 3;

    #elif defined(TEST_NUMINT)

      #ifdef USE_WORKSPACE_FOR_PDE_COEFF
	  nr_access_shared += 4;
      #endif
	  nr_oper += 9;

    #elif defined(HEAT)

    #endif

  #endif
    
#endif // end if computing RHS vector

//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
//-------------------------------------------------------------

	  //} // the end of using registers for u (shp_fun_u, fun_u_der.)

//-------------------------------------------------------------
// ************************* second loop over shape functions ****************************//
        for(jdof = 0; jdof < num_shap; jdof++){
	  
//-------------------------------------------------------------
//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF JDOF SHAPE FUNCTION ******//

#ifdef COMPUTE_ALL_SHAPE_FUN_DER
	  
  #ifdef USE_WORKSPACE_FOR_SHAPE_FUN

//	  SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*jdof];
//	  SCALAR fun_v_derx = workspace[my_elem_num*(3*num_shap+PADDING)+3*jdof];
//	  SCALAR fun_v_dery = workspace[my_elem_num*(3*num_shap+PADDING)+3*jdof+1];
//	  SCALAR fun_v_derz = workspace[my_elem_num*(3*num_shap+PADDING)+3*jdof+2];

      temp9 = shpfun_ref[igauss*4*num_shap+4*jdof];
	  temp1 = workspace[my_elem_num*(3*num_shap+PADDING)+3*jdof];
	  temp2 = workspace[my_elem_num*(3*num_shap+PADDING)+3*jdof+1];
	  temp3 = workspace[my_elem_num*(3*num_shap+PADDING)+3*jdof+2];


    #ifdef COUNT_OPER
	  nr_access_shared += 4;  // including 1 for constant cache
    #endif

  #else // if not USE_WORKSPACE_FOR_SHAPE_FUN
 
//	  SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*jdof];
//	  SCALAR fun_v_derx = tab_fun_u_derx[jdof];
//	  SCALAR fun_v_dery = tab_fun_u_dery[jdof];
//	  SCALAR fun_v_derz = tab_fun_u_derz[jdof];

	  temp9 = shpfun_ref[igauss*4*num_shap+4*jdof];
	  temp1 = tab_fun_u_derx[jdof];
	  temp2 = tab_fun_u_dery[jdof];
	  temp3 = tab_fun_u_derz[jdof];

    #ifdef COUNT_OPER
	  nr_access_shared += 1; // constant cache access
    #endif
	  
  #endif // end if not USE_WORKSPACE_FOR_SHAPE_FUN
	  
#else // if not COMPUTE_ALL_SHAPE_FUN_DER

	// read proper values of shape functions and their derivatives
//	SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*jdof];
//	temp1 = shpfun_ref[igauss*4*num_shap+4*jdof+1];
//	temp2 = shpfun_ref[igauss*4*num_shap+4*jdof+2];
//	temp3 = shpfun_ref[igauss*4*num_shap+4*jdof+3];

	temp1 = shpfun_ref[igauss*4*num_shap+4*jdof+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*jdof+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*jdof+3];
	
	// compute derivatives wrt global coordinates
	// 15 operations
//	SCALAR fun_v_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
//	SCALAR fun_v_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
//	SCALAR fun_v_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

	temp8 = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	temp9 = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	temp3 = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

	temp1 = temp8;
	temp2 = temp9;

	temp9 = shpfun_ref[igauss*4*num_shap+4*jdof];
	
  #ifdef COUNT_OPER
	nr_access_shared += 4; // constant cache accesses
	nr_oper += 15;
  #endif
	  
#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//********* ACTUAL FINAL CALCULATIONS FOR SM ENTRY  *********//

#ifdef USE_WORKSPACE_FOR_STIFF_MAT

  #ifdef COUNT_OPER
	nr_access_shared += 2;
  #endif

	workspace[my_elem_num*(num_dofs*(num_dofs+1)+PADDING)+idof*num_dofs+jdof] += (

#else

	stiff_mat[idof2*num_dofs+jdof] += (


#endif

#ifdef LAPLACE

//      	    temp4 * fun_v_derx +
//       	    temp5 * fun_v_dery +
//       	    temp6 * fun_v_derz

			temp4 * temp1 +
       	    temp5 * temp2 +
       	    temp6 * temp3

#elif defined(TEST_NUMINT)

//      	    temp4 * fun_v_derx +
//       	    temp5 * fun_v_dery +
//       	    temp6 * fun_v_derz +
//       	    temp7 * shp_fun_v

      	    temp4 * temp1 +
       	    temp5 * temp2 +
       	    temp6 * temp3 +
       	    temp7 * temp9

#elif defined(HEAT)

//      	    temp4 * fun_v_derx +
//       	    temp5 * fun_v_dery +
//       	    temp6 * fun_v_derz +
//       	    temp7 * shp_fun_v
				temp4 * temp1 +
				temp5 * temp2 +
				temp6 * temp3 +
				temp7 * temp9

#endif

					    ) * vol;

//	  barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
//	  if(get_group_id(0)==0)
//	  {
//		  printf("thread=%d,igauss=%d,idof2=%d,stiff_mat=%lf=temp4=%lf x fun_v_derx=%lf, vol=%lf\n",thread_id,igauss,idof2,stiff_mat[idof2*num_dofs+jdof],temp4,fun_v_derx,vol);
//	  }

#ifdef COUNT_OPER
  #ifdef LAPLACE
	nr_oper += 7;
  #elif defined(TEST_NUMINT)
	nr_oper += 9;
  #elif defined(HEAT)

  #endif
#endif

//*** THE END OF: ACTUAL FINAL CALCULATIONS FOR SM ENTRY  ***//
//-------------------------------------------------------------

	}//jdof

//******* THE END OF: first loop over shape functions *******//
//-------------------------------------------------------------

      }//idof

//******* THE END OF: second loop over shape functions *******//
//-------------------------------------------------------------

    }//gauss

// ******** THE END OF: loop over integration points ********//
//-------------------------------------------------------------

//-------------------------------------------------------------
//******** WRITING OUTPUT SM AND LV TO GLOBAL MEMORY ********//


//#ifdef ONE_EL_TWO_THREADS
//
//      barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
//
//      if(get_group_id(0)==0)
//      {
//
//		  printf("WG=%d\n",get_group_id(0));
//		  printf("Thread=%d\n",get_local_id(0));
//		  for(idof=0; idof < num_shap/2; idof++)
//			  for(jdof=0; jdof < num_shap; jdof++)
//				  printf("sm[%d]=%lf\n",idof*num_dofs+jdof, stiff_mat[idof*num_dofs+jdof]);
//
//
//		  for(idof=0; idof < num_shap/2; idof++)
//			  printf("lv[%d]=%lf\n",idof, load_vec[idof]);
//
//      }
//
//#endif



#ifdef COAL_WRITE

    offset = (element_index-my_elem_num)*(num_shap*num_shap+num_shap);
    i=0;

#ifdef ONE_EL_ONE_THREAD
  // for 1 element per thread
    for(idof=0; idof < num_shap; idof++) {
#elif defined(ONE_EL_TWO_THREADS)
  // for 1 element per 2 threads
    for(idof=0; idof < num_shap/2; idof++) {
#endif

    	for(jdof=0; jdof < num_shap; jdof++)
	  {

  #ifdef USE_WORKSPACE_FOR_STIFF_MAT

	    stiff_mat_out[offset+i*NR_ELEMS_PER_WORKGROUP+thread_id] =
	      workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+idof*num_dofs+jdof];

  #else

	    stiff_mat_out[offset+i*WORK_GROUP_SIZE+thread_id] = stiff_mat[idof*num_dofs+jdof];

//	    if(get_group_id(0)==0)
//	    {
//	    	printf("thread=%d,smo[%d]=sm[%d]=[%lf]\n",thread_id,offset+i*WORK_GROUP_SIZE+thread_id,idof*num_dofs+jdof,stiff_mat[idof*num_dofs+jdof]);
//	    }

  #endif

	    i++;
	  }
      }

  #ifdef COUNT_OPER
    nr_global_access += num_dofs*num_dofs; // we neglect shared memory accesses at this stage
  #endif


  #ifdef LOAD_VEC_COMP

//#ifdef  ONE_EL_TWO_THREADS
//  // for 1 element per 2 threads
//    if(!is_even) offset += num_shap/2;
//#endif

    offset+=(num_shap*num_shap/2)*WORK_GROUP_SIZE;

#ifdef ONE_EL_ONE_THREAD
  // for 1 element per thread
    for(i=0; i < num_shap; i++){
#elif defined(ONE_EL_TWO_THREADS)
  // for 1 element per 2 threads
    for(i=0; i < num_shap/2; i++){
#endif

      // write load vector

    #ifdef USE_WORKSPACE_FOR_STIFF_MAT

      stiff_mat_out[offset+(num_shap*num_shap+i)*NR_ELEMS_PER_WORKGROUP+thread_id] =
	workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+num_dofs*num_dofs+i];

    #else

      stiff_mat_out[offset+i*WORK_GROUP_SIZE+thread_id] = load_vec[i];

    #endif

//	    if(get_group_id(0)==0)
//	    {
//	    	printf("thread=%d,smo[%d]=lv[%d]=[%lf]\n",thread_id,offset+i*WORK_GROUP_SIZE+thread_id,i,load_vec[i]);
//	    }

    }

    #ifdef COUNT_OPER
    nr_global_access += num_dofs; // we neglect shared memory accesses at this stage
    #endif

  #endif // end if not LOAD_VEC_COMP

#else // if not coalesced write

    // write stiffness matrix - threads compute subsequent elements
    offset = element_index*(num_shap*num_shap+num_shap);
    i=0;
    for(idof=0; idof < num_shap; idof++)
      {
    	for(jdof=0; jdof < num_shap; jdof++)
	  {

  #ifdef USE_WORKSPACE_FOR_STIFF_MAT

	    stiff_mat_out[offset+i] =
	      workspace[my_elem_num*(num_dofs*(num_dofs+1)+PADDING)+idof*num_dofs+jdof];

  #else

	    stiff_mat_out[offset+i] = stiff_mat[idof*num_dofs+jdof];

  #endif

	    i++;
	  }
      }

  #ifdef COUNT_OPER
    nr_global_access += num_dofs*num_dofs; // we neglect shared memory accesses at this stage
  #endif

  #ifdef LOAD_VEC_COMP

    for(i=0; i < num_shap; i++){
      // write load vector

    #ifdef USE_WORKSPACE_FOR_STIFF_MAT

      stiff_mat_out[offset+num_shap*num_shap+i] =
	workspace[my_elem_num*(num_dofs*(num_dofs+1)+PADDING)+num_dofs*num_dofs+i];

    #else

      stiff_mat_out[offset+num_shap*num_shap+i] = load_vec[i];

    #endif

    }

    #ifdef COUNT_OPER
    nr_global_access += num_dofs; // we neglect shared memory accesses at this stage
    #endif

  #endif // end if LOAD_VEC_COMP

#endif // end if not COAL_WRITE

// *** THE END OF: WRITING OUTPUT SM AND LV TO GLOBAL MEMORY ***//
//-------------------------------------------------------------

  } // the end of loop over elements

// ************* THE END OF: LOOP OVER ELEMENTS *************//
//-------------------------------------------------------------

#ifdef COUNT_OPER

  if(get_group_id(0)==0 && get_local_id(0)==0){
    stiff_mat_out[0] = nr_oper;
    stiff_mat_out[1] = nr_access_shared;
    stiff_mat_out[2] = nr_global_access;
  }

#endif


};
