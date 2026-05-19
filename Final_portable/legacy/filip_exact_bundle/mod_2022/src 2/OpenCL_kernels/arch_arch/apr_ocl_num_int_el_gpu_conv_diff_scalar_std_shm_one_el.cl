#if defined(cl_amd_fp64)
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#elif defined(cl_khr_fp64)
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#else
    #error "Double precision floating point not supported by OpenCL implementation."
#endif

#define SCALAR double

#define WORK_GROUP_SIZE_MAX 64

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

// Laplace
//#define NR_PDE_COEFF_MAT 3
//#define NR_PDE_COEFF_VEC 1
// conv-diff 
//#define NR_PDE_COEFF_MAT 
//#define NR_PDE_COEFF_VEC 
// full set
#define NR_PDE_COEFF_MAT 16
#define NR_PDE_COEFF_VEC  4

// either coefficients constant for the whole element
#define NR_COEFF_SETS_PER_ELEMENT 1
// or different for every integration point
//#define NR_COEFF_SETS_PER_ELEMENT num_gauss


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

  //FK - added here becouse the same for all elements

  if(thread_id<=NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC)
  {
	  pde_coeff_workspace[thread_id]=el_data_in[EL_GEO_DAT_SIZE+thread_id];
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
	int offset = group_id*nr_elems_per_thread*work_group_size*(EL_GEO_DAT_SIZE+NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC) + thread_id * nr_elems_per_thread * (EL_GEO_DAT_SIZE+NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC) + ielem*(EL_GEO_DAT_SIZE+NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
    // for ONE_EL threads from one work-group read data in a coalesced way for many elements
    for(i = 0; i < EL_GEO_DAT_SIZE; i++){

      // we read in coalesced way but write to shared memory in element order
    	geo_dat_workspace[thread_id*EL_GEO_DAT_SIZE+i] = el_data_in[offset+i];

    }

    //FK ALL Coeff are the same - written earlier


    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!


    SCALAR stiff_mat[num_dofs*num_dofs];
    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = 0.0;
    SCALAR load_vec[num_dofs];
    for(i = 0; i < num_dofs*num_dofs; i++) load_vec[i] = 0.0;

    //nr_registers += 2; //

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

      // derivatives of geometrical shape functions

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
      SCALAR temp7=0.0, temp8=0.0, temp9=0.0;
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

      int idof, jdof;
      for(idof = 0; idof < num_shap; idof++){

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

//	  if(group_id==0&&thread_id==0&&ielem==0&&igauss==2)
//	  {
//		  stiff_mat_out[idof*3]=fun_u_derx;
//		  stiff_mat_out[idof*3+1]=fun_u_dery;
//		  stiff_mat_out[idof*3+2]=fun_u_derz;
//		  //stiff_mat_out[4]=fun_v_derx;
//	  }

	for(jdof = 0; jdof < num_shap; jdof++){

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

	  if(NR_PDE_COEFF_MAT==16)
	  {
		  // 39 operations - conv-diff
		  stiff_mat[idof*num_dofs+jdof] += (

								(pde_coeff_workspace[0]*fun_u_derx +
							 pde_coeff_workspace[1]*fun_u_dery +
							 pde_coeff_workspace[2]*fun_u_derz ) * fun_v_derx +

								(pde_coeff_workspace[3]*fun_u_derx +
							 pde_coeff_workspace[4]*fun_u_dery +
							 pde_coeff_workspace[5]*fun_u_derz ) * fun_v_dery +

								(pde_coeff_workspace[6]*fun_u_derx +
							 pde_coeff_workspace[7]*fun_u_dery +
							 pde_coeff_workspace[8]*fun_u_derz ) * fun_v_derz +

								(pde_coeff_workspace[9]*fun_u_derx +
							 pde_coeff_workspace[10]*fun_u_dery +
							 pde_coeff_workspace[11]*fun_u_derz ) * shp_fun_v +

								(pde_coeff_workspace[12]*fun_v_derx +
							 pde_coeff_workspace[13]*fun_v_dery +
							 pde_coeff_workspace[14]*fun_v_derz ) * shp_fun_u +

				   pde_coeff_workspace[15] * shp_fun_u * shp_fun_v

					  ) * vol;
		  if(idof==jdof){
				 load_vec[idof] += (
							   pde_coeff_workspace[16] * fun_v_derx +
							   pde_coeff_workspace[17] * fun_v_dery +
							   pde_coeff_workspace[18] * fun_v_derz +
							   pde_coeff_workspace[19] * shp_fun_v
							   ) * vol;
		  }

	  }//if
	  else if(NR_PDE_COEFF_MAT==3)
	  {
		// 7 or 10 operations - laplace
			  stiff_mat[idof*num_dofs+jdof] += (

						  pde_coeff_workspace[0] * fun_u_derx * fun_v_derx +

						  pde_coeff_workspace[1] * fun_u_dery * fun_v_dery +

						  pde_coeff_workspace[2] * fun_u_derz * fun_v_derz

						  ) * vol;
			  if(idof==jdof){

					// 3 or 4 operations - laplace
					load_vec[idof] += (
							pde_coeff_workspace[3] * shp_fun_v
							) * vol;

			  }
	  }//elseif

	}//jdof
      }//idof

    }//gauss

    // write stiffness matrix - threads compute subsequent elements
    offset = group_id*WORK_GROUP_SIZE_MAX*nr_elems_per_thread*(num_shap*num_shap+num_shap)+thread_id*nr_elems_per_thread*(num_shap*num_shap+num_shap)+ielem*(num_shap*num_shap+num_shap);
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

    for(i=0; i < num_shap; i++){
      // write load vector
      stiff_mat_out[offset+num_shap*num_shap+i] = load_vec[i]; //group_id*WORK_GROUP_SIZE_MAX*nr_elems_per_thread+thread_id*nr_elems_per_thread+ielem;//group_id;//load_vec[i];
    }


  } // the end of loop over elements

};
