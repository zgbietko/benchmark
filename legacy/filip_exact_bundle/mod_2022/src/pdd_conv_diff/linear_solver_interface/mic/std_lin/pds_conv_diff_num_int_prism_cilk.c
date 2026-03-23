#include "mic.h"
#include<unistd.h>

#define LOCAL_STIFF

#define NGAUSS 6
#define NSHAP 6
#define NGEO 8
#define NDOFS 6
#define STRIDE 8

//#define THR 240

int pdr_num_int_el_QSS_prism(
		SCALAR * gauss_dat_host, // integration points data of elements having given p
		SCALAR * shape_fun_host, // shape functions on a reference element
		SCALAR * el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
		SCALAR * el_data_out, // result of integration of NR_ELEMS_THIS_KERCALL elements
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

	//printf("rozmiary:%d,%d,%d,%d,%d,%d\n",size_el_out, size_el_in, size_shp,geo_dat_size,one_el_stiff_mat_size,one_el_load_vec_size);

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
#pragma offload target(mic) nocopy(gauss_dat_host[0:1344]:align(ALIGN) alloc_if(0) free_if(1)) nocopy(shape_fun_host[0:size_shp]:align(ALIGN) alloc_if(0) free_if(1)) \
    nocopy(el_data_in[0:size_el_in]:align(ALIGN) alloc_if(0) free_if(1)) nocopy(el_data_out[0:size_el_out]:align(ALIGN) alloc_if(1) free_if(0))
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
//#pragma vector aligned
//#pragma loop_count (97800)
	#pragma omp for schedule(guided) nowait
    for(ielem = 0; ielem < nr_elem_mic; ielem++){

		#ifdef COMPUTE_ALL_SHAPE_FUN_DER
			//register SCALAR *workspace = (SCALAR *) malloc((geo_dat_size+nr_coeff)*sizeof(SCALAR));
//				SCALAR *tab_fun_u_derx = (SCALAR *) _mm_malloc(NSHAP*sizeof(SCALAR),ALIGN);
//				SCALAR *tab_fun_u_dery = (SCALAR *) _mm_malloc(NSHAP*sizeof(SCALAR),ALIGN);
//				SCALAR *tab_fun_u_derz = (SCALAR *) _mm_malloc(NSHAP*sizeof(SCALAR),ALIGN);
    			SCALAR tab_fun_u_derx[STRIDE] __attribute__((aligned(ALIGN)));
    			SCALAR tab_fun_u_dery[STRIDE] __attribute__((aligned(ALIGN)));
    			SCALAR tab_fun_u_derz[STRIDE] __attribute__((aligned(ALIGN)));

		#endif

		#ifdef LOCAL_STIFF
			SCALAR stiff_mat[NDOFS*NDOFS]  __attribute__((aligned(ALIGN)));;
			SCALAR load_vec[STRIDE]  __attribute__((aligned(ALIGN)));;
		#endif

		int i;

	//-------------------------------------------------------------
	// ******************* READING INPUT DATA *********************


	    //printf("nr_coeff=%d\n",nr_coeff);

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

//				if(ielem==0)
//				{
//					printf("coeff00=%lf,coeff01=%lf,coeff02=%lf,coeff10=%lf,coeff11=%lf,coeff12=%lf,coeff20=%lf,coeff21=%lf,coeff22=%lf,"
//							"coeff30=%lf,coeff31=%lf,coeff32=%lf,coeff03=%lf,coeff13=%lf,coeff23=%lf,coeff33=%lf,coeff04=%lf,coeff14=%lf,coeff24=%lf,coeff34=%lf\n",
//							coeff00,coeff01,coeff02,coeff10,coeff11,coeff12,coeff20,coeff21,coeff22,coeff30,coeff31,
//							coeff32,coeff03,coeff13,coeff23,coeff33,coeff04,coeff14,coeff24,coeff34);
//					printf("\n");
//				}



	    #endif

	// ******* THE END OF: READING INPUT DATA *********************
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//******************** INITIALIZING SM AND LV ******************//

	#ifdef LOCAL_STIFF
#pragma ivdep
//#pragma vector aligned
//#pragma loop_count (36)
#pragma simd
		    for(i = 0; i < NDOFS*NDOFS; i++) stiff_mat[i] = zero;

			  #ifdef LOAD_VEC_COMP
#pragma ivdep
//#pragma vector aligned
//#pragma loop_count (6)
#pragma simd
			    for(i = 0; i < STRIDE; i++) load_vec[i] = zero;
			  #endif

	#endif
//	    offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
//#pragma ivdep
////#pragma vector aligned
//	    for(i = 0; i < one_el_stiff_mat_size; i++) el_data_out[offset+i] = zero;
//
//	  #ifdef LOAD_VEC_COMP
//#pragma ivdep
////#pragma vector aligned
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
			////#pragma vector aligned
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

	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

	      //} // the end of block to indicate the scope of jac_x registers
	 //************ loop for computing ALL shape function values at integration point **********//



	  //  tab_fun_u_derx[0:NSHAP] = shape_fun_host[igauss*4*NSHAP+4*idof+1]*jac_0+shape_fun_host[igauss*4*NSHAP+4*idof+2]*jac_3+shape_fun_host[igauss*4*NSHAP+4*idof+3]*jac_6;
		//tab_fun_u_dery[0:NSHAP] = shape_fun_host[igauss*4*NSHAP+4*idof+1]*jac_1+shape_fun_host[igauss*4*NSHAP+4*idof+2]*jac_4+shape_fun_host[igauss*4*NSHAP+4*idof+3]*jac_7;
	//	tab_fun_u_derz[0:NSHAP] = shape_fun_host[igauss*4*NSHAP+4*idof+1]*jac_2+shape_fun_host[igauss*4*NSHAP+4*idof+2]*jac_5+shape_fun_host[igauss*4*NSHAP+4*idof+3]*jac_8;


////#pragma vector aligned
//#pragma ivdep
//#pragma simd
//	      for(i=0;i<NSHAP;i++)
//		tab_fun_u_derx[i] = shape_fun_host[igauss*NSHAP+i]*jac_0+shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+i]*jac_3+shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+i]*jac_6;
////#pragma vector aligned
//#pragma ivdep
//#pragma simd
//	      for(i=0;i<NSHAP;i++)
//		tab_fun_u_derx[i] = shape_fun_host[igauss*NSHAP+i]*jac_1+shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+i]*jac_4+shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+i]*jac_7;
////#pragma vector aligned
//#pragma ivdep
//#pragma simd
//	      for(i=0;i<NSHAP;i++)
//		tab_fun_u_derx[i] = shape_fun_host[igauss*NSHAP+i]*jac_2+shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+i]*jac_5+shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+i]*jac_8;

	      SCALAR shp[STRIDE]  __attribute__((aligned(ALIGN)));;
	      SCALAR shpx[STRIDE]  __attribute__((aligned(ALIGN)));;
	      SCALAR shpy[STRIDE]  __attribute__((aligned(ALIGN)));;
	      SCALAR shpz[STRIDE]  __attribute__((aligned(ALIGN)));;

//#pragma vector aligned
#pragma ivdep
	      shpx[0:STRIDE]=shape_fun_host[igauss*STRIDE:STRIDE];
//#pragma vector aligned
#pragma ivdep
	      shpy[0:STRIDE]=shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:STRIDE];
//#pragma vector aligned
#pragma ivdep
	      shpz[0:STRIDE]=shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:STRIDE];
//#pragma vector aligned
#pragma ivdep
	      shp[0:STRIDE]=shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:STRIDE];

//#pragma vector aligned
//#pragma ivdep
		tab_fun_u_derx[0:STRIDE] = shpx[0:STRIDE]*jac_0+shpy[0:STRIDE]*jac_3+shpz[0:STRIDE]*jac_6;
//#pragma vector aligned
//#pragma ivdep
		tab_fun_u_dery[0:STRIDE] = shpx[0:STRIDE]*jac_1+shpy[0:STRIDE]*jac_4+shpz[0:STRIDE]*jac_7;
//#pragma vector aligned
//#pragma ivdep
		tab_fun_u_derz[0:STRIDE] = shpx[0:STRIDE]*jac_2+shpy[0:STRIDE]*jac_5+shpz[0:STRIDE]*jac_8;

////#pragma vector aligned
//#pragma ivdep
//#pragma simd
//	      for(idof = 0; idof < NSHAP; idof++){
//

//		// read proper values of shape functions and their derivatives
//		temp1 = shape_fun_host[igauss*4*NSHAP+4*idof+1];
//		temp2 = shape_fun_host[igauss*4*NSHAP+4*idof+2];
//		temp3 = shape_fun_host[igauss*4*NSHAP+4*idof+3];
//		//if(ielem==0)
//		//printf("igauss=%d,idof=%d,temp1,2,3=%lf,%lf,%lf\n",igauss,idof,temp1,temp2,temp3);
//
//		tab_fun_u_derx[idof] = temp1*jac_0+temp2*jac_3+temp3*jac_6;
//		tab_fun_u_dery[idof] = temp1*jac_1+temp2*jac_4+temp3*jac_7;
//		tab_fun_u_derz[idof] = temp1*jac_2+temp2*jac_5+temp3*jac_8;
//		//if(ielem==0)
//		//printf("igauss=%d,idof=%d,tab_fun_u_derx,y,z=%lf,%lf,%lf\n",igauss,idof,tab_fun_u_derx[idof],tab_fun_u_dery[idof],tab_fun_u_derz[idof]);
//
//	      } // end loop over shape functions for which global derivatives were computed

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

	#ifdef TEST_SCALAR

	      SCALAR temp[4][NSHAP] __attribute__((aligned(ALIGN)));;

	#endif


	//*** THE END OF: SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS ***//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//********************* first loop over shape functions ***********************//



//	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);  //loop invariant code motion
////#pragma vector aligned
//#pragma ivdep
//#pragma loop_count(6)
//#pragma simd
//	      for(idof = 0; idof < NSHAP; idof++){

		//{ // beginning of using registers for u  (shp_fun_u, fun_u_der.)

	//-------------------------------------------------------------
	//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ******//

//	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

	  		  // read proper values of shape functions and their derivatives
	          //SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];
//	          SCALAR fun_u_derx = tab_fun_u_derx[idof];
//	          SCALAR fun_u_dery = tab_fun_u_dery[idof];
//	          SCALAR fun_u_derz = tab_fun_u_derz[idof];
//	#else // if not COMPUTE_ALL_SHAPE_FUN_DER
	          // read proper values of shape functions and their derivatives
			  //SCALAR shp_fun_u = shape_fun_host[3*NSHAP*NGAUSS+igauss*NSHAP+idof];
//			  temp1 = shape_fun_host[igauss*NSHAP+idof];
//			  temp2 = shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof];
//			  temp3 = shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof];
//
//
//			  // compute derivatives wrt global coordinates
//			  // 15 operations
//			  SCALAR fun_u_derx = temp1*jac_0 + temp2*jac_3 + temp3*jac_6;
//			  SCALAR fun_u_dery = temp1*jac_1 + temp2*jac_4 + temp3*jac_7;
//			  SCALAR fun_u_derz = temp1*jac_2 + temp2*jac_5 + temp3*jac_8;
//
//			  SCALAR fun_u_derx = shape_fun_host[igauss*NSHAP+idof]*jac_0 + shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof]*jac_3 + shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof]*jac_6;
//			  SCALAR fun_u_dery = shape_fun_host[igauss*NSHAP+idof]*jac_1 + shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof]*jac_4 + shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof]*jac_7;
//			  SCALAR fun_u_derz = shape_fun_host[igauss*NSHAP+idof]*jac_2 + shape_fun_host[NSHAP*NGAUSS+igauss*NSHAP+idof]*jac_5 + shape_fun_host[2*NSHAP*NGAUSS+igauss*NSHAP+idof]*jac_8;
//

//	#endif // COMPUTE_ALL_SHAPE_FUN_DER

	//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//

			  //TODO sprawdzic czy kolejnosc indeksow ma znacznie zaminic uzycie temp123 z temp4567 od tego miejsca

	#ifdef TEST_SCALAR

#ifdef COMPUTE_ALL_SHAPE_FUN_DER

		#ifdef REGISTERS

	  	  temp[0][0:NSHAP] = coeff00*tab_fun_u_derx[0:NSHAP] + coeff01*tab_fun_u_dery[0:NSHAP] + coeff02*tab_fun_u_derz[0:NSHAP] + coeff03*shp[0:NSHAP];
		  temp[1][0:NSHAP] = coeff10*tab_fun_u_derx[0:NSHAP] + coeff11*tab_fun_u_dery[0:NSHAP] + coeff12*tab_fun_u_derz[0:NSHAP] + coeff13*shp[0:NSHAP];
		  temp[2][0:NSHAP] = coeff20*tab_fun_u_derx[0:NSHAP] + coeff21*tab_fun_u_dery[0:NSHAP] + coeff22*tab_fun_u_derz[0:NSHAP] + coeff23*shp[0:NSHAP];
		  temp[3][0:NSHAP] = coeff30*tab_fun_u_derx[0:NSHAP] + coeff31*tab_fun_u_dery[0:NSHAP] + coeff32*tab_fun_u_derz[0:NSHAP] + coeff33*shp[0:NSHAP];

		#else
		#ifdef MIC
	      register int __attribute__((target(mic))) offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;
		#else
	      register int offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;
		#endif


		  temp[0][0:NSHAP] = el_data_in[offset2+0]*tab_fun_u_derx[0:NSHAP] + el_data_in[offset2+1]*tab_fun_u_dery[0:NSHAP] + el_data_in[offset2+2]*tab_fun_u_derz[0:NSHAP] + el_data_in[offset2+12]*shp[0:NSHAP];
		  temp[1][0:NSHAP] = el_data_in[offset2+3]*tab_fun_u_derx[0:NSHAP] + el_data_in[offset2+4]*tab_fun_u_dery[0:NSHAP] + el_data_in[offset2+5]*tab_fun_u_derz[0:NSHAP] + el_data_in[offset2+13]*shp[0:NSHAP];
		  temp[2][0:NSHAP] = el_data_in[offset2+6]*tab_fun_u_derx[0:NSHAP] + el_data_in[offset2+7]*tab_fun_u_dery[0:NSHAP] + el_data_in[offset2+8]*tab_fun_u_derz[0:NSHAP] + el_data_in[offset2+14]*shp[0:NSHAP];
		  temp[3][0:NSHAP] = el_data_in[offset2+9]*tab_fun_u_derx[0:NSHAP] + el_data_in[offset2+10]*tab_fun_u_dery[0:NSHAP] + el_data_in[offset2+11]*tab_fun_u_derz[0:NSHAP] + el_data_in[offset2+15]*shp[0:NSHAP];

		#endif

#else
		#ifdef REGISTERS

		  temp[0][0:NSHAP] = coeff00*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + coeff01*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + coeff02*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + coeff03*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];
		  temp[1][0:NSHAP] = coeff10*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + coeff11*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + coeff12*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + coeff13*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];
		  temp[2][0:NSHAP] = coeff20*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + coeff21*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + coeff22*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + coeff23*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];
		  temp[3][0:NSHAP] = coeff30*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + coeff31*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + coeff32*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + coeff33*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];

		#else

		#ifdef MIC
	      register int __attribute__((target(mic))) offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;
		#else
	      register int offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;
		#endif

		  temp[0][0:NSHAP] = el_data_in[offset2+0]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + el_data_in[offset2+1]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + el_data_in[offset2+2]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + el_data_in[offset2+12]*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];
		  temp[1][0:NSHAP] = el_data_in[offset2+3]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + el_data_in[offset2+4]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + el_data_in[offset2+5]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + el_data_in[offset2+13]*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];
		  temp[2][0:NSHAP] = el_data_in[offset2+6]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + el_data_in[offset2+7]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + el_data_in[offset2+8]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + el_data_in[offset2+14]*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];
		  temp[3][0:NSHAP] = el_data_in[offset2+9]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) + el_data_in[offset2+10]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) + el_data_in[offset2+11]*(shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) + el_data_in[offset2+15]*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP];

		#endif

#endif
//		  if(ielem<2&&igauss==5)
//		  {
//			  printf("\nielem=%d\n",ielem);
//			  for(i=0;i<NSHAP;i++)
//				  printf("temp[0][i]=%lf\n",temp[0][i]);
//		  }

	#elif defined(HEAT)

	#endif


	//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

//		  printf("stiff_mat_out_rhs[%d]=%lf\n",offset+NDOFS*NDOFS+idof,el_data_out[offset+NDOFS*NDOFS+idof]);


			offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);

	#ifdef LOAD_VEC_COMP

//#pragma vector nontemporal
			//#pragma vector always

	#ifdef LOCAL_STIFF
		  load_vec[0:NSHAP] += (
	#else
		 el_data_out[offset+one_el_stiff_mat_size:NSHAP] += (
		//		  tmp[:] +=(
	#endif

	  #ifdef LAPLACE

#ifdef COMPUTE_ALL_SHAPE_FUN_DER

		#ifdef REGISTERS

				     coeff03 * shp[0:NSHAP]

		#else
				     el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shp[0:NSHAP]

		#endif

#else

		#ifdef REGISTERS

					 coeff03 * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]

		#else
					 el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]

		#endif

#endif


	  #elif defined(TEST_SCALAR)

#ifdef COMPUTE_ALL_SHAPE_FUN_DER

	    #ifdef REGISTERS

			 coeff04 * tab_fun_u_derx[0:NSHAP] +
			 coeff14 * tab_fun_u_dery[0:NSHAP] +
			 coeff24 * tab_fun_u_derz[0:NSHAP] +
			 coeff34 * shp[0:NSHAP]

		#else

		   el_data_in[offset2+16] * tab_fun_u_derx[0:NSHAP] +
		   el_data_in[offset2+17] * tab_fun_u_dery[0:NSHAP] +
		   el_data_in[offset2+18] * tab_fun_u_derz[0:NSHAP] +
		   el_data_in[offset2+19] * shp[0:NSHAP]

	    #endif // REGISTERS

#else

		#ifdef REGISTERS

			 coeff04 * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) +
			 coeff14 * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) +
			 coeff24 * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) +
			 coeff34 * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]

		#else

		   el_data_in[offset2+16] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) +
		   el_data_in[offset2+17] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) +
		   el_data_in[offset2+18] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) +
		   el_data_in[offset2+19] * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]

		#endif // REGISTERS

#endif



	  #elif defined(HEAT)

	  #endif

				     ) * vol;


//		  if(ielem==4)
//		  {
//			  printf("igauss=%d\n",igauss);
//			  for(i=0;i<NSHAP;i++)
//			  {
//				  printf("load_vec[%d]=coeff04(%lf) * tab_fun_u_derx[i](%lf) + coeff14(%lf) * tab_fun_u_dery[i](%lf)" \
//						  " + coeff24(%lf) * tab_fun_u_derz[i](%lf) + coeff34(%lf) * shp[i](%lf)",i,coeff04,tab_fun_u_derx[i],coeff14, \
//						  tab_fun_u_dery[i],coeff24,tab_fun_u_derz[i], coeff34, shp[i]);
//				  printf("\n");
//			  }
//
//			  printf("\n");
//		  }

//		  if(ielem>=4&&ielem<8)
//		  {
//			  printf("Ielem=%d,igauss=%d, load_vec[i]=",ielem,igauss);
//			  for(i=0;i<NSHAP;i++)
//			  {
//				  printf("%lf, ",load_vec[i]);
//			  }
//			  printf("\n");
//		  }



	#endif // end if computing RHS vector

	//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
	//-------------------------------------------------------------

	 //     }//idof

		 // } // the end of using registers for u (shp_fun_u, fun_u_der.)

	//-------------------------------------------------------------
	// ************************* second loop over shape functions ****************************//

//		  if(ielem<4)
//		  {
//			  printf("ielem-%d,igauss-%d - tab_fun_u_derx:\n",ielem,igauss);
//		  }

//	  	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);  //loop invariant code motion
	  #pragma vector aligned
	  #pragma ivdep
	 // #pragma loop_count(6)
	  //#pragma simd reduction(+:stiff_mat)
	  	      for(idof = 0; idof < NSHAP; idof++){
//SCALAR tmp[NSHAP];
//idof=0;
//	  	    	if(ielem<4)
//	  	    	{
//					#ifdef COMPUTE_ALL_SHAPE_FUN_DER
//	  	    			printf("[%d] - %10.4lf\t",idof,tab_fun_u_derx[idof]);
//					#endif
//	  	    	}


//#pragma vector aligned
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

			#ifdef COMPUTE_ALL_SHAPE_FUN_DER

				tab_fun_u_derx[idof] * tab_fun_u_derx[0:NSHAP] +
				tab_fun_u_dery[idof] * tab_fun_u_dery[0:NSHAP] +
				tab_fun_u_derz[idof] * tab_fun_u_derz[0:NSHAP]

			#else //comp_all_shp

			(shape_fun_host[igauss*STRIDE+idof]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_6) * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_0+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) +
			(shape_fun_host[igauss*STRIDE+idof]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_7) * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_1+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) +
			(shape_fun_host[igauss*STRIDE+idof]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_8) * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_2+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8)

			#endif

		#elif defined(TEST_SCALAR)

			#ifdef COMPUTE_ALL_SHAPE_FUN_DER
									temp[0][idof] * tab_fun_u_derx[0:NSHAP] +
									temp[1][idof] * tab_fun_u_dery[0:NSHAP] +
									temp[2][idof] * tab_fun_u_derz[0:NSHAP] +
									temp[3][idof] * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]
			#else //comp_all_shp

								   temp[0][idof] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_0+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) +
								   temp[1][idof] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_1+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) +
								   temp[2][idof] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_2+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) +
								   temp[3][idof] * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]
			#endif

		#elif defined(HEAT)

						temp1 * fun_v_derx +
						temp2 * fun_v_dery +
						temp3 * fun_v_derz +
						temp4 * shp_fun_v

			#endif

									) * vol;

//			if(ielem==0&&igauss==0&&idof==4)
//			{
//				printf("idof=%d,temp[0][idof]=%lf x tab_fun_u_derx[0-6]=",idof,temp[0][idof]);
//				for(i=0;i<NSHAP;i++)
//					printf("%lf, ",temp[0][idof]*tab_fun_u_derx[i]);
//				printf("\n idof=%d,temp[1][idof]=%lf x tab_fun_u_dery[0-6]=",idof,temp[1][idof]);
//				for(i=0;i<NSHAP;i++)
//					printf("%lf, ",temp[1][idof]*tab_fun_u_dery[i]);
//				printf("\n idof=%d,temp[2][idof]=%lf x tab_fun_u_derz[0-6]=",idof,temp[2][idof]);
//				for(i=0;i<NSHAP;i++)
//					printf("%lf, ",temp[2][idof]*tab_fun_u_derz[i]);
//				printf("\n idof=%d,temp[3][idof]=%lf x shp[0-6]=",idof,temp[3][idof]);
//				for(i=0;i<NSHAP;i++)
//					printf("%lf, ",temp[3][idof]*shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE+i]);
//				printf(" vol=%lf",vol);
//				printf("\n");
//
//			}


//	******* THE END OF: first loop over shape functions *******//
//	-------------------------------------------------------------

	      }//idof
//	  	    if(ielem<4)
//	  	    {
//	  	      printf("\n");
//	  	    }

	//******* THE END OF: second loop over shape functions *******//
	//-------------------------------------------------------------

	    }//gauss
//
//	// ******** THE END OF: loop over integration points ********//
//	//-------------------------------------------------------------

//	    if(ielem<=4)
//	    {
//	    	int j;
//	    	printf("\nielem=%d\n",ielem);
//			for(i=0;i<NSHAP;i++)
//			{
//				for(j=0;j<NSHAP;j++)
//				{
//					printf("S[%d][%d]=%lf\t",i,j,stiff_mat[i*NSHAP+j]);
//				}
//				printf("\n");
//			}
//	    }


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
#pragma simd
    for(i = 0; i < STRIDE; i++) el_data_out[offset+one_el_stiff_mat_size+i] = load_vec[i];
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
