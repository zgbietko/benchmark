#include "mic.h"
#include "immintrin.h"


#define LOCAL_STIFF

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
	register int offset,offset2;
#endif

#pragma vector always

	__assume_aligned(gauss_dat_host,ALIGN);
	__assume_aligned(shape_fun_host,ALIGN);
	__assume_aligned(el_data_in,ALIGN);
	__assume_aligned(el_data_out,ALIGN);



#pragma omp parallel default(none) private(offset,offset2,ielem) firstprivate(nr_elem_mic,size_el_out,size_el_in,size_shp,geo_dat_size,nr_coeff,one_el_stiff_mat_size,one_el_load_vec_size) shared(gauss_dat_host,el_data_in,el_data_out,shape_fun_host)
{
	//-------------------------------------------------------------
	//******************* loop over elements processed by a thread *********************
//__attribute__((concurrency_safe(profitable)))
#pragma ivdep
#pragma vector aligned
//#pragma loop_count (97800)
	#pragma omp for schedule(guided) nowait
    for(ielem = 0; ielem < nr_elem_mic; ielem++){

		SCALAR tab_fun_u_derx[STRIDE*STRIDE] __attribute__((aligned(ALIGN)));
		SCALAR tab_fun_u_dery[STRIDE*STRIDE] __attribute__((aligned(ALIGN)));
		SCALAR tab_fun_u_derz[STRIDE*STRIDE] __attribute__((aligned(ALIGN)));

		#ifdef LOCAL_STIFF
			SCALAR stiff_mat[NDOFS*NDOFS]  __attribute__((aligned(ALIGN)));;
			SCALAR load_vec[NDOFS]  __attribute__((aligned(ALIGN)));;
		#endif

			//SCALAR transpose[STRIDE][STRIDE] __attribute__((aligned(ALIGN)));

	    int i;

	//-------------------------------------------------------------
	// ******************* READING INPUT DATA *********************


	    //printf("nr_coeff=%d\n",nr_coeff);

		#ifdef REGISTERS

	    	offset=nr_elem_mic*geo_dat_size+ielem*nr_coeff;
	    	//if(ielem<10)
	    	//printf("offset=%d\n",offset);

			#ifdef LAPLACE

	    		SCALAR coeff[NGAUSS] __attribute__((aligned(ALIGN)));
	    		//coeff[0:NGAUSS] = el_data_in[offset:NGAUSS];
				#pragma simd
	    		for(i=0;i<NGAUSS;i++)
	    			coeff[i]=el_data_in[offset+i];



	    	#elif defined(TEST_SCALAR)

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
	//******************** INITIALIZING SM AND LV ******************//

	#ifdef LOCAL_STIFF
#pragma ivdep
#pragma vector aligned
//#pragma loop_count (36)
#pragma simd
		    for(i = 0; i < NDOFS*NDOFS; i++) stiff_mat[i] = zero;

			  #ifdef LOAD_VEC_COMP
#pragma ivdep
#pragma vector aligned
//#pragma loop_count (6)
#pragma simd
			    for(i = 0; i < STRIDE; i++) load_vec[i] = zero;
			  #endif
	#endif


	SCALAR shp[STRIDE*STRIDE]  __attribute__((aligned(ALIGN)));;
	SCALAR shpx[STRIDE*STRIDE]  __attribute__((aligned(ALIGN)));;
	SCALAR shpy[STRIDE*STRIDE]  __attribute__((aligned(ALIGN)));;
	SCALAR shpz[STRIDE*STRIDE]  __attribute__((aligned(ALIGN)));;

#ifdef TEST_SCALAR
      SCALAR temp[4*STRIDE][NSHAP] __attribute__((aligned(ALIGN)));;
#endif

      SCALAR vol[STRIDE];
      vol[:]= weight_linear_prism; // vol = weight CONSTANT FOR LINEAR PRISM!!!

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
//#pragma simd
	    for(igauss = 0; igauss < NGAUSS; igauss++){


	      // integration data read from cached constant or shared  memory

	      SCALAR daux = gauss_dat_host[igauss];
	      SCALAR faux = gauss_dat_host[NGAUSS+igauss];
	      SCALAR eaux = gauss_dat_host[2*NGAUSS+igauss];
	      //SCALAR vol = gauss_dat_host[4*igauss+3]; // vol = weight

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
	      vol[igauss] *= daux; // vol = weight * det J

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

	      offset=igauss*STRIDE;

#pragma vector aligned
#pragma ivdep
	      shpx[offset:STRIDE]=shape_fun_host[igauss*STRIDE:STRIDE];
#pragma vector aligned
#pragma ivdep
	      shpy[offset:STRIDE]=shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:STRIDE];
#pragma vector aligned
#pragma ivdep
	      shpz[offset:STRIDE]=shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:STRIDE];
#pragma vector aligned
#pragma ivdep
	      shp[offset:STRIDE]=shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:STRIDE];


#pragma vector aligned
//#pragma ivdep
		tab_fun_u_derx[offset:STRIDE] = shpx[offset:STRIDE]*jac_0+shpy[offset:STRIDE]*jac_3+shpz[offset:STRIDE]*jac_6;
#pragma vector aligned
//#pragma ivdep
		tab_fun_u_dery[offset:STRIDE] = shpx[offset:STRIDE]*jac_1+shpy[offset:STRIDE]*jac_4+shpz[offset:STRIDE]*jac_7;
#pragma vector aligned
//#pragma ivdep
		tab_fun_u_derz[offset:STRIDE] = shpx[offset:STRIDE]*jac_2+shpy[offset:STRIDE]*jac_5+shpz[offset:STRIDE]*jac_8;


	//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
	//-------------------------------------------------------------

	#ifdef TEST_SCALAR

		#ifdef REGISTERS

	  	  temp[igauss*4+0][0:NSHAP] = coeff00*tab_fun_u_derx[offset:NSHAP] + coeff01*tab_fun_u_dery[offset:NSHAP] + coeff02*tab_fun_u_derz[offset:NSHAP] + coeff03*shp[offset:NSHAP];
		  temp[igauss*4+1][0:NSHAP] = coeff10*tab_fun_u_derx[offset:NSHAP] + coeff11*tab_fun_u_dery[offset:NSHAP] + coeff12*tab_fun_u_derz[offset:NSHAP] + coeff13*shp[offset:NSHAP];
		  temp[igauss*4+2][0:NSHAP] = coeff20*tab_fun_u_derx[offset:NSHAP] + coeff21*tab_fun_u_dery[offset:NSHAP] + coeff22*tab_fun_u_derz[offset:NSHAP] + coeff23*shp[offset:NSHAP];
		  temp[igauss*4+3][0:NSHAP] = coeff30*tab_fun_u_derx[offset:NSHAP] + coeff31*tab_fun_u_dery[offset:NSHAP] + coeff32*tab_fun_u_derz[offset:NSHAP] + coeff33*shp[offset:NSHAP];

		#else

		  register int offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;

		  temp[igauss*4+0][0:NSHAP] = el_data_in[offset2+0]*tab_fun_u_derx[offset:NSHAP] + el_data_in[offset2+1]*tab_fun_u_dery[offset:NSHAP] + el_data_in[offset2+2]*tab_fun_u_derz[offset:NSHAP] + el_data_in[offset2+12]*shp[offset:NSHAP];
		  temp[igauss*4+1][0:NSHAP] = el_data_in[offset2+3]*tab_fun_u_derx[offset:NSHAP] + el_data_in[offset2+4]*tab_fun_u_dery[offset:NSHAP] + el_data_in[offset2+5]*tab_fun_u_derz[offset:NSHAP] + el_data_in[offset2+13]*shp[offset:NSHAP];
		  temp[igauss*4+2][0:NSHAP] = el_data_in[offset2+6]*tab_fun_u_derx[offset:NSHAP] + el_data_in[offset2+7]*tab_fun_u_dery[offset:NSHAP] + el_data_in[offset2+8]*tab_fun_u_derz[offset:NSHAP] + el_data_in[offset2+14]*shp[offset:NSHAP];
		  temp[igauss*4+3][0:NSHAP] = el_data_in[offset2+9]*tab_fun_u_derx[offset:NSHAP] + el_data_in[offset2+10]*tab_fun_u_dery[offset:NSHAP] + el_data_in[offset2+11]*tab_fun_u_derz[offset:NSHAP] + el_data_in[offset2+15]*shp[offset:NSHAP];

		#endif

	#endif


	//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	    }//gauss
//
//	// ******** THE END OF: loop over integration points ********//
//	//-------------------------------------------------------------

	    //separate loop for vectorization
#pragma vector aligned
#pragma ivdep
//#pragma loop_count (6)
//#pragma simd vectorlength(4)
	    for(igauss = 0; igauss < NGAUSS; igauss++){

	//-------------------------------------------------------------
	//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

//		  printf("stiff_mat_out_rhs[%d]=%lf\n",offset+NDOFS*NDOFS+idof,el_data_out[offset+NDOFS*NDOFS+idof]);


			offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
			offset2=igauss*STRIDE;

	#ifdef LOAD_VEC_COMP
//#pragma vector nontemporal
			//#pragma vector always

	#ifdef LOCAL_STIFF
		  //load_vec[igauss] = __sec_reduce_add(
			load_vec[0:NSHAP] += (
	#else
		  el_data_out[offset+one_el_stiff_mat_size+igauss] = __sec_reduce_add(
		//		  tmp[:] +=(
	#endif

	  #ifdef LAPLACE

		#ifdef REGISTERS

				     coeff[igauss] * shp[offset2:NSHAP]
				 //el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shp[offset2:NSHAP]

		#else
				     el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shp[offset2:NSHAP]

		#endif

	  #elif defined(TEST_SCALAR)

	    #ifdef REGISTERS

			 coeff04 * tab_fun_u_derx[offset2:NSHAP] +
			 coeff14 * tab_fun_u_dery[offset2:NSHAP] +
			 coeff24 * tab_fun_u_derz[offset2:NSHAP] +
			 coeff34 * shp[offset2:NSHAP]

		#else

		   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+16] * tab_fun_u_derx[offset2:NSHAP] +
		   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+17] * tab_fun_u_dery[offset2:NSHAP] +
		   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+18] * tab_fun_u_derz[offset2:NSHAP] +
		   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+19] * shp[offset2:NSHAP]

	    #endif // REGISTERS

	  #endif

				     ) * vol[igauss];



//#ifdef LOCAL_STIFF
//	  load_vec[0+4:2] += (
//#else
//	 el_data_out[offset+one_el_stiff_mat_size+4:2] += (
//	//		  tmp[:] +=(
//#endif
//
//  #ifdef LAPLACE
//
//	#ifdef REGISTERS
//
//			     coeff[igauss] * shp[offset2+4:2]
//			 //el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shp[offset2+4:2]
//
//	#else
//			     el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shp[offset2+4:2]
//
//	#endif
//
//  #elif defined(TEST_SCALAR)
//
//    #ifdef REGISTERS
//
//		 coeff04 * tab_fun_u_derx[offset2+4:2] +
//		 coeff14 * tab_fun_u_dery[offset2+4:2] +
//		 coeff24 * tab_fun_u_derz[offset2+4:2] +
//		 coeff34 * shp[offset2+4:2]
//
//	#else
//
//	   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+16] * tab_fun_u_derx[offset2+4:2] +
//	   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+17] * tab_fun_u_dery[offset2+4:2] +
//	   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+18] * tab_fun_u_derz[offset2+4:2] +
//	   el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+19] * shp[offset2+4:2]
//
//    #endif // REGISTERS
//
//  #endif
//
//			     ) * vol[igauss];


	 //printf("stiff_mat_out_rhs[%d]=%lf\n",offset+NDOFS*NDOFS+idof,el_data_out[offset+NDOFS*NDOFS+idof]);

	 //printf("el_data_in[%d]=%lf\n",nr_elem_mic*geo_dat_size+ielem*nr_coeff+idof,el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+idof]); //ok
	 //printf("shp_fun_u=%lf\n",shp_fun_u);

	#endif // end if computing RHS vector

	//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	// ************************* second loop over shape functions ****************************//

//	  	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);  //loop invariant code motion
#pragma vector aligned
#pragma ivdep
	 // #pragma loop_count(6)
	  //#pragma simd reduction(+:stiff_mat)
for(idof = 0; idof < NSHAP; idof++){
//SCALAR tmp[NSHAP];
//idof=0;

#pragma vector aligned
#pragma ivdep

//#pragma unroll(2)
	#ifdef LOCAL_STIFF
	  	 stiff_mat[idof*NDOFS:NSHAP] += (
		//tmp[:]+=(
	#else
		 __assume_aligned(el_data_out,ALIGN);
		 el_data_out[offset+idof*NDOFS:NSHAP] += (
	#endif

		#ifdef LAPLACE

				tab_fun_u_derx[offset2:NSHAP] * tab_fun_u_derx[offset2+idof] +
				tab_fun_u_dery[offset2:NSHAP] * tab_fun_u_dery[offset2+idof] +
				tab_fun_u_derz[offset2:NSHAP] * tab_fun_u_derz[offset2+idof]

		#elif defined(TEST_SCALAR)

				temp[igauss*4+0][idof] * tab_fun_u_derx[offset2:NSHAP] +
				temp[igauss*4+1][idof] * tab_fun_u_dery[offset2:NSHAP] +
				temp[igauss*4+2][idof] * tab_fun_u_derz[offset2:NSHAP] +
				temp[igauss*4+3][idof] * shp[offset2:NSHAP]

		#endif

									) * vol[igauss];

//#pragma vector aligned
//#pragma ivdep
////#pragma unroll(2)
//	#ifdef LOCAL_STIFF
//	  	 stiff_mat[idof*NDOFS+4:2] += (
//		//tmp[:]+=(
//	#else
//		 __assume_aligned(el_data_out,ALIGN);
//		 el_data_out[offset+idof*NDOFS+4:2] += (
//	#endif
//
//		#ifdef LAPLACE
//
//				tab_fun_u_derx[offset2+idof] * tab_fun_u_derx[offset2+4:2] +
//				tab_fun_u_dery[offset2+idof] * tab_fun_u_dery[offset2+4:2] +
//				tab_fun_u_derz[offset2+idof] * tab_fun_u_derz[offset2+4:2]
//
//		#elif defined(TEST_SCALAR)
//
//				temp[igauss*4+0][idof] * tab_fun_u_derx[offset2+4:2] +
//				temp[igauss*4+1][idof] * tab_fun_u_dery[offset2+4:2] +
//				temp[igauss*4+2][idof] * tab_fun_u_derz[offset2+4:2] +
//				temp[igauss*4+3][idof] * shp[offset2+4:2]
//
//		#endif
//
//									) * vol[igauss];


//	******* THE END OF: first loop over shape functions *******//
//	-------------------------------------------------------------

	      }//idof

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
	el_data_out[offset:NDOFS*NDOFS]=stiff_mat[0:NDOFS*NDOFS];
  #ifdef LOAD_VEC_COMP
    __assume_aligned(el_data_out,ALIGN);
#pragma vector aligned
#pragma ivdep
    //for(i = 0; i < STRIDE; i++) el_data_out[offset+one_el_stiff_mat_size+i] = load_vec[i];
    el_data_out[offset+one_el_stiff_mat_size:STRIDE]=load_vec[0:STRIDE];
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
