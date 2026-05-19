#include "mic.h"
//#include "immintrin.h"
#include<unistd.h>

#define LOCAL_STIFF

#define NGAUSS 80
#define NSHAP 75
#define NGEO 8
#define NDOFS 75
#define STRIDE 4  //stride for AVX and double
#define PAD 4

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

#ifdef TIME_TEST
    double t_begin_in = time_clock();
#endif

    printf("wlazło!-nr_elem_mic=%d,size_el_out=%d,el_in=%d,shp=%d,geo=%d,coeff=%d,stiff=%d,load=%d\n",nr_elem_mic,size_el_out,size_el_in,size_shp,geo_dat_size,nr_coeff,one_el_stiff_mat_size,one_el_load_vec_size);

		#ifdef MIC
#pragma offload target(mic:0) in(gauss_dat_host: length(1344) alloc_if(1) free_if(1)) in(shape_fun_host: length(size_shp) alloc_if(1) free_if(1)) \
    in(el_data_in: length(size_el_in) alloc_if(1) free_if(1)) out(el_data_out: length(size_el_out) alloc_if(1) free_if(1))
		#endif
{

	int iter,ielem;

#pragma vector always

	__assume_aligned(gauss_dat_host,ALIGN);
	__assume_aligned(shape_fun_host,ALIGN);
	__assume_aligned(el_data_in,ALIGN);
	__assume_aligned(el_data_out,ALIGN);

	printf("nr_elem_mic=%d\n",nr_elem_mic);


#pragma omp parallel default(none) private(ielem,iter) firstprivate(nr_elem_mic,size_el_out,size_el_in,size_shp,geo_dat_size,nr_coeff,one_el_stiff_mat_size,one_el_load_vec_size) shared(gauss_dat_host,el_data_in,el_data_out,shape_fun_host)
{
	//-------------------------------------------------------------
	//******************* loop over elements processed by a thread *********************
//__attribute__((concurrency_safe(profitable)))
#pragma ivdep
#pragma vector aligned
//#pragma loop_count (97800)
//#pragma simd
	#pragma omp for schedule(guided) nowait
    for(iter = 0; iter < nr_elem_mic/STRIDE; iter++){

    	int ielem[STRIDE] __attribute__((aligned(ALIGN)));
    	int offset[STRIDE] __attribute__((aligned(ALIGN)));
    	int off;
    	int i,j;

    	for(i=0;i<STRIDE;i++)
    		ielem[i]=STRIDE*iter+i;

    	SCALAR tab_fun_u_derx[STRIDE*NSHAP] __attribute__((aligned(ALIGN)));
    	SCALAR tab_fun_u_dery[STRIDE*NSHAP] __attribute__((aligned(ALIGN)));
    	SCALAR tab_fun_u_derz[STRIDE*NSHAP] __attribute__((aligned(ALIGN)));

//		SCALAR tab_fun_u_derx0[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derx1[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derx2[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derx3[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derx4[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derx5[STRIDE] __attribute__((aligned(ALIGN)));
//
//		SCALAR tab_fun_u_dery0[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_dery1[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_dery2[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_dery3[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_dery4[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_dery5[STRIDE] __attribute__((aligned(ALIGN)));
//
//		SCALAR tab_fun_u_derz0[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derz1[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derz2[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derz3[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derz4[STRIDE] __attribute__((aligned(ALIGN)));
//		SCALAR tab_fun_u_derz5[STRIDE] __attribute__((aligned(ALIGN)));

		#ifdef LOCAL_STIFF
			SCALAR stiff_mat0[STRIDE*NDOFS*NDOFS]  __attribute__((aligned(ALIGN)));;
//			SCALAR stiff_mat1[STRIDE*NDOFS]  __attribute__((aligned(ALIGN)));
//			SCALAR stiff_mat2[STRIDE*NDOFS]  __attribute__((aligned(ALIGN)));
//			SCALAR stiff_mat3[STRIDE*NDOFS]  __attribute__((aligned(ALIGN)));
//			SCALAR stiff_mat4[STRIDE*NDOFS]  __attribute__((aligned(ALIGN)));
//			SCALAR stiff_mat5[STRIDE*NDOFS]  __attribute__((aligned(ALIGN)));
			SCALAR load_vec[STRIDE*NDOFS]  __attribute__((aligned(ALIGN)));;
		#endif

	//-------------------------------------------------------------
	// ******************* READING INPUT DATA *********************

	    #ifdef TEST_SCALAR

			offset[:]=nr_elem_mic*geo_dat_size+ielem[:]*nr_coeff;
			//off=nr_elem_mic*geo_dat_size+iter*nr_coeff*STRIDE;

	#ifdef MIC
			SCALAR __attribute__((target(mic))) coeff00[STRIDE];
			coeff00[:]=el_data_in[off+0*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff01[STRIDE];
			coeff01[:]=el_data_in[off+1*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff02[STRIDE];
			coeff02[:]=el_data_in[off+2*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff10[STRIDE];
			coeff10[:]=el_data_in[off+3*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff11[STRIDE];
			coeff11[:]=el_data_in[off+4*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff12[STRIDE];
			coeff12[:]=el_data_in[off+5*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff20[STRIDE];
			coeff20[:]=el_data_in[off+6*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff21[STRIDE];
			coeff21[:]=el_data_in[off+7*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff22[STRIDE];
			coeff22[:]=el_data_in[off+8*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff30[STRIDE];
			coeff30[:]=el_data_in[off+9*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff31[STRIDE];
			coeff31[:]=el_data_in[off+10*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff32[STRIDE];
			coeff32[:]=el_data_in[off+11*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff03[STRIDE];
//			coeff03[:]=el_data_in[off+12*STRIDE:STRIDE];
//			SCALAR __attribute__((target(mic))) coeff13[STRIDE];
//			coeff13[:]=el_data_in[off+13*STRIDE:STRIDE];
//			SCALAR __attribute__((target(mic))) coeff23[STRIDE];
//			coeff23[:]=el_data_in[off+14*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff33[STRIDE];
			coeff33[:]=el_data_in[off+15*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff04[STRIDE];
			coeff04[:]=el_data_in[off+16*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff14[STRIDE];
			coeff14[:]=el_data_in[off+17*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff24[STRIDE];
			coeff24[:]=el_data_in[off+18*STRIDE:STRIDE];
			SCALAR __attribute__((target(mic))) coeff34[STRIDE];
			coeff34[:]=el_data_in[off+19*STRIDE:STRIDE];

	#else
//			SCALAR coeff00[STRIDE];
//			coeff00[:]=el_data_in[off+0*STRIDE:STRIDE];
//			SCALAR coeff01[STRIDE];
//			coeff01[:]=el_data_in[off+1*STRIDE:STRIDE];
//			SCALAR coeff02[STRIDE];
//			coeff02[:]=el_data_in[off+2*STRIDE:STRIDE];
//			SCALAR coeff10[STRIDE];
//			coeff10[:]=el_data_in[off+3*STRIDE:STRIDE];
//			SCALAR coeff11[STRIDE];
//			coeff11[:]=el_data_in[off+4*STRIDE:STRIDE];
//			SCALAR coeff12[STRIDE];
//			coeff12[:]=el_data_in[off+5*STRIDE:STRIDE];
//			SCALAR coeff20[STRIDE];
//			coeff20[:]=el_data_in[off+6*STRIDE:STRIDE];
//			SCALAR coeff21[STRIDE];
//			coeff21[:]=el_data_in[off+7*STRIDE:STRIDE];
//			SCALAR coeff22[STRIDE];
//			coeff22[:]=el_data_in[off+8*STRIDE:STRIDE];
//			SCALAR coeff30[STRIDE];
//			coeff30[:]=el_data_in[off+9*STRIDE:STRIDE];
//			SCALAR coeff31[STRIDE];
//			coeff31[:]=el_data_in[off+10*STRIDE:STRIDE];
//			SCALAR coeff32[STRIDE];
//			coeff32[:]=el_data_in[off+11*STRIDE:STRIDE];
//			SCALAR coeff03[STRIDE];
//			coeff03[:]=el_data_in[off+12*STRIDE:STRIDE];
//			SCALAR coeff13[STRIDE];
//			coeff13[:]=el_data_in[off+13*STRIDE:STRIDE];
//			SCALAR coeff23[STRIDE];
//			coeff23[:]=el_data_in[off+14*STRIDE:STRIDE];
//			SCALAR coeff33[STRIDE];
//			coeff33[:]=el_data_in[off+15*STRIDE:STRIDE];
//			SCALAR coeff04[STRIDE];
//			coeff04[:]=el_data_in[off+16*STRIDE:STRIDE];
//			SCALAR coeff14[STRIDE];
//			coeff14[:]=el_data_in[off+17*STRIDE:STRIDE];
//			SCALAR coeff24[STRIDE];
//			coeff24[:]=el_data_in[off+18*STRIDE:STRIDE];
//			SCALAR coeff34[STRIDE];
//			coeff34[:]=el_data_in[off+19*STRIDE:STRIDE];

			SCALAR coeff00[STRIDE];
			coeff00[:]=el_data_in[offset[:]+0];
			SCALAR coeff01[STRIDE];
			coeff01[:]=el_data_in[offset[:]+1];
			SCALAR coeff02[STRIDE];
			coeff02[:]=el_data_in[offset[:]+2];
			SCALAR coeff10[STRIDE];
			coeff10[:]=el_data_in[offset[:]+3];
			SCALAR coeff11[STRIDE];
			coeff11[:]=el_data_in[offset[:]+4];
			SCALAR coeff12[STRIDE];
			coeff12[:]=el_data_in[offset[:]+5];
			SCALAR coeff20[STRIDE];
			coeff20[:]=el_data_in[offset[:]+6];
			SCALAR coeff21[STRIDE];
			coeff21[:]=el_data_in[offset[:]+7];
			SCALAR coeff22[STRIDE];
			coeff22[:]=el_data_in[offset[:]+8];
			SCALAR coeff30[STRIDE];
			coeff30[:]=el_data_in[offset[:]+9];
			SCALAR coeff31[STRIDE];
			coeff31[:]=el_data_in[offset[:]+10];
			SCALAR coeff32[STRIDE];
			coeff32[:]=el_data_in[offset[:]+11];
			/*SCALAR coeff03[STRIDE];
			coeff03[:]=el_data_in[offset[:]+12];
			SCALAR coeff13[STRIDE];
			coeff13[:]=el_data_in[offset[:]+13];
			SCALAR coeff23[STRIDE];
			coeff23[:]=el_data_in[offset[:]+14];*/
			SCALAR coeff33[STRIDE];
			coeff33[:]=el_data_in[offset[:]+15];
			SCALAR coeff04[STRIDE];
			coeff04[:]=el_data_in[offset[:]+16];
			SCALAR coeff14[STRIDE];
			coeff14[:]=el_data_in[offset[:]+17];
			SCALAR coeff24[STRIDE];
			coeff24[:]=el_data_in[offset[:]+18];
			SCALAR coeff34[STRIDE];
			coeff34[:]=el_data_in[offset[:]+19];

	#endif
//			if(iter==0)
//			{
//				for(i=0;i<STRIDE;i++)
//				{
//					printf("coeff00[%d]=%lf,coeff01=%lf,coeff02=%lf,coeff10=%lf,coeff11=%lf,coeff12=%lf,coeff20=%lf,coeff21=%lf,coeff22=%lf,"
//							"coeff30=%lf,coeff31=%lf,coeff32=%lf,coeff03=%lf,coeff13=%lf,coeff23=%lf,coeff33=%lf,coeff04=%lf,coeff14=%lf,coeff24=%lf,coeff34=%lf\n",
//							i, coeff00[i],coeff01[i],coeff02[i],coeff10[i],coeff11[i],coeff12[i],coeff20[i],coeff21[i],coeff22[i],coeff30[i],coeff31[i],
//							coeff32[i],coeff03[i],coeff13[i],coeff23[i],coeff33[i],coeff04[i],coeff14[i],coeff24[i],coeff34[i]);
//					printf("\n");
//				}
//			}

			#endif

//coeff00=1.005556,coeff01=0.011111,coeff02=0.016667,coeff10=0.011111,coeff11=1.022222,coeff12=0.033333,coeff20=0.016667,coeff21=0.033333,coeff22=1.050000,coeff30=0.333333,coeff31=0.666667,coeff32=1.000000,coeff03=1.000000,coeff13=0.666667,coeff23=0.333333,coeff33=10.000000,coeff04=1.000000,coeff14=2.000000,coeff24=3.000000,coeff34=20.000000



	// ******* THE END OF: READING INPUT DATA *********************
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//******************** INITIALIZING SM AND LV ******************//

	#ifdef LOCAL_STIFF
#pragma ivdep
#pragma vector aligned
//#pragma simd
	for(i=0;i<NDOFS*NDOFS;i++)
		stiff_mat0[i*STRIDE:STRIDE] = zero;
//#pragma ivdep
//#pragma vector aligned
////#pragma simd
//	for(i=0;i<NDOFS;i++)
//		stiff_mat1[0:STRIDE] = zero;
//#pragma ivdep
//#pragma vector aligned
////#pragma simd
//	for(i=0;i<NDOFS;i++)
//		stiff_mat2[0:STRIDE] = zero;
//#pragma ivdep
//#pragma vector aligned
////#pragma simd
//	for(i=0;i<NDOFS;i++)
//		stiff_mat3[0:STRIDE] = zero;
//#pragma ivdep
//#pragma vector aligned
////#pragma simd
//	for(i=0;i<NDOFS;i++)
//		stiff_mat4[0:STRIDE] = zero;
//#pragma ivdep
//#pragma vector aligned
////#pragma simd
//	for(i=0;i<NDOFS;i++)
//		stiff_mat5[0:STRIDE] = zero;

	#ifdef LOAD_VEC_COMP
#pragma ivdep
#pragma vector aligned
//#pragma simd
		for(i = 0; i < NDOFS; i++)
			load_vec[i*STRIDE:STRIDE] = zero;
	#endif

	#endif

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
	      SCALAR vol[STRIDE] __attribute__((aligned(ALIGN)));
	      SCALAR volc[NSHAP*STRIDE]  __attribute__((aligned(ALIGN)));
	      vol[:]= gauss_dat_host[3*NGAUSS+igauss]; // vol = weight CONSTANT FOR LINEAR PRISM!!!
	//-------------------------------------------------------------
	//************************* JACOBIAN TERMS CALCULATIONS *************************//

	      SCALAR jac_0[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_0[:]= zero;
	      SCALAR jac_1[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_1[:]= zero;
	      SCALAR jac_2[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_2[:]= zero;
	      SCALAR jac_3[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_3[:]= zero;
	      SCALAR jac_4[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_4[:]= zero;
	      SCALAR jac_5[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_5[:]= zero;
	      SCALAR jac_6[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_6[:]= zero;
	      SCALAR jac_7[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_7[:]= zero;
	      SCALAR jac_8[STRIDE] __attribute__((aligned(ALIGN)));
	      jac_8[:]= zero;

	      // derivatives of geometrical shape functions
	      { // block to indicate the scope of jac_data

	        // derivatives of geometrical shape functions are stored in jac_data
			//#pragma vector aligned
	    	SCALAR jac_data[24]  __attribute__((aligned(ALIGN)));
			SCALAR temp1[STRIDE] __attribute__((aligned(ALIGN)));
			temp1[:] = zero;
			SCALAR temp2[STRIDE] __attribute__((aligned(ALIGN)));
			temp2[:] = zero;
			SCALAR temp3[STRIDE] __attribute__((aligned(ALIGN)));
			temp3[:] = zero;
			SCALAR temp4[STRIDE] __attribute__((aligned(ALIGN)));
			temp4[:] = zero;
			SCALAR temp5[STRIDE] __attribute__((aligned(ALIGN)));
			temp5[:] = zero;
			SCALAR temp6[STRIDE] __attribute__((aligned(ALIGN)));
			temp6[:] = zero;
			SCALAR temp7[STRIDE] __attribute__((aligned(ALIGN)));
			temp7[:] = zero;
			SCALAR temp8[STRIDE] __attribute__((aligned(ALIGN)));
			temp8[:] = zero;
			SCALAR temp9[STRIDE] __attribute__((aligned(ALIGN)));
			temp9[:] = zero;

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

		offset[0:STRIDE]=ielem[0:STRIDE]*geo_dat_size;

		//#pragma ivdep
		//#pragma vector aligned
		//#pragma loop_count(6)
		//#pragma simd
		for(i=0;i<6;i++){

		  jac_1[:] = jac_data[i];
		  jac_2[:] = jac_data[NGEO+i];
		  jac_3[:] = jac_data[2*NGEO+i];

		  jac_4[:] = el_data_in[offset[:]+3*i];  //node coor  //time is extremally small so I don't optimize this access
		  jac_5[:] = el_data_in[offset[:]+3*i+1];
		  jac_6[:] = el_data_in[offset[:]+3*i+2];

//		  if(iter==0)
//		  {
//			  printf("jac_4[0]=%lf\n",jac_4[0]);
//			  printf("jac_4[1]=%lf\n",jac_4[1]);
//			  printf("jac_4[2]=%lf\n",jac_4[2]);
//			  printf("jac_4[3]=%lf\n",jac_4[3]);
//		  }

		  temp1[:] += jac_4[:] * jac_1[:];
		  temp2[:] += jac_4[:] * jac_2[:];
		  temp3[:] += jac_4[:] * jac_3[:];
		  temp4[:] += jac_5[:] * jac_1[:];
		  temp5[:] += jac_5[:] * jac_2[:];
		  temp6[:] += jac_5[:] * jac_3[:];
		  temp7[:] += jac_6[:] * jac_1[:];
		  temp8[:] += jac_6[:] * jac_2[:];
		  temp9[:] += jac_6[:] * jac_3[:];

		}

	      jac_0[:] = (temp5[:]*temp9[:] - temp8[:]*temp6[:]);
	      jac_1[:] = (temp8[:]*temp3[:] - temp2[:]*temp9[:]);
	      jac_2[:] = (temp2[:]*temp6[:] - temp3[:]*temp5[:]);

	      SCALAR daux1[STRIDE] __attribute__((aligned(ALIGN)));
	      SCALAR faux[STRIDE] __attribute__((aligned(ALIGN)));

	      daux1[:] = temp1[:]*jac_0[:] + temp4[:]*jac_1[:] + temp7[:]*jac_2[:];

	      /* Jacobian calculations - |J| and inverse of the Jacobian matrix*/
	      vol[:] *= daux1[:]; // vol = weight * det J

	      for(i=0;i<NSHAP;i++)
	    	  volc[i*STRIDE:STRIDE]=vol[:];

	      faux[:] = one/daux1[:];

	      jac_0[:] *= faux[:];
	      jac_1[:] *= faux[:];
	      jac_2[:] *= faux[:];

	      jac_3[:] = (temp6[:]*temp7[:] - temp4[:]*temp9[:])*faux[:];
	      jac_4[:] = (temp1[:]*temp9[:] - temp7[:]*temp3[:])*faux[:];
	      jac_5[:] = (temp3[:]*temp4[:] - temp1[:]*temp6[:])*faux[:];

	      jac_6[:] = (temp4[:]*temp8[:] - temp5[:]*temp7[:])*faux[:];
	      jac_7[:] = (temp2[:]*temp7[:] - temp1[:]*temp8[:])*faux[:];
	      jac_8[:] = (temp1[:]*temp5[:] - temp2[:]*temp4[:])*faux[:];

	      } // the end of scope for jac_data

	//************* THE END OF: JACOBIAN TERMS CALCULATIONS *************************//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//

	      SCALAR tmp[NSHAP]  __attribute__((aligned(ALIGN)));
	      SCALAR shp[STRIDE*NSHAP]  __attribute__((aligned(ALIGN)));;
	      SCALAR shpx[STRIDE*NSHAP]  __attribute__((aligned(ALIGN)));;
	      SCALAR shpy[STRIDE*NSHAP]  __attribute__((aligned(ALIGN)));;
	      SCALAR shpz[STRIDE*NSHAP]  __attribute__((aligned(ALIGN)));;

#pragma vector aligned
#pragma ivdep
	      //shpx[0:STRIDE]=shape_fun_host[igauss*STRIDE:STRIDE];
	      tmp[0:NSHAP]=shape_fun_host[igauss*PAD:NSHAP];
	      for(i=0;i<NSHAP;i++)
	    	  shpx[i*STRIDE:STRIDE]=tmp[i];

#pragma vector aligned
#pragma ivdep
	      //shpy[0:STRIDE]=shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:STRIDE];
	      tmp[0:NSHAP]=shape_fun_host[PAD*NGAUSS+igauss*PAD:NSHAP];
	      for(i=0;i<NSHAP;i++)
	    	  shpy[i*STRIDE:STRIDE]=tmp[i];

#pragma vector aligned
#pragma ivdep
	      //shpz[0:STRIDE]=shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:STRIDE];
	      tmp[0:NSHAP]=shape_fun_host[2*PAD*NGAUSS+igauss*PAD:NSHAP];
	      for(i=0;i<NSHAP;i++)
	    	  shpz[i*STRIDE:STRIDE]=tmp[i];

#pragma vector aligned
#pragma ivdep
	      //shp[0:STRIDE]=shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:STRIDE];
	      tmp[0:NSHAP]=shape_fun_host[3*PAD*NGAUSS+igauss*PAD:NSHAP];
	      for(i=0;i<NSHAP;i++)
	    	  shp[i*STRIDE:STRIDE]=tmp[i];
	      //shape_fun_host[3*ngauss*PAD+ki*PAD+i] = base_phi_ref[i];
#pragma vector aligned
//#pragma ivdep
	      for(i=0;i<NSHAP;i++)
	    	  tab_fun_u_derx[i*STRIDE:STRIDE] = shpx[i*STRIDE:STRIDE]*jac_0[:]+shpy[i*STRIDE:STRIDE]*jac_3[:]+shpz[i*STRIDE:STRIDE]*jac_6[:];
#pragma vector aligned
//#pragma ivdep
	      for(i=0;i<NSHAP;i++)
	    	  tab_fun_u_dery[i*STRIDE:STRIDE] = shpx[i*STRIDE:STRIDE]*jac_1[:]+shpy[i*STRIDE:STRIDE]*jac_4[:]+shpz[i*STRIDE:STRIDE]*jac_7[:];
#pragma vector aligned
//#pragma ivdep
	      for(i=0;i<NSHAP;i++)
	    	  tab_fun_u_derz[i*STRIDE:STRIDE] = shpx[i*STRIDE:STRIDE]*jac_2[:]+shpy[i*STRIDE:STRIDE]*jac_5[:]+shpz[i*STRIDE:STRIDE]*jac_8[:];

	//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
	//-------------------------------------------------------------

	  #ifdef LAPLACE

	      // offset for reading data
	      offset[:]=nr_elem_mic*geo_dat_size+ielem[:]*nr_coeff;

	      SCALAR coeff03[STRIDE]  __attribute__((aligned(ALIGN)));
	      coeff03[:] = el_data_in[offset[:]+igauss];

	  #endif // end if LAPLACE

	#ifdef TEST_SCALAR

	      SCALAR temp0[STRIDE*NSHAP] __attribute__((aligned(ALIGN)));;
	      SCALAR temp1[STRIDE*NSHAP] __attribute__((aligned(ALIGN)));;
	      SCALAR temp2[STRIDE*NSHAP] __attribute__((aligned(ALIGN)));;
	      SCALAR temp3[STRIDE*NSHAP] __attribute__((aligned(ALIGN)));;

	#endif


	//*** THE END OF: SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS ***//
	//-------------------------------------------------------------

	#ifdef TEST_SCALAR
	      for(i=0;i<NSHAP;i++)
	      {
			  temp0[i*STRIDE:STRIDE] = coeff00[:]*tab_fun_u_derx[i*STRIDE:STRIDE] + coeff01[:]*tab_fun_u_dery[i*STRIDE:STRIDE] + coeff02[:]*tab_fun_u_derz[i*STRIDE:STRIDE];// + coeff03[:]*shp[i*STRIDE:STRIDE];
			  temp1[i*STRIDE:STRIDE] = coeff10[:]*tab_fun_u_derx[i*STRIDE:STRIDE] + coeff11[:]*tab_fun_u_dery[i*STRIDE:STRIDE] + coeff12[:]*tab_fun_u_derz[i*STRIDE:STRIDE];// + coeff13[:]*shp[i*STRIDE:STRIDE];
			  temp2[i*STRIDE:STRIDE] = coeff20[:]*tab_fun_u_derx[i*STRIDE:STRIDE] + coeff21[:]*tab_fun_u_dery[i*STRIDE:STRIDE] + coeff22[:]*tab_fun_u_derz[i*STRIDE:STRIDE];// + coeff23[:]*shp[i*STRIDE:STRIDE];
			  temp3[i*STRIDE:STRIDE] = coeff30[:]*tab_fun_u_derx[i*STRIDE:STRIDE] + coeff31[:]*tab_fun_u_dery[i*STRIDE:STRIDE] + coeff32[:]*tab_fun_u_derz[i*STRIDE:STRIDE] + coeff33[:]*shp[i*STRIDE:STRIDE];
//	      if(iter==1 && igauss==5)
//	    	  printf("temp0[i*NSHAP:0]=%lf\n",temp0[i*NSHAP+0]);
	      }


	#elif defined(HEAT)

	#endif


	//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

			//TODO offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);

	#ifdef LOAD_VEC_COMP

//#pragma vector nontemporal
			//#pragma vector always


//		  if(iter==1)
//		  {
//			  for(i=0;i<STRIDE;i++)
//			  {
//				  printf("coeff03[%d]=%lf\n",i,coeff03[i]);
//				  printf("vol=%lf\n",vol[i]);
//			  }
//			  for(i=0;i<NSHAP;i++)
//			  {
//				  for(j=0;j<STRIDE;j++)
//				  {
//					  printf("shp[%d]=%lf\n",i*STRIDE+j,shp[i*STRIDE+j]);
//				  }
//				  printf("\n");
//			  }
//
//		  }
//if(iter==1)
//{
//		  printf("igauss=%d\n",igauss);
//		  for(i=0;i<NSHAP;i++)
//		  {
//			  //printf("ielem[%d]=%d\n",i,ielem[i]);
//			  if(iter==1)
//			  {
//				  printf("load_vec[%d]=%lf\n",i*STRIDE,load_vec[i*STRIDE]);
//				  //printf("coeff03[0]=%lf * shp[%d]=%lf - vol[0]=%lf\n",coeff03[0],i*STRIDE,shp[i*STRIDE],vol[0]);
//			  }
//		  }
//}


			for(i=0;i<NSHAP;i++)
			{

	#ifdef LOCAL_STIFF
		  load_vec[i*STRIDE:STRIDE] += (
	#else
				  //TODO
		 el_data_out[offset[:]+one_el_stiff_mat_size:STRIDE] += (
		//		  tmp[:] +=(
	#endif

	  #ifdef LAPLACE

			coeff03[:] * shp[i*STRIDE:STRIDE]

	  #elif defined(TEST_SCALAR)

			 coeff04[:] * tab_fun_u_derx[i*STRIDE:STRIDE] +
			 coeff14[:] * tab_fun_u_dery[i*STRIDE:STRIDE] +
			 coeff24[:] * tab_fun_u_derz[i*STRIDE:STRIDE] +
			 coeff34[:] * shp[i*STRIDE:STRIDE]

	  #elif defined(HEAT)

	  #endif

				     ) * vol[:];

			}//i
			//ielem==4

	#endif // end if computing RHS vector

	//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// ************************* second loop over shape functions ****************************//

			//SCALAR stiff_mat0[STRIDE*NDOFS]

//#pragma vector aligned
//	  #pragma ivdep
	  	      //for(jdof = 0; jdof < NSHAP; jdof++){

			SCALAR multx[NSHAP*STRIDE]  __attribute__((aligned(ALIGN)));
			SCALAR multy[NSHAP*STRIDE]  __attribute__((aligned(ALIGN)));
			SCALAR multz[NSHAP*STRIDE]  __attribute__((aligned(ALIGN)));

	#ifdef LAPLACE
			//volc[0:NSHAP*STRIDE]=vol[:];

//			if(iter==0)
//			{
//
//				printf("iter-%d,igauss-%d - tab_fun_u_derx:\n",iter,igauss);
//
//			}

	  #pragma vector aligned
	  #pragma ivdep
	  	      for(idof = 0; idof < NSHAP; idof++){

	  	    	  for(i=0;i<NSHAP;i++)
	  	    	  {
	  	    		  multx[i*STRIDE:STRIDE]=tab_fun_u_derx[idof*STRIDE:STRIDE];
	  	    		  multy[i*STRIDE:STRIDE]=tab_fun_u_dery[idof*STRIDE:STRIDE];
	  	    		  multz[i*STRIDE:STRIDE]=tab_fun_u_derz[idof*STRIDE:STRIDE];
	  	    	  }

//	  	    	if (iter==0)
//	  	    	{
//	  	    		printf("[%d] - %10.4lf\t%10.4lf\t%10.4lf\t%10.4lf\n",idof,tab_fun_u_derx[idof*STRIDE],tab_fun_u_derx[idof*STRIDE+1],tab_fun_u_derx[idof*STRIDE+2],tab_fun_u_derx[idof*STRIDE+3]);
//	  	    	}


//#pragma vector aligned
//#pragma ivdep
//#pragma unroll(2)
	#ifdef LOCAL_STIFF
	  	 stiff_mat0[idof*STRIDE*NDOFS:NDOFS*STRIDE] += (
		//tmp[:]+=(
	#else
		 __assume_aligned(el_data_out,ALIGN);
		 el_data_out[offset+idof*NDOFS:NSHAP] += (
	#endif

			#ifdef COMPUTE_ALL_SHAPE_FUN_DER

				 multx[0:NSHAP*STRIDE] * tab_fun_u_derx[0:NDOFS*STRIDE] + //tab_fun_u_derx[idof*STRIDE:STRIDE] * tab_fun_u_derx[STRIDE:STRIDE]+tab_fun_u_derx[idof*STRIDE:STRIDE] * tab_fun_u_derx[2*STRIDE:STRIDE]+tab_fun_u_derx[idof*STRIDE:STRIDE] * tab_fun_u_derx[3*STRIDE:STRIDE]+tab_fun_u_derx[idof*STRIDE:STRIDE] * tab_fun_u_derx[4*STRIDE:STRIDE]+tab_fun_u_derx[idof*STRIDE:STRIDE] * tab_fun_u_derx[5*STRIDE:STRIDE] +
				 multy[0:NSHAP*STRIDE] * tab_fun_u_dery[0:NDOFS*STRIDE] + //tab_fun_u_dery[idof*STRIDE:STRIDE] * tab_fun_u_dery[STRIDE:STRIDE]+tab_fun_u_dery[idof*STRIDE:STRIDE] * tab_fun_u_dery[2*STRIDE:STRIDE]+tab_fun_u_dery[idof*STRIDE:STRIDE] * tab_fun_u_dery[3*STRIDE:STRIDE]+tab_fun_u_dery[idof*STRIDE:STRIDE] * tab_fun_u_dery[4*STRIDE:STRIDE]+tab_fun_u_dery[idof*STRIDE:STRIDE] * tab_fun_u_dery[5*STRIDE:STRIDE] +
				 multz[0:NSHAP*STRIDE] * tab_fun_u_derz[0:NDOFS*STRIDE] //+ tab_fun_u_derz[idof*STRIDE:STRIDE] * tab_fun_u_derz[STRIDE:STRIDE]+tab_fun_u_derz[idof*STRIDE:STRIDE] * tab_fun_u_derz[2*STRIDE:STRIDE]+tab_fun_u_derz[idof*STRIDE:STRIDE] * tab_fun_u_derz[3*STRIDE:STRIDE]+tab_fun_u_derz[idof*STRIDE:STRIDE] * tab_fun_u_derz[4*STRIDE:STRIDE]+tab_fun_u_derz[idof*STRIDE:STRIDE] * tab_fun_u_derz[5*STRIDE:STRIDE]

			#else //comp_all_shp

			(shape_fun_host[igauss*STRIDE+idof]*jac_0 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_3 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_6) * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_0+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) +
			(shape_fun_host[igauss*STRIDE+idof]*jac_1 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_4 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_7) * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_1+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) +
			(shape_fun_host[igauss*STRIDE+idof]*jac_2 + shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_5 + shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE+idof]*jac_8) * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_2+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8)

			#endif

		 ) * volc[:];

	  	      }//idof

#endif //LAPLACE

		#ifdef TEST_SCALAR

		SCALAR mult[NSHAP*STRIDE]  __attribute__((aligned(ALIGN)));

#pragma vector aligned
	  #pragma ivdep
	  	      for(idof = 0; idof < NSHAP; idof++){


	  	    	  for(i=0;i<NSHAP;i++)
				  {
					  multx[i*STRIDE:STRIDE]=temp0[idof*STRIDE:STRIDE];
					  multy[i*STRIDE:STRIDE]=temp1[idof*STRIDE:STRIDE];
					  multz[i*STRIDE:STRIDE]=temp2[idof*STRIDE:STRIDE];
					  mult[i*STRIDE:STRIDE]=temp3[idof*STRIDE:STRIDE];
				  }


			#ifdef LOCAL_STIFF
				 stiff_mat0[idof*STRIDE*NDOFS:NDOFS*STRIDE] += (
				//tmp[:]+=(
			#else
				 __assume_aligned(el_data_out,ALIGN);
				 el_data_out[offset+idof*NDOFS:NSHAP] += (
			#endif

			#ifdef COMPUTE_ALL_SHAPE_FUN_DER
							multx[0:NSHAP*STRIDE] * tab_fun_u_derx[0:NSHAP*STRIDE] +
							multy[0:NSHAP*STRIDE] * tab_fun_u_dery[0:NSHAP*STRIDE] +
							multz[0:NSHAP*STRIDE] * tab_fun_u_derz[0:NSHAP*STRIDE] +
							mult[0:NSHAP*STRIDE] * shp[0:NSHAP*STRIDE]
			#else //comp_all_shp

								   temp[0][idof] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_0+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_3+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_6) +
								   temp[1][idof] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_1+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_4+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_7) +
								   temp[2][idof] * (shape_fun_host[igauss*STRIDE:NSHAP]*jac_2+shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_5+shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]*jac_8) +
								   temp[3][idof] * shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE:NSHAP]
			#endif


									) * volc[:];


//	******* THE END OF: first loop over shape functions *******//
//	-------------------------------------------------------------

	      }//idof

#endif //test scalar

	  	      //}
//	  	    if(iter==0)
//			{
//			  printf("\n");
//			}


	//******* THE END OF: second loop over shape functions *******//
	//-------------------------------------------------------------

	    }//gauss

//
//	// ******** THE END OF: loop over integration points ********//
//	//-------------------------------------------------------------

//#define REWR_OUT //after changing it you need to change rewrite in ls_intf

#ifdef LOCAL_STIFF

	int offset2=iter*STRIDE*(one_el_stiff_mat_size+one_el_load_vec_size);
__assume_aligned(el_data_out,ALIGN);
#pragma vector aligned
#pragma ivdep
    //for(i = 0; i < NDOFS*NDOFS; i++) el_data_out[offset+i] = stiff_mat[i];

#ifdef REWR_OUT

	el_data_out[offset2:STRIDE*NDOFS*NDOFS]=stiff_mat0[0:STRIDE*NDOFS*NDOFS];

#else //rewrite locally

	    for(i=0;i<NSHAP*NSHAP;i++)
		{
	    	for(idof=0;idof<STRIDE;idof++)
				el_data_out[ielem[idof]*(one_el_stiff_mat_size+one_el_load_vec_size)+i] = stiff_mat0[i*STRIDE+idof];
//				el_data_out[ielem[1]*(one_el_stiff_mat_size+one_el_load_vec_size)+i] = stiff_mat0[i*STRIDE+1];
//				el_data_out[ielem[2]*(one_el_stiff_mat_size+one_el_load_vec_size)+i] = stiff_mat0[i*STRIDE+2];
//				el_data_out[ielem[3]*(one_el_stiff_mat_size+one_el_load_vec_size)+i] = stiff_mat0[i*STRIDE+3];
		}

#endif




	#ifdef LOAD_VEC_COMP
    __assume_aligned(el_data_out,ALIGN);
#pragma vector aligned
#pragma ivdep
//    for(i = 0; i < NSHAP; i++)
//    	el_data_out[offset[0]:STRIDE] = load_vec[i*STRIDE:STRIDE];
    //offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);

#ifdef REWR_OUT

    el_data_out[offset2+one_el_stiff_mat_size*STRIDE:STRIDE*NDOFS] = load_vec[0:STRIDE*NDOFS];

#else

    for(i=0;i<NSHAP;i++)
    {
    	for(idof=0;idof<STRIDE;idof++)
    		el_data_out[ielem[idof]*(one_el_stiff_mat_size+one_el_load_vec_size)+one_el_stiff_mat_size+i] = load_vec[i*STRIDE+idof];
//    	el_data_out[ielem[1]*(one_el_stiff_mat_size+one_el_load_vec_size)+one_el_stiff_mat_size+i] = load_vec[i*STRIDE+1];
//    	el_data_out[ielem[2]*(one_el_stiff_mat_size+one_el_load_vec_size)+one_el_stiff_mat_size+i] = load_vec[i*STRIDE+2];
//    	el_data_out[ielem[3]*(one_el_stiff_mat_size+one_el_load_vec_size)+one_el_stiff_mat_size+i] = load_vec[i*STRIDE+3];
    }

#endif

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
