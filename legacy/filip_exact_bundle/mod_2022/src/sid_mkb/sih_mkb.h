/************************************************************************
File sih_mkb.h - internal information for the interface module
   between the iterative block based Krylow solver and the finite element
   code (the module forms part of the fem code): definition of parameters,
   data types, global variables and external functions)


Contains declarations of data types, constants and global variables 

------------------------------  			
History:        
	02.2002 - Krzysztof Banas, initial version		
*************************************************************************/

#ifndef _sih_mkb_
#define _sih_mkb_

#include "sih_intf.h"

/* provided interface of the multigrid_krylow_bliter module */
#include "../lsd_mkb/lsh_mkb_intf.h"

/*** CONSTANTS ***/
#define SIC_MAX_NUM_LEV  LSC_MKB_MAX_NUM_LEV

#define SIC_PDEG_COARSE_HIGH        -4
#define SIC_PDEG_COARSE_LOW         -3
#define SIC_PDEG_COARSE_HIGH_ALL    -2
#define SIC_PDEG_COARSE_LOW_ALL     -1
#define SIC_PDEG_FINEST            -10

/*** DATA TYPES ***/

/* dof structure with data useful for creating flexible interfaces*/
/*  between FEM code and different solvers */
typedef struct{
  int dof_ent_type;  /* type of the associated FEM code (mesh) entity */
  int dof_ent_id;    /* ID of the associated FEM code (mesh) entity */
  int nr_int_ent; /* number of  integration entities providing SMs and LVs*/
  int l_int_ent_index[SIC_MAX_INT_PER_DOF]; 
                 /* list of integration entities providing SMs and LVs*/
  int block_id;   /* ID of the associated block in the vector of unknowns */
  int nrdof;      /* number of DOFs */
  int posglob;    /* position in a global stiffness matrix */
  int nrneig;     /* number of neighboring DOF structures */
  int l_neig[SIC_MAX_DOF_STR_NGB];  /* list of neighboring DOF structures */
}  sit_dof_struct;


/* solver data structure for a single level */
typedef struct {

  int nr_int_ent;   /* number of integration entities - entities that */
                    /* provide solver with stiffness matrices and load vectors*/
  int nr_dof_ent;   /* number of dof entities - mesh entities with which */
                    /* degrees of freedom are associated */
  int nr_dof_bl;    /* the number of dof blocks - currently it is assumed */
                    /* that there is ono-to-one correspondence between dof */
                    /* entities and dof blocks, hence nr_dof_bl=nr_dof_ent */
  int pdeg_coarse;  /* degree of approximation for coarse levels */
  int nrdof_glob;     /* the global number of degrees of freedom */
  int max_dof_ent_id; /* maximal id of dof_ent */
  /* if there are more types of dof entities - for each type a separate number */
  /* should be supplied to enable finding respective dof structures */
  // int max_elem_id; int max_face_id; max_edge_id; max_vert_id;
  int max_dof_int_ent; /* maximal number of dofs per integration entity, i.e. */
                       /* maximal size of the local stiffness matrix */
  int max_dof_dof_ent; /* maximal number of dofs per dof entity */

/* arrays for assembling local stiffness matrices into global stiffness matrix*/
  int* l_int_ent_type; /*list of types of entities providing local SMs and LVs */
  int* l_int_ent_id; /* list of ID's of entities providing local SMs and LVs */

  int* l_dof_ent_to_struct; /* list of dof_struct IDs for given dof_ents */
  /* if there are more types of dof entities - for each type a separate table */
  /* should be supplied to enable finding respective dof structures */
  //int* l_vert_to_struct;  int* l_edge_to_struct;  
  //int* l_face_to_struct;  int* l_elem_to_struct;
  sit_dof_struct *l_dof_struct; /* list of dof structures with data useful for */
                               /* creating flexible interfaces between FEM code*/
                               /* and different solvers */
  /* for the simple case of DG approximation the order on the list is the same */
  /* as provided by the problem routine pdr_get_list_ent */

/* renumbering arrays */
  int* l_bl_to_struct;    /* block to structure correspondence */

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


#endif










