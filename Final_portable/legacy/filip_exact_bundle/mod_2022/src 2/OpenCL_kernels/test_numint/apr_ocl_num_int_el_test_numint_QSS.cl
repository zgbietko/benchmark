//QSS
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

//#define USE_WORKSPACE

//#define USE_REGISTERS_FOR_COEFF

//#define USE_SHAPE_FUN_WORKSPACE // saves registers because frees JAC automatic unknowns
//#define USE_REGISTERS_FOR_SHAPE_FUN
//#define USE_SHAPE_FUN_REF_DIRECTLY

//#define STIFF_MAT_IN_SHARED

//#define PADDING 0

#define COAL_READ
#define COAL_WRITE

#define CONSTANT_COEFF

//#define COUNT_OPER

//#define WORK_GROUP_SIZE 64

#define NR_EXEC_PARAMS 16  // size of array with execution parameters
// here: the smallest work-group for reading data is selected
// exec_params are read from global to shared memory and used when needed
// if shared memory resources are scarce this can be reduced

// FOR SCALAR PROBLEMS !!!!!!!!!!!!!!!!!!!!!
#define nreq 1
// FOR NS_SUPG PROBLEM !!!!!!!!!!!!!!!!!!!!!
//#define nreq 4

// FOR LINEAR PRISMS
#define num_shap 6
#define num_gauss 6
#define num_dofs (num_gauss*nreq)
#define num_geo_dofs 6

#define EL_GEO_DAT_SIZE (3*num_geo_dofs)

// J_AND_DETJ_SIZE=10 - for NOJAC variants
//#define J_AND_DETJ_SIZE 10


//#define LAPLACE
#ifdef LAPLACE
  #define NR_PDE_COEFF_MAT 3
  #define NR_PDE_COEFF_VEC 1
#else
  #define NR_PDE_COEFF_MAT 16
  #define NR_PDE_COEFF_VEC  4
#endif

// either coefficients constant for the whole element
#define NR_COEFF_SETS_PER_ELEMENT 1
// or different for every integration point
//#define NR_COEFF_SETS_PER_ELEMENT num_gauss

#if EL_GEO_DAT_SIZE>(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)
	#define WORKSPACE_SIZE (EL_GEO_DAT_SIZE*WORK_GROUP_SIZE)
#else
	#define WORKSPACE_SIZE ((NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)*WORK_GROUP_SIZE) //laplace
#endif

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

#ifdef USE_WORKSPACE
   __local SCALAR workspace[WORKSPACE_SIZE]; //
#endif

#ifdef USE_SHAPE_FUN_WORKSPACE
  #ifdef USE_SHAPE_FUN_REF_DIRECTLY
    #define SHAPE_FUN_WORKSPACE_SIZE (3*num_shap*WORK_GROUP_SIZE+PADDING*WORK_GROUP_SIZE)
  #else
    #define SHAPE_FUN_WORKSPACE_SIZE (4*num_shap*WORK_GROUP_SIZE+PADDING*WORK_GROUP_SIZE)
  #endif
  __local SCALAR shape_fun_workspace[SHAPE_FUN_WORKSPACE_SIZE];
#else
    SCALAR tab_fun_u_derx[num_shap];
    SCALAR tab_fun_u_dery[num_shap];
    SCALAR tab_fun_u_derz[num_shap];
#endif

#ifdef STIFF_MAT_IN_SHARED
  #define STIFF_MAT_WORKSPACE_SIZE (WORK_GROUP_SIZE*num_dofs*(num_dofs+1)+PADDING*WORK_GROUP_SIZE)
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

    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
                   (element_index - thread_id) * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);

#else // if not COAL_READ

    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
		  element_index * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);

#endif // end if not COAL_READ


#ifdef USE_WORKSPACE
	#ifdef USE_REGISTERS_FOR_COEFF

    for(i=0;i<NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC;i++) {

    #ifdef COAL_READ
      workspace[i*WORK_GROUP_SIZE+thread_id]=el_data_in[offset+i*WORK_GROUP_SIZE+thread_id];
    #else
      workspace[thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)+i]=el_data_in[offset+i];
    #endif

#ifdef COUNT_OPER
    nr_global_access += 1;
#endif

    }

    barrier(CLK_LOCAL_MEM_FENCE); // !! It is needed for proper rewriting to registers

    offset=thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
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

    	#endif//use register for coeff
    #else // if not USE_WORKSPACE
    // combined assumptions: if data are not CONSTANT they are assumed to be non-linear
      #ifdef CONSTANT_COEFF

    	#ifdef USE_REGISTERS_FOR_COEFF

    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
		  element_index * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);

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

#ifdef COUNT_OPER
    nr_global_access += 20;
#endif

    #endif // end if  USE_REGISTERS_FOR_COEFF

  #endif // end if CONSTANT_COEFF (not computed for each integration point separately)

#endif // end if not USE_PDE_COEFF_WORKSPACE

    barrier(CLK_LOCAL_MEM_FENCE); // !! It is needed for freeing the workspace

#ifdef USE_WORKSPACE

    // read geometry data - each thread reads  EL_GEO_DAT_SIZE entries
    // but the entries are not from a single element - the whole array is read
    // and then threads use entries for their elements
  #ifdef COAL_READ
    offset = (element_index-thread_id)*(EL_GEO_DAT_SIZE);
  #else
    offset = element_index*(EL_GEO_DAT_SIZE);
  #endif

    for(i = 0; i < EL_GEO_DAT_SIZE; i++){

  #ifdef COAL_READ
      workspace[i*WORK_GROUP_SIZE+thread_id] = el_data_in[offset+i*WORK_GROUP_SIZE+thread_id];
  #else
      workspace[thread_id*EL_GEO_DAT_SIZE+i] = el_data_in[offset+i];
  #endif

    }

#ifdef COUNT_OPER
    nr_global_access += EL_GEO_DAT_SIZE;
#endif

#endif // end if used geo_dat workspace


//******************** INITIALIZING SM AND LV IN SHARED MEMORY ******************//

#ifdef STIFF_MAT_IN_SHARED

    for(i = 0; i < num_dofs*(num_dofs+1); i++) {
      stiff_mat_workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+i] = zero;
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
#ifdef USE_WORKSPACE
	offset=thread_id*EL_GEO_DAT_SIZE;
#else
	offset=element_index*(EL_GEO_DAT_SIZE);
#endif


	for(i=0;i<num_geo_dofs;i++){

	  jac_1 = jac_data[i];
	  jac_2 = jac_data[num_geo_dofs+i];
	  jac_3 = jac_data[2*num_geo_dofs+i];

#ifdef USE_WORKSPACE

	  jac_4 = workspace[offset+3*i];  //node coor
	  jac_5 = workspace[offset+3*i+1];
	  jac_6 = workspace[offset+3*i+2];

#ifdef COUNT_OPER
    nr_access_shared += 3;
#endif

#else // if not USE_GEO_DAT_WORKSPACE

	  jac_4 = el_data_in[offset+3*i];  //node coor
	  jac_5 = el_data_in[offset+3*i+1];
	  jac_6 = el_data_in[offset+3*i+2];

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



 //************ loop for computing ALL shape function values at integration point **********//
      for(idof = 0; idof < num_shap; idof++){

	// read proper values of shape functions and their derivatives
	temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];

	// compute derivatives wrt global coordinates
	// 15 operations

#ifdef USE_SHAPE_FUN_WORKSPACE

  #ifdef USE_SHAPE_FUN_REF_DIRECTLY

	shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof]   = temp1*jac_0+temp2*jac_3+temp3*jac_6;
	shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
	shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

  #ifdef COUNT_OPER
	nr_access_shared += (3+3); // 3 reads from constant cache and 3 writes to shared memory
  #endif

  #else

	shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof] = shpfun_ref[igauss*4*num_shap+4*idof];
	shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] = temp1*jac_0+temp2*jac_3+temp3*jac_6;
	shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
	shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

	#ifdef COUNT_OPER
 		nr_access_shared += (4+4)*num_shap; // 4 reads from constant cache and 4 writes to shared memory
	#endif

  #endif
#else // if not using shape_fun_workspace

	tab_fun_u_derx[idof] = temp1*jac_0+temp2*jac_3+temp3*jac_6;
	tab_fun_u_dery[idof] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
	tab_fun_u_derz[idof] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

#endif //USE_SHAPE_FUN_WORKSPACE

      }

#ifdef COUNT_OPER
 nr_oper += 15*num_shap; // after optimization?
#endif

#ifdef USE_SHAPE_FUN_WORKSPACE
      } // the end of block to indicate the scope of jac_x registers
#endif

#ifdef USE_REGISTERS_FOR_COEFF
      offset=thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
#else
      offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
      	  			element_index * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
#endif


//********************* first loop over shape functions ***********************//
      for(idof = 0; idof < num_shap; idof++){

#ifdef USE_SHAPE_FUN_WORKSPACE

  #ifdef USE_REGISTERS_FOR_SHAPE_FUN

     #ifdef USE_SHAPE_FUN_REF_DIRECTLY

	SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
	SCALAR fun_u_derx = shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof];
	SCALAR fun_u_dery = shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1];
	SCALAR fun_u_derz = shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2];

    #else

	SCALAR shp_fun_u = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];
	SCALAR fun_u_derx = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1];
	SCALAR fun_u_dery = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2];
	SCALAR fun_u_derz = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3];

    #endif


#ifdef COUNT_OPER
    nr_access_shared += 4;
#endif

  #endif // end if USE_REGISTERS_FOR_SHAPE_FUN

#else // if not USE_SHAPE_FUN_WORKSPACE
    // read proper values of shape functions and their derivatives
          SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
          SCALAR fun_u_derx = tab_fun_u_derx[idof];
          SCALAR fun_u_dery = tab_fun_u_dery[idof];
          SCALAR fun_u_derz = tab_fun_u_derz[idof];

      #ifdef COUNT_OPER
          nr_access_shared += 1;
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

	temp4 = coeff00*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
          coeff01*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
          coeff02*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
          coeff03*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp5 = coeff10*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
          coeff11*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
          coeff12*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
          coeff13*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp6 = coeff20*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
          coeff21*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
          coeff22*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
          coeff23*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp7 = coeff30*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
          coeff31*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
          coeff32*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
          coeff33*shpfun_ref[igauss*4*num_shap+4*idof];

        #else

	temp4 = coeff00*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
          coeff01*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
          coeff02*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
          coeff03*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];
	
	temp5 = coeff10*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
          coeff11*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
          coeff12*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
          coeff13*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];
	
	temp6 = coeff20*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
          coeff21*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
          coeff22*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
          coeff23*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];
	
	temp7 = coeff30*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
          coeff31*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
          coeff32*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
          coeff33*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];

        #endif


#ifdef COUNT_OPER
	nr_access_shared += 16; // constant cache and shared memory accesses
#endif

      #endif // end if not registers for shape_fun

    #else // if not registers for coeff

      #ifdef USE_REGISTERS_FOR_SHAPE_FUN

	temp4 = el_data_in[offset+0]*fun_u_derx +
			el_data_in[offset+1]*fun_u_dery +
			el_data_in[offset+2]*fun_u_derz +
			el_data_in[offset+12]*shp_fun_u ;
	
	temp5 = el_data_in[offset+3]*fun_u_derx +
			el_data_in[offset+4]*fun_u_dery +
			el_data_in[offset+5]*fun_u_derz +
			el_data_in[offset+13]*shp_fun_u;
	
	temp6 = el_data_in[offset+6]*fun_u_derx +
			el_data_in[offset+7]*fun_u_dery +
			el_data_in[offset+8]*fun_u_derz +
			el_data_in[offset+14]*shp_fun_u;
	
	temp7 = el_data_in[offset+9] *fun_u_derx +
			el_data_in[offset+10]*fun_u_dery +
			el_data_in[offset+11]*fun_u_derz +
			el_data_in[offset+15] *shp_fun_u;
	
#ifdef COUNT_OPER
	nr_access_shared += 16;
#endif
	
      #else // if NOT USE_REGISTERS_FOR_SHAPE_FUN

        #ifdef USE_SHAPE_FUN_REF_DIRECTLY

	temp4 = el_data_in[offset+0]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
			el_data_in[offset+1]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
			el_data_in[offset+2]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
			el_data_in[offset+12]*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp5 = el_data_in[offset+3]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
			el_data_in[offset+4]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
			el_data_in[offset+5]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
			el_data_in[offset+13]*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp6 = el_data_in[offset+6]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
			el_data_in[offset+7]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
			el_data_in[offset+8]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
			el_data_in[offset+14]*shpfun_ref[igauss*4*num_shap+4*idof];
	
	temp7 = el_data_in[offset+ 9]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
			el_data_in[offset+10]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
			el_data_in[offset+11]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
			el_data_in[offset+15] * shpfun_ref[igauss*4*num_shap+4*idof];

        #else

	temp4 = el_data_in[offset+0]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
			el_data_in[offset+1]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
			el_data_in[offset+2]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
			el_data_in[offset+12]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];
	
	temp5 = el_data_in[offset+3]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
			el_data_in[offset+4]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
			el_data_in[offset+5]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
			el_data_in[offset+13]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];
	
	temp6 = el_data_in[offset+6]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
			el_data_in[offset+7]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
			el_data_in[offset+8]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
			el_data_in[offset+14]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];
	
	temp7 = el_data_in[offset+ 9]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
			el_data_in[offset+10]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
			el_data_in[offset+11]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
			el_data_in[offset+15]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof];

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
	  SCALAR fun_v_derx = shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*jdof];
	  SCALAR fun_v_dery = shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*jdof+1];
	  SCALAR fun_v_derz = shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*jdof+2];
    #else
	  SCALAR shp_fun_v = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof];
	  SCALAR fun_v_derx = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof+1];
	  SCALAR fun_v_dery = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof+2];
	  SCALAR fun_v_derz = shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof+3];
    #endif

#ifdef COUNT_OPER
	  nr_access_shared += 4;
#endif
	  
  #endif // end if USE_REGISTERS_FOR_SHAPE_FUN
	  
#else // if not  USE_SHAPE_FUN_WORKSPACE

	SCALAR shp_fun_v = shpfun_ref[igauss*4*num_shap+4*jdof];
	SCALAR fun_v_derx = tab_fun_u_derx[jdof];
	SCALAR fun_v_dery = tab_fun_u_dery[jdof];
	SCALAR fun_v_derz = tab_fun_u_derz[jdof];

#ifdef COUNT_OPER
	  nr_access_shared += 1;
#endif

#endif // end if not  USE_SHAPE_FUN_WORKSPACE

    #ifdef STIFF_MAT_IN_SHARED

	  stiff_mat_workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+idof*num_dofs+jdof] += (

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
			temp4 * shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*jdof] +
			temp5 * shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*jdof+1] +
			temp6 * shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*jdof+2] +
			temp7 * shpfun_ref[igauss*4*num_shap+4*jdof]
        #else
       	    temp4 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof+1] +
       	    temp5 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof+2] +
       	    temp6 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof+3] +
    	    temp7 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*jdof]
        #endif

     #endif // if not registers for shape_fun


					    ) * vol;

#ifdef COUNT_OPER

#ifdef STIFF_MAT_IN_SHARED
	nr_access_shared += 2;
#endif

 nr_oper += 9; // after optimization?
	#ifndef USE_REGISTERS_FOR_SHAPE_FUN
 	 	 nr_access_shared += 4; // constant cache and shared memory accesses
	#endif
#endif


             }//jdof


#ifdef LOAD_VEC_COMP

  #ifdef STIFF_MAT_IN_SHARED

	     stiff_mat_workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+num_dofs*num_dofs+idof] += (

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
	       coeff04 * shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
	       coeff14 * shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
	       coeff24 * shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
	       coeff34 * shpfun_ref[igauss*4*num_shap+4*idof]
      #else
	       coeff04 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
	       coeff14 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
	       coeff24 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
	       coeff34 * shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof]
      #endif

    #endif // end if not using registers for shape fun

  #else // if not using registers for PDE coeff

    #ifdef USE_REGISTERS_FOR_SHAPE_FUN

	       el_data_in[offset+16] * fun_u_derx +
	       el_data_in[offset+17] * fun_u_dery +
	       el_data_in[offset+18] * fun_u_derz +
	       el_data_in[offset+19] * shp_fun_u

    #else // if not using registers for shape fun

      #ifdef USE_SHAPE_FUN_REF_DIRECTLY
	       el_data_in[offset+16]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof] +
	       el_data_in[offset+17]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+1] +
	       el_data_in[offset+18]*shape_fun_workspace[thread_id*(3*num_shap+PADDING)+3*idof+2] +
	       el_data_in[offset+19]*shpfun_ref[igauss*4*num_shap+4*idof]
      #else
	       el_data_in[offset+16]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+1] +
	       el_data_in[offset+17]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+2] +
	       el_data_in[offset+18]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof+3] +
	       el_data_in[offset+19]*shape_fun_workspace[thread_id*(4*num_shap+PADDING)+4*idof]
      #endif

    #endif // end if not using registers for shape fun

  #endif // end if not using registers for PDE coeff

	   		       ) * vol;

#ifdef COUNT_OPER
	#ifdef STIFF_MAT_IN_SHARED
	   nr_access_shared += 2;
	#endif
	#ifdef USE_REGISTERS_FOR_COEFF
	  #ifndef USE_REGISTERS_FOR_SHAPE_FUN
	   	   nr_access_shared += 4;
	  #endif
	#else // if not using registers for PDE coeff
	   #ifdef USE_REGISTERS_FOR_SHAPE_FUN
	   	   nr_access_shared += 4;
	   #else // if not using registers for shape fun
	   	   nr_access_shared += 8;
	   #endif // end if not using registers for shape fun
	#endif // end if not using registers for PDE coeff
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
	      stiff_mat_workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+idof*num_dofs+jdof];

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
	stiff_mat_workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+num_dofs*num_dofs+i];

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
	      stiff_mat_workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+idof*num_dofs+jdof];

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
	stiff_mat_workspace[thread_id*(num_dofs*(num_dofs+1)+PADDING)+num_dofs*num_dofs+i];

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
