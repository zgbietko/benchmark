float mat3_inv(float m0, float m1, float m2, float m3,
               float m4, float m5, float m6, float m7, float m8,
               float mi0, float mi1, float mi2, float mi3,
               float mi4, float mi5, float mi6, float mi7, float mi8
              )
{

float rjac = m0*(m4*m8-m7*m5) + m3*(m7*m2-m1*m8) + m6*(m1*m5-m4*m2);
float rjac_inv = 1.0/rjac;
//float rjac_inv = 1.0/(m0*(m4*m8-m7*m5) + m3*(m7*m2-m1*m8) + m6*(m1*m5-m4*m2));

mi0 = (m4*m8 - m7*m5)*rjac_inv;
mi3 = (m6*m5 - m3*m8)*rjac_inv;
mi6 = (m3*m7 - m6*m4)*rjac_inv;

mi1 = (m7*m2 - m1*m8)*rjac_inv;
mi4 = (m0*m8 - m6*m2)*rjac_inv;
mi7 = (m6*m1 - m0*m7)*rjac_inv;

mi2 = (m1*m5 - m4*m2)*rjac_inv;
mi5 = (m3*m2 - m0*m5)*rjac_inv;
mi8 = (m0*m4 - m3*m1)*rjac_inv;

return(fabs(rjac));
//return(fabs(1.0/rjac_inv));

// approx 50 operations (plus two inverses and fabs or one inverse and fabs)
// 1 or 2 registers

}

// VERSION 1
// padding the whole stiff_mat (not every row)
//
// VERSION 2
//  if we can proceed row by row
//    NR_ENTRIES_PER_THREAD = NR_ROWS_AT_ONCE 
//  else
//    NR_ENTRIES_PER_THREAD = NR_ROWS_AT_ONCE * (ROW_SIZE / NR_THREADS_PER_ELEMENT)
// ROW_SIZE is a multiple of NR_THREADS_PER_ELEMENT (padding of stiffness matrix)

#define EL_DATA_IN_SIZE 20
#define EL_DATA_IN_PARAMETERS_SIZE 2

#define nreq 1
#define nreq2 1

// VERSION 1

__kernel void apr_ocl_num_int_el_cell(
  __global int* execution_parameters,
  __global float* gauss_dat, // data for integration of elements having given p
  __global float* el_data_in, // data for integration of NR_ELEM_PER_KERNEL elements
  __global float* stiff_mat_out, // result of integration of NR_ELEM_PER_KERNEL elements
  __local float4 *part_of_stiff_mat,
  __local float *num_shape_workspace,
  __local float* gauss_cache
)
{

//int nr_elems_per_kernel = nr_elems_per_work_group * nr_work_groups;
//int part_of_stiff_mat_size = 9 * work_group_size * nr_iter_within_part;

// ASSUMPTION: one element = one work_group (there may be several work_groups per
//             compute unit and of course per device)
//             one work_group = one or more elements processed in a sequence

	// SHOULD BE CHANGED FOR CPU AND CELL !!!!!!!!!!!!!
	  __local float el_data_loc[EL_DATA_IN_SIZE]; // local data for integration of 1 element

	  const int group_id = get_group_id(0);
	  const int thread_id = get_local_id(0);
	  const int global_id = get_global_id(0);

	  const int nr_work_groups = execution_parameters[0];
	  const int work_group_size = execution_parameters[1];
	  const int nr_elems_per_work_group = execution_parameters[2];
	  const int nr_parts_of_stiff_mat = execution_parameters[3];
	  //const int nr_iter_within_part = execution_parameters[4];


	  // to save registers this is #defined
	  //const int nreq = execution_parameters[5];
	  //const int nreq2 = nreq*nreq;

	  const int pdeg = execution_parameters[6];

	  // if there is not enough registers we can substitute inline calculations
	  // for num_shap and num_dofs and ngauss
	  const int num_shap = (pdeg+1)*(pdeg+1)*(pdeg+2)/2;
	  const int num_dofs = nreq*num_shap;
	  //const int num_shap = execution_parameters[7];

          const int nr_rows_within_part = execution_parameters[8];
          const int nr_rows_last_part = execution_parameters[9];



	  const int ngauss = execution_parameters[10];
	  if(global_id==120)
    {
  printf("group_id=%d,thread_id=%d,global_id=%d,nr_work_groups=%d,work_grup_size=%d,"
	  "nr_elems_per_work_group=%d,nr_parts_of_stiff_mat=%d,nr_rows_within_part=%d,"
          "pdeg=%d,num_shap=%d,num_dofs=%d,ngauss=%d, nr_rows_last_part %d\n",
	  group_id, thread_id, global_id, nr_work_groups, work_group_size,
          nr_elems_per_work_group, nr_parts_of_stiff_mat, nr_rows_within_part,
          pdeg, num_shap, num_dofs, ngauss, nr_rows_last_part);
  }

	//int nr_elems_per_kernel = nr_elems_per_work_group * nr_work_groups;
	//int part_of_stiff_mat_size = nreq2 * work_group_size * nr_iter_within_part;

	  float nr_oper = 0;
	  float nr_access = 0;

	  int i,j,k; // for small loops
	  int start; // starting index used during auxiliary calculations
	  int local_offset; // offset used for accesing local matrices
	  int aux_offset; // offset used for accesing arrays (one - to save registers)

	  float temp1, temp2, temp3;
	  float temp4, temp5, temp6;
	  float temp7, temp8, temp9;
	  float daux, faux, eaux, vol;

	// UP TO NOW - 33 registers + 1 or 2 in matrix inverse

	  int ielem;

		  for(ielem = 0; ielem < nr_elems_per_work_group; ielem++){

//printf("ielem %d\n", ielem);

		  // SHOULD BE CHANGED FOR CPU AND CELL !!!!!!!!!!!!!
		      // first EL_DATA_IN_SIZE threads from a given thread block read element data
		      // from global memory to local memory  - COALESCED READ
		      for(i=0; i<EL_DATA_IN_SIZE; i++){
//el_data_loc[0+i] = 0;
//printf("i %d, indeks %d\n", i, EL_DATA_IN_SIZE*ielem+i);
		        el_data_loc[0+i] = el_data_in[EL_DATA_IN_SIZE*ielem+i];
		      }

		      // loop over parts of stiffness matrix
		      int ipart;
		      for(ipart = 0; ipart < nr_parts_of_stiff_mat; ipart++){

//printf("ipart %d\n", ipart);


                        int nr_iter_within_part = nr_rows_within_part*num_shap;
                        if(ipart==nr_parts_of_stiff_mat-1) nr_iter_within_part = nr_rows_last_part*num_shap;

		        int iiter;
		        for(iiter = 0; iiter < nr_iter_within_part; iiter++ ){
//printf("iiter %d\n", iiter);

		            part_of_stiff_mat[iiter] = (float4)(0.0f);

		        } // end loop over iterations over part of stiffness matrix
		          // i.e. entries per single thread


		        int igauss;
		        // in a loop over gauss points
		        for(igauss = 0; igauss < ngauss; igauss++){
//printf("igauss %d\n", igauss);

		  //     compute apr_elem_calc_3D data - SINGLE thread
		  //       1. copy gauss data to registers
		  //       2. perform calculations using registers
		  //       3. save data to local variables

		  // UP TO NOW - 37 registers + 1 or 2 in matrix inverse

		          if(thread_id == 0) {
		    //       if(thread_id == 0 && ipart < 0) {

		  //******************* BEGINNING OF APR_ELEM_CALC_3D **********************//
		    //float coor_1 = gauss_dat[4*igauss];
		    //float coor_2 = gauss_dat[4*igauss+1];
		    //float coor_3 = gauss_dat[4*igauss+2];

		  // to save registers
		    daux = gauss_dat[4*igauss];
		    faux = gauss_dat[4*igauss+1];
		    eaux = gauss_dat[4*igauss+2];

//printf("daux %f, faux %f, eaux %f\n", daux, faux, eaux);

		    // partition of workspace for shape functions
		    // 4*((pdeg+1)*(pdeg+1)*(pdeg+2)/2) - 3D shape functions and their 3 derivatives
		    // 3*((pdeg+1)*(pdeg+2)/2) - 2D shape functions for bases and their 2 derivatives
		    // 3*2*(pdeg+1) - 3 times auxiliary 1D shape functions and their 1 derivative

		    // shape_fun_3D starts at 0 (size_3D = ((pdeg+1)*(pdeg+1)*(pdeg+2)/2))
		    // shape_fun_2D starts at start_2D = 4*size_3D
		    // size_2D = (pdeg+1)*(pdeg+2)/2
		    // shape_fun_1D for x starts at start_1D = start_2D + 3*size_2D
		    // shape_fun_1D for y starts at start_1D + 2*size_1D (size_1D=(pdeg+1))
		    // shape_fun_1D for z starts at start_1D + 4*size_1D

		    //int size_3D = (pdeg+1)*(pdeg+1)*(pdeg+2)/2;
		    //int start_2D = 4*size_3D;
		    //int size_2D = (pdeg+1)*(pdeg+2)/2;
		    //int start_1D = start_2D + 3*size_2D;
		    //int size_1D = (pdeg+1);


		  // first we compute 3 times 1D shape functions
		  // for coor_1=daux, coor_2=faux and coor_3=eaux
		    //start = start_2D + 3*size_2D;
		    start = 4*(pdeg+1)*(pdeg+1)*(pdeg+2)/2 + 3*(pdeg+1)*(pdeg+2)/2;
//printf("start %d, end %d\n", start, start+pdeg+5*(pdeg+1)); 
		    num_shape_workspace[start+0] = 1.0;
		    num_shape_workspace[start+1] = daux;
		    num_shape_workspace[start+0+(pdeg+1)]  = 0.0;
		    num_shape_workspace[start+1+(pdeg+1)]  = 1.0;
		    num_shape_workspace[start+0+2*(pdeg+1)] = 1.0;
		    num_shape_workspace[start+1+2*(pdeg+1)] = faux;
		    num_shape_workspace[start+0+3*(pdeg+1)]  = 0.0;
		    num_shape_workspace[start+1+3*(pdeg+1)]  = 1.0;
		    num_shape_workspace[start+0+4*(pdeg+1)] = 1.0;
		    num_shape_workspace[start+1+4*(pdeg+1)] = eaux;
		    num_shape_workspace[start+0+5*(pdeg+1)]  = 0.0;
		    num_shape_workspace[start+1+5*(pdeg+1)]  = 1.0;
		    temp1 = 1.0;
		    temp2 = 1.0;
		    temp3 = 1.0;
		    for (i=2;i<=pdeg; i++) {
		      temp1  *= daux;
		      temp2  *= faux;
		      temp3  *= eaux;
		      num_shape_workspace[start+i]  = temp1*daux;
		      num_shape_workspace[start+i+(pdeg+1)]  = temp1*i;
		      num_shape_workspace[start+i+2*(pdeg+1)]  = temp2*faux;
		      num_shape_workspace[start+i+3*(pdeg+1)]  = temp2*i;
		      num_shape_workspace[start+i+4*(pdeg+1)]  = temp3*eaux;
		      num_shape_workspace[start+i+5*(pdeg+1)]  = temp3*i;
		    }


		  // then we compute 2D shape functions
		    //int aux_offset = start_2D + 3*size_2D;
		    // start for 1D shape functions for daux
		    aux_offset = 4*(pdeg+1)*(pdeg+1)*(pdeg+2)/2 + 3*(pdeg+1)*(pdeg+2)/2;
		    // start for 1D shape functions for faux
		    local_offset = aux_offset + 2*(pdeg+1);
		    // start for 2D shape functions
		    start = 4*(pdeg+1)*(pdeg+1)*(pdeg+2)/2;
		    k=0;
		    for (i=0;i<=pdeg;i++) {
		      for (j=0;j<=pdeg-i;j++) {
		        temp1 = num_shape_workspace[aux_offset+i];
		        temp2 = num_shape_workspace[local_offset+j];
		        temp3 = num_shape_workspace[aux_offset+(pdeg+1)+i];
		        temp4 = num_shape_workspace[local_offset+(pdeg+1)+j];
		        num_shape_workspace[start+k] =   temp1 * temp2;
		        num_shape_workspace[start+k+1] = temp3 * temp2;
		        num_shape_workspace[start+k+2] = temp1 * temp4;
		        k+=3;
		      }
		    }

		  // and finally 3D shape functions
		    // start for 2D shape functions - already set
		    //int start = 4*(pdeg+1)*(pdeg+1)*(pdeg+2)/2;
		    // start for 1D shape functions for eaux
		    aux_offset = start + 3*(pdeg+1)*(pdeg+2)/2 + 4*(pdeg+1);
		    // start for 3D shape functions is 0
		    k=0;
		    for(i=0;i<(pdeg+1)*(pdeg+2)/2;i++){
		      for(j=0;j<pdeg+1;j++){
		        temp1 = num_shape_workspace[start+i];
		        temp2 = num_shape_workspace[aux_offset+j];
		        temp3 = num_shape_workspace[start+i+1];
		        temp4 = num_shape_workspace[start+i+2];
		        temp5 = num_shape_workspace[aux_offset+(pdeg+1)+j];
		        num_shape_workspace[k] =   temp1 * temp2;
		        num_shape_workspace[k+1] = temp3 * temp2;
		        num_shape_workspace[k+2] = temp4 * temp2;
		        num_shape_workspace[k+3] = temp1 * temp5;
		        k+=4;
		      }
		    }

		  // geometrical shape functions are computed at place for 2D shape functions
		    // start for 2D shape functions - already set
		    //int start = 4*(pdeg+1)*(pdeg+1)*(pdeg+2)/2;
		    num_shape_workspace[start+0]=(1.0-daux-faux)*(1.0-eaux)/2.0;
		    num_shape_workspace[start+1]=daux*(1.0-eaux)/2.0;
		    num_shape_workspace[start+2]=faux*(1.0-eaux)/2.0;
		    num_shape_workspace[start+3]=(1.0-daux-faux)*(1.0+eaux)/2.0;
		    num_shape_workspace[start+4]=daux*(1.0+eaux)/2.0;
		    num_shape_workspace[start+5]=faux*(1.0+eaux)/2.0;

		  // derivatives of geometrical shape functions
		    aux_offset = start + 6;
		    num_shape_workspace[aux_offset+0] = -(1.0-eaux)/2.0;
		    num_shape_workspace[aux_offset+1] =  (1.0-eaux)/2.0;
		    num_shape_workspace[aux_offset+2] =  0.0;
		    num_shape_workspace[aux_offset+3] = -(1.0+eaux)/2.0;
		    num_shape_workspace[aux_offset+4] =  (1.0+eaux)/2.0;
		    num_shape_workspace[aux_offset+5] =  0.0;
		    aux_offset+=6;
		    num_shape_workspace[aux_offset+0] = -(1.0-eaux)/2.0;
		    num_shape_workspace[aux_offset+1] =  0.0;
		    num_shape_workspace[aux_offset+2] =  (1.0-eaux)/2.0;
		    num_shape_workspace[aux_offset+3] = -(1.0+eaux)/2.0;
		    num_shape_workspace[aux_offset+4] =  0.0;
		    num_shape_workspace[aux_offset+5] =  (1.0+eaux)/2.0;
		    aux_offset+=6;
		    num_shape_workspace[aux_offset+0] = -(1.0-daux-faux)/2.0;
		    num_shape_workspace[aux_offset+1] = -daux/2.0;
		    num_shape_workspace[aux_offset+2] = -faux/2.0;
		    num_shape_workspace[aux_offset+3] =  (1.0-daux-faux)/2.0;
		    num_shape_workspace[aux_offset+4] =  daux/2.0;
		    num_shape_workspace[aux_offset+5] =  faux/2.0;

		    aux_offset += 6;
		    num_shape_workspace[aux_offset+0] = 0.0;
		    num_shape_workspace[aux_offset+1] = 0.0;
		    num_shape_workspace[aux_offset+2] = 0.0;
		    num_shape_workspace[aux_offset+3] = 0.0;
		    num_shape_workspace[aux_offset+4] = 0.0;
		    num_shape_workspace[aux_offset+5] = 0.0;
		    num_shape_workspace[aux_offset+6] = 0.0;
		    num_shape_workspace[aux_offset+7] = 0.0;
		    num_shape_workspace[aux_offset+8] = 0.0;

		  /* Jacobian matrix J */
		    local_offset = EL_DATA_IN_PARAMETERS_SIZE;
		    for(i=0;i<6;i++){
		      temp1 = num_shape_workspace[start+6+i];
		      temp2 = num_shape_workspace[start+12+i];
		      temp3 = num_shape_workspace[start+18+i];
		      temp4 = el_data_loc[local_offset+3*i];
		      temp5 = el_data_loc[local_offset+3*i+1];
		      temp6 = el_data_loc[local_offset+3*i+2];
		      num_shape_workspace[aux_offset+0] += temp4 * temp1;
		      num_shape_workspace[aux_offset+1] += temp4 * temp2;
		      num_shape_workspace[aux_offset+2] += temp4 * temp3;
		      num_shape_workspace[aux_offset+3] += temp5 * temp1;
		      num_shape_workspace[aux_offset+4] += temp5 * temp2;
		      num_shape_workspace[aux_offset+5] += temp5 * temp3;
		      num_shape_workspace[aux_offset+6] += temp6 * temp1;
		      num_shape_workspace[aux_offset+7] += temp6 * temp2;
		      num_shape_workspace[aux_offset+8] += temp6 * temp3;
		    }

		  /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
		    vol = gauss_dat[4*igauss+3] *
		          mat3_inv( num_shape_workspace[aux_offset+0],
		                    num_shape_workspace[aux_offset+1],
		                    num_shape_workspace[aux_offset+2],
		                    num_shape_workspace[aux_offset+3],
		                    num_shape_workspace[aux_offset+4],
		                    num_shape_workspace[aux_offset+5],
		                    num_shape_workspace[aux_offset+6],
		                    num_shape_workspace[aux_offset+7],
		                    num_shape_workspace[aux_offset+8],
		  //                  temp1, temp2, temp3,
		  //                  temp4, temp5, temp6,
		  //                  temp7, temp8, temp9);
		                    num_shape_workspace[aux_offset+9],
		                    num_shape_workspace[aux_offset+10],
		                    num_shape_workspace[aux_offset+11],
		                    num_shape_workspace[aux_offset+12],
		                    num_shape_workspace[aux_offset+13],
		                    num_shape_workspace[aux_offset+14],
		                    num_shape_workspace[aux_offset+15],
		                    num_shape_workspace[aux_offset+16],
		                    num_shape_workspace[aux_offset+17]
		                  );


		    // start of inverse Jacobian matrix - just 9 entries after start of Jacobian
		    start = aux_offset + 9;


		      nr_oper += (4*num_shap + 3*(pdeg+1)*(pdeg+2)/2 + 6*(pdeg+1) + 220);

} // end artificial !!!!!!!!!!!!


		  //******************* THE END OF SINGLE THREAD REGION  **********************//

//		          barrier(CLK_LOCAL_MEM_FENCE);

                       float4 num_shape_der[288];

		            for(i=0;i<num_shap;i++){


		    // start for 3D shape functions is 0
		              daux = num_shape_workspace[4*i+1];
		              eaux = num_shape_workspace[4*i+2];
		              faux = num_shape_workspace[4*i+3];

//printf("ishape %d, daux %f, faux %f, eaux %f\n", i, daux, faux, eaux);
                              //num_shape_der[i].x = daux;
                              //num_shape_der[i].y = eaux;
                              //num_shape_der[i].z = faux;


		              temp1 = num_shape_workspace[start+0];
		              temp2 = num_shape_workspace[start+1];
		              temp3 = num_shape_workspace[start+2];
		              temp4 = num_shape_workspace[start+3];
		              temp5 = num_shape_workspace[start+4];
		              temp6 = num_shape_workspace[start+5];
		              temp7 = num_shape_workspace[start+6];
		              temp8 = num_shape_workspace[start+7];
		              temp9 = num_shape_workspace[start+8];

                              num_shape_der[i].x = daux*temp1 + eaux*temp4 +  faux*temp7;
                              num_shape_der[i].y = daux*temp2 + eaux*temp5 +  faux*temp8;
                              num_shape_der[i].z = daux*temp3 + eaux*temp6 +  faux*temp9;

		         }

		      nr_oper += 15*num_shap;
		      //nr_oper += 15*j*work_group_size;

		  //******************* THE END OF APR_ELEM_CALC_3D **********************//


		  // UP TO NOW - 37 registers + 1 or 2 in matrix inverse

                          int idofs=0; int jdofs=0; iiter=0;

                          int start = nr_rows_within_part*ipart;
                          int end;
                          if(ipart<nr_parts_of_stiff_mat-1) end = nr_rows_within_part*(ipart+1); 
                          else end = start + nr_rows_last_part;
                          float4 vol_vec;
                          vol_vec.x = vol;
                          vol_vec.y = vol;
                          vol_vec.z = vol;



                          for(idofs = start; idofs < end; idofs+=8 ){

                            float4 dphii=num_shape_der[idofs]; 
                            float4 dphii2=num_shape_der[idofs+1]; 
                            float4 dphii3=num_shape_der[idofs+2]; 
                            float4 dphii4=num_shape_der[idofs+3]; 

                            float4 dphii5=num_shape_der[idofs+4]; 
                            float4 dphii6=num_shape_der[idofs+5]; 
                            float4 dphii7=num_shape_der[idofs+6]; 
                            float4 dphii8=num_shape_der[idofs+7]; 


                            for(jdofs=0; jdofs<num_shap; jdofs +=2){ 

                              float4 dphij=num_shape_der[jdofs];
                              float4 dphij2=num_shape_der[jdofs];

                              float4 bb=dphii*dphij;
                              float4 bb2=dphii2*dphij;
                              float4 bb3=dphii*dphij2;
                              float4 bb4=dphii2*dphij2;
                              float4 bb5=dphii3*dphij;
                              float4 bb6=dphii3*dphij2;
                              float4 bb7=dphii4*dphij;
                              float4 bb8=dphii4*dphij2;
                              float4 bb9=dphii5*dphij;
                              float4 bb10=dphii5*dphij2;
                              float4 bb11=dphii6*dphij;
                              float4 bb12=dphii6*dphij2;
                              float4 bb13=dphii7*dphij;
                              float4 bb14=dphii7*dphij2;
                              float4 bb15=dphii8*dphij;
                              float4 bb16=dphii8*dphij2;

                              part_of_stiff_mat[iiter]+=vol_vec*bb;
                              part_of_stiff_mat[iiter+1]+=vol_vec*bb2;
                              part_of_stiff_mat[iiter+2]+=vol_vec*bb3;
                              part_of_stiff_mat[iiter+3]+=vol_vec*bb4;
                              part_of_stiff_mat[iiter+4]+=vol_vec*bb5;
                              part_of_stiff_mat[iiter+5]+=vol_vec*bb6;
                              part_of_stiff_mat[iiter+6]+=vol_vec*bb7;
                              part_of_stiff_mat[iiter+7]+=vol_vec*bb8;
                              part_of_stiff_mat[iiter+8]+=vol_vec*bb9;
                              part_of_stiff_mat[iiter+9]+=vol_vec*bb10;
                              part_of_stiff_mat[iiter+10]+=vol_vec*bb11;
                              part_of_stiff_mat[iiter+11]+=vol_vec*bb12;
                              part_of_stiff_mat[iiter+12]+=vol_vec*bb13;
                              part_of_stiff_mat[iiter+13]+=vol_vec*bb14;
                              part_of_stiff_mat[iiter+14]+=vol_vec*bb15;
                              part_of_stiff_mat[iiter+15]+=vol_vec*bb16;

                              iiter+=16;


                          } // for jdofs

		          } // end loop over iterations over entries of stiff mat 
                         
		        } // end loop over Gauss points
		 
		        nr_oper += 12*nr_iter_within_part*ngauss*work_group_size;
		        nr_access += 2*4*nr_iter_within_part*ngauss*work_group_size;

		        // write a part of stiffness matrix to global stiffness matrix - COALESCED
		        // write back data from local part_of_stiffness_matrix to its place in global memory
		        // zero part of global stiffness matrix - coalesced

		        // for one nreqxnreq block
		        aux_offset = nreq2*work_group_size*(nr_iter_within_part*
		  	      (ipart+nr_parts_of_stiff_mat*(ielem+nr_elems_per_work_group*group_id)));
		        local_offset = 0;

		        for(iiter = 0; iiter < nr_iter_within_part; iiter++ ){

                            //laplace
//                              a_int[kk+idofs] +=      (
//                              base_dphix[jdofs] *base_dphix[idofs] +
//                              base_dphiy[jdofs] *base_dphiy[idofs] +
//                              base_dphiz[jdofs] *base_dphiz[idofs]
//                                                ) * vol;

		            stiff_mat_out[aux_offset+thread_id] = part_of_stiff_mat[local_offset+thread_id].x+
                                                                  part_of_stiff_mat[local_offset+thread_id].y+
                                                                  part_of_stiff_mat[local_offset+thread_id].z;
		            aux_offset += work_group_size;
		            local_offset += work_group_size;

		        } // end loop over iterations over part of stiffness matrix
		          // i.e. entries per single thread

		      } // end loop over parts of stiffness matrix

		    } // end loop over elements processed by work_group

		    //barrier(CLK_GLOBAL_MEM_FENCE);

		    if(global_id == 0){

		      stiff_mat_out[0] = nr_oper;
		      stiff_mat_out[1] = nr_access;
		     //stiff_mat_out[2] = el_data_in[0];
		     //stiff_mat_out[3] = el_data_in[1];
		     //stiff_mat_out[4] = ngauss;
		     //stiff_mat_out[5] = nr_work_groups;
		     //stiff_mat_out[6] = work_group_size;
		     //stiff_mat_out[7] = nr_elems_per_work_group;
		     //stiff_mat_out[8] = nr_parts_of_stiff_mat;
		     //stiff_mat_out[9] = nr_iter_within_part;
		     //stiff_mat_out[10] = num_shap;
		     //stiff_mat_out[11] = num_dofs;
		     //stiff_mat_out[12] = pdeg;
		     //stiff_mat_out[13] = nreq;
		     //stiff_mat_out[14] = el_data_in[14];
		     //stiff_mat_out[15] = el_data_in[15];
		     //stiff_mat_out[16] = el_data_in[16];
		     //stiff_mat_out[17] = el_data_in[17];
		     //stiff_mat_out[18] = el_data_in[18];
		     //stiff_mat_out[19] = el_data_in[19];

		     //stiff_mat_out[20] = gauss_dat[0];
		     //stiff_mat_out[21] = gauss_dat[1];
		     //stiff_mat_out[22] = gauss_dat[2];
		     //stiff_mat_out[23] = gauss_dat[3];
		     //stiff_mat_out[24] = gauss_dat[4];
		     //stiff_mat_out[25] = gauss_dat[5];
		     //stiff_mat_out[26] = gauss_dat[6];
		     //stiff_mat_out[27] = gauss_dat[7];
		     //stiff_mat_out[28] = gauss_dat[8];
		     //stiff_mat_out[29] = gauss_dat[9];

		     // j = 9*num_shap*num_shap-10;
		     // for(i=0;i<10;i++){
		       //stiff_mat_out[i+j] = num_shape_workspace[i];
		     // }

		    }




}
