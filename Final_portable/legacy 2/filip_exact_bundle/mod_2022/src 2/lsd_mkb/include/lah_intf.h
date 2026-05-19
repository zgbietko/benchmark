/* lah_intf.h - header for matrix storage and operations package */
/* Assumption: Each matrix is stored with the corresponding preconditioner */

/*
  lar_allocate_SM_and_LV - to allocate space for stiffness matrix and load vector
  lar_initialize_SM_and_LV - to initialize stiffness matrix and/or load vector
  lar_get_storage - to compute storage of SM, LV and preconditioner
  lar_assemble_SM_and_LV - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
lar_allocate_preconditioner - to allocate space for preconditioner 
  lar_fill_preconditioner - to fill preconditioner
  lar_free_preconditioner - to free space for a block structure
  lar_free_SM_and_LV - to free space for a block structure

lar_compute_residual - to compute the residual of the not preconditioned 
	system of equations, v = ( b - Ax )
lar_perform_BJ_or_GS_iterations - to perform one iteration of block Gauss-Seidel
	or block Jacobi algorithm
     v_out = v_in + M^-1 * ( b - A * v_in )
lar_perform_rhsub - to perform forward reduction and back-substitution for ILU
           preconditioning


*/

#ifndef _lah_intf_
#define _lah_intf_

/*** External Parameters ***/

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



/**--------------------------------------------------------
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

/**--------------------------------------------------------
  lar_initialize_SM_and_LV - to initialize stiffness matrix and/or load vector
---------------------------------------------------------*/
extern int lar_initialize_SM_and_LV(
  int Matrix_id,   /* in: matrix ID */
  int Comp_type    /* in: indicator for the scope of computations: */
                   /*   ITC_SOLVE - solve the system */
                   /*   ITC_RESOLVE - resolve for the new rhs vector */
  );

/**--------------------------------------------------------
  lar_get_storage - to compute storage of SM, LV and preconditioner
---------------------------------------------------------*/
extern double lar_get_storage( /* returns: storage in MB */
  int Matrix_id   /* in: matrix ID */
			);

/**-----------------------------------------------------------
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


/**--------------------------------------------------------
lar_allocate_preconditioner - to allocate space for preconditioner 
---------------------------------------------------------*/
extern int lar_allocate_preconditioner( /* returns:   >0 number of diagonal blocks */
                          /*	       <=0 - error */
  int Matrix_id   /* in: matrix ID */
  );

/**--------------------------------------------------------
  lar_fill_preconditioner - to fill preconditioner
---------------------------------------------------------*/
extern int lar_fill_preconditioner( 
  int Matrix_id   /* in: matrix ID */
	);

/**--------------------------------------------------------
  lar_free_preconditioner - to free space for a block structure
---------------------------------------------------------*/
extern int lar_free_preconditioner(
  int Matrix_id   /* in: matrix ID */
  );

/**--------------------------------------------------------
  lar_free_SM_and_LV - to free space for a block structure
---------------------------------------------------------*/
extern int lar_free_SM_and_LV(
  int Matrix_id   /* in: matrix ID */
  );


/**--------------------------------------------------------
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

/**--------------------------------------------------------
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


/**--------------------------------------------------------
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


#endif
