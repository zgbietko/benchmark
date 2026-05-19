#if defined(cl_amd_fp64)
  #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#elif defined(cl_khr_fp64)
  #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#else
  #error "Double precision floating point not supported by OpenCL implementation."
#endif

//#define SCALAR float
//#define zero 0.0f
//#define one 1.0f
//#define two 2.0f

#define SCALAR double
#define zero 0.0
#define one 1.0
#define two 2.0

//load vector computing

#define LOAD_VEC_COMP

#define WORK_GROUP_SIZE 64

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

#define EL_GEO_DAT_SIZE (3*num_geo_dofs)

// J_AND_DETJ_SIZE=10 - for NOJAC variants
//#define J_AND_DETJ_SIZE 10

#define LAPLACE

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

#define USE_PDE_COEFF_WORKSPACE // at least one of the two PDE_COEFF options must be active
#define USE_REGISTERS_FOR_COEFF

//#define USE_GEO_DAT_WORKSPACE

//#define USE_SHAPE_FUN_WORKSPACE
#define USE_REGISTERS_FOR_SHAPE_FUN


//#define COAL_READ

//#define STIFF_MAT_IN_SHARED

//#define NO_JACOBIAN_CALCULATIONS

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

  //int nr_registers=1;
  int j,i;
  
  const int group_id = get_group_id(0); 
  const int thread_id = get_local_id(0);
  //const int work_group_size = get_local_size(0);
  const int nr_work_groups = get_num_groups(0);
  
  
  
  // instead of having three workspaces we use one (large enough), that is used in order:
  // for reading geometry data and using it for Jacobian calculations
  // for reading PDE coeffs and storing data in register
  // for storing computed values of shape functions and their global derivatives
  // 24 >= 4*num_shap (16 for tetra, 24 for prisms)
  // 24 > 20 >= NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC
  // 24 > 3*num_geo_dofs (12 for tetra, 18 for prisms)
#ifdef USE_PDE_COEFF_WORKSPACE
  #ifdef LAPLACE
    #define PDE_COEFF_WORKSPACE_SIZE (num_gauss*NR_PDE_COEFF_VEC*WORK_GROUP_SIZE)
  #else
    #define PDE_COEFF_WORKSPACE_SIZE ((NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)*WORK_GROUP_SIZE)
  #endif
  __local SCALAR pde_coeff_workspace[PDE_COEFF_WORKSPACE_SIZE]; // 
#endif

#ifdef USE_GEO_DAT_WORKSPACE
  #define GEO_DAT_WORKSPACE_SIZE (EL_GEO_DAT_SIZE*WORK_GROUP_SIZE)
  __local SCALAR geo_dat_workspace[GEO_DAT_WORKSPACE_SIZE]; // geo dofs 
#endif

#ifdef USE_SHAPE_FUN_WORKSPACE
  #define SHAPE_FUN_WORKSPACE_SIZE (4*num_shap*WORK_GROUP_SIZE)
  __local SCALAR shape_fun_workspace[SHAPE_FUN_WORKSPACE_SIZE];
#endif

 
  // ASSUMPTION: one element = one thread 
  
  int nr_elems_per_thread = execution_parameters[0];  
  int nr_elems_this_kercall = execution_parameters[1];  
  
  int ielem;
  int offset; 
  
  // loop over elements processed by a thread
  for(ielem = 0; ielem < nr_elems_per_thread; ielem++){
    
    int element_index = group_id * nr_elems_per_thread * WORK_GROUP_SIZE + 
                                                 ielem * WORK_GROUP_SIZE +
                                                               thread_id ; 

#ifdef COAL_READ
    
  #ifdef LAPLACE
    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
                   (element_index - thread_id) * (NR_PDE_COEFF_VEC*num_gauss);
  #else
    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
                   (element_index - thread_id) * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
  #endif

#else

  #ifdef LAPLACE
    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
				element_index * (NR_PDE_COEFF_VEC*num_gauss);
  #else
    offset= nr_elems_this_kercall * EL_GEO_DAT_SIZE +
		  element_index * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
  #endif

#endif
    
#ifdef USE_PDE_COEFF_WORKSPACE
#ifdef LAPLACE
    for(i=0;i<NR_PDE_COEFF_VEC*num_gauss;i++) {
      
  #ifdef COAL_READ
      pde_coeff_workspace[i*WORK_GROUP_SIZE+thread_id]=el_data_in[offset+i*WORK_GROUP_SIZE+thread_id];
  #else
      pde_coeff_workspace[thread_id*(NR_PDE_COEFF_VEC*num_gauss)+i]=el_data_in[offset+i];
  #endif

    }
#else
    for(i=0;i<NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC;i++) {
      
  #ifdef COAL_READ
      pde_coeff_workspace[i*WORK_GROUP_SIZE+thread_id]=el_data_in[offset+i*WORK_GROUP_SIZE+thread_id];
  #else
      pde_coeff_workspace[thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)+i]=el_data_in[offset+i];
  #endif
      
    }
#endif
#endif // USE_PDE_COEFF_WORKSPACE
    
#ifdef USE_REGISTERS_FOR_COEFF
#ifndef USE_PDE_COEFF_WORKSPACE
  #ifdef LAPLACE
    SCALAR coeff10=el_data_in[offset+0];
    SCALAR coeff11=el_data_in[offset+1];
    SCALAR coeff12=el_data_in[offset+2];
    SCALAR coeff20=el_data_in[offset+3];
    SCALAR coeff21=el_data_in[offset+4];
    SCALAR coeff22=el_data_in[offset+5];
    SCALAR coeff03;
  #else
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
  #endif
#endif        
#endif        
    

#ifdef USE_GEO_DAT_WORKSPACE

    // read geometry data - each thread reads  EL_GEO_DAT_SIZE entries
    // but the entries are not from a single element - the whole array is read
    // and then threads use entries for their elements
#ifdef COAL_READ
    offset = (element_index-thread_id)*(EL_GEO_DAT_SIZE);
#else
    offset = element_index*(EL_GEO_DAT_SIZE);
#endif

    for(i = 0; i < EL_GEO_DAT_SIZE; i++){
      
      // we should read in coalesced way but write to shared memory in element order
      // below is wrong


#ifdef COAL_READ
      geo_dat_workspace[i*WORK_GROUP_SIZE+thread_id] = el_data_in[offset+i*WORK_GROUP_SIZE+thread_id];
#else
      geo_dat_workspace[thread_id*EL_GEO_DAT_SIZE+i] = el_data_in[offset+i];
#endif
    }

#endif
    
    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
    
    SCALAR stiff_mat[num_dofs*num_dofs];
    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = zero;
#ifdef LOAD_VEC_COMP
    SCALAR load_vec[num_dofs];
    for(i = 0; i < num_dofs; i++) load_vec[i] = zero;
#endif
    //nr_registers += 2; //
    
    // in a loop over gauss points
    int igauss;
    int pos=0;
    for(igauss = 0; igauss < num_gauss; igauss++){

      
      // integration data read from cached constant memory
      /* SCALAR daux = gauss_dat[4*igauss]; */
      /* SCALAR faux = gauss_dat[4*igauss+1]; */
      /* SCALAR eaux = gauss_dat[4*igauss+2]; */
      // integration data read from shared memory
      SCALAR daux = gauss_dat[4*igauss];
      SCALAR faux = gauss_dat[4*igauss+1];
      SCALAR eaux = gauss_dat[4*igauss+2];
      
      // FOR PRISMATIC ELEMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // geometrical shape functions are stored in el_data_jac
      
      SCALAR vol = zero;
      vol = one/two;
      SCALAR temp1 = ((one-daux-faux)*(one-eaux))*vol;
      SCALAR temp2 = daux*(one-eaux)*vol;
      SCALAR temp3 = faux*(one-eaux)*vol;
      SCALAR temp4 = (one-daux-faux)*(one+eaux)*vol;
      SCALAR temp5 = daux*(one+eaux)*vol;
      SCALAR temp6 = faux*(one+eaux)*vol;
      SCALAR temp7 = zero;
      SCALAR temp8 = zero;
      SCALAR temp9 = zero;
      
      int idof, jdof;

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
	
	SCALAR jac_data[3*6];
	jac_data[0] = -(one-eaux)*vol;
	jac_data[1] =  (one-eaux)*vol;
	jac_data[2] =  zero;
	jac_data[3] = -(one+eaux)*vol;
	jac_data[4] =  (one+eaux)*vol;
	jac_data[5] =  zero;
	jac_data[6] = -(one-eaux)*vol;
	jac_data[7] =  zero;
	jac_data[8] =  (one-eaux)*vol;
	jac_data[9] = -(one+eaux)*vol;
	jac_data[10] =  zero;
	jac_data[11] =  (one+eaux)*vol;
	jac_data[12] = -(one-daux-faux)*vol;
	jac_data[13] = -daux*vol;
	jac_data[14] = -faux*vol;
	jac_data[15] =  (one-daux-faux)*vol;
	jac_data[16] =  daux*vol;
	jac_data[17] =  faux*vol;
	
	temp1=zero, temp2=zero, temp3=zero;
	temp4=zero, temp5=zero, temp6=zero;
	
	/* Jacobian matrix J */
#ifdef USE_GEO_DAT_WORKSPACE
	offset=thread_id*EL_GEO_DAT_SIZE;
#else
	offset=element_index*(EL_GEO_DAT_SIZE);
#endif
	
	for(j=0;j<6;j++){
	  jac_1 = jac_data[j];
	  jac_2 = jac_data[6+j];
	  jac_3 = jac_data[12+j];
#ifdef USE_GEO_DAT_WORKSPACE
	  jac_4 = geo_dat_workspace[offset+3*j];  //node coor
	  jac_5 = geo_dat_workspace[offset+3*j+1];
	  jac_6 = geo_dat_workspace[offset+3*j+2];
#else
	  jac_4 = el_data_in[offset+3*j];  //node coor
	  jac_5 = el_data_in[offset+3*j+1];
	  jac_6 = el_data_in[offset+3*j+2];
#endif
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
      
      daux = temp1*(temp5*temp9-temp8*temp6);
      daux += temp4*(temp8*temp3-temp2*temp9);
      daux += temp7*(temp2*temp6-temp5*temp3);
      
      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
      vol = gauss_dat[4*igauss+3] * daux;
      
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
      
      //nr_oper += 175;
      //nr_access_local += 141;
      
      // coefficients can be written to temp4-temp9 !!!!

#ifdef USE_SHAPE_FUN_WORKSPACE

      for(idof = 0; idof < num_shap; idof++){
	
	// read proper values of shape functions and their derivatives
	temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];
	
	// compute derivatives wrt global coordinates
	// 15 operations
	shape_fun_workspace[thread_id*4*num_shap+4*idof] = shpfun_ref[igauss*4*num_shap+4*idof];
	shape_fun_workspace[thread_id*4*num_shap+4*idof+1] = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	shape_fun_workspace[thread_id*4*num_shap+4*idof+2] = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	shape_fun_workspace[thread_id*4*num_shap+4*idof+3] = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
	
      }
#endif
      
#ifdef USE_SHAPE_FUN_WORKSPACE
      } // the end of block to indicate the scope of jac_x registers
#endif

#ifdef USE_REGISTERS_FOR_COEFF
#ifdef USE_PDE_COEFF_WORKSPACE
  #ifdef LAPLACE
    offset=thread_id*(NR_PDE_COEFF_VEC*num_gauss);
    SCALAR coeff03=pde_coeff_workspace[offset+igauss];
  #else
    offset=thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
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
  #endif
#endif
#endif

#ifdef LAPLACE
#ifndef USE_PDE_COEFF_WORKSPACE
  #ifdef USE_REGISTERS_FOR_COEFF
      switch(igauss){
      case 0:
	coeff03 = coeff10;
	break;
      case 1:
	coeff03 = coeff11;
	break;
      case 2:
	coeff03 = coeff12;
	break;
      case 3:
	coeff03 = coeff20;
	break;
      case 4:
	coeff03 = coeff21;
 	break;
      case 5:
	coeff03 = coeff22;
	break;
      }
  #endif
#endif
#endif

#ifdef LAPLACE
      offset=thread_id*(NR_PDE_COEFF_VEC*num_gauss);
#else
      offset=thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
#endif

      for(idof = 0; idof < num_shap; idof++){
	
#ifdef USE_SHAPE_FUN_WORKSPACE

#ifdef USE_REGISTERS_FOR_SHAPE_FUN
	SCALAR shp_fun_u = shape_fun_workspace[thread_id*4*num_shap+4*idof];
	SCALAR fun_u_derx = shape_fun_workspace[thread_id*4*num_shap+4*idof+1];
	SCALAR fun_u_dery = shape_fun_workspace[thread_id*4*num_shap+4*idof+2];
	SCALAR fun_u_derz = shape_fun_workspace[thread_id*4*num_shap+4*idof+3];
#endif
	
#else
	// read proper values of shape functions and their derivatives
	SCALAR shp_fun_u = shpfun_ref[igauss*4*num_shap+4*idof];
	SCALAR temp1 = shpfun_ref[igauss*4*num_shap+4*idof+1];
	SCALAR temp2 = shpfun_ref[igauss*4*num_shap+4*idof+2];
	SCALAR temp3 = shpfun_ref[igauss*4*num_shap+4*idof+3];
	
	
	// compute derivatives wrt global coordinates
	// 15 operations
	SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
	
#endif
	
	
	for(jdof = 0; jdof < num_shap; jdof++){
	  
#ifdef USE_SHAPE_FUN_WORKSPACE

#ifdef USE_REGISTERS_FOR_SHAPE_FUN
	  SCALAR shp_fun_v = shape_fun_workspace[thread_id*4*num_shap+4*jdof];
	  SCALAR fun_v_derx = shape_fun_workspace[thread_id*4*num_shap+4*jdof+1];
	  SCALAR fun_v_dery = shape_fun_workspace[thread_id*4*num_shap+4*jdof+2];
	  SCALAR fun_v_derz = shape_fun_workspace[thread_id*4*num_shap+4*jdof+3];
#endif	  

#else
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
	  
#endif
	  
#ifndef LAPLACE
	  
	  // 37 operations - conv-diff
	  stiff_mat[idof*num_dofs+jdof] += (
#ifdef USE_REGISTERS_FOR_COEFF
#ifdef USE_REGISTERS_FOR_SHAPE_FUN
  (coeff00*fun_u_derx + coeff01*fun_u_dery + coeff02*fun_u_derz + coeff03*shp_fun_u) * fun_v_derx +
  (coeff10*fun_u_derx + coeff11*fun_u_dery + coeff12*fun_u_derz + coeff13*shp_fun_u) * fun_v_dery +
  (coeff20*fun_u_derx + coeff21*fun_u_dery + coeff22*fun_u_derz + coeff23*shp_fun_u) * fun_v_derz +
  (coeff30*fun_u_derx + coeff31*fun_u_dery + coeff32*fun_u_derz + coeff33*shp_fun_u) * shp_fun_v
#else
  (coeff00*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] + 
   coeff01*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] + 
   coeff02*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] + 
   coeff03*shpfun_ref[igauss*4*num_shap+4*idof]) * 
                  shape_fun_workspace[thread_id*4*num_shap+4*jdof+1] +

  (coeff10*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] + 
   coeff11*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] + 
   coeff12*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] + 
   coeff13*shpfun_ref[igauss*4*num_shap+4*idof]) * 
                  shape_fun_workspace[thread_id*4*num_shap+4*jdof+2] +

  (coeff20*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] + 
   coeff21*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] + 
   coeff22*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] + 
   coeff23*shpfun_ref[igauss*4*num_shap+4*idof]) * 
                  shape_fun_workspace[thread_id*4*num_shap+4*jdof+3] +

  (coeff30*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] + 
   coeff31*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] + 
   coeff32*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] + 
   coeff33*shpfun_ref[igauss*4*num_shap+4*idof]) * 
                  shape_fun_workspace[thread_id*4*num_shap+4*jdof]

#endif // end if not registers for shape_fun

#else // if not registers for coeff

#ifdef USE_REGISTERS_FOR_SHAPE_FUN
  (pde_coeff_workspace[offset+0]*fun_u_derx +
   pde_coeff_workspace[offset+1]*fun_u_dery +
   pde_coeff_workspace[offset+2]*fun_u_derz +
   pde_coeff_workspace[offset+12]*shp_fun_u ) * 
 fun_v_derx +

  (pde_coeff_workspace[offset+3]*fun_u_derx +
   pde_coeff_workspace[offset+4]*fun_u_dery +
   pde_coeff_workspace[offset+5]*fun_u_derz +
   pde_coeff_workspace[offset+13]*shp_fun_u ) * 
 fun_v_dery +

  (pde_coeff_workspace[offset+6]*fun_u_derx +
   pde_coeff_workspace[offset+7]*fun_u_dery +
   pde_coeff_workspace[offset+8]*fun_u_derz +
   pde_coeff_workspace[offset+14]*shp_fun_u ) * 
 fun_v_derz +

  (pde_coeff_workspace[offset+9] *fun_u_derx +
   pde_coeff_workspace[offset+10]*fun_u_dery +
   pde_coeff_workspace[offset+11]*fun_u_derz +
   pde_coeff_workspace[offset+15] *shp_fun_u ) * 
 shp_fun_v 
  
#else

  (pde_coeff_workspace[offset+0]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
   pde_coeff_workspace[offset+1]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
   pde_coeff_workspace[offset+2]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
   pde_coeff_workspace[offset+12]*shpfun_ref[igauss*4*num_shap+4*idof]
   ) * shape_fun_workspace[thread_id*4*num_shap+4*jdof+1] +

  (pde_coeff_workspace[offset+3]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
   pde_coeff_workspace[offset+4]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
   pde_coeff_workspace[offset+5]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
   pde_coeff_workspace[offset+13]*shpfun_ref[igauss*4*num_shap+4*idof]
   ) * shape_fun_workspace[thread_id*4*num_shap+4*jdof+2] +

  (pde_coeff_workspace[offset+6]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
   pde_coeff_workspace[offset+7]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
   pde_coeff_workspace[offset+8]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
   pde_coeff_workspace[offset+14]*shpfun_ref[igauss*4*num_shap+4*idof]
   ) * shape_fun_workspace[thread_id*4*num_shap+4*jdof+3] +

  (pde_coeff_workspace[offset+ 9]*shape_fun_workspace[thread_id*4*num_shap+4*idof+1] +
   pde_coeff_workspace[offset+10]*shape_fun_workspace[thread_id*4*num_shap+4*idof+2] +
   pde_coeff_workspace[offset+11]*shape_fun_workspace[thread_id*4*num_shap+4*idof+3] +
   pde_coeff_workspace[offset+15] * shpfun_ref[igauss*4*num_shap+4*idof]          
   ) * shape_fun_workspace[thread_id*4*num_shap+4*jdof]
  

#endif // end if not registers for shape fun
#endif // end if not registers for coeff
  
					    ) * vol;

#ifdef LOAD_VEC_COMP
	  if(idof==jdof){
	    load_vec[idof] += (
#ifdef USE_REGISTERS_FOR_COEFF
			       coeff04 * fun_v_derx +
			       coeff14 * fun_v_dery +
			       coeff24 * fun_v_derz +
			       coeff34 * shp_fun_v
#else
			       pde_coeff_workspace[offset+16] * fun_v_derx +
			       pde_coeff_workspace[offset+17] * fun_v_dery +
			       pde_coeff_workspace[offset+18] * fun_v_derz +
			       pde_coeff_workspace[offset+19] * shp_fun_v
#endif
	   		       ) * vol;
	  }
#endif
	  
	  
#else
	  // 7 or 10 operations - laplace
	  stiff_mat[idof*num_dofs+jdof] += (
					    
					    fun_u_derx * fun_v_derx +
					    fun_u_dery * fun_v_dery +
					    fun_u_derz * fun_v_derz
					    
					    ) * vol;
#ifdef LOAD_VEC_COMP
	  if(idof==jdof){
#ifdef USE_REGISTERS_FOR_COEFF
	    // 2 or 3 operations - laplace
	    load_vec[idof] += coeff03 * shp_fun_v * vol;
#else
	    load_vec[idof] += pde_coeff_workspace[offset+igauss] * shp_fun_v * vol;
#endif
	  }
#endif
#endif		  
	  
	}//jdof
      }//idof
      
    }//gauss
    
    
    // write stiffness matrix - threads compute subsequent elements
    offset = element_index*(num_shap*num_shap+num_shap);
    i=0;
    int idof,jdof;
    for(idof=0; idof < num_shap; idof++)
      {
    	for(jdof=0; jdof < num_shap; jdof++)
	  {
	    stiff_mat_out[offset+i] = stiff_mat[idof*num_dofs+jdof];
	    i++;
	  }
      }
#ifdef LOAD_VEC_COMP
    for(i=0; i < num_shap; i++){
      // write load vector
      stiff_mat_out[offset+num_shap*num_shap+i] = load_vec[i];
    }
#endif
    
  } // the end of loop over elements
  
};
