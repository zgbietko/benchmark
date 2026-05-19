/************************************************************************
File pds_ns_supg_ls_intf.c - interface between the problem dependent
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
  pdr_ns_supg_give_me_temperature_at_point - to provide the temperature and its
    gradient at a particular point given its local coordinates within an element
NS_SUPG MODULE ASKS FOR IMPLEMENTATION - it has to be provided by procedures
defined in ls_intf directory of the problem module that uses ns_supg as submodule

Two local procedures :

  pdr_ns_supg_comp_el_stiff_mat - to construct a stiffness matrix and a load
                            vector for an element (used by pdr_comp_stiff_mat) 

  pdr_ns_supg_comp_fa_stiff_mat - to construct a stiffness matrix and a load
                           vector for a face (used by pdr_comp_stiff_mat)

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
// for sir_assemble_local_stiff_mat
#include "sih_intf.h"

#ifdef PARALLEL
/* interface of parallel mesh manipulation modules */
#include "mmph_intf.h"		/* USES */
/* interface for all parallel approximation modules */
#include "apph_intf.h"		/* USES */
/* interface for parallel communication modules */
#include "pch_intf.h"		/* USES */
#endif

/* problem module's types and functions */
#include "../include/pdh_ns_supg.h"	/* USES */
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
// bc and material header files are included in problem header files
/* weakform functions */
#include "../../pdd_ns_supg/include/pdh_ns_supg_weakform.h"	/* USES */


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

  return utr_create_assemble_stiff_mat(Problem_id, Level_id, Comp_type,
				Nr_int_ent, L_int_ent_type,
				L_int_ent_id, Max_dofs_int_ent);


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
  int Dof_ent_type, 
  int Dof_ent_id, 
  int Nrdof, 
  double *Vect_dofs	
                        /* in: dofs to be written */
    )
{

  int i, field_id;
  int vect_id = 0; // problem module decides from where to read data for solver

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
  int Dof_ent_type, 
  int Dof_ent_id, 
  int Nrdof, 
  double *Vect_dofs	/* in: dofs to be written */
			)
{
  int vect_id = 0; // problem module decides where to write data from solver
  
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
  pdr_ns_supg_select_el_coeff_vect(Problem_id, Coeff_vect_ind);
  
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

  /* allocate storage for coefficients and select the needed ones */
  select_coeff=pdr_ns_supg_select_el_coeff(Problem_id, Mval,
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

// There are two possible ways to navigate through problem parameters
// 1. get problem structure and then get directly parameters
// (the method is FASTER but requires the knowledge of problem structure type)

  pdt_ns_supg_problem *problem = 
    (pdt_ns_supg_problem *)pdr_get_problem_structure(Problem_id);
  int field_id = problem->ctrl.field_id;
  int mesh_id = problem->ctrl.mesh_id;
  //int nreq = problem->ctrl.nreq;

// 2. get particular parameters using interface functions
// (the method is slower but can be used in modules that do not know the type
//  of problem structure for ns_supg problem)

  //int nreq = pdr_ctrl_i_params(Problem_id, 5);

  /* select the proper field */
  //field_id = pdr_ctrl_i_params(Problem_id, 3);
  //mesh_id = apr_get_mesh_id(field_id);
  //nreq =apr_get_nreq(field_id);

  /* get temperature at gauss point */
  double tk=0.0;
  pdr_ns_supg_give_me_temperature_at_point(Problem_id, Elem, X_loc, Base_phi,
  				             Base_dphix, Base_dphiy, Base_dphiz,
  				             &tk, NULL, NULL, NULL);

/*kbw
    printf("temperature at point %lf, %lf, %lf : %lf\n",
	   Xcoor[0], Xcoor[1], Xcoor[2], tk); 
/*kew*/

  /*! ----------------------------------------------------------------------! */
  /*! -------------------- MATERIAL DATA AT GAUSS POINT ----------------- --! */
  /*! ----------------------------------------------------------------------! */
  double ref_temperature = problem->ctrl.ref_temperature;
  double dyn_visc, density, ref_density;

  // for Navier-Stokes with constant density and viscosity (ref_temperature <= 0)
  if(ref_temperature<=0){
    dyn_visc = problem->ctrl.dynamic_viscosity;
    density = problem->ctrl.density;
    ref_density = problem->ctrl.density;
  }
  else{

    // for Navier-Stokes with density or viscosity temperature dependent 
    utt_material_query_params qparams;
    utt_material_query_result qresult;
    int i;

    //1.set query parameters (which material and temperature)
    qparams.group_idx = Mat_num;	// query by material index ...
    /* printf("\nAS: (pdr_ns_supg_el_coeff) mat = %d", Mat_num); */
    qparams.name = "";	// ... not by material name 
    qparams.temperature = tk;	// current temperature
    qparams.cell_id = Elem;
    for( i=0; i<3; i++ ){
      qparams.xg[i] = Xcoor[i];
    }
    qparams.query_type = QUERY_POINT;
    //2.get query results
    pdr_ns_supg_material_query(&qparams, &qresult);
    //3.set values to those obtained with query
    dyn_visc = qresult.dynamic_viscosity;
    //double tconductivity = qresult.thermal_conductivity;
    //double specific_heat = qresult.specific_heat;
    density = qresult.density;
    //double thermal_diffusivity = tconductivity / ( density * specific_heat );
    //double texpansion = qresult.thermal_expansion_coefficient;
    //4.get reference density (using reference temperature)
    // method 1:
    // double ref_temperature = pdr_ctrl_d_params(Problem_id, 20);
    // method 2:
    qparams.temperature = ref_temperature;
    //ref_density = density at reference temperature
    //AS: the same as above query_params.xg's and query_params.query_type's
    pdr_ns_supg_material_query(&qparams, &qresult);
    ref_density = qresult.density;

  }
#ifdef TURBULENTFLOW
  double tv;
  TurbulentViscosity(El_id,X_loc,&tv);
  dyn_visc+=tv;
#endif //TURBULENTFLOW

  double delta_t = problem->time.cur_dtime;

/*kbw
  if(El_id==5){
    printf("\ndtime %lf, ref_density %lf, dynamic_viscosity %lf\n",
	   delta_t, ref_density, dyn_visc);
    printf("Uk: %lf, %lf, %lf\n", Uk_val[0], Uk_val[1], Uk_val[2]);
    printf("Un: %lf, %lf, %lf\n", Un_val[0], Un_val[1], Un_val[2]);	   
  }
/*kew */

  if(delta_t <= 0.0 || dyn_visc <= 0.0 || ref_density <= 0.0 || density <= 0.0){
    printf("Error in setting parameters for ns_supg_el_coeff !!!\n");
    printf("\ndtime %lf, density %lf, ref_density %lf, dynamic_viscosity %lf\n",
	   delta_t, density, ref_density, dyn_visc);
    printf("Exiting!!!\n");
  }

  /*! ----------------------------------------------------------------------! */
  /*! ------------ CALCULATE BOUSINESSQUE FORCE AT GAUSS POINT -------------! */
  /*! ----------------------------------------------------------------------! */
  /*
  /* !!!!!!! body force should be in [kg/m^2/sec^2] or equivalent */

  //OPTION 1: get density from texpansion, tk and ref_temperature
  /* f_x = Reference_density*(1.0-texp*(tk-ref_temperature))*g_pdv_cfg.gravity_field[0]; */
  /* f_y = Reference_density*(1.0-texp*(tk-ref_temperature))*g_pdv_cfg.gravity_field[1]; */
  /* f_z = Reference_density*(1.0-texp*(tk-ref_temperature))*g_pdv_cfg.gravity_field[2]; */
  
  //OPTION 2: get density from (piecewise linear interpolated) values from file
  // gravity_field components returned as double control parameters
  // gravity_filed[i] = pdr_ctrl_d_params(Problem_id, 10+i);
  //double f_x = density * pdr_ctrl_d_params(Problem_id, 10);
  //double f_y = density * pdr_ctrl_d_params(Problem_id, 11);
  //double f_z = density * pdr_ctrl_d_params(Problem_id, 12);
  double f_x = density * problem->ctrl.gravity_field[0];
  double f_y = density * problem->ctrl.gravity_field[1];
  double f_z = density * problem->ctrl.gravity_field[2];

  //double implicit = pdr_time_d_params(Problem_id, 2);
  double implicit = problem->time.alpha;

  // reference velocity
  double u_ref = problem->ctrl.ref_velocity;

  /* get coefficients for ns_supg weak formulation */
  pdr_ns_supg_el_coeff(Problem_id, Elem, Mat_num, Hsize, Pdeg, X_loc,
		       Base_phi, Base_dphix, Base_dphiy, Base_dphiz,
		       Xcoor, Uk_val, Uk_x, Uk_y, Uk_z, Un_val, Un_x, Un_y, Un_z,
		       Mval, Axx, Axy, Axz, Ayx, Ayy, Ayz, Azx, Azy, Azz,
		       Bx, By, Bz, Tx, Ty, Tz, Cval, Lval, Qx, Qy, Qz, Sval, 
		       tk, dyn_visc, density, ref_density, u_ref, 
		       delta_t, implicit, f_x, f_y, f_z);

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
  double timer = time_clock();
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
  
  /* compute UNCONSTRAINED stiffness matrices (no hanging nodes considered) */
  if (Int_ent_type == PDC_ELEMENT) {
    pdr_ns_supg_comp_el_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg, 
				  Nr_dof_ent, List_dof_ent_type, 
				  List_dof_ent_id, List_dof_ent_nrdofs, 
				  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs);

    elem = Int_ent_id;
    
    
  } else if (Int_ent_type == PDC_FACE) {
    pdr_ns_supg_comp_fa_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg, 
				  Nr_dof_ent, List_dof_ent_type, 
				  List_dof_ent_id, List_dof_ent_nrdofs, 
				  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs);

    mesh_id = apr_get_mesh_id(field_id);
    mmr_fa_neig(mesh_id, Int_ent_id, face_neig, NULL, NULL, NULL, NULL, NULL); 
    elem = abs(face_neig[0]);
    
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
  //if(Comp_sm!=PDC_NO_COMP)
	  {
		int i;
    printf("In pdr_com_el_stiff_mat: field_id %d, El_id %d, Comp_sm %d, Nr_dof_ent %d\n",
     field_id, Int_ent_id, Comp_sm, *Nr_dof_ent);
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
     printf("\nElement %d: Modified stiffness matrix:\n",El_id);
     for (idofs=0;idofs<*Nrdofs_loc;idofs++) { //for each row!!!!
     for (jdofs=0;jdofs<*Nrdofs_loc;jdofs++) { // for each element in row !!!
     printf("%7.3lf",Stiff_mat[idofs+jdofs*(*Nrdofs_loc)]);
     }
     printf("\n");
     }
     printf("Element %d: Rhs_vect:\n",El_id);
     for (idofs=0;idofs<*Nrdofs_loc;idofs++) {
     printf("%7.3lf",Rhs_vect[idofs]);
     }
     printf("\n\n");
     //getchar();
     }
     /* */

  pdv_ns_supg_timer_pdr_comp_el_stiff_mat += time_clock() - timer;

  return (1);
}


/*------------------------------------------------------------
  pdr_ns_supg_give_me_temperature_at_point - to provide the temperature and its
    gradient at a particular point given its local coordinates within an element
NS_SUPG MODULE ASKS FOR IMPLEMENTATION - it has to be provided by procedures
defined in ls_intf directory of the problem module that uses ns_supg as submodule
HERE: ns_supg gives itself trivial solution
------------------------------------------------------------*/
int pdr_ns_supg_give_me_temperature_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Temp, // temperature
  double *DTemp_dx, // x-derivative of temperature
  double *DTemp_dy, // y-derivative of temperature
  double *DTemp_dz // z-derivative of temperature
  )
{

  /* get temperature at gauss point */
  *Temp=pdv_ns_supg_problem.ctrl.ref_temperature; 
  if(DTemp_dx != NULL){
    *DTemp_dx=0.0; *DTemp_dy=0.0; *DTemp_dz=0.0;
  }
  return(0);
}
