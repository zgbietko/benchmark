/************************************************************************
File sih_krylow_bliter.h - internal information for the interface module
   between the iterative block based Krylow solver and the finite element
   code (the module forms part of the fem code): definition of parameters,
   data types, global variables and external functions)


Contains declarations of data types, constants and global variables 

------------------------------  			
History:        
	02.2002 - Krzysztof Banas, initial version		
*************************************************************************/

#ifndef _sih_amg_
#define _sih_amg_

#include "sih_intf.h"

/* provided interface of the multigrid_krylow_bliter module */
#include "../lsd_mkb/lsh_mkb_intf.h"


#ifdef __cplusplus
extern "C" {
#endif

/*** CONSTANTS ***/

#define SIC_MAX_NUM_LEV 1

/*** DATA TYPES ***/

/* dof structure with data useful for creating flexible interfaces*/
/*  between FEM code and different solvers */
typedef struct{
  int dof_ent_type;  /* type of the associated FEM code (mesh) entity */
  int dof_ent_id;    /* ID of the associated FEM code (mesh) entity */
  int nr_int_ent; /* number of  integration entities providing SMs and LVs*/
  int l_int_ent_index[SIC_MAX_INT_PER_DOF]; 
                 /* list of integration entities providing SMs and LVs*/
  int block_id; /* ID for solver - used for renumbering */
  int nrdofs;      /* number of DOFs */
  int posglob;    /* position in a global stiffness matrix */
  int nrneig;     /* number of neighboring DOF structures */
  //* Two lists of neighbours - the order on lists may be different !!! */
  int l_neig[SIC_MAX_DOF_STR_NGB];  /* list of neighboring DOF structures */
  int l_neig_bl[SIC_MAX_DOF_STR_NGB]; /* list of IDs for solver (block IDs) */
}  sit_dof_struct;

typedef struct list {
	int row;
	struct list *next;
}list_row;

/* solver data structure for a single (or the only) level */
typedef struct {

  int nr_int_ent;   /* number of integration entities - entities that */
                    /* provide solver with stiffness matrices and load vectors*/
  int nr_dof_ent;   /* number of dof entities - mesh entities with which */
                    /* degrees of freedom are associated */
  int nrdofs_glob;     /* the global number of degrees of freedom */
  int max_dofs_int_ent; /* maximal number of dofs per integration entity, i.e. */
                       /* maximal size of the local stiffness matrix */
  int max_dofs_dof_ent; /* maximal number of dofs per dof entity */

/* arrays for assembling local stiffness matrices into global stiffness matrix*/
  int* l_int_ent_type; /*list of types of entities providing local SMs and LVs */
  int* l_int_ent_id; /* list of ID's of entities providing local SMs and LVs */

  sit_dof_struct *l_dof_struct; /* list of dof structures with data useful for */
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

} sit_levels;

/* definition of sit_solvers - data type for multi-level iterative solver */
typedef struct {

  int problem_id;          /* ID of the problem associated with the solver */
  int parallel;            /* parameter specifying sequential (SIC_SEQUENTIAL) */
                           /* or parallel (SIC_PARALLEL) execution */
  int nr_levels;	   /* number of levels in multi-level GMRES */
  int cur_level;           /* current level number in multi-level GMRES */
  sit_levels level[SIC_MAX_NUM_LEV];    /* array of solver data structures */
                           /* corresponding to different levels */
} sit_solvers;		    

/*** GLOBAL VARIABLES (for the solver module only) ***/

extern int   siv_nr_solvers;     /* the number of solvers in the problem */
extern int   siv_cur_solver_id;              /* ID of the current solver */
extern sit_solvers siv_solver[SIC_MAX_NUM_SOLV];     /* array of solvers */

#ifdef __cplusplus
}
#endif

#endif
