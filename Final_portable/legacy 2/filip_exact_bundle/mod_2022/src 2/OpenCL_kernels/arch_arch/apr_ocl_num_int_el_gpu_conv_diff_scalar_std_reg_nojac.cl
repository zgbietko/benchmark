#if defined(cl_amd_fp64)
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#elif defined(cl_khr_fp64)
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#else
    #error "Double precision floating point not supported by OpenCL implementation."
#endif

#define SCALAR double

#define NR_EXEC_PARAMS 32  // size of array with execution parameters
    // here: the smallest work-group for reading data is selected
    // exec_params are read from global to shared memory and used when needed
    // if shared memory resources are scarce this can be reduced

// Laplace
//#define NR_PDE_COEFF_MAT 3
//#define NR_PDE_COEFF_VEC 1
// conv-diff 
//#define NR_PDE_COEFF_MAT 
//#define NR_PDE_COEFF_VEC 
// full set
#define NR_PDE_COEFF_MAT 16
#define NR_PDE_COEFF_VEC  4

  // FOR LINEAR PRISMS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define num_shap 6
#define num_gauss 6

// J_AND_DETJ_SIZE=10 - for NOJAC variants
#define J_AND_DETJ_SIZE 10

kernel void tmr_ocl_num_int_el(
  //__global int* execution_parameters,
  __constant int* execution_parameters,
  //__global SCALAR* gauss_dat, // integration points data of elements having given p
  __constant SCALAR* gauss_dat, // integration points data of elements having given p
  //__global SCALAR* shpfun_ref, // shape functions on a reference element
  __constant SCALAR* shpfun_ref, // shape functions on a reference element
  __global SCALAR* el_data_in, // data for integration of NR_ELEMS_PER_KERNEL elements
  __global SCALAR* stiff_mat_out, // result of integration of NR_ELEMS_PER_KERNEL elements
  __local SCALAR *part_of_stiff_mat,
  __local SCALAR *shape_fun_workspace,
  __local SCALAR *pde_coeff_workspace
){

  //int nr_registers=1;

  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);
  //nr_registers += 2; //  group_id, thread_id -> 3

  __local SCALAR exec_params[NR_EXEC_PARAMS]; // shared memory copy of execution parameters

  __local SCALAR el_data_jac[J_AND_DETJ_SIZE*num_gauss]; // data for integration in 1 element

  // read execution parameters to shared memory for faster reads to registers
  if(thread_id < NR_EXEC_PARAMS){

    exec_params[thread_id] = execution_parameters[thread_id];

  }

  // read data common to all elements (depending on the kernel type)

  // gauss_dat (if(kernel_version_alg==SHM_JAC ||kernel_version_alg==REG_JAC ) )

  // ... TO BE DONE IN ANOTHER KERNEL

  // shape functions 
  //   - functions only if( kernel_version_alg==SHM_GL_DER || kernel_version_alg==REG_GL_DER) )

  // ... TO BE DONE IN ANOTHER KERNEL

  //   - functions and derivatives for all other kernels
  //4*num_shap*ngauss
  //int num_shap = exec_params[];
  //int num_gauss = exec_params[];
  int igauss;
  //nr_registers += 1; //  igauss -> 4, num_gauss, num_shap, nreq - #defined
  for(igauss=0; igauss<num_gauss; igauss++){

    if(thread_id < 4*num_shap){

      // the layout may be important !!!
      shape_fun_workspace[igauss*4*num_shap+thread_id] = shpfun_ref[igauss*4*num_shap+thread_id];

    }

  }

  barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!
  
  // FOR A SINGLE PART OF STIFF MAT
  // compute position in STIFF_MAT for a given thread
  // FOR SCALAR PROBLEMS !!!!!!!!!!

  // FOR ROWWISE STORAGE !!!!!!!!!!!!!!!!!!!!!
  //int jdof = thread_id/num_shap;
  //int idofs = (thread_id%num_shap);
  //int idof = (thread_id-jdof*num_shap);

  // FOR COLUMNWISE STORAGE !!!!!!!!!!!!!!!!!!!!!
  int idof = thread_id/num_shap;
  //int jdofs = (thread_id%num_shap);
  int jdof = (thread_id-idof*num_shap);

  //nr_registers += 2; //  idof, jdof -> 6
 

// ASSUMPTION: one element = one work_group (there may be several work_groups per
//             compute unit and of course per device)
//             one work_group = one or more elements processed in a sequence
  int ielem; 
  int nr_elems_per_work_group = exec_params[0];
  //nr_registers += 2; // ielem, nr_elems_per_work_group -> 8

  // loop over elements processed by work_group
  for(ielem = 0; ielem < nr_elems_per_work_group; ielem++){

    // OMITTED FOR LINEAR ELEMENTS !!!!
    // loop over parts of stiffness matrix
    //int ipart;
    //for(ipart = 0; ipart < nr_parts_of_stiff_mat; ipart++){

      // IF MORE THAN ONE PART
      // block number within stiff_mat for computing idofs and jdofs
      //aux_offset = work_group_size*ipart+thread_id;
      //int jdofs = aux_offset/num_shap;
      //int idofs = (aux_offset%num_shap);
      //int idofs = (aux_offset-jdofs*num_shap);


      // FOR ELEMENTS WITH CONSTANT COEFFICIENTS 
      // read coefficients of PDE
      // FOR SCALAR PROBLEMS !!!!!!!
      int offset = (group_id*nr_elems_per_work_group + ielem)
                     * (NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC + J_AND_DETJ_SIZE*num_gauss);
      // FOR VECTOR PROBLEMS ??????
      // int offset = .... + ielem) * (NR_PDE_COEFF_MAT*NREQ*NREQ + NR_PDE_COEFF_VEC*NREQ + 10);

      //nr_registers += 1; // offset -> 9

      if(thread_id < NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC){
      // if(thread_id < (NR_PDE_COEFF_MAT*NREQ*NREQ + NR_PDE_COEFF_VEC*NREQ){

	pde_coeff_workspace[thread_id] = el_data_in[offset + thread_id];

      }

    
      // SCALAR PROBLEMS
      offset += NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC;
      // VECTOR PROBLEMS     
      // offset += NR_PDE_COEFF_MAT*NREQ*NREQ + NR_PDE_COEFF_VEC*NREQ;
    
      if(thread_id < J_AND_DETJ_SIZE*num_gauss){
      
	el_data_jac[thread_id] = el_data_in[offset + thread_id];
	
      }
      
      barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!

      if(thread_id < num_shap*num_shap) {

        SCALAR stiff_mat_entry=0.0;
	SCALAR load_vec_entry=0.0;
	
        //nr_registers += 2; // stiff_mat_entry, load_vec_entry -> 11

	// in a loop over gauss points
	for(igauss = 0; igauss < num_gauss; igauss++){
	  
  	  SCALAR jac_0 = el_data_jac[igauss*J_AND_DETJ_SIZE + 0];
	  SCALAR jac_1 = el_data_jac[igauss*J_AND_DETJ_SIZE + 1];
	  SCALAR jac_2 = el_data_jac[igauss*J_AND_DETJ_SIZE + 2];
	  SCALAR jac_3 = el_data_jac[igauss*J_AND_DETJ_SIZE + 3];
	  SCALAR jac_4 = el_data_jac[igauss*J_AND_DETJ_SIZE + 4];
	  SCALAR jac_5 = el_data_jac[igauss*J_AND_DETJ_SIZE + 5];
	  SCALAR jac_6 = el_data_jac[igauss*J_AND_DETJ_SIZE + 6];
	  SCALAR jac_7 = el_data_jac[igauss*J_AND_DETJ_SIZE + 7];
	  SCALAR jac_8 = el_data_jac[igauss*J_AND_DETJ_SIZE + 8];
	  SCALAR vol = el_data_jac[igauss*J_AND_DETJ_SIZE + 9];
	  
	  // read proper values of shape functions and their derivatives
	  SCALAR shp_fun_u = shape_fun_workspace[igauss*4*num_shap+4*idof];
	  SCALAR temp1 = shape_fun_workspace[igauss*4*num_shap+4*idof+1];
	  SCALAR temp2 = shape_fun_workspace[igauss*4*num_shap+4*idof+2];
	  SCALAR temp3 = shape_fun_workspace[igauss*4*num_shap+4*idof+3];
	  
	  // compute derivatives wrt global coordinates
	  // 15 operations
	  /* SCALAR fun_u_derx = temp1*el_data_jac[igauss*J_AND_DETJ_SIZE + 0] */
	  /*                   + temp2*el_data_jac[igauss*J_AND_DETJ_SIZE + 3] */
	  /*                   + temp3*el_data_jac[igauss*J_AND_DETJ_SIZE + 6]; */
	  /* SCALAR fun_u_dery = temp1*el_data_jac[igauss*J_AND_DETJ_SIZE + 1] */
          /*                   + temp2*el_data_jac[igauss*J_AND_DETJ_SIZE + 4] */
          /*                   + temp3*el_data_jac[igauss*J_AND_DETJ_SIZE + 7]; */
	  /* SCALAR fun_u_derz = temp1*el_data_jac[igauss*J_AND_DETJ_SIZE + 2] */
          /*                   + temp2*el_data_jac[igauss*J_AND_DETJ_SIZE + 5] */
          /*                   + temp3*el_data_jac[igauss*J_AND_DETJ_SIZE + 8]; */
	  SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	  SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	  SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
	  
	  
	  SCALAR shp_fun_v = shape_fun_workspace[igauss*4*num_shap+4*jdof];
	  temp1 = shape_fun_workspace[igauss*4*num_shap+4*jdof+1];
	  temp2 = shape_fun_workspace[igauss*4*num_shap+4*jdof+2];
	  temp3 = shape_fun_workspace[igauss*4*num_shap+4*jdof+3];
	  
	  // compute derivatives wrt global coordinates
	  // 15 operations
	  /* SCALAR fun_v_derx = temp1*el_data_jac[igauss*J_AND_DETJ_SIZE + 0] */
	  /*                   + temp2*el_data_jac[igauss*J_AND_DETJ_SIZE + 3] */
	  /*                   + temp3*el_data_jac[igauss*J_AND_DETJ_SIZE + 6]; */
	  /* SCALAR fun_v_dery = temp1*el_data_jac[igauss*J_AND_DETJ_SIZE + 1] */
          /*                   + temp2*el_data_jac[igauss*J_AND_DETJ_SIZE + 4] */
          /*                   + temp3*el_data_jac[igauss*J_AND_DETJ_SIZE + 7]; */
	  /* SCALAR fun_v_derz = temp1*el_data_jac[igauss*J_AND_DETJ_SIZE + 2] */
          /*                   + temp2*el_data_jac[igauss*J_AND_DETJ_SIZE + 5] */
          /*                   + temp3*el_data_jac[igauss*J_AND_DETJ_SIZE + 8]; */
	  SCALAR fun_v_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	  SCALAR fun_v_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	  SCALAR fun_v_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
	  
	  
	  
/*kbw
    if(ielem==0){
      if(thread_id<num_shap && group_id==0){
	  
	  stiff_mat_out[4*num_shap*igauss + 4*thread_id + 0] = shp_fun_u;
	  stiff_mat_out[4*num_shap*igauss + 4*thread_id+1] = fun_u_derx;
	  stiff_mat_out[4*num_shap*igauss + 4*thread_id+2] = fun_u_dery;
	  stiff_mat_out[4*num_shap*igauss + 4*thread_id+3] = fun_u_derz;
 	
      }
    }
//kew*/

// 39 operations - conv-diff
	  stiff_mat_entry += (

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
			    
			      //) * el_data_jac[igauss*J_AND_DETJ_SIZE + 9];
			      ) * vol;

// 7 or 10 operations - laplace
	  /* stiff_mat_entry += ( */

          /*                  pde_coeff_workspace[0] * fun_u_derx * fun_v_derx + */

          /*                  pde_coeff_workspace[1] * fun_u_dery * fun_v_dery + */

          /*                  pde_coeff_workspace[2] * fun_u_derz * fun_v_derz  */

	  /* 		      ) * vol; */


	  if(thread_id < num_shap){

	    // for ROWWISE STORAGE 
	    // subsequnt threads have subsequent values of U shape function !!!
	    /* load_vec_entry += ( */
	    /* 		       pde_coeff_workspace[16] * fun_u_derx + */
	    /* 		       pde_coeff_workspace[17] * fun_u_dery + */
	    /* 		       pde_coeff_workspace[18] * fun_u_derz + */
	    /* 		       pde_coeff_workspace[19] * shp_fun_u  */
	    /* 		       ) * vol; */

	    // for COLUMNWISE STORAGE
	    // subsequnt threads have subsequent values of U shape function !!!
	    // 9 operations - full conv-diff
	    load_vec_entry += (
	    		       pde_coeff_workspace[16] * fun_v_derx +
	    		       pde_coeff_workspace[17] * fun_v_dery +
	    		       pde_coeff_workspace[18] * fun_v_derz +
	    		       pde_coeff_workspace[19] * shp_fun_v
			       //) * el_data_jac[igauss*J_AND_DETJ_SIZE + 9];
			       ) * vol;

	    // 3 or 4 operations - laplace
	    /* load_vec_entry += ( */
	    /* 		       pde_coeff_workspace[3] * shp_fun_v  */
	    /* 		       ) * vol; */

	  }
			     
          //nr_registers += 11; // shp_fun, temp -> 22

      
	}

	offset = (group_id*nr_elems_per_work_group+ielem)*(num_shap*num_shap+num_shap);
	stiff_mat_out[offset+thread_id] = stiff_mat_entry;
	if(thread_id<num_shap){
	    // subsequnt threads have subsequent values of U shape function !!!
	  stiff_mat_out[offset+num_shap*num_shap+thread_id] = load_vec_entry;
	}

      }

    //}

/*kbw
    if(ielem==0){
      if(thread_id==0 && group_id==0){
	for(i=0; i < J_AND_DETJ_SIZE*num_gauss; i++){
	  
	  stiff_mat_out[i] = el_data_jac[i];
 	
	}
      }
    }
//kew*/
  
    
  }

  /* if(thread_id < NR_EXEC_PARAMS){ */
    
  /*   stiff_mat_out[thread_id] = exec_params[thread_id]; */
    
  /* } */

/*kbw
  if(thread_id==0 && group_id==0){
    //for(i=0;i<4*num_shap*num_gauss;i++){
    //for(i=0; i<NR_EXEC_PARAMS; i++){
    for(i=0; i < NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC; i++){

      //stiff_mat_out[i] = exec_params[i];
      //stiff_mat_out[i] = shape_fun_workspace[i];
      stiff_mat_out[i] = pde_coeff_workspace[i];

    }

  }
//kew*/

};
