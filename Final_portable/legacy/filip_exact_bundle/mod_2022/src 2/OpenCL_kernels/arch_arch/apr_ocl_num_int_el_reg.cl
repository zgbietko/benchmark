#define NULL 0 


// execution parameters: size of element PDE coeff input data, size of necessary  nodal
// coordinates, total size of element input data
#define EL_DATA_IN_SIZE_GPU_COEFF 2 // for elasticity g and rl
#define EL_DATA_IN_SIZE_GPU_GEO 18 // nodal coordinates
#define EL_DATA_IN_SIZE (EL_DATA_IN_SIZE_GPU_COEFF+EL_DATA_IN_SIZE_GPU_GEO)

#define EL_DATA_LOC_SIZE 32 // must be: EL_DATA_LOC_SIZE > EL_DATA_IN_SIZE !!!!!!!!!!


#define EL_DATA_IN_SIZE_GPU_JAC 0 // necessary Jacobian terms

// size of space for Jacobian terms or Jacobian calculations
#define EL_DATA_JAC_SIZE 64 // must be EL_DATA_JAC_SIZE > EL_DATA_IN_SIZE_GPU_JAC !!!!!!!!


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
  __local float *num_shape_workspace
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
  //const int nr_iter_within_part = execution_parameters[4];
  
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
  int aux_offset; // offset used for accesing arrays (one - to save registers) 
  
  float temp1, temp2, temp3;
  float temp4, temp5, temp6;
  float temp7, temp8, temp9;
  float daux, faux, eaux, vol;
  
  
  // UP TO NOW - 33 registers 
  
  
  int ielem;
  // loop over elements processed by work_group
  for(ielem = 0; ielem < nr_elems_per_work_group; ielem++){
    
    // wersja JAC - g, rl + 18 nodal coordinates
    if(thread_id < EL_DATA_IN_SIZE){
      el_data_loc[thread_id] = el_data_in[(group_id*nr_elems_per_work_group +
                                           ielem)*EL_DATA_IN_SIZE+thread_id];
    }

    //barrier(CLK_LOCAL_MEM_FENCE);

    //nr_access_global += EL_DATA_IN_SIZE;
    //nr_access_local += EL_DATA_IN_SIZE;
    
    // loop over parts of stiffness matrix
    int ipart;
    for(ipart = 0; ipart < nr_parts_of_stiff_mat; ipart++){
      
      // for storing row-wise - which is NOT the case
      // order: [jeq,ieq] = 0,0; 0,1; 0,2; 1,0; 1,1; 1,2; 2,0; 2,1; 2,2;
      
// position in element stiffness matrix is given by four parameters idofs, jdofs, ieq, jeq,
// where idofs and ieq are related to test functions and go up and down
// while jdofs and jeq are related to the solution and go from left to right
// in standard implementation jdofs is outer, idofs is inner (in columns)
// since matrices are stored columnwise in vectors the index is computed as:
//      vector[jdofs*nreq*num_dofs+idofs*nreq+jeq*num_dofs+ieq]
//      (num_dofs=num_shap*nreq)

      // block number within stiff_mat for computing idofs and jdofs
      aux_offset = work_group_size*ipart+thread_id;
      int jdofs = aux_offset/num_shap;
      //int idofs = (aux_offset%num_shap);
      int idofs = (aux_offset-jdofs*num_shap);

      // for storing column-wise
      // order: [ieq,jeq] = 0,0; 1,0; 2,0; 0,1; 1,1; 2,1; 0,2; 1,2; 2,2;
      // we can assume: ps00, ps10, ps20, ps01, ps11, ps21, ps02, ps12, ps22
      
      float ps00 = 0.0; float ps10 = 0.0; float ps20 = 0.0; 
      float ps01 = 0.0; float ps11 = 0.0; float ps21 = 0.0;
      float ps02 = 0.0; float ps12 = 0.0; float ps22 = 0.0;
      
      int igauss;
      // in a loop over gauss points
      for(igauss = 0; igauss < ngauss; igauss++){
	
	//     compute apr_elem_calc_3D data - SINGLE thread
	//       1. copy gauss data to registers
	//       2. perform calculations using registers
	//       3. save data to local variables
	
	// UP TO NOW - ?? registers 
	
	// to save registers
	daux = gauss_dat[4*igauss];
	faux = gauss_dat[4*igauss+1];
	eaux = gauss_dat[4*igauss+2];
	
	//nr_access_local +=3;

	// read the values of shape functions	  		
	j = (4*num_shap) / work_group_size;
	for(k=0; k<j; k++){
	  num_shape_workspace[thread_id+k*work_group_size]=
	    shpfun_ref[igauss*4*num_shap+thread_id+k*work_group_size];
	}
	if(thread_id < 4*num_shap-j*work_group_size) {
	  num_shape_workspace[thread_id+j*work_group_size] = 
	    shpfun_ref[igauss*4*num_shap+thread_id+j*work_group_size];
	}
	
        //barrier(CLK_LOCAL_MEM_FENCE);
	
	//nr_access_global += 4*num_shap;
	//nr_access_local += 4*num_shap;
	
	if(thread_id == 0) {
	  
	  // geometrical shape functions are stored in el_data_jac 
          vol = 1.0/2.0;
          temp1 = 1.0-eaux;
	  temp2 = 1.0+eaux;
	  temp3 = 1.0-daux-faux;
	  temp4 = temp1*vol;
	  temp5 = temp2*vol;
	  temp6 = temp3*vol;
	  el_data_jac[0]=temp3*temp4;
	  el_data_jac[1]=daux*temp4;
	  el_data_jac[2]=faux*temp4;
	  el_data_jac[3]=temp3*temp5;
	  el_data_jac[4]=daux*temp5;
	  el_data_jac[5]=faux*temp5;
	  
	  // derivatives of geometrical shape functions
	  aux_offset = 6;
	  el_data_jac[aux_offset+0] = -temp4;  
	  el_data_jac[aux_offset+1] =  temp4;  
	  el_data_jac[aux_offset+2] =  0.0; 
	  el_data_jac[aux_offset+3] = -temp5;  
	  el_data_jac[aux_offset+4] =  temp5;  
	  el_data_jac[aux_offset+5] =  0.0; 
	  aux_offset+=6;
	  el_data_jac[aux_offset+0] = -temp4;
	  el_data_jac[aux_offset+1] =  0.0;
	  el_data_jac[aux_offset+2] =  temp4;
	  el_data_jac[aux_offset+3] = -temp5;
	  el_data_jac[aux_offset+4] =  0.0;
	  el_data_jac[aux_offset+5] =  temp5;
	  aux_offset+=6;
	  el_data_jac[aux_offset+0] = -temp6;
	  el_data_jac[aux_offset+1] = -daux*vol;
	  el_data_jac[aux_offset+2] = -faux*vol;
	  el_data_jac[aux_offset+3] =  temp6;
	  el_data_jac[aux_offset+4] =  daux*vol;
	  el_data_jac[aux_offset+5] =  faux*vol;
	  
	  aux_offset += 6;
	  el_data_jac[aux_offset+0] = 0.0; 
	  el_data_jac[aux_offset+1] = 0.0; 
	  el_data_jac[aux_offset+2] = 0.0;
	  el_data_jac[aux_offset+3] = 0.0; 
	  el_data_jac[aux_offset+4] = 0.0; 
	  el_data_jac[aux_offset+5] = 0.0;
	  el_data_jac[aux_offset+6] = 0.0; 
	  el_data_jac[aux_offset+7] = 0.0; 
	  el_data_jac[aux_offset+8] = 0.0;
	  
	  /* Jacobian matrix J */
	  k = EL_DATA_IN_SIZE_GPU_COEFF;
	  for(j=0;j<6;j++){
	    temp1 = el_data_jac[6+j];
	    temp2 = el_data_jac[12+j];
	    temp3 = el_data_jac[18+j];
	    temp4 = el_data_loc[k+3*j];
	    temp5 = el_data_loc[k+3*j+1];
	    temp6 = el_data_loc[k+3*j+2];
	    el_data_jac[aux_offset+0] += temp4 * temp1;
	    el_data_jac[aux_offset+1] += temp4 * temp2;
	    el_data_jac[aux_offset+2] += temp4 * temp3;
	    el_data_jac[aux_offset+3] += temp5 * temp1;
	    el_data_jac[aux_offset+4] += temp5 * temp2;
	    el_data_jac[aux_offset+5] += temp5 * temp3;
	    el_data_jac[aux_offset+6] += temp6 * temp1;
	    el_data_jac[aux_offset+7] += temp6 * temp2;
	    el_data_jac[aux_offset+8] += temp6 * temp3;
	  }
	  
	  temp1 = el_data_jac[aux_offset+0];
	  temp2 = el_data_jac[aux_offset+1];
	  temp3 = el_data_jac[aux_offset+2];
	  temp4 = el_data_jac[aux_offset+3];
	  temp5 = el_data_jac[aux_offset+4];
	  temp6 = el_data_jac[aux_offset+5];
	  temp7 = el_data_jac[aux_offset+6];
	  temp8 = el_data_jac[aux_offset+7];
	  temp9 = el_data_jac[aux_offset+8];
	  
	  daux = temp1*(temp5*temp9-temp8*temp6);
	  daux += temp4*(temp8*temp3-temp2*temp9);
	  daux += temp7*(temp2*temp6-temp5*temp3);

	  /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
	  vol = gauss_dat[4*igauss+3] * daux;
	  el_data_jac[EL_DATA_JAC_SIZE-1] = vol;

	  faux = 1.0/daux;
	  
	  aux_offset = EL_DATA_JAC_SIZE-10;
	  el_data_jac[aux_offset] = (temp5*temp9 - temp8*temp6)*faux;
          el_data_jac[aux_offset+1] = (temp8*temp3 - temp2*temp9)*faux;
          el_data_jac[aux_offset+2] = (temp2*temp6 - temp3*temp5)*faux;

          el_data_jac[aux_offset+3] = (temp6*temp7 - temp4*temp9)*faux;
          el_data_jac[aux_offset+4] = (temp1*temp9 - temp7*temp3)*faux;
          el_data_jac[aux_offset+5] = (temp3*temp4 - temp1*temp6)*faux;

          el_data_jac[aux_offset+6] = (temp4*temp8 - temp5*temp7)*faux;
          el_data_jac[aux_offset+7] = (temp2*temp7 - temp1*temp8)*faux;
          el_data_jac[aux_offset+8] = (temp1*temp5 - temp2*temp4)*faux;

	  nr_oper += 175;
	  //nr_access_local += 141;
	  	  
        }
//******************* THE END OF SINGLE THREAD REGION  **********************//
	
        barrier(CLK_LOCAL_MEM_FENCE);
	
	aux_offset = EL_DATA_JAC_SIZE-10;
        temp1 = el_data_jac[aux_offset + 0];
        temp2 = el_data_jac[aux_offset + 1];
        temp3 = el_data_jac[aux_offset + 2];
        temp4 = el_data_jac[aux_offset + 3];
        temp5 = el_data_jac[aux_offset + 4];
        temp6 = el_data_jac[aux_offset + 5];
        temp7 = el_data_jac[aux_offset + 6];
        temp8 = el_data_jac[aux_offset + 7];
        temp9 = el_data_jac[aux_offset + 8];

	vol = el_data_jac[EL_DATA_JAC_SIZE-1];
	//vol = el_data_jac[aux_offset + 9];
	
        j = num_shap / work_group_size ;
        for(k=0; k<j; k++){
	  
// functions at k*work_group_size+thread_id are not accessed - we use derivatives only
	  daux = num_shape_workspace[num_shap+k*work_group_size+thread_id];
	  eaux = num_shape_workspace[2*num_shap+k*work_group_size+thread_id];
	  faux = num_shape_workspace[3*num_shap+k*work_group_size+thread_id];
	  //
	  num_shape_workspace[num_shap+k*work_group_size+thread_id] = 
	                                 daux*temp1 + eaux*temp4 +  faux*temp7;
	  num_shape_workspace[2*num_shap+k*work_group_size+thread_id] = 
	                                 daux*temp2 + eaux*temp5 +  faux*temp8;
	  num_shape_workspace[3*num_shap+k*work_group_size+thread_id] = 
	                                 daux*temp3 + eaux*temp6 +  faux*temp9;
	  
	  
	}  
	if(j*work_group_size+thread_id < num_shap) {
	
// functions at j*work_group_size+thread_id are not accessed - we use derivatives only
	  daux = num_shape_workspace[num_shap+j*work_group_size+thread_id];
	  eaux = num_shape_workspace[2*num_shap+j*work_group_size+thread_id];
	  faux = num_shape_workspace[3*num_shap+j*work_group_size+thread_id];
	  // 
	  num_shape_workspace[num_shap+j*work_group_size+thread_id] = 
	                                 daux*temp1 + eaux*temp4 +  faux*temp7;
	  num_shape_workspace[2*num_shap+j*work_group_size+thread_id] = 
	                                 daux*temp2 + eaux*temp5 +  faux*temp8;
	  num_shape_workspace[3*num_shap+j*work_group_size+thread_id] = 
	                                 daux*temp3 + eaux*temp6 +  faux*temp9;
  
	}
	
	//nr_access_local += 6*num_shap;
	//nr_access_local += 6*j*work_group_size;
	nr_oper += 15*num_shap;
	//nr_oper += 15*(j+1)*work_group_size;
	
	//if(igauss==4&&ielem==57&&ipart==0) goto koniec;
	
	barrier(CLK_LOCAL_MEM_FENCE);

 
//******************* THE END OF APR_ELEM_CALC_3D **********************//
	
	
	//  to save registers !!!!!!!!
	temp1 = num_shape_workspace[idofs+num_shap];
	temp2 = num_shape_workspace[idofs+2*num_shap];
	temp3 = num_shape_workspace[idofs+3*num_shap];
	temp4 = num_shape_workspace[jdofs+num_shap];
	temp5 = num_shape_workspace[jdofs+2*num_shap];
	temp6 = num_shape_workspace[jdofs+3*num_shap];
	
	//float g = el_data_loc[0];
	//float rl = el_data_loc[1];
	//float gprl = g+rl;
	
	temp7 = el_data_loc[0];
	temp8 = el_data_loc[1];
	temp9 = temp7+temp8;
	
	// to save registers
	daux=temp7*(temp4*temp1+temp5*temp2+temp6*temp3);
	
	// convention: ps_ieq_jeq
	ps00 +=    (temp9*temp4*temp1 + daux)*vol;
	
	ps10 +=    (temp8*temp4*temp2 + temp7*temp5*temp1)*vol;
	
	ps20 +=    (temp8*temp4*temp3 + temp7*temp6*temp1)*vol;
	
	ps01 +=    (temp8*temp5*temp1+ temp7*temp4*temp2)*vol;
	
	ps11 +=    (temp9*temp5*temp2+daux)*vol;
	
	ps21 +=    (temp8*temp5 *temp3 + temp7*temp6*temp2)*vol;
	
	ps02 +=    (temp8*temp6 *temp1 + temp7*temp4*temp3)*vol;
	
	ps12 +=    (temp8*temp6 *temp2 + temp7*temp5*temp3)*vol;
	
	ps22 +=    (temp9*temp6*temp3+daux)*vol;
	
	
	nr_oper += 63*work_group_size;
	
      } // end loop over Gauss points
      
      
      // write a part of stiffness matrix to global stiffness matrix - COALESCED
      // write back data from local part_of_stiffness_matrix to its place in global memory
      // zero part of global stiffness matrix - coalesced
      
      // for one nreqxnreq block
      aux_offset = (group_id*nr_elems_per_work_group+ielem)*num_shap*num_shap*nreq2+
	            nreq2*work_group_size*ipart;

      if(ipart==nr_parts_of_stiff_mat-1){
	j = num_shap*num_shap -  ipart*work_group_size;
      }
      else{
	j = work_group_size;
      }

      if(thread_id < j) {

	stiff_mat_out[aux_offset+thread_id] = ps00;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps10;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps20;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps01;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps11;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps21;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps02;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps12;
	aux_offset += j;
	stiff_mat_out[aux_offset+thread_id] = ps22;

      }
      
      //nr_access_global += (nreq2)*work_group_size;
      
      
    } // end loop over parts of stiffness matrix
    
  } // end loop over elements processed by work_group
  
  /* barrier(CLK_GLOBAL_MEM_FENCE); */
  
  /* if(global_id == 0){  */

  /*   stiff_mat_out[0] = nr_oper; */

  /* }   */

  return;

 /* koniec:  */

 /*   if(global_id == 0){   */

/*     stiff_mat_out[0] = nr_oper; */
//     stiff_mat_out[1] = ipart; 
//   stiff_mat_out[2] = nr_parts_of_stiff_mat; 
    /* stiff_mat_out[2] = nr_access_global; */
   /* stiff_mat_out[3] = el_data_in[0]; */
   /* stiff_mat_out[4] = el_data_in[1]; */
   /* stiff_mat_out[5] = ngauss; */
   /* stiff_mat_out[6] = work_group_size; */
   /* stiff_mat_out[7] = nr_elems_per_work_group; */
   /* stiff_mat_out[8] = nr_parts_of_stiff_mat; */
   /* stiff_mat_out[9] = nr_iter_within_part; */
   /* stiff_mat_out[10] = num_shap; */
   /* stiff_mat_out[11] = num_dofs; */
   /* stiff_mat_out[12] = pdeg; */
   /* stiff_mat_out[13] = nreq; */
  /*  stiff_mat_out[14] = el_data_in[14]; */
  /*  stiff_mat_out[15] = el_data_in[15]; */
  /*  stiff_mat_out[16] = el_data_in[16]; */
  /*  stiff_mat_out[17] = el_data_in[17]; */
  /*  stiff_mat_out[18] = el_data_in[18]; */
  /*  stiff_mat_out[19] = el_data_in[19]; */

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
  /*    stiff_mat_out[i+j] = num_shape_workspace[i]; */
  /*  } */

  //}

}

// UP TO NOW - ?? registers
