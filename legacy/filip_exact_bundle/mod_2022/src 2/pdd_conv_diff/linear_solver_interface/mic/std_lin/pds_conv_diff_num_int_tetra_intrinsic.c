#include "mic.h"
#include "immintrin.h"

#define LOCAL_STIFF

//for Haswell
//#define FMA

#define NGAUSS 4
#define NSHAP 4
#define NGEO 4
#define NDOFS 4
#define STRIDE 4

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


		register __m256d tab_fun_u_derx1;
		register __m256d tab_fun_u_dery1;
		register __m256d tab_fun_u_derz1;

		#ifdef LOCAL_STIFF

			register __m256d stiff_mat1a;
			register __m256d stiff_mat2a;
			register __m256d stiff_mat3a;
			register __m256d stiff_mat4a;

			register __m256d load_vec1;
		#endif


			int i;

		//-------------------------------------------------------------
		// ******************* READING INPUT DATA *********************

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

  		    stiff_mat1a=_mm256_setzero_pd();

  			stiff_mat2a=_mm256_setzero_pd();

  			stiff_mat3a=_mm256_setzero_pd();

  			stiff_mat4a=_mm256_setzero_pd();

  			#ifdef LOAD_VEC_COMP

  		    	load_vec1=_mm256_setzero_pd();

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
    for(igauss = 0; igauss < NGAUSS; igauss++){

	//-------------------------------------------------------------
	//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//

	      register __m256d shp1;
	      register __m256d shpx1;
		  register __m256d shpy1;
		  register __m256d shpz1;

		  shp1=_mm256_load_pd(&shape_fun_host[3*STRIDE*NGAUSS+igauss*STRIDE]);

		  shpx1=_mm256_load_pd(&shape_fun_host[igauss*STRIDE]);
		  shpy1=_mm256_load_pd(&shape_fun_host[STRIDE*NGAUSS+igauss*STRIDE]);
		  shpz1=_mm256_load_pd(&shape_fun_host[2*STRIDE*NGAUSS+igauss*STRIDE]);

		  register __m256d tmp1;
		  register __m256d tmp2;
		  register __m256d tmp3;

		  tmp1=_mm256_set1_pd(jac_0);
		  tmp2=_mm256_set1_pd(jac_1);
		  tmp3=_mm256_set1_pd(jac_2);

		  tab_fun_u_derx1 = _mm256_mul_pd(shpx1,tmp1);
		  tab_fun_u_dery1 = _mm256_mul_pd(shpx1,tmp2);
		  tab_fun_u_derz1 = _mm256_mul_pd(shpx1,tmp3);

		  tmp1=_mm256_set1_pd(jac_3);
		  tmp2=_mm256_set1_pd(jac_4);
		  tmp3=_mm256_set1_pd(jac_5);

#ifdef FMA
		  tab_fun_u_derx1=_mm256_fmadd_pd(shpy1,tmp1,tab_fun_u_derx1);
#else
		  shpx1 = _mm256_mul_pd(shpy1,tmp1);
		  tab_fun_u_derx1 = _mm256_add_pd(shpx1,tab_fun_u_derx1);
#endif

#ifdef FMA
		  tab_fun_u_dery1=_mm256_fmadd_pd(shpy1,tmp2,tab_fun_u_dery1);
#else
		  shpx1 = _mm256_mul_pd(shpy1,tmp2);
		  tab_fun_u_dery1 = _mm256_add_pd(shpx1,tab_fun_u_dery1);
#endif

#ifdef FMA
		  tab_fun_u_derz1=_mm256_fmadd_pd(shpy1,tmp3,tab_fun_u_derz1);
#else
		  shpx1 = _mm256_mul_pd(shpy1,tmp3);
		  tab_fun_u_derz1 = _mm256_add_pd(shpx1,tab_fun_u_derz1);
#endif
		  tmp1=_mm256_set1_pd(jac_6);
		  tmp2=_mm256_set1_pd(jac_7);
		  tmp3=_mm256_set1_pd(jac_8);
#ifdef FMA
		  tab_fun_u_derx1=_mm256_fmadd_pd(shpz1,tmp1,tab_fun_u_derx1);
#else
		  shpx1 = _mm256_mul_pd(shpz1,tmp1);
		  tab_fun_u_derx1 = _mm256_add_pd(shpx1,tab_fun_u_derx1);
#endif

#ifdef FMA
		  tab_fun_u_dery1=_mm256_fmadd_pd(shpz1,tmp2,tab_fun_u_dery1);
#else
		  shpx1 = _mm256_mul_pd(shpz1,tmp2);
		  tab_fun_u_dery1 = _mm256_add_pd(shpx1,tab_fun_u_dery1);
#endif

#ifdef FMA
		  tab_fun_u_derz1=_mm256_fmadd_pd(shpz1,tmp3,tab_fun_u_derz1);
#else
		  shpx1 = _mm256_mul_pd(shpz1,tmp3);
		  tab_fun_u_derz1 = _mm256_add_pd(shpx1,tab_fun_u_derz1);
#endif


	//*** THE END OF: SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS ***//
	//-------------------------------------------------------------


//-------------------------------------------------------------
//***** SUBSTITUTING ACTUAL COEFFICIENTS FOR SM AND LV CALCULATIONS *****//

#ifdef LAPLACE

      // offset for reading data
      offset=nr_elem_mic*geo_dat_size+ielem*nr_coeff;

      register __m256d coeff03;
      coeff03 = _mm256_set1_pd(el_data_in[offset+igauss]);

#endif // end if LAPLACE

#ifdef TEST_SCALAR

      //SCALAR temp[4][NSHAP] __attribute__((aligned(ALIGN)));
      register __m256d temp0a;
      register __m256d temp1a;
	  register __m256d temp2a;
	  register __m256d temp3a;

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

#endif

//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
//-------------------------------------------------------------

//-------------------------------------------------------------
//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

		//-------------------------------------------------------------
		//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

				offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);

		#ifdef LOAD_VEC_COMP

			#ifdef LAPLACE

				shp1=_mm256_mul_pd(coeff03,shp1);
				coeff03 = _mm256_set1_pd(vol);   //reuse to save registers
			#ifdef FMA
				load_vec1=_mm256_fmadd_pd(coeff03,shp1,load_vec1);
			#else
				shp1=_mm256_mul_pd(coeff03,shp1);
				load_vec1=_mm256_add_pd(load_vec1,shp1);
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

			#endif

		#endif // end if computing RHS vector

//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
//-------------------------------------------------------------

//	  } // the end of using registers for u (shp_fun_u, fun_u_der.)

				//-------------------------------------------------------------
				// ************************* second loop over shape functions ****************************//


#ifdef LAPLACE

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


#endif

#ifdef TEST_SCALAR

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
				_mm256_store_pd (&el_data_out[offset+NDOFS],stiff_mat2a);
				_mm256_store_pd (&el_data_out[offset+2*NDOFS],stiff_mat3a);
				_mm256_store_pd (&el_data_out[offset+3*NDOFS],stiff_mat4a);

			  #ifdef LOAD_VEC_COMP
			    __assume_aligned(el_data_out,ALIGN);
			    _mm256_store_pd (&el_data_out[offset+one_el_stiff_mat_size],load_vec1);
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
