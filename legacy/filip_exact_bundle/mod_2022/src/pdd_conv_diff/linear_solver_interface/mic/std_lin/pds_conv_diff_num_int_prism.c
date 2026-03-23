#include "mic.h"

#define LOCAL_STIFF
//#define VECTORIZE

#define NGAUSS 6
#define NSHAP 6
#define NGEO 6
#define NDOFS 6

//#define THR 240

int pdr_num_int_el_QSS_prism(
		SCALAR *gauss_dat_host, // integration points data of elements having given p
		SCALAR *shape_fun_host, // shape functions on a reference element
		SCALAR *el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
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
	//printf("inside!!!\n");
	//printf("gauss_dat->%p, *gauss_dat->%p\n",gauss_dat_host,*gauss_dat_host);
	//printf("shape_fun_host->%p, *shape_fun_host->%p\n",shape_fun_host,*shape_fun_host);
	//printf("el_data_in->%p, *el_data_in->%p\n",el_data_in,*el_data_in);
	//printf("el_data_out->%p, *el_data_out->%p\n",el_data_out,*el_data_out);

//	printf("nr_coeff=%d, geo_dat_size=%d\n",nr_coeff,geo_dat_size);

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
//const int chunk_size=nr_elem_mic/24;

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

#pragma omp parallel default(none) private(offset,ielem) firstprivate(nr_elem_mic,size_el_out,size_el_in,size_shp,geo_dat_size,nr_coeff,one_el_stiff_mat_size,one_el_load_vec_size) shared(gauss_dat_host,el_data_in,el_data_out,shape_fun_host)
{
	//-------------------------------------------------------------
	//******************* loop over elements processed by a thread *********************
//__attribute__((concurrency_safe(profitable)))
#pragma ivdep
#pragma vector aligned
	#pragma omp for schedule(guided) nowait
    for(ielem = 0; ielem < nr_elem_mic; ielem++){

					
		#ifdef COMPUTE_ALL_SHAPE_FUN_DER
			//register SCALAR *workspace = (SCALAR *) malloc((geo_dat_size+nr_coeff)*sizeof(SCALAR));

			#ifdef VECTORIZE
    			//SCALAR *tab_fun_u_der = (SCALAR *) _mm_malloc(3*NSHAP*sizeof(SCALAR),ALIGN);
    			__declspec(align(ALIGN)) SCALAR tab_fun_u_der[3*NSHAP];

			#else
				//SCALAR *tab_fun_u_derx = (SCALAR *) _mm_malloc(NSHAP*sizeof(SCALAR),ALIGN);
				//SCALAR *tab_fun_u_dery = (SCALAR *) _mm_malloc(NSHAP*sizeof(SCALAR),ALIGN);
				//SCALAR *tab_fun_u_derz = (SCALAR *) _mm_malloc(NSHAP*sizeof(SCALAR),ALIGN);

    			__declspec(align(ALIGN)) SCALAR tab_fun_u_derx[NSHAP];
    			__declspec(align(ALIGN)) SCALAR tab_fun_u_dery[NSHAP];
    			__declspec(align(ALIGN)) SCALAR tab_fun_u_derz[NSHAP];

			#endif
		#endif

		#ifdef LOCAL_STIFF
			__declspec(align(ALIGN)) SCALAR stiff_mat[NDOFS*NDOFS];
			__declspec(align(ALIGN)) SCALAR load_vec[NDOFS];
		#endif


//printf("Thr:%d,el:%d\t",omp_get_thread_num(),ielem);

	    int i;

	//-------------------------------------------------------------
	// ******************* READING INPUT DATA *********************


	    //printf("nr_coeff=%d\n",nr_coeff);

		#ifdef REGISTERS

	    	#ifdef TEST_SCALAR

	    		offset=nr_elem_mic*geo_dat_size+ielem*nr_coeff;

#ifdef VECTORIZE
	    		SCALAR *coeff = (SCALAR *) _mm_malloc(nr_coeff*sizeof(SCALAR),ALIGN);
				#pragma vector aligned
	    		//#pragma simd
	    		for(i=0;i<nr_coeff;i++)
	    			coeff[i]=el_data_in[offset+i];
#else

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

	    #endif

	// ******* THE END OF: READING INPUT DATA *********************
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//******************** INITIALIZING SM AND LV ******************//

	#ifdef LOCAL_STIFF
#pragma ivdep
#pragma vector aligned
//#pragma simd
		    for(i = 0; i < NDOFS*NDOFS; i++) stiff_mat[i] = zero;

			  #ifdef LOAD_VEC_COMP
#pragma ivdep
#pragma vector aligned
//#pragma simd
			    for(i = 0; i < NDOFS; i++) load_vec[i] = zero;
			  #endif
	#endif
//	    offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
//#pragma ivdep
//#pragma vector aligned
//	    for(i = 0; i < one_el_stiff_mat_size; i++) el_data_out[offset+i] = zero;
//
//	  #ifdef LOAD_VEC_COMP
//#pragma ivdep
//#pragma vector aligned
//	    for(i = 0; i < one_el_load_vec_size ; i++) el_data_out[offset+one_el_stiff_mat_size+i] = zero;
//	  #endif
//	#endif

	//******************** END OF: INITIALIZING SM AND LV ******************//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//************************* LOOP OVER INTEGRATION POINTS ************************//

	    // in a loop over gauss points
	    int igauss;
	    int idof, jdof;
#pragma vector aligned
#pragma ivdep
	    for(igauss = 0; igauss < NGAUSS; igauss++){

	      // integration data read from cached constant or shared  memory
#ifdef VECTORIZE
	  __declspec(align(ALIGN)) SCALAR aux[3];

		#pragma vector aligned
		////#pragma simd
	   	   for(i=0;i<3;i++)
	   	   {
	  		   __assume_aligned(gauss_dat_host,ALIGN); 
			   aux[i]=gauss_dat_host[4*igauss+i];
	   	   }


#else
	      SCALAR daux = gauss_dat_host[4*igauss];
	      SCALAR faux = gauss_dat_host[4*igauss+1];
	      SCALAR eaux = gauss_dat_host[4*igauss+2];
	      //SCALAR vol = gauss_dat_host[4*igauss+3]; // vol = weight

#endif
	      SCALAR vol = weight_linear_prism; // vol = weight CONSTANT FOR LINEAR PRISM!!!
	//-------------------------------------------------------------
	//************************* JACOBIAN TERMS CALCULATIONS *************************//

	      // when geometrical shape functions are not necessary
	      // (only derivatives are used for Jacobian calculations)

#ifdef VECTORIZE
	   __declspec(align(ALIGN)) SCALAR temp[9];
		#pragma vector aligned
	   //#pragma simd
	   for(i=0;i<9;i++)
	   {
		   temp[i]=zero;
	   }
#else
	      SCALAR temp1 = zero;
	      SCALAR temp2 = zero;
	      SCALAR temp3 = zero;
	      SCALAR temp4 = zero;
	      SCALAR temp5 = zero;
	      SCALAR temp6 = zero;
	      SCALAR temp7 = zero;
	      SCALAR temp8 = zero;
	      SCALAR temp9 = zero;
#endif

#ifdef COMPUTE_ALL_SHAPE_FUN_DER
	      { // block to indicate the scope of jac_x registers
#endif


#ifdef VECTORIZE
	   	   __declspec(align(ALIGN)) SCALAR jac[9];
	   	   //#pragma simd
	   	   for(i=0;i<9;i++)
	   	   {
	   		   jac[i]=zero;
	   	   }
#else
	      SCALAR jac_0 = zero;
	      SCALAR jac_1 = zero;
	      SCALAR jac_2 = zero;
	      SCALAR jac_3 = zero;
	      SCALAR jac_4 = zero;
	      SCALAR jac_5 = zero;
	      SCALAR jac_6 = zero;
	      SCALAR jac_7 = zero;
	      SCALAR jac_8 = zero;
#endif

	      // derivatives of geometrical shape functions
	      { // block to indicate the scope of jac_data

	        // derivatives of geometrical shape functions are stored in jac_data
	    	__declspec(align(ALIGN)) SCALAR jac_data[3*NGEO];


#ifdef VECTORIZE
		jac_data[0] = -(one-aux[2])*half;
		jac_data[1] =  (one-aux[2])*half;
		jac_data[2] =  zero;
		jac_data[3] = -(one+aux[2])*half;
		jac_data[4] =  (one+aux[2])*half;
		jac_data[5] =  zero;
		jac_data[6] = -(one-aux[2])*half;
		jac_data[7] =  zero;
		jac_data[8] =  (one-aux[2])*half;
		jac_data[9] = -(one+aux[2])*half;
		jac_data[10] =  zero;
		jac_data[11] =  (one+aux[2])*half;
		jac_data[12] = -(one-aux[0]-aux[1])*half;
		jac_data[13] = -aux[0]*half;
		jac_data[14] = -aux[1]*half;
		jac_data[15] =  (one-aux[0]-aux[1])*half;
		jac_data[16] =  aux[0]*half;
		jac_data[17] =  aux[1]*half;
#else
	    jac_data[0] = -(one-eaux)*half;
		jac_data[1] =  (one-eaux)*half;
		jac_data[2] =  zero;
		jac_data[3] = -(one+eaux)*half;
		jac_data[4] =  (one+eaux)*half;
		jac_data[5] =  zero;
		jac_data[6] = -(one-eaux)*half;
		jac_data[7] =  zero;
		jac_data[8] =  (one-eaux)*half;
		jac_data[9] = -(one+eaux)*half;
		jac_data[10] =  zero;
		jac_data[11] =  (one+eaux)*half;
		jac_data[12] = -(one-daux-faux)*half;
		jac_data[13] = -daux*half;
		jac_data[14] = -faux*half;
		jac_data[15] =  (one-daux-faux)*half;
		jac_data[16] =  daux*half;
		jac_data[17] =  faux*half;
#endif

		/* Jacobian matrix J */

		offset=ielem*geo_dat_size;
		//#pragma simd
		#pragma ivdep
		#pragma vector aligned
		for(i=0;i<NGEO;i++){

#ifdef VECTORIZE
		  jac[1] = jac_data[i];
		  jac[2] = jac_data[NGEO+i];
		  jac[3] = jac_data[2*NGEO+i];

		  jac[4] = el_data_in[offset+3*i];  //node coor
		  jac[5] = el_data_in[offset+3*i+1];
		  jac[6] = el_data_in[offset+3*i+2];

		  //printf("el_data_in_geo_inside[%d]=%lf\n",offset+3*i,el_data_in[offset+3*i]);  //ok

		  temp[0] += jac[4] * jac[1];
		  temp[1] += jac[4] * jac[2];
		  temp[2] += jac[4] * jac[3];
		  temp[3] += jac[5] * jac[1];
		  temp[4] += jac[5] * jac[2];
		  temp[5] += jac[5] * jac[3];
		  temp[6] += jac[6] * jac[1];
		  temp[7] += jac[6] * jac[2];
		  temp[8] += jac[6] * jac[3];
#else

		  jac_1 = jac_data[i];
		  jac_2 = jac_data[NGEO+i];
		  jac_3 = jac_data[2*NGEO+i];

		  jac_4 = el_data_in[offset+3*i];  //node coor
		  jac_5 = el_data_in[offset+3*i+1];
		  jac_6 = el_data_in[offset+3*i+2];

		  //printf("el_data_in_geo_inside[%d]=%lf\n",offset+3*i,el_data_in[offset+3*i]);  //ok

		  temp1 += jac_4 * jac_1;
		  temp2 += jac_4 * jac_2;
		  temp3 += jac_4 * jac_3;
		  temp4 += jac_5 * jac_1;
		  temp5 += jac_5 * jac_2;
		  temp6 += jac_5 * jac_3;
		  temp7 += jac_6 * jac_1;
		  temp8 += jac_6 * jac_2;
		  temp9 += jac_6 * jac_3;
#endif
		}

	      } // the end of scope for jac_data
#ifdef VECTORIZE

	      jac[0] = (temp[4]*temp[8] - temp[7]*temp[5]);
	      jac[1] = (temp[7]*temp[2] - temp[1]*temp[8]);
	      jac[2] = (temp[1]*temp[5] - temp[2]*temp[4]);

	      SCALAR daux = temp[0]*jac[0] + temp[3]*jac[1] + temp[6]*jac[2];

	      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
	      vol *= daux; // vol = weight * det J

	      SCALAR faux = one/daux;

	      jac[0] *= faux;
	      jac[1] *= faux;
	      jac[2] *= faux;

	      jac[3] = (temp[5]*temp[6] - temp[3]*temp[8])*faux;
	      jac[4] = (temp[0]*temp[8] - temp[6]*temp[2])*faux;
	      jac[5] = (temp[2]*temp[3] - temp[0]*temp[5])*faux;

	      jac[6] = (temp[3]*temp[7] - temp[4]*temp[6])*faux;
	      jac[7] = (temp[1]*temp[6] - temp[0]*temp[7])*faux;
	      jac[8] = (temp[0]*temp[4] - temp[1]*temp[3])*faux;

//	      if(ielem==0)
//	      for(i=0;i<9;i++)
//	    	  printf("jac[%d]=%lf\n",i,jac[i]);

#else
	      jac_0 = (temp5*temp9 - temp8*temp6);
	      jac_1 = (temp8*temp3 - temp2*temp9);
	      jac_2 = (temp2*temp6 - temp3*temp5);

	      daux = temp1*jac_0 + temp4*jac_1 + temp7*jac_2;

	      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
	      vol *= daux; // vol = weight * det J

	      faux = one/daux;

	      jac_0 *= faux;
	      jac_1 *= faux;
	      jac_2 *= faux;

	      jac_3 = (temp6*temp7 - temp4*temp9)*faux;
	      jac_4 = (temp1*temp9 - temp7*temp3)*faux;
	      jac_5 = (temp3*temp4 - temp1*temp6)*faux;

	      jac_6 = (temp4*temp8 - temp5*temp7)*faux;
	      jac_7 = (temp2*temp7 - temp1*temp8)*faux;
	      jac_8 = (temp1*temp5 - temp2*temp4)*faux;


#endif
	//************* THE END OF: JACOBIAN TERMS CALCULATIONS *************************//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//

	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

	 //************ loop for computing ALL shape function values at integration point **********//
#pragma vector aligned
#pragma ivdep
////#pragma simd
	      for(idof = 0; idof < NSHAP; idof++){

#ifdef VECTORIZE
		// read proper values of shape functions and their derivatives
		//#pragma simd
	    for(i=0;i<3;i++)
	    {
	    	temp[i]=shape_fun_host[i*NSHAP*NGAUSS+igauss*NSHAP+idof];
	    	//if(ielem==0)
	    	//printf("igauss=%d,idof=%d,temp[%d]=%lf\n",igauss,idof,i,temp[i]);
	    }

		//#pragma simd
	    for(i=0;i<3;i++)
	    {
	    	tab_fun_u_der[3*idof+i] = temp[0]*jac[i]+temp[1]*jac[i+3]+temp[2]*jac[i+6];
	    	//if(ielem==0)
	    	//printf("igauss=%d,idof=%d,tab_fun_u_der[%d]=%lf\n",igauss,idof,3*idof+i,tab_fun_u_der[3*idof+i]);
	    }

//		tab_fun_u_der[3*idof] = temp[0]*jac[0]+temp[1]*jac[3]+temp[2]*jac[6];
//		tab_fun_u_der[3*idof+1] = temp[0]*jac[1]+temp[1]*jac[4]+temp[2]*jac[7];
//		tab_fun_u_der[3*idof+2] = temp[0]*jac[2]+temp[1]*jac[5]+temp[2]*jac[8];

#else

		// read proper values of shape functions and their derivatives
		temp1 = shape_fun_host[igauss*NSHAP+idof];
		temp2 = shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof];
		temp3 = shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof];
//		if(ielem==0)
//			printf("igauss=%d,idof=%d,temp1,2,3=%lf,%lf,%lf\n",igauss,idof,temp1,temp2,temp3);

		tab_fun_u_derx[idof] = temp1*jac_0+temp2*jac_3+temp3*jac_6;
		tab_fun_u_dery[idof] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
		tab_fun_u_derz[idof] = temp1*jac_2+temp2*jac_5+temp3*jac_8;
		//if(ielem==0)
		//printf("igauss=%d,idof=%d,tab_fun_u_derx,y,z=%lf,%lf,%lf\n",igauss,idof,tab_fun_u_derx[idof],tab_fun_u_dery[idof],tab_fun_u_derz[idof]);

#endif
	      } // end loop over shape functions for which global derivatives were computed

	      } // the end of block to indicate the scope of jac_x registers

	#endif // COMPUTE_ALL_SHAPE_FUN_DER

	//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//***** SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS *****//

	#ifdef HEAT

	      // for non-constant, non-linear coefficients a place for call to problem dependent
	      // function calculating actual PDE coefficients based on data in coeff
	      // workspace or registers and  storing data back in workspace or in registers

	#endif


	#ifdef REGISTERS

	  #ifdef LAPLACE

	      // offset for reading data
	      offset=nr_elem_mic*geo_dat_size+ielem*nr_coeff;

#ifdef MIC
	      register SCALAR __attribute__((target(mic))) coeff03 = el_data_in[offset+igauss];
#else
	      register SCALAR coeff03 = el_data_in[offset+igauss];
#endif

	  #endif // end if LAPLACE

	#endif

	//*** THE END OF: SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS ***//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//********************* first loop over shape functions ***********************//


	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);  //loop invariant code motion
#pragma vector aligned
#pragma ivdep
////#pragma simd
	      for(idof = 0; idof < NSHAP; idof++){
//printf("idof=%d\n",idof);
		{ // beginning of using registers for u  (shp_fun_u, fun_u_der.)

	//-------------------------------------------------------------
	//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ******//

	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE
			  // read proper values of shape functions and their derivatives
			__declspec(align(ALIGN)) SCALAR fun_u_der[3];
			  SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];

//			#pragma vector aligned
//			#pragma ivdep
			  //#pragma simd
			  for(i=0;i<3;i++)
				  fun_u_der[i]=tab_fun_u_der[3*idof+i];
#else
	  		  // read proper values of shape functions and their derivatives
	          SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];
	          SCALAR fun_u_derx = tab_fun_u_derx[idof];
	          SCALAR fun_u_dery = tab_fun_u_dery[idof];
	          SCALAR fun_u_derz = tab_fun_u_derz[idof];
#endif//vect
	#else // if not COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE

			// read proper values of shape functions and their derivatives
	        __declspec(align(ALIGN)) SCALAR fun_u_der[3];
	        SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];

#pragma vector aligned
#pragma ivdep
			////#pragma simd
			for(i=0;i<3;i++)
				temp[i]=shape_fun_host[i*NSHAP*NGAUSS+igauss*NSHAP+idof];
#pragma vector aligned
#pragma ivdep
			////#pragma simd
			for(i=0;i<3;i++)
				fun_u_der[i] = temp[0]*jac[i]+temp[1]*jac[i+3]+temp[2]*jac[i+6];

#else
	          // read proper values of shape functions and their derivatives
			  SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];
			  temp1 = shape_fun_host[igauss*NSHAP+idof];
			  temp2 = shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof];
			  temp3 = shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof];


			  // compute derivatives wrt global coordinates
			  // 15 operations
			  SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
			  SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
			  SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
#endif//vect

	#endif // COMPUTE_ALL_SHAPE_FUN_DER

	//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//

			  //TODO sprawdzic czy kolejnosc indeksow ma znacznie zaminic uzycie temp123 z temp4567 od tego miejsca
#ifdef VECTORIZE

	#ifdef LAPLACE

		//#pragma simd
			  for(i=0;i<3;i++)
				  temp[3+i]=fun_u_der[i];

	#elif defined(TEST_SCALAR)

		#ifdef REGISTERS

	  	  temp[3] = coeff[0]*fun_u_der[0] + coeff[1]*fun_u_der[1] + coeff[2]*fun_u_der[2] + coeff[12]*shp_fun_u;
		  temp[4] = coeff[3]*fun_u_der[0] + coeff[4]*fun_u_der[1] + coeff[5]*fun_u_der[2] + coeff[13]*shp_fun_u;
		  temp[5] = coeff[6]*fun_u_der[0] + coeff[7]*fun_u_der[1] + coeff[8]*fun_u_der[2] + coeff[14]*shp_fun_u;
		  temp[6] = coeff[9]*fun_u_der[0] + coeff[10]*fun_u_der[1] + coeff[11]*fun_u_der[2] + coeff[15]*shp_fun_u;

		#else

		  register int offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;

		  temp[3] = el_data_in[offset2+0]*fun_u_der[0] + el_data_in[offset2+1]*fun_u_der[1] + el_data_in[offset2+2]*fun_u_der[2] + el_data_in[offset2+12]*shp_fun_u;
		  temp[4] = el_data_in[offset2+3]*fun_u_der[0] + el_data_in[offset2+4]*fun_u_der[1] + el_data_in[offset2+5]*fun_u_der[2] + el_data_in[offset2+13]*shp_fun_u;
		  temp[5] = el_data_in[offset2+6]*fun_u_der[0] + el_data_in[offset2+7]*fun_u_der[1] + el_data_in[offset2+8]*fun_u_der[2] + el_data_in[offset2+14]*shp_fun_u;
		  temp[6] = el_data_in[offset2+9]*fun_u_der[0] + el_data_in[offset2+10]*fun_u_der[1] + el_data_in[offset2+11]*fun_u_der[2] + el_data_in[offset2+15]*shp_fun_u;

		#endif

	#elif defined(HEAT)

	#endif

#else

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

#endif//vectorize

	//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

//		  printf("stiff_mat_out_rhs[%d]=%lf\n",offset+NDOFS*NDOFS+idof,el_data_out[offset+NDOFS*NDOFS+idof]);


	#ifdef LOAD_VEC_COMP
//#pragma vector nontemporal
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

#ifdef VECTORIZE

		#ifdef REGISTERS

			 coeff[16] * fun_u_der[0] +
			 coeff[17] * fun_u_der[1] +
			 coeff[18] * fun_u_der[2] +
			 coeff[19] * shp_fun_u

		#else

		   el_data_in[offset2+16] * fun_u_der[0] +
		   el_data_in[offset2+17] * fun_u_der[1] +
		   el_data_in[offset2+18] * fun_u_der[2] +
		   el_data_in[offset2+19] * shp_fun_u

		#endif // REGISTERS

#else

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

#endif //vectorize

	  #elif defined(HEAT)

	  #endif

				     ) * vol;

		 //printf("stiff_mat_out_rhs[%d]=%lf\n",offset+NDOFS*NDOFS+idof,el_data_out[offset+NDOFS*NDOFS+idof]);

		 //printf("el_data_in[%d]=%lf\n",nr_elem_mic*geo_dat_size+ielem*nr_coeff+idof,el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+idof]); //ok
		 //printf("shp_fun_u=%lf\n",shp_fun_u);

	#endif // end if computing RHS vector

	//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
	//-------------------------------------------------------------

		  } // the end of using registers for u (shp_fun_u, fun_u_der.)
//printf("TU!\n");
	//-------------------------------------------------------------
	// ************************* second loop over shape functions ****************************//
#pragma ivdep
#pragma vector aligned
//#pragma simd
	        for(jdof = 0; jdof < NSHAP; jdof++){
	//-------------------------------------------------------------
	//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF JDOF SHAPE FUNCTION ******//

	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE
			  // read proper values of shape functions and their derivatives
			__declspec(align(ALIGN)) SCALAR fun_v_der[3];
			  SCALAR shp_fun_v = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+jdof];
#pragma vector aligned
#pragma ivdep
			  ////#pragma simd
			  for(i=0;i<3;i++)
				  fun_v_der[i]=tab_fun_u_der[3*jdof+i];
#else
		//__assume_aligned(tab_fun_u_derx,ALIGN);
        //        __assume_aligned(tab_fun_u_dery,ALIGN);
        //        __assume_aligned(tab_fun_u_derz,ALIGN);

	  		  // read proper values of shape functions and their derivatives
			  SCALAR shp_fun_v = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+jdof];
			  SCALAR fun_v_derx = tab_fun_u_derx[jdof];
			  SCALAR fun_v_dery = tab_fun_u_dery[jdof];
			  SCALAR fun_v_derz = tab_fun_u_derz[jdof];
#endif//vect

	#else // if not COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE
//to lepiej zrobic -wszystko do wektora!!
			// read proper values of shape functions and their derivatives
	        __declspec(align(ALIGN)) SCALAR fun_v_der[3];
	        SCALAR shp_fun_v = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+jdof];

			////#pragma simd
			for(i=0;i<3;i++)
				temp[i]=shape_fun_host[i*NSHAP*NGAUSS+igauss*NSHAP+jdof];
#pragma vector aligned
#pragma ivdep
			//#pragma simd
			for(i=0;i<3;i++)
				fun_v_der[i] = temp[0]*jac[i]+temp[1]*jac[i+3]+temp[2]*jac[i+6];

#else

		// read proper values of shape functions and their derivatives
		SCALAR shp_fun_v = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+jdof];
		temp1 = shape_fun_host[igauss*NSHAP+jdof];
		temp2 = shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+jdof];
		temp3 = shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+jdof];

		// compute derivatives wrt global coordinates
		// 15 operations
		SCALAR fun_v_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
		SCALAR fun_v_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
		SCALAR fun_v_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;

#endif//vectorize

	#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

	//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//********* ACTUAL FINAL CALCULATIONS FOR SM ENTRY  *********//
	//
	//
	//
	
		__assume_aligned(el_data_out,ALIGN);

		#ifdef LOCAL_STIFF
		  	  stiff_mat[idof*NDOFS+jdof] += (
		#else
			  el_data_out[offset+idof*NDOFS+jdof] += (
		#endif

	#ifdef LAPLACE

#ifdef VECTORIZE

				 temp[3] * fun_v_der[0] +
				 temp[4] * fun_v_der[1] +
				 temp[5] * fun_v_der[2]


#else

	      	    temp4 * fun_v_derx +
	       	    temp5 * fun_v_dery +
	       	    temp6 * fun_v_derz

#endif // vectorize

	#elif defined(TEST_SCALAR)

#ifdef VECTORIZE
	       	    temp[3] * fun_v_der[0] +
				temp[4] * fun_v_der[1] +
				temp[5] * fun_v_der[2] +
				temp[6] * shp_fun_v
#else
				temp4 * fun_v_derx +
	       	    temp5 * fun_v_dery +
	       	    temp6 * fun_v_derz +
	       	    temp7 * shp_fun_v
#endif//vectorize

	#elif defined(HEAT)

	      	    temp4 * fun_v_derx +
	       	    temp5 * fun_v_dery +
	       	    temp6 * fun_v_derz +
	       	    temp7 * shp_fun_v

	#endif

						    ) * vol;

	//*** THE END OF: ACTUAL FINAL CALCULATIONS FOR SM ENTRY  ***//
	//-------------------------------------------------------------

		 //printf("el_data_out[%d]=%lf\n",offset+idof*NDOFS+jdof,el_data_out[offset+idof*NDOFS+jdof]);

		}//jdof

	//******* THE END OF: first loop over shape functions *******//
	//-------------------------------------------------------------
//	printf("TU2!\n");

	      }//idof
//printf("TU3!\n");
	//******* THE END OF: second loop over shape functions *******//
	//-------------------------------------------------------------

	    }//gauss
//
//	// ******** THE END OF: loop over integration points ********//
//	//-------------------------------------------------------------
//
#ifdef LOCAL_STIFF

	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
    for(i = 0; i < NDOFS*NDOFS; i++) el_data_out[offset+i] = stiff_mat[i];
  #ifdef LOAD_VEC_COMP
    for(i = 0; i < NDOFS; i++) el_data_out[offset+one_el_stiff_mat_size+i] = load_vec[i];
  #endif

#endif




//#ifdef COMPUTE_ALL_SHAPE_FUN_DER
//	//register SCALAR *workspace = (SCALAR *) malloc((geo_dat_size+nr_coeff)*sizeof(SCALAR));
//	#ifdef VECTORIZE
//		_mm_free(tab_fun_u_der);
//	#else
//		_mm_free(tab_fun_u_derx);
//		_mm_free(tab_fun_u_dery);
//		_mm_free(tab_fun_u_derz);
//	#endif
//#endif

	  } // the end of loop over elements

	// ************* THE END OF: LOOP OVER ELEMENTS *************//
	//-------------------------------------------------------------

}//parallel region

}//offload

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
