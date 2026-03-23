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
#define NR_PDE_COEFF_MAT 3
#define NR_PDE_COEFF_VEC 1
// conv-diff 
//#define NR_PDE_COEFF_MAT 
//#define NR_PDE_COEFF_VEC 
// full set
//#define NR_PDE_COEFF_MAT 16
//#define NR_PDE_COEFF_VEC  4

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

  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);
  const int work_group_size = get_local_size(0);
  const int nr_work_groups = get_num_groups(0);

  //nr_registers += 2; //  group_id, thread_id -> 3

  __local SCALAR exec_params_workspace[NR_EXEC_PARAMS]; // shared memory copy of parameters

  __local SCALAR geo_dat_workspace[EL_GEO_DAT_SIZE*WORK_GROUP_SIZE_MAX]; // geo dofs 

  __local SCALAR gauss_workspace[4*num_gauss]; // workspace for Jacobian calculations

  // read execution parameters to shared memory for faster reads to registers
  if(thread_id < NR_EXEC_PARAMS){

    exec_params_workspace[thread_id] = execution_parameters[thread_id];

  }

  //int nr_elems_per_work_group = exec_params_workspace[0];

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
  //4*num_shap*ngauss
  //int num_shap = exec_params_workspace[];
  //int num_gauss = exec_params_workspace[];
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


  // in exec_params_workspace[0] we send the number of elements per work_group
  // (nr_elems_per_work_group = work_group_size * nr_elems_per_thread)
  int nr_elems_per_thread = exec_params_workspace[0]/work_group_size;

  int ielem; 
  // loop over elements processed by a thread
  for(ielem = 0; ielem < nr_elems_per_thread; ielem++){

    // read geometry data - each thread reads  EL_GEO_DAT_SIZE entries
    // but the entries are not from a single element - the whole array is read
    // and then threads use entries for their elements
    // size of array = nr_work_groups * nr_elems_per_work_group * EL_GEO_DAT_SIZE
    //int offset = (group_id*nr_elems_per_work_group + ielem*work_group_size) * EL_GEO_DAT_SIZE;
    int offset = (group_id*exec_params_workspace[0] + ielem*work_group_size) * EL_GEO_DAT_SIZE;

    // for ONE_EL threads from one work-group read data in a coalesced way for many elements
    for(i = 0; i < EL_GEO_DAT_SIZE; i++){ 
      
      // we read in coalesced way but write to shared memory in element order
      geo_dat_workspace[i*work_group_size+thread_id] = el_data_in[offset+i*work_group_size+thread_id];
      
    }
    
    // OMITTED FOR LINEAR ELEMENTS !!!!
    // loop over parts of stiffness matrix
    //int ipart;
    //for(ipart = 0; ipart < nr_parts_of_stiff_mat; ipart++){


    // FOR ELEMENTS WITH CONSTANT COEFFICIENTS 
    // read coefficients of PDE
    // FOR SCALAR PROBLEMS !!!!!!!
    /* offset = nr_work_groups * nr_elems_per_work_group * EL_GEO_DAT_SIZE + */
    /*   (group_id*nr_elems_per_work_group + ielem*work_group_size)  */
    /*                                       * (NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC); */
    offset = nr_work_groups * exec_params_workspace[0] * EL_GEO_DAT_SIZE +
      (group_id*exec_params_workspace[0] + ielem*work_group_size) 
                                          * (NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC);
    // FOR VECTOR PROBLEMS ??????
    // int offset = .... + ielem) * (NR_PDE_COEFF_MAT*NREQ*NREQ + NR_PDE_COEFF_VEC*NREQ + 10);
    
 
    // for ONE_EL threads from one work-group read data in a coalesced way for many elements
    for(i = 0; i < NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC; i++){ 
      
      pde_coeff_workspace[i*work_group_size+thread_id] = 
	                         el_data_in[offset + i*work_group_size + thread_id];
      
    }
        
    barrier(CLK_LOCAL_MEM_FENCE); // !!!!!!!!!!!!!!!!!!!!!!
    
      
    SCALAR stiff_mat[num_dofs*num_dofs];
    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = 0.0;
    SCALAR load_vec[num_dofs];
    for(i = 0; i < num_dofs*num_dofs; i++) load_vec[i] = 0.0;
    
    //nr_registers += 2; //
    
    // in a loop over gauss points
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
      vol = 0.5;
      SCALAR temp1 = 1.0-eaux;
      SCALAR temp2 = 1.0+eaux;
      SCALAR temp3 = 1.0-daux-faux;
      SCALAR temp4 = temp1*vol;
      SCALAR temp5 = temp2*vol;
      SCALAR temp6 = temp3*vol;
      
      // derivatives of geometrical shape functions
      SCALAR jac_data[3*num_geo_dofs];
      jac_data[0] = -temp4;  
      jac_data[1] =  temp4;  
      jac_data[2] =  0.0; 
      jac_data[3] = -temp5;  
      jac_data[4] =  temp5;  
      jac_data[5] =  0.0; 
      
      jac_data[6+0] = -temp4;
      jac_data[6+1] =  0.0;
      jac_data[6+2] =  temp4;
      jac_data[6+3] = -temp5;
      jac_data[6+4] =  0.0;
      jac_data[6+5] =  temp5;
      
      jac_data[12+0] = -temp6;
      jac_data[12+1] = -daux*vol;
      jac_data[12+2] = -faux*vol;
      jac_data[12+3] =  temp6;
      jac_data[12+4] =  daux*vol;
      jac_data[12+5] =  faux*vol;
      
      temp1=0.0, temp2=0.0, temp3=0.0;
      temp4=0.0, temp5=0.0, temp6=0.0;
      SCALAR temp7=0.0, temp8=0.0, temp9=0.0;
      /* Jacobian matrix J */
      //offset = ielem + ???????????;
      for(j=0;j<num_geo_dofs;j++){
	jac_1 = jac_data[j];
	jac_2 = jac_data[6+j];
	jac_3 = jac_data[12+j];
	jac_4 = geo_dat_workspace[offset+3*j];
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
      //vol = gauss_dat[4*igauss+3] * daux;
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
      
      //nr_oper += ;
      //nr_access_local += ;

      // coefficients can be written to temp4-temp9 !!!!

      // variant 1: global derivatives computed in a separate loop

      SCALAR shape_fun_real_der[3*num_shap];
      int idof, jdof;
      for(idof = 0; idof < num_shap; idof++){
	
	// read proper values of shape functions and their derivatives
	SCALAR temp1 = shape_fun_workspace[igauss*4*num_shap+4*idof+1];
	SCALAR temp2 = shape_fun_workspace[igauss*4*num_shap+4*idof+2];
	SCALAR temp3 = shape_fun_workspace[igauss*4*num_shap+4*idof+3];
	shape_fun_real_der[3*idof]   = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	shape_fun_real_der[3*idof+1] = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	shape_fun_real_der[3*idof+2] = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
      }


     for(idof = 0; idof < num_shap; idof++){
	
	// read proper values of shape functions and their derivatives
	SCALAR shp_fun_u = shape_fun_workspace[igauss*4*num_shap+4*idof];
	SCALAR fun_u_derx = shape_fun_real_der[3*idof];
	SCALAR fun_u_dery = shape_fun_real_der[3*idof+1];
	SCALAR fun_u_derz = shape_fun_real_der[3*idof+2];
	
	
	for(jdof = 0; jdof < num_shap; jdof++){
	    	  
	  // read proper values of shape functions and their derivatives
	  SCALAR shp_fun_v = shape_fun_real_der[igauss*4*num_shap+4*jdof];
	  SCALAR fun_v_derx = shape_fun_real_der[3*jdof];
	  SCALAR fun_v_dery = shape_fun_real_der[3*jdof+1];
	  SCALAR fun_v_derz = shape_fun_real_der[3*jdof+2];
	  
  
	  // variant 2: global derivatives computed on-the-fly
      /* int idofs, jdofs; */
      /* for(idofs = 0; idofs < num_shap; idofs++){ */
	
      /* 	// read proper values of shape functions and their derivatives */
      /* 	SCALAR shp_fun_u = shape_fun_workspace[igauss*4*num_shap+4*idof]; */
      /* 	SCALAR temp1 = shape_fun_workspace[igauss*4*num_shap+4*idof+1]; */
      /* 	SCALAR temp2 = shape_fun_workspace[igauss*4*num_shap+4*idof+2]; */
      /* 	SCALAR temp3 = shape_fun_workspace[igauss*4*num_shap+4*idof+3]; */
	
      /* 	// compute derivatives wrt global coordinates */
      /* 	// 15 operations */
      /* 	SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6; */
      /* 	SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7; */
      /* 	SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8; */
	
      /* 	for(jdofs = 0; jdofs < num_shap; jdofs++){ */
	    	  
      /* 	  // read proper values of shape functions and their derivatives */
      /* 	  SCALAR shp_fun_v = shape_fun_workspace[igauss*4*num_shap+4*jdof]; */
      /* 	  temp1 = shape_fun_workspace[igauss*4*num_shap+4*jdof+1]; */
      /* 	  temp2 = shape_fun_workspace[igauss*4*num_shap+4*jdof+2]; */
      /* 	  temp3 = shape_fun_workspace[igauss*4*num_shap+4*jdof+3]; */
	  
      /* 	  // compute derivatives wrt global coordinates */
      /* 	  // 15 operations */
      /* 	  SCALAR fun_v_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6; */
      /* 	  SCALAR fun_v_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7; */
      /* 	  SCALAR fun_v_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8; */
	  
	  
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

// FROM ORIGINAL APR_NUM_INT_EL
	    /* kk=0; */
	    /* for (udofs=0;udofs<num_shap;udofs++) { */
	    /*   for (wdofs=0;wdofs<num_shap;wdofs++) { */
		
	    /* 	if(Diagonal[1]==0){ */
		  
	    /* 	  for(ueq=0;ueq<nreq;ueq++){ */
	    /* 	    for(weq=0;weq<nreq;weq++){ */
		      
		      
	    /* 	      Stiff_mat[kk+wdofs*nreq+ueq*num_dofs+weq] += (  */
	    /* 	    // axx[ueq*nreq+weq] * dw[weq]/dx * du[ueq]/dx  */
	    /* 	        axx[ueq*nreq+weq] *base_dphix[udofs] *base_dphix[wdofs] + */
	    /* 	    // axy[ueq*nreq+weq] * dw[weq]/dx * du[ueq]/dy */
	    /* 		axy[ueq*nreq+weq] *base_dphiy[udofs] *base_dphix[wdofs] + */
	    /* 	    // axz[ueq*nreq+weq] * dw[weq]/dx * du[ueq]/dz */
	    /* 		axz[ueq*nreq+weq] *base_dphiz[udofs] *base_dphix[wdofs] + */
	    /* 	    // ayx[ueq*nreq+weq] * dw[weq]/dy * du[ueq]/dx */
	    /* 		ayx[ueq*nreq+weq] *base_dphix[udofs] *base_dphiy[wdofs] + */
	    /* 	    // ayy[ueq*nreq+weq] * dw[weq]/dy * du[ueq]/dy */
	    /* 		ayy[ueq*nreq+weq] *base_dphiy[udofs] *base_dphiy[wdofs] + */
	    /* 	    // ayz[ueq*nreq+weq] * dw[weq]/dy * du[ueq]/dz */
	    /* 		ayz[ueq*nreq+weq] *base_dphiz[udofs] *base_dphiy[wdofs] + */
	    /* 	    // azx[ueq*nreq+weq] * dw[weq]/dz * du[ueq]/dx */
	    /* 		azx[ueq*nreq+weq] *base_dphix[udofs] *base_dphiz[wdofs] + */
	    /* 	    // azy[ueq*nreq+weq] * dw[weq]/dz * du[ueq]/dy */
	    /* 		azy[ueq*nreq+weq] *base_dphiy[udofs] *base_dphiz[wdofs] + */
	    /* 	    // azz[ueq*nreq+weq] * dw[weq]/dz * du[ueq]/dz */
	    /* 		azz[ueq*nreq+weq] *base_dphiz[udofs] *base_dphiz[wdofs]   */
	    /* 							    ) * vol; */

	    /* 	    } */
	    /* 	  } */


// 39 operations - conv-diff
	  /* stiff_mat_entry += ( */

          /*                  (pde_coeff_workspace[0]*fun_u_derx + */
	  /*                   pde_coeff_workspace[1]*fun_u_dery + */
	  /*                   pde_coeff_workspace[2]*fun_u_derz ) * fun_v_derx + */

          /*                  (pde_coeff_workspace[3]*fun_u_derx + */
	  /*                   pde_coeff_workspace[4]*fun_u_dery + */
	  /*                   pde_coeff_workspace[5]*fun_u_derz ) * fun_v_dery + */

          /*                  (pde_coeff_workspace[6]*fun_u_derx + */
	  /*                   pde_coeff_workspace[7]*fun_u_dery + */
	  /*                   pde_coeff_workspace[8]*fun_u_derz ) * fun_v_derz + */

          /*                  (pde_coeff_workspace[9]*fun_u_derx + */
	  /*                   pde_coeff_workspace[10]*fun_u_dery + */
	  /*                   pde_coeff_workspace[11]*fun_u_derz ) * shp_fun_v + */

          /*                  (pde_coeff_workspace[12]*fun_v_derx + */
	  /*                   pde_coeff_workspace[13]*fun_v_dery + */
	  /*                   pde_coeff_workspace[14]*fun_v_derz ) * shp_fun_u + */

	  /* 		   pde_coeff_workspace[15] * shp_fun_u * shp_fun_v  */
			    
	  /* 		      ) * vol; */

// 7 or 10 operations - laplace
	  stiff_mat[idof*num_dofs+jdof] += (
			      
			      pde_coeff_workspace[0] * fun_u_derx * fun_v_derx +
			      
			      pde_coeff_workspace[1] * fun_u_dery * fun_v_dery +
			      
			      pde_coeff_workspace[2] * fun_u_derz * fun_v_derz 
			      
			      ) * vol;
	  
	  
	  if(idof==jdof){
	    
	    // for ROWWISE STORAGE 
	    // subsequnt threads have subsequent values of U shape function !!!
	    /* load_vec_entry += ( */
	    /* 		       pde_coeff_workspace[16] * fun_u_derx + */
	    /* 		       pde_coeff_workspace[17] * fun_u_dery + */
	    /* 		       pde_coeff_workspace[18] * fun_u_derz + */
	    /* 		       pde_coeff_workspace[19] * shp_fun_u  */
	    /* 		       ) * el_data_jac[9]; */

	    // for COLUMNWISE STORAGE
	    // subsequnt threads have subsequent values of U shape function !!!
	    // 9 operations - full conv-diff
	    /* load_vec_entry += ( */
	    /* 		       pde_coeff_workspace[16] * fun_v_derx + */
	    /* 		       pde_coeff_workspace[17] * fun_v_dery + */
	    /* 		       pde_coeff_workspace[18] * fun_v_derz + */
	    /* 		       pde_coeff_workspace[19] * shp_fun_v  */
	    /* 		       ) * el_data_jac[9]; */

	    // FROM ORIGINAL APR_NUM_INT_EL
	  /* for (wdofs=0;wdofs<num_shap;wdofs++) { */
	
	  /*   for(weq=0;weq<nreq;weq++){ */

	  /*     Rhs_vect[wdofs*nreq+weq] += (  */
	  /* 		      qx[weq] * base_dphix[wdofs] + */
	  /* 		      qy[weq] * base_dphiy[wdofs] + */
	  /* 		      qz[weq] * base_dphiz[wdofs] ) * vol; */
	      
	  /*   } */

	  /* } /\* wdofs *\/ */

	    // 3 or 4 operations - laplace
	    load_vec[idof] += (
				pde_coeff_workspace[3] * shp_fun_v 
				) * vol;
	    
	  }
	  
          //nr_registers += 11; // shp_fun, temp -> 22
	  
	  
	}

      }

    }

    // write stiffness matrix - threads compute subsequent elements
    //offset = ????????????/;
    for(i=0; i < num_dofs*num_dofs; i++){
      //stiff_mat_out[offset+thread_id] = stiff_mat[???????];
    }
    
    for(i=0; i < num_dofs; i++){
      // write load vector 
      //stiff_mat_out[offset+num_shap*num_shap+thread_id] = load_vec;
    }
    
  

/*kbw
    if(ielem==0){
      if(thread_id==0 && group_id==0){
	for(i=0; i < J_AND_DETJ_SIZE*num_gauss; i++){
	  
	  stiff_mat_out[i] = el_data_jac[i];
 	
	}
      }
    }
//kew*/
  
  } // the end of loop over elements  
    
  /* if(thread_id < NR_EXEC_PARAMS){ */
    
  /*   stiff_mat_out[thread_id] = exec_params_workspace[thread_id]; */
    
  /* } */

/*kbw
  if(thread_id==0 && group_id==0){
    //for(i=0;i<4*num_shap*num_gauss;i++){
    //for(i=0; i<NR_EXEC_PARAMS; i++){
    for(i=0; i < NR_PDE_COEFF_MAT + NR_PDE_COEFF_VEC; i++){

      //stiff_mat_out[i] = exec_params_workspace[i];
      //stiff_mat_out[i] = shape_fun_workspace[i];
      stiff_mat_out[i] = pde_coeff_workspace[i];

    }

  }
//kew*/
  
};
