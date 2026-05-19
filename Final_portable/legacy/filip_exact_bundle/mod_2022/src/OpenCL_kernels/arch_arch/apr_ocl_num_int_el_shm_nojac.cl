#define NULL 0 


// execution parameters: size of element PDE coeff input data, 
// size of necessary  nodal coordinates, total size of element input data
#define EL_DATA_IN_SIZE_GPU_COEFF 2 // for elasticity g and rl
#define EL_DATA_IN_SIZE_GPU_GEO 0 // nodal coordinates
#define EL_DATA_IN_SIZE EL_DATA_IN_SIZE_GPU_COEFF+EL_DATA_IN_SIZE_GPU_GEO

#define EL_DATA_LOC_SIZE 32 // must be: EL_DATA_LOC_SIZE > EL_DATA_IN_SIZE !!!!


#define EL_DATA_IN_SIZE_GPU_JAC 10 // necessary Jacobian terms

// size of space for Jacobian terms or Jacobian calculations
#define EL_DATA_JAC_SIZE 64 // must be EL_DATA_JAC_SIZE > EL_DATA_IN_SIZE_GPU_JAC

#define J_AND_DETJ_SIZE EL_DATA_IN_SIZE_GPU_JAC

// ELASTICITY !!!!!!!!!!!!!
#define nreq 3
#define nreq2 9

__kernel void apr_ocl_num_int_el(
  __global int* execution_parameters,
  __global float* gauss_dat, // data for integration of elements having given p
  __global float* shpfun_ref, // shape functions on a reference element
  __global float* el_data_in, // data for integration of NR_ELEM_PER_KERNEL elements
  __global float* stiff_mat_out, // result of integration of NR_ELEM_PER_KERNEL elements
  __local float *part_of_stiff_mat,
  __local float *shape_fun_workspace
				   )
{
  
  
// ASSUMPTION: one element = one work_group (there may be several work_groups per
//             compute unit and of course per device)
//             one work_group = one or more elements processed in a sequence
  
  // SHOULD BE CHANGED FOR CPU AND CELL !!!!!!!!!!!!!
  __local float el_data_loc[EL_DATA_LOC_SIZE]; // local data for integration of 1 element
  __local float el_data_jac[EL_DATA_JAC_SIZE]; // local data for integration of 1 element
  
  const int group_id = get_group_id(0);
  const int thread_id = get_local_id(0);
  //const int global_id = get_global_id(0);
  
  //const int nr_work_groups = execution_parameters[0];
  const int work_group_size = execution_parameters[1];
  const int nr_elems_per_work_group = execution_parameters[2];
  const int nr_parts_of_stiff_mat = execution_parameters[3];
  const int nr_iter_within_part = execution_parameters[4];
  
  // to save registers this is #defined
  //const int nreq = execution_parameters[5];
  //const int nreq2 = nreq*nreq;
  
  //const int pdeg = execution_parameters[6];
  
  // if there is not enough registers we can substitute inline calculations 
  // for num_shap and num_dofs and ngauss
  // const int num_shap = (pdeg+1)*(pdeg+1)*(pdeg+2)/2;
  // const int num_dofs = nreq*num_shap;
  const int num_shap = execution_parameters[7];
  //const int num_dofs = execution_parameters[8];
  const int ngauss = execution_parameters[10];
  
  //int nr_elems_per_kernel = nr_elems_per_work_group * nr_work_groups;
  //int part_of_stiff_mat_size = nreq2 * work_group_size * nr_iter_within_part;
  
  float nr_oper = 0;
  
  int j,k; // for small loops
  int local_offset; // offset used for accesing local matrices 
  int aux_offset; // offset used for accesing arrays (one - to save registers) 
  
  float temp1, temp2, temp3;
  float temp4, temp5, temp6;
  float temp7, temp8, temp9;
  float daux, faux, eaux, vol;
  
  
  // UP TO NOW - 33 registers 
  
  
  int ielem;
  // loop over elements processed by work_group
  for(ielem = 0; ielem < nr_elems_per_work_group; ielem++){
    
    // wersja NOJAC - g, rl only
    if(thread_id < EL_DATA_IN_SIZE){
      el_data_loc[thread_id] = el_data_in[(group_id*nr_elems_per_work_group +
                                           ielem)*EL_DATA_IN_SIZE+thread_id];
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    //nr_access_global += EL_DATA_IN_SIZE;
    //nr_access_local += EL_DATA_IN_SIZE;
    
    // loop over parts of stiffness matrix
    int ipart;
    for(ipart = 0; ipart < nr_parts_of_stiff_mat; ipart++){
      
      int iiter;
      local_offset = 0;

      // zero part of global stiffness matrix
      for(iiter = 0; iiter < nr_iter_within_part; iiter++ ){
	
	for(j=0;j<nreq2;j++){
	  part_of_stiff_mat[local_offset + thread_id] = 0.0f;
	  local_offset += work_group_size;
	}
      }
      
      // nr_access_local += nr_iter_within_part*nreq2*work_group_size;

      int igauss;
      // in a loop over gauss points
      for(igauss = 0; igauss < ngauss; igauss++){
	
	// read the values of shape functions	  		
	j = (4*num_shap) / work_group_size;
	for(k=0; k<j; k++){
	  shape_fun_workspace[thread_id+k*work_group_size]=
	    shpfun_ref[igauss*4*num_shap+thread_id+k*work_group_size];
	}
	if(thread_id < 4*num_shap-j*work_group_size) {
	  shape_fun_workspace[thread_id+j*work_group_size] = 
	    shpfun_ref[igauss*4*num_shap+thread_id+j*work_group_size];
	}
	
        //barrier(CLK_LOCAL_MEM_FENCE);

 	//nr_access_global += 4*num_shap;
 	//nr_access_local += 4*num_shap;

	// NO JACOBIAN CALCULATIONS - read values from global memory
	// Jacobian data are stored in gauss_dat - we read one at a time !!!!
	if(thread_id < J_AND_DETJ_SIZE){
	  el_data_jac[thread_id] = 
            gauss_dat[((group_id*nr_elems_per_work_group+
                        ielem)*ngauss+igauss)*J_AND_DETJ_SIZE+thread_id];
	}
	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	vol = el_data_jac[9];
	
        temp1 = el_data_jac[0];
        temp2 = el_data_jac[1];
        temp3 = el_data_jac[2];
        temp4 = el_data_jac[3];
        temp5 = el_data_jac[4];
        temp6 = el_data_jac[5];
        temp7 = el_data_jac[6];
        temp8 = el_data_jac[7];
        temp9 = el_data_jac[8];
	
	
        j = num_shap / work_group_size ;
        for(k=0; k<j; k++){
	  
// functions at k*work_group_size+thread_id are not accessed - we use derivatives only
	  daux = shape_fun_workspace[num_shap+k*work_group_size+thread_id];
	  eaux = shape_fun_workspace[2*num_shap+k*work_group_size+thread_id];
	  faux = shape_fun_workspace[3*num_shap+k*work_group_size+thread_id];
	  //
	  shape_fun_workspace[num_shap+k*work_group_size+thread_id] = 
	                                 daux*temp1 + eaux*temp4 +  faux*temp7;
	  shape_fun_workspace[2*num_shap+k*work_group_size+thread_id] = 
	                                 daux*temp2 + eaux*temp5 +  faux*temp8;
	  shape_fun_workspace[3*num_shap+k*work_group_size+thread_id] = 
	                                 daux*temp3 + eaux*temp6 +  faux*temp9;
	  	  
	}  
	if(j*work_group_size+thread_id < num_shap) {
	  
// functions at j*work_group_size+thread_id are not accessed - we use derivatives only
	  daux = shape_fun_workspace[num_shap+j*work_group_size+thread_id];
	  eaux = shape_fun_workspace[2*num_shap+j*work_group_size+thread_id];
	  faux = shape_fun_workspace[3*num_shap+j*work_group_size+thread_id];
	  // 
	  shape_fun_workspace[num_shap+j*work_group_size+thread_id] = 
	                                 daux*temp1 + eaux*temp4 +  faux*temp7;
	  shape_fun_workspace[2*num_shap+j*work_group_size+thread_id] = 
	                                 daux*temp2 + eaux*temp5 +  faux*temp8;
	  shape_fun_workspace[3*num_shap+j*work_group_size+thread_id] = 
	                                 daux*temp3 + eaux*temp6 +  faux*temp9;
  
	}
	
	//nr_access_local += 6*num_shap;
	//nr_access_local += 6*j*work_group_size;
	nr_oper += 15*num_shap;
	//nr_oper += 15*(j+1)*work_group_size;
	
	//if(igauss==4&&ielem==57&&ipart==0) goto koniec;
	
	barrier(CLK_LOCAL_MEM_FENCE);
	
//******************* THE END OF APR_ELEM_CALC_3D **********************//
	
	
	// loop over entries per thread
	for(iiter = 0; iiter < nr_iter_within_part; iiter++ ){
	    
// position in element stiffness matrix is given by four parameters idofs, jdofs, ieq, jeq,
// where idofs and ieq are related to test functions and go up and down
// while jdofs and jeq are related to the solution and go from left to right
// in standard implementation jdofs is outer, idofs is inner (in columns)
// since matrices are stored columnwise in vectors the index is computed as:
//      vector[jdofs*nreq*num_dofs+idofs*nreq+jeq*num_dofs+ieq]
//      (num_dofs=num_shap*nreq)

	  local_offset = nreq2*work_group_size*iiter; // index of starting
	  // entry in the part_of_stiff_mat for given iiter 
	  
	  // block number within stiff_mat for computing idofs and jdofs
	  aux_offset = work_group_size*(iiter+nr_iter_within_part*ipart)+thread_id;
	  int jdofs = aux_offset/num_shap;
	  //int idofs = (aux_offset%num_shape);
	  int idofs = (aux_offset-jdofs*num_shap);
	  
	  
	  //  to save registers !!!!!!!!
	  temp1 = shape_fun_workspace[idofs+num_shap];
	  temp2 = shape_fun_workspace[idofs+2*num_shap];
	  temp3 = shape_fun_workspace[idofs+3*num_shap];
	  temp4 = shape_fun_workspace[jdofs+num_shap];
	  temp5 = shape_fun_workspace[jdofs+2*num_shap];
	  temp6 = shape_fun_workspace[jdofs+3*num_shap];
	  
	  //float g = el_data_loc[0];
	  //float rl = el_data_loc[1];
	  //float gprl = g+rl;
	  
	  temp7 = el_data_loc[0];
	  temp8 = el_data_loc[1];
	  temp9 = temp7+temp8;
	  
	  // to save registers
	  daux=temp7*(temp4*temp1+temp5*temp2+temp6*temp3);
	  
	  //	 ieq=0; jeq=0
	  part_of_stiff_mat[local_offset + 0*work_group_size + thread_id] +=  
	    (temp9*temp4*temp1 + daux)*vol;
	  
	  //	 ieq=1; jeq=0
	  part_of_stiff_mat[local_offset + 1*work_group_size + thread_id] +=  
	    (temp8*temp4*temp2 + temp7*temp5*temp1)*vol;
	  
	  //	 ieq=2; jeq=0
	  part_of_stiff_mat[local_offset + 2*work_group_size + thread_id] +=  
	    (temp8*temp4*temp3 + temp7*temp6*temp1)*vol;
	  
	  //	 ieq=0; jeq=1
	  part_of_stiff_mat[local_offset + 3*work_group_size + thread_id] +=  
	    (temp8*temp5*temp1+ temp7*temp4*temp2)*vol;
	  
	  //	 ieq=1; jeq=1
	  part_of_stiff_mat[local_offset + 4*work_group_size + thread_id] +=  
	    (temp9*temp5*temp2+daux)*vol;
	  
	  //	 ieq=2; jeq=1
	  part_of_stiff_mat[local_offset + 5*work_group_size + thread_id] +=  
	    (temp8*temp5 *temp3 + temp7*temp6*temp2)*vol;
	  
	  //	 ieq=0; jeq=2
	  part_of_stiff_mat[local_offset + 6*work_group_size + thread_id] +=  
	    (temp8*temp6 *temp1 + temp7*temp4*temp3)*vol;
	  
	  //	 ieq=1; jeq=2
	  part_of_stiff_mat[local_offset + 7*work_group_size + thread_id] +=  
	    (temp8*temp6 *temp2 + temp7*temp5*temp3)*vol;
	  
	  part_of_stiff_mat[local_offset + 8*work_group_size + thread_id] +=  
	    //	 ieq=2; jeq=2
	    (temp9*temp6*temp3+daux)*vol;
	  
	} // end loop over entries per thread (iterations over part of stiffness matrix)
	
	
	nr_oper += 63*nr_iter_within_part*work_group_size;
	
      } // end loop over Gauss points
      
      
      // write a part of stiffness matrix to global stiffness matrix - COALESCED
      // write back data from local part_of_stiffness_matrix to its place in global memory
      // zero part of global stiffness matrix - coalesced
      
      // for one nreqxnreq block
      //aux_offset = nreq2*work_group_size*nr_iter_within_part*
      //	(ipart+nr_parts_of_stiff_mat*(ielem+nr_elems_per_work_group*group_id));
      //local_offset = 0;
      
      int last_iter;
      if(ipart == nr_parts_of_stiff_mat-1){
	last_iter = 1+(num_shap*num_shap-ipart*nr_iter_within_part*work_group_size)/work_group_size;
      }
      else{
	last_iter = nr_iter_within_part;
      }
      for(iiter = 0; iiter < last_iter; iiter++ ){
	
	local_offset = iiter*work_group_size*nreq2;
	aux_offset = (group_id*nr_elems_per_work_group+ielem)*num_shap*num_shap*nreq*nreq+
	            + ipart*nr_iter_within_part*work_group_size*nreq*nreq + local_offset;
	
	if(ipart == nr_parts_of_stiff_mat-1 && iiter==last_iter-1){
	  k = num_shap*num_shap -  work_group_size*(iiter+nr_iter_within_part*ipart);
	}
	else{
	  k = work_group_size;
	}
	if(thread_id<k){
	  for(j=0;j<nreq2;j++){
	    stiff_mat_out[aux_offset+thread_id] = part_of_stiff_mat[local_offset+thread_id];
	    aux_offset += k;
	    local_offset += work_group_size;
	  }
	}
	
      } // end loop over iterations over part of stiffness matrix
        // i.e. entries per single thread
      
    } // end loop over parts of stiffness matrix
    
  } // end loop over elements processed by work_group
  
  /* barrier(CLK_GLOBAL_MEM_FENCE); */
  
  /* if(thread_id == 0 && group_id == 0){ */

  /*   stiff_mat_out[0] = nr_oper; */


  /* } */

  return;

/* koniec: */
  
  /* if(thread_id == 0 && group_id == 0){ */
    
/*     stiff_mat_out[0] = nr_oper; */
/*     //stiff_mat_out[1] = nr_access_local; */
/*     //stiff_mat_out[2] = nr_access_global; */
/*     stiff_mat_out[2] = nreq; */
/*     stiff_mat_out[3] = el_data_in[0]; */
/*     stiff_mat_out[4] = el_data_in[1]; */
/*     stiff_mat_out[5] = ngauss; */
/*     stiff_mat_out[6] = work_group_size; */
/*     stiff_mat_out[7] = nr_elems_per_work_group; */
/*     stiff_mat_out[8] = nr_parts_of_stiff_mat; */
/*     stiff_mat_out[9] = nr_iter_within_part; */
/*     stiff_mat_out[10] = num_shap; */
/*     //stiff_mat_out[11] = num_dofs; */
/*     //stiff_mat_out[12] = pdeg; */
/*     for(i=0;i<22;i++){ */
/*       stiff_mat_out[11+i] = el_data_jac[i]; */
/*     } */
/*     for(i=0;i<22;i++){ */
/*       stiff_mat_out[11+i] = shape_fun_workspace[2*num_shap+i]; */
/*     } */
   /* stiff_mat_out[15] = el_data_in[15]; */
   /* stiff_mat_out[16] = el_data_in[16]; */
   /* stiff_mat_out[17] = el_data_in[17]; */
   /* stiff_mat_out[18] = el_data_in[18]; */
   /* stiff_mat_out[19] = el_data_in[19]; */

  /*  stiff_mat_out[20] = gauss_dat[0]; */
  /*  stiff_mat_out[21] = gauss_dat[1]; */
  /*  stiff_mat_out[22] = gauss_dat[2]; */
  /*  stiff_mat_out[23] = gauss_dat[3]; */
  /*  stiff_mat_out[24] = gauss_dat[4]; */
  /*  stiff_mat_out[25] = gauss_dat[5]; */
  /*  stiff_mat_out[26] = gauss_dat[6]; */
  /*  stiff_mat_out[27] = gauss_dat[7]; */
  /*  stiff_mat_out[28] = gauss_dat[8]; */
  /*  stiff_mat_out[29] = gauss_dat[9]; */

  /*  j = 9*num_shap*num_shap-10; */
  /*  for(i=0;i<10;i++){ */
  /*    stiff_mat_out[i+j] = shape_fun_workspace[i]; */
  /*  } */

  /* }  */

}

// UP TO NOW - ?? registers
