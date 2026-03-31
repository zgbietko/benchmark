// kernel version: 88

#if defined(cl_amd_fp64)
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#elif defined(cl_khr_fp64)
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#else
    #error "Double precision floating point not supported by OpenCL implementation."
#endif

#define SCALAR float
//#define SCALAR double

#define USE_REGISTERS_FOR_COEFF

#define GLOBAL_SHAPE_FUN_IN_SHARED

#define STIFF_MAT_IN_SHARED

//#define NO_JACOBIAN_CALCULATIONS

// either coefficients constant for the whole element
#define NR_COEFF_SETS_PER_ELEMENT 1
// or different for every integration point
//#define NR_COEFF_SETS_PER_ELEMENT num_gauss

#define WORK_GROUP_SIZE_MAX 64

//#define LAPLACE

#ifdef LAPLACE
	#define NR_PDE_COEFF_MAT 3
	#define NR_PDE_COEFF_VEC 1
#else
	#define NR_PDE_COEFF_MAT 16
	#define NR_PDE_COEFF_VEC  4
#endif


#define NR_EXEC_PARAMS 32  // size of array with execution parameters
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
#define J_AND_DETJ_SIZE 10



kernel void tmr_ocl_num_int_el(
  __constant int* execution_parameters,
  //__global int* execution_parameters,
  __constant SCALAR* gauss_dat, // integration points data of elements having given p
  //__global SCALAR* gauss_dat, // integration points data of elements having given p
  __constant SCALAR* shpfun_ref, // shape functions on a reference element
  //__global SCALAR* shpfun_ref, // shape functions on a reference element
  __global SCALAR* el_data_in, // data for integration of NR_ELEMS_PER_KERNEL elements
  __global SCALAR* stiff_mat_out, // result of integration of NR_ELEMS_PER_KERNEL elements
  __local SCALAR *part_of_stiff_mat,
  __local SCALAR *shape_fun_workspace,
  __local SCALAR *pde_coeff_workspace
){

  //int nr_registers=1;
  int j,i;

  const int group_id = get_group_id(0); //0-111
  const int thread_id = get_local_id(0); //0-63
  const int work_group_size = get_local_size(0); //64
  const int nr_work_groups = get_num_groups(0); //112

  //nr_registers += 2; //  group_id, thread_id -> 3

  __local SCALAR exec_params_workspace[NR_EXEC_PARAMS]; // shared memory copy of parameters

  __local SCALAR geo_dat_workspace[EL_GEO_DAT_SIZE*WORK_GROUP_SIZE_MAX]; // geo dofs 

  __local SCALAR gauss_workspace[4*num_gauss]; // workspace for Jacobian calculations

  __local SCALAR stiff_mat[num_dofs*num_dofs*WORK_GROUP_SIZE_MAX];
  __local SCALAR load_vec[num_dofs*WORK_GROUP_SIZE_MAX];

#ifdef GLOBAL_SHAPE_FUN_IN_SHARED
  __local SCALAR global_shape_fun_workspace[4*num_shap*WORK_GROUP_SIZE_MAX];
#endif

  // read execution parameters to shared memory for faster reads to registers
  if(thread_id < NR_EXEC_PARAMS){

    exec_params_workspace[thread_id] = execution_parameters[thread_id];

  }
  // gauss_dat (if(kernel_version_alg == ..._JAC or ..._ONE_EL) )
  // gauss data can be read directly from constant memory, assuming it is cached and
  // further accesses are realized from cache
  if(thread_id < 4*num_gauss){

    gauss_workspace[thread_id] = gauss_dat[thread_id];

  }

  // shape functions 
  //   - functions only if( kernel_version_alg==SHM_GL_DER || kernel_version_alg==REG_GL_DER) )

  // ... TO BE DONE IN ANOTHER KERNEL

  //   - functions and derivatives for all other kernels
  int igauss;
  //nr_registers += 1; //  igauss -> 4, num_gauss, num_shap, nreq - #defined
  for(igauss=0; igauss<num_gauss; igauss++){

    if(thread_id < 4*num_shap){

      // the layout may be important !!!
      shape_fun_workspace[igauss*4*num_shap+thread_id] = shpfun_ref[igauss*4*num_shap+thread_id];

    }

  }

  barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!
 

// ASSUMPTION: one element = one thread 

  int nr_elems_per_thread = exec_params_workspace[9]/work_group_size/nr_work_groups;  //100352/64/112=14
  //FK adjusted nr-elems per wg as in old version gives here 13 and it was wrong

  int ielem; 

  // loop over elements processed by a thread
  for(ielem = 0; ielem < nr_elems_per_thread; ielem++){

	    // read geometry data - each thread reads  EL_GEO_DAT_SIZE entries
	    // but the entries are not from a single element - the whole array is read
	    // and then threads use entries for their elements
	    // size of array = nr_work_groups * nr_elems_per_work_group * EL_GEO_DAT_SIZE
	    //int offset = (group_id*nr_elems_per_work_group + ielem*work_group_size) * EL_GEO_DAT_SIZE;
	    //int offset = (group_id*nr_work_groups*nr_elems_per_thread + ielem*work_group_size) * EL_GEO_DAT_SIZE;
		//int offset = group_id*nr_elems_per_thread*work_group_size*(EL_GEO_DAT_SIZE) + thread_id * nr_elems_per_thread * (EL_GEO_DAT_SIZE) + ielem*(EL_GEO_DAT_SIZE);
	    int offset = group_id*work_group_size*(EL_GEO_DAT_SIZE)+ielem*nr_work_groups*work_group_size*(EL_GEO_DAT_SIZE); // + thread_id * nr_elems_per_thread * (EL_GEO_DAT_SIZE) + ielem*(EL_GEO_DAT_SIZE);

	    // for ONE_EL threads from one work-group read data in a coalesced way for many elements
	//	if(group_id==0&&thread_id==0)
	//	{
	//	    stiff_mat_out[ielem*4]=offset;
	//	    stiff_mat_out[ielem*4+1]=group_id*exec_params_workspace[0];
	//	    stiff_mat_out[ielem*4+2]=thread_id * nr_elems_per_thread;
	//	    stiff_mat_out[ielem*4+3]=ielem*(EL_GEO_DAT_SIZE+NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
	//	}
	    for(i = 0; i < EL_GEO_DAT_SIZE; i++){

	      // we read in coalesced way but write to shared memory in element order
	    	geo_dat_workspace[i*work_group_size+thread_id] = el_data_in[offset+i*work_group_size+thread_id];
	    	//geo_dat_workspace[thread_id*EL_GEO_DAT_SIZE+i] = el_data_in[offset+i];
//	    	if(group_id==40&&ielem==9)
//	    		stiff_mat_out[i*work_group_size+thread_id]=geo_dat_workspace[i*work_group_size+thread_id];
	    		//stiff_mat_out[i]=geo_dat_workspace[i*work_group_size+thread_id];
	    	//ok sprawdzone

	    }

	    // OMITTED FOR LINEAR ELEMENTS !!!!
	    // loop over parts of stiffness matrix
	    //int ipart;
	    //for(ipart = 0; ipart < nr_parts_of_stiff_mat; ipart++){

	    //offset=exec_params_workspace[8]*EL_GEO_DAT_SIZE+group_id*nr_elems_per_thread*work_group_size*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss) + thread_id * nr_elems_per_thread * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss) + ielem*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss);
#ifdef LAPLACE
	    offset=exec_params_workspace[8]*EL_GEO_DAT_SIZE+group_id*work_group_size*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss)+ielem*nr_work_groups*work_group_size*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss);
#else
	    offset=exec_params_workspace[8]*EL_GEO_DAT_SIZE+group_id*work_group_size*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)+ielem*nr_work_groups*work_group_size*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
#endif
//	    if(group_id==0&&thread_id==0)
//	    	stiff_mat_out[ielem]=offset;

	   // for(igauss=0; igauss<num_gauss; igauss++){
#ifdef LAPLACE
	    	for(i=0;i<NR_PDE_COEFF_MAT+(NR_PDE_COEFF_VEC*num_gauss);i++)
	    	{
	    		pde_coeff_workspace[i*work_group_size+thread_id]=el_data_in[offset+i*work_group_size+thread_id];
       	   		//pde_coeff_workspace[thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss)+i]=el_data_in[offset+i];
//	       	   		if(group_id==0&&thread_id==0&&ielem==0)
//	       	   			stiff_mat_out[i]=pde_coeff_workspace[thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss)+i];
	       	}
#else
	    	for(i=0;i<NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC;i++)
	    	{
	    		pde_coeff_workspace[i*work_group_size+thread_id]=el_data_in[offset+i*work_group_size+thread_id];
       	   		//pde_coeff_workspace[thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss)+i]=el_data_in[offset+i];
//	       	   		if(group_id==0&&thread_id==0&&ielem==0)
//	       	   			stiff_mat_out[i]=pde_coeff_workspace[thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss)+i];
	       	}
#endif


//	    }


	    // FOR ELEMENTS WITH CONSTANT COEFFICIENTS
	    // read coefficients of PDE
	    // FOR SCALAR PROBLEMS !!!!!!!
	    /* offset = nr_work_groups * nr_elems_per_work_group * EL_GEO_DAT_SIZE + */
	    /*   (group_id*nr_elems_per_work_group + ielem*work_group_size)  */
	    /*                                       * (NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC); */
	    //offset = nr_work_groups * exec_params_workspace[0] * EL_GEO_DAT_SIZE +
	    //  (group_id*exec_params_workspace[0] + ielem*work_group_size)
	    //                                      * (NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC);
	    //offset+=EL_GEO_DAT_SIZE;
	    // FOR VECTOR PROBLEMS ??????
	    // int offset = .... + ielem) * (NR_PDE_COEFF_MAT*NREQ*NREQ + NR_PDE_COEFF_VEC*NREQ + 10);

	    // for ONE_EL threads from one work-group read data in a coalesced way for many elements
	//    for(i = 0; i < NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC; i++){
	//
	//      //pde_coeff_workspace[i*work_group_size+thread_id] =
	//	  //                       el_data_in[offset + i*work_group_size + thread_id];
	//    	pde_coeff_workspace[thread_id*(NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC)+i]=el_data_in[offset+i];
	////    	if(group_id==9&&thread_id==14&&ielem==11)
	////    	    stiff_mat_out[EL_GEO_DAT_SIZE+i]=7; //geo_dat_workspace[thread_id*EL_GEO_DAT_SIZE+i];
	//    }
	    //ALL Coeff are the same - written earlier


    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!


//    SCALAR stiff_mat[num_dofs*num_dofs];
//    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = 0.0;
//    SCALAR load_vec[num_dofs];
//    for(i = 0; i < num_dofs; i++) load_vec[i] = 0.0;

    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i*work_group_size+thread_id] = 0.0;
    for(i = 0; i < num_dofs; i++) load_vec[i*work_group_size+thread_id] = 0.0;

    //nr_registers += 2; //

	// since coeffs are the same for all points
#ifdef USE_REGISTERS_FOR_COEFF
	SCALAR coeff00=pde_coeff_workspace[0];
	SCALAR coeff01=pde_coeff_workspace[1];
	SCALAR coeff02=pde_coeff_workspace[2];
	SCALAR coeff10=pde_coeff_workspace[3];
	SCALAR coeff11=pde_coeff_workspace[4];
	SCALAR coeff12=pde_coeff_workspace[5];
	SCALAR coeff20=pde_coeff_workspace[6];
	SCALAR coeff21=pde_coeff_workspace[7];
	SCALAR coeff22=pde_coeff_workspace[8];
	SCALAR coeff30=pde_coeff_workspace[9];
	SCALAR coeff31=pde_coeff_workspace[10];
	SCALAR coeff32=pde_coeff_workspace[11];
	SCALAR coeff03=pde_coeff_workspace[12];
	SCALAR coeff13=pde_coeff_workspace[13];
	SCALAR coeff23=pde_coeff_workspace[14];
	SCALAR coeff33=pde_coeff_workspace[15];
	SCALAR coeff04=pde_coeff_workspace[16];
	SCALAR coeff14=pde_coeff_workspace[17];
	SCALAR coeff24=pde_coeff_workspace[18];
	SCALAR coeff34=pde_coeff_workspace[19];
#endif

    // in a loop over gauss points
    int pos=0;
    for(igauss = 0; igauss < num_gauss; igauss++){

      SCALAR jac_0 = 0.0;
      SCALAR jac_1 = 0.0;
      SCALAR jac_2 = 0.0;
      SCALAR jac_3 = 0.0;
      SCALAR jac_4 = 0.0;
      SCALAR jac_5 = 0.0;
      SCALAR jac_6 = 0.0;
      SCALAR jac_7 = 0.0;
      SCALAR jac_8 = 0.0;
      SCALAR vol = 0.0;

      // integration data read from cached constant memory
      /* SCALAR daux = gauss_dat[4*igauss]; */
      /* SCALAR faux = gauss_dat[4*igauss+1]; */
      /* SCALAR eaux = gauss_dat[4*igauss+2]; */
      // integration data read from shared memory
      SCALAR daux = gauss_workspace[4*igauss];
      SCALAR faux = gauss_workspace[4*igauss+1];
      SCALAR eaux = gauss_workspace[4*igauss+2];

      // FOR PRISMATIC ELEMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // geometrical shape functions are stored in el_data_jac

      vol = 1.0/2.0;
      SCALAR temp1 = ((1.0-daux-faux)*(1-eaux))*vol;
      SCALAR temp2 = daux*(1.0-eaux)*vol;
      SCALAR temp3 = faux*(1.0-eaux)*vol;
      SCALAR temp4 = (1.0-daux-faux)*(1.0+eaux)*vol;
      SCALAR temp5 = daux*(1.0+eaux)*vol;
      SCALAR temp6 = faux*(1.0+eaux)*vol;
      SCALAR temp7=0.0, temp8=0.0, temp9=0.0;

      // derivatives of geometrical shape functions

      { // to make jac_data a local storage
	SCALAR jac_data[3*6];
	jac_data[0] = -(1.0-eaux)*vol;
	jac_data[1] =  (1.0-eaux)*vol;
	jac_data[2] =  0.0;
	jac_data[3] = -(1.0+eaux)*vol;
	jac_data[4] =  (1.0+eaux)*vol;
	jac_data[5] =  0.0;
	jac_data[6] = -(1.0-eaux)*vol;
	jac_data[7] =  0.0;
	jac_data[8] =  (1.0-eaux)*vol;
	jac_data[9] = -(1.0+eaux)*vol;
	jac_data[10] =  0.0;
	jac_data[11] =  (1.0+eaux)*vol;
	jac_data[12] = -(1.0-daux-faux)*vol;
	jac_data[13] = -daux*vol;
	jac_data[14] = -faux*vol;
	jac_data[15] =  (1.0-daux-faux)*vol;
	jac_data[16] =  daux*vol;
	jac_data[17] =  faux*vol;
	
	temp1=0.0, temp2=0.0, temp3=0.0;
	temp4=0.0, temp5=0.0, temp6=0.0;
	temp7=0.0, temp8=0.0, temp9=0.0;
	/* Jacobian matrix J */
	offset=thread_id*EL_GEO_DAT_SIZE;
	
	for(j=0;j<6;j++){
	  jac_1 = jac_data[j];
	  jac_2 = jac_data[6+j];
	  jac_3 = jac_data[12+j];
	  jac_4 = geo_dat_workspace[offset+3*j];  //node coor
	  jac_5 = geo_dat_workspace[offset+3*j+1];
	  jac_6 = geo_dat_workspace[offset+3*j+2];
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
      }

      daux = temp1*(temp5*temp9-temp8*temp6);
      daux += temp4*(temp8*temp3-temp2*temp9);
      daux += temp7*(temp2*temp6-temp5*temp3);

      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
      vol = gauss_workspace[4*igauss+3] * daux;

      faux = 1.0/daux;

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
#ifdef LAPLACE
      offset=thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC*num_gauss);
#else
      offset=thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
#endif
      //offset for stiffness matrix and load_vector
      int off1=thread_id*(num_shap*num_shap);
      int off2=thread_id*(num_shap);

      int idof, jdof;

#ifdef GLOBAL_SHAPE_FUN_IN_SHARED
      for(idof = 0; idof < num_shap; idof++){

	// read proper values of shape functions and their derivatives
	temp1 = shape_fun_workspace[igauss*4*num_shap+4*idof+1];
	temp2 = shape_fun_workspace[igauss*4*num_shap+4*idof+2];
	temp3 = shape_fun_workspace[igauss*4*num_shap+4*idof+3];

	// compute derivatives wrt global coordinates
	// 15 operations
	global_shape_fun_workspace[thread_id*4*num_shap+4*idof] = 
	  shape_fun_workspace[igauss*4*num_shap+4*idof];
	global_shape_fun_workspace[thread_id*4*num_shap+4*idof+1] = 
	  temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	global_shape_fun_workspace[thread_id*4*num_shap+4*idof+2] = 
	  temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	global_shape_fun_workspace[thread_id*4*num_shap+4*idof+3] = 
	  temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

      }
#endif

      for(idof = 0; idof < num_shap; idof++){

#ifdef GLOBAL_SHAPE_FUN_IN_SHARED
	SCALAR shp_fun_u = global_shape_fun_workspace[thread_id*4*num_shap+4*idof];
	SCALAR fun_u_derx = global_shape_fun_workspace[thread_id*4*num_shap+4*idof+1];
	SCALAR fun_u_dery = global_shape_fun_workspace[thread_id*4*num_shap+4*idof+2];
	SCALAR fun_u_derz = global_shape_fun_workspace[thread_id*4*num_shap+4*idof+3];

#else
	// read proper values of shape functions and their derivatives
	SCALAR shp_fun_u = shape_fun_workspace[igauss*4*num_shap+4*idof];
	SCALAR temp1 = shape_fun_workspace[igauss*4*num_shap+4*idof+1];
	SCALAR temp2 = shape_fun_workspace[igauss*4*num_shap+4*idof+2];
	SCALAR temp3 = shape_fun_workspace[igauss*4*num_shap+4*idof+3];


	// compute derivatives wrt global coordinates
	// 15 operations
	SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

#endif

//	  if(group_id==0&&thread_id==0&&ielem==0&&igauss==2)
//	  {
//		  stiff_mat_out[idof*3]=fun_u_derx;
//		  stiff_mat_out[idof*3+1]=fun_u_dery;
//		  stiff_mat_out[idof*3+2]=fun_u_derz;
//		  //stiff_mat_out[4]=fun_v_derx;
//	  }

	for(jdof = 0; jdof < num_shap; jdof++){

#ifdef GLOBAL_SHAPE_FUN_IN_SHARED
	  SCALAR shp_fun_v = global_shape_fun_workspace[thread_id*4*num_shap+4*jdof];
	  SCALAR fun_v_derx = global_shape_fun_workspace[thread_id*4*num_shap+4*jdof+1];
	  SCALAR fun_v_dery = global_shape_fun_workspace[thread_id*4*num_shap+4*jdof+2];
	  SCALAR fun_v_derz = global_shape_fun_workspace[thread_id*4*num_shap+4*jdof+3];

#else
	  // read proper values of shape functions and their derivatives
	  SCALAR shp_fun_v = shape_fun_workspace[igauss*4*num_shap+4*jdof];
	  temp1 = shape_fun_workspace[igauss*4*num_shap+4*jdof+1];
	  temp2 = shape_fun_workspace[igauss*4*num_shap+4*jdof+2];
	  temp3 = shape_fun_workspace[igauss*4*num_shap+4*jdof+3];

	  // compute derivatives wrt global coordinates
	  // 15 operations
	  SCALAR fun_v_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	  SCALAR fun_v_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	  SCALAR fun_v_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

#endif

#ifndef LAPLACE

#ifdef USE_REGISTERS_FOR_COEFF

	  // 37 operations - conv-diff
	  	  stiff_mat[off1+idof*num_dofs+jdof] += (

(coeff00*fun_u_derx + coeff01*fun_u_dery + coeff02*fun_u_derz + coeff03*shp_fun_u) * fun_v_derx +
(coeff10*fun_u_derx + coeff11*fun_u_dery + coeff12*fun_u_derz + coeff13*shp_fun_u) * fun_v_dery +
(coeff20*fun_u_derx + coeff21*fun_u_dery + coeff22*fun_u_derz + coeff23*shp_fun_u) * fun_v_derz +
(coeff30*fun_u_derx + coeff31*fun_u_dery + coeff32*fun_u_derz + coeff33*shp_fun_u) * shp_fun_v 

	  			      //) * el_data_jac[igauss*J_AND_DETJ_SIZE + 9];
	  			      ) * vol;

	  if(idof==jdof){
	    load_vec[off2+idof] += (
	    		       coeff04 * fun_v_derx +
	    		       coeff14 * fun_v_dery +
	    		       coeff24 * fun_v_derz +
	    		       coeff34 * shp_fun_v
			       ) * vol;
	  }

#else

		  // 39 operations - conv-diff
		  stiff_mat[off1+idof*num_dofs+jdof] += (

								(pde_coeff_workspace[offset+0]*fun_u_derx +
							 pde_coeff_workspace[offset+1]*fun_u_dery +
							 pde_coeff_workspace[offset+2]*fun_u_derz ) * fun_v_derx +

								(pde_coeff_workspace[offset+3]*fun_u_derx +
							 pde_coeff_workspace[offset+4]*fun_u_dery +
							 pde_coeff_workspace[offset+5]*fun_u_derz ) * fun_v_dery +

								(pde_coeff_workspace[offset+6]*fun_u_derx +
							 pde_coeff_workspace[offset+7]*fun_u_dery +
							 pde_coeff_workspace[offset+8]*fun_u_derz ) * fun_v_derz +

								(pde_coeff_workspace[offset+9]*fun_u_derx +
							 pde_coeff_workspace[offset+10]*fun_u_dery +
							 pde_coeff_workspace[offset+11]*fun_u_derz ) * shp_fun_v +

								(pde_coeff_workspace[offset+12]*fun_v_derx +
							 pde_coeff_workspace[offset+13]*fun_v_dery +
							 pde_coeff_workspace[offset+14]*fun_v_derz ) * shp_fun_u +

				   pde_coeff_workspace[offset+15] * shp_fun_u * shp_fun_v

					  ) * vol;
		  if(idof==jdof){
				 load_vec[off2+idof] += (
							   pde_coeff_workspace[offset+16] * fun_v_derx +
							   pde_coeff_workspace[offset+17] * fun_v_dery +
							   pde_coeff_workspace[offset+18] * fun_v_derz +
							   pde_coeff_workspace[offset+19] * shp_fun_v
							   ) * vol;
		  }

#endif

#else
		  // 7 or 10 operations - laplace
			  stiff_mat[off1+idof*num_dofs+jdof] += (

						  pde_coeff_workspace[offset+0] * fun_u_derx * fun_v_derx +

						  pde_coeff_workspace[offset+1] * fun_u_dery * fun_v_dery +

						  pde_coeff_workspace[offset+2] * fun_u_derz * fun_v_derz

						  ) * vol;
			  if(idof==jdof){

					// 3 or 4 operations - laplace
					load_vec[off2+idof] += (
							pde_coeff_workspace[offset+3+igauss] * shp_fun_v
							) * vol;

			  }
#endif

	}//jdof
      }//idof

    }//gauss

    // write stiffness matrix - threads compute subsequent elements
    //offset = group_id*WORK_GROUP_SIZE_MAX*nr_elems_per_thread*(num_shap*num_shap+num_shap)+thread_id*nr_elems_per_thread*(num_shap*num_shap+num_shap)+ielem*(num_shap*num_shap+num_shap);

    barrier(CLK_LOCAL_MEM_FENCE);

    offset = group_id*WORK_GROUP_SIZE_MAX*nr_elems_per_thread*(num_shap*num_shap+num_shap)+ielem*WORK_GROUP_SIZE_MAX*(num_shap*num_shap+num_shap);
    for(i=0; i < num_shap*num_shap; i++)
    {
      	     stiff_mat_out[offset+i*work_group_size+thread_id] = stiff_mat[i*work_group_size+thread_id];
    }
    for(i=0; i < num_shap; i++){
      // write load vector
      stiff_mat_out[offset+WORK_GROUP_SIZE_MAX*num_shap*num_shap+i*work_group_size+thread_id] = load_vec[i*work_group_size+thread_id]; //group_id*WORK_GROUP_SIZE_MAX*nr_elems_per_thread+thread_id*nr_elems_per_thread+ielem;//group_id;//load_vec[i];
    }


  } // the end of loop over elements

};
