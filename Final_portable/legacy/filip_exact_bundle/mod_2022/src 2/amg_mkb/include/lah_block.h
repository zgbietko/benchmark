/* lah_block.h - header for matrix storage and operations package */
/*    based on block storage and providing basic operations for */
/*    standard iterative methods (Jacobi, Gauss-Seidel, additive Schwarz, */
/*    multiplicative Schwarz)     */
/* Assumption: Each matrix is stored with the corresponding preconditioner */

/*
  it_create_blocks - to allocate space for a block structure
  it_init_blocks - to initialize block structure
  it_assemble_blocks - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
  it_create_blocks_dia - to create preconditioner blocks corresponding
                       to small subdomains of neighboring elements
  it_factor_blocks_dia - to factorize the stiffness matrix, either only
                diagonal blocks or block ILU(0)
  it_free_blocks_dia - to free space for a block structure
  it_free_blocks - to free space for a block structure

it_compres - to compute the residual of the not preconditioned 
	system of equations, v = ( b - Ax )
it_blsiter - to perform one iteration of block Gauss-Seidel
	or block Jacobi algorithm - for small blocks
it_blliter - to perform one iteration of block Gauss-Seidel
	or block Jacobi algorithm - for large blocks
it_rhsub - to perform forward reduction and back-substitution for ILU
           preconditioning
it_mfaiter - to perform matrix vector multiplication (possibly in
	matrix free manner) and additive Schwarz approximate solve
it_mfmiter - to perform one iteration of block Gauss-Seidel
 	for matrix-free GMRES

*/

#ifndef _lah_block_
#define _lah_block_


/*** External Parameters ***/

#define ITC_MAX_MATRICES 10

#define LAC_SOLVE 0
#define LAC_RESOLVE 1

/* preconditioners-smoothers  !!! MUST BE THE SAME AS IN SOLVER PACKAGE */
#define NO_PRECON	0
#define BLOCK_JACOBI 	1
#define BLOCK_GS     	2
#define ADD_SCHWARZ     3
#define MULTI_ILU       4
#define BLOCK_ILU       5

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


/*** Data types ***/

/* definition of type itt_block */
typedef struct {
/* number of degrees of freedom in a single diagonal elementary block */
  int Ndof;
/* list of neighbor blocks, Lngb[0] - number of neighbors */
  int *Lngb;
/* position of the first dof in the global rhs vector */
  int Posg;
/* array for pivoting information*/
  int *Ips;
/* stiffness matrix for diagonal elementary block */
  double *Dia;
/* RHS vector for diagonal elementary block*/
  double *Rhs;
/* stiffness matrices for off diagonal elementary blocks */
  double **Aux;
  } itt_blocks;

/* definition of type itt_blocks_dia */
typedef struct {
/* list of elementary blocks' numbers, Lsmall[0] - number of blocks */
  int *Lsmall;
/* list of elements in the subdoamin, Lelem[0] - number of elements */
  int *Lelem;
/* list of neighbors of the subdoamin, Lneig[0] - number of neighbors */
  int *Lneig;
/* list of positions of first dofs from elementary blocks in diagonal 
subarray; Lpos[0] - total number of dofs for dia */
  int *Lpos;
/* list of lower subdiagonal neighbors - for ILU preconditioning, */
/*  Llowerneig[0] - number of neighbors */
  int *Llowerneig;
/* list of upper subdiagonal neighbors - for ILU preconditioning, */
/*  Lupperneig[0] - number of neighbors */
  int *Lupperneig;
/* array for pivoting information*/
  int *Ips;
/* factorized diagonal blocks (subarray) */
  double *Dia;
/* factorized off diagonal blocks */
  double **Aux;
  } itt_blocks_dia;


typedef struct {

/* control variables */
  int Block_type; /* block types: number of nodes in a block */
		  /*   or some other indicator application dependent */
  int Precon;     /* type of preconditioner determining the storage format */
		  /*   of the preconditioner matrix Block_dia */
/* parameters */
  int Max_SM_size;      /* the maximal number of dofs in a stiffness matrix */
  int Nrblocks;		/* total number of small blocks */
  int Nrblocks_dia;	/* total number of diagonal blocks */
  int Nrdofgl;		/* total number of degrees of freedom */

/* blocks for storing system array and preconditioner arrays */
  itt_blocks **Block;	      /* array of pointers to small blocks */
  itt_blocks_dia **Block_dia; /* array of pointers to diagonal blocks */

} itt_matrices;

typedef struct {

	long matrix_size;
	long entries_number;
	double* matrix_entries;
	long* column_indices;
	long* row_indices;
	double* rhs;

} itt_matrices_crs;


/* GLOBAL VARIABLES */
extern int   itv_nr_matrices;        /* the number of solvers in the problem */
extern int   itv_cur_matrix_id;                /* ID of the current problem */
extern itt_matrices itv_matrices[ITC_MAX_MATRICES];        /* array of solvers */

extern void print_block_matrix();

extern itt_matrices_crs* lar_get_crs_matrix(itt_matrices *matrices, int matrix_id);

/*---------------------------------------------------------
  lar_allocate_SM_and_LV - to allocate space for stiffness matrix and load vector
---------------------------------------------------------*/
extern int lar_allocate_SM_and_LV( // returns: matrix index in itv_matrices array
  int Max_SM_size, /* maximal size of element stiffness matrix */
  int Nrblocks,    /* in: number of DOF blocks */
  int Nrdof_glob,  /* in: total number of DOFs */
  int* Nrdofbl,	   /* in: list of numbers of dofs in a block */
  int* Posglob,	   /* in: list of global numbers of first dof */
  int* Nroffbl,	   /* in: list of numbers of off diagonal blocks */
  int** L_offbl,   /* in: list of lists of off diagonal blocks */
  int Block_type,  /* in: number of elementary DOF blocks in a solver block */
  int Precon       /* in: type of preconditioner - see lah_block.h - line circa 45 */
  );

/*---------------------------------------------------------
  lar_initialize_SM_and_LV - to initialize stiffness matrix and/or load vector
---------------------------------------------------------*/
extern int lar_initialize_SM_and_LV(
  int Matrix_id,   /* in: matrix ID */
  int Comp_type    /* in: indicator for the scope of computations: */
                   /*   ITC_SOLVE - solve the system */
                   /*   ITC_RESOLVE - resolve for the new rhs vector */
  );

/*---------------------------------------------------------
  lar_get_storage - to compute storage of SM, LV and preconditioner
---------------------------------------------------------*/
extern double lar_get_storage( /* returns: storage in MB */
  int Matrix_id   /* in: matrix ID */
			);

/*------------------------------------------------------------
  lar_assemble_SM_and_LV - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
------------------------------------------------------------*/
extern int lar_assemble_SM_and_LV( 
                         /* returns: >=0 - success code, <0 - error code */
  int Matrix_id,   /* in: matrix ID */
  int Comp_type,         /* in: indicator for the scope of computations: */
                         /*   ITC_SOLVE - solve the system */
                         /*   ITC_RESOLVE - resolve for the new rhs vector */
  int Nr_dof_bl,         /* in: number of global dof blocks */
                         /*     associated with the local stiffness matrix */
  int* L_bl_id,          /* in: list of dof blocks' IDs */
  int* L_bl_nrdof,       /* in: list of blocks' numbers of dof */
  double* Stiff_mat,     /* in: stiffness matrix stored columnwise */
  double* Rhs_vect,      /* in: rhs vector */
  char* Rewr_dofs         /* in: flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
  );


/*---------------------------------------------------------
lar_allocate_preconditioner - to allocate space for preconditioner 
---------------------------------------------------------*/
extern int lar_allocate_preconditioner( /* returns:   >0 number of diagonal blocks */
                          /*	       <=0 - error */
  int Matrix_id   /* in: matrix ID */
  );

/*---------------------------------------------------------
  lar_fill_preconditioner - to fill preconditioner
---------------------------------------------------------*/
extern int lar_fill_preconditioner( 
  int Matrix_id   /* in: matrix ID */
	);

/*---------------------------------------------------------
  lar_free_preconditioner - to free space for a block structure
---------------------------------------------------------*/
extern int lar_free_preconditioner(
  int Matrix_id   /* in: matrix ID */
  );

/*---------------------------------------------------------
  lar_free_SM_and_LV - to free space for a block structure
---------------------------------------------------------*/
extern int lar_free_SM_and_LV(
  int Matrix_id   /* in: matrix ID */
  );


/*---------------------------------------------------------
lar_compute_residual - to compute the residual of the not preconditioned 
	system of equations, v = ( b - Ax )
---------------------------------------------------------*/
extern void lar_compute_residual ( 
  int Matrix_id,   /* in: matrix ID */
  int Use_rhs,	/* in: indicator whether to use RHS */
  int Ini_zero,	/* in: flag for zero initial guess */ 
  int Ndof, 	/* in: number of unknowns (components of x) */
  double* X, 	/* in: initial guess vector */
  double* B,	/* in:  the rhs vector, if NULL take rhs */
                /*      from block data structure */
  double* V 	/* out: initial residual, v = M^-1*(b-Ax) */
				   );

/*---------------------------------------------------------
lar_perform_BJ_or_GS_iterations - to perform one iteration of block Gauss-Seidel
	or block Jacobi algorithm
     v_out = v_in + M^-1 * ( b - A * v_in )
---------------------------------------------------------*/
extern void lar_perform_BJ_or_GS_iterations(
  int Matrix_id,   /* in: matrix ID */
  int Use_rhs,	/* in: 0 - no rhs, 1 - with rhs */
  int Ini_zero,	/* in: flag for zero initial guess */ 
  int Nr_prec,  /* in: number of preconditioner iterations */
  int Ndof,	/* in: number of unknowns (components of v*) */ 
  double* V,	/* in,out: vector of unknowns updated */
                /* during the loop over subdomains */
  double* B	/* in:  the rhs vector, if NULL take rhs */
                /*      from block data structure */
	);


/*---------------------------------------------------------
lar_perform_rhsub - to perform forward reduction and back-substitution for ILU
           preconditioning
---------------------------------------------------------*/
extern void lar_perform_rhsub(
  int Matrix_id,   /* in: matrix ID */
  int Ndof,	   /* in: number of unknowns (components of v*) */ 
  double* V,	   /* in,out: vector of unknowns updated */
                   /* during the loop over subdomains */
  double* B	   /* in:  the rhs vector, if NULL take rhs */
                   /*      from block data structure */
	);

////////////////////////////////////////////////////////

// internal utilities
/*
lar_util_dvector - to allocate a double vector: name[0..ncom-1]:
lar_util_ivector - to allocate an integer vector: name[0..ncom-1]:
lar_util_imatrix - to allocate an integer matrix name[0..nrow-1][0..ncol-1]: 
                  name=imatrix(nrow,ncol,error_text) 
lar_util_dmatrix - to allocate a double matrix name[0..nrow-1][0..ncol-1]: 
                  name=imatrix(nrow,ncol,error_text) 
lar_util_chk_list - to check whether a number is on the list
lar_util_put_list - to put Num on the list List with length Ll 
lar_util_d_zero - to zero a double vector
lar_util_i_zero - to zero an integer vector
lar_util_sort - to heap-sort an array
lar_util_dgetrf - quasi-LU decomposition of a matrix
lar_util_dgetrs - to perform forward reduction and back substitution
    of the RHS vector for solving a system of linear equations
*/


extern double *lar_util_dvector( 
	/* return: pointer to allocated vector */
	int Ncom,  	/* in: number of components */
	char Error_text[]/* in: error text to be printed */
	);

extern int *lar_util_ivector(    
	/* return: pointer to allocated vector */
	int Ncom, 	/* in: number of components */
	char Error_text[]/* in: error text to be printed */
	);

extern int **lar_util_imatrix( /* returns: pointer to array of pointers to integers */
	int Nrow, 	/* in: number of rows */
	int Ncol, 	/* in: number of columns */
	char Error_text[]/* in: text to print in case of error */
	);

extern double **lar_util_dmatrix( /* returns: pointer to array of pointers to doubles */
	int Nrow, 	/* in: number of rows */
	int Ncol, 	/* in: number of columns */
	char Error_text[]/* in: text to print in case of error */
	);

extern int lar_util_chk_list(	/* returns: */
			/* >0 - position on the list */
            		/* 0 - not found on the list */
	int Num, 	/* number to be checked */
	int* List, 	/* list of numbers */
	int Ll		/* length of the list */
	);

extern int lar_util_put_list( /* returns*/
		/*  >0 - position already occupied on the list */
             	/*   0 - put on the list */
            	/*  -1 - list full, not found on the list */
	int Num, 	/* in: number to put on the list */
	int* List, 	/* in: list */
	int Ll		/* in: total list's lengths */
	);

extern void lar_util_d_zero(double *Vec, int Num);

extern void lar_util_i_zero(int *Vec, int Num);

extern void lar_util_sort(
   int    *Ind_array,    /* in/out: index array for sorting */
   double *Val_array     /* in: array of values used for sorting */
   );

extern void lar_util_dgetrf(double* a, int m, int* ips);

extern void lar_util_dgetrs(double* a, int m, double* b, double* x, int* ips);



// kept for sentimental reasons and bleak outlooks for the future - matrix free approach
/* extern void it_mfaiter( */
/*         itt_levels* It_level,/\* in: pointer to current level data structure *\/ */
/* 	int Use_rhs,	/\* in: 0 - no rhs, 1 - with rhs *\/ */
/* 	int Ini_zero,	/\* in: flag for zero initial guess *\/  */
//      int Nr_prec,  /* in: number of preconditioner iterations */
/* 	int Ndof,	/\* in: number of unknowns (components of v*) *\/  */
/* 	double* V,	/\* in,out: vector of unknowns updated *\/ */
/* 			/\* during the loop over subdomains *\/ */
/*         double* B	/\* in:  the rhs vector, if NULL take rhs *\/ */
/* 			/\*      from block data structure *\/ */
/* 	); */

/* extern void it_mfmiter( */
/*         itt_levels* It_level,/\* in: pointer to current level data structure *\/ */
/* 	int Use_rhs,	/\* in: 0 - no rhs, 1 - with rhs *\/ */
/* 	int Ini_zero,	/\* in: flag for zero initial guess *\/  */
//      int Nr_prec,  /* in: number of preconditioner iterations */
/* 	int Ndof,	/\* in: number of unknowns (components of v*) *\/  */
/* 	double* V,	/\* in,out: vector of unknowns updated *\/ */
/* 			/\* during the loop over subdomains *\/ */
/*         double* B	/\* in:  the rhs vector, if NULL take rhs *\/ */
/* 			/\*      from block data structure *\/ */
/* 	); */


#endif
