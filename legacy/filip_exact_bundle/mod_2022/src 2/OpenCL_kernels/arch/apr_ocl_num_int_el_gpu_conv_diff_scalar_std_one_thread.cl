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

//to compute load vector
#define LOAD

#define SCALAR double
#define zero 0.0
#define one 1.0
#define two 2.0

#define WORK_GROUP_SIZE_MAX 64

#define NR_EXEC_PARAMS 16  // size of array with execution parameters
    // here: the smallest work-group for reading data is selected
    // exec_params are read from global to shared memory and used when needed
    // if shared memory resources are scarce this can be reduced

// FOR SCALAR PROBLEMS !!!!!!!!!!!!!!!!!!!!!
#define nreq 1

// FOR LINEAR PRISMS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define num_shap 6
#define num_gauss 6
#define num_dofs (num_gauss*nreq)
#define num_geo_dofs 6

#define EL_GEO_DAT_SIZE (3*num_geo_dofs)

// J_AND_DETJ_SIZE=10 - for NOJAC variants
#define J_AND_DETJ_SIZE 10

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

kernel void tmr_ocl_num_int_el(
  __constant int* execution_parameters,
  //__global int* execution_parameters,
  __constant SCALAR* gauss_dat, // integration points data of elements having given p
  //__global SCALAR* gauss_dat, // integration points data of elements having given p
  __constant SCALAR* shpfun_ref, // shape functions on a reference element
  //__global SCALAR* shpfun_ref, // shape functions on a reference element
  __global SCALAR* el_data_in, // data for integration of NR_ELEMS_PER_KERNEL elements
  __global SCALAR* stiff_mat_out // result of integration of NR_ELEMS_PER_KERNEL elements
)
{

  //int nr_registers=1;
  int j,i;
  const int group_id = get_group_id(0); //0-111
  const int thread_id = get_local_id(0); //0-63
  //const int work_group_size = get_local_size(0); //save register
  const int nr_work_groups = get_num_groups(0); //112
  int offset;
  int igauss;


  __local SCALAR exec_params_workspace[NR_EXEC_PARAMS]; // shared memory copy of parameters
  __local SCALAR gauss_workspace[4*num_gauss]; // workspace for Jacobian calculations
  __local SCALAR shape_fun_workspace[4*num_shap*num_gauss];
  __local SCALAR geo_dat_workspace[EL_GEO_DAT_SIZE*WORK_GROUP_SIZE_MAX]; // geo dofs
#ifdef LAPLACE
  __local SCALAR pde_coeff_workspace[num_gauss*NR_PDE_COEFF_VEC*WORK_GROUP_SIZE_MAX];
#else
  __local SCALAR pde_coeff_workspace[(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)*WORK_GROUP_SIZE_MAX];
#endif

  // read execution parameters to shared memory for faster reads to registers
  if(thread_id < NR_EXEC_PARAMS){
    exec_params_workspace[thread_id] = execution_parameters[thread_id];
  }

  for(i=0;i<4*num_gauss;i++)
	  gauss_workspace[i] = gauss_dat[i];


  for(igauss=0; igauss<num_gauss; igauss++){
	  for(i=0;i<4*num_shap;i++)
		  shape_fun_workspace[igauss*4*num_shap+i] = shpfun_ref[igauss*4*num_shap+i];
  }

  //barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!

  int nr_elems_per_thread = exec_params_workspace[1]/WORK_GROUP_SIZE_MAX/nr_work_groups;  //100352/64/112=14

  int ielem;

  // loop over elements processed by a thread
  for(ielem = 0; ielem < nr_elems_per_thread; ielem++){

			offset = group_id*nr_elems_per_thread*WORK_GROUP_SIZE_MAX*(EL_GEO_DAT_SIZE) + thread_id * nr_elems_per_thread * (EL_GEO_DAT_SIZE) + ielem*(EL_GEO_DAT_SIZE);

			for(i = 0; i < EL_GEO_DAT_SIZE; i++){
				geo_dat_workspace[thread_id*EL_GEO_DAT_SIZE+i] = el_data_in[offset+i];
			}


		#ifdef LAPLACE
			offset=exec_params_workspace[0]*EL_GEO_DAT_SIZE+group_id*nr_elems_per_thread*WORK_GROUP_SIZE_MAX*(NR_PDE_COEFF_VEC*num_gauss) + thread_id * nr_elems_per_thread * (NR_PDE_COEFF_VEC*num_gauss) + ielem*(NR_PDE_COEFF_VEC*num_gauss);
		#else
			offset=exec_params_workspace[0]*EL_GEO_DAT_SIZE+group_id*nr_elems_per_thread*WORK_GROUP_SIZE_MAX*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC) + thread_id * nr_elems_per_thread * (NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC) + ielem*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
		#endif

		#ifdef LAPLACE
				for(i=0;i<(NR_PDE_COEFF_VEC*num_gauss);i++)
				{
					pde_coeff_workspace[thread_id*(NR_PDE_COEFF_VEC*num_gauss)+i]=el_data_in[offset+i];
				}
		#else
				for(i=0;i<NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC;i++)
				{
					pde_coeff_workspace[thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC)+i]=el_data_in[offset+i];
				}
		#endif

    //barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

    SCALAR stiff_mat[num_dofs*num_dofs];
    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = zero;
	#ifdef LOAD
     SCALAR load_vec[num_dofs];
     for(i = 0; i < num_dofs; i++) load_vec[i] = zero;
	#endif

    int pos=0;

    for(igauss = 0; igauss < num_gauss; igauss++){

      SCALAR jac_0 = zero;
      SCALAR jac_1 = zero;
      SCALAR jac_2 = zero;
      SCALAR jac_3 = zero;
      SCALAR jac_4 = zero;
      SCALAR jac_5 = zero;
      SCALAR jac_6 = zero;
      SCALAR jac_7 = zero;
      SCALAR jac_8 = zero;
      SCALAR vol = zero;

      SCALAR daux = gauss_workspace[4*igauss];
      SCALAR faux = gauss_workspace[4*igauss+1];
      SCALAR eaux = gauss_workspace[4*igauss+2];

      // geometrical shape functions are stored in el_data_jac

      vol = one/two;
      SCALAR temp1 = ((one-daux-faux)*(one-eaux))*vol;
      SCALAR temp2 = daux*(one-eaux)*vol;
      SCALAR temp3 = faux*(one-eaux)*vol;
      SCALAR temp4 = (one-daux-faux)*(one+eaux)*vol;
      SCALAR temp5 = daux*(one+eaux)*vol;
      SCALAR temp6 = faux*(one+eaux)*vol;

      // derivatives of geometrical shape functions

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
      SCALAR temp7=zero, temp8=zero, temp9=zero;
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


#ifdef LAPLACE
      offset=thread_id*(NR_PDE_COEFF_VEC*num_gauss);
#else
      offset=thread_id*(NR_PDE_COEFF_MAT+NR_PDE_COEFF_VEC);
#endif

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

#ifndef LAPLACE

	  stiff_mat[idof*num_dofs+jdof] += (
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

	  #ifdef LOAD
		 if(idof==jdof){
			 load_vec[idof] += (
				   pde_coeff_workspace[offset+16] * fun_v_derx +
				   pde_coeff_workspace[offset+17] * fun_v_dery +
				   pde_coeff_workspace[offset+18] * fun_v_derz +
				   pde_coeff_workspace[offset+19] * shp_fun_v
				   ) * vol;
		   }
	  #endif //load

#else //laplace

	  stiff_mat[idof*num_dofs+jdof] += (

			  fun_u_derx * fun_v_derx +
			  fun_u_dery * fun_v_dery +
			  fun_u_derz * fun_v_derz
		) * vol;

	#ifdef LOAD
		if(idof==jdof){
			 load_vec[idof] += (
							pde_coeff_workspace[offset+igauss] * shp_fun_v
			 ) * vol;
 		}
	#endif //load

#endif //ndef laplace

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
		#ifdef LOAD
		 for(i=0; i < num_shap; i++){
		   // write load vector
		   stiff_mat_out[offset+num_shap*num_shap+i] = load_vec[i]; //group_id*WORK_GROUP_SIZE_MAX*nr_elems_per_thread+thread_id*nr_elems_per_thread+ielem;//group_id;//load_vec[i];
		 }
		#endif

  } // the end of loop over elements

};
