/************************************************************************
File sih_pardiso.h - internal information for the interface with the PARDISO
                    linear solver (based on sid_lapack)

Contains:
  - constants
  - data types 
  - global variables (for the whole module)
  - function headers:                     
  sir_direct_select_solver - to return the pointer to a given solver
  sir_direct_assemble_stiff_mat - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector

------------------------------  			
History:        
        02.2002 - Krzysztof Banas, initial version (lapack)
        09.2019 - Przemek Plaszewski
*************************************************************************/

#ifndef _sih_pardiso_
#define _sih_pardiso_

#include<vector>
#include<map>


/*** DATA TYPES ***/
typedef struct{
  int mtype;  // Matrix type
  int maxfct; // Maximum number of numerical factorizations.
  int mnum;   // Which factorization to use.
  int msglvl; // Print statistical information in file
  
  int iparm1; //use solver defaults?
  int iparm2; //fill-in reducing ordering.
  int iparm3; //currently is not used.
  int iparm4; //preconditioned CGS.
  int iparm5; //user permutation.
  int iparm6; //write solution on x.
  
  int iparm8; //iterative refinement step.
  int iparm9; //must be set to 0.
  int iparm10; //pivoting perturbation.
  int iparm11; //scaling vectors.
  int iparm12; //must be set to 0.
  int iparm13; //improved accuracy using (non-)symmetric weighted matchings.
  
  int iparm18; //-1
  int iparm19; //MFlops of factorization.
  
  int iparm21; //pivoting for symmetric indefinite matrices.
  
  int iparm27; //matrix checker. default is 0
  int iparm28; //single or double precision of PARDISO.
  int iparm60; //version of PARDISO.
  
  void setDefaults()
  {
    mtype = 1; // Matrix type - real, structurally symmetric
    maxfct = 1; // Maximum number of numerical factorizations.
    mnum = 1; // Which factorization to use.
    msglvl = 0; // Print statistical information in file
    
    iparm1 = 0; //use solver defaults?
    /* iparm2 = 2; //fill-in reducing ordering. */
    /* iparm3 = 1; //currently is not used. */
    /* iparm4 = 0; //preconditioned CGS. */
    /* iparm5 = 0; //user permutation. */
    /* iparm6 = 0; //write solution on x. */
    
    /* iparm8 = 0; //iterative refinement step. */
    /* iparm9 = 0; //must be set to 0. */
    /* iparm10 = 13; //pivoting perturbation. */
    /* iparm11 = 1; //scaling vectors. */
    /* iparm12 = 0; //must be set to 0. */
    /* iparm13 = 1; //improved accuracy using (non-)symmetric weighted matchings. */
    
    /* iparm18 = -1; //-1 */
    /* iparm19 = 0; //MFlops of factorization. */
    
    /* iparm21 = 1; //pivoting for symmetric indefinite matrices. */
    
    /* iparm27 = 0; //matrix checker. default is 0 */
    /* iparm28 = 0; //single or double precision of PARDISO. */
    /* iparm60 = 0; //version of PARDISO. */
  }
} sit_pardiso_config;


/* dof structure with data useful for creating flexible interfaces*/
/*  between FEM code and different solvers */
typedef struct{
  int dof_ent_type;  /* type of the associated FEM code (mesh) entity */
  int dof_ent_id;    /* ID of the associated FEM code (mesh) entity */
  int nr_int_ent; /* number of  integration entities providing SMs and LVs*/
  int l_int_ent_index[SIC_MAX_INT_PER_DOF]; 
                 /* list of integration entities providing SMs and LVs*/
  int nrdofs;      /* number of DOFs */
  int posglob;    /* position in a global stiffness matrix */
  int nrneig;     /* number of neighboring DOF structures */
  int l_neig[SIC_MAX_DOF_STR_NGB];  /* list of neighboring DOF structures */
}  sit_direct_dof_struct;


/* solver data for PARDISO procedures */
typedef struct {

  int problem_id;   /* ID of the problem associated with the solver */
  int nr_int_ent;   /* number of integration entities - entities that */
                    /* provide solver with stiffness matrices and load vectors*/
  int nr_dof_ent;   /* number of dof entities - mesh entities with which */
                    /* degrees of freedom are associated */
  int nrdofs_glob;     /* the global number of degrees of freedom */
  int max_dofs_int_ent; /* maximal number of dofs per integration entity, i.e. */
                       /* maximal size of the local stiffness matrix */

/* arrays for assembling local stiffness matrices into global stiffness matrix*/
  int* l_int_ent_type; /*list of types of entities providing local SMs and LVs */
  int* l_int_ent_id; /* list of ID's of entities providing local SMs and LVs */

  sit_direct_dof_struct *l_dof_struct; /* list of dof structures with data useful for */
                               /* creating flexible interfaces between FEM code*/
                               /* and different solvers */

  /* for each possible type of dof entity - its corresponding dof structure */
  int* l_dof_vert_to_struct;  
  int* l_dof_edge_to_struct;  
  int* l_dof_face_to_struct;  
  int* l_dof_elem_to_struct;
  /* dimensions of the above arrays */
  int max_dof_vert_id;
  int max_dof_edge_id;
  int max_dof_face_id;
  int max_dof_elem_id; 

  double *stiff_mat_crs;  /* the global crs stiffness matrix */
  double *rhs_vect;   /* the global right hand side vector */
  double *sol_vect;   /* the global solution vector */ 
  int    *aux_vect;   /* auxiliary vector for pivoting information */

  int *rows_crs;
  int *columns_crs;


} sit_direct_solver;		    


/*** GLOBAL VARIABLES (for the solver module only) ***/

extern int   siv_direct_nr_solvers;     /* the number of solvers in the problem */
extern int   siv_direct_cur_solver_id;              /* ID of the current solver */
extern sit_direct_solver siv_direct_solvers[SIC_MAX_NUM_SOLV];     /* array of solvers */
extern sit_pardiso_config siv_pardiso_config; //PARDISO control data


/*** AUXILIARY LOCAL PROCEDURES ***/

/**-----------------------------------------------------------
  sir_direct_select_solver - to return the pointer to a given solver
------------------------------------------------------------*/
extern sit_direct_solver* sir_direct_select_solver( /* returns: pointer to solver */
  int Solver_id    /* in: solver identification */
  );


/**-----------------------------------------------------------
  sir_direct_assemble_stiff_mat - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
------------------------------------------------------------*/
extern int sir_direct_assemble_stiff_mat( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Comp_type,         /* in: indicator for the scope of computations: */
                         /*   SIC_SOLVE - solve the system */
                         /*   SIC_RESOLVE - resolve for the new rhs vector */
  int Nr_dof_bl,         /* in: number of global dof blocks */
                         /*     associated with the local stiffness matrix */
  int* L_bl_nrdofs,       /* in: list of dof blocks' nr dofs */
  int* L_bl_posglob,     /* in: list of blocks' global positions */
  double* Stiff_mat,     /* in: stiffness matrix stored columnwise */
  double* Rhs_vect,      /* in: rhs vector */
  char* Rewr_dofs,         /* in: flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */

  std::vector<std::map<int, double> > &dov_matrix
  );

/**-----------------------------------------------------------
readPardisoConfig - reads PARDISO config file into sit_pardiso_config
------------------------------------------------------------*/
extern bool readPardisoConfig(sit_pardiso_config &config, const char* filePath);

#endif

