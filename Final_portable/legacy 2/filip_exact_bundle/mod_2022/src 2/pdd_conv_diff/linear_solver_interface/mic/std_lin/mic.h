#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>

#include <omp.h>

#ifndef WRAP

	#include "tmh_intf.h"

	/* interface for all mesh manipulation modules */
	#include "mmh_intf.h"

	/* interface for all approximation modules */
	#include "aph_intf.h"

	#ifdef PARALLEL
	  /* interface for parallel mesh manipulation modules */
	  #include "mmph_intf.h"

	  /* interface with parallel communication library */
	  #include "pch_intf.h"
	#endif

	/* interface for general purpose utilities - for all problem dependent modules*/
	#include "uth_intf.h"

	/* interface for linear algebra packages */
	#include "lin_alg_intf.h"

	/* interface for control parameters and some general purpose functions */
	/* from problem dependent module */
	#include "pdh_control_intf.h"

	#include "pdh_intf.h"

#endif

//#include "ittnotify.h"

#define TIME_TEST
#ifdef TIME_TEST
double t_begin;
double t_end;
double total_time;
#endif

#define TUNING

//#define MIC
#define ALIGN 32
// SWITCH 1: float versus double (MUST BE COMPATIBLE WITH KERNEL SWITCH!!!!)
// data type for integration
//#define SCALAR float
#define SCALAR double

//----------------------------------------------------
	// TWO MASTER SWITCHES (float<->double, work_group_size)
	//#define FLOAT
	#ifdef FLOAT
	  #define SCALAR float
	  #define zero 0.0f
	  #define one 1.0f
	  #define two 2.0f
	  #define half 0.5f
	  #define one_fourth 0.25f
	  #define one_sixth (0.16666666667f)
	#else
	  #define SCALAR double
	  #define zero 0.0
	  #define one 1.0
	  #define two 2.0
	  #define half 0.5
	  #define one_fourth 0.25
	  #define one_sixth (0.166666666666666667)
	#endif

// Less important switches - hacks for specific versions of kernels
// SWITCH 3: generic conv-diff (with plenty of coeffcients) versus Laplace
//#define GENERIC_CONV_DIFF
//#define LAPLACE
// artificial example - coefficients constant for all integration points
//#define TEST_SCALAR

#define weight_linear_prism (one_sixth)
	//#define weight_gauss weight_linear_prism

#define weight_linear_tetra (one_fourth*one_sixth)
	//#define weight_gauss weight_linear_tetra

#ifdef TUNING
    FILE *resuf;
    FILE *headuf;  //header file only for result titles
    //#define COUNT_OPER
	#ifdef MIC
		__attribute__((target(mic))) unsigned int line_count;
	#else
		unsigned int line_count;
	#endif

#endif

//		//ugly global values
//
//		SCALAR gauss_dat_host[1344] __attribute__((aligned(ALIGN)));
//		SCALAR shape_fun_host[192] __attribute__((aligned(ALIGN)));
//		SCALAR el_data_in[2738176] __attribute__((aligned(ALIGN)));
//		SCALAR el_data_out[7041024] __attribute__((aligned(ALIGN)));
//
//		//Align - After:shape_fun_host_size=144
//		//Align - After:d+res=28,size=2738176,el_data_in_bytes=21905408
//
//		//el_data_out_bytes=56328192
//		// - size_out=4302848




//#define REGISTERS
//#define COMPUTE_ALL_SHAPE_FUN_DER

#define LOAD_VEC_COMP

extern int pdr_num_int_el_QSS_prism(
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
	);

extern int pdr_num_int_el_QSS_tetra(
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
	);
