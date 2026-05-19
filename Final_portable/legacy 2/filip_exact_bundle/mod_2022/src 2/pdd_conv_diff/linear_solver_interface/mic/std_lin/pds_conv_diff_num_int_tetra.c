#include "mic.h"

#define LOCAL_STIFF

#define NGAUSS 4
#define NSHAP 4
#define NGEO 4
#define NDOFS 4

//#define THR 240

int pdr_num_int_el_QSS_tetra(
		SCALAR* gauss_dat_host, // integration points data of elements having given p
		SCALAR* shape_fun_host, // shape functions on a reference element
		SCALAR* el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
		SCALAR* el_data_out, // result of integration of NR_ELEMS_THIS_KERCALL elements
		const int nr_elem_mic,
		const int size_el_out,
		const int size_el_in,
		const int size_shp,
		const int geo_dat_size,
		const int nr_coeff,
		const int one_el_stiff_mat_size,
		const int one_el_load_vec_size
)
{
	
	//printf("gauss_dat->%p, *gauss_dat->%p\n",gauss_dat_host,*gauss_dat_host);
	//printf("shape_fun_host->%p, *shape_fun_host->%p\n",shape_fun_host,*shape_fun_host);
	//printf("el_data_in->%p, *el_data_in->%p\n",el_data_in,*el_data_in);
	//printf("el_data_out->%p, *el_data_out->%p\n",el_data_out,*el_data_out);
/*
#ifdef TUNING
	int nthr=1;
	#ifdef MIC
		#pragma offload target(mic) out(nthr)
	#endif
		{
			#pragma omp parallel
			{
				if(omp_get_thread_num()==0)
					{
					nthr=omp_get_num_threads();
					}
			}
		}
		if(line_count==0)
			fprintf(headuf,"Threads,");
		fprintf(resuf,"%d,",nthr);
#endif
*/

#ifdef TIME_TEST
    double t_begin_in = time_clock();
#endif

		#ifdef MIC
#pragma offload target(mic:0) in(gauss_dat_host: length(1344) alloc_if(1) free_if(1)) in(shape_fun_host: length(size_shp) alloc_if(1) free_if(1)) \
    in(el_data_in: length(size_el_in) alloc_if(1) free_if(1)) out(el_data_out: length(size_el_out) alloc_if(1) free_if(1))
		#endif
{

	int ielem;
#ifdef MIC
	register int __attribute__((target(mic))) offset;
#else
	register int offset;
#endif

#ifdef THR
	omp_set_num_threads(THR);
#endif

#pragma vector always

	__assume_aligned(gauss_dat_host,ALIGN);
	__assume_aligned(shape_fun_host,ALIGN);
	__assume_aligned(el_data_in,ALIGN);
	__assume_aligned(el_data_out,ALIGN);

	//omp_set_num_threads(1);

#pragma omp parallel default(none) private(offset,ielem) firstprivate(nr_elem_mic,size_el_out,size_el_in,size_shp,geo_dat_size,nr_coeff,one_el_stiff_mat_size,one_el_load_vec_size) shared(gauss_dat_host,el_data_in,el_data_out,shape_fun_host)
{
	//-------------------------------------------------------------
	//******************* loop over elements processed by a thread *********************

#pragma ivdep
#pragma vector aligned
	#pragma omp for schedule (guided) nowait
	 for(ielem = 0; ielem < nr_elem_mic; ielem++){


			#ifdef COMPUTE_ALL_SHAPE_FUN_DER
				//register SCALAR *workspace = (SCALAR *) malloc((geo_dat_size+nr_coeff)*sizeof(SCALAR));
//				SCALAR *tab_fun_u_derx = (SCALAR *) _mm_malloc(num_shap*sizeof(SCALAR),64);
//				SCALAR *tab_fun_u_dery = (SCALAR *) _mm_malloc(num_shap*sizeof(SCALAR),64);
//				SCALAR *tab_fun_u_derz = (SCALAR *) _mm_malloc(num_shap*sizeof(SCALAR),64);

		 	 	__declspec(align(ALIGN)) SCALAR tab_fun_u_derx[NSHAP];
		     	__declspec(align(ALIGN)) SCALAR tab_fun_u_dery[NSHAP];
		     	__declspec(align(ALIGN)) SCALAR tab_fun_u_derz[NSHAP];

			#endif

			#ifdef LOCAL_STIFF
				__declspec(align(ALIGN)) SCALAR stiff_mat[NDOFS*NDOFS];
				__declspec(align(ALIGN)) SCALAR load_vec[NDOFS];
			#endif

			int i;

		//-------------------------------------------------------------
		// ******************* READING INPUT DATA *********************

		//	    printf("nr_coeff=%d\n",nr_coeff);

			#ifdef REGISTERS

				#ifdef TEST_SCALAR

					offset=nr_elem_mic*geo_dat_size+ielem*nr_coeff;

#ifdef MIC
			   register SCALAR __attribute__((target(mic))) coeff00=el_data_in[offset+0];
			   register SCALAR __attribute__((target(mic))) coeff01=el_data_in[offset+1];
			   register SCALAR __attribute__((target(mic))) coeff02=el_data_in[offset+2];
			   register SCALAR __attribute__((target(mic))) coeff10=el_data_in[offset+3];
			   register SCALAR __attribute__((target(mic))) coeff11=el_data_in[offset+4];
			   register SCALAR __attribute__((target(mic))) coeff12=el_data_in[offset+5];
			   register SCALAR __attribute__((target(mic))) coeff20=el_data_in[offset+6];
			   register SCALAR __attribute__((target(mic))) coeff21=el_data_in[offset+7];
			   register SCALAR __attribute__((target(mic))) coeff22=el_data_in[offset+8];
			   register SCALAR __attribute__((target(mic))) coeff30=el_data_in[offset+9];
			   register SCALAR __attribute__((target(mic))) coeff31=el_data_in[offset+10];
			   register SCALAR __attribute__((target(mic))) coeff32=el_data_in[offset+11];
			   register SCALAR __attribute__((target(mic))) coeff03=el_data_in[offset+12];
			   register SCALAR __attribute__((target(mic))) coeff13=el_data_in[offset+13];
			   register SCALAR __attribute__((target(mic))) coeff23=el_data_in[offset+14];
			   register SCALAR __attribute__((target(mic))) coeff33=el_data_in[offset+15];
			   register SCALAR __attribute__((target(mic))) coeff04=el_data_in[offset+16];
			   register SCALAR __attribute__((target(mic))) coeff14=el_data_in[offset+17];
			   register SCALAR __attribute__((target(mic))) coeff24=el_data_in[offset+18];
			   register SCALAR __attribute__((target(mic))) coeff34=el_data_in[offset+19];
#else
			   register SCALAR  coeff00=el_data_in[offset+0];
			   register SCALAR  coeff01=el_data_in[offset+1];
			   register SCALAR  coeff02=el_data_in[offset+2];
			   register SCALAR  coeff10=el_data_in[offset+3];
			   register SCALAR  coeff11=el_data_in[offset+4];
			   register SCALAR  coeff12=el_data_in[offset+5];
			   register SCALAR  coeff20=el_data_in[offset+6];
			   register SCALAR  coeff21=el_data_in[offset+7];
			   register SCALAR  coeff22=el_data_in[offset+8];
			   register SCALAR  coeff30=el_data_in[offset+9];
			   register SCALAR  coeff31=el_data_in[offset+10];
			   register SCALAR  coeff32=el_data_in[offset+11];
			   register SCALAR  coeff03=el_data_in[offset+12];
			   register SCALAR  coeff13=el_data_in[offset+13];
			   register SCALAR  coeff23=el_data_in[offset+14];
			   register SCALAR  coeff33=el_data_in[offset+15];
			   register SCALAR  coeff04=el_data_in[offset+16];
			   register SCALAR  coeff14=el_data_in[offset+17];
			   register SCALAR  coeff24=el_data_in[offset+18];
			   register SCALAR  coeff34=el_data_in[offset+19];
#endif

				#endif

			#endif

		// ******* THE END OF: READING INPUT DATA *********************
		//-------------------------------------------------------------


//-------------------------------------------------------------
//************************* JACOBIAN TERMS CALCULATIONS *************************//

    SCALAR vol = weight_linear_tetra;	// vol = weight CONSTANT FOR TETRA!!!

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
    SCALAR daux;

#ifdef COMPUTE_ALL_SHAPE_FUN_DER
    //{ // block to indicate the scope of jac_x registers
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

      offset=ielem*geo_dat_size;

      jac_1 = el_data_in[offset+3*0];
      jac_2 = el_data_in[offset+3*0+1];
      jac_3 = el_data_in[offset+3*0+2];
      temp1 = el_data_in[offset+3*1]   - jac_1;;
      temp4 = el_data_in[offset+3*1+1] - jac_2;
      temp7 = el_data_in[offset+3*1+2] - jac_3;
      temp2 = el_data_in[offset+3*2]   - jac_1;;
      temp5 = el_data_in[offset+3*2+1] - jac_2;
      temp8 = el_data_in[offset+3*2+2] - jac_3;
      temp3 = el_data_in[offset+3*3]   - jac_1;;
      temp6 = el_data_in[offset+3*3+1] - jac_2;
      temp9 = el_data_in[offset+3*3+2] - jac_3;

      jac_0 = (temp5*temp9 - temp8*temp6);
      jac_1 = (temp8*temp3 - temp2*temp9);
      jac_2 = (temp2*temp6 - temp3*temp5);

      daux = temp1*jac_0 + temp4*jac_1 + temp7*jac_2;

      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
      vol *= daux; // vol = weight * det J

      SCALAR faux = one/daux;

      jac_0 *= faux;
      jac_1 *= faux;
      jac_2 *= faux;

      jac_3 = (temp6*temp7 - temp4*temp9)*faux;
      jac_4 = (temp1*temp9 - temp7*temp3)*faux;
      jac_5 = (temp3*temp4 - temp1*temp6)*faux;

      jac_6 = (temp4*temp8 - temp5*temp7)*faux;
      jac_7 = (temp2*temp7 - temp1*temp8)*faux;
      jac_8 = (temp1*temp5 - temp2*temp4)*faux;

//************* THE END OF: JACOBIAN TERMS CALCULATIONS *************************//
//-------------------------------------------------------------

//-------------------------------------------------------------
//******************** INITIALIZING SM AND LV ******************//

#ifdef LOCAL_STIFF
	#pragma ivdep
	#pragma vector aligned
	#pragma simd
			for(i = 0; i < NDOFS*NDOFS; i++) stiff_mat[i] = zero;

			  #ifdef LOAD_VEC_COMP
	#pragma ivdep
	#pragma vector aligned
	#pragma simd
				for(i = 0; i < NDOFS; i++) load_vec[i] = zero;
			  #endif
#else
		offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
		for(i = 0; i < one_el_stiff_mat_size; i++) el_data_out[offset+i] = zero;

	  #ifdef LOAD_VEC_COMP
		for(i = 0; i < one_el_load_vec_size; i++) el_data_out[offset+one_el_stiff_mat_size+i] = zero;
	  #endif
#endif




//******************** END OF: INITIALIZING SM AND LV ******************//
//-------------------------------------------------------------


//-------------------------------------------------------------
//************************* LOOP OVER INTEGRATION POINTS ************************//

    // in a loop over gauss points
    int igauss;
    int idof, jdof;
    for(igauss = 0; igauss < NGAUSS; igauss++){

    	//-------------------------------------------------------------
    	//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//

    	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

    	//************ loop for computing ALL shape function values **********//
    	#pragma vector aligned
    	#pragma ivdep
    	      for(idof = 0; idof < NSHAP; idof++){

    		// read proper values of shape functions and their derivatives
    	//	temp1 = shape_fun_host[4*idof+1];
    	//	temp2 = shape_fun_host[4*idof+2];
    	//	temp3 = shape_fun_host[4*idof+3];

    		temp1 = shape_fun_host[igauss*NSHAP+idof];
    		temp2 = shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof];
    		temp3 = shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof];

    		tab_fun_u_derx[idof] = temp1*jac_0+temp2*jac_3+temp3*jac_6;
    		tab_fun_u_dery[idof] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
    		tab_fun_u_derz[idof] = temp1*jac_2+temp2*jac_5+temp3*jac_8;

    	      } // end loop over shape functions for which global derivatives were computed

    	      //} // the end of block to indicate the scope of jac_x registers

    	#endif // end if COMPUTE_ALL_SHAPE_FUN_DER

    	//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
    	//-------------------------------------------------------------


//-------------------------------------------------------------
//***** SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS *****//

#ifdef REGISTERS

  #ifdef LAPLACE

      // offset for reading data
      register int offset2=nr_elem_mic*geo_dat_size+ielem*nr_coeff;

#ifdef MIC
	      register SCALAR __attribute__((target(mic))) coeff03 = el_data_in[offset2+igauss];
#else
	      register SCALAR coeff03 = el_data_in[offset2+igauss];
#endif

  #endif // end if LAPLACE

#endif

//*** THE END OF: SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS ***//
//-------------------------------------------------------------


//-------------------------------------------------------------
//********************* first loop over shape functions ***********************//
      for(idof = 0; idof < NSHAP; idof++){

	{ // beginning of using registers for u  (shp_fun_u, fun_u_der.)


//-------------------------------------------------------------
//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ******//

#ifdef COMPUTE_ALL_SHAPE_FUN_DER

		// read proper values of shape functions and their derivatives

		SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];
		//SCALAR shp_fun_u = shape_fun_host[igauss*4*num_shap+4*idof];
		SCALAR fun_u_derx = tab_fun_u_derx[idof];
		SCALAR fun_u_dery = tab_fun_u_dery[idof];
		SCALAR fun_u_derz = tab_fun_u_derz[idof];

#else // if not COMPUTE_ALL_SHAPE_FUN_DER

	  // read proper values of shape functions and their derivatives
//	  SCALAR shp_fun_u = shape_fun_host[igauss*4*num_shap+4*idof];
//	  temp1 = shape_fun_host[4*idof+1];
//	  temp2 = shape_fun_host[4*idof+2];
//	  temp3 = shape_fun_host[4*idof+3];

	  SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];
	  temp1 = shape_fun_host[igauss*NSHAP+idof];
	  temp2 = shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof];
	  temp3 = shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof];


	  // compute derivatives wrt global coordinates
	  // 15 operations
	  SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
	  SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
	  SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//*** ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//

#ifdef LAPLACE

		temp4=fun_u_derx;
		temp5=fun_u_dery;
		temp6=fun_u_derz;

#elif defined(TEST_SCALAR)

		#ifdef REGISTERS

	  	  temp4 = coeff00*fun_u_derx + coeff01*fun_u_dery + coeff02*fun_u_derz + coeff03*shp_fun_u;
		  temp5 = coeff10*fun_u_derx + coeff11*fun_u_dery + coeff12*fun_u_derz + coeff13*shp_fun_u;
		  temp6 = coeff20*fun_u_derx + coeff21*fun_u_dery + coeff22*fun_u_derz + coeff23*shp_fun_u;
		  temp7 = coeff30*fun_u_derx + coeff31*fun_u_dery + coeff32*fun_u_derz + coeff33*shp_fun_u;

		#else

		  register int offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;

		  temp4 = el_data_in[offset2+0]*fun_u_derx + el_data_in[offset2+1]*fun_u_dery + el_data_in[offset2+2]*fun_u_derz + el_data_in[offset2+12]*shp_fun_u;
		  temp5 = el_data_in[offset2+3]*fun_u_derx + el_data_in[offset2+4]*fun_u_dery + el_data_in[offset2+5]*fun_u_derz + el_data_in[offset2+13]*shp_fun_u;
		  temp6 = el_data_in[offset2+6]*fun_u_derx + el_data_in[offset2+7]*fun_u_dery + el_data_in[offset2+8]*fun_u_derz + el_data_in[offset2+14]*shp_fun_u;
		  temp7 = el_data_in[offset2+9]*fun_u_derx + el_data_in[offset2+10]*fun_u_dery + el_data_in[offset2+11]*fun_u_derz + el_data_in[offset2+15]*shp_fun_u;

		#endif

	#elif defined(HEAT)

#endif

//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

#ifdef LOAD_VEC_COMP

	#ifdef LOCAL_STIFF
		  load_vec[idof] += (
	#else
		 el_data_out[offset+one_el_stiff_mat_size+idof] += (
	#endif


	  #ifdef LAPLACE

		#ifdef REGISTERS

				     coeff03 * shp_fun_u

		#else
				     el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shp_fun_u

		#endif


	  #elif defined(TEST_SCALAR)

	    #ifdef REGISTERS

			 coeff04 * fun_u_derx +
			 coeff14 * fun_u_dery +
			 coeff24 * fun_u_derz +
			 coeff34 * shp_fun_u

		#else

		   el_data_in[offset2+16] * fun_u_derx +
		   el_data_in[offset2+17] * fun_u_dery +
		   el_data_in[offset2+18] * fun_u_derz +
		   el_data_in[offset2+19] * shp_fun_u

	    #endif // REGISTERS

	  #elif defined(HEAT)

	  #endif

			     ) * vol;

#endif // end if computing RHS vector

//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
//-------------------------------------------------------------

	  } // the end of using registers for u (shp_fun_u, fun_u_der.)

//-------------------------------------------------------------
// ************************* second loop over shape functions ****************************//
#pragma ivdep
#pragma vector aligned
        for(jdof = 0; jdof < NSHAP; jdof++){

        	//-------------------------------------------------------------
        	//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF JDOF SHAPE FUNCTION ******//

        	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

        		  //SCALAR shp_fun_v = shape_fun_host[igauss*4*num_shap+4*jdof];
        		  SCALAR shp_fun_v = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+jdof];
        		  SCALAR fun_v_derx = tab_fun_u_derx[jdof];
        		  SCALAR fun_v_dery = tab_fun_u_dery[jdof];
        		  SCALAR fun_v_derz = tab_fun_u_derz[jdof];

        	#else // if not COMPUTE_ALL_SHAPE_FUN_DER

        		// read proper values of shape functions and their derivatives
//        		SCALAR shp_fun_v = shape_fun_host[igauss*4*num_shap+4*jdof];
//        		temp1 = shape_fun_host[igauss*4*num_shap+4*jdof+1];
//        		temp2 = shape_fun_host[igauss*4*num_shap+4*jdof+2];
//        		temp3 = shape_fun_host[igauss*4*num_shap+4*jdof+3];

				SCALAR shp_fun_v = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+jdof];
				temp1 = shape_fun_host[igauss*NSHAP+jdof];
				temp2 = shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+jdof];
				temp3 = shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+jdof];


        		// compute derivatives wrt global coordinates
        		// 15 operations
        		SCALAR fun_v_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
        		SCALAR fun_v_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
        		SCALAR fun_v_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

        	#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

        	//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
        	//-------------------------------------------------------------

        		//-------------------------------------------------------------
        		//********* ACTUAL FINAL CALCULATIONS FOR SM ENTRY  *********//



        		#ifdef LOCAL_STIFF
        		  	  stiff_mat[idof*NDOFS+jdof] += (
        		#else
        		  	__assume_aligned(el_data_out,ALIGN);
        			  el_data_out[offset+idof*NDOFS+jdof] += (
        		#endif


        		#ifdef LAPLACE

        		      	    temp4 * fun_v_derx +
        		       	    temp5 * fun_v_dery +
        		       	    temp6 * fun_v_derz

        		#elif defined(TEST_SCALAR)

        		      	    temp4 * fun_v_derx +
        		       	    temp5 * fun_v_dery +
        		       	    temp6 * fun_v_derz +
        		       	    temp7 * shp_fun_v

        		#elif defined(HEAT)

        		      	    temp4 * fun_v_derx +
        		       	    temp5 * fun_v_dery +
        		       	    temp6 * fun_v_derz +
        		       	    temp7 * shp_fun_v

        		#endif

        							    ) * vol;

        		//*** THE END OF: ACTUAL FINAL CALCULATIONS FOR SM ENTRY  ***//
        		//-------------------------------------------------------------

        			 //printf("stiff_mat[%d]=%lf\n",idof*num_dofs+jdof,el_data_out[offset+idof*num_dofs+jdof]);

        			}//jdof

        		//******* THE END OF: first loop over shape functions *******//
        		//-------------------------------------------------------------

        		      }//idof

        		//******* THE END OF: second loop over shape functions *******//
        		//-------------------------------------------------------------

//      if(ielem==0)
//      {
//      	printf("Stiff_mat igauss:%d\n",igauss);
//      	for(i = 0; i < NDOFS*NDOFS; i++)
//      	{
//      		printf("%lf\t",stiff_mat[i]);
//      		if(!((i+1)%4))
//      				printf("\n");
//      	}
//      }

//      Stiff_mat igauss:0
//      0.000890        -0.000025       0.000024        -0.000906
//      -0.000016       0.001707        -0.000865       -0.000817
//      0.000026        -0.000867       0.000866        -0.000036
//      -0.000905       -0.000818       -0.000036       0.001783
//      Stiff_mat igauss:1
//      0.001779        -0.000042       0.000049        -0.001820
//      -0.000031       0.003412        -0.001726       -0.001641
//      0.000052        -0.001729       0.001732        -0.000077
//      -0.001811       -0.001650       -0.000077       0.003587
//      Stiff_mat igauss:2
//      0.002660        -0.000062       0.000068        -0.002715
//      -0.000042       0.005118        -0.002587       -0.002469
//      0.000073        -0.002591       0.002598        -0.000113
//      -0.002705       -0.002479       -0.000113       0.005370
//      Stiff_mat igauss:3
//      0.003550        -0.000079       0.000085        -0.003622
//      -0.000056       0.006823        -0.003444       -0.003296
//      0.000097        -0.003456       0.003455        -0.000139
//      -0.003610       -0.003308       -0.000139       0.007153

        		    }//gauss
        	//
        	//	// ******** THE END OF: loop over integration points ********//
        	//	//-------------------------------------------------------------
        	//

//        	#ifdef COMPUTE_ALL_SHAPE_FUN_DER
//        		//register SCALAR *workspace = (SCALAR *) malloc((geo_dat_size+nr_coeff)*sizeof(SCALAR));
//        		_mm_free(tab_fun_u_derx);
//        		_mm_free(tab_fun_u_dery);
//        		_mm_free(tab_fun_u_derz);
//        	#endif
//
//    if(ielem==0)
//    {
//    	printf("Stiff_mat:\n");
//    	for(i = 0; i < NDOFS*NDOFS; i++)
//    	{
//    		printf("%lf\t",el_data_out[offset+i]);
//    		if(!((i+1)%4))
//    				printf("\n");
//    	}
//    }


#ifdef LOCAL_STIFF

	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
    for(i = 0; i < NDOFS*NDOFS; i++) el_data_out[offset+i] = stiff_mat[i];
  #ifdef LOAD_VEC_COMP
    for(i = 0; i < NDOFS; i++) el_data_out[offset+one_el_stiff_mat_size+i] = load_vec[i];
  #endif

#endif


        		  } // the end of loop over elements

        		// ************* THE END OF: LOOP OVER ELEMENTS *************//
        		//-------------------------------------------------------------

}//end parallel region

}//end offload

#ifdef TIME_TEST
    double t_end_in = time_clock();
    printf("\nEXECUTION TIME: Numerical integration inside function %lf\n",
	   t_end_in-t_begin_in);
    total_time += t_end_in-t_begin_in;

#ifdef TUNING
	if(line_count==0)
		fprintf(headuf,"Num int inside,");
	fprintf(resuf,"%lf,",t_end_in-t_begin_in);
#endif


#endif

		return 1;

}
