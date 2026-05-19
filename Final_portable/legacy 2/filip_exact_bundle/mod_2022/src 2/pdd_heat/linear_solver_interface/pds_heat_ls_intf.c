/************************************************************************
File pds_heat_ls_intf.c - interface between the problem dependent
         module and linear solver modules (direct and iterative)

Contains definitions of routines:

Implementation of pdh_intf.h:
  
  pdr_get_list_ent - to return to the solver module :
                          1. the list of integration entities - entities
                             for which stiffness matrices and load vectors are
                             provided by the FEM code
                          2. the list of DOF entities - entities with which  
                             there are dofs associated by the given approximation
  pdr_get_list_ent_coarse - the same as above but for COARSE level and
                            given the corresponding lists from the fine level 
  pdr_create_assemble_stiff_mat - to create element stiffness matrices
                                 and assemble them to the global SM
  pdr_assemble_local_stiff_mat - to assemble an element stiffness matrix
                                   to the global SM
  pdr_comp_stiff_mat - to construct a stiffness matrix and a load vector for
                      some given mesh entity
  pdr_read_sol_dofs - to read a vector of dofs associated with a given 
                   mesh entity from approximation field data structure
  pdr_write_sol_dofs - to write a vector of dofs associated with a given 
                   mesh entity to approximation field data structure
  pdr_L2_proj_sol - to project solution between elements of different generations
  pdr_renum_coeff - to return a coefficient being a basis for renumbering
  pdr_get_ent_pdeg - to return the degree of approximation index 
                      associated with a given mesh entity
  pdr_dof_ent_sons - to return a list of dof entity sons
  pdr_proj_sol_lev - to project solution between mesh levels
  pdr_vec_norm - to compute a norm of global vector in parallel
  pdr_sc_prod - to compute a scalar product of two global vectors
  pdr_create_exchange_tables - to create tables to exchange dofs 
  pdr_exchange_dofs - to exchange dofs between processors
  
  pdr_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals in weak formulation
           (the procedure indicates which terms are non-zero in weak form)

  pdr_el_coeff - to return coefficients for internal integrals

Special procedure:
  pdr_heat_give_me_velocity_at_point - to provide the velocity and its
    gradient at a particular point given its local coordinates within an element
HEAT MODULE ASKS FOR IMPLEMENTATION - it has to be provided by procedures
defined in ls_intf directory of the problem module that uses heat as submodule


------------------------------
History:
	02.2002 - Krzysztof Banas, initial version
	2011    - Przemyslaw Plaszewski
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)

*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<assert.h>

/* problem dependent module interface */
#include "pdh_intf.h"		/* IMPLEMENTS */
#include "pdh_control_intf.h"		/* IMPLEMENTS */
/* interface for all mesh manipulation modules */
#include "mmh_intf.h"		/* USES */
/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* utilities - including simple time measurement library */
#include "uth_intf.h"		/* USES */

#ifdef PARALLEL
/* interface of parallel mesh manipulation modules */
#include "mmph_intf.h"		/* USES */
/* interface for all parallel approximation modules */
#include "apph_intf.h"		/* USES */
/* interface for parallel communication modules */
#include "pch_intf.h"		/* USES */
#endif

/* problem module's types and functions */
#include "../include/pdh_heat.h"	/* USES */
/* types and functions related to problem structures */
#include "../include/pdh_heat_problem.h" 
// bc and material header files are included in problem header files
/* weakform functions */
#include "../include/pdh_heat_weakform.h"  	/* USES */


/**************************************/
/* PDH_INTF.H PROCEDURES              */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
  pdr_get_list_ent - to return to the solver module :
                          1. the list of integration entities - entities
                             for which stiffness matrices and load vectors are
                             provided by the FEM code
                          2. the list of DOF entities - entities with which  
                             there are dofs associated by the given approximation
------------------------------------------------------------*/
int pdr_get_list_ent(  /* returns: >=0 - success code, <0 - error code */
  int Problem_id,	/* in:  problem (and solver) identification */
  int *Nr_int_ent,	/* out: number of integration entitites */
  int **List_int_ent_type,	/* out: list of types of integration entitites */
  int **List_int_ent_id,	/* out: list of IDs of integration entitites */
  int *Nr_dof_ent,	/* out: number of dof entities (entities with which there
			   are dofs associated by the given approximation) */
  int **List_dof_ent_type,	/* out: list of types of integration entitites */
  int **List_dof_ent_id,	/* out: list of IDs of integration entitites */
  int **List_dof_ent_nrdofs,	/* out: list of no of dofs for 'dof' entity */
  int *Nrdofs_glob,	/* out: global number of degrees of freedom (unknowns) */
  int *Max_dofs_per_dof_ent	/* out: maximal number of dofs per dof entity */
  )
{
  
  // default procedure for returning lists of integration and dof entities
  // for standard and discontinuous Galerkin approximations
  utr_get_list_ent(Problem_id, Nr_int_ent, List_int_ent_type, List_int_ent_id, 
	 Nr_dof_ent, List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs, 
         Nrdofs_glob, Max_dofs_per_dof_ent);



  return (1);
}

/*------------------------------------------------------------
  pdr_get_list_ent_coarse - the same as above but for COARSE level and
                            given the corresponding lists from the fine level 
------------------------------------------------------------*/
int pdr_get_list_ent_coarse( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,	/* in:  problem (and solver) identification */
  int Nr_int_ent_fine,	/* in: number of integration entitites */
  int *List_int_ent_type_fine,	/* in: list of types of integration entitites */
  int *List_int_ent_id_fine,	/* in: list of IDs of integration entitites */
  int Nr_dof_ent_fine,	/* in: number of dof entities (entities with which there
			   are dofs associated by the given approximation) */
  int *List_dof_ent_type_fine,	/* in: list of types of integration entitites */
  int *List_dof_ent_id_fine,	/* in: list of IDs of integration entitites */
  int *List_dof_ent_nrdof_fine,	/* in: list of no of dofs for 'dof' entity */
  int Nrdof_glob_fine,	/* in: global number of degrees of freedom (unknowns) */
  int Max_dof_per_ent_fine,	/* in: maximal number of dofs per dof entity */
  int *Pdeg_coarse_p,	/* in: degree of approximation for coarse space */
  int *Nr_int_ent_p,	/* out: number of integration entitites */
  int **List_int_ent_type,	/* out: list of types of integration entitites */
  int **List_int_ent_id,	/* out: list of IDs of integration entitites */
  int *Nr_dof_ent_p,	/* out: number of dof entities (entities with which there
			   are dofs associated by the given approximation) */
  int **List_dof_ent_type,	/* out: list of types of integration entitites */
  int **List_dof_ent_id,	/* out: list of IDs of integration entitites */
  int **List_dof_ent_nrdofs,	/* out: list of no of dofs for 'dof' entity */
  int *Nrdof_glob,	/* out: global number of degrees of freedom (unknowns) */
  int *Max_dof_per_ent	/* out: maximal number of dofs per dof entity */
  )
{
  printf("pdr_get_list_ent_coarse NOT IMPLEMENTED!");
  exit (-1);
}


/*------------------------------------------------------------
 pdr_create_assemble_stiff_mat - to create element stiffness matrices
                                 and assemble them to the global SM
------------------------------------------------------------*/
int pdr_create_assemble_stiff_mat(
  int Problem_id, 
  int Level_id, 
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_int_ent,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
)
{

  utr_create_assemble_stiff_mat(Problem_id, Level_id, Comp_type,
				Nr_int_ent, L_int_ent_type,
				L_int_ent_id, Max_dofs_int_ent);

  return(1);
}


/*------------------------------------------------------------
  pdr_assemble_local_stiff_mat - to assemble an element stiffness matrix
                                   to the global SM
------------------------------------------------------------*/
int pdr_assemble_local_stiff_mat( 
                         /* returns: >=0 - success code, <0 - error code */
  int Problem_id,        /* in: solver ID (used to identify the subproblem) */
  int Level_id,          /* in: level ID */
  int Comp_type,         /* in: indicator for the scope of computations: */
                         /*   MKB_SOLVE - solve the system */
                         /*   MKB_RESOLVE - resolve for the new rhs vector */
  int Nr_dof_ent,        /* in: number of global dof blocks */
                         /*     associated with the local stiffness matrix */
  int* L_dof_ent_type,   /* in: list of dof blocks' IDs */
  int* L_dof_ent_id,     /* in: list of dof blocks' IDs */
  int* L_dof_ent_nrdofs, /* in: list of blocks' numbers of dof */
  double* Stiff_mat,     /* in: stiffness matrix stored columnwise */
  double* Rhs_vect,      /* in: rhs vector */
  char* Rewr_dofs        /* in: flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
)
{
  int i=6;
  int solver_id = pdr_ctrl_i_params(Problem_id,i);

  sir_assemble_local_stiff_mat(solver_id, Level_id, Comp_type,
			       Nr_dof_ent, L_dof_ent_type,
			       L_dof_ent_id,L_dof_ent_nrdofs, 
			       Stiff_mat, Rhs_vect, Rewr_dofs);

  return(1);

}

/*------------------------------------------------------------
  pdr_read_sol_dofs - to read a vector of dofs associated with a given
                   mesh entity from approximation field data structure
------------------------------------------------------------*/
int pdr_read_sol_dofs( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,	/* in: solver ID (used to identify the subproblem) */
  int Dof_ent_type, int Dof_ent_id, int Nrdof, double *Vect_dofs	
                        /* in: dofs to be written */
    )
{

  int i, field_id;
  int vect_id = 0;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/*kbw
  printf("in pdr_read_sol_dofs before apr_read_ent_dofs\n");
  printf("problem_id %d, Dof_ent_type %d, Dof_ent_id %d, nrdof %d\n",
  Problem_id, Dof_ent_type, Dof_ent_id,  Nrdof);
/*kew */

  i=3; field_id=pdr_ctrl_i_params(Problem_id,i);
  apr_read_ent_dofs(field_id, Dof_ent_type, Dof_ent_id, 
		    Nrdof, vect_id, Vect_dofs);

/*kbw
  printf("in pdr_read_sol_dofs after apr_read_ent_dofs\n");
  for(i=0;i<Nrdof;i++){
  printf("%10.6lf",Vect_dofs[i]);
  }
  printf("\n");
/*kew */


  return (1);
}

/*------------------------------------------------------------
  pdr_write_sol_dofs - to write a vector of dofs associated with a given
                   mesh entity to approximation field data structure
------------------------------------------------------------*/
int pdr_write_sol_dofs(	/* returns: >=0 - success code, <0 - error code */
  int Problem_id,	/* in: solver ID (used to identify the subproblem) */
  int Dof_ent_type, int Dof_ent_id, int Nrdof, 
  double *Vect_dofs	/* in: dofs to be written */
			)
{
  int vect_id = 0;
  
  int field_id=pdr_ctrl_i_params(Problem_id,3);
  apr_write_ent_dofs(field_id, Dof_ent_type, Dof_ent_id, 
		     Nrdof, vect_id, Vect_dofs);
  
  return (1);
}

/*---------------------------------------------------------
  pdr_L2_proj_sol - to project solution between elements of different generations
----------------------------------------------------------*/
int pdr_L2_proj_sol(
  int Problem_id,	/* in: problem ID */
  int El,	/* in: element number */
  int *Pdeg,	/* in: element degree of approximation */
  double *Dofs,	/* out: workspace for degress of freedom of El */
  /*    NULL - write to  data structure */
  int *El_from,	/* in: list of elements to provide function */
  int *Pdeg_from,	/* in: degree of polynomial for each El_from */
  double *Dofs_from	/* in: Dofs of El_from or... */
		    )
{
  
  int field_id, i;		

  i=3; field_id=pdr_ctrl_i_params(Problem_id,i);
  i = -1; 	/* mode: -1 - projection from father to son */
  apr_L2_proj(field_id, i, El, Pdeg, Dofs, El_from, Pdeg_from, Dofs_from, NULL);

  return (0);
}

/*---------------------------------------------------------
pdr_renum_coeff - to return a coefficient being a basis for renumbering
----------------------------------------------------------*/
int pdr_renum_coeff(
  int Problem_id,	/* in: problem ID */
  int Ent_type,	/* in: type of mesh entity */
  int Ent_id,	/* in: mesh entity ID */
  double *Ren_coeff	/* out: renumbering coefficient */
		    )
{
  *Ren_coeff = 1.0;
  return (1);
}


/*------------------------------------------------------------
  pdr_get_ent_pdeg - to return the degree of approximation index
                      associated with a given mesh entity
------------------------------------------------------------*/
int pdr_get_ent_pdeg(  /* returns: >0 - approximation index,
				   <0 - error code */
  int Problem_id,	/* in: approximation field ID  */
  int Ent_type,	/* in: type of mesh entity */
  int Ent_id	/* in: mesh entity ID */
    )
{
  int i, field_id;
  i=3; field_id=pdr_ctrl_i_params(Problem_id,i);
  return (apr_get_ent_pdeg(field_id, Ent_type, Ent_id));

}


/*---------------------------------------------------------
  pdr_dof_ent_sons - to return a list of dof entity sons
---------------------------------------------------------*/
int pdr_dof_ent_sons(		/* returns: success >=0 or <0 - error code */
  int Problem_id,	/* in: problem ID  */
  int Ent_type,	/* in: type of mesh entity */
  int Ent_id,	/* in: mesh entity ID */
  int *Ent_sons	/* out: list of dof entity sons */
                /*         Ent_sons[0] - number of sons */
    )
{

  int field_id, mesh_id;

  field_id = pdr_ctrl_i_params(Problem_id,3);
  mesh_id = apr_get_mesh_id(field_id);
  if (Ent_type == PDC_ELEMENT) {
/*kbw
    printf("pdr_dof_ent_sons: element %d, mesh_id %d\n", Ent_id, mesh_id);
/*kew*/

    mmr_el_fam(mesh_id, Ent_id, Ent_sons, NULL);
/*kbw
      printf("sons:");
      for(i=0;i<Ent_sons[0];i++){
        printf("%d  ", Ent_sons[i+1]);
      }
      printf("\n");
/*kew*/
  } else if (Ent_type == PDC_FACE) {
    mmr_fa_fam(mesh_id, Ent_id, Ent_sons, NULL);
  } else if (Ent_type == PDC_EDGE) {
    mmr_edge_sons(mesh_id, Ent_id, Ent_sons);
  } else {
    Ent_sons[0] = 0;
  }

  return (0);
}


/*---------------------------------------------------------
  pdr_proj_sol_lev - to L2 project solution dofs between mesh levels
---------------------------------------------------------*/
int pdr_proj_sol_lev(		/* returns: >=0 - success; <0 - error code */
  int Problem_id,	/* in: problem ID */
  int Solver_id,	/* in: solver data structure to be used */
  int Ilev_from,	/* in: level number to project from */
  double *Vec_from,	/* in: vector of values to project */
  int Ilev_to,	/* in: level number to project to */
  double *Vec_to	/* out: vector of projected values */
  ) 
{

  printf("pdr_proj_sol_lev NOT IMPLEMENTED!");
  exit (-1);
}

/*---------------------------------------------------------
  pdr_vec_norm - to compute a norm of global vector (in parallel)
---------------------------------------------------------*/
double pdr_vec_norm(		/* returns: L2 norm of global Vector */
  int Problem_id,	/* in: problem ID */
  int Solver_id,	/* in: solver data structure to be used */
  int Level_id,	/* in: level number */
  int Nrdof,	/* in: number of vector components */
  double *Vector	/* in: local part of global Vector */
  )
{

  double vec_norm = 0.0;
  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

#ifdef PARALLEL
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);

/*kbw
 printf("Problem_id %d, Solver_id %d, Field_id %d, Level_id %d, Nrdof %d\n", 
	Problem_id, Solver_id, field_id, Level_id, Nrdof);
/*kew*/


  vec_norm = appr_sol_vec_norm(field_id, Level_id, Nrdof, Vector);
#endif

  return (vec_norm);
}


/*---------------------------------------------------------
  pdr_sc_prod - to compute a scalar product of two global vectors 
---------------------------------------------------------*/
double pdr_sc_prod(	/* retruns: scalar product of Vector1 and Vector2 */
  int Problem_id,	/* in: problem ID */
  int Solver_id,	/* in: solver data structure to be used */
  int Level_id,	/* in: level number */
  int Nrdof,	/* in: number of vector components */
  double *Vector1,	/* in: local part of global Vector */
  double *Vector2	/* in: local part of global Vector */
  )
{

  double sc_prod = 0.0;
  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

#ifdef PARALLEL
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);
  sc_prod = appr_sol_sc_prod(field_id, Level_id, Nrdof, Vector1, Vector2);
#endif

  return (sc_prod);
}

/*---------------------------------------------------------
  pdr_create_exchange_tables - to create tables to exchange dofs 
---------------------------------------------------------*/
int pdr_create_exchange_tables(
				/* returns: >=0 -success code, <0 -error code */
  int Problem_id,	/* in: problem ID */
  int Solver_id,	/* in: solver data structure to be used */
  int Level_id,	/* in: level ID */
  int Nr_dof_ent,	/* in: number of DOF entities in the level */
  /* all four subsequent arrays are indexed by block IDs with 1(!!!) offset */
  int *L_dof_ent_type,	/* in: list of DOF entities associated with DOF blocks */
  int *L_dof_ent_id,	/* in: list of DOF entities associated with DOF blocks */
  int *L_bl_nrdof,	/* in: list of nrdofs for each dof block */
  int *L_bl_posg,	/* in: list of positions within the global */
  /*     vector of dofs for each dof block */
  int *L_elem_to_bl,	/* in: list of DOF blocks associated with DOF entities */
  int *L_face_to_bl,	/* in: list of DOF blocks associated with DOF entities */
  int *L_edge_to_bl,	/* in: list of DOF blocks associated with DOF entities */
  int *L_vert_to_bl	/* in: list of DOF blocks associated with DOF entities */
  )
{

  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

#ifdef PARALLEL
  // simplified setting, only one problem and one field
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);

  appr_create_exchange_tables(field_id, Level_id, Nr_dof_ent, 
		    L_dof_ent_type, L_dof_ent_id, L_bl_nrdof, L_bl_posg, 
		    L_elem_to_bl, L_face_to_bl, L_edge_to_bl, L_vert_to_bl);
#endif

  return (0);

}

/*---------------------------------------------------------
  pdr_exchange_dofs - to exchange dofs between processors
---------------------------------------------------------*/
int pdr_exchange_dofs(
  int Problem_id,	/* in: problem ID */
  int Solver_id,	/* in: solver data structure to be used */
  int Level_id,	/* in: level number */
  double *Vec_dofs	/* in: vector of dofs to be exchanged */
  )
{

  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

#ifdef PARALLEL
  // simplified setting, only one problem and one field
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);
  appr_exchange_dofs(field_id, Level_id, Vec_dofs);
#endif

  return (1);
}


/*------------------------------------------------------------
  pdr_select_el_coeff_vect - to select coefficients returned to approximation
                        routines for element integrals in weak formulation
           (the procedure indicates which terms are non-zero in weak form)
------------------------------------------------------------*/
int pdr_select_el_coeff_vect( // returns success indicator
  int Problem_id,
  int *Coeff_vect_ind	/* out: coefficient indicator */
			      )
{

  pdr_heat_select_el_coeff_vect(Problem_id, Coeff_vect_ind);

  return(1);

}

/*!!!!!! OLD OBSOLETE VERSION !!!!!!*/
/*------------------------------------------------------------
  pdr_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals in weak formulation
           (the procedure indicates which terms are non-zero in weak form)
------------------------------------------------------------*/
double *pdr_select_el_coeff( /* returns: pointer !=NULL to indicate selection */
  int Problem_id,
  double **Mval,	/* out: mass matrix coefficient */
  double **Axx,double **Axy,double **Axz, /* out:diffusion coefficients, e.g.*/
  double **Ayx,double **Ayy,double **Ayz, /* Axy denotes scalar or matrix */
  double **Azx,double **Azy,double **Azz, /* related to terms with dv/dx*du/dy */
  /* second order derivatives in weak formulation (scalar for scalar problems */
  /* matrix for vector problems) */
  double **Bx,double **By,double **Bz,	/* out: convection coefficients */
  /* Bx denotes scalar or matrix related to terms with du/dx*v in weak form */
  double **Tx,double **Ty,double **Tz,	/* out: convection coefficients */
  /* Tx denotes scalar or matrix related to terms with u*dv/dx in weak form */
  double **Cval,/* out: reaction coefficients - for terms without derivatives */
  /*  in weak form (as usual: scalar for scalar problems, matrix for vectors) */
  double **Lval,/* out: rhs coefficient for time term, Lval denotes scalar */
  /* or matrix corresponding to time derivative - similar as mass matrix but  */
  /* with known solution at the previous time step (usually denoted by u_n) */
  double **Qx,/* out: rhs coefficients for terms with derivatives */
  double **Qy,/* Qy denotes scalar or matrix corresponding to terms with dv/dy */
  double **Qz,/* derivatives in weak formulation */
  double **Sval	/* out: rhs coefficients without derivatives (source terms) */
  )
{


  double *select_coeff=NULL;

  select_coeff=pdr_heat_select_el_coeff(Problem_id, Mval,
					Axx,Axy,Axz,Ayx,Ayy,Ayz,Azx,Azy,Azz,
					Bx,By,Bz,Tx,Ty,Tz,Cval,Lval,Qx,Qy,Qz,Sval);
  
  return(select_coeff);

}


/*------------------------------------------------------------
pdr_el_coeff - to return coefficients for internal integrals
------------------------------------------------------------*/
int pdr_el_coeff(
  int Problem_id,
  int Elem,	/* in: element number */
  int Mat_num,	/* in: material number */
  double Hsize,	/* in: size of an element */
  int Pdeg,	/* in: local degree of polynomial */
  double *X_loc,      /* in: local coordinates of point within element */
  double *Base_phi,   /* in: basis functions */
  double *Base_dphix, /* in: x-derivatives of basis functions */
  double *Base_dphiy, /* in: y-derivatives of basis functions */
  double *Base_dphiz, /* in: z-derivatives of basis functions */
  double *Xcoor,	/* in: global coordinates of a point */
  double* Uk_val, 	/* in: computed solution from previous iteration */
  double* Uk_x, 	/* in: gradient of computed solution Uk_val */
  double* Uk_y,   	/* in: gradient of computed solution Uk_val */
  double* Uk_z,   	/* in: gradient of computed solution Uk_val */
  double* Un_val, 	/* in: computed solution from previous time step */
  double* Un_x, 	/* in: gradient of computed solution Un_val */
  double* Un_y,   	/* in: gradient of computed solution Un_val */
  double* Un_z,   	/* in: gradient of computed solution Un_val */
  double* Mval,	/* out: mass matrix coefficient */
  double *Axx, double *Axy, double *Axz,  /* out:diffusion coefficients */
  double *Ayx, double *Ayy, double *Ayz,  /* e.g. Axy denotes scalar or matrix */
  double *Azx, double *Azy, double *Azz,  /* related to terms with dv/dx*du/dy */
  /* second order derivatives in weak formulation (scalar for scalar problems */
  /* matrix for vector problems) */
  double *Bx, double *By, double *Bz,	/* out: convection coefficients */
  /* Bx denotes scalar or matrix related to terms with du/dx*v in weak form */
  double *Tx, double *Ty, double *Tz,	/* out: convection coefficients */
  /* Tx denotes scalar or matrix related to terms with u*dv/dx in weak form */
  double *Cval,	/* out: reaction coefficients - for terms without derivatives */
  /*  in weak form (as usual: scalar for scalar problems, matrix for vectors) */
  double *Lval,	/* out: rhs coefficient for time term, Lval denotes scalar */
  /* or matrix corresponding to time derivative - similar as mass matrix but  */
  /* with known solution at the previous time step (usually denoted by u_n) */
  double *Qx, /* out: rhs coefficients for terms with derivatives */
  double *Qy, /* Qy denotes scalar or matrix corresponding to terms with dv/dy */
  double *Qz, /* derivatives in weak formulation */
  double *Sval	/* out: rhs coefficients without derivatives (source terms) */
  )
{

    /* get velocity at gauss point */
    double vel[3];
    pdr_heat_get_velocity_at_point(Problem_id, Elem, X_loc, Base_phi,
				      NULL, NULL, NULL,
				      //Base_dphix, Base_dphiy, Base_dphiz,
				      vel, NULL, NULL, NULL);
/*kbw
    printf("velocity at point %lf, %lf, %lf : %lf, %lf, %lf \n",
	   Xcoor[0], Xcoor[1], Xcoor[2], vel[0], vel[1], vel[2]); 
/*kew*/

// There are two possible ways to navigate through problem parameters
// 1. get problem structure and then get directly parameters
// (the method is FASTER but requires the knowledge of problem structure type)
    
    pdt_heat_problem *problem = 
      (pdt_heat_problem *)pdr_get_problem_structure(Problem_id);
    int field_id = problem->ctrl.field_id;
    int mesh_id = problem->ctrl.mesh_id;
    // heating-cooling problem
    //int field_dtdt_id = pdr_ctrl_i_params(PDC_HEAT_DTDT_ID, 3);
    //double sol_dofs_dtdt[APC_MAXELSD];	/* solution dofs */
    int num_eq;
    
    double delta_t = problem->time.cur_dtime;
    double implicit = problem->time.alpha;
    
/*kbw
  printf("delta_t %lf, implicitness alpha = %lf\n",delta_t, implicit);
/*kew*/
  //int nreq = problem->ctrl.nreq;

// 2. get particular parameters using interface functions
// (the method is slower but can be used in modules that do not know the type
//  of problem structure for heat problem)

  //int nreq = pdr_ctrl_i_params(Problem_id, 5);

  /* select the proper field */
  //field_id = pdr_ctrl_i_params(Problem_id, 3);
  //mesh_id = apr_get_mesh_id(field_id);
  //nreq =apr_get_nreq(field_id);

  // nreq substituted as constant to allow compilers for constants propagation
    int nreq = PDC_HEAT_NREQ;
#ifdef DEBUG
    if(nreq != apr_get_nreq(field_id)){
      printf("wrong parameter HEAT_NREQ in pdr_el_coeff 1\n");
      printf("%d != %d. Exiting !!!",nreq, apr_get_nreq(field_id));
      exit(-1);
    }
    if(nreq != pdr_ctrl_i_params(Problem_id,5)){
      printf("wrong parameter HEAT_NREQ in pdr_el_coeff 2\n");
      printf("%d != %d. Exiting !!!",nreq, pdr_ctrl_i_params(Problem_id,5));
      exit(-1);
    }
    if(nreq != problem->ctrl.nreq){
      printf("wrong parameter HEAT_NREQ in pdr_el_coeff 3\n");
      printf("%d != %d. Exiting !!!",nreq, problem->ctrl.nreq);
      exit(-1);
    }
#endif

    /*! ----------------------------------------------------------------------! */
    /*! -------------------- MATERIAL DATA AT GAUSS POINT ----------------- --! */
    /*! ----------------------------------------------------------------------! */
    double ref_temperature = problem->ctrl.ref_temperature;
    double thermal_conductivity;
    double specific_heat;
    double density;

    // for heat problems with constant material parameters (ref_temperature <= 0)
    if(ref_temperature<=0){
      thermal_conductivity = problem->ctrl.thermal_conductivity;
      density = problem->ctrl.density;
      specific_heat = problem->ctrl.specific_heat;
    }
    // for heat problems with material parameters temperature dependent (ref_temperature > 0)
    else{
      utt_material_query_params qparams;
      utt_material_query_result qresult;
      int i;
      
      //1.set query parameters (which material and temperature)
      qparams.group_idx = Mat_num;	//query by material index ...
      //qparams.material_idx = 1;	//query by material index ...
      
      /* printf("\nAS: (pdr_el_coeff) mat = %d", Mat_num); */
      qparams.name = "";	//... not by material name 
      double tk = Uk_val[0]; // temperature is the only unknown
      qparams.temperature = tk;	//temperature from last iteration
      qparams.cell_id = Elem;
      for( i=0; i<3; i++ ){
	qparams.xg[i] = Xcoor[i];
      }
      //2.get query results
      pdr_heat_material_query( &qparams, &qresult);
      //3.set values to those obtained with query
      thermal_conductivity = qresult.thermal_conductivity;
      specific_heat = qresult.specific_heat;
      density = qresult.density;
      //double thermal_diffusivity = tconductivity / ( density * specific_heat );
      //double texpansion = qresult.thermal_expansion_coefficient;
      
    }
      /*kbw
  printf("in supg_heat: conductivity %lf, specific_heat %lf, density %lf\n",
	 thermal_conductivity,specific_heat,density );
/*kew*/

    /* get coefficients for heat weak formulation */
    /* pdr_heat_el_coeff(Problem_id, Elem, Mat_num, Hsize, Pdeg, X_loc, */
    /* 		      Base_phi, Base_dphix, Base_dphiy, Base_dphiz, */
    /* 		      Xcoor, Uk_val, Uk_x, Uk_y, Uk_z, Un_val, Un_x, Un_y, Un_z, */
    /* 		      Mval, Axx, Axy, Axz, Ayx, Ayy, Ayz, Azx, Azy, Azz, */
    /* 		      Bx, By, Bz, Tx, Ty, Tz, Cval, Lval, Qx, Qy, Qz, Sval,  */
    /* 		      vel, thermal_diffusivity, delta_t, implicit); */
    double daux = density*specific_heat;
    pdr_heat_el_coeff(Problem_id, Elem, Mat_num, Hsize, Pdeg, X_loc,
		      Base_phi, Base_dphix, Base_dphiy, Base_dphiz,
		      Xcoor, Uk_val, Uk_x, Uk_y, Uk_z, Un_val, Un_x, Un_y, Un_z,
		      Mval, Axx, Axy, Axz, Ayx, Ayy, Ayz, Azx, Azy, Azz,
		      Bx, By, Bz, Tx, Ty, Tz, Cval, Lval, Qx, Qy, Qz, Sval, 
		      vel, thermal_conductivity, daux, delta_t, implicit);

  return (0);
}


/*------------------------------------------------------------
  pdr_comp_stiff_mat - to provide a solver with a stiffness matrix
                      and a load vector corresponding to the specified
                      mesh entity, together with information on how to
                      assemble entries into the global stiffness matrix
                      and the global load vector
------------------------------------------------------------*/
int pdr_comp_stiff_mat(	 /* returns: >=0 - success code, <0 - error code */
  int Problem_id,	/* in: approximation field ID  */
  int Int_ent_type,	/* in: unique identifier of the integration entity */
  int Int_ent_id,	/* in: unique identifier of the integration entity */
  int Comp_sm,	/* in: indicator for the scope of computations: */
  /*   PDC_NO_COMP  - do not compute anything */
  /*   PDC_COMP_SM - compute entries to stiff matrix only */
  /*   PDC_COMP_RHS - compute entries to rhs vector only */
  /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int *Pdeg_vec,	/* in: enforced degree of polynomial (if > 0 ) */
  int *Nr_dof_ent,	/* in: size of arrays, */
			/* out: number of mesh entities with which dofs and */
			/*      stiffness matrix blocks are associated */
  int *List_dof_ent_type,	/* out: list of types for 'dof' entities */
  int *List_dof_ent_id,	/* out: list of ids for 'dof' entities */
  int *List_dof_ent_nrdofs,	/* out: list of no of dofs for 'dof' entity */
  int *Nrdofs_loc,	/* in(optional): size of Stiff_mat and Rhs_vect */
	       /* out(optional): actual number of dofs per integration entity */
  double *Stiff_mat,	/* out(optional): stiffness matrix stored columnwise */
  double *Rhs_vect,	/* outpds_elast_ls_std_intf.c(optional): rhs vector */
  char *Rewr_dofs	/* out(optional): flag to rewrite or sum up entries */
			/*   'T' - true, rewrite entries when assembling */
			/*   'F' - false, sum up entries when assembling */
 )
{
  int field_id, mesh_id, face_neig[2];
  int elem;
  
  int pdeg;
  /* element degree of approximation for linear prisms is a single number */
  if (Pdeg_vec == NULL)
    pdeg = 0;
  else
    pdeg = Pdeg_vec[0];

/*kbw
     printf("in pdr_comp_stiff_mat: problem_id %d, int_ent: type %d, id %d, enforced pdeg %d\n",
     Problem_id, Int_ent_type, Int_ent_id, pdeg);
/*kew */

  field_id = pdr_ctrl_i_params(Problem_id, 3);
  /* definitions of both functions  in pds_heat_weakform */
  if (Int_ent_type == PDC_ELEMENT) {


    double timer = time_clock();

    pdr_heat_comp_el_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg, 
			  Nr_dof_ent, List_dof_ent_type, 
			  List_dof_ent_id, List_dof_ent_nrdofs, 
			  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs);

    elem = Int_ent_id;

    pdv_heat_timer_pdr_comp_el_stiff_mat += time_clock() - timer;

  } else if (Int_ent_type == PDC_FACE) {


    double timer = time_clock();
  
    pdr_heat_comp_fa_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg, 
			       Nr_dof_ent, List_dof_ent_type, 
			       List_dof_ent_id, List_dof_ent_nrdofs, 
			       Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs);

    mesh_id = apr_get_mesh_id(field_id);
    mmr_fa_neig(mesh_id, Int_ent_id, face_neig, NULL, NULL, NULL, NULL, NULL); 
    elem = abs(face_neig[0]);
    
    pdv_heat_timer_pdr_comp_fa_stiff_mat += time_clock() - timer;
    
    
  } else {
    printf("ERROR: Wrong integration entity type in pdr_comp_stiff_mat!\n");
    exit(-1);
  }

  /* change the option compute SM and RHSV to rewrite SM and RHSV */
  if (Comp_sm != PDC_NO_COMP)  Comp_sm += 3;
  
  /* obligatory procedure to fill Lists of dof_ents and rewite SM and RHSV */
  /* the reason is to take into account POSSIBLE CONSTRAINTS (HANGING NODES) */
  apr_get_stiff_mat_data(field_id, elem, Comp_sm, 'N',  
			 pdeg, 0, Nr_dof_ent, List_dof_ent_type, 
			 List_dof_ent_id, List_dof_ent_nrdofs, 
			 Nrdofs_loc, Stiff_mat, Rhs_vect);
  
/*kbw
  if(Comp_sm!=PDC_NO_COMP)
    {
      int i;
      printf("In pdr_com_el_stiff_mat: field_id %d, El_id %d, Comp_sm %d, Nr_dof_ent %d\n",
	     field_id, elem, Comp_sm, *Nr_dof_ent);
      printf("For each block: \ttype, \tid, \tnrdof\n");
      for(i=0;i<*Nr_dof_ent;i++){
	printf("\t\t\t%d\t%d\t%d\n",
	       List_dof_ent_type[i],List_dof_ent_id[i],List_dof_ent_nrdofs[i]);
      }
      printf("\n\n");
    }
  //getchar();getchar();
/*kew */

  /* matrix displayed by rows, altghough stored by columns !!!!!!!!! */
/*kbw
  if(Comp_sm!=PDC_NO_COMP)
    {
      int idofs, jdofs;
      printf("\nElement %d: Modified stiffness matrix:\n",elem);
      for (idofs=0;idofs<*Nrdofs_loc;idofs++) { //for each row!!!!
	for (jdofs=0;jdofs<*Nrdofs_loc;jdofs++) { // for each element in row !!!
	  printf("%7.3lf",Stiff_mat[idofs+jdofs*(*Nrdofs_loc)]);
	}
	printf("\n");
      }
      printf("Element %d: Rhs_vect:\n",elem);
      for (idofs=0;idofs<*Nrdofs_loc;idofs++) {
	printf("%7.3lf",Rhs_vect[idofs]);
      }
      printf("\n\n");
      //getchar();
    }
  /* */
  
  return (1);
}



/*------------------------------------------------------------
  pdr_heat_give_me_velocity_at_point - to provide the velocity and its
    gradient at a particular point given its local coordinates within an element
HEAT MODULE ASKS FOR IMPLEMENTATION - it has to be provided by procedures
defined in ls_intf directory of the problem module that uses heat as submodule
------------------------------------------------------------*/
int pdr_heat_give_me_velocity_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Velocity, // temperature
  double *DVel_dx, // x-derivative of temperature
  double *DVel_dy, // y-derivative of temperature
  double *DVel_dz // z-derivative of temperature
  )
{

  // we call procedure to specify velocity
  pdr_heat_get_velocity_at_point(Problem_id, El_id, X_loc, Base_phi,  
				    Base_dphix, Base_dphiy, Base_dphiz,
				    Velocity, DVel_dx, DVel_dy, DVel_dz);
  
  return(0);
}
