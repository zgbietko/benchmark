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

#pragma OPENCL EXTENSION cl_amd_printf : enable

//----------------------------------------------------
// TWO MASTER SWITCHES (float<->double, work_group_size)
#define FLOAT
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

#define WORK_GROUP_SIZE 128 // for GPUs
//----------------------------------------------------

#define WG_SIZE_JAC 160 //32*5

#define NR_EXEC_PARAMS 16  // size of array with execution parameters

#define num_shap 126
#define num_gauss 150
#define num_geo_dofs 6

#define num_shap_pad 128

#define NENTPT 18 //18

#define nr_parts (int)ceil((double)(num_shap*num_shap)/(NENTPT*WORK_GROUP_SIZE)) //32

//#define OUTSIDE  //1 rejestr wiecej-czasy porównywalne

#ifdef OUTSIDE 
	#define SIZE 7*18
#endif

#define EL_GEO_DAT_SIZE (3*num_geo_dofs)


#define TEST_NUMINT

  #define NR_COEFFS_SENT_PER_ELEMENT 20
  #define NR_COEFFS_SENT_PER_INT_POINT 0
  #define NR_COEFFS_IN_SM_CALCULATIONS 20

#define NR_PDE_COEFFS_SENT (NR_COEFFS_SENT_PER_ELEMENT + NR_COEFFS_SENT_PER_INT_POINT*num_gauss)

//----------------------------------------------------
// SWITCHES FOR DIFFERENT OPTIMIZATION OPTIONS !!!!!!!!!!!!!!!!!!!!!!!!

//load vector computing - not defined only for some tests
//#define LOAD_VEC_COMP

// COAL_READ - both PDE_COEFF and GEO_DATA are read in a coalesced way (using a single workspace)
// coalesced reading may be good for GPUs (requires large workspace and several barriers)
// (the workspace can be further used by GEO_DAT or PDE_COEFF or SHAPE_FUN or STIFF_MAT !!!)
#define COAL_READ

// coalesced writing requires host code to adapt to the order of data!!!
// coalesced writing should be switched on for GPUs
//#define COAL_WRITE

// COMPUTE_ALL_SHAPE_FUN_DER - to compute all shape functions and their derivatives
//                         before entering the loops over shape functions
#define COMPUTE_ALL_SHAPE_FUN_DER

//#define COUNT_OPER

// USE_WORKSPACE_FOR_SHAPE_FUN - to use shared memory for shape functions 
//                               and their derivatives during SM calculations
// otherwise - registers are used
#define USE_WORKSPACE_FOR_SHAPE_FUN

//#define USE_WORKSPACE_FOR_JACOBIAN_DATA

// FOR EACH ARCHITECTURE PADDING SHOULD BE TESTED TO DETECT SHARED MEMORY BANK CONFLICTS
#define WORKSPACE_PADDING 0
#define PADDING WORKSPACE_PADDING

// THE END OF: SWITCHES FOR DIFFERENT OPTIMIZATION OPTIONS
//----------------------------------------------------

kernel void tmr_ocl_num_int_el(
  __constant int* execution_parameters,
  __global SCALAR* shpfun_ref, // shape functions on a reference element
  __global SCALAR* el_data_in_coeff, // data for integration of NR_ELEMS_THIS_KERCALL elements
  __global SCALAR* jac_dat,
  __global SCALAR* stiff_mat_out // result of integration of NR_ELEMS_THIS_KERCALL elements
){

#ifdef COUNT_OPER
  SCALAR nr_oper=0.0;

  SCALAR nr_access_shared=0.0;
  SCALAR nr_global_access=0.0;
#endif

  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);

//----------------------------------------------------
// DEFINITIONS DEPENDENT ON OPTIMIZATION OPTIONS

#ifdef USE_WORKSPACE_FOR_JACOBIAN_DATA
	#define WORKSPACE_SIZE_FOR_JACOBIAN_DATA num_gauss*10
  	__local SCALAR workspace_jac[WORKSPACE_SIZE_FOR_JACOBIAN_DATA]; //
#else
	#define WORKSPACE_SIZE_FOR_JACOBIAN_DATA 0
#endif

#ifdef USE_WORKSPACE_FOR_SHAPE_FUN
  #define WORKSPACE_SIZE_FOR_SHAPE_FUN (3*num_shap_pad)
#else
  #define WORKSPACE_SIZE_FOR_SHAPE_FUN 0
  #ifdef COMPUTE_ALL_SHAPE_FUN_DER
  SCALAR tab_fun_u_derx[num_shap];
  SCALAR tab_fun_u_dery[num_shap];
  SCALAR tab_fun_u_derz[num_shap];
  #endif
#endif

#ifdef COAL_READ
  #define WORKSPACE_SIZE_FOR_READING NR_PDE_COEFFS_SENT
#else
  #define WORKSPACE_SIZE_FOR_READING 0
#endif

#define TMP_SIZE (WORKSPACE_SIZE_FOR_SHAPE_FUN+WORKSPACE_SIZE_FOR_JACOBIAN_DATA)

#if (TMP_SIZE) > (WORKSPACE_SIZE_FOR_READING)
  #define WORKSPACE_SIZE ((TMP_SIZE+WORKSPACE_PADDING))
#else
  #define WORKSPACE_SIZE ((WORKSPACE_SIZE_FOR_READING+WORKSPACE_PADDING))
#endif

#if (WORKSPACE_SIZE) > 0
  __local SCALAR workspace[WORKSPACE_SIZE]; //
#endif

// THE END OF: DEFINITIONS DEPENDENT ON OPTIMIZATION OPTIONS
//----------------------------------------------------

  // ASSUMPTION: one element = one thread

  int nr_elems_per_wg = execution_parameters[2];

  int ielem;
  int offset;
  int aux_offset;
  
//-------------------------------------------------------------
//******************* loop over elements processed by a thread *********************
  for(ielem = 0; ielem < nr_elems_per_wg; ielem++){

    int element_index = group_id * nr_elems_per_wg + ielem;
    int i;

//-------------------------------------------------------------
// ******************* READING INPUT DATA *********************

#ifdef COAL_READ

    offset = element_index * NR_PDE_COEFFS_SENT;

    //barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

    if(thread_id<NR_PDE_COEFFS_SENT)
	{
    	workspace[thread_id] =	el_data_in_coeff[offset+thread_id];
	}

    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

		offset=0;

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
	//    SCALAR coeff03=workspace[offset+12];
	//    SCALAR coeff13=workspace[offset+13];
	//    SCALAR coeff23=workspace[offset+14];
		SCALAR coeff33=workspace[offset+15];
		SCALAR coeff04=workspace[offset+16];
		SCALAR coeff14=workspace[offset+17];
		SCALAR coeff24=workspace[offset+18];
		SCALAR coeff34=workspace[offset+19];

	#ifdef USE_WORKSPACE_FOR_JACOBIAN_DATA
		barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
		#ifdef COAL_WRITE
			//int element_index = group_id * nr_elems_per_thread * WG_SIZE_JAC + ielem * WG_SIZE_JAC +thread_id ;
			//jac_dat[offset+i*WG_SIZE_JAC+thread_id];
			for(i=0;i<floor(num_gauss/WORK_GROUP_SIZE);i++)
			{
				workspace_jac[i*WORK_GROUP_SIZE+thread_id]=//?
			}
			int res=num_gauss-floor(num_gauss/WORK_GROUP_SIZE)*WORK_GROUP_SIZE;
			if(thread_id<res)
				workspace_jac[floor(num_gauss/WORK_GROUP_SIZE)*WORK_GROUP_SIZE+thread_id]=//?
		#else
			offset = element_index*WG_SIZE_JAC*10;

			for(i=0;i<floor((double)((10*num_gauss)/WORK_GROUP_SIZE));i++)
			{
				workspace_jac[i*WORK_GROUP_SIZE+thread_id]=jac_dat[offset+i*WORK_GROUP_SIZE+thread_id];
			}
			int res=num_gauss*10 - i*WORK_GROUP_SIZE;
			if(thread_id<res)
			{
				workspace_jac[i*WORK_GROUP_SIZE+thread_id]=jac_dat[offset+i*WORK_GROUP_SIZE+thread_id];
			}

			barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

		#endif
	#endif

#endif // end if not COAL_READ

// ******* THE END OF: READING INPUT DATA *********************
//-------------------------------------------------------------

#if (WORKSPACE_SIZE) > 0
    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
#endif

#ifdef OUTSIDE

    SCALAR stiff_mat[SIZE];
    		for(i=0;i<SIZE;i++)
    			stiff_mat[i]=zero;

    #ifdef LOAD_VEC_COMP
    		//do poprawy
//    	SCALAR l;
//    	for(i=0;i<NENTPT;i++)
//    	{
//    			idof=(aux_offset-jdof*num_shap)+i;
//    			if (idof==jdof)
//    				l=zero;
//    	}
    #endif

#endif

int ipart;

    for(ipart=0;ipart<nr_parts;ipart++)
    {

    	aux_offset = WORK_GROUP_SIZE*ipart*NENTPT+thread_id*NENTPT;
    	int jdof=aux_offset/num_shap;
    	int idof=(aux_offset-jdof*num_shap);

if(jdof<num_shap)
{

	#ifndef OUTSIDE

			SCALAR stiff_mat[NENTPT];
			for(i=0;i<NENTPT;i++)
				stiff_mat[i]=zero;

			//		if(ielem==0&&group_id==0&&ipart<4)
		//		printf("thread_id=%d,aux_offset=%d,ipart=%d,nr_parts=%d,jdof=%d,idof=%d\n",thread_id,aux_offset,ipart,nr_parts,jdof,idof);

		#ifdef LOAD_VEC_COMP
			SCALAR l;
			for(i=0;i<NENTPT;i++)
			{
					idof=(aux_offset-jdof*num_shap)+i;
					if (idof==jdof)
						l=zero;
			}
		#endif

	#endif

//-------------------------------------------------------------
//************************* LOOP OVER INTEGRATION POINTS ************************//

    // in a loop over gauss points
    int igauss;

    for(igauss = 0; igauss < num_gauss; igauss++){

    	SCALAR temp1 = zero;
    	SCALAR temp2 = zero;
    	SCALAR temp3 = zero;
    	SCALAR temp4 = zero;
    	SCALAR temp5 = zero;
    	SCALAR temp6 = zero;
    	SCALAR temp7 = zero;
    	SCALAR jac[10];

		#ifndef USE_WORKSPACE_FOR_JACOBIAN_DATA

			#ifndef COAL_WRITE
    			offset=element_index*WG_SIZE_JAC*10+igauss*10;
    			for(i=0;i<10;i++)
    				jac[i]=jac_dat[offset+i];
			#else
    			//dorobic
			#endif

		#else
			#ifndef COAL_WRITE
    			for(i=0;i<10;i++)
    			    jac[i]=workspace_jac[igauss*10+i];
			#else
    			//dorobic
			#endif
		#endif
//-------------------------------------------------------------
//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//


#ifdef COMPUTE_ALL_SHAPE_FUN_DER

 //************ loop for computing ALL shape function values at integration point **********//

if(thread_id<num_shap)
{
	// read proper values of shape functions and their derivatives
	temp1 = shpfun_ref[igauss*4*num_shap+4*thread_id+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*thread_id+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*thread_id+3];

  #ifdef USE_WORKSPACE_FOR_SHAPE_FUN

	workspace[3*thread_id]   = temp1*jac[0]+temp2*jac[3]+temp3*jac[6];
	workspace[3*thread_id+1] = temp1*jac[1]+temp2*jac[4]+temp3*jac[7];
	workspace[3*thread_id+2] = temp1*jac[2]+temp2*jac[5]+temp3*jac[8];

	barrier(CLK_LOCAL_MEM_FENCE);

#ifdef COUNT_OPER
	nr_oper += 15;
#endif

  #else  // if not USE_WORKSPACE_FOR_SHAPE_FUN

	tab_fun_u_derx[thread_id] = temp1*jac[0]+temp2*jac[3]+temp3*jac[6];
	tab_fun_u_dery[thread_id] = temp1*jac[1]+temp2*jac[4]+temp3*jac[7];
	tab_fun_u_derz[thread_id] = temp1*jac[2]+temp2*jac[5]+temp3*jac[8];

  #endif // end if not USE_WORKSPACE_FOR_SHAPE_FUN

} // end loop over shape functions for which global derivatives were computed

#endif // end if COMPUTE_ALL_SHAPE_FUN_DER

//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//***** SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS *****//

//-------------------------------------------------------------
//********************* first loop over shape functions ***********************//

//-------------------------------------------------------------
//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ******//

#ifdef COMPUTE_ALL_SHAPE_FUN_DER

  #ifdef USE_WORKSPACE_FOR_SHAPE_FUN

	  SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*jdof];
	  SCALAR fun_u_derx = workspace[3*jdof];
	  SCALAR fun_u_dery = workspace[3*jdof+1];
	  SCALAR fun_u_derz = workspace[3*jdof+2];

  #else // if not USE_WORKSPACE_FOR_SHAPE_FUN

	  // read proper values of shape functions and their derivatives
          SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*jdof];
          SCALAR fun_u_derx = tab_fun_u_derx[jdof];
          SCALAR fun_u_dery = tab_fun_u_dery[jdof];
          SCALAR fun_u_derz = tab_fun_u_derz[jdof];

  #endif // end if not USE_SHAPE_FUN_WORKSPACE

#else // if not COMPUTE_ALL_SHAPE_FUN_DER

	  // read proper values of shape functions and their derivatives
	  SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*jdof];
	  temp1 = shpfun_ref[igauss*4*num_shap+4*jdof+1];
	  temp2 = shpfun_ref[igauss*4*num_shap+4*jdof+2];
	  temp3 = shpfun_ref[igauss*4*num_shap+4*jdof+3];

	  // compute derivatives wrt global coordinates
	  // 15 operations
	  SCALAR fun_u_derx = temp1*jac[0] + temp2*jac[3] + temp3*jac[6];
	  SCALAR fun_u_dery = temp1*jac[1] + temp2*jac[4] + temp3*jac[7];
	  SCALAR fun_u_derz = temp1*jac[2] + temp2*jac[5] + temp3*jac[8];

  #ifdef COUNT_OPER
	  nr_oper += 15; // after optimization
  #endif

#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//*** ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//

	  temp4 = coeff00*fun_u_derx + coeff01*fun_u_dery + coeff02*fun_u_derz; //+ coeff03*shp_fun_u;
	  temp5 = coeff10*fun_u_derx + coeff11*fun_u_dery + coeff12*fun_u_derz; //+ coeff13*shp_fun_u;
	  temp6 = coeff20*fun_u_derx + coeff21*fun_u_dery + coeff22*fun_u_derz; //+ coeff23*shp_fun_u;
	  temp7 = coeff30*fun_u_derx + coeff31*fun_u_dery + coeff32*fun_u_derz + coeff33*shp_fun_u;

  #ifdef COUNT_OPER
	  nr_oper += 5*4+2;
  #endif

//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

	  for(i=0;i<NENTPT;i++)  //NENTPT loop
	  {
	  	idof=(aux_offset-jdof*num_shap)+i;

//-------------------------------------------------------------
//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

#ifdef LOAD_VEC_COMP

	  if(idof==jdof)
	  {
		  l += (
 			     coeff04 * fun_u_derx +
			     coeff14 * fun_u_dery +
			     coeff24 * fun_u_derz +
			     coeff34 * shp_fun_u

		  ) * jac[9];

  #ifdef COUNT_OPER
      nr_oper += 9;
  #endif

	  }//end idof==jdof

#endif // end if computing RHS vector

//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
// ************************* second loop over shape functions ****************************//

//-------------------------------------------------------------
//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF JDOF SHAPE FUNCTION ******//

#ifdef COMPUTE_ALL_SHAPE_FUN_DER

  #ifdef USE_WORKSPACE_FOR_SHAPE_FUN

	  SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*idof];
	  SCALAR fun_v_derx = workspace[3*idof];
	  SCALAR fun_v_dery = workspace[3*idof+1];
	  SCALAR fun_v_derz = workspace[3*idof+2];

  #else // if not USE_WORKSPACE_FOR_SHAPE_FUN

	  SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*idof];
	  SCALAR fun_v_derx = tab_fun_u_derx[idof];
	  SCALAR fun_v_dery = tab_fun_u_dery[idof];
	  SCALAR fun_v_derz = tab_fun_u_derz[idof];

  #endif // end if not USE_WORKSPACE_FOR_SHAPE_FUN

#else // if not COMPUTE_ALL_SHAPE_FUN_DER

	// read proper values of shape functions and their derivatives
	SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*idof];
	temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];

	// compute derivatives wrt global coordinates
	// 15 operations
	SCALAR fun_v_derx = temp1*jac[0] + temp2*jac[3] + temp3*jac[6];
	SCALAR fun_v_dery = temp1*jac[1] + temp2*jac[4] + temp3*jac[7];
	SCALAR fun_v_derz = temp1*jac[2] + temp2*jac[5] + temp3*jac[8];

  #ifdef COUNT_OPER
	nr_oper += 15;
  #endif

#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//********* ACTUAL FINAL CALCULATIONS FOR SM ENTRY  *********//

	#ifdef OUTSIDE
		stiff_mat[ipart*NENTPT+i] += (
	#else
		stiff_mat[i] += (
	#endif

      	    temp4 * fun_v_derx +
       	    temp5 * fun_v_dery +
       	    temp6 * fun_v_derz +
       	    temp7 * shp_fun_v

					    ) * jac[9];

#ifdef COUNT_OPER
  	nr_oper += 9;
#endif

      }//idof//nentpt

//******* THE END OF: second loop over shape functions *******//
//-------------------------------------------------------------
    }//gauss

#ifndef OUTSIDE

    aux_offset = (group_id*nr_elems_per_wg+ielem)*(num_shap*num_shap+num_shap)+ipart*WORK_GROUP_SIZE*NENTPT;

    for(i=0;i<NENTPT;i++)
    {
    	stiff_mat_out[aux_offset+i*NENTPT+thread_id] = stiff_mat[i];//jdof*1000+idof;//stiff_mat[i];
    }

	#ifdef LOAD_VEC_COMP

	 	 aux_offset = (group_id*nr_elems_per_wg+ielem)*(num_shap*num_shap+num_shap);

	    if(idof==jdof)
	    {
	    	stiff_mat_out[aux_offset+num_shap*num_shap+thread_id] = l;
	    }


	#endif

#endif

    //barrier(CLK_GLOBAL_MEM_FENCE);

}//end if jdof<num_shap

    }//end ipart

// ******** THE END OF: loop over integration points ********//
//-------------------------------------------------------------

#ifdef OUTSIDE

    aux_offset = (group_id*nr_elems_per_wg+ielem)*(num_shap*num_shap+num_shap);

    for(i=0;i<SIZE;i++)
    {
     	stiff_mat_out[aux_offset+i*SIZE+thread_id] = stiff_mat[i];//jdof*1000+idof;//stiff_mat[i];
    }

	#ifdef LOAD_VEC_COMP
    //not implemented
//	 	 aux_offset = (group_id*nr_elems_per_wg+ielem)*(num_shap*num_shap+num_shap);
//
//	    if(idof==jdof)
//	    {
//	    	stiff_mat_out[aux_offset+num_shap*num_shap+thread_id] = l;
//	    }

	#endif

#endif

  } // the end of loop over elements

// ************* THE END OF: LOOP OVER ELEMENTS *************//
//-------------------------------------------------------------

#ifdef COUNT_OPER

  if(group_id==0)
	  stiff_mat_out[thread_id] = nr_oper;

#endif


};


kernel void tmr_ocl_prepare_jacobian_data(
  // execution_parameters can be read directly from constant memory, assuming it is cached and
  // further accesses are realized from cache
  __constant int* execution_parameters,
  //__global int* execution_parameters,
  // gauss data can be read directly from constant memory, assuming it is cached and
  // further accesses are realized from cache
  __constant SCALAR* gauss_dat, // integration points data of elements having given p
  __global SCALAR* geo_in,

  __global SCALAR* jac_dat // integration points data of elements having given p
  // shape function values can be read directly from constant memory, assuming it is cached and
  // further accesses are realized from cache
//  __constant SCALAR* shpfun_ref, // shape functions on a reference element
//  //__global SCALAR* shpfun_ref, // shape functions on a reference element
//  __global SCALAR* el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
//  // TODO!!!!!!!!!!!!!!!!!!
//  //__global SCALAR* el_data_in_geo, // geo data for integration of NR_ELEMS_THIS_KERCALL elements
//  //__global SCALAR* el_data_in_coeff, // coeff data for integration of elements
//  __global SCALAR* stiff_mat_out // result of integration of NR_ELEMS_THIS_KERCALL elements
){


  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);
  //const int work_group_size = get_local_size(0);
  //const int nr_work_groups = get_num_groups(0);

//----------------------------------------------------
// DEFINITIONS DEPENDENT ON OPTIMIZATION OPTIONS

#ifdef USE_WORKSPACE_FOR_GEO_DATA
  #define WORKSPACE_SIZE_FOR_GEO_DATA (3*num_geo_dofs)
#else
  #define WORKSPACE_SIZE_FOR_GEO_DATA 0
  SCALAR geo_dat[3*num_geo_dofs];
#endif

#ifdef COAL_READ
  #define WORKSPACE_SIZE_FOR_READING_J (3*num_geo_dofs)
#else
  #define WORKSPACE_SIZE_FOR_READING_J 0
#endif

#define TMP_SIZE_J (WORKSPACE_SIZE_FOR_GEO_DATA)

#if (TMP_SIZE_J) > (WORKSPACE_SIZE_FOR_READING_J)
  #define WORKSPACE_SIZE_J ((TMP_SIZE_J+WORKSPACE_PADDING)*WG_SIZE_JAC)
#else
  #define WORKSPACE_SIZE_J ((WORKSPACE_SIZE_FOR_READING_J+WORKSPACE_PADDING)*WG_SIZE_JAC)
#endif

#if (WORKSPACE_SIZE_J) > 0
  __local SCALAR workspace[WORKSPACE_SIZE_J]; //
#endif

// THE END OF: DEFINITIONS DEPENDENT ON OPTIMIZATION OPTIONS
//----------------------------------------------------

  // ASSUMPTION: one element = one thread

  int nr_elems_per_thread = execution_parameters[0];
  int nr_elems_this_kercall = execution_parameters[1];

  int ielem;
  int offset;

//-------------------------------------------------------------
//******************* loop over elements processed by a thread *********************
  for(ielem = 0; ielem < nr_elems_per_thread; ielem++){

	int element_index = group_id * nr_elems_per_thread * WG_SIZE_JAC +
											   ielem * WG_SIZE_JAC +
											   	   	   	   	   thread_id ;
	int i;


//-------------------------------------------------------------
// ******************* READING INPUT DATA *********************

#ifdef COAL_READ

    // after rewriting PDE_COEFF to registers we read GEO_DATA to workspace
    offset = (element_index-thread_id)*(EL_GEO_DAT_SIZE);
    barrier(CLK_LOCAL_MEM_FENCE);
    // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
    for(i = 0; i < EL_GEO_DAT_SIZE; i++){

      workspace[i*WG_SIZE_JAC+thread_id] =
	                         geo_in[offset+i*WG_SIZE_JAC+thread_id];
	// TODO !!!!
	// el_data_in_geo[offset+i*WORK_GROUP_SIZE+thread_id];

    }
    barrier(CLK_LOCAL_MEM_FENCE);

	#ifndef USE_WORKSPACE_FOR_GEO_DATA

		  // we rewrite geo_data to registers
		  offset=thread_id*EL_GEO_DAT_SIZE;

		  for(i=0;i<num_geo_dofs;i++){

			geo_dat[3*i] = workspace[offset+3*i];  //node coor
			geo_dat[3*i+1] = workspace[offset+3*i+1];
			geo_dat[3*i+2] = workspace[offset+3*i+2];

		  }
	#endif

		  barrier(CLK_LOCAL_MEM_FENCE);

#else //if not coal read

	#ifdef USE_WORKSPACE_FOR_GEO_DATA

	  offset = (element_index-thread_id)*(EL_GEO_DAT_SIZE);

	  //barrier(CLK_LOCAL_MEM_FENCE); // I don't know why but without barrier here, one per ten runs gives bad results

	  // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
	  for(i = 0; i < EL_GEO_DAT_SIZE; i++){

		workspace[i*WG_SIZE_JAC+thread_id] =
					  el_data_in[offset+i*WG_SIZE_JAC+thread_id];
		// TODO !!!!!!!!!
		// el_data_in_geo[offset+i*WORK_GROUP_SIZE+thread_id];

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

#endif // end if not COAL_READ


// ******* THE END OF: READING INPUT DATA *********************
//-------------------------------------------------------------

#if (WORKSPACE_SIZE) > 0
    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
#endif


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
      SCALAR vol = gauss_dat[4*igauss+3]; // vol = weight
      //SCALAR vol = weight_gauss; // vol = weight CONSTANT FOR LINEAR PRISM!!!

#ifdef COUNT_OPER
    //nr_access_shared += 4;
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

      SCALAR jac[10];
      for(i=0;i<10;i++)
    	  jac[i]=zero;
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

	/* Jacobian matrix J */
#ifdef USE_WORKSPACE_FOR_GEO_DATA
	offset=thread_id*EL_GEO_DAT_SIZE;
#endif

	for(i=0;i<num_geo_dofs;i++){

	  jac[1] = jac_data[i];
	  jac[2] = jac_data[num_geo_dofs+i];
	  jac[3] = jac_data[2*num_geo_dofs+i];

#ifdef USE_WORKSPACE_FOR_GEO_DATA

	  jac[4] = workspace[offset+3*i];  //node coor
	  jac[5] = workspace[offset+3*i+1];
	  jac[6] = workspace[offset+3*i+2];

#else // if not USE_WORKSPACE_FOR_GEO_DATA (geo data in registers)

	  jac[4] = geo_dat[3*i];  //node coor
	  jac[5] = geo_dat[3*i+1];
	  jac[6] = geo_dat[3*i+2];

#endif // end if not USE_GEO_DAT_WORKSPACE

	  temp1 += jac[4] * jac[1];
	  temp2 += jac[4] * jac[2];
	  temp3 += jac[4] * jac[3];
	  temp4 += jac[5] * jac[1];
	  temp5 += jac[5] * jac[2];
	  temp6 += jac[5] * jac[3];
	  temp7 += jac[6] * jac[1];
	  temp8 += jac[6] * jac[2];
	  temp9 += jac[6] * jac[3];

	}

      } // the end of scope for jac_data

      jac[0] = (temp5*temp9 - temp8*temp6);
      jac[1] = (temp8*temp3 - temp2*temp9);
      jac[2] = (temp2*temp6 - temp3*temp5);

      daux = temp1*jac[0] + temp4*jac[1] + temp7*jac[2];

      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
      jac[9] = vol * daux; // vol = weight * det J

      faux = one/daux;

      jac[0] *= faux;
      jac[1] *= faux;
      jac[2] *= faux;

      jac[3] = (temp6*temp7 - temp4*temp9)*faux;
      jac[4] = (temp1*temp9 - temp7*temp3)*faux;
      jac[5] = (temp3*temp4 - temp1*temp6)*faux;

      jac[6] = (temp4*temp8 - temp5*temp7)*faux;
      jac[7] = (temp2*temp7 - temp1*temp8)*faux;
      jac[8] = (temp1*temp5 - temp2*temp4)*faux;

//************* THE END OF: JACOBIAN TERMS CALCULATIONS *************************//
//-------------------------------------------------------------

      barrier(CLK_LOCAL_MEM_FENCE);

#ifdef COAL_WRITE

    // write jac_data
    offset = (element_index-thread_id)*(10);

    for(i=0;i<10;i++)
    	jac_dat[offset+i*WG_SIZE_JAC+thread_id]=jac[i];

#else
    barrier(CLK_GLOBAL_MEM_FENCE);
    offset = (element_index)*(WG_SIZE_JAC)*10+igauss*10;
    for(i=0;i<10;i++)
     	jac_dat[offset+i]=jac[i];
//    if(group_id==0 && thread_id==79 && igauss == 0)
//    {
//    	printf("igauss=%d,wg=%d,elem_ind=%d,thread=%d,offset=%d\n",igauss,group_id,element_index,thread_id,offset);
//    	for(i=0;i<10;i++)
//    	{
//    		printf("jac[%d]=%lf\n",i,jac[i]);
//    		printf("jac_dat[%d]=%lf\n",offset+i,jac_dat[offset+i]);
//    	}
//    }

#endif // end if not COAL_WRITE

    barrier(CLK_GLOBAL_MEM_FENCE);
//		if(group_id==0 && thread_id==0)
//		{
//		  printf("igauss=%d,AAAjac_dat[169]=%lf\n",igauss,jac_dat[169]);
//		}

    }//igauss



  }//ielem

  //barrier(CLK_GLOBAL_MEM_FENCE);
};
