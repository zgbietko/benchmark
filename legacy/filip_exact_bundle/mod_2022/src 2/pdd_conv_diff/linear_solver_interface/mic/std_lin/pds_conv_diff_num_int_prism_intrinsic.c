#include "mic.h"
#include "immintrin.h"
#include<unistd.h>

#define LOCAL_STIFF

//for Haswell
//#define FMA

#define NGAUSS 6
#define NSHAP 6
#define NGEO 8
#define NDOFS 6
#define STRIDE 8

int pdr_num_int_el_QSS_prism(
		SCALAR * restrict gauss_dat_host, // integration points data of elements having given p
		SCALAR * restrict shape_fun_host, // shape functions on a reference element
		SCALAR * restrict el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
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

	//const int NDOFS=NSHAP*nreq;

	int ielem;
#ifdef MIC
	register int __attribute__((target(mic))) offset;
#else
	register int offset;
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
//#pragma loop_count (97800)
	#pragma omp for schedule(guided) nowait
    for(ielem = 0; ielem < nr_elem_mic; ielem++){

//   		SCALAR tab_fun_u_derx[STRIDE] __attribute__((aligned(ALIGN)));
//    		SCALAR tab_fun_u_dery[STRIDE] __attribute__((aligned(ALIGN)));
//    		SCALAR tab_fun_u_derz[STRIDE] __attribute__((aligned(ALIGN)));
    	register __m256d tab_fun_u_derx1;
    	register __m128d tab_fun_u_derx2;
    	register __m256d tab_fun_u_dery1;
		register __m128d tab_fun_u_dery2;
		register __m256d tab_fun_u_derz1;
		register __m128d tab_fun_u_derz2;

		#ifdef LOCAL_STIFF
			//SCALAR stiff_mat[NDOFS*NDOFS]  __attribute__((aligned(ALIGN)));;
			//SCALAR load_vec[NDOFS]  __attribute__((aligned(ALIGN)));;

			register __m256d stiff_mat1a;
			register __m128d stiff_mat1b;
			register __m256d stiff_mat2a;
			register __m128d stiff_mat2b;
			register __m256d stiff_mat3a;
			register __m128d stiff_mat3b;
			register __m256d stiff_mat4a;
			register __m128d stiff_mat4b;
			register __m256d stiff_mat5a;
			register __m128d stiff_mat5b;
			register __m256d stiff_mat6a;
			register __m128d stiff_mat6b;

			register __m256d load_vec1;
			register __m128d load_vec2;
		#endif


//printf("Thr:%d,el:%d\t",omp_get_thread_num(),ielem);

	    int i;

	//-------------------------------------------------------------
	// ******************* READING INPUT DATA *********************

	    //printf("nr_coeff=%d\n",nr_coeff);

		#ifdef TEST_SCALAR

	    	   offset=nr_elem_mic*geo_dat_size+ielem*nr_coeff;
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

			   register __m256d coeff0;
			   coeff0 = _mm256_set_pd(coeff00,coeff10,coeff20,coeff30);
			   register __m256d coeff1;
			   coeff1 = _mm256_set_pd(coeff01,coeff11,coeff21,coeff31);
			   register __m256d coeff2;
			   coeff2 = _mm256_set_pd(coeff02,coeff12,coeff22,coeff32);
			   register __m256d coeff3;
			   coeff3 = _mm256_load_pd(&el_data_in[offset+12]);
			   //coeff3 = _mm256_set_pd(coeff03,coeff13,coeff23,coeff33);
			   register __m256d coeff4;
			   coeff3 = _mm256_load_pd(&el_data_in[offset+16]);

			#endif

	// ******* THE END OF: READING INPUT DATA *********************
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//******************** INITIALIZING SM AND LV ******************//

	#ifdef LOCAL_STIFF
//#pragma ivdep
//#pragma vector aligned
////#pragma loop_count (36)
//#pragma simd
//		    for(i = 0; i < NDOFS*NDOFS; i++) stiff_mat[i] = zero;
		    stiff_mat1a=_mm256_setzero_pd();
			stiff_mat1b=_mm_setzero_pd();
			stiff_mat2a=_mm256_setzero_pd();
			stiff_mat2b=_mm_setzero_pd();
			stiff_mat3a=_mm256_setzero_pd();
			stiff_mat3b=_mm_setzero_pd();
			stiff_mat4a=_mm256_setzero_pd();
			stiff_mat4b=_mm_setzero_pd();
			stiff_mat5a=_mm256_setzero_pd();
			stiff_mat5b=_mm_setzero_pd();
			stiff_mat6a=_mm256_setzero_pd();
			stiff_mat6b=_mm_setzero_pd();

			  #ifdef LOAD_VEC_COMP
//#pragma ivdep
//#pragma vector aligned
////#pragma loop_count (6)
//#pragma simd
//			    for(i = 0; i < STRIDE; i++) load_vec[i] = zero;
//			  #endif
		    load_vec1=_mm256_setzero_pd();
		    load_vec2=_mm_setzero_pd();
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
//#pragma loop_count (6)
	    for(igauss = 0; igauss < NGAUSS; igauss++){


	      // integration data read from cached constant or shared  memory

	      SCALAR daux = gauss_dat_host[igauss];
	      SCALAR faux = gauss_dat_host[NGAUSS+igauss];
	      SCALAR eaux = gauss_dat_host[2*NGAUSS+igauss];
	      //SCALAR vol = gauss_dat_host[4*igauss+3]; // vol = weight
	      SCALAR vol = weight_linear_prism; // vol = weight CONSTANT FOR LINEAR PRISM!!!
	//-------------------------------------------------------------
	//************************* JACOBIAN TERMS CALCULATIONS *************************//

	      // when geometrical shape functions are not necessary
	      // (only derivatives are used for Jacobian calculations)

//#ifdef COMPUTE_ALL_SHAPE_FUN_DER
	      //{ // block to indicate the scope of jac_x registers
//#endif

	      SCALAR jac_0 = zero;
	      SCALAR jac_1 = zero;
	      SCALAR jac_2 = zero;
	      SCALAR jac_3 = zero;
	      SCALAR jac_4 = zero;
	      SCALAR jac_5 = zero;
	      SCALAR jac_6 = zero;
	      SCALAR jac_7 = zero;
	      SCALAR jac_8 = zero;

	      // derivatives of geometrical shape functions
	      { // block to indicate the scope of jac_data

	        // derivatives of geometrical shape functions are stored in jac_data
			//#pragma vector aligned
	    	SCALAR jac_data[24]  __attribute__((aligned(ALIGN)));
			SCALAR temp1 = zero;
			SCALAR temp2 = zero;
			SCALAR temp3 = zero;
			SCALAR temp4 = zero;
			SCALAR temp5 = zero;
			SCALAR temp6 = zero;
			SCALAR temp7 = zero;
			SCALAR temp8 = zero;
			SCALAR temp9 = zero;


	    jac_data[0] = -(one-eaux)*half;
		jac_data[1] =  (one-eaux)*half;
		jac_data[2] =  zero;
		jac_data[3] = -(one+eaux)*half;
		jac_data[4] =  (one+eaux)*half;
		jac_data[5] =  zero;
		jac_data[8] = -(one-eaux)*half;
		jac_data[9] =  zero;
		jac_data[10] =  (one-eaux)*half;
		jac_data[11] = -(one+eaux)*half;
		jac_data[12] =  zero;
		jac_data[13] =  (one+eaux)*half;
		jac_data[16] = -(one-daux-faux)*half;
		jac_data[17] = -daux*half;
		jac_data[18] = -faux*half;
		jac_data[19] =  (one-daux-faux)*half;
		jac_data[20] =  daux*half;
		jac_data[21] =  faux*half;

		/* Jacobian matrix J */

		offset=ielem*geo_dat_size;

		#pragma ivdep
		#pragma vector aligned
		//#pragma loop_count(6)
		//#pragma simd
		for(i=0;i<6;i++){

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

		}

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

	      } // the end of scope for jac_data

//	      if(ielem==0)
//	      {
//	      	 printf("jac_0=%lf\n",jac_0);
//	      	printf("jac_1=%lf\n",jac_1);
//	      	printf("jac_2=%lf\n",jac_2);
//	      	printf("jac_3=%lf\n",jac_3);
//	      	printf("jac_4=%lf\n",jac_4);
//	      	printf("jac_5=%lf\n",jac_5);
//	      	printf("jac_6=%lf\n",jac_6);
//	      	printf("jac_7=%lf\n",jac_7);
//	      	printf("jac_8=%lf\n",jac_8);
//	      }

	//************* THE END OF: JACOBIAN TERMS CALCULATIONS *************************//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//

	      //SCALAR shp[STRIDE]  __attribute__((aligned(ALIGN)));;
//	      SCALAR shpx[STRIDE]  __attribute__((aligned(ALIGN)));;
//		  SCALAR shpy[STRIDE]  __attribute__((aligned(ALIGN)));;
//		  SCALAR shpz[STRIDE]  __attribute__((aligned(ALIGN)));;

	      register __m256d shp1;
	      register __m128d shp2;

	      register __m256d shpx1;
		  register __m128d shpx2;
		  register __m256d shpy1;
		  register __m128d shpy2;
		  register __m256d shpz1;
		  register __m128d shpz2;

		  shp1=_mm256_load_pd(&shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE]);
		  shp2=_mm_load_pd(&shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE+4]);

		  shpx1=_mm256_load_pd(&shape_fun_host[igauss*STRIDE]);
		  shpx2=_mm_load_pd(&shape_fun_host[igauss*STRIDE+4]);
		  shpy1=_mm256_load_pd(&shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE]);
		  shpy2=_mm_load_pd(&shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE+4]);
		  shpz1=_mm256_load_pd(&shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE]);
		  shpz2=_mm_load_pd(&shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE+4]);

		  register __m256d tmp1;
		  register __m256d tmp2;
		  register __m256d tmp3;

		  tmp1=_mm256_set1_pd(jac_0);
		  tmp2=_mm256_set1_pd(jac_1);
		  tmp3=_mm256_set1_pd(jac_2);

		  tab_fun_u_derx1 = _mm256_mul_pd(shpx1,tmp1);
		  tab_fun_u_derx2 = _mm_mul_pd(shpx2,_mm256_castpd256_pd128(tmp1));
		  tab_fun_u_dery1 = _mm256_mul_pd(shpx1,tmp2);
		  tab_fun_u_dery2 = _mm_mul_pd(shpx2,_mm256_castpd256_pd128(tmp2));
		  tab_fun_u_derz1 = _mm256_mul_pd(shpx1,tmp3);
		  tab_fun_u_derz2 = _mm_mul_pd(shpx2,_mm256_castpd256_pd128(tmp3));

		  tmp1=_mm256_set1_pd(jac_3);
		  tmp2=_mm256_set1_pd(jac_4);
		  tmp3=_mm256_set1_pd(jac_5);

#ifdef FMA
		  tab_fun_u_derx1=_mm256_fmadd_pd(shpy1,tmp1,tab_fun_u_derx1);
		  tab_fun_u_derx2=_mm_fmadd_pd(shpy2,_mm256_castpd256_pd128(tmp1),tab_fun_u_derx2);
#else
		  shpx1 = _mm256_mul_pd(shpy1,tmp1);
		  shpx2 = _mm_mul_pd(shpy2,_mm256_castpd256_pd128(tmp1));
		  tab_fun_u_derx1 = _mm256_add_pd(shpx1,tab_fun_u_derx1);
		  tab_fun_u_derx2 = _mm_add_pd(shpx2,tab_fun_u_derx2);
#endif

#ifdef FMA
		  tab_fun_u_dery1=_mm256_fmadd_pd(shpy1,tmp2,tab_fun_u_dery1);
		  tab_fun_u_dery2=_mm_fmadd_pd(shpy2,_mm256_castpd256_pd128(tmp2),tab_fun_u_dery2);
#else
		  shpx1 = _mm256_mul_pd(shpy1,tmp2);
		  shpx2 = _mm_mul_pd(shpy2,_mm256_castpd256_pd128(tmp2));
		  tab_fun_u_dery1 = _mm256_add_pd(shpx1,tab_fun_u_dery1);
		  tab_fun_u_dery2 = _mm_add_pd(shpx2,tab_fun_u_dery2);
#endif

#ifdef FMA
		  tab_fun_u_derz1=_mm256_fmadd_pd(shpy1,tmp3,tab_fun_u_derz1);
		  tab_fun_u_derz2=_mm_fmadd_pd(shpy2,_mm256_castpd256_pd128(tmp3),tab_fun_u_derz2);
#else
		  shpx1 = _mm256_mul_pd(shpy1,tmp3);
		  shpx2 = _mm_mul_pd(shpy2,_mm256_castpd256_pd128(tmp3));
		  tab_fun_u_derz1 = _mm256_add_pd(shpx1,tab_fun_u_derz1);
		  tab_fun_u_derz2 = _mm_add_pd(shpx2,tab_fun_u_derz2);
#endif
		  tmp1=_mm256_set1_pd(jac_6);
		  tmp2=_mm256_set1_pd(jac_7);
		  tmp3=_mm256_set1_pd(jac_8);
#ifdef FMA
		  tab_fun_u_derx1=_mm256_fmadd_pd(shpz1,tmp1,tab_fun_u_derx1);
		  tab_fun_u_derx2=_mm_fmadd_pd(shpz2,_mm256_castpd256_pd128(tmp1),tab_fun_u_derx2);
#else
		  shpx1 = _mm256_mul_pd(shpz1,tmp1);
		  shpx2 = _mm_mul_pd(shpz2,_mm256_castpd256_pd128(tmp1));
		  tab_fun_u_derx1 = _mm256_add_pd(shpx1,tab_fun_u_derx1);
		  tab_fun_u_derx2 = _mm_add_pd(shpx2,tab_fun_u_derx2);
#endif

#ifdef FMA
		  tab_fun_u_dery1=_mm256_fmadd_pd(shpz1,tmp2,tab_fun_u_dery1);
		  tab_fun_u_dery2=_mm_fmadd_pd(shpz2,_mm256_castpd256_pd128(tmp2),tab_fun_u_dery2);
#else
		  shpx1 = _mm256_mul_pd(shpz1,tmp2);
		  shpx2 = _mm_mul_pd(shpz2,_mm256_castpd256_pd128(tmp2));
		  tab_fun_u_dery1 = _mm256_add_pd(shpx1,tab_fun_u_dery1);
		  tab_fun_u_dery2 = _mm_add_pd(shpx2,tab_fun_u_dery2);
#endif

#ifdef FMA
		  tab_fun_u_derz1=_mm256_fmadd_pd(shpz1,tmp3,tab_fun_u_derz1);
		  tab_fun_u_derz2=_mm_fmadd_pd(shpz2,_mm256_castpd256_pd128(tmp3),tab_fun_u_derz2);
#else
		  shpx1 = _mm256_mul_pd(shpz1,tmp3);
		  shpx2 = _mm_mul_pd(shpz2,_mm256_castpd256_pd128(tmp3));
		  tab_fun_u_derz1 = _mm256_add_pd(shpx1,tab_fun_u_derz1);
		  tab_fun_u_derz2 = _mm_add_pd(shpx2,tab_fun_u_derz2);
#endif

//#pragma vector aligned
////#pragma ivdep
//		tab_fun_u_derx[0:STRIDE] = shpx[0:STRIDE]*jac_0+shpy[0:STRIDE]*jac_3+shpz[0:STRIDE]*jac_6;
//#pragma vector aligned
////#pragma ivdep
//		tab_fun_u_dery[0:STRIDE] = shpx[0:STRIDE]*jac_1+shpy[0:STRIDE]*jac_4+shpz[0:STRIDE]*jac_7;
//#pragma vector aligned
////#pragma ivdep
//		tab_fun_u_derz[0:STRIDE] = shpx[0:STRIDE]*jac_2+shpy[0:STRIDE]*jac_5+shpz[0:STRIDE]*jac_8;


	//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//***** SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS *****//

	#ifdef HEAT

	      // for non-constant, non-linear coefficients a place for call to problem dependent
	      // function calculating actual PDE coefficients based on data in coeff
	      // workspace or registers and  storing data back in workspace or in registers

	#endif

		  register __m128d tmp4;
		  register __m128d tmp5;

	#ifdef REGISTERS

	  #ifdef LAPLACE

	      // offset for reading data
	      offset=nr_elem_mic*geo_dat_size+ielem*nr_coeff;

#ifdef MIC
	      register SCALAR __attribute__((target(mic))) coeff03 = el_data_in[offset+igauss];
#else
	      register __m256d coeff03;
	      coeff03 = _mm256_set1_pd(el_data_in[offset+igauss]);

#endif

	  #endif // end if LAPLACE

	#endif

	#ifdef TEST_SCALAR

	      //SCALAR temp[4][NSHAP] __attribute__((aligned(ALIGN)));
	      register __m256d temp0a;
	      register __m128d temp0b;
		  register __m256d temp1a;
		  register __m128d temp1b;
		  register __m256d temp2a;
		  register __m128d temp2b;
		  register __m256d temp3a;
		  register __m128d temp3b;

		  register __m128d tmp6;
		  register __m128d tmp7;
		  //register __m256d tmpX;
	#endif


	//*** THE END OF: SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS ***//
	//-------------------------------------------------------------

	#ifdef TEST_SCALAR

		  tmp1 = _mm256_set1_pd(coeff00);
		  tmp2 = _mm256_set1_pd(coeff01);

		  tmp1 = _mm256_mul_pd(tmp1,tab_fun_u_derx1);
#ifdef FMA
		  temp0a=_mm256_fmadd_pd(tmp2,tab_fun_u_dery1,tmp1);
#else
		  tmp2 = _mm256_mul_pd(tmp2,tab_fun_u_dery1);
		  temp0a = _mm256_add_pd(tmp1,tmp2);
#endif
		  tmp1 = _mm256_set1_pd(coeff02);
#ifdef FMA
		  temp0a=_mm256_fmadd_pd(tmp1,tab_fun_u_derz1,temp0a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,tab_fun_u_derz1);
		  temp0a = _mm256_add_pd(temp0a,tmp2);
#endif
		  tmp1 = _mm256_set1_pd(coeff03);
#ifdef FMA
		  temp0a=_mm256_fmadd_pd(tmp1,shp1,temp0a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,shp1);
		  temp0a = _mm256_add_pd(temp0a,tmp2);
#endif
	  	  //temp[0][0:NSHAP] = coeff00*tab_fun_u_derx[0:NSHAP] + coeff01*tab_fun_u_dery[0:NSHAP] + coeff02*tab_fun_u_derz[0:NSHAP] + coeff03*shp[0:NSHAP];
		  tmp4 = _mm_set1_pd(coeff00);
		  tmp5 = _mm_set1_pd(coeff01);

		  tmp4 = _mm_mul_pd(tmp4,tab_fun_u_derx2);

#ifdef FMA
		  temp0b=_mm_fmadd_pd(tmp5,tab_fun_u_dery2,tmp4);
#else
		  tmp5 = _mm_mul_pd(tmp5,tab_fun_u_dery2);
		  temp0b = _mm_add_pd(tmp4,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff02);
#ifdef FMA
		  temp0b=_mm_fmadd_pd(tmp4,tab_fun_u_derz2,temp0b);
#else
		  tmp5 = _mm_mul_pd(tmp4,tab_fun_u_derz2);
		  temp0b = _mm_add_pd(temp0b,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff03);
#ifdef FMA
		  temp0b=_mm_fmadd_pd(tmp4,shp2,temp0b);
#else
		  tmp5 = _mm_mul_pd(tmp4,shp2);
		  temp0b = _mm_add_pd(temp0b,tmp5);
#endif
//		  temp[1][0:NSHAP] = coeff10*tab_fun_u_derx[0:NSHAP] + coeff11*tab_fun_u_dery[0:NSHAP] + coeff12*tab_fun_u_derz[0:NSHAP] + coeff13*shp[0:NSHAP];

		  tmp1 = _mm256_set1_pd(coeff10);
		  tmp2 = _mm256_set1_pd(coeff11);

		  tmp1 = _mm256_mul_pd(tmp1,tab_fun_u_derx1);
#ifdef FMA
		  temp1a=_mm256_fmadd_pd(tmp2,tab_fun_u_dery1,tmp1);
#else
		  tmp2 = _mm256_mul_pd(tmp2,tab_fun_u_dery1);
		  temp1a = _mm256_add_pd(tmp1,tmp2);
#endif
		  tmp1 = _mm256_set1_pd(coeff12);
#ifdef FMA
		  temp1a=_mm256_fmadd_pd(tmp1,tab_fun_u_derz1,temp1a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,tab_fun_u_derz1);
		  temp1a = _mm256_add_pd(temp1a,tmp2);
#endif
		  tmp1 = _mm256_set1_pd(coeff13);
#ifdef FMA
		  temp1a=_mm256_fmadd_pd(tmp1,shp1,temp1a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,shp1);
		  temp1a = _mm256_add_pd(temp1a,tmp2);
#endif

		  tmp4 = _mm_set1_pd(coeff10);
		  tmp5 = _mm_set1_pd(coeff11);

		  tmp4 = _mm_mul_pd(tmp4,tab_fun_u_derx2);
#ifdef FMA
		  temp1b=_mm_fmadd_pd(tmp5,tab_fun_u_dery2,tmp4);
#else
		  tmp5 = _mm_mul_pd(tmp5,tab_fun_u_dery2);
		  temp1b = _mm_add_pd(tmp4,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff12);
#ifdef FMA
		  temp1b=_mm_fmadd_pd(tmp4,tab_fun_u_derz2,temp1b);
#else
		  tmp5 = _mm_mul_pd(tmp4,tab_fun_u_derz2);
		  temp1b = _mm_add_pd(temp1b,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff13);
#ifdef FMA
		  temp1b=_mm_fmadd_pd(tmp4,shp2,temp1b);
#else
		  tmp5 = _mm_mul_pd(tmp4,shp2);
		  temp1b = _mm_add_pd(temp1b,tmp5);
#endif
//		  temp[2][0:NSHAP] = coeff20*tab_fun_u_derx[0:NSHAP] + coeff21*tab_fun_u_dery[0:NSHAP] + coeff22*tab_fun_u_derz[0:NSHAP] + coeff23*shp[0:NSHAP];

		  tmp1 = _mm256_set1_pd(coeff20);
		  tmp2 = _mm256_set1_pd(coeff21);

		  tmp1 = _mm256_mul_pd(tmp1,tab_fun_u_derx1);
#ifdef FMA
		  temp2a=_mm256_fmadd_pd(tmp2,tab_fun_u_dery1,tmp1);
#else
		  tmp2 = _mm256_mul_pd(tmp2,tab_fun_u_dery1);
		  temp2a = _mm256_add_pd(tmp1,tmp2);
#endif
		  tmp1 = _mm256_set1_pd(coeff22);
#ifdef FMA
		  temp2a=_mm256_fmadd_pd(tmp1,tab_fun_u_derz1,temp2a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,tab_fun_u_derz1);
		  temp2a = _mm256_add_pd(temp2a,tmp2);
#endif
		  tmp1 = _mm256_set1_pd(coeff23);
#ifdef FMA
		  temp2a=_mm256_fmadd_pd(tmp1,shp1,temp2a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,shp1);
		  temp2a = _mm256_add_pd(temp2a,tmp2);
#endif
		  tmp4 = _mm_set1_pd(coeff20);
		  tmp5 = _mm_set1_pd(coeff21);

		  tmp4 = _mm_mul_pd(tmp4,tab_fun_u_derx2);
#ifdef FMA
		  temp2b=_mm_fmadd_pd(tmp5,tab_fun_u_dery2,tmp4);
#else
		  tmp5 = _mm_mul_pd(tmp5,tab_fun_u_dery2);
		  temp2b = _mm_add_pd(tmp4,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff22);
#ifdef FMA
		  temp2b=_mm_fmadd_pd(tmp4,tab_fun_u_derz2,temp2b);
#else
		  tmp5 = _mm_mul_pd(tmp4,tab_fun_u_derz2);
		  temp2b = _mm_add_pd(temp2b,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff23);
#ifdef FMA
		  temp2b=_mm_fmadd_pd(tmp4,shp2,temp2b);
#else
		  tmp5 = _mm_mul_pd(tmp4,shp2);
		  temp2b = _mm_add_pd(temp2b,tmp5);
#endif

//		  temp[3][0:NSHAP] = coeff30*tab_fun_u_derx[0:NSHAP] + coeff31*tab_fun_u_dery[0:NSHAP] + coeff32*tab_fun_u_derz[0:NSHAP] + coeff33*shp[0:NSHAP];

		  tmp1 = _mm256_set1_pd(coeff30);
		  tmp2 = _mm256_set1_pd(coeff31);

		  tmp1 = _mm256_mul_pd(tmp1,tab_fun_u_derx1);
#ifdef FMA
		  temp3a=_mm256_fmadd_pd(tmp2,tab_fun_u_dery1,tmp1);
#else
		  tmp2 = _mm256_mul_pd(tmp2,tab_fun_u_dery1);
		  temp3a = _mm256_add_pd(tmp1,tmp2);
#endif

		  tmp1 = _mm256_set1_pd(coeff32);
#ifdef FMA
		  temp3a=_mm256_fmadd_pd(tmp1,tab_fun_u_derz1,temp3a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,tab_fun_u_derz1);
		  temp3a = _mm256_add_pd(temp3a,tmp2);
#endif
		  tmp1 = _mm256_set1_pd(coeff33);
#ifdef FMA
		  temp3a=_mm256_fmadd_pd(tmp1,shp1,temp3a);
#else
		  tmp2 = _mm256_mul_pd(tmp1,shp1);
		  temp3a = _mm256_add_pd(temp3a,tmp2);
#endif

		  tmp4 = _mm_set1_pd(coeff30);
		  tmp5 = _mm_set1_pd(coeff31);

		  tmp4 = _mm_mul_pd(tmp4,tab_fun_u_derx2);
#ifdef FMA
		  temp3b=_mm_fmadd_pd(tmp5,tab_fun_u_dery2,tmp4);
#else
		  tmp5 = _mm_mul_pd(tmp5,tab_fun_u_dery2);
		  temp3b = _mm_add_pd(tmp4,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff32);

#ifdef FMA
		  temp3b=_mm_fmadd_pd(tmp4,tab_fun_u_derz2,temp3b);
#else
		  tmp5 = _mm_mul_pd(tmp4,tab_fun_u_derz2);
		  temp3b = _mm_add_pd(temp3b,tmp5);
#endif
		  tmp4 = _mm_set1_pd(coeff33);
#ifdef FMA
		  temp3b=_mm_fmadd_pd(tmp4,shp2,temp3b);
#else
		  tmp5 = _mm_mul_pd(tmp4,shp2);
		  temp3b = _mm_add_pd(temp3b,tmp5);
#endif

//		  temp[0][i]=-4.285679
//		  temp[0][i]=-5.288616
//		  temp[0][i]=9.118954
//		  temp[0][i]=-15.468589
//		  temp[0][i]=-17.634250
//		  temp[0][i]=34.558181

//		  if(ielem<2&&igauss==5)
//		  {
//			  double* f = (double*)&temp0a;
//			  printf("%lf %lf %lf %lf\n",
//				  f[0], f[1], f[2], f[3]);
//			  double* g = (double*)&temp0b;
//			  printf("%lf %lf\n",
//				  g[0], g[1]);
//		  }

	#endif


	//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

			offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);

	#ifdef LOAD_VEC_COMP

		#ifdef LAPLACE

			shp1=_mm256_mul_pd(coeff03,shp1);
			shp2=_mm_mul_pd(_mm256_castpd256_pd128(coeff03),shp2);
			coeff03 = _mm256_set1_pd(vol);   //reuse to save registers
#ifdef FMA
			load_vec1=_mm256_fmadd_pd(coeff03,shp1,load_vec1);
			load_vec2=_mm_fmadd_pd(_mm256_castpd256_pd128(coeff03),shp2,load_vec2);
#else
			shp1=_mm256_mul_pd(coeff03,shp1);
			load_vec1=_mm256_add_pd(load_vec1,shp1);
			shp2=_mm_mul_pd(_mm256_castpd256_pd128(coeff03),shp2);
			load_vec2=_mm_add_pd(load_vec2,shp2);
#endif

		#elif defined (TEST_SCALAR)

			tmp1 = _mm256_set1_pd(coeff04);
			tmp2 = _mm256_mul_pd(tmp1,tab_fun_u_derx1);
		    tmp1 = _mm256_set1_pd(coeff14);
#ifdef FMA
		    tmp2=_mm256_fmadd_pd(tmp1,tab_fun_u_dery1,tmp2);
#else
		    tmp1 = _mm256_mul_pd(tmp1,tab_fun_u_dery1);
			tmp2 = _mm256_add_pd(tmp2,tmp1);
#endif
			tmp1 = _mm256_set1_pd(coeff24);
#ifdef FMA
		    tmp2=_mm256_fmadd_pd(tmp1,tab_fun_u_derz1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tmp1,tab_fun_u_derz1);
			tmp2 = _mm256_add_pd(tmp2,tmp1);
#endif
			tmp1 = _mm256_set1_pd(coeff34);
#ifdef FMA
		    tmp2=_mm256_fmadd_pd(tmp1,shp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tmp1,shp1);
			tmp2 = _mm256_add_pd(tmp2,tmp1);
#endif
			tmp1 = _mm256_set1_pd(vol);
#ifdef FMA
			load_vec1=_mm256_fmadd_pd(tmp2,tmp1,load_vec1);
#else
			tmp2 = _mm256_mul_pd(tmp2,tmp1);
			load_vec1 = _mm256_add_pd(load_vec1,tmp2);
#endif

			tmp4 = _mm_set1_pd(coeff04);
			tmp5 = _mm_mul_pd(tmp4,tab_fun_u_derx2);
		    tmp4 = _mm_set1_pd(coeff14);
#ifdef FMA
		    tmp5=_mm_fmadd_pd(tmp4,tab_fun_u_dery2,tmp5);
#else
		    tmp4 = _mm_mul_pd(tmp4,tab_fun_u_dery2);
			tmp5 = _mm_add_pd(tmp4,tmp5);
#endif
			tmp4 = _mm_set1_pd(coeff24);
#ifdef FMA
		    tmp5=_mm_fmadd_pd(tmp4,tab_fun_u_derz2,tmp5);
#else
			tmp4 = _mm_mul_pd(tmp4,tab_fun_u_derz2);
			tmp5 = _mm_add_pd(tmp5,tmp4);
#endif
			tmp4 = _mm_set1_pd(coeff34);
#ifdef FMA
		    tmp5=_mm_fmadd_pd(tmp4,shp2,tmp5);
#else
			tmp4 = _mm_mul_pd(tmp4,shp2);
			tmp5 = _mm_add_pd(tmp5,tmp4);
#endif
			tmp4 = _mm_set1_pd(vol);
#ifdef FMA
			load_vec2=_mm_fmadd_pd(tmp5,tmp4,load_vec2);
#else
			tmp5 = _mm_mul_pd(tmp5,tmp4);
			load_vec2 = _mm_add_pd(load_vec2,tmp5);
#endif

//			coeff04 * tab_fun_u_derx[0:NSHAP] +
//			 coeff14 * tab_fun_u_dery[0:NSHAP] +
//			 coeff24 * tab_fun_u_derz[0:NSHAP] +
//			 coeff34 * shp[0:NSHAP]		//TODO

		#endif

	#endif // end if computing RHS vector

	//-------------------------------------------------------------
	// ************************* second loop over shape functions ****************************//


	#ifdef LAPLACE
//			register __m256d stiff_mat1a;
//						register __m128d stiff_mat1b;
//						register __m256d stiff_mat2a;
//						register __m128d stiff_mat2b;
//						register __m256d stiff_mat3a;
//						register __m128d stiff_mat3b;
//						register __m256d stiff_mat4a;
//						register __m128d stiff_mat4b;
//						register __m256d stiff_mat5a;
//						register __m128d stiff_mat5b;
//						register __m256d stiff_mat6a;
//						register __m128d stiff_mat6b;
		//stiff_mat1a=
		//tmp1=_mm256_set1_pd(jac_0);

//			tab_fun_u_derx1 = _mm256_mul_pd(shpx1,tmp1);
//					  tab_fun_u_derx2 = _mm_mul_pd(shpx2,_mm256_castpd256_pd128(tmp1));
//					  tab_fun_u_dery1 = _mm256_mul_pd(shpx1,tmp2);
//					  tab_fun_u_dery2 = _mm_mul_pd(shpx2,_mm256_castpd256_pd128(tmp2));
//					  tab_fun_u_derz1 = _mm256_mul_pd(shpx1,tmp3);
//					  tab_fun_u_derz2

//			shufps $0x1b, %xmm0, %xmm0 # reverse order of the 4 floats
//			        shufps $0x00, %xmm1, %xmm1 # Broadcast least significant element to all elements
//			        shufps $0x55, %xmm2, %xmm2 # Broadcast second element to all elements
//			        shufps $0xAA, %xmm3, %xmm3 # Broadcast third element to all elements
//			        shufps $0xFF, %xmm4, %xmm4 # Broadcast most significant element to all elements
//			        shufps $0x39, %xmm5, %xmm5 # Rotate elements right
//			        shufps $0x93, %xmm6, %xmm6 # Rotate elements left

//			if(ielem==0)
//			{
//				SCALAR tmp[4];
//				_mm256_store_pd (&tmp[0],tab_fun_u_derx1);
//				for (i=0;i<4;i++)
//					printf("tab_fun_u_derx1[%d]=%lf\n",i,tmp[i]);
//				_mm_store_pd (&tmp[0],tab_fun_u_derx2);
//				for (i=0;i<2;i++)
//					printf("tab_fun_u_derx2[%d]=%lf\n",i,tmp[i]);
//			}

			//idof==0 //first 4

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2=_mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2=_mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
#ifdef FMA
			stiff_mat1a=_mm256_fmadd_pd(tmp2,coeff03,stiff_mat1a);
#else
			tmp1 = _mm256_mul_pd(tmp2,coeff03);
			stiff_mat1a = _mm256_add_pd(tmp1,stiff_mat1a);
#endif
			//idof==0 //last 2

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4=_mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4=_mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
#ifdef FMA
			stiff_mat1b = _mm_fmadd_pd(tmp4,_mm256_castpd256_pd128(coeff03),stiff_mat1b);
#else
			tmp5 = _mm_mul_pd(tmp4,_mm256_castpd256_pd128(coeff03));
			stiff_mat1b = _mm_add_pd(tmp5,stiff_mat1b);
#endif

			//idof==1 //first 4

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 0); //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
#ifdef FMA
			stiff_mat2a = _mm256_fmadd_pd(tmp2,coeff03,stiff_mat2a);
#else
			tmp1 = _mm256_mul_pd(tmp2,coeff03);
			stiff_mat2a = _mm256_add_pd(tmp1,stiff_mat2a);
#endif
			//idof==1 //last 2

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
#ifdef FMA
			stiff_mat2b = _mm_fmadd_pd(tmp4,_mm256_castpd256_pd128(coeff03),stiff_mat2b);
#else
			tmp5 = _mm_mul_pd(tmp4,_mm256_castpd256_pd128(coeff03));
			stiff_mat2b = _mm_add_pd(tmp5,stiff_mat2b);
#endif

			//idof==2 //first 4

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 85);//3 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
#ifdef FMA
			stiff_mat3a = _mm256_fmadd_pd(tmp2,coeff03,stiff_mat3a);
#else
			tmp1 = _mm256_mul_pd(tmp2,coeff03);
			stiff_mat3a = _mm256_add_pd(tmp1,stiff_mat3a);
#endif

			//idof==2 //last 2

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
#ifdef FMA
			stiff_mat3b = _mm_fmadd_pd(tmp4,_mm256_castpd256_pd128(coeff03),stiff_mat3b);
#else
			tmp5 = _mm_mul_pd(tmp4,_mm256_castpd256_pd128(coeff03));
			stiff_mat3b = _mm_add_pd(tmp5,stiff_mat3b);
#endif

			//idof==3 //first 4

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 85);//4 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
#ifdef FMA
			stiff_mat4a = _mm256_fmadd_pd(tmp2,coeff03,stiff_mat4a);
#else
			tmp1 = _mm256_mul_pd(tmp2,coeff03);
			stiff_mat4a = _mm256_add_pd(tmp1,stiff_mat4a);
#endif
			//idof==3 //last 2

			tmp1=_mm256_permute2f128_pd (tab_fun_u_derx1, tab_fun_u_derx1, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (tab_fun_u_dery1, tab_fun_u_dery1, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (tab_fun_u_derz1, tab_fun_u_derz1, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
#ifdef FMA
			stiff_mat4b = _mm_fmadd_pd(tmp4,_mm256_castpd256_pd128(coeff03),stiff_mat4b);
#else
			tmp5 = _mm_mul_pd(tmp4,_mm256_castpd256_pd128(coeff03));
			stiff_mat4b = _mm_add_pd(tmp5,stiff_mat4b);
#endif

			//idof==4 //first 4

			tmp4 = _mm_permute_pd(tab_fun_u_derx2,0); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp4 = _mm_permute_pd(tab_fun_u_dery2,0); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp4 = _mm_permute_pd(tab_fun_u_derz2,0); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
#ifdef FMA
			stiff_mat5a = _mm256_fmadd_pd(tmp2,coeff03,stiff_mat5a);
#else
			tmp1 = _mm256_mul_pd(tmp2,coeff03);
			stiff_mat5a = _mm256_add_pd(tmp1,stiff_mat5a);
#endif

			//idof==4 //last 2

			tmp5 = _mm_permute_pd(tab_fun_u_derx2,0); //5 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,tmp5);   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp5 = _mm_permute_pd(tab_fun_u_dery2,0); //5 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,tmp5);     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_permute_pd(tab_fun_u_derz2,0); //5 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,tmp5);     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
#ifdef FMA
			stiff_mat5b = _mm_fmadd_pd(tmp4,_mm256_castpd256_pd128(coeff03),stiff_mat5b);
#else
			tmp5 = _mm_mul_pd(tmp4,_mm256_castpd256_pd128(coeff03));
			stiff_mat5b = _mm_add_pd(tmp5,stiff_mat5b);
#endif
			//idof==5 //first 4

		    tmp4 = _mm_permute_pd(tab_fun_u_derx2,3); //6 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp4 = _mm_permute_pd(tab_fun_u_dery2,3); //6 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp4 = _mm_permute_pd(tab_fun_u_derz2,3); //6 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
#ifdef FMA
			stiff_mat6a = _mm256_fmadd_pd(tmp2,coeff03,stiff_mat6a);
#else
			tmp1 = _mm256_mul_pd(tmp2,coeff03);
			stiff_mat6a = _mm256_add_pd(tmp1,stiff_mat6a);
#endif

			//idof==5 //last 2

			tmp5 = _mm_permute_pd(tab_fun_u_derx2,3); //6 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,tmp5);   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp5 = _mm_permute_pd(tab_fun_u_dery2,3); //6 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,tmp5);     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_permute_pd(tab_fun_u_derz2,3); //6 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,tmp5);     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
#ifdef FMA
			stiff_mat6b = _mm_fmadd_pd(tmp4,_mm256_castpd256_pd128(coeff03),stiff_mat6b);
#else
			tmp5 = _mm_mul_pd(tmp4,_mm256_castpd256_pd128(coeff03));
			stiff_mat6b = _mm_add_pd(tmp5,stiff_mat6b);
#endif


//binarnie 0=0
//			1111=15
//			01010101=85

//			if(ielem==0)
//			{
//				//printf("%lf,%lf,%lf,%lf\n",tmp1.m256d_f64[0],tmp1.m256d_f64[1],tmp1.m256d_f64[2],tmp1.m256d_f64[3]);
//				SCALAR tmp[4];
//				_mm256_store_pd (&tmp[0],tmp1);
//				for (i=0;i<4;i++)
//					printf("shuffle[%d]=%lf\n",i,tmp[i]);
////				_mm_store_pd (&tmp[0],tmp2);
////				for (i=0;i<2;i++)
////					printf("tab_fun_u_derx2[%d]=%lf\n",i,tmp[i]);
//			}


	#endif

	#ifdef TEST_SCALAR

			//TODO

			tmp1 = _mm256_permute2f128_pd (temp0a, temp0a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = temp0[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = temp1[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = temp2[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(shp1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(shp1,tmp1);     //tmp1 = temp3[idof] * shp[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1 = _mm256_set1_pd(vol);
#ifdef FMA
			stiff_mat1a = _mm256_fmadd_pd(tmp2,tmp1,stiff_mat1a);
#else
			tmp1 = _mm256_mul_pd(tmp2,tmp1);
			stiff_mat1a = _mm256_add_pd(tmp1,stiff_mat1a);
#endif
//			temp[0][idof] * tab_fun_u_derx[0:NSHAP] +
//			temp[1][idof] * tab_fun_u_dery[0:NSHAP] +
//			temp[2][idof] * tab_fun_u_derz[0:NSHAP] +
//			temp[3][idof] * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]

			//idof==0 //last 2

			tmp1=_mm256_permute2f128_pd (temp0a, temp0a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = temp0[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = temp1[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(shp2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(shp2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_set1_pd(vol);
#ifdef FMA
			stiff_mat1b = _mm_fmadd_pd(tmp4,tmp5,stiff_mat1b);
#else
			tmp4 = _mm_mul_pd(tmp4,tmp5);
			stiff_mat1b = _mm_add_pd(tmp4,stiff_mat1b);
#endif
			//idof==1 //first 4

			tmp1=_mm256_permute2f128_pd (temp0a, temp0a, 0); //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = temp0[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 0);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(shp1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(shp1,tmp1);     //tmp1 = temp3[idof] * shp[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1 = _mm256_set1_pd(vol);
#ifdef FMA
			stiff_mat2a = _mm256_fmadd_pd(tmp2,tmp1,stiff_mat2a);
#else
			tmp1 = _mm256_mul_pd(tmp2,tmp1);
			stiff_mat2a = _mm256_add_pd(tmp1,stiff_mat2a);
#endif
			//idof==1 //last 2

			tmp1=_mm256_permute2f128_pd (temp0a, temp0a, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 0);  //2 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(shp2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(shp2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_set1_pd(vol);
#ifdef FMA
			stiff_mat2b = _mm_fmadd_pd(tmp4,tmp5,stiff_mat2b);
#else
			tmp4 = _mm_mul_pd(tmp4,tmp5);
			stiff_mat2b = _mm_add_pd(tmp4,stiff_mat2b);
#endif

			//idof==2 //first 4

			tmp1=_mm256_permute2f128_pd (temp0a, temp0a, 85);//3 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 85);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(shp1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(shp1,tmp1);     //tmp1 = temp3[idof] * shp[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1 = _mm256_set1_pd(vol);
#ifdef FMA
			stiff_mat3a = _mm256_fmadd_pd(tmp2,tmp1,stiff_mat3a);
#else
			tmp1 = _mm256_mul_pd(tmp2,tmp1);
			stiff_mat3a = _mm256_add_pd(tmp1,stiff_mat3a);
#endif
			//idof==2 //last 2

			tmp1=_mm256_permute2f128_pd (temp0a, temp0a, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 85);  //3 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 85);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,0);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(shp2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(shp2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_set1_pd(vol);
#ifdef FMA
			stiff_mat3b = _mm_fmadd_pd(tmp4,tmp5,stiff_mat3b);
#else
			tmp4 = _mm_mul_pd(tmp4,tmp5);
			stiff_mat3b = _mm_add_pd(tmp4,stiff_mat3b);
#endif
			//idof==3 //first 4

			tmp1=_mm256_permute2f128_pd (temp0a, temp0a, 85);//4 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 85);  //1 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(shp1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(shp1,tmp1);     //tmp1 = temp3[idof] * shp[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1 = _mm256_set1_pd(vol);
#ifdef FMA
			stiff_mat4a = _mm256_fmadd_pd(tmp2,tmp1,stiff_mat4a);
#else
			tmp1 = _mm256_mul_pd(tmp2,tmp1);
			stiff_mat4a = _mm256_add_pd(tmp1,stiff_mat4a);
#endif
			//idof==3 //last 2

			tmp1=_mm256_permute2f128_pd (temp0a, temp0a, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,_mm256_castpd256_pd128(tmp1));   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp1=_mm256_permute2f128_pd (temp1a, temp1a, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp2a, temp2a, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp1=_mm256_permute2f128_pd (temp3a, temp3a, 85);  //4 element
			tmp1 = _mm256_permute_pd(tmp1,15);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(shp2,_mm256_castpd256_pd128(tmp1),tmp4);
#else
			tmp5 = _mm_mul_pd(shp2,_mm256_castpd256_pd128(tmp1));     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_set1_pd(vol);
#ifdef FMA
			stiff_mat4b = _mm_fmadd_pd(tmp4,tmp5,stiff_mat4b);
#else
			tmp4 = _mm_mul_pd(tmp4,tmp5);
			stiff_mat4b = _mm_add_pd(tmp4,stiff_mat4b);
#endif
			//idof==4 //first 4

			tmp4 = _mm_permute_pd(temp0b,0); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);

//			if(ielem<1&&igauss==0)
//			{
//			  double* f = (double*)&tmp1;
//			  printf("5 element temp0 = %lf %lf %lf %lf\n",
//				  f[0], f[1], f[2], f[3]);
//
//			}


			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

//			if(ielem<1&&igauss==0)
//			{
//			  double* ff = (double*)&tmp2;
//			  printf("5 element temp0*tab_fun_u_derx = %lf %lf %lf %lf\n",
//				  ff[0], ff[1], ff[2], ff[3]);
//
//			}


			tmp4 = _mm_permute_pd(temp1b,0); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);

#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp4 = _mm_permute_pd(temp2b,0); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp4 = _mm_permute_pd(temp3b,0); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);

#ifdef FMA
			tmp2 = _mm256_fmadd_pd(shp1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(shp1,tmp1);     			//tmp1 = temp3[idof] * shp[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1 = _mm256_set1_pd(vol);

#ifdef FMA
			stiff_mat5a = _mm256_fmadd_pd(tmp2,tmp1,stiff_mat5a);
#else
			tmp1 = _mm256_mul_pd(tmp2,tmp1);
			stiff_mat5a = _mm256_add_pd(tmp1,stiff_mat5a);
#endif
			//idof==4 //last 2

			tmp5 = _mm_permute_pd(temp0b,0); //5 element

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,tmp5);   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp5 = _mm_permute_pd(temp1b,0); //5 element
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,tmp5);     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_permute_pd(temp2b,0); //5 element
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,tmp5);     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_permute_pd(temp3b,0); //5 element
#ifdef FMA
			tmp4 = _mm_fmadd_pd(shp2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(shp2,tmp5);     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_set1_pd(vol);
#ifdef FMA
			stiff_mat5b = _mm_fmadd_pd(tmp4,tmp5,stiff_mat5b);
#else
			tmp4 = _mm_mul_pd(tmp4,tmp5);
			stiff_mat5b = _mm_add_pd(tmp4,stiff_mat5b);
#endif
			//idof==5 //first 4

		    tmp4 = _mm_permute_pd(temp0b,3); //6 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);

			tmp2 = _mm256_mul_pd(tab_fun_u_derx1,tmp1);   //tmp2 = tab_fun_u_derx[idof] * tab_fun_u_derx[0:4]

			tmp4 = _mm_permute_pd(temp1b,3); //6 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_dery1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_dery1,tmp1);     //tmp1 = tab_fun_u_dery[idof] * tab_fun_u_dery[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp4 = _mm_permute_pd(temp2b,3); //6 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(tab_fun_u_derz1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(tab_fun_u_derz1,tmp1);     //tmp1 = tab_fun_u_derz[idof] * tab_fun_u_derz[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp4 = _mm_permute_pd(temp3b,3); //5 element
			tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp2 = _mm256_fmadd_pd(shp1,tmp1,tmp2);
#else
			tmp1 = _mm256_mul_pd(shp1,tmp1);     //tmp1 = temp3[idof] * shp[0:4]
			tmp2 = _mm256_add_pd(tmp2,tmp1);				//tmp2 = tmp2 + tmp1
#endif
			tmp1 = _mm256_set1_pd(vol);
#ifdef FMA
			stiff_mat6a = _mm256_fmadd_pd(tmp2,tmp1,stiff_mat6a);
#else
			tmp1 = _mm256_mul_pd(tmp2,tmp1);
			stiff_mat6a = _mm256_add_pd(tmp1,stiff_mat6a);
#endif
			//idof==5 //last 2

			tmp5 = _mm_permute_pd(temp0b,3); //6 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);

			tmp4 = _mm_mul_pd(tab_fun_u_derx2,tmp5);   //tmp4 = tab_fun_u_derx[idof] * tab_fun_u_derx[4:2]

			tmp5 = _mm_permute_pd(temp1b,3); //6 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_dery2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_dery2,tmp5);     //tmp5 = tab_fun_u_dery[idof] * tab_fun_u_dery[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_permute_pd(temp2b,3); //6 element
			//tmp1 = _mm256_set_m128d(tmp4,tmp4);
#ifdef FMA
			tmp4 = _mm_fmadd_pd(tab_fun_u_derz2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(tab_fun_u_derz2,tmp5);     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_permute_pd(temp3b,3); //6 element
#ifdef FMA
			tmp4 = _mm_fmadd_pd(shp2,tmp5,tmp4);
#else
			tmp5 = _mm_mul_pd(shp2,tmp5);     //tmp5 = tab_fun_u_derz[idof] * tab_fun_u_derz[4:2]
			tmp4 = _mm_add_pd(tmp4,tmp5);				//tmp4 = tmp5 + tmp4
#endif
			tmp5 = _mm_set1_pd(vol);
#ifdef FMA
			stiff_mat6b = _mm_fmadd_pd(tmp4,tmp5,stiff_mat6b);
#else
			tmp4 = _mm_mul_pd(tmp4,tmp5);
			stiff_mat6b = _mm_add_pd(tmp4,stiff_mat6b);
#endif


//			idof=0,temp[0][idof]=-15.629807,tab_fun_u_derx[0-6]=-15.775443, -18.350593, 34.126035, -4.227017, -4.917026, 9.144044, shp[0-6]=0.525783, 0.131446, 0.131446, 0.140883, 0.035221, 0.035221,  vol=0.000003
//			idof=1,temp[0][idof]=-18.584143,tab_fun_u_derx[0-6]=-15.775443, -18.350593, 34.126035, -4.227017, -4.917026, 9.144044, shp[0-6]=0.525783, 0.131446, 0.131446, 0.140883, 0.035221, 0.035221,  vol=0.000003
//			idof=2,temp[0][idof]=34.335959,tab_fun_u_derx[0-6]=-15.775443, -18.350593, 34.126035, -4.227017, -4.917026, 9.144044, shp[0-6]=0.525783, 0.131446, 0.131446, 0.140883, 0.035221, 0.035221,  vol=0.000003
//			idof=3,temp[0][idof]=-3.624461,tab_fun_u_derx[0-6]=-15.775443, -18.350593, 34.126035, -4.227017, -4.917026, 9.144044, shp[0-6]=0.525783, 0.131446, 0.131446, 0.140883, 0.035221, 0.035221,  vol=0.000003
//			idof=4,temp[0][idof]=-4.838723,tab_fun_u_derx[0-6]=-15.775443, -18.350593, 34.126035, -4.227017, -4.917026, 9.144044, shp[0-6]=0.525783, 0.131446, 0.131446, 0.140883, 0.035221, 0.035221,  vol=0.000003
//			idof=5,temp[0][idof]=9.341176,tab_fun_u_derx[0-6]=-15.775443, -18.350593, 34.126035, -4.227017, -4.917026, 9.144044, shp[0-6]=0.525783, 0.131446, 0.131446, 0.140883, 0.035221, 0.035221,  vol=0.000003

//			if(ielem==0)
//			{
//				//printf("%lf,%lf,%lf,%lf\n",tmp1.m256d_f64[0],tmp1.m256d_f64[1],tmp1.m256d_f64[2],tmp1.m256d_f64[3]);
//				SCALAR tmp[4];
//				_mm256_store_pd (&tmp[0],tmp1);
//				for (i=0;i<4;i++)
//					printf("shuffle[%d]=%lf\n",i,tmp[i]);
//				printf("tab_fun_u_derx2[%d]=%lf\n",i,tmp[i]);
//			}
//		  if(ielem<1&&igauss==0)
//		  {
//			  printf("temp0a = %lf %lf %lf %lf\n",temp0a.m256d_f64[0],temp0a.m256d_f64[1],temp0a.m256d_f64[2],temp0a.m256d_f64[3]);
//			  printf("temp0b = %lf %lf\n",temp0b.f64[0],temp0b.f64[1]);
//
//		  }

//		  if(ielem<1&&igauss==0)
//		  {
//			  double* f = (double*)&temp0a;
//			  printf("temp0a = %lf %lf %lf %lf\n",
//				  f[0], f[1], f[2], f[3]);
//			  double* g = (double*)&temp0b;
//			  printf("temp0b = %lf %lf\n",
//				  g[0], g[1]);
//
//			  double* h = (double*)&tab_fun_u_derx1;
//			  printf("tabfunuderx1 = %lf %lf %lf %lf\n",
//				  h[0], h[1], h[2], h[3]);
//			  double* hh = (double*)&tab_fun_u_derx2;
//			  printf("tabfunuderx2 = %lf %lf\n",
//				  hh[0], hh[1]);
//
//			  double* ff = (double*)&shp1;
//			  printf("shp1 = %lf %lf %lf %lf\n",
//				  ff[0], ff[1], ff[2], ff[3]);
//			  double* gg = (double*)&shp2;
//			  printf("shp2 = %lf %lf\n",
//				  gg[0], gg[1]);
//
//			  printf("%lf\n ",vol);
//
//		  }

	#endif


//	******* THE END OF: first loop over shape functions *******//
//	-------------------------------------------------------------

//	      }//idof

	//******* THE END OF: second loop over shape functions *******//
	//-------------------------------------------------------------

	    }//gauss
//
//	// ******** THE END OF: loop over integration points ********//
//	//-------------------------------------------------------------

#ifdef LOCAL_STIFF

	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
	__assume_aligned(el_data_out,ALIGN);
#pragma vector aligned
#pragma ivdep
    //for(i = 0; i < NDOFS*NDOFS; i++) el_data_out[offset+i] = stiff_mat[i];
	//el_data_out[offset:NDOFS*NDOFS]=stiff_mat[0:NDOFS*NDOFS];
	_mm256_store_pd (&el_data_out[offset],stiff_mat1a);
	_mm_store_pd (&el_data_out[offset+4],stiff_mat1b);
	_mm256_store_pd (&el_data_out[offset+NDOFS],stiff_mat2a);
	_mm_store_pd (&el_data_out[offset+NDOFS+4],stiff_mat2b);
	_mm256_store_pd (&el_data_out[offset+2*NDOFS],stiff_mat3a);
	_mm_store_pd (&el_data_out[offset+2*NDOFS+4],stiff_mat3b);
	_mm256_store_pd (&el_data_out[offset+3*NDOFS],stiff_mat4a);
	_mm_store_pd (&el_data_out[offset+3*NDOFS+4],stiff_mat4b);
	_mm256_store_pd (&el_data_out[offset+4*NDOFS],stiff_mat5a);
	_mm_store_pd (&el_data_out[offset+4*NDOFS+4],stiff_mat5b);
	_mm256_store_pd (&el_data_out[offset+5*NDOFS],stiff_mat6a);
	_mm_store_pd (&el_data_out[offset+5*NDOFS+4],stiff_mat6b);

  #ifdef LOAD_VEC_COMP
    __assume_aligned(el_data_out,ALIGN);
//#pragma vector aligned
//#pragma ivdep
    //for(i = 0; i < STRIDE; i++) el_data_out[offset+one_el_stiff_mat_size+i] = load_vec[i];
    _mm256_store_pd (&el_data_out[offset+one_el_stiff_mat_size],load_vec1);
    _mm_store_pd (&el_data_out[offset+one_el_stiff_mat_size+4],load_vec2);
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
