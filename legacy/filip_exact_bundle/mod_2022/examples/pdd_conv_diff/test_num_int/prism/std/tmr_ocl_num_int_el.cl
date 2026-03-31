//!!!!!!!!!!! TEST DIFFERENT CONFIGURATIONS OF SHARED / L1 MEMORY

//!!!!!!!!!!! LOOP UNROLLING = REGISTER BLOCKING FOR INNERMOST LOOPS ??????????

// optimize storage in shared - subsequent threads read subsequent locations:
// - for shape_fun_ref
// - for shape_fun_workspace
// - for coeff_workspace
// alternatively - introduce offsets in accessing these tables (off_shape, off_coeff)


#if defined(cl_amd_fp64)
  #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#elif defined(cl_khr_fp64)
  #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#else
  #error "Double precision floating point not supported by OpenCL implementation."
#endif


//#define FLOAT
#ifdef FLOAT
  #define SCALAR float
  #define zero 0.0f
  #define one 1.0f
  #define two 2.0f
  #define half 0.5f
#else
  #define SCALAR double
  #define zero 0.0
  #define one 1.0
  #define two 2.0
  #define half 0.5
#endif

//load vector computing
#define LOAD_VEC_COMP

//#define USE_PDE_COEFF_WORKSPACE // at least one of the two PDE_COEFF options must be active
#define USE_REGISTERS_FOR_COEFF

#define USE_GEO_DAT_WORKSPACE

//#define USE_SHAPE_FUN_WORKSPACE // saves registers because frees JAC automatic unknowns
#define USE_REGISTERS_FOR_SHAPE_FUN
//#define USE_SHAPE_FUN_REF_DIRECTLY

//#define STIFF_MAT_IN_SHARED

//#define NO_JACOBIAN_CALCULATIONS

#define COAL_READ
#define COAL_WRITE

#define CONSTANT_COEFF

//#define COUNT_OPER

//#define WORK_GROUP_SIZE 16 // for XEON_PHI
#define WORK_GROUP_SIZE 64 // for GPUs
//#define WORK_GROUP_SIZE 8 // for CPUs

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
#define num_dofs (num_gauss*nreq)
#define num_geo_dofs 6

// FOR EACH ARCHITECTURE OFFSET SHOULD BE TESTED TO DETECT SHARED MEMORY BANK CONFLICTS
#define GEO_DAT_OFFSET 0
#define EL_GEO_DAT_SIZE (3*num_geo_dofs+GEO_DAT_OFFSET)

// J_AND_DETJ_SIZE=10 - for NOJAC variants
//#define J_AND_DETJ_SIZE 10

// the number of coefficients sent for elements
// either coefficients constant for the whole element
// or different for every integration point

#define HEAT
#ifdef LAPLACE
  #define NR_COEFFS_PER_ELEMENT 0
  #define NR_COEFFS_PER_INT_POINT 3
#elif defined TEST_NUMINT
  #define NR_COEFFS_PER_ELEMENT 20
  #define NR_COEFFS_PER_INT_POINT 0
#elif defined HEAT
  #define NR_COEFFS_PER_ELEMENT (num_dofs+1)
  #define NR_COEFFS_PER_INT_POINT 5
#endif

#define NR_PDE_COEFFS_SENT (NR_COEFFS_PER_ELEMENT + NR_COEFFS_PER_INT_POINT*num_gauss)

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
  __global SCALAR* el_data_in_geo, // geo data for integration of NR_ELEMS_THIS_KERCALL elements
  __global SCALAR* el_data_in_coeff, // coeff data for integration of elements
  __global SCALAR* stiff_mat_out // result of integration of NR_ELEMS_THIS_KERCALL elements
){

#ifdef COUNT_OPER
SCALAR nr_oper=0.0;
SCALAR nr_access_shared=0.0;
SCALAR nr_global_access=0.0;
#endif

  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);
  //const int work_group_size = get_local_size(0);
  const int nr_work_groups = get_num_groups(0);

#ifdef USE_PDE_COEFF_WORKSPACE
#define PDE_COEFF_WORKSPACE_SIZE (NR_PDE_COEFFS_SENT*WORK_GROUP_SIZE)
  __local SCALAR pde_coeff_workspace[PDE_COEFF_WORKSPACE_SIZE]; //
#endif

#ifdef USE_GEO_DAT_WORKSPACE
  #define GEO_DAT_WORKSPACE_SIZE (EL_GEO_DAT_SIZE*WORK_GROUP_SIZE)
  __local SCALAR geo_dat_workspace[GEO_DAT_WORKSPACE_SIZE]; // geo dofs
#endif

#ifdef USE_SHAPE_FUN_WORKSPACE
  #ifdef USE_SHAPE_FUN_REF_DIRECTLY  
    #define SHAPE_FUN_WORKSPACE_SIZE (3*num_shap*WORK_GROUP_SIZE)
  #else
    #define SHAPE_FUN_WORKSPACE_SIZE (4*num_shap*WORK_GROUP_SIZE)
  #endif
  __local SCALAR shape_fun_workspace[SHAPE_FUN_WORKSPACE_SIZE];
#endif

#ifdef STIFF_MAT_IN_SHARED
  #define STIFF_MAT_WORKSPACE_SIZE (WORK_GROUP_SIZE*num_dofs*(num_dofs+1))
  __local SCALAR stiff_mat_workspace[STIFF_MAT_WORKSPACE_SIZE];
#endif

  // ASSUMPTION: one element = one thread

  int nr_elems_per_thread = execution_parameters[0];
  int nr_elems_this_kercall = execution_parameters[1];

  int ielem;
  int offset;


//******************* loop over elements processed by a thread *********************
  for(ielem = 0; ielem < nr_elems_per_thread; ielem++){

    int element_index = group_id * nr_elems_per_thread * WORK_GROUP_SIZE +
                                                 ielem * WORK_GROUP_SIZE +
                                                               thread_id ;
    int i;

#ifdef COAL_READ

    offset= (element_index - thread_id) * NR_PDE_COEFFS_SENT;

#else // if not COAL_READ

    offset= element_index * NR_PDE_COEFFS_SENT;

#endif // end if not COAL_READ


#ifdef USE_PDE_COEFF_WORKSPACE

    // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
    for(i=0;i<NR_PDE_COEFFS_SENT;i++) {

    #ifdef COAL_READ

      pde_coeff_workspace[i*WORK_GROUP_SIZE+thread_id] = 
	el_data_in_coeff[offset+i*WORK_GROUP_SIZE+thread_id];

    #else

      pde_coeff_workspace[thread_id*(NR_PDE_COEFFS_SENT)+i] =
	el_data_in_coeff[offset+i];

    #endif

#ifdef COUNT_OPER
    nr_global_access += 1;
#endif

    }

#else // if not USE_PDE_COEFF_WORKSPACE

// combined assumptions: if data are not CONSTANT they are assumed to be non-linear
  #ifdef CONSTANT_COEFF  

    #ifdef USE_REGISTERS_FOR_COEFF 

    offset= element_index * NR_PDE_COEFFS_SENT;

    SCALAR coeff00=el_data_in_coeff[offset+0];
    SCALAR coeff01=el_data_in_coeff[offset+1];
    SCALAR coeff02=el_data_in_coeff[offset+2];
    SCALAR coeff10=el_data_in_coeff[offset+3];
    SCALAR coeff11=el_data_in_coeff[offset+4];
    SCALAR coeff12=el_data_in_coeff[offset+5];
    SCALAR coeff20=el_data_in_coeff[offset+6];
    SCALAR coeff21=el_data_in_coeff[offset+7];
    SCALAR coeff22=el_data_in_coeff[offset+8];
    SCALAR coeff30=el_data_in_coeff[offset+9];
    SCALAR coeff31=el_data_in_coeff[offset+10];
    SCALAR coeff32=el_data_in_coeff[offset+11];
    SCALAR coeff03=el_data_in_coeff[offset+12];
    SCALAR coeff13=el_data_in_coeff[offset+13];
    SCALAR coeff23=el_data_in_coeff[offset+14];
    SCALAR coeff33=el_data_in_coeff[offset+15];
    SCALAR coeff04=el_data_in_coeff[offset+16];
    SCALAR coeff14=el_data_in_coeff[offset+17];
    SCALAR coeff24=el_data_in_coeff[offset+18];
    SCALAR coeff34=el_data_in_coeff[offset+19];

#ifdef COUNT_OPER
    nr_global_access += 20;
#endif


    #endif // end if  USE_REGISTERS_FOR_COEFF

  #endif // end if CONSTANT_COEFF (not computed for each integration point separately)

#endif // end if not USE_PDE_COEFF_WORKSPACE


#ifdef USE_GEO_DAT_WORKSPACE

    // read geometry data - each thread reads  EL_GEO_DAT_SIZE entries
    // but the entries are not from a single element - the whole array is read
    // and then threads use entries for their elements
  #ifdef COAL_READ
    offset = (element_index-thread_id)*(EL_GEO_DAT_SIZE);
  #else
    offset = element_index*(EL_GEO_DAT_SIZE);
  #endif

    // TRY TO UNROLL THIS LOOP TO INCREASE MEMORY PARALLELISM !!!
    for(i = 0; i < EL_GEO_DAT_SIZE; i++){

      // we should read in coalesced way but write to shared memory in element order

  #ifdef COAL_READ
      geo_dat_workspace[i*WORK_GROUP_SIZE+thread_id] = el_data_in_geo[offset+i*WORK_GROUP_SIZE+thread_id];
  #else
      geo_dat_workspace[thread_id*EL_GEO_DAT_SIZE+i] = el_data_in_geo[offset+i];
  #endif

    }

#ifdef COUNT_OPER
    nr_global_access += EL_GEO_DAT_SIZE;
#endif

#endif // end if used geo_dat workspace


//******************** INITIALIZING SM AND LV IN SHARED MEMORY ******************//

#ifdef STIFF_MAT_IN_SHARED

    // stiff_mat_workspace holds SM and LV
    for(i = 0; i < num_dofs*(num_dofs+1); i++) {
      stiff_mat_workspace[thread_id*num_dofs*(num_dofs+1)+i] = zero;
    }

#ifdef COUNT_OPER
    nr_access_shared += num_dofs*(num_dofs+1);
#endif

#else

    SCALAR stiff_mat[num_dofs*num_dofs];
    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = zero;
  #ifdef LOAD_VEC_COMP
    SCALAR load_vec[num_dofs];
    for(i = 0; i < num_dofs; i++) load_vec[i] = zero;
  #endif

#endif // end if  STIFF_MAT_IN_SHARED


    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

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

#ifdef COUNT_OPER
    nr_access_shared += 4;
#endif


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

#ifdef USE_SHAPE_FUN_WORKSPACE
      { // block to indicate the scope of jac_x registers
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
 //nr_oper += 25; // without optimization
 nr_oper += 8; // after optimization? + 4 sign changes
#endif

	temp1=zero, temp2=zero, temp3=zero;
	temp4=zero, temp5=zero, temp6=zero;
	temp7=zero, temp8=zero, temp9=zero;

	/* Jacobian matrix J */
#ifdef USE_GEO_DAT_WORKSPACE
	offset=thread_id*EL_GEO_DAT_SIZE;
#else
	offset=element_index*(EL_GEO_DAT_SIZE);
#endif

	for(i=0;i<num_geo_dofs;i++){

	  jac_1 = jac_data[i];
	  jac_2 = jac_data[num_geo_dofs+i];
	  jac_3 = jac_data[2*num_geo_dofs+i];

#ifdef USE_GEO_DAT_WORKSPACE

	  jac_4 = geo_dat_workspace[offset+3*i];  //node coor
	  jac_5 = geo_dat_workspace[offset+3*i+1];
	  jac_6 = geo_dat_workspace[offset+3*i+2];

#ifdef COUNT_OPER
    nr_access_shared += 3;
#endif

#else // if not USE_GEO_DAT_WORKSPACE

	  jac_4 = el_data_in_geo[offset+3*i];  //node coor
	  jac_5 = el_data_in_geo[offset+3*i+1];
	  jac_6 = el_data_in_geo[offset+3*i+2];

#ifdef COUNT_OPER
    nr_global_access += 3;
#endif

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
 nr_oper += 18*num_geo_dofs; // after optimization?
#endif


      daux = temp1*(temp5*temp9-temp8*temp6);
      daux += temp4*(temp8*temp3-temp2*temp9);
      daux += temp7*(temp2*temp6-temp5*temp3);

      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
      vol *= daux; // vol = weight * det J

      faux = one/daux;

      jac_0 = (temp5*temp9 - temp8*temp6)*faux;
      jac_1 = (temp8*temp3 - temp2*temp9)*faux;
      jac_2 = (temp2*temp6 - temp3*temp5)*faux;

      jac_3 = (temp6*temp7 - temp4*temp9)*faux;
      jac_4 = (temp1*temp9 - temp7*temp3)*faux;
      jac_5 = (temp3*temp4 - temp1*temp6)*faux;

      jac_6 = (temp4*temp8 - temp5*temp7)*faux;
      jac_7 = (temp2*temp7 - temp1*temp8)*faux;
      jac_8 = (temp1*temp5 - temp2*temp4)*faux;

#ifdef COUNT_OPER
 nr_oper += 15+36; // after optimization?
 // total: 13+5+18*num_geo_dofs+15+36 = 177 (for prisms)
#endif


#ifdef USE_SHAPE_FUN_WORKSPACE

 //************ loop for computing ALL shape function values at integration point **********//
      for(idof = 0; idof < num_shap; idof++){

	// read proper values of shape functions and their derivatives
	temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];

	// compute derivatives wrt global coordinates
	// 15 operations

  #ifdef USE_SHAPE_FUN_REF_DIRECTLY

	shape_fun_workspace[thread_id*3*num_shap+3*idof]   = temp1*jac_0+temp2*jac_3+temp3*jac_6;
	shape_fun_workspace[thread_id*3*num_shap+3*idof+1] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
	shape_fun_workspace[thread_id*3*num_shap+3*idof+2] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

  #else

	shape_fun_workspace[thread_id*4*num_shap+4*idof] = shpfun_ref[igauss*4*num_shap+4*idof];
	shape_fun_workspace[thread_id*4*num_shap+4*idof+1] = temp1*jac_0+temp2*jac_3+temp3*jac_6;
	shape_fun_workspace[thread_id*4*num_shap+4*idof+2] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
	shape_fun_workspace[thread_id*4*num_shap+4*idof+3] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

  #endif


      }

#ifdef COUNT_OPER
 nr_access_shared += (4+4)*num_shap; // 4 reads from constant cache and 4 writes to shared memory
 nr_oper += 15*num_shap; // after optimization?
#endif

#endif // end if USE_SHAPE_FUN_WORKSPACE

#ifdef USE_SHAPE_FUN_WORKSPACE
      } // the end of block to indicate the scope of jac_x registers
#endif


#ifdef CONSTANT_COEFF

  #ifdef USE_REGISTERS_FOR_COEFF

    #ifdef USE_PDE_COEFF_WORKSPACE

    offset=thread_id*(NR_PDE_COEFFS_SENT);
    SCALAR coeff00=pde_coeff_workspace[offset+0];
    SCALAR coeff01=pde_coeff_workspace[offset+1];
    SCALAR coeff02=pde_coeff_workspace[offset+2];
    SCALAR coeff10=pde_coeff_workspace[offset+3];
    SCALAR coeff11=pde_coeff_workspace[offset+4];
    SCALAR coeff12=pde_coeff_workspace[offset+5];
    SCALAR coeff20=pde_coeff_workspace[offset+6];
    SCALAR coeff21=pde_coeff_workspace[offset+7];
    SCALAR coeff22=pde_coeff_workspace[offset+8];
    SCALAR coeff30=pde_coeff_workspace[offset+9];
    SCALAR coeff31=pde_coeff_workspace[offset+10];
    SCALAR coeff32=pde_coeff_workspace[offset+11];
    SCALAR coeff03=pde_coeff_workspace[offset+12];
    SCALAR coeff13=pde_coeff_workspace[offset+13];
    SCALAR coeff23=pde_coeff_workspace[offset+14];
    SCALAR coeff33=pde_coeff_workspace[offset+15];
    SCALAR coeff04=pde_coeff_workspace[offset+16];
    SCALAR coeff14=pde_coeff_workspace[offset+17];
    SCALAR coeff24=pde_coeff_workspace[offset+18];
    SCALAR coeff34=pde_coeff_workspace[offset+19];

#ifdef COUNT_OPER
    nr_access_shared += 20;
#endif

    #endif // end if USE_PDE_COEFF_WORKSPACE

  #endif // end if USE_REGISTERS_FOR_COEFF

#else // if not CONSTANT COEFF

    //????????????????????? call function to calculate coefficients 
    //????????????????????? based on data in coeff workspace

    //??????????? store data back in workspace or in registers

#endif


      offset=thread_id*(NR_PDE_COEFFS_SENT);


//********************* first loop over shape functions ***********************//
      for(idof = 0; idof < num_shap; idof++){

#ifdef USE_SHAPE_FUN_WORKSPACE

  #ifdef USE_REGISTERS_FOR_SHAPE_FUN

     #ifdef USE_SHAPE_FUN_REF_DIRECTLY

	SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
	SCALAR fun_u_derx = shape_fun_workspace[thread_id*3*num_shap+3*idof];
	SCALAR fun_u_dery = shape_fun_workspace[thread_id*3*num_shap+3*idof+1];
	SCALAR fun_u_derz = shape_fun_workspace[thread_id*3*num_shap+3*idof+2];

    #else

	SCALAR shp_fun_u = shape_fun_workspace[thread_id*4*num_shap+4*idof];
	SCALAR fun_u_derx = shape_fun_workspace[thread_id*4*num_shap+4*idof+1];
	SCALAR fun_u_dery = shape_fun_workspace[thread_id*4*num_shap+4*idof+2];
	SCALAR fun_u_derz = shape_fun_workspace[thread_id*4*num_shap+4*idof+3];

    #endif


#ifdef COUNT_OPER
    nr_access_shared += 4;
#endif

  #endif // end if USE_REGISTERS_FOR_SHAPE_FUN

#else // if not USE_SHAPE_FUN_WORKSPACE

	// read proper values of shape functions and their derivatives
	SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
        temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];


	// compute derivatives wrt global coordinates
	// 15 operations
	SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

#ifdef COUNT_OPER
	nr_access_shared += 4; // constant cache accesses
	nr_oper += 15; // after optimization?
 // total: 13+5+18*num_geo_dofs+15+36+15*num_shap = 177+90 = 267 (for prisms)
#endif

#endif // end if not USE_SHAPE_FUN_WORKSPACE


    #ifdef USE_REGISTERS_FOR_COEFF

      #ifdef USE_REGISTERS_FOR_SHAPE_FUN

	temp4 = coeff00*fun_u_derx + coeff01*fun_u_dery + coeff02*fun_u_derz + coeff03*shp_fun_u;
	temp5 = coeff10*fun_u_derx + coeff11*fun_u_dery + coeff12*fun_u_derz + coeff13*shp_fun_u;
	temp6 = coeff20*fun_u_derx + coeff21*fun_u_dery + coeff22*fun_u_derz + coeff23*shp_fun_u;
	temp7 = coeff30*fun_u_derx + coeff31*fun_u_dery + coeff32*fun_u_derz + coeff33*shp_fun_u;

      #else // if not registers for shape_fun

        #ifdef USE_SHAPE_FUN_REF_DIRECTLY

	temp4 = coeff00*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          coeff01*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          coeff02*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          coeff03*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp5 = coeff10*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          coeff11*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          coeff12*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          coeff13*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp6 = coeff20*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          coeff21*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          coeff22*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          coeff23*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp7 = coeff30*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          coeff31*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          coeff32*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          coeff33*shpfun_ref[igauss*4*num_shap+4*idof];

        #else

	temp4 = coeff00*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          coeff01*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          coeff02*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          coeff03*shape_fun_workspace[thread_id*4*num_shap+4*idof];
	
	temp5 = coeff10*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          coeff11*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          coeff12*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          coeff13*shape_fun_workspace[thread_id*4*num_shap+4*idof];
	
	temp6 = coeff20*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          coeff21*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          coeff22*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          coeff23*shape_fun_workspace[thread_id*4*num_shap+4*idof];
	
	temp7 = coeff30*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          coeff31*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          coeff32*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          coeff33*shape_fun_workspace[thread_id*4*num_shap+4*idof];

        #endif


#ifdef COUNT_OPER
	nr_access_shared += 16; // constant cache and shared memory accesses
#endif

      #endif // end if not registers for shape_fun

    #else // if not registers for coeff

      #ifdef USE_REGISTERS_FOR_SHAPE_FUN

	temp4 = pde_coeff_workspace[offset+0]*fun_u_derx +
          pde_coeff_workspace[offset+1]*fun_u_dery +
          pde_coeff_workspace[offset+2]*fun_u_derz +
          pde_coeff_workspace[offset+12]*shp_fun_u ;
	
	temp5 = pde_coeff_workspace[offset+3]*fun_u_derx +
          pde_coeff_workspace[offset+4]*fun_u_dery +
          pde_coeff_workspace[offset+5]*fun_u_derz +
          pde_coeff_workspace[offset+13]*shp_fun_u;
	
	temp6 = pde_coeff_workspace[offset+6]*fun_u_derx +
          pde_coeff_workspace[offset+7]*fun_u_dery +
          pde_coeff_workspace[offset+8]*fun_u_derz +
          pde_coeff_workspace[offset+14]*shp_fun_u;
	
	temp7 = pde_coeff_workspace[offset+9] *fun_u_derx +
          pde_coeff_workspace[offset+10]*fun_u_dery +
          pde_coeff_workspace[offset+11]*fun_u_derz +
          pde_coeff_workspace[offset+15] *shp_fun_u;
	
#ifdef COUNT_OPER
	nr_access_shared += 16;
#endif
	
      #else // if NOT USE_REGISTERS_FOR_SHAPE_FUN

        #ifdef USE_SHAPE_FUN_REF_DIRECTLY

	temp4 = pde_coeff_workspace[offset+0]*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          pde_coeff_workspace[offset+1]*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          pde_coeff_workspace[offset+2]*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          pde_coeff_workspace[offset+12]*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp5 = pde_coeff_workspace[offset+3]*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          pde_coeff_workspace[offset+4]*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          pde_coeff_workspace[offset+5]*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          pde_coeff_workspace[offset+13]*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp6 = pde_coeff_workspace[offset+6]*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          pde_coeff_workspace[offset+7]*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          pde_coeff_workspace[offset+8]*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          pde_coeff_workspace[offset+14]*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp7 = pde_coeff_workspace[offset+ 9]*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
          pde_coeff_workspace[offset+10]*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
          pde_coeff_workspace[offset+11]*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
          pde_coeff_workspace[offset+15] * shpfun_ref[igauss*4*num_shap+4*idof];

        #else

	temp4 = pde_coeff_workspace[offset+0]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          pde_coeff_workspace[offset+1]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          pde_coeff_workspace[offset+2]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          pde_coeff_workspace[offset+12]*shape_fun_workspace[thread_id*4*num_shap+4*idof];
	
	temp5 = pde_coeff_workspace[offset+3]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          pde_coeff_workspace[offset+4]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          pde_coeff_workspace[offset+5]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          pde_coeff_workspace[offset+13]*shape_fun_workspace[thread_id*4*num_shap+4*idof];
	
	temp6 = pde_coeff_workspace[offset+6]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          pde_coeff_workspace[offset+7]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          pde_coeff_workspace[offset+8]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          pde_coeff_workspace[offset+14]*shape_fun_workspace[thread_id*4*num_shap+4*idof];
	
	temp7 = pde_coeff_workspace[offset+ 9]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
          pde_coeff_workspace[offset+10]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
          pde_coeff_workspace[offset+11]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
          pde_coeff_workspace[offset+15]*shape_fun_workspace[thread_id*4*num_shap+4*idof];

        #endif

#ifdef COUNT_OPER
	nr_access_shared += 32;
#endif
	
      #endif // if not registers for shape fun

    #endif // if not registers for coeff
	
#ifdef COUNT_OPER
	nr_oper += 7*4;
#endif


// ************************* second loop over shape functions ****************************//
	for(jdof = 0; jdof < num_shap; jdof++){
	  
#ifdef USE_SHAPE_FUN_WORKSPACE
	  
  #ifdef USE_REGISTERS_FOR_SHAPE_FUN

    #ifdef USE_SHAPE_FUN_REF_DIRECTLY
	  SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*jdof];
	  SCALAR fun_v_derx = shape_fun_workspace[thread_id*3*num_shap+3*jdof];
	  SCALAR fun_v_dery = shape_fun_workspace[thread_id*3*num_shap+3*jdof+1];
	  SCALAR fun_v_derz = shape_fun_workspace[thread_id*3*num_shap+3*jdof+2];
    #else
	  SCALAR shp_fun_v = shape_fun_workspace[thread_id*4*num_shap+4*jdof];
	  SCALAR fun_v_derx = shape_fun_workspace[thread_id*4*num_shap+4*jdof+1];
	  SCALAR fun_v_dery = shape_fun_workspace[thread_id*4*num_shap+4*jdof+2];
	  SCALAR fun_v_derz = shape_fun_workspace[thread_id*4*num_shap+4*jdof+3];
    #endif

#ifdef COUNT_OPER
	  nr_access_shared += 4;
#endif
	  
  #endif // end if USE_REGISTERS_FOR_SHAPE_FUN
	  
#else // if not  USE_SHAPE_FUN_WORKSPACE

	  // read proper values of shape functions and their derivatives
	  SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*jdof];
	  temp1 = shpfun_ref[igauss*4*num_shap+4*jdof+1];
	  temp2 = shpfun_ref[igauss*4*num_shap+4*jdof+2];
	  temp3 = shpfun_ref[igauss*4*num_shap+4*jdof+3];
	  
	  // compute derivatives wrt global coordinates
	  // 15 operations
	  SCALAR fun_v_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	  SCALAR fun_v_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	  SCALAR fun_v_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
	  
#ifdef COUNT_OPER
	  nr_access_shared += 4; // constant cache accesses
	  nr_oper += 15; // after optimization?
#endif
	  
#endif // end if not  USE_SHAPE_FUN_WORKSPACE

    #ifdef STIFF_MAT_IN_SHARED

	  stiff_mat_workspace[thread_id*num_dofs*(num_dofs+1)+idof*num_dofs+jdof] += (


#ifdef COUNT_OPER
    nr_access_shared += 2;
#endif

    #else

	  stiff_mat[idof*num_dofs+jdof] += (

    #endif

      #ifdef USE_REGISTERS_FOR_SHAPE_FUN

      	    temp4 * fun_v_derx +
       	    temp5 * fun_v_dery +
       	    temp6 * fun_v_derz +
       	    temp7 * shp_fun_v

      #else // if not USE_REGISTERS_FOR_SHAPE_FUN

        #ifdef USE_SHAPE_FUN_REF_DIRECTLY
	    temp4 * shape_fun_workspace[thread_id*3*num_shap+3*jdof] +
	    temp5 * shape_fun_workspace[thread_id*3*num_shap+3*jdof+1] +
	    temp6 * shape_fun_workspace[thread_id*3*num_shap+3*jdof+2] +
	    temp7 * shpfun_ref[igauss*4*num_shap+4*jdof]
        #else
       	    temp4 * shape_fun_workspace[thread_id*4*num_shap+4*jdof+1] +
       	    temp5 * shape_fun_workspace[thread_id*4*num_shap+4*jdof+2] +
       	    temp6 * shape_fun_workspace[thread_id*4*num_shap+4*jdof+3] +
    	    temp7 * shape_fun_workspace[thread_id*4*num_shap+4*jdof]
        #endif

#ifdef COUNT_OPER
    nr_access_shared += 4; // constant cache and shared memory accesses
#endif

     #endif // if not registers for shape_fun


					    ) * vol;

#ifdef COUNT_OPER
 nr_oper += 9; // after optimization?
#endif


             }//jdof


#ifdef LOAD_VEC_COMP

  #ifdef STIFF_MAT_IN_SHARED

	     stiff_mat_workspace[thread_id*num_dofs*(num_dofs+1)+num_dofs*num_dofs+idof] += (

#ifdef COUNT_OPER
    nr_access_shared += 2;
#endif

  #else

             load_vec[idof] += (

  #endif

  #ifdef USE_REGISTERS_FOR_COEFF

    #ifdef USE_REGISTERS_FOR_SHAPE_FUN

	       coeff04 * fun_u_derx +
	       coeff14 * fun_u_dery +
	       coeff24 * fun_u_derz +
	       coeff34 * shp_fun_u

    #else // if not using registers for shape fun

      #ifdef USE_SHAPE_FUN_REF_DIRECTLY
	       coeff04 * shape_fun_workspace[thread_id*3*num_shap+3*idof] +
	       coeff14 * shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
	       coeff24 * shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
	       coeff34 * shpfun_ref[igauss*4*num_shap+4*idof]
      #else
	       coeff04 * shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
	       coeff14 * shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
	       coeff24 * shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
	       coeff34 * shape_fun_workspace[thread_id*4*num_shap+4*idof]
      #endif

#ifdef COUNT_OPER
    nr_access_shared += 4;
#endif

    #endif // end if not using registers for shape fun

  #else // if not using registers for PDE coeff

    #ifdef USE_REGISTERS_FOR_SHAPE_FUN

	       pde_coeff_workspace[offset+16] * fun_u_derx +
	       pde_coeff_workspace[offset+17] * fun_u_dery +
	       pde_coeff_workspace[offset+18] * fun_u_derz +
	       pde_coeff_workspace[offset+19] * shp_fun_u


#ifdef COUNT_OPER
    nr_access_shared += 4;
#endif

    #else // if not using registers for shape fun

      #ifdef USE_SHAPE_FUN_REF_DIRECTLY
	       pde_coeff_workspace[offset+16]*shape_fun_workspace[thread_id*3*num_shap+3*idof] +
	       pde_coeff_workspace[offset+17]*shape_fun_workspace[thread_id*3*num_shap+3*idof+1] +
	       pde_coeff_workspace[offset+18]*shape_fun_workspace[thread_id*3*num_shap+3*idof+2] +
	       pde_coeff_workspace[offset+19]*shpfun_ref[igauss*4*num_shap+4*idof]
      #else
	       pde_coeff_workspace[offset+16]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
	       pde_coeff_workspace[offset+17]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
	       pde_coeff_workspace[offset+18]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
	       pde_coeff_workspace[offset+19]*shape_fun_workspace[thread_id*4*num_shap+4*idof]
      #endif

#ifdef COUNT_OPER
    nr_access_shared += 8;
#endif

    #endif // end if not using registers for shape fun

  #endif // end if not using registers for PDE coeff

	   		       ) * vol;

#ifdef COUNT_OPER
 nr_oper += 9; // after optimization?
#endif
    
#endif // end if computing RHS vector

      }//idof

    }//gauss


#ifdef COAL_WRITE

    // write stiffness matrix - in a coalesced way
    offset = (element_index-thread_id)*(num_shap*num_shap+num_shap);
    i=0;
    for(idof=0; idof < num_shap; idof++)
      {
    	for(jdof=0; jdof < num_shap; jdof++)
	  {

  #ifdef STIFF_MAT_IN_SHARED

	    stiff_mat_out[offset+i*WORK_GROUP_SIZE+thread_id] =
	      stiff_mat_workspace[thread_id*num_dofs*(num_dofs+1)+idof*num_dofs+jdof];

  #else

	    stiff_mat_out[offset+i*WORK_GROUP_SIZE+thread_id] = stiff_mat[idof*num_dofs+jdof];

  #endif

	    i++;
	  }
      }

#ifdef COUNT_OPER
    nr_global_access += num_dofs*num_dofs;
#endif


  #ifdef LOAD_VEC_COMP

    for(i=0; i < num_shap; i++){
      // write load vector

    #ifdef STIFF_MAT_IN_SHARED

      stiff_mat_out[offset+(num_shap*num_shap+i)*WORK_GROUP_SIZE+thread_id] =
	stiff_mat_workspace[thread_id*num_dofs*(num_dofs+1)+num_dofs*num_dofs+i];

    #else

      stiff_mat_out[offset+(num_shap*num_shap+i)*WORK_GROUP_SIZE+thread_id] = load_vec[i];

    #endif

    }

#ifdef COUNT_OPER
    nr_global_access += num_dofs;
#endif

  #endif



#else // if not coalesced write

    // write stiffness matrix - threads compute subsequent elements
    offset = element_index*(num_shap*num_shap+num_shap);
    i=0;
    for(idof=0; idof < num_shap; idof++)
      {
    	for(jdof=0; jdof < num_shap; jdof++)
	  {

  #ifdef STIFF_MAT_IN_SHARED

	    stiff_mat_out[offset+i] =
	      stiff_mat_workspace[thread_id*num_dofs*(num_dofs+1)+idof*num_dofs+jdof];

  #else

	    stiff_mat_out[offset+i] = stiff_mat[idof*num_dofs+jdof];

  #endif

	    i++;
	  }
      }

#ifdef COUNT_OPER
    nr_global_access += num_dofs*num_dofs;
#endif

  #ifdef LOAD_VEC_COMP

    for(i=0; i < num_shap; i++){
      // write load vector

    #ifdef STIFF_MAT_IN_SHARED

      stiff_mat_out[offset+num_shap*num_shap+i] =
	stiff_mat_workspace[thread_id*num_dofs*(num_dofs+1)+num_dofs*num_dofs+i];

    #else

      stiff_mat_out[offset+num_shap*num_shap+i] = load_vec[i];

    #endif

    }

#ifdef COUNT_OPER
    nr_global_access += num_dofs;
#endif

  #endif

#endif


  } // the end of loop over elements


#ifdef COUNT_OPER

  if(group_id==0 && thread_id==0){
    stiff_mat_out[0] = nr_oper;
    stiff_mat_out[1] = nr_access_shared;
    stiff_mat_out[2] = nr_global_access;
  }

#endif


};
