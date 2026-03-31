#include "mic.h"

//#define LOCAL_STIFF
#define VECTORIZE

int pdr_num_int_el_QSS_prism(
		SCALAR* gauss_dat_host, // integration points data of elements having given p
		SCALAR* shape_fun_host, // shape functions on a reference element
		SCALAR* el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
		SCALAR* el_data_out, // result of integration of NR_ELEMS_THIS_KERCALL elements
		const int nreq,
		const int num_gauss,
		const int num_shap,
		const int num_geo_dofs,
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
const int chunk_size=nr_elem_mic/24;

		#ifdef MIC
#pragma offload target(mic:0) in(gauss_dat_host: length(1344) alloc_if(1) free_if(1)) in(shape_fun_host: length(size_shp) alloc_if(1) free_if(1)) \
    in(el_data_in: length(size_el_in) alloc_if(1) free_if(1)) out(el_data_out: length(size_el_out) alloc_if(1) free_if(1))
		#endif
{

	const int num_dofs=num_shap*nreq;

	int ielem;
#ifdef MIC
	register int __attribute__((target(mic))) offset;
#else
	register int offset;
#endif

	__assume_aligned(gauss_dat_host,ALIGN);
	__assume_aligned(shape_fun_host,ALIGN);
	__assume_aligned(el_data_in,ALIGN);
	__assume_aligned(el_data_out,ALIGN);

#pragma omp parallel private(offset,ielem) firstprivate(num_dofs,nreq,num_gauss,num_shap,num_geo_dofs,nr_elem_mic,size_el_out,size_el_in,size_shp,geo_dat_size,nr_coeff,one_el_stiff_mat_size,one_el_load_vec_size)
{
	//-------------------------------------------------------------
	//******************* loop over elements processed by a thread *********************
#pragma ivdep
#pragma vector aligned
	#pragma omp for schedule(guided) nowait
    for(ielem = 0; ielem < nr_elem_mic; ielem++){

		#ifdef COMPUTE_ALL_SHAPE_FUN_DER
			//register SCALAR *workspace = (SCALAR *) malloc((geo_dat_size+nr_coeff)*sizeof(SCALAR));

			#ifdef VECTORIZE
    			SCALAR *tab_fun_u_der = (SCALAR *) _mm_malloc(3*num_shap*sizeof(SCALAR),ALIGN);
			#else
				SCALAR *tab_fun_u_derx = (SCALAR *) _mm_malloc(num_shap*sizeof(SCALAR),ALIGN);
				SCALAR *tab_fun_u_dery = (SCALAR *) _mm_malloc(num_shap*sizeof(SCALAR),ALIGN);
				SCALAR *tab_fun_u_derz = (SCALAR *) _mm_malloc(num_shap*sizeof(SCALAR),ALIGN);
			#endif
		#endif

		#ifdef LOCAL_STIFF
			__declspec(align(ALIGN)) SCALAR stiff_mat[num_dofs*num_dofs];
			__declspec(align(ALIGN)) SCALAR load_vec[num_dofs];
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
	    		#pragma simd
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
#pragma simd
		    for(i = 0; i < num_dofs*num_dofs; i++) stiff_mat[i] = zero;

			  #ifdef LOAD_VEC_COMP
#pragma ivdep
#pragma vector aligned
#pragma simd
			    for(i = 0; i < num_dofs; i++) load_vec[i] = zero;
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
	    for(igauss = 0; igauss < num_gauss; igauss++){


	      // integration data read from cached constant or shared  memory
//#ifdef VECTORIZE
//	   __declspec(align(ALIGN)) SCALAR aux[3];
//
//		#pragma simd
//	   	   for(i=0;i<3;i++)
//	   	   {
//	   		   aux[i]=gauss_dat_host[4*igauss+i];
//	   	   }
//
//
//#else
//	      SCALAR daux = gauss_dat_host[4*igauss];
//	      SCALAR faux = gauss_dat_host[4*igauss+1];
//	      SCALAR eaux = gauss_dat_host[4*igauss+2];
//	      //SCALAR vol = gauss_dat_host[4*igauss+3]; // vol = weight
//
//#endif
	      SCALAR vol = weight_linear_prism; // vol = weight CONSTANT FOR LINEAR PRISM!!!
	//-------------------------------------------------------------
	//************************* JACOBIAN TERMS CALCULATIONS *************************//

	      // when geometrical shape functions are not necessary
	      // (only derivatives are used for Jacobian calculations)

#ifdef VECTORIZE
	   __declspec(align(ALIGN)) SCALAR temp[9];
		#pragma vector aligned
	   #pragma simd
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
	   	   #pragma simd
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
	    	__declspec(align(ALIGN)) SCALAR jac_data[3*num_geo_dofs];


#ifdef VECTORIZE
		jac_data[0] = -(one-gauss_dat_host[4*igauss+2])*half;
		jac_data[1] =  (one-gauss_dat_host[4*igauss+2])*half;
		jac_data[2] =  zero;
		jac_data[3] = -(one+gauss_dat_host[4*igauss+2])*half;
		jac_data[4] =  (one+gauss_dat_host[4*igauss+2])*half;
		jac_data[5] =  zero;
		jac_data[6] = -(one-gauss_dat_host[4*igauss+2])*half;
		jac_data[7] =  zero;
		jac_data[8] =  (one-gauss_dat_host[4*igauss+2])*half;
		jac_data[9] = -(one+gauss_dat_host[4*igauss+2])*half;
		jac_data[10] =  zero;
		jac_data[11] =  (one+gauss_dat_host[4*igauss+2])*half;
		jac_data[12] = -(one-gauss_dat_host[4*igauss]-gauss_dat_host[4*igauss+1])*half;
		jac_data[13] = -gauss_dat_host[4*igauss]*half;
		jac_data[14] = -gauss_dat_host[4*igauss+1]*half;
		jac_data[15] =  (one-gauss_dat_host[4*igauss]-gauss_dat_host[4*igauss+1])*half;
		jac_data[16] =  gauss_dat_host[4*igauss]*half;
		jac_data[17] =  gauss_dat_host[4*igauss+1]*half;
#else
	    jac_data[0] = -(one-gauss_dat_host[4*igauss+2])*half;
		jac_data[1] =  (one-gauss_dat_host[4*igauss+2])*half;
		jac_data[2] =  zero;
		jac_data[3] = -(one+gauss_dat_host[4*igauss+2])*half;
		jac_data[4] =  (one+gauss_dat_host[4*igauss+2])*half;
		jac_data[5] =  zero;
		jac_data[6] = -(one-gauss_dat_host[4*igauss+2])*half;
		jac_data[7] =  zero;
		jac_data[8] =  (one-gauss_dat_host[4*igauss+2])*half;
		jac_data[9] = -(one+gauss_dat_host[4*igauss+2])*half;
		jac_data[10] =  zero;
		jac_data[11] =  (one+gauss_dat_host[4*igauss+2])*half;
		jac_data[12] = -(one-gauss_dat_host[4*igauss]-gauss_dat_host[4*igauss+1])*half;
		jac_data[13] = -gauss_dat_host[4*igauss]*half;
		jac_data[14] = -gauss_dat_host[4*igauss+1]*half;
		jac_data[15] =  (one-gauss_dat_host[4*igauss]-gauss_dat_host[4*igauss+1])*half;
		jac_data[16] =  gauss_dat_host[4*igauss]*half;
		jac_data[17] =  gauss_dat_host[4*igauss+1]*half;
#endif

		/* Jacobian matrix J */

		offset=ielem*geo_dat_size;
		#pragma simd
		#pragma ivdep
		#pragma vector aligned
		for(i=0;i<num_geo_dofs;i++){

#ifdef VECTORIZE
//		  jac[1] = jac_data[i];
//		  jac[2] = jac_data[num_geo_dofs+i];
//		  jac[3] = jac_data[2*num_geo_dofs+i];

//		  jac[4] = el_data_in[offset+3*i];  //node coor
//		  jac[5] = el_data_in[offset+3*i+1];
//		  jac[6] = el_data_in[offset+3*i+2];

		  //printf("el_data_in_geo_inside[%d]=%lf\n",offset+3*i,el_data_in[offset+3*i]);  //ok

		  temp[0] += el_data_in[offset+3*i] * jac_data[i];
		  temp[1] += el_data_in[offset+3*i] * jac_data[num_geo_dofs+i];
		  temp[2] += el_data_in[offset+3*i] * jac_data[2*num_geo_dofs+i];
		  temp[3] += el_data_in[offset+3*i+1] * jac_data[i];
		  temp[4] += el_data_in[offset+3*i+1] * jac_data[num_geo_dofs+i];
		  temp[5] += el_data_in[offset+3*i+1] * jac_data[2*num_geo_dofs+i];
		  temp[6] += el_data_in[offset+3*i+2] * jac_data[i];
		  temp[7] += el_data_in[offset+3*i+2] * jac_data[num_geo_dofs+i];
		  temp[8] += el_data_in[offset+3*i+2] * jac_data[2*num_geo_dofs+i];
#else

//		  jac_1 = jac_data[i];
//		  jac_2 = jac_data[num_geo_dofs+i];
//		  jac_3 = jac_data[2*num_geo_dofs+i];
//
//		  jac_4 = el_data_in[offset+3*i];  //node coor
//		  jac_5 = el_data_in[offset+3*i+1];
//		  jac_6 = el_data_in[offset+3*i+2];

		  //printf("el_data_in_geo_inside[%d]=%lf\n",offset+3*i,el_data_in[offset+3*i]);  //ok

//		  temp1 += jac_4 * jac_1;
//		  temp2 += jac_4 * jac_2;
//		  temp3 += jac_4 * jac_3;
//		  temp4 += jac_5 * jac_1;
//		  temp5 += jac_5 * jac_2;
//		  temp6 += jac_5 * jac_3;
//		  temp7 += jac_6 * jac_1;
//		  temp8 += jac_6 * jac_2;
//		  temp9 += jac_6 * jac_3;
		  temp1 += el_data_in[offset+3*i] * jac_data[i];
		  temp2 += el_data_in[offset+3*i] * jac_data[num_geo_dofs+i];
		  temp3 += el_data_in[offset+3*i] * jac_data[2*num_geo_dofs+i];
		  temp4 += el_data_in[offset+3*i+1] * jac_data[i];
		  temp5 += el_data_in[offset+3*i+1] * jac_data[num_geo_dofs+i];
		  temp6 += el_data_in[offset+3*i+1] * jac_data[2*num_geo_dofs+i];
		  temp7 += el_data_in[offset+3*i+2] * jac_data[i];
		  temp8 += el_data_in[offset+3*i+2] * jac_data[num_geo_dofs+i];
		  temp9 += el_data_in[offset+3*i+2] * jac_data[2*num_geo_dofs+i];
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

	      SCALAR daux = temp1*jac_0 + temp4*jac_1 + temp7*jac_2;

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

#endif
	//************* THE END OF: JACOBIAN TERMS CALCULATIONS *************************//
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	//***** SEPARATE COMPUTING OF ALL GLOBAL DERIVATIVES OF ALL SHAPE FUNCTIONS *****//

	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

	 //************ loop for computing ALL shape function values at integration point **********//
#pragma vector aligned
#pragma ivdep
//#pragma simd
	      for(idof = 0; idof < num_shap; idof++){

#ifdef VECTORIZE
		// read proper values of shape functions and their derivatives
//		#pragma simd
//	    for(i=0;i<3;i++)
//	    {
//	    	temp[i]=shape_fun_host[igauss*4*num_shap+4*idof+i+1];
//	    	//if(ielem==0)
//	    	//printf("igauss=%d,idof=%d,temp[%d]=%lf\n",igauss,idof,i,temp[i]);
//	    }

		#pragma simd
	    for(i=0;i<3;i++)
	    {
	    	tab_fun_u_der[3*idof+i] = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac[i]+shape_fun_host[igauss*4*num_shap+4*idof+2]*jac[i+3]+shape_fun_host[igauss*4*num_shap+4*idof+3]*jac[i+6];
	    	//if(ielem==0)
	    	//printf("igauss=%d,idof=%d,tab_fun_u_der[%d]=%lf\n",igauss,idof,3*idof+i,tab_fun_u_der[3*idof+i]);
	    }

//		tab_fun_u_der[3*idof] = temp[0]*jac[0]+temp[1]*jac[3]+temp[2]*jac[6];
//		tab_fun_u_der[3*idof+1] = temp[0]*jac[1]+temp[1]*jac[4]+temp[2]*jac[7];
//		tab_fun_u_der[3*idof+2] = temp[0]*jac[2]+temp[1]*jac[5]+temp[2]*jac[8];

#else

		// read proper values of shape functions and their derivatives
//		temp1 = shape_fun_host[igauss*4*num_shap+4*idof+1];
//		temp2 = shape_fun_host[igauss*4*num_shap+4*idof+2];
//		temp3 = shape_fun_host[igauss*4*num_shap+4*idof+3];
		//if(ielem==0)
		//printf("igauss=%d,idof=%d,temp1,2,3=%lf,%lf,%lf\n",igauss,idof,temp1,temp2,temp3);

		tab_fun_u_derx[idof] = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac_0+shape_fun_host[igauss*4*num_shap+4*idof+2]*jac_3+shape_fun_host[igauss*4*num_shap+4*idof+3]*jac_6;
		tab_fun_u_dery[idof] = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac_1+shape_fun_host[igauss*4*num_shap+4*idof+2]*jac_4+shape_fun_host[igauss*4*num_shap+4*idof+3]*jac_7;
		tab_fun_u_derz[idof] = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac_2+shape_fun_host[igauss*4*num_shap+4*idof+2]*jac_5+shape_fun_host[igauss*4*num_shap+4*idof+3]*jac_8;
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
//#pragma simd
	      for(idof = 0; idof < num_shap; idof++){

		{ // beginning of using registers for u  (shp_fun_u, fun_u_der.)

	//-------------------------------------------------------------
	//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ******//

	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE
			  // read proper values of shape functions and their derivatives
			__declspec(align(ALIGN)) SCALAR fun_u_der[3];
			  //SCALAR shp_fun_u = shape_fun_host[igauss*4*num_shap+4*idof];

//			#pragma vector aligned
//			#pragma ivdep
			  #pragma simd
			  for(i=0;i<3;i++)
				  fun_u_der[i]=tab_fun_u_der[3*idof+i];
#else
	  		  // read proper values of shape functions and their derivatives
	          //SCALAR shp_fun_u = shape_fun_host[igauss*4*num_shap+4*idof];
	          SCALAR fun_u_derx = tab_fun_u_derx[idof];
	          SCALAR fun_u_dery = tab_fun_u_dery[idof];
	          SCALAR fun_u_derz = tab_fun_u_derz[idof];
#endif//vect
	#else // if not COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE

			// read proper values of shape functions and their derivatives
	        __declspec(align(ALIGN)) SCALAR fun_u_der[3];
	        //SCALAR shp_fun_u = shape_fun_host[igauss*4*num_shap+4*idof];

//#pragma vector aligned
//#pragma ivdep
//			#pragma simd
//			for(i=0;i<3;i++)
//				temp[i]=shape_fun_host[igauss*4*num_shap+4*idof+i+1];
#pragma vector aligned
#pragma ivdep
			#pragma simd
			for(i=0;i<3;i++)
				fun_u_der[i] = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac[i]+shape_fun_host[igauss*4*num_shap+4*idof+2]*jac[i+3]+shape_fun_host[igauss*4*num_shap+4*idof+3]*jac[i+6];

#else
	          // read proper values of shape functions and their derivatives
//			  SCALAR shp_fun_u = shape_fun_host[igauss*4*num_shap+4*idof];
//			  temp1 = shape_fun_host[igauss*4*num_shap+4*idof+1];
//			  temp2 = shape_fun_host[igauss*4*num_shap+4*idof+2];
//			  temp3 = shape_fun_host[igauss*4*num_shap+4*idof+3];


			  // compute derivatives wrt global coordinates
			  // 15 operations
			  SCALAR fun_u_derx = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac_0 + shape_fun_host[igauss*4*num_shap+4*idof+2]*jac_3 + shape_fun_host[igauss*4*num_shap+4*idof+3]*jac_6;
			  SCALAR fun_u_dery = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac_1 + shape_fun_host[igauss*4*num_shap+4*idof+2]*jac_4 + shape_fun_host[igauss*4*num_shap+4*idof+3]*jac_7;
			  SCALAR fun_u_derz = shape_fun_host[igauss*4*num_shap+4*idof+1]*jac_2 + shape_fun_host[igauss*4*num_shap+4*idof+2]*jac_5 + shape_fun_host[igauss*4*num_shap+4*idof+3]*jac_8;
#endif//vect

	#endif // COMPUTE_ALL_SHAPE_FUN_DER

	//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//

			  //TODO sprawdzic czy kolejnosc indeksow ma znacznie zaminic uzycie temp123 z temp4567 od tego miejsca
#ifdef VECTORIZE

	#ifdef LAPLACE

		#pragma simd
			  for(i=0;i<3;i++)
				  temp[3+i]=fun_u_der[i];

	#elif defined(TEST_SCALAR)

		#ifdef REGISTERS

	  	  temp[3] = coeff[0]*fun_u_der[0] + coeff[1]*fun_u_der[1] + coeff[2]*fun_u_der[2] + coeff[12]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp[4] = coeff[3]*fun_u_der[0] + coeff[4]*fun_u_der[1] + coeff[5]*fun_u_der[2] + coeff[13]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp[5] = coeff[6]*fun_u_der[0] + coeff[7]*fun_u_der[1] + coeff[8]*fun_u_der[2] + coeff[14]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp[6] = coeff[9]*fun_u_der[0] + coeff[10]*fun_u_der[1] + coeff[11]*fun_u_der[2] + coeff[15]*shape_fun_host[igauss*4*num_shap+4*idof];

		#else

		  register int offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;

		  temp[3] = el_data_in[offset2+0]*fun_u_der[0] + el_data_in[offset2+1]*fun_u_der[1] + el_data_in[offset2+2]*fun_u_der[2] + el_data_in[offset2+12]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp[4] = el_data_in[offset2+3]*fun_u_der[0] + el_data_in[offset2+4]*fun_u_der[1] + el_data_in[offset2+5]*fun_u_der[2] + el_data_in[offset2+13]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp[5] = el_data_in[offset2+6]*fun_u_der[0] + el_data_in[offset2+7]*fun_u_der[1] + el_data_in[offset2+8]*fun_u_der[2] + el_data_in[offset2+14]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp[6] = el_data_in[offset2+9]*fun_u_der[0] + el_data_in[offset2+10]*fun_u_der[1] + el_data_in[offset2+11]*fun_u_der[2] + el_data_in[offset2+15]*shape_fun_host[igauss*4*num_shap+4*idof];

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

	  	  temp4 = coeff00*fun_u_derx + coeff01*fun_u_dery + coeff02*fun_u_derz + coeff03*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp5 = coeff10*fun_u_derx + coeff11*fun_u_dery + coeff12*fun_u_derz + coeff13*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp6 = coeff20*fun_u_derx + coeff21*fun_u_dery + coeff22*fun_u_derz + coeff23*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp7 = coeff30*fun_u_derx + coeff31*fun_u_dery + coeff32*fun_u_derz + coeff33*shape_fun_host[igauss*4*num_shap+4*idof];

		#else

		  register int offset2=geo_dat_size*nr_elem_mic+ielem*nr_coeff;

		  temp4 = el_data_in[offset2+0]*fun_u_derx + el_data_in[offset2+1]*fun_u_dery + el_data_in[offset2+2]*fun_u_derz + el_data_in[offset2+12]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp5 = el_data_in[offset2+3]*fun_u_derx + el_data_in[offset2+4]*fun_u_dery + el_data_in[offset2+5]*fun_u_derz + el_data_in[offset2+13]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp6 = el_data_in[offset2+6]*fun_u_derx + el_data_in[offset2+7]*fun_u_dery + el_data_in[offset2+8]*fun_u_derz + el_data_in[offset2+14]*shape_fun_host[igauss*4*num_shap+4*idof];
		  temp7 = el_data_in[offset2+9]*fun_u_derx + el_data_in[offset2+10]*fun_u_dery + el_data_in[offset2+11]*fun_u_derz + el_data_in[offset2+15]*shape_fun_host[igauss*4*num_shap+4*idof];

		#endif

	#elif defined(HEAT)

	#endif

#endif//vectorize

	//*** THE END OF: ACTUAL INTERMEDIATE CALCULATIONS FOR IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//*** ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//

//		  printf("stiff_mat_out_rhs[%d]=%lf\n",offset+num_dofs*num_dofs+idof,el_data_out[offset+num_dofs*num_dofs+idof]);


	#ifdef LOAD_VEC_COMP
//#pragma vector nontemporal
	#ifdef LOCAL_STIFF
		  load_vec[idof] += (
	#else
		 el_data_out[offset+one_el_stiff_mat_size+idof] += (
	#endif

	  #ifdef LAPLACE

		#ifdef REGISTERS

				     coeff03 * shape_fun_host[igauss*4*num_shap+4*idof]

		#else
				     el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+igauss] * shape_fun_host[igauss*4*num_shap+4*idof]

		#endif


	  #elif defined(TEST_SCALAR)

#ifdef VECTORIZE

		#ifdef REGISTERS

			 coeff[16] * fun_u_der[0] +
			 coeff[17] * fun_u_der[1] +
			 coeff[18] * fun_u_der[2] +
			 coeff[19] * shape_fun_host[igauss*4*num_shap+4*idof]

		#else

		   el_data_in[offset2+16] * fun_u_der[0] +
		   el_data_in[offset2+17] * fun_u_der[1] +
		   el_data_in[offset2+18] * fun_u_der[2] +
		   el_data_in[offset2+19] * shape_fun_host[igauss*4*num_shap+4*idof]

		#endif // REGISTERS

#else

	    #ifdef REGISTERS

			 coeff04 * fun_u_derx +
			 coeff14 * fun_u_dery +
			 coeff24 * fun_u_derz +
			 coeff34 * shape_fun_host[igauss*4*num_shap+4*idof]

		#else

		   el_data_in[offset2+16] * fun_u_derx +
		   el_data_in[offset2+17] * fun_u_dery +
		   el_data_in[offset2+18] * fun_u_derz +
		   el_data_in[offset2+19] * shape_fun_host[igauss*4*num_shap+4*idof]

	    #endif // REGISTERS

#endif //vectorize

	  #elif defined(HEAT)

	  #endif

				     ) * vol;

		 //printf("stiff_mat_out_rhs[%d]=%lf\n",offset+num_dofs*num_dofs+idof,el_data_out[offset+num_dofs*num_dofs+idof]);

		 //printf("el_data_in[%d]=%lf\n",nr_elem_mic*geo_dat_size+ielem*nr_coeff+idof,el_data_in[nr_elem_mic*geo_dat_size+ielem*nr_coeff+idof]); //ok
		 //printf("shp_fun_u=%lf\n",shp_fun_u);

	#endif // end if computing RHS vector

	//*** THE END OF: ACTUAL CALCULATIONS FOR LOAD VECTOR (AND IDOF SHAPE FUNCTION) ***//
	//-------------------------------------------------------------

		  } // the end of using registers for u (shp_fun_u, fun_u_der.)

	//-------------------------------------------------------------
	// ************************* second loop over shape functions ****************************//
#pragma ivdep
#pragma vector aligned
//#pragma simd
	        for(jdof = 0; jdof < num_shap; jdof++){

	//-------------------------------------------------------------
	//****** SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF JDOF SHAPE FUNCTION ******//

	#ifdef COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE
			  // read proper values of shape functions and their derivatives
			__declspec(align(ALIGN)) SCALAR fun_v_der[3];
			  //SCALAR shp_fun_v = shape_fun_host[igauss*4*num_shap+4*jdof];
#pragma vector aligned
#pragma ivdep
			  #pragma simd
			  for(i=0;i<3;i++)
				  fun_v_der[i]=tab_fun_u_der[3*jdof+i];
#else
	  		  // read proper values of shape functions and their derivatives
			  //SCALAR shp_fun_v = shape_fun_host[igauss*4*num_shap+4*jdof];
			  SCALAR fun_v_derx = tab_fun_u_derx[jdof];
			  SCALAR fun_v_dery = tab_fun_u_dery[jdof];
			  SCALAR fun_v_derz = tab_fun_u_derz[jdof];
#endif//vect

	#else // if not COMPUTE_ALL_SHAPE_FUN_DER

#ifdef VECTORIZE
//to lepiej zrobic -wszystko do wektora!!
			// read proper values of shape functions and their derivatives
	        __declspec(align(ALIGN)) SCALAR fun_v_der[3];
	        //SCALAR shp_fun_v = shape_fun_host[igauss*4*num_shap+4*jdof];

//			#pragma simd
//			for(i=0;i<3;i++)
//				temp[i]=shape_fun_host[igauss*4*num_shap+4*jdof+i+1];
#pragma vector aligned
#pragma ivdep
			#pragma simd
			for(i=0;i<3;i++)
				fun_v_der[i] = shape_fun_host[igauss*4*num_shap+4*jdof+1]*jac[i]+shape_fun_host[igauss*4*num_shap+4*jdof+2]*jac[i+3]+shape_fun_host[igauss*4*num_shap+4*jdof+3]*jac[i+6];

#else

		// read proper values of shape functions and their derivatives
		//SCALAR shp_fun_v = shape_fun_host[igauss*4*num_shap+4*jdof];
//		temp1 = shape_fun_host[igauss*4*num_shap+4*jdof+1];
//		temp2 = shape_fun_host[igauss*4*num_shap+4*jdof+2];
//		temp3 = shape_fun_host[igauss*4*num_shap+4*jdof+3];

		// compute derivatives wrt global coordinates
		// 15 operations
		SCALAR fun_v_derx = shape_fun_host[igauss*4*num_shap+4*jdof+1]*jac_0 + shape_fun_host[igauss*4*num_shap+4*jdof+2]*jac_3 + shape_fun_host[igauss*4*num_shap+4*jdof+3]*jac_6;
		SCALAR fun_v_dery = shape_fun_host[igauss*4*num_shap+4*jdof+1]*jac_1 + shape_fun_host[igauss*4*num_shap+4*jdof+2]*jac_4 + shape_fun_host[igauss*4*num_shap+4*jdof+3]*jac_7;
		SCALAR fun_v_derz = shape_fun_host[igauss*4*num_shap+4*jdof+1]*jac_2 + shape_fun_host[igauss*4*num_shap+4*jdof+2]*jac_5 + shape_fun_host[igauss*4*num_shap+4*jdof+3]*jac_8;

#endif//vectorize

	#endif // end if not COMPUTE_ALL_SHAPE_FUN_DER

	//*** THE END OF: SUBSTITUTING OR COMPUTING GLOBAL DERIVATIVES OF IDOF SHAPE FUNCTION ***//
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	//********* ACTUAL FINAL CALCULATIONS FOR SM ENTRY  *********//
		#ifdef LOCAL_STIFF
		  	  stiff_mat[idof+num_dofs*jdof] += (
		#else
			  el_data_out[offset+idof*num_dofs+jdof] += (
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
				temp[6] * shape_fun_host[igauss*4*num_shap+4*jdof]
#else
				temp4 * fun_v_derx +
	       	    temp5 * fun_v_dery +
	       	    temp6 * fun_v_derz +
	       	    temp7 * shape_fun_host[igauss*4*num_shap+4*jdof]
#endif//vectorize

	#elif defined(HEAT)

	      	    temp4 * fun_v_derx +
	       	    temp5 * fun_v_dery +
	       	    temp6 * fun_v_derz +
	       	    temp7 * shape_fun_host[igauss*4*num_shap+4*jdof]

	#endif

						    ) * vol;

	//*** THE END OF: ACTUAL FINAL CALCULATIONS FOR SM ENTRY  ***//
	//-------------------------------------------------------------

		 //printf("el_data_out[%d]=%lf\n",offset+idof*num_dofs+jdof,el_data_out[offset+idof*num_dofs+jdof]);

		}//jdof

	//******* THE END OF: first loop over shape functions *******//
	//-------------------------------------------------------------

	      }//idof

	//******* THE END OF: second loop over shape functions *******//
	//-------------------------------------------------------------

	    }//gauss
//
//	// ******** THE END OF: loop over integration points ********//
//	//-------------------------------------------------------------
//
#ifdef LOCAL_STIFF

	offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);
    for(i = 0; i < num_dofs*num_dofs; i++) el_data_out[offset+i] = stiff_mat[i];
  #ifdef LOAD_VEC_COMP
    for(i = 0; i < num_dofs; i++) el_data_out[offset+one_el_stiff_mat_size+i] = load_vec[i];
  #endif

#endif




#ifdef COMPUTE_ALL_SHAPE_FUN_DER
	//register SCALAR *workspace = (SCALAR *) malloc((geo_dat_size+nr_coeff)*sizeof(SCALAR));
	#ifdef VECTORIZE
		_mm_free(tab_fun_u_der);
	#else
		_mm_free(tab_fun_u_derx);
		_mm_free(tab_fun_u_dery);
		_mm_free(tab_fun_u_derz);
	#endif
#endif

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
