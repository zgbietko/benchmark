/* it_bliter.h - internal data of the multigrid iterative solver */
/*    or multigrid preconditioned Krylow methods solver (currently GMRES only) */
/*    based on block storage of the stiffness matrix and block versions */
/*    of standard iterative methods (Jacobi, Gauss-Seidel, additive Schwarz, */
/*    multiplicative Schwarz): definition of parameters,      */
/*    data types, global variables and external functions     */

#ifndef _it_bliter_
#define _it_bliter_

/*** External Parameters ***/

/* Scope of calculations */
#define ITC_SOLVE    0
#define ITC_RESOLVE  1

#define ITC_SEQUENTIAL 0
#define ITC_PARALLEL   1

/* Monitoring options */
#define  ITC_SILENT      0
#define  ITC_ERRORS      1
#define  ITC_INFO        2
#define  ITC_ALLINFO     3

/* Parameters */
#define ITC_MAX_NUM_LEV  10
#define ITC_MAX_NUM_SOLV 10


/* BLAS and LAPACK names with or without underscore */
#ifdef WITHOUT_
#define ddot_ ddot
#define dnrm2_ dnrm2
#define dscal_ dscal
#define dcopy_ dcopy
#define daxpy_ daxpy
#define dgemv_ dgemv
#define dgetrf_ dgetrf
#define dgetrs_ dgetrs
#define drot_ drot
#define drotg_ drotg
#define dtrsv_ dtrsv
#endif


/* solvers */
#define DIRECT       0
#define GMRES        1
#define MULTI_GMRES  2
#define STANDARD_IT 10
#define MULTI_GRID  20

/* GMRES type */
#define STANDARD_GMRES	0
#define MATRIX_FREE	1

/* preconditioners-smoothers !!! MUST BE THE SAME AS IN LA PACKAGE */
#define NO_PRECON	0
#define BLOCK_JACOBI 	1
#define BLOCK_GS     	2
#define ADD_SCHWARZ     3
#define MULTI_ILU       4
#define BLOCK_ILU       5


/* Types of convergence measures */
#define REL_RES_INI	0
#define ABS_RES		1
#define REL_RES_RHS	2


/*** Data types ***/


/* definition of type itt_levels - data structure for mesh levels */
/*	and associated solvers */
typedef struct {

/* control variables */
  int Solver;	/* linear equations solver: */
		/*	0 - direct solver */
		/*	1 - GMRES */
		/*	2 - multi-level GMRES */
  int GMRES_type;/* type of GMRES */
		/*	0 - standard */
		/*	1 - matrix free */
  int Krylbas;	/* number of Krylov space vectors for GMRES */
  int Pdeg;	/* indicator for choosing degree of approximation for coarse */
                /* meshes (if >=0 indicates the actual degree of approximation */
  int Precon;  	/* preconditioning type */
		/*	0 - block Jacobi */
		/*	1 - block Gauss-Seidel */
		/*	2 - additive Schwarz (BJ with global product) */
  int Nr_prec;	/* number of block iterations for preconditioning */
  int Block_type;/* block types: number of nodes in a block */
		/*    or some other indicator application dependent */
  int Max_iter;	/* maximum number of GMRES iterations to be performed */
  int Conv_type; /* residual convergence criterium: */
		/* 	0 - relative to initial residual */
		/* 	1 - absolute residual */
		/* 	2 - relative to rhs */
  double Conv_meas;	/* allowable convergence measure */
  int Monitor; 	/* monitoring flag: */
		/*	0 - silent run, 1 - warning messages */
		/*	2 - 1+restart data 3 - iteration data */

  int Nrdofgl;		/* total number of degrees of freedom */
  int SM_and_LV_id; // identifier of global stiffness matrix assigned by linear algebra package

} itt_levels;


/* definition of itt_solvers - data type for multi-level iterative solver */
typedef struct {

  int solver_id;           /* solver_id */
  int parallel;            /* parameter specifying sequential (ITC_SEQUENTIAL) */
                           /* or parallel (ITC_PARALLEL) execution */
  int nr_level;	           /* number of levels in multi-level GMRES */
  int cur_level;           /* current level number in multi-level GMRES */
  itt_levels level[ITC_MAX_NUM_LEV];    /* array of solver data structures */
                           /* corresponding to different levels */
} itt_solvers;		    


/* GLOBAL VARIABLES */
extern int   itv_nr_solvers;        /* the number of solvers in the problem */
extern int   itv_cur_solver_id;                /* ID of the current problem */
extern itt_solvers itv_solver[ITC_MAX_NUM_SOLV];        /* array of solvers */


/* PROCEDURES */

/* in file "it_gmres.c":
it_gmres - to solve a system of linear equations Ax = b
	using the left preconditioned GMRES method 
*/ 
extern int it_gmres(	/* returns: convergence indicator: */
			/* 1 - convergence */
			/* 0 - noconvergence */
	int Solver_id,      /* in: solver ID */
	int Ndof, 	/* in: 	the number of degrees of freedom */
	int Ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* X, 	/* in: 	the initial guess */
			/* out:	the iterated solution */
        double* B,	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	int Krylbas, 	/* in: 	number of Krylov space vectors */
	int*  Iter, 	/* in:	the maximum GMRES iterations to be performed */
			/* out:	actual number of iterations performed */
	double* Resid, 	/* in:	tolerance level for residual */
			/* out:	the final value of residual */
	double* Rel_res, /* in:	tolerance level for relative residual */
			/* out:	the final value of relative residual */
	double* Rhs_res, /* in:	tolerance level for ratio rhs/residual */
			/* out:	the final value of the ratio rhs/residual */
	int Monitor,	/* in:	flag to determine monitoring level */
			/*	0 - silent run, 1 - warning messages */
			/*	2 - 1+restart data, 3 - 2+iteration data */
	double* Pconvr	/* out: convergence rate */
	);

/* in file "it_bliter_core.c":
it_comp_norm_rhs - to compute the norm of the preconditioned rhs 
it_standard - to solve the problem by standard iterations
it_vcycle - to perform one V-cycle of multigrid 
it_smooth - to perform one smoothing step using different algorithms
it_multi_precon - to perform  multi-level preconditioning
it_precon - to perform preconditioning using different algorithms
it_compreres - to compute the residual of the left preconditioned 
	system of equations, v = M^-1 * ( b - Ax )
        ( used also to compute the product v = M^-1 * Ax)
it_solve_coarse - to launch solver for the coarse grid problem
*/

extern double it_comp_norm_rhs ( /* return: the norm of the rhs vector */
	int Solver_id      /* in: solver ID */
	);

extern int it_standard( /* returns: convergence indicator: */
			/* 1 - convergence */
			/* 0 - noconvergence */
	int Solver_id,      /* in: solver ID */
	int Ndof, 	/* in: 	the number of degrees of freedom */
	int Ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* X, 	/* in: 	the initial guess */
			/* out:	the iterated solution */
        double* B,	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	int*  Iter, 	/* in:	the maximum iterations to be performed */
			/* out:	actual number of iterations performed */
	double* Toler, 	/* in:	tolerance level for max norm of update */
			/* out:	the final value of max norm of update */
	int Monitor,	/* in:	flag to determine monitoring level */
			/*	0 - silent run, 1 - warning messages */
			/*	2 - 1+restart data, 3 - 2+iteration data */
	double* Pconvr	/* out: convergence rate */
	);

extern void it_vcycle ( 
	int Solver_id,      /* in: solver ID */
	int Ilev,	/* in: index of the mesh (level) */
	int Use_rhs,	/* in: flag for considering RHS */ 
	int Ini_zero,	/* in: flag for zero initial guess */ 
	double* X, 	/* in/out: initial guess vector and solution */
        double* B	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	);

extern void it_smooth( 
	int Solver_id,      /* in: solver ID */
	int Ilev,	/* in: index of the mesh (level) */
	int Use_rhs,	/* in: flag for considering RHS */ 
	int Ini_zero,	/* in: flag for zero initial guess */ 
	double* X, 	/* in/out: initial guess vector and solution */
        double* B	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	);

extern void it_multi_precon ( 
	int Solver_id,      /* in: solver ID */
	int Ilev,	/* in: index of the mesh (level) */
	double* X, 	/* in/out: initial guess vector and solution */
        double* B	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	);

extern void it_precon( 
	int Solver_id,      /* in: solver ID */
	int Ilev,	/* in: index of the mesh (level) */
	double* X, 	/* in/out: initial guess vector and solution */
        double* B	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	);

extern void it_compreres ( 
        int Solver_id,      /* in: pointer to solver data structure to be passed
	                       to data structure dependent routines */
	int Control,	/* in: indicator whether to compute residual (1) 
			       or matrix-vector product (0)  */
	int Ini_zero,	/* in: flag for zero input vector X */
	int Ndof, 	/* in: number of unknowns (components of X and V) */
	double* X, 	/* in: input vector */
        double* B,	/* in: the rhs vector, if NULL take rhs */
			/*     from block data structure */
	double* V 	/* out: output vector, V = M^-1 * ( B - A*X ) */
	);


extern int it_solve_coarse( /* returns: 1 - success; <=0 - error code*/
	int Solver_id,      /* in: solver ID */
	int Ilev,	/* in: index of the mesh (level) */
	int Ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* X,	/* in/out: initial guess and solution vector */
	double* B	/* in: rhs vector */
	);


/* in file "it_bliter_util.c":
it_dvector - to allocate a double vector: name[0..ncom-1]:
it_ivector - to allocate an integer vector: name[0..ncom-1]:
it_imatrix - to allocate an integer matrix name[0..nrow-1][0..ncol-1]: 
                  name=imatrix(nrow,ncol,error_text) 
it_dmatrix - to allocate a double matrix name[0..nrow-1][0..ncol-1]: 
                  name=imatrix(nrow,ncol,error_text) 
it_chk_list - to check whether a number is on the list
it_put_list - to put Num on the list List with length Ll 
it_d_zero - to zero a double vector
it_i_zero - to zero an integer vector
it_sort - to heap-sort an array
it_dgetrf - quasi-LU decomposition of a matrix
it_dgetrs - to perform forward reduction and back substitution
    of the RHS vector for solving a system of linear equations
*/

extern double *it_dvector( 
	/* return: pointer to allocated vector */
	int Ncom,  	/* in: number of components */
	char Error_text[]/* in: error text to be printed */
	);

extern int *it_ivector(    
	/* return: pointer to allocated vector */
	int Ncom, 	/* in: number of components */
	char Error_text[]/* in: error text to be printed */
	);

extern int **it_imatrix( /* returns: pointer to array of pointers to integers */
	int Nrow, 	/* in: number of rows */
	int Ncol, 	/* in: number of columns */
	char Error_text[]/* in: text to print in case of error */
	);

extern double **it_dmatrix( /* returns: pointer to array of pointers to doubles */
	int Nrow, 	/* in: number of rows */
	int Ncol, 	/* in: number of columns */
	char Error_text[]/* in: text to print in case of error */
	);

extern int it_chk_list(	/* returns: */
			/* >0 - position on the list */
            		/* 0 - not found on the list */
	int Num, 	/* number to be checked */
	int* List, 	/* list of numbers */
	int Ll		/* length of the list */
	);

extern int it_put_list( /* returns*/
		/*  >0 - position already occupied on the list */
             	/*   0 - put on the list */
            	/*  -1 - list full, not found on the list */
	int Num, 	/* in: number to put on the list */
	int* List, 	/* in: list */
	int Ll		/* in: total list's lengths */
	);

extern void it_d_zero(double *Vec, int Num);

extern void it_i_zero(int *Vec, int Num);

extern void it_sort(
   int    *Ind_array,    /* in/out: index array for sorting */
   double *Val_array     /* in: array of values used for sorting */
   );

extern void it_dgetrf(double* a, int m, int* ips);

extern void it_dgetrs(double* a, int m, double* b, double* x, int* ips);



/*---------------------------------------------------------
ut_skip_rest_of_line - to allow for comments in input files
---------------------------------------------------------*/
extern void it_skip_rest_of_line(
			  FILE *Fp  /* in: input file */
			  );


/*||begin||*/

/*---------------------------------------------------------
  itr_proj_sol_lev - to L2 project solution dofs between mesh levels
---------------------------------------------------------*/
int itr_proj_sol_lev( /* returns: 1 - success; <=0 - error code*/
	int Solver_id,        /* in: solver data structure to be used */
	int Ilev_from,    /* in: level number to project from */
        double* Vec_from, /* in: vector of values to project */
	int Ilev_to,      /* in: level number to project to */
        double* Vec_to    /* out: vector of projected values */
  );

/*---------------------------------------------------------
  it_par_vec_norm - to compute a norm of global vector in parallel
---------------------------------------------------------*/
extern double it_par_vec_norm( /* returns: L2 norm of global Vector */
  int Nrdof,            /* in: number of vector components */
  double* Vector        /* in: local part of global Vector */
  );

/*---------------------------------------------------------
  it_par_sc_prod - to compute a scalar product of two global vectors 
---------------------------------------------------------*/
extern double it_par_sc_prod( 
                       /* retruns: scalar product of Vector1 and Vector2 */
  int Nrdof,           /* in: number of vector components */
  double* Vector1,     /* in: local part of global Vector */
  double* Vector2      /* in: local part of global Vector */
  );

/*---------------------------------------------------------
  it_par_exchange_dofs - to exchange dofs between processors
---------------------------------------------------------*/
extern int it_par_exchange_dofs(
  double* Vec_dofs  /* in: vector of dofs to be exchanged */
  );

/*||end||*/


#endif
