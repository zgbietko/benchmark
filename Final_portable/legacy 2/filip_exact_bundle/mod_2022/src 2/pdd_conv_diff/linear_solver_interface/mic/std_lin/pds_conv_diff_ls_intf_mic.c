/************************************************************************
File pds_conv_diff_ls_intf_mic - interactions with linear solver
                          for conv_diff module implemented for MIC architecture(Xeon PHI)

Contains definitions of routines:


  pdr_create_assemble_stiff_mat_mic_elem -
                                 to create element stiffness matrices
                                 and assemble them to the global SM

------------------------------
History:
	08.2015 - Filip Krużel, initial version
*************************************************************************/

#include "mic.h"

//mic wrapper save data
//#define WRAP
//#define THR 240

#ifdef NORMAL
	#define PAD 6

#elif defined CILK
	#define PAD 8

#elif defined TETRA
	#define PAD 4
#endif


//#define PAD 4
//pad=8 or 6(normal) for prism and 4 for tetra
#define STRIDE 8 //for AVX registers with stride version(with other set to one)

/*------------------------------------------------------------
 pdr_create_assemble_stiff_mat_opencl_elem - to create element stiffness matrices
                                 and assemble them to the global SM
// GENERIC PROCEDURE - MANY PROBLEM DEPENDENT OPTIMIZATIONS ARE POSSIBLE
// SEVERAL SUITABLE PLACES ARE INDICATED BY:
// !!!OPT_PDT!!!
------------------------------------------------------------*/
int pdr_create_assemble_stiff_mat_mic_elem(
  int Problem_id, 
  int Level_id, 
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_elems_mic,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
					      )
{
	int i,j,k;

#ifdef MIC
//init
	#pragma offload_transfer target(mic)
#endif


//#ifdef NORMAL
//printf("Normal - pad=%d\n",PAD);
//#elif defined CILK
//printf("Cilk - pad=%d\n",PAD);
//#elif defined TETRA
//printf("TETRA - pad=%d\n",PAD);
//#endif
printf("PAD=%d!\n",PAD);



#ifdef TUNING

	  line_count = 0;

	  resuf = fopen("result.csv", "a+");
	  if(!resuf) {
		 printf("Could not open results file!\n");
		 exit(-1);
	  }

	  int c;

	  while ( (c=fgetc(resuf)) != EOF ) {
		 if ( c == '\n' )
				line_count++;
	  }

	  printf("Result file has %u lines\n", line_count);
	  if(line_count==0)
	  {
		 headuf = fopen("header.csv", "w");
			  if(!headuf) {
				 printf("Could not open header file!\n");
				 exit(-1);
		  }
		  fprintf(headuf,"REGISTERS,COMPUTE_ALL_SHAPE_FUN_DER,");
	  }

		#ifdef REGISTERS
			  fprintf(resuf,"1,");
		#else
			  fprintf(resuf,"0,");
		#endif

		#ifdef COMPUTE_ALL_SHAPE_FUN_DER
			  fprintf(resuf,"1,");
		#else
			  fprintf(resuf,"0,");
		#endif

#endif  //TUNING

	#ifdef TIME_TEST
	    t_begin = time_clock();
	#endif

	int nrdfobl;
	int idofent;
	int nr_dof_ent = PDC_MAX_DOF_PER_INT;
	int max_nrdofs=Max_dofs_int_ent;
	int nrdofs_int_ent = max_nrdofs;
	int ielem;
	int l_dof_ent_id[PDC_MAX_DOF_PER_INT], l_dof_ent_nrdof[PDC_MAX_DOF_PER_INT];
	//int l_dof_ent_posglob[PDC_MAX_DOF_PER_INT];
	int l_dof_ent_type[PDC_MAX_DOF_PER_INT];
	char rewrite;
	int level_id=0;

	//
	// PREPARE PROBLEM DEPENDENT DATA FOR INTEGRATION
	//

	int name=pdr_ctrl_i_params(Problem_id,1);
	int field_id = pdr_ctrl_i_params(Problem_id, 3);
	int mesh_id = apr_get_mesh_id(field_id);
	int nreq = apr_get_nreq(field_id);

	if(nreq>PDC_MAXEQ){
	printf("nreq (%d) > PDC_MAXEQ (%d) in pdr_create_assemble_stiff_mat_mic_elem\n",
	   nreq, PDC_MAXEQ);
	printf("Exiting!\n"); exit(-1);
	}

	// get the active PDE coefficient matrices
	/* pde coefficients */
	static int coeff_ind = 0;
	int coeff_ind_vect[23] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	// 1 - mval, 2 - axx, 3 - axy, 4 - axz, 5 - ayx, 6 - ayy, 7 - ayz,
	// 8 - azx, 9 - azy, 10 - azz, 11 - bx, 12 - by, 13 - bz
	// 14 - tx, 15 - ty, 16 - tz, 17 - cval
	// 18 - lval, 19 - qx, 20 - qy, 21 - qz, 22 - sval

	double axx[PDC_MAXEQ*PDC_MAXEQ];
	double axy[PDC_MAXEQ*PDC_MAXEQ];
	double axz[PDC_MAXEQ*PDC_MAXEQ];
	double ayx[PDC_MAXEQ*PDC_MAXEQ];
	double ayy[PDC_MAXEQ*PDC_MAXEQ];
	double ayz[PDC_MAXEQ*PDC_MAXEQ];
	double azx[PDC_MAXEQ*PDC_MAXEQ];
	double azy[PDC_MAXEQ*PDC_MAXEQ];
	double azz[PDC_MAXEQ*PDC_MAXEQ];
	double bx[PDC_MAXEQ*PDC_MAXEQ];
	double by[PDC_MAXEQ*PDC_MAXEQ];
	double bz[PDC_MAXEQ*PDC_MAXEQ];
	double tx[PDC_MAXEQ*PDC_MAXEQ];
	double ty[PDC_MAXEQ*PDC_MAXEQ];
	double tz[PDC_MAXEQ*PDC_MAXEQ];
	double cval[PDC_MAXEQ*PDC_MAXEQ];
	double mval[PDC_MAXEQ*PDC_MAXEQ];
	double qx[PDC_MAXEQ];
	double qy[PDC_MAXEQ];
	double qz[PDC_MAXEQ];
	double sval[PDC_MAXEQ];
	double lval[PDC_MAXEQ];

	pdr_select_el_coeff_vect(Problem_id, coeff_ind_vect);

	// there are two choices:
	// 1. consider all terms (16 arrays and 4 vectors)
	// 2. use coeff_vector_indicator to select which terms are non-zero
	int coeff_array_indicator[16];
	int nr_coeff_arrays = 0;
	for(i=0; i<16; i++) {
		coeff_array_indicator[i]=coeff_ind_vect[i+2];
		if(coeff_ind_vect[i+2]==1) nr_coeff_arrays++;
	}
	// in create_assemble we combine mval (1) with cval (17)
	// we use this for vectorization of kernels
	if(coeff_ind_vect[1]==1) {
	if(coeff_array_indicator[15]!=1) nr_coeff_arrays++;
		coeff_array_indicator[15]=1;
	}
	int coeff_vector_indicator[4];
	int nr_coeff_vectors = 0;
	for(i=0; i<4; i++) {
		coeff_vector_indicator[i]=coeff_ind_vect[i+19];
	if(coeff_ind_vect[i+19]==1) nr_coeff_vectors++;
	}
	// in create_assemble we combine lval (18) with sval (22)
	// we use this for vectorization of kernels
	if(coeff_ind_vect[18]==1) {
	if(coeff_vector_indicator[3]!=1) nr_coeff_vectors++;
		coeff_vector_indicator[3]=1;
	}


	/*kbw*/
	printf("Problem ID: %d, name %d\n", Problem_id, name);
	printf("\nthe number of coefficients arrays %d, indicator = \n",nr_coeff_arrays);
	for(i=0;i<16;i++){
		printf("%2d",coeff_array_indicator[i]);
	}
	printf("\n");
	printf("the number of coefficients vectors %d, indicator = \n",nr_coeff_vectors);
	for(i=0;i<4;i++){
		printf("%2d",coeff_vector_indicator[i]);
	}
	printf("\n");
	/*kew*/


//#ifdef TUNING
//	if(line_count==0)
//		fprintf(headuf,"Num elems,");
//	fprintf(resuf,"%d,",Nr_elems_mic);
//#endif

	    //
	    // BASED ON PDEG COMPUTE EXECUTION CHARACTERISTICS
	    //

	    // choose an example element for a given pdeg and color
	    int el_id = L_int_ent_id[Nr_elems_mic-1];	//it can be last elem=we assume elems of the same type

	    int pdeg = apr_get_el_pdeg(field_id, el_id, NULL);
	    int num_shap = apr_get_el_pdeg_numshap(field_id, el_id, &pdeg);
	    int num_dofs = num_shap * nreq;
	    int el_nodes[MMC_MAXELVNO+1];        /* list of nodes of El */
	    mmr_el_node_coor(mesh_id, el_id, el_nodes, NULL);
	    /* for geometrically (multi)linear elements number of geometrical  */
	    /* degrees of freedom is equal to the number of vertices - classical FEM nodes */
	    int num_geo_dofs = el_nodes[0];
	    int pdeg_single = pdeg;
	    if(pdeg>100) {
	      pdeg_single = pdeg/100;
	      if(pdeg != pdeg_single*100 + pdeg_single){
		printf("wrong pdeg %d ( > 100 but not x0x )\n", pdeg);
		exit(-1);
	      }
	    }

	/*kbw*/
	      printf("problem and element characteristics: nreq %d, pdeg %d, num_shap %d, num_dofs %d!\n",
		     nreq, pdeg, num_shap, num_dofs);
	//kew*/

		// SIZES OF DATA STRUCTURES

		// 1. QUADRATURE DATA AND JACOBIAN TERMS

		// get the size of quadrature data
		int base = apr_get_base_type(field_id, el_id);
		int ngauss;            /* number of gauss points */
		double xg[3000];   	 /* coordinates of gauss points in 3D */
		double wg[1000];       /* gauss weights */
		apr_set_quadr_3D(base, &pdeg, &ngauss, xg, wg);

		// we may need quadrature data for the reference element
		int ref_el_quadr_dat_size;
		// but we may need also/instead quadrature related Jacobian data for each element
		int one_el_quadr_dat_size;

		if(num_dofs==4)
	    {
	        // for tetrahedrons only weight are sent for reference element
	        ref_el_quadr_dat_size = ngauss;
	        // we do not need Jacobian terms, but we need geometry DOFs
	        one_el_quadr_dat_size = 0;
	    }else{
			// for each gauss point its coordinates and weight are sent for reference element
			ref_el_quadr_dat_size = ngauss*4;
			// we do not need Jacobian terms, but we need geometry DOFs
			one_el_quadr_dat_size = 0;
	    }

	    // 2. GEO_DOFS

	    // get the size of geometry data for one element - we assume multi-linear elements
	    double geo_dofs[3*MMC_MAXELVNO];  /* coord of nodes of El */

	    int one_el_geo_dat_size;
	    one_el_geo_dat_size = 3*num_geo_dofs;

	    // 3. SHAPE FUNCTIONS

	    // space for element shape functions' values and derivatives in global memory
	    int ref_el_shape_fun_size; // for the reference element
	    int one_el_shape_fun_size; // for each processed element

	    // we need all shape functions and their derivatives
	    // at all integration points for the reference element
	    ref_el_shape_fun_size = 4*num_shap*ngauss;
	    // we do not store any data for particular elements
	    one_el_shape_fun_size = 0;

	    // 4. PDE COEFFICIENTS

	    // there are two choices:
	    // 1. consider all terms (16 arrays and 4 vectors)
	    // 2. use coeff_vector_indicator to select which terms are non-zero
	    // this options have to be taken into account when rewriting coefficients returned
	    // by problem dependent module to coeff array
	    // HERE coeff_vector_indicator is used to calculate pde_coeff_size
	    int pde_coeff_size = nr_coeff_arrays*nreq*nreq + nr_coeff_vectors*nreq;

	    // different parameters to differentiate between different cases
	    int all_el_pde_coeff_size = 0; // size for all elements
	    int one_el_pde_coeff_size = 0; // size for one element and all integration point
	    int one_int_p_pde_coeff_size = 0; // size for one element and one integration point

	    // default - not practical: all coefficients at all integration points sent
	    one_int_p_pde_coeff_size  = pde_coeff_size;

			  // special versions
		#ifdef LAPLACE
			all_el_pde_coeff_size = 0;
			one_el_pde_coeff_size = 0;
			one_int_p_pde_coeff_size = 1; // one RHS coefficient per integration point
		#endif
		#ifdef TEST_SCALAR
			all_el_pde_coeff_size = 0;
			one_el_pde_coeff_size = pde_coeff_size; // all coeff are constant over element
			one_int_p_pde_coeff_size  = 0;
		#endif


		// 5. COMPUTED STIFFNESS MATRIX AND LOAD VECTOR
		//int one_el_stiff_mat_size = num_dofs*num_dofs;
		//int one_el_load_vec_size = num_dofs;
		int one_el_stiff_mat_size = PAD*PAD;
		int one_el_load_vec_size = PAD;


	/*kbw
		printf("\nAssumed data structure sizes:\n");
		printf("\tQuadrature data: global %d, for each element %d\n",
		   ref_el_quadr_dat_size, one_el_quadr_dat_size);
		printf("\tGeo dofs for each element: %d\n", one_el_geo_dat_size);
		printf("\tShape functions and derivatives: reference el. %d, each el. %d\n",
		   ref_el_shape_fun_size, one_el_shape_fun_size);
		printf("\tPDE coefficients for each element %d or each integration point %d\n",
		   one_el_pde_coeff_size, one_int_p_pde_coeff_size);
		printf("\tSM for each element %d\n", one_el_stiff_mat_size);
		printf("\tLoad vector for each element %d\n", one_el_load_vec_size);
	/*kew*/



    // ACTUAL GLOBAL!!! MEMORY CALCULATIONS
    int global_memory_req_in = 0;
    int global_memory_req_one_el_in = 0;
    int global_memory_req_out = 0; // size of array with execution parameters
    int global_memory_req_one_el_out = 0;

    // 1. QUADRATURE DATA AND JACOBIAN TERMS
    //  - coordinates and weights for JAC - for the reference element
    //  - Jacobian terms for NOJAC - for all elements
    global_memory_req_in += ref_el_quadr_dat_size;
    global_memory_req_one_el_in += one_el_quadr_dat_size;

    // 2. GEO_DOFS - geo_dofs for JAC, nothing for NOJAC
    global_memory_req_one_el_in += one_el_geo_dat_size;

    // 3. SHAPE FUNCTIONS
    // for the reference element
    //  - all functions and derivatives at all integration points - JAC and NOJAC
    global_memory_req_in += ref_el_shape_fun_size;
    // for each considered element - nothing for the time being
    global_memory_req_one_el_in += one_el_shape_fun_size;

    // 4. PDE COEFFICIENTS
    // global coefficients - the same for all elements
    global_memory_req_in += all_el_pde_coeff_size;
    // COEFFICIENTS DIFFERENT FOR EACH ELEMENT BUT THE SAME FOR ALL INTEGRATION POINTS
    global_memory_req_one_el_in += one_el_pde_coeff_size;
    // COEFFICIENTS DIFFERENT FOR EACH ELEMENT AND EACH INTEGRATION POINT ?????????????
    global_memory_req_one_el_in += ngauss*one_int_p_pde_coeff_size;


    // 5. COMPUTED STIFFNESS MATRIX AND LOAD VECTOR
    global_memory_req_one_el_out += one_el_stiff_mat_size + one_el_load_vec_size;

    int global_memory_req = global_memory_req_in + global_memory_req_out;
    int global_memory_req_one_el = global_memory_req_one_el_in + global_memory_req_one_el_out;


		//TODO
		//Ewentualne alokacje aligned buforów

    	const size_t gauss_dat_host_dev_bytes = ref_el_quadr_dat_size * sizeof(SCALAR);
		#define MAX_SIZE_ARRAY_GAUSS 1344
	    // 1344 - maximal value of arrGaussSize (for p=707), (for 909 is 2920)


    	//ALIGNMENT
//#ifdef MIC
//	   SCALAR __attribute__((target(mic))) gauss_dat_host[MAX_SIZE_ARRAY_GAUSS] __attribute__((aligned(ALIGN))); //it is aligned to ALIGN
//#else
	   //SCALAR gauss_dat_host[MAX_SIZE_ARRAY_GAUSS] __attribute__((aligned(ALIGN))); //it is aligned to ALIGN

    	SCALAR *gauss_dat_host = (SCALAR*)_mm_malloc(1344*sizeof(SCALAR),ALIGN);
 //#endif

	   const size_t shape_fun_dev_bytes  = ref_el_shape_fun_size * sizeof(SCALAR);

	   //int shape_fun_host_bytes = shape_fun_dev_bytes;

	   //stride ngauss=8, num_shap=8;
       int size_shap = 4*PAD*ngauss;
	   int shape_fun_host_bytes = size_shap*sizeof(SCALAR);

	   //ALIGNMENT
//	   printf("Align - Before:shape_fun_host_bytes=%d\t",shape_fun_host_bytes);
//	   int div,res,d,size_shap;
//	   //int coeff;
//	   d=(ALIGN/sizeof(SCALAR));
//	   div=shape_fun_host_bytes/d;
//	   res=shape_fun_host_bytes%d;
//	   shape_fun_host_bytes=div*d+res*d;
//	   size_shap=shape_fun_host_bytes/sizeof(SCALAR);
	   printf("Align - After:shape_fun_host_size=%d\n",shape_fun_host_bytes/8);

	   SCALAR *shape_fun_host = (SCALAR*)_mm_malloc(shape_fun_host_bytes,ALIGN);
	   //SCALAR *shape_fun_host = (SCALAR*)malloc(shape_fun_host_bytes);

//	   int size_in = global_memory_req_one_el_in * Nr_elems_mic * sizeof(SCALAR);
//
//
//	   printf("Align - Before:el_data_in_bytes=%d\t",size_in);
//	   div=size_in/d;
//	   res=size_in%d;
//	   size_in=div*d+res*d;
//	   printf("Align - After:el_data_in_bytes=%d\n",size_in);
//	   const size_t el_data_in_bytes = size_in;
//	   size_in=size_in/sizeof(SCALAR);


	   //aligment of Nr_elems
//save for checking
	   int Nr_elems_check=Nr_elems_mic;

#ifdef MIC
	int __attribute__((target(mic))) Threads;
	#pragma offload target(mic)
	{
	   Threads=omp_get_num_procs();
	}
	#ifdef THR
		Threads=THR;
	#endif
#else
	#ifdef THR
		const int Threads=THR;
	#else
	   const int Threads=omp_get_num_procs();
	#endif
#endif
	   printf("threads=%d\n",Threads);
	   int div,res,d;
	   div=Nr_elems_mic/Threads;
	   res=Nr_elems_mic%Threads;
	   if(res)
		 Nr_elems_mic+=Threads-res;

	   printf("\nAlign nr elems to threads num div=%d,res=%d,Nr_elems_mic=%d\n",div,res,Nr_elems_mic);

#ifdef TUNING
	if(line_count==0)
		fprintf(headuf,"Num elems,");
	fprintf(resuf,"%d,",Nr_elems_mic);
#endif

		d=num_geo_dofs*3*sizeof(SCALAR);
		div=d/ALIGN;
		res=d%ALIGN;
		res=res==0?0:1;
		d=(div*ALIGN+res*ALIGN)/sizeof(SCALAR);
		//div=d-num_geo_dofs*3;
	#ifdef LAPLACE
//		div=ngauss*sizeof(SCALAR)/ALIGN
//		res=ngauss*sizeof(SCALAR)/ALIGN
//		res=()/sizeof(SCALAR); //2
//
//		printf("\n!!res=%d\n",res);
//
//		res=res+ngauss;
		res=(int)(ceil(((double)ngauss*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);//2

		//res=(int)(ceil(((double)PAD*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);

		//printf("\n!!res+ngauss=%d\n",res);


	#endif
	#ifdef TEST_SCALAR
		res=(int)(ceil((20.*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);//24
	#endif

		//printf("d+res=%d\ALIGN",d+res);

	   int size_in=(d+res)*Nr_elems_mic;
	   const size_t el_data_in_bytes = size_in*sizeof(SCALAR);

	   printf("Align - After:d+res=%d,size=%d,el_data_in_bytes=%d\n",d+res,size_in,el_data_in_bytes);

	SCALAR* el_data_in = (SCALAR *)_mm_malloc(el_data_in_bytes,ALIGN); // host input buffer (standard array)
	//SCALAR* el_data_in = (SCALAR *)malloc(el_data_in_bytes); // host input buffer (standard array)


	//printf("el_data_out_size=%d\n",global_memory_req_one_el_out*Nr_elems_mic);

	//one_el_stiff_mat_size + one_el_load_vec_size;

	//int size_out = global_memory_req_one_el_out * Nr_elems_mic * sizeof(SCALAR);

	one_el_stiff_mat_size=(int)(ceil(((double)one_el_stiff_mat_size*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);

	printf("one_el_stiff_mat_size=%d\n",one_el_stiff_mat_size);

	one_el_load_vec_size=(int)(ceil(((double)one_el_load_vec_size*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);

	printf("one_el_load_vec_size=%d\n",one_el_load_vec_size);

	int size_out = (one_el_stiff_mat_size + one_el_load_vec_size) * Nr_elems_mic;

	const size_t el_data_out_bytes = size_out*sizeof(SCALAR);

	printf("\nel_data_out_bytes=%d\n",el_data_out_bytes);

	 SCALAR* el_data_out = (SCALAR *)_mm_malloc(el_data_out_bytes,ALIGN);
	//el_data_out = (SCALAR *)malloc(el_data_out_bytes);

	//printf("Align - size_out=%d\n",size_out);

	//
	memset(el_data_out,0,el_data_out_bytes);

//#ifdef MIC
//	#pragma offload target(mic) nocopy(el_data_out: length(size_out) alloc_if(1) free_if(0))
//	{}
//#endif

	#ifdef TUNING
	    if(line_count==0)
		{
			fprintf(headuf,"el_data_in [MB],el_data_out [MB],");
		}
	    fprintf(resuf,"%.3lf,%.3lf,",
		   (double)el_data_in_bytes*1.0e-6, (double)el_data_out_bytes*1.0e-6);
	#endif
	//
	//
	// fill and send buffers with integration data when necessary
	if(gauss_dat_host_dev_bytes != 0){

	// This is the version when we send only necessary variables but we do not care about sending
		if(num_dofs==4)
		{
			for(i=0; i<ngauss; i++){
			  gauss_dat_host[i] = wg[i];
			}
		}
		else
		{
			for(i=0; i<ngauss; i++){
			  gauss_dat_host[i] = xg[3*i];
			  gauss_dat_host[ngauss+i] = xg[3*i+1];
			  gauss_dat_host[2*ngauss+i] = xg[3*i+2];
			  //gauss_dat_host[4*i+3] = wg[i];
			}
		}
	}

	#ifdef MIC
		#pragma offload target(mic) in(gauss_dat_host[0:MAX_SIZE_ARRAY_GAUSS]:align(ALIGN) alloc_if(1) free_if(0))
		{}
	#endif

	// shape functions and derivatives are computed here but used also later on


	double base_phi_ref[APC_MAXELVD];    /* basis functions */
	double base_dphix_ref[APC_MAXELVD];  /* x-derivatives of basis function */
	double base_dphiy_ref[APC_MAXELVD];  /* y-derivatives of basis function */
	double base_dphiz_ref[APC_MAXELVD];  /* z-derivatives of basis function */

	// fill and send buffers with shape function values for the reference element
	if(shape_fun_dev_bytes != 0){

		// we need all shape functions and their derivatives
		// at all integration points for the reference element
		int ki;
		for (ki=0;ki<ngauss;ki++) {

		  int temp = apr_shape_fun_3D(base, pdeg, &xg[3*ki],
						  base_phi_ref, base_dphix_ref,base_dphiy_ref,base_dphiz_ref);

		  assert(temp==num_shap);

		  for(i=0;i<num_shap;i++){


			shape_fun_host[ki*PAD+i] = base_dphix_ref[i];
			shape_fun_host[ngauss*PAD+ki*PAD+i] = base_dphiy_ref[i];
			shape_fun_host[2*ngauss*PAD+ki*PAD+i] = base_dphiz_ref[i];

			shape_fun_host[3*ngauss*PAD+ki*PAD+i] = base_phi_ref[i];

//			shape_fun_host[ki*3*num_shap+i] = base_dphix_ref[i];
//			shape_fun_host[num_shap+ki*3*num_shap+i] = base_dphiy_ref[i];
//			shape_fun_host[2*num_shap+ki*3*num_shap+i] = base_dphiz_ref[i];


//			shape_fun_host[ki*4*num_shap+4*i] = base_phi_ref[i];
//			shape_fun_host[ki*4*num_shap+4*i+1] = base_dphix_ref[i];
//			shape_fun_host[ki*4*num_shap+4*i+2] = base_dphiy_ref[i];
//			shape_fun_host[ki*4*num_shap+4*i+3] = base_dphiz_ref[i];



			//printf("shape_fun_host[%d]=%lf\n",ki*4*num_shap+4*i,shape_fun_host[ki*4*num_shap+4*i]); ok

		  }

		}

	}

#ifdef MIC
	#pragma offload target(mic:0) in(shape_fun_host[0:size_shap]:align(ALIGN) alloc_if(1) free_if(0))
	{}
#endif
	// !!!!!!!!!!!!!!!***************!!!!!!!!!!!!!!!!!

	// finally fill element input data
	int packed_bytes = 0;
	//int final_position = 0; // for testing packing procedure

	memset(el_data_in, 0, el_data_in_bytes);

    // for one_el_one_thread strategy we keep geo_dofs and coeffs for each element separately
	// i.e. geo_dofs for all elements and next coeffs for all elements
	int position_geo_dofs = 0;
	//int jot=0;

	d=num_geo_dofs*3*sizeof(SCALAR);
	div=d/ALIGN;
	res=d%ALIGN;
	res=res==0?0:1;
	d=(div*ALIGN+res*ALIGN)/sizeof(SCALAR);
	div=d-num_geo_dofs*3;
	int position_coeff = Nr_elems_mic*d;
#ifdef LAPLACE
	res=(int)(ceil(((double)ngauss*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);//2
	//res=(int)(ceil(((double)PAD*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);
#endif
#ifdef TEST_SCALAR
	res=(int)(ceil((20.*sizeof(SCALAR))/ALIGN)*ALIGN)/sizeof(SCALAR);//24
#endif

	//printf("\n res=%d\n",res);


	for(ielem=0; ielem < Nr_elems_mic; ielem++){

		// element ID
		el_id = L_int_ent_id[ielem];

		int el_mate = mmr_el_groupID(mesh_id, el_id);
		double hsize = mmr_el_hsize(mesh_id, el_id, NULL,NULL,NULL);

		// checking whether this element has the same data as assumed for this color
		assert( pdeg == apr_get_el_pdeg(field_id, el_id, NULL) );
		assert( num_shap == apr_get_el_pdeg_numshap(field_id, el_id, &pdeg) );
		assert( base == apr_get_base_type(field_id, el_id) );

		// IDs of element vertices (nodes) and their coordinates as geo_dofs
		mmr_el_node_coor(mesh_id, el_id, NULL, geo_dofs);

		for(i=0;i<num_geo_dofs;i++){
		  el_data_in[position_geo_dofs] = geo_dofs[3*i];
		  el_data_in[position_geo_dofs+1] = geo_dofs[3*i+1];
		  el_data_in[position_geo_dofs+2] = geo_dofs[3*i+2];
		  //if(ielem==0||ielem==1)
		  //printf("ielem=%d, el_data_in[%d]=%lf\n",ielem, position_geo_dofs,el_data_in[position_geo_dofs]);
		  position_geo_dofs += 3;
		  //jot+=3;
		}
		position_geo_dofs += div;
		packed_bytes += (div+num_geo_dofs*3)*sizeof(SCALAR);

		// PDE coefficients

	#ifdef LAPLACE

		int ki;
		for(ki=0;ki<ngauss;ki++)
		  {
		    double xcoor[3] = {0,0,0};

		    apr_elem_calc_3D(2, nreq, &pdeg, base,
				     &xg[3*ki], geo_dofs, NULL,
				     NULL,NULL,NULL,NULL,
				     xcoor,NULL,NULL,NULL,NULL,NULL);

		    pdr_el_coeff(Problem_id, el_id, el_mate, hsize, pdeg, NULL,
				 NULL, NULL, NULL, NULL,
				 xcoor, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
				 //base_phi_ref, base_dphix, base_dphiy, base_dphiz,
				 //xcoor, uk_val, uk_x, uk_y, uk_z, un_val, un_x, un_y, un_z,
				 NULL, axx, axy, axz, ayx, ayy, ayz, azx, azy, azz,
				 bx, by, bz, tx, ty, tz, cval, NULL, qx, qy, qz, sval);

		    // for Laplace (Poisson) problem we send RHS coefficients only
		    // (stiffness matrix coefficients are all 1.0)
		    el_data_in[position_coeff+ki] = sval[0];
		    // printf("el_data_in[%d]=%lf\n",position_coeff+ki,el_data_in[position_coeff+ki]);
		    //jot++;

		  }

		position_coeff += res;
		packed_bytes += res*sizeof(SCALAR);

	#endif // LAPLACE

	#ifdef TEST_SCALAR

		double xcoor_middle[3];
		xcoor_middle[0] = (geo_dofs[0+0]+geo_dofs[3+0]+geo_dofs[6+0]
				   +geo_dofs[9+0]+geo_dofs[12+0]+geo_dofs[15+0])/6.0;
		xcoor_middle[1] = (geo_dofs[0+1]+geo_dofs[3+1]+geo_dofs[6+1]
				   +geo_dofs[9+1]+geo_dofs[12+1]+geo_dofs[15+1])/6.0;
		xcoor_middle[2] = (geo_dofs[0+2]+geo_dofs[3+2]+geo_dofs[6+2]
				   +geo_dofs[9+2]+geo_dofs[12+2]+geo_dofs[15+2])/6.0;
		pdr_el_coeff(Problem_id, el_id, el_mate, hsize, pdeg, NULL,
			     NULL, NULL, NULL, NULL,
			     xcoor_middle, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			     //base_phi_ref, base_dphix, base_dphiy, base_dphiz,
			     //xcoor, uk_val, uk_x, uk_y, uk_z, un_val, un_x, un_y, un_z,
			     NULL, axx, axy, axz, ayx, ayy, ayz, azx, azy, azz,
			     bx, by, bz, tx, ty, tz, cval, NULL, qx, qy, qz, sval);
//TODO +wspolczynnik zalezny od nr_el%STRIDE

		//if(STRIDE==1)
		//{

				el_data_in[position_coeff+0] = axx[0];
				el_data_in[position_coeff+1] = axy[0];
				el_data_in[position_coeff+2] = axz[0];
				el_data_in[position_coeff+3] = ayx[0];
				el_data_in[position_coeff+4] = ayy[0];
				el_data_in[position_coeff+5] = ayz[0];
				el_data_in[position_coeff+6] = azx[0];
				el_data_in[position_coeff+7] = azy[0];
				el_data_in[position_coeff+8] = azz[0];

				el_data_in[position_coeff+9] = bx[0];
				el_data_in[position_coeff+10] = by[0];
				el_data_in[position_coeff+11] = bz[0];

				el_data_in[position_coeff+12] = tx[0];
				el_data_in[position_coeff+13] = ty[0];
				el_data_in[position_coeff+14] = tz[0];

				el_data_in[position_coeff+15] = cval[0];

				el_data_in[position_coeff+16] = qx[0];
				el_data_in[position_coeff+17] = qy[0];
				el_data_in[position_coeff+18] = qz[0];

				el_data_in[position_coeff+19] = sval[0];

//				if(ielem==0)
//					printf("el_data_in[%d]=%lf\n",position_coeff+0,el_data_in[position_coeff+0]);

				packed_bytes += res*sizeof(SCALAR);
				position_coeff += res;
		//}else
//		//{
////				if(ielem<12)
////				{
////					printf("\nielem=%d,res=%d,pos_coeff=%d,div=%d,d=%d\n",ielem,res,position_coeff,ielem%STRIDE,ielem/STRIDE);
////				}
//				d=ielem/STRIDE;
//				div=ielem%STRIDE;
////				if(ielem<12)
////				{
////					printf("first - position_coeff+0*STRIDE+div = %d\n",position_coeff+0+div);
////					printf("second - position_coeff+1*STRIDE+div = %d\n",position_coeff+1*STRIDE+div);
////					printf("last - position_coeff+19*STRIDE+div = %d\n",position_coeff+19*STRIDE+div);
////				}
//
//				el_data_in[position_coeff+0*STRIDE+div] = axx[0];
//				el_data_in[position_coeff+1*STRIDE+div] = axy[0];
//				el_data_in[position_coeff+2*STRIDE+div] = axz[0];
//				el_data_in[position_coeff+3*STRIDE+div] = ayx[0];
//				el_data_in[position_coeff+4*STRIDE+div] = ayy[0];
//				el_data_in[position_coeff+5*STRIDE+div] = ayz[0];
//				el_data_in[position_coeff+6*STRIDE+div] = azx[0];
//				el_data_in[position_coeff+7*STRIDE+div] = azy[0];
//				el_data_in[position_coeff+8*STRIDE+div] = azz[0];
//
//				el_data_in[position_coeff+9*STRIDE+div] = bx[0];
//				el_data_in[position_coeff+10*STRIDE+div] = by[0];
//				el_data_in[position_coeff+11*STRIDE+div] = bz[0];
//
//				el_data_in[position_coeff+12*STRIDE+div] = tx[0];
//				el_data_in[position_coeff+13*STRIDE+div] = ty[0];
//				el_data_in[position_coeff+14*STRIDE+div] = tz[0];
//
//				el_data_in[position_coeff+15*STRIDE+div] = cval[0];
//
//				el_data_in[position_coeff+16*STRIDE+div] = qx[0];
//				el_data_in[position_coeff+17*STRIDE+div] = qy[0];
//				el_data_in[position_coeff+18*STRIDE+div] = qz[0];
//
//				el_data_in[position_coeff+19*STRIDE+div] = sval[0];
//
//				if(ielem==0)
//					printf("el_data_in[%d]=%lf\n",position_coeff+0*STRIDE+div,el_data_in[position_coeff+0*STRIDE+div]);
//
//				packed_bytes += res*sizeof(SCALAR);
//				if(ielem>0&&div==3)
//					position_coeff += STRIDE*res;


//		}

		//assert(pde_coeff_size<=res);


	#endif // TEST_SCALAR

	} // end for all elements in input (nr_elems_mic)

#ifdef MIC
	#pragma offload target(mic:0) in(el_data_in[0:size_in]:align(ALIGN) alloc_if(1) free_if(0))
	{}

#endif

	//printf("real size=%d\n",jot);
	//printf("packet_bytes=%d,pos_coeff=%d, el_id=%d\n",packed_bytes/(sizeof(SCALAR)), position_coeff,el_id);

	//packet_bytes=3862783,pos_coeff=3911680
	//if(STRIDE==1)
	//{
		//assert(packed_bytes == position_coeff*sizeof(SCALAR));
	//}


#ifdef TIME_TEST
    t_end = time_clock();
    printf("\nEXECUTION TIME: Initial settings on CPU and creating and filling buffers %lf\n",
	   t_end-t_begin);
    total_time += t_end-t_begin;

#ifdef TUNING
	if(line_count==0)
		fprintf(headuf,"Buffers create,");
	fprintf(resuf,"%lf,",t_end-t_begin);
#endif

#endif


#ifdef TIME_TEST
    t_begin = time_clock();
#endif

//tu funcja uruchamiajaca

//printf("gauss_dat->%p, &gauss_dat[0]->%p\n",gauss_dat_host,&gauss_dat_host[0]);
//printf("shape_fun_host->%p, &shape_fun_host->%p\n",shape_fun_host,&shape_fun_host[0]);
//printf("el_data_in->%p, &el_data_in->%p\n",el_data_in,&el_data_in[0]);
//printf("el_data_out->%p, &el_data_out->%p\n",el_data_out,&el_data_out[0]);

//for reducing the overhead of thread creation


#ifdef MIC
#pragma offload target(mic:0)
{
#endif

#pragma omp parallel
{
}

#ifdef MIC
}
#endif

//for MIC wrapper

#ifdef WRAP

	FILE *input;
	FILE *output;

	#ifdef LAPLACE

	if(num_dofs==6)
	{
		input = fopen("input_LAPLACE_PRISM.txt", "w");
	}
	else
	{
		input = fopen("input_LAPLACE_TETRA.txt", "w");
	}

	#elif defined TEST_SCALAR

	if(num_dofs==6)
	{
		input = fopen("input_TEST_PRISM.txt", "w");
	}
	else
	{
		input = fopen("input_TEST_TETRA.txt", "w");
	}

	#endif

	fprintf(input,"%d\n",Nr_elems_mic);
	fprintf(input,"%d\n",size_in);
	fprintf(input,"%d\n",size_out);
	fprintf(input,"%d\n",size_shap);
	fprintf(input,"%d\n",div+num_geo_dofs*3);
	fprintf(input,"%d\n",res);
	fprintf(input,"%d\n",one_el_stiff_mat_size);
	fprintf(input,"%d\n",one_el_load_vec_size);
	for(i=0;i<1344;i++)
		fprintf(input,"%lf ",gauss_dat_host[i]);
	fprintf(input,"\n");
	for(i=0;i<size_shap;i++)
		fprintf(input,"%lf ",shape_fun_host[i]);
	fprintf(input,"\n");
	for(i=0;i<size_in;i++)
		fprintf(input,"%lf ",el_data_in[i]);
	fprintf(input,"\n");

	fclose(input);

#endif //wrapper

//printf("div+num_geo-dofs*3=%d\n",div+num_geo_dofs*3);

#ifdef LAPLACE

	if(num_dofs==6)
	{
		//__itt_resume();
		pdr_num_int_el_QSS_prism(gauss_dat_host,shape_fun_host,
			  el_data_in, el_data_out, Nr_elems_mic,
			  size_out,size_in,size_shap,div+num_geo_dofs*3,res,one_el_stiff_mat_size,one_el_load_vec_size);
		//__itt_pause();
	}else
	{
		//__itt_resume();
		pdr_num_int_el_QSS_tetra(gauss_dat_host,shape_fun_host,
					  el_data_in, el_data_out,Nr_elems_mic,
					  size_out,size_in,size_shap,div+num_geo_dofs*3,res,one_el_stiff_mat_size,one_el_load_vec_size);
		//__itt_pause();
	}

#elif defined TEST_SCALAR

		if(num_dofs==6)
		{
			//__itt_resume();
			pdr_num_int_el_QSS_prism(gauss_dat_host,shape_fun_host,
				  			  el_data_in, el_data_out, Nr_elems_mic,
				  			  size_out,size_in,size_shap,div+num_geo_dofs*3,res,one_el_stiff_mat_size,one_el_load_vec_size);
			//__itt_pause();
		}else
		{
			//__itt_resume();
			pdr_num_int_el_QSS_tetra(gauss_dat_host,shape_fun_host,
				  			  el_data_in, el_data_out,Nr_elems_mic,
				  			  size_out,size_in,size_shap,div+num_geo_dofs*3,res,one_el_stiff_mat_size,one_el_load_vec_size);
			//__itt_pause();
		}


#elif defined HEAT

	//  pdr_num_int_el_QSS_prism(gauss_dat_host,shape_fun_host,
	// 	  			  el_data_in, el_data_out, nreq, ngauss, num_shap,
	// 	  			  num_geo_dofs, Nr_elems_mic, ???);

#endif

#ifdef TIME_TEST
    t_end = time_clock();
    printf("\nEXECUTION TIME: Numerical integration %lf\n",
	   t_end-t_begin);
    total_time += t_end-t_begin;

#ifdef TUNING
	if(line_count==0)
		fprintf(headuf,"Num int,");
	fprintf(resuf,"%lf,",t_end-t_begin);
#endif

#endif

//copying data back
#ifdef MIC
	#pragma offload target(mic:0) out(el_data_out[0:size_out]:align(ALIGN) alloc_if(0) free_if(1))
    {}
#endif

#ifdef WRAP

	#ifdef LAPLACE

	if(num_dofs==6)
	{
		output = fopen("output_LAPLACE_PRISM.txt", "w");
	}
	else
	{
		output = fopen("output_LAPLACE_TETRA.txt", "w");
	}

	#elif defined TEST_SCALAR

	if(num_dofs==6)
	{
		output = fopen("output_TEST_PRISM.txt", "w");
	}
	else
	{
		output = fopen("output_TEST_TETRA.txt", "w");
	}

	#endif

	for(i=0;i<size_out;i++)
			fprintf(output,"%lf ",el_data_out[i]);
	fprintf(output,"\n");
	fclose(output);


#endif



//#define TESTING_CORRECTNESS

#ifdef TESTING_CORRECTNESS


for(ielem=0; ielem < Nr_elems_check; ielem++){
	// element ID
	int intent = ielem; //L_int_ent_id[ielem];
	double *stiff_mat = (double *)malloc(num_dofs*num_dofs*sizeof(double));
	double *rhs_vect = (double *)malloc(num_dofs*sizeof(double));
	//int l_dof_ent_id[PDC_MAX_DOF_PER_INT], l_dof_ent_nrdof[PDC_MAX_DOF_PER_INT];

	//int l_dof_ent_type[PDC_MAX_DOF_PER_INT];
	//int nr_dof_ent = PDC_MAX_DOF_PER_INT;
	//int nrdofs_int_ent = Max_dofs_int_ent;
	//char rewrite;
	// FOR REWRITING STIFF_MAT AND LOAD_VEC THEY ARE NOT COPUTED
	// only data necessary for asembling are obtained
	/* pdr_comp_stiff_mat(Problem_id, L_int_ent_type[intent], */
	/* 		   L_int_ent_id[intent], PDC_NO_COMP, NULL, */
	/* 		   &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdof, */
	/* 		   &nrdofs_int_ent, NULL, NULL, &rewrite); */


	// FOR TESTING CORRECTNESS STIFF_MAT AND LOAD_VEC ARE COMPUTED
	  /* initialize the matrices to zero */
	  for(i=0;i<num_dofs*num_dofs;i++) stiff_mat[i]=0.0;

	  /* initialize the vector to zero */
	  for(i=0;i<num_dofs;i++) rhs_vect[i]=0.0;


		pdr_comp_stiff_mat(Problem_id, L_int_ent_type[intent],
				   L_int_ent_id[intent], PDC_COMP_BOTH, NULL,
				   &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdof,
				   &nrdofs_int_ent, stiff_mat, rhs_vect, &rewrite);

		int offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);

		double tol = 1.e-6;
		int index_GPU; //=offset;

		for(i=0;i<num_dofs*num_dofs;i++)
		{
			index_GPU=offset+i;
			//el_data_out[index_GPU]=stiff_mat[i];

			if((fabs(stiff_mat[i]) <  tol &&
					     fabs(el_data_out[index_GPU])> tol)
					    || (fabs(stiff_mat[i]) >  tol &&
					        fabs(stiff_mat[i]-el_data_out[index_GPU])/
					        fabs(stiff_mat[i]) >  tol))
					{

					  printf("ielem=%d,index MIC %d,\tindex CPU %d,\tvalue GPU %12.6lf,\tvalue CPU %12.6lf\n",
						 ielem, index_GPU, i, el_data_out[index_GPU],
						 stiff_mat[i]);
					  getchar();
					}

		}

		//index_GPU=offset+one_el_stiff_mat_size;

		for(i=0;i<num_dofs;i++)
		{
			index_GPU=offset+one_el_stiff_mat_size+i;
			//el_data_out[index_GPU]=rhs_vect[i];
			if((fabs(rhs_vect[i]) <  tol &&
					    fabs(el_data_out[index_GPU])> tol)
					   || (fabs(rhs_vect[i]) >  tol &&
					       fabs(rhs_vect[i]-el_data_out[index_GPU])/
					       fabs(rhs_vect[i]) >  tol))
			{
				  	  printf("ielem=%d,index MIC_v %d,\tindex CPU_v %d,\tvalue GPU_v %12.6lf,\tvalue CPU_v %12.6lf\n",
				  			  ielem, index_GPU, i, el_data_out[index_GPU],
				  			  rhs_vect[i]);
				  	  getchar();
					}
			//kew*/
		}


      free(stiff_mat);
      free(rhs_vect);


    } // end loop over elements

#endif




//#ifdef TIME_TEST
//    t_begin = time_clock();
//#endif
//

//
//#ifdef TIME_TEST
//    t_end = time_clock();
//    printf("\nEXECUTION TIME: Copy back %lf\n",
//	   t_end-t_begin);
//    total_time += t_end-t_begin;
//
//#ifdef TUNING
//	if(line_count==0)
//		fprintf(headuf,"Copy back,");
//	fprintf(resuf,"%lf,",t_end-t_begin);
//#endif
//
//#endif

//printf("outside!!\n");
	      //assembling

//#define REWRITE

#ifdef REWRITE
	double *stiff_mat = (double *)malloc(max_nrdofs*max_nrdofs*sizeof(double));
	double *rhs_vect = (double *)malloc(max_nrdofs*sizeof(double));
#endif

#ifdef TIME_TEST
    t_begin = time_clock();
#endif

#ifndef REWRITE
			for(ielem=0;ielem<Nr_elems_check;ielem++)
	  		{
	  			int offset=ielem*(one_el_stiff_mat_size+one_el_load_vec_size);

//	  			//rewrite el_data_out to ielem
//	  			for(i=0;i<num_shap*num_shap;i++)
//	  			{
//	  				//printf("stiff_mat_mic[%d]=%lf\n",offset+i,el_data_out[offset+i]);
//	  				stiff_mat[i]=el_data_out[offset+i];
//	  			}
//
////printf("\n");
//
//	  			for(i=0;i<num_shap;i++)
//	  			{
//	  				//printf("rhs_mic[%d]=%lf\n",offset+num_shap*num_shap+i,el_data_out[offset+num_shap*num_shap+i]);
//	  				rhs_vect[i]=el_data_out[offset+num_shap*num_shap+i];
//	  			}

	  			/* change the option compute SM and RHSV to rewrite SM and RHSV */
				int Comp_sm = Comp_type;
				if(Comp_sm!=PDC_NO_COMP) Comp_sm += 3;
				apr_get_stiff_mat_data(field_id, L_int_ent_id[ielem], Comp_sm, 'N',
							   pdeg, 0, &nr_dof_ent, l_dof_ent_type,
							   l_dof_ent_id, l_dof_ent_nrdof,
							   &nrdofs_int_ent, NULL, NULL);


	  			#pragma omp critical(assembling)
	  			{

	  				 pdr_assemble_local_stiff_mat(Problem_id, level_id, Comp_type,
	  					  							   nr_dof_ent, l_dof_ent_type,
	  					  							   l_dof_ent_id,l_dof_ent_nrdof,
	  					  							&el_data_out[offset], &el_data_out[offset+one_el_stiff_mat_size], &rewrite);

	  			}

	  		}
#else
			int iter;
			for(iter=0;iter<Nr_elems_check/STRIDE;iter++)
	  		{
				int elem[STRIDE];
				for(i=0;i<STRIDE;i++)
				    elem[i]=STRIDE*iter+i;
				//el_data_out[ielem[0]*(one_el_stiff_mat_size+one_el_load_vec_size)+i] = stiff_mat0[i*STRIDE];

				for(j=0;j<STRIDE;j++)
				{

//					double *stiff_mat2 = (double *)malloc(num_dofs*num_dofs*sizeof(double));
//					double *rhs_vect2 = (double *)malloc(num_dofs*sizeof(double));
//
//					double tol = 1.e-6;
//
//					pdr_comp_stiff_mat(Problem_id, L_int_ent_type[elem[j]],
//									   L_int_ent_id[elem[j]], PDC_COMP_BOTH, NULL,
//									   &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdof,
//									   &nrdofs_int_ent, stiff_mat2, rhs_vect2, &rewrite);

					for(i=0;i<num_shap*num_shap;i++)
					{
						stiff_mat[i]=el_data_out[iter*STRIDE*(one_el_stiff_mat_size+one_el_load_vec_size)+i*STRIDE+j];
//						if(iter==0)
//							printf("elem=%d,index=%d, stiff_mat=%lf\n",elem[j], iter*STRIDE*(one_el_stiff_mat_size+one_el_load_vec_size)+i*STRIDE+j,stiff_mat[i]);
//
//
//						if((fabs(stiff_mat2[i]) <  tol &&
//							 fabs(stiff_mat[i])> tol)
//							|| (fabs(stiff_mat2[i]) >  tol &&
//								fabs(stiff_mat2[i]-stiff_mat[i])/
//								fabs(stiff_mat2[i]) >  tol))
//						{
//
//						  printf("ielem=%d,index MIC %d,\tindex CPU %d,\tvalue GPU %12.6lf,\tvalue CPU %12.6lf\n",
//							 elem[j], i, i, stiff_mat[i],
//							 stiff_mat2[i]);
//						  getchar();
//						}

					}
					for(i=0;i<num_shap;i++)
					{
						rhs_vect[i]=el_data_out[iter*STRIDE*(one_el_stiff_mat_size+one_el_load_vec_size)+one_el_stiff_mat_size*STRIDE+i*STRIDE+j];
//						if((fabs(rhs_vect2[i]) <  tol &&
//								fabs(rhs_vect[i])> tol)
//							   || (fabs(rhs_vect2[i]) >  tol &&
//								   fabs(rhs_vect2[i]-rhs_vect[i])/
//								   fabs(rhs_vect2[i]) >  tol))
//							{
//							  printf("ielem=%d,index MIC_v %d,\tindex CPU_v %d,\tvalue GPU_v %12.6lf,\tvalue CPU_v %12.6lf\n",
//									  elem[j], i, i, rhs_vect[i],
//									  rhs_vect2[i]);
//							  getchar();
//							}


					}





					/* change the option compute SM and RHSV to rewrite SM and RHSV */
					int Comp_sm = Comp_type;
					if(Comp_sm!=PDC_NO_COMP) Comp_sm += 3;
					apr_get_stiff_mat_data(field_id, L_int_ent_id[elem[j]], Comp_sm, 'N',
								   pdeg, 0, &nr_dof_ent, l_dof_ent_type,
								   l_dof_ent_id, l_dof_ent_nrdof,
								   &nrdofs_int_ent, NULL, NULL);


					#pragma omp critical(assembling)
					{

						 pdr_assemble_local_stiff_mat(Problem_id, level_id, Comp_type,
														   nr_dof_ent, l_dof_ent_type,
														   l_dof_ent_id,l_dof_ent_nrdof,
														stiff_mat, rhs_vect, &rewrite);

					}
				} //end j

	  		}

#endif

#ifdef TIME_TEST
    t_end = time_clock();
    printf("\nEXECUTION TIME: Assembling %lf\n",
	   t_end-t_begin);
    total_time += t_end-t_begin;

#ifdef TUNING
	if(line_count==0)
		fprintf(headuf,"Assembling,");
	fprintf(resuf,"%lf,",t_end-t_begin);
#endif

#endif


	#ifdef TIME_TEST
		printf("\nTOTAL EXECUTION TIME: %lf\n", total_time);

#ifdef TUNING
	if(line_count==0)
	{
		fprintf(headuf,"TOTAL TIME\n");
		fclose(headuf);
	}
	fprintf(resuf,"%lf\n",total_time);
	fclose(resuf);
#endif

	#endif

//#ifdef MIC
//	#pragma offload target(mic:0) nocopy(gauss_dat_host: length(0) alloc_if(0) free_if(1)) \
//	nocopy(shape_fun_host: length(0) alloc_if(0) free_if(1)) nocopy(el_data_in: length(0) alloc_if(0) free_if(1))
//	{}
//#endif

	//free(shape_fun_host);
	//free(el_data_in);
	//free(el_data_out);

	_mm_free(shape_fun_host);
	_mm_free(el_data_in);
	_mm_free(el_data_out);

	return 1;

}

