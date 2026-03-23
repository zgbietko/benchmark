/*************************************************************
lsh_mkb_intf.h - interface of the Multigrid iterative solver or Multigrid
   preconditioned Krylow methods solver (currently GMRES only)
   based on Block storage of the stiffness matrix and Block versions
   of standard iterative methods (Jacobi, Gauss-Seidel, additive Schwarz,
   multiplicative Schwarz)
   The file contains the provided interface with the headers of routines
   called from the FEM code

   The solver can handle several instances of solver data, each with
   a different set of control parameters, system matrix, etc.
   Hence it can be used to solve coupled problems (also transient)
   with two or more finite element problems solved simultaneously

   The actual solution phase (lsr_mkb_solve procedure) has a parameter
   that controls whether the solver is a distributed memory parallel solver
   or a standard sequential solver. In the first case the solver calls
   the finite element external part to perform three operations:
       fem_vec_norm - to compute a global (inter-processor) vector norm
       fem_sc_prod - to compute a global (inter-processor) scalar product
       fem_exchange_dofs - to exchange the values of degrees of freedom
                      between subdomains (in overlapping Schwarz manner)
   For multigrid solution the solver calls the finite element part with
       fem_proj_sol_lev - to project solution between levels (grids)
   The required interface of the solver containing headers of the
   procedures above is defined in the file "it_bliter_fem_intf.h"

Contents (declarations of the following routines):
  lsr_mkb_init - to create a new solver instance, read its control parameters
             and initialize its data structure
  lsr_mkb_create_matrix - to allocate space for a global system matrix
  lsr_mkb_create_precon - to create preconditioner blocks corresponding
                       to small subdomains of neighboring elements
  lsr_mkb_clear_matrix - to initialize block structure of system matrix
  lsr_mkb_assemble_local_sm - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
  lsr_mkb_fill_precon - to prepare preconditioner by factorizing the stiffness 
                    matrix, either only diagonal blocks or block ILU(0)
  lsr_mkb_solve - to solve a system of equations, given previously constructed
             system matrix, preconditioner
  lsr_mkb_free_matrix - to free space for solver data structure (system
                       matrix, preconditioner and possibly other)
  lsr_mkb_destroy - to destroy a particular instance of the solver
  lsr_get_pdeg_coarse - to get enforced pdeg for the coarse mesh

History:
        08.2013 - Krzysztof Banas, initial version

*************************************************************/
	  

#ifndef _lsh_mkb_intf_
#define _lsh_mkb_intf_

#include "./include/it_mkb.h"
//#include "./include/lah_block.h"

#ifdef __cplusplus
extern "C" {
#endif

/*** Parameters ***/

#define LSC_MKB_SOLVE     ITC_SOLVE
#define LSC_MKB_RESOLVE   ITC_RESOLVE

#define LSC_SEQUENTIAL    ITC_SEQUENTIAL
#define LSC_PARALLEL      ITC_PARALLEL

/* Monitoring options */
#define LSC_MKB_SILENT    ITC_SILENT
#define LSC_MKB_ERRORS    ITC_ERRORS
#define LSC_MKB_INFO      ITC_INFO
#define LSC_MKB_ALLINFO   ITC_ALLINFO

#define LSC_MKB_MAX_NUM_SOLV ITC_MAX_NUM_SOLV  /* maximal number of solvers */
#define LSC_MKB_MAX_NUM_LEV  ITC_MAX_NUM_LEV   /* maximal number of levels  */

/*------------------------------------------------------------
  lsr_mkb_init - to create a new solver instance, read its control parameters
             and initialize its data structure
------------------------------------------------------------*/
extern int lsr_mkb_init( /* returns: >0 - solver ID, <0 - error code */
  int Parallel,      /* parameter specifying sequential (LSC_SEQUENTIAL) */
                     /* or parallel (LSC_PARALLEL) execution */
  int* Nr_levels_p,  /* in: number of levels for multigrid: */
                  /*     0 - read from supplied file Filename */
                  /*     1 - enforce single level solver */
                  /*     >1 - enforce the number of levels */
                  /* out: actual number of levels !!! */
  char* Filename,  /* in: name of the file with control parameters */	    
  int Max_iter, /* maximal number of iterations, -1 for values from Filename */
  int Error_type, /* type of error norm (stopping criterion), -1 for Filename*/
  double Error_tolerance, /* value for stopping criterion, -1.0 for Filename */ 
  int Monitoring_level /* Level of output, -1 for Filename */
  );

/*---------------------------------------------------------
  lsr_mkb_create_matrix - to allocate space for a global system matrix
---------------------------------------------------------*/
extern int lsr_mkb_create_matrix( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,   /* in: solver ID (used to identify the subproblem) */
  int Level_id,    /* in: level ID */
  int Nrblocks,    /* in: number of DOF blocks */
  int Nrdof_glob,  /* in: total number of DOFs */
  int Max_sm_size, /* in: maximal size of the stiffness matrix */
  int* Nrdofbl,	   /* in: list of numbers of dofs in a block */
  int* Posglob,	   /* in: list of global numbers of first dof */
  int* Nroffbl,	   /* in: list of numbers of off diagonal blocks */
  int** L_offbl	   /* in: list of lists of off diagonal blocks */
  );

/*---------------------------------------------------------
lsr_mkb_create_precon - to create preconditioner blocks corresponding
                       to small subdomains of neighboring elements
---------------------------------------------------------*/
extern int lsr_mkb_create_precon( /* returns:   >0 number of diagonal blocks */
                          /*	       <=0 - error */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id           /* in: level ID */
  );

/*---------------------------------------------------------
  lsr_mkb_clear_matrix - to initialize block structure of system matrix
---------------------------------------------------------*/
extern int lsr_mkb_clear_matrix( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,   /* in: solver ID (used to identify the subproblem) */
  int Level_id,    /* in: level ID */
  int Comp_type    /* in: indicator for the scope of computations: */
                   /*   MKB_SOLVE - solve the system */
                   /*   MKB_RESOLVE - resolve for the new rhs vector */
  );

/*------------------------------------------------------------
  lsr_mkb_assemble_local_sm - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
------------------------------------------------------------*/
extern int lsr_mkb_assemble_local_sm( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id,          /* in: level ID */
  int Comp_type,         /* in: indicator for the scope of computations: */
                         /*   MKB_SOLVE - solve the system */
                         /*   MKB_RESOLVE - resolve for the new rhs vector */
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

  extern int lsr_mkb_show_blocks_martix( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id          /* in: level ID */

  );
  
  
/*---------------------------------------------------------
  lsr_mkb_fill_precon - to prepare preconditioner by factorizing the stiffness matrix,
                   either only diagonal blocks or block ILU(0)
---------------------------------------------------------*/
extern int lsr_mkb_fill_precon(  
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id           /* in: level ID */
  );

/*---------------------------------------------------------
  lsr_mkb_solve - to solve a system of equations, given previously constructed
             system matrix, preconditioner
---------------------------------------------------------*/
extern int amg_solve(int Solver_id, int nrdof_glob, int Ini_guess, double* X, double* B);

extern int lsr_mkb_solve( /* returns: convergence indicator: */
			/* 1 - convergence */
			/* 0 - noconvergence */
                        /* <0 - error code */
	int Solver_id,  /* in: solver ID */
	int Ndof, 	/* in: 	the number of degrees of freedom */
	int Ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* X, 	/* in: 	the initial guess */
			/* out:	the iterated solution */
        double* B,	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	int* Nr_iter, 	/* in:	the maximum iterations to be performed */
			/* out:	actual number of iterations performed */
	double* Toler, 	/* in:	tolerance level for chosen measure */
			/* out:	the final value of convergence measure */
	int Monitor,	/* in:	flag to determine monitoring level */
			/*	0 - silent run, 1 - warning messages */
			/*	2 - 1+restart data, 3 - 2+iteration data */
	double* Conv_rate /* out: convergence rate */
	);

/*---------------------------------------------------------
  lsr_mkb_free_matrix - to free space for solver data structure (system
                       matrix, preconditioner and possibly other)
---------------------------------------------------------*/
extern int lsr_mkb_free_matrix(
  int Solver_id   /* in: solver ID (used to identify the subproblem) */
  );

/*---------------------------------------------------------
  lsr_mkb_destroy - to destroy a particular instance of the solver
---------------------------------------------------------*/
extern int lsr_mkb_destroy(
  int Solver_id   /* in: solver ID (used to identify the subproblem) */
  );

/*---------------------------------------------------------
  lsr_get_pdeg_coarse - to get enforced pdeg for the coarse mesh
---------------------------------------------------------*/
extern int lsr_get_pdeg_coarse( // returns: enforced pdeg for the coarse mesh
  int Solver_id,   /* in: solver ID (used to identify the subproblem) */
  int Level_id /* in: level number */
  );

#ifdef __cplusplus
}
#endif

#endif
