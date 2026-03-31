/************************************************************************
File pds_test_ls_std_intf.c - interface between the problem dependent
         module for testing using Laplace's equation, the discontinuous Galerkin
         approximation  and linear solver modules (direct and iterative)

Contains definitions of routines:

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
  pdr_comp_stiff_mat - to provide a solver with a stiffness matrix 
                      and a load vector corresponding to the specified
                      mesh entity, together with information on how to
                      assemble entries into the global stiffness matrix
                      and the global load vector
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


------------------------------
History:
	02.2002 - Krzysztof Banas, initial version
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

/* header file of the problem dependent module for Laplace's equation */
#include "../pdh_laplace.h"

/* interface of the problem dependent module for fe solvers */
#include "pdh_intf.h"

/* interface of the mesh manipulation module */
#include "mmh_intf.h"

/* interface for approximation modules */
#include "aph_intf.h"

#ifdef PARALLEL	
#include "mmph_intf.h"	
#include "apph_intf.h"	
#endif

#include "lin_alg_intf.h"

#define ut_min(x,y) ((x)<(y)?(x):(y))

/*** CONSTANTS ***/

const int PDC_ELEMENT = APC_ELEMENT;
const int PDC_FACE    = APC_FACE   ;
const int PDC_EDGE    = APC_EDGE   ;
const int PDC_VERTEX    = APC_VERTEX   ;

const int PDC_NO_COMP  = APC_NO_COMP  ;/* do not compute stiff mat and rhs vect*/
const int PDC_COMP_SM  = APC_COMP_SM  ;/* compute entries to stiff matrix only */
const int PDC_COMP_RHS = APC_COMP_RHS ;/* compute entries to rhs vector only */
const int PDC_COMP_BOTH = APC_COMP_BOTH; /* compute entries for sm and rhsv */

/*** functions definitions ***/

/*------------------------------------------------------------
  pdr_get_list_ent - to return the list of integration entities - entities
                         for which stiffness matrices and load vectors are
                         provided by the FEM code to the solver module,
                         and DOF entities - entities with which there are dofs
                         associated by the given approximation
------------------------------------------------------------*/
int pdr_get_list_ent( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,     /* in:  problem (and solver) identification */
  int* Nr_int_ent,    /* out: number of integration entitites */
  int** List_int_ent_type,/* out: list of types of integration entitites */
  int** List_int_ent_id,  /* out: list of IDs of integration entitites */
  int* Nr_dof_ent,    /* out: number of dof entities (entities with which there
		              are dofs associated by the given approximation) */
  int** List_dof_ent_type,/* out: list of types of integration entitites */
  int** List_dof_ent_id,  /* out: list of IDs of integration entitites */
  int** List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_glob,    /* out: global number of degrees of freedom (unknowns) */
  int* Max_dofs_per_dof_ent/* out: maximal number of dofs per dof entity */
  )
{

/*++++++++++++++++ executable statements ++++++++++++++++*/

// default procedure for returning lists of integration and dof entities
// for standard and discontinuous Galerkin approximations
  utr_get_list_ent(Problem_id, Nr_int_ent, 
		       List_int_ent_type, List_int_ent_id, 
                       Nr_dof_ent, List_dof_ent_type, 
		       List_dof_ent_id, List_dof_ent_nrdofs, 
		       Nrdofs_glob, Max_dofs_per_dof_ent);

  return(1);
}

/*------------------------------------------------------------
  pdr_get_list_ent_coarse - to return the list of integration entities - entities
                         for which stiffness matrices and load vectors are
                         provided by the FEM code to the solver module,
                         and DOF entities - entities with which there are dofs
                         associated by the given approximation for COARSE level
                         given the corresponding lists from the fine level
------------------------------------------------------------*/
int pdr_get_list_ent_coarse( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,       /* in:  problem (and solver) identification */
  int Nr_int_ent_fine, /* in: number of integration entitites */
  int* List_int_ent_type_fine,/* in: list of types of integration entitites */
  int* List_int_ent_id_fine,  /* in: list of IDs of integration entitites */
  int Nr_dof_ent_fine, /* in: number of dof entities (entities with which there
		             are dofs associated by the given approximation) */
  int* List_dof_ent_type_fine,/* in: list of types of integration entitites */
  int* List_dof_ent_id_fine,  /* in: list of IDs of integration entitites */
  int* List_dof_ent_nrdofs_fine,/* in: list of no of dofs for 'dof' entity */
  int Nrdofs_glob_fine,  /* in: global number of degrees of freedom (unknowns) */
  int Max_dof_per_ent_fine, /* in: maximal number of dofs per dof entity */
  int* Pdeg_coarse_p,  /* in: degree of approximation for coarse space */
  int* Nr_int_ent_p,    /* out: number of integration entitites */
  int** List_int_ent_type,/* out: list of types of integration entitites */
  int** List_int_ent_id,  /* out: list of IDs of integration entitites */
  int* Nr_dof_ent_p,    /* out: number of dof entities (entities with which there
		              are dofs associated by the given approximation) */
  int** List_dof_ent_type,/* out: list of types of integration entitites */
  int** List_dof_ent_id,  /* out: list of IDs of integration entitites */
  int** List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_glob,    /* out: global number of degrees of freedom (unknowns) */
  int* Max_dof_per_ent/* out: maximal number of dofs per dof entity */
  )
{


  return(1);
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
			       L_dof_ent_id, L_dof_ent_nrdofs, 
			       Stiff_mat, Rhs_vect, Rewr_dofs);

  return(1);

}


/* two internal routines - one for elements, one for faces */
int pdr_comp_el_stiff_mat(/*returns: >=0 -success code, <0 -error code*/
  int Problem_id,     /* in: approximation field ID  */
  int El_id,        /* in: unique identifier of the element */
  int Comp_sm,       /* in: indicator for the scope of computations: */
                     /*   PDC_NO_COMP  - do not compute anything */
                     /*   PDC_COMP_SM - compute entries to stiff matrix only */
                     /*   PDC_COMP_RHS - compute entries to rhs vector only */
                     /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int Pdeg,        /* in: enforced degree of polynomial (if > 0 ) */
  int* Nr_dof_ent,        /* out: number of mesh entities with which dofs and */
                          /*      stiffness matrix blocks are associated */
  int* List_dof_ent_type, /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_id,   /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofss_loc,        /* in(optional): size of Stiff_mat and Rhs_vect */
                /* out(optional): actual number of dofs per integration entity*/
  double* Stiff_mat,      /* out(optional): stiffness matrix stored columnwise*/
  double* Rhs_vect,       /* out(optional): rhs vector */
  char* Rewr_dofs        /* out(optional): flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
  );

int pdr_comp_fa_stiff_mat(/*returns: >=0 -success code, <0 -error code*/
  int Problem_id,     /* in: approximation field ID  */
  int El_id,        /* in: unique identifier of the element */
  int Comp_sm,       /* in: indicator for the scope of computations: */
                     /*   PDC_NO_COMP  - do not compute anything */
                     /*   PDC_COMP_SM - compute entries to stiff matrix only */
                     /*   PDC_COMP_RHS - compute entries to rhs vector only */
                     /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int Pdeg,        /* in: enforced degree of polynomial (if > 0 ) */
  int* Nr_dof_ent,   /* in: size of arrays, */
                /* out: no of filled entries, i.e. number of mesh entities*/
                /* with which dofs and stiffness matrix blocks are associated */
  int* List_dof_ent_type, /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_id,   /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_loc,        /* in(optional): size of Stiff_mat and Rhs_vect */
                /* out(optional): actual number of dofs per integration entity*/
  double* Stiff_mat,      /* out(optional): stiffness matrix stored columnwise*/
  double* Rhs_vect,       /* out(optional): rhs vector */
  char* Rewr_dofs        /* out(optional): flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
  );

/*------------------------------------------------------------
  pdr_comp_stiff_mat - to construct a stiffness matrix and a load vector for
                      some given mesh entity
------------------------------------------------------------*/
int pdr_comp_stiff_mat( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,      /* in: approximation field ID  */
  int Int_ent_type,    /* in: unique identifier of the integration entity */
  int Int_ent_id,    /* in: unique identifier of the integration entity */
  int Comp_sm,       /* in: indicator for the scope of computations: */
                     /*   PDC_NO_COMP  - do not compute anything */
                     /*   PDC_COMP_SM - compute entries to stiff matrix only */
                     /*   PDC_COMP_RHS - compute entries to rhs vector only */
                     /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int *Pdeg_vec,        /* in: enforced degree of polynomial (if != NULL ) */
  int* Nr_dof_ent,   /* in: size of arrays, */
                          /* out: number of mesh entities with which dofs and */
                          /*      stiffness matrix blocks are associated */
  int* List_dof_ent_type, /* out: list of types for 'dof' entities */
  int* List_dof_ent_id,   /* out: list of ids for 'dof' entities */
  int* List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_loc,        /* in(optional): size of Stiff_mat and Rhs_vect */
                /* out(optional): actual number of dofs per integration entity*/
  double* Stiff_mat,      /* out(optional): stiffness matrix stored columnwise*/
  double* Rhs_vect,       /* out(optional): rhs vector */
  char* Rewr_dofs         /* out(optional): flag to rewrite or sum up entries */
                          /*   'T' - true, rewrite entries when assembling */
                          /*   'F' - false, sum up entries when assembling */
  )
{

  int pdeg;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* for uniform approximation pdeg is a number */
  if(Pdeg_vec==NULL) pdeg = 0;
  else pdeg = Pdeg_vec[0];

/*kbw
    printf("in pdr_comp_stiff_mat: problem_id %d, int_ent: type %d, id %d\n",
	   Problem_id, Int_ent_type, Int_ent_id);
    printf("in pdr_comp_stiff_mat: Comp_sm %d, Pdeg_vec %d, Nr_dof_ent %d\n",
	   Comp_sm, pdeg, *Nr_dof_ent);
/*kew*/

  if( Int_ent_type == PDC_ELEMENT ){

    pdr_comp_el_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg,
			  Nr_dof_ent, List_dof_ent_type,
			  List_dof_ent_id, List_dof_ent_nrdofs,
			  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs);


  }
  else if(Int_ent_type==PDC_FACE){

    pdr_comp_fa_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg,
			  Nr_dof_ent, List_dof_ent_type,
			  List_dof_ent_id, List_dof_ent_nrdofs,
			  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs);

  }
  else{
    printf("Wrong integration entity type in pdr_comp_stiff_mat!\n");
    exit(-1);
  }

  return(1);
}

/*------------------------------------------------------------
  pdr_comp_el_stiff_mat - to construct a stiffness matrix and a load vector
                          for an element
------------------------------------------------------------*/
int pdr_comp_el_stiff_mat(/*returns: >=0 -success code, <0 -error code*/
  int Problem_id,     /* in: approximation field ID  */
  int El_id,        /* in: unique identifier of the element */
  int Comp_sm,       /* in: indicator for the scope of computations: */
                     /*   PDC_NO_COMP  - do not compute anything */
                     /*   PDC_COMP_SM - compute entries to stiff matrix only */
                     /*   PDC_COMP_RHS - compute entries to rhs vector only */
                     /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int Pdeg_in,        /* in: enforced degree of polynomial (if > 0 ) */
  int* Nr_dof_ent,   /* in: size of arrays, */
                /* out: no of filled entries, i.e. number of mesh entities*/
                /* with which dofs and stiffness matrix blocks are associated */
  int* List_dof_ent_type, /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_id,   /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_loc,        /* in(optional): size of Stiff_mat and Rhs_vect */
                /* out(optional): actual number of dofs per integration entity*/
  double* Stiff_mat,      /* out(optional): stiffness matrix stored columnwise*/
  double* Rhs_vect,       /* out(optional): rhs vector */
  char* Rewr_dofs        /* out(optional): flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
  )
{

  int pdeg;		/* degree of polynomial */
  int el_mate;		/* element material */
  int sol_vec_id;       /* indicator for the solution dofs */
  int num_shap;         /* number of element shape functions */
  int num_dofs;         /* number of element degrees of freedom */
  double sol_dofs[APC_MAXELSD]; /* solution dofs */

  int field_id, mesh_id;
  int kk, idofs, jdofs;
  int i,j,k;
  int max_dof_ent, max_nrdofs, glob_nr_dofs, nreq;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper field */
  field_id = Problem_id;

  if(Comp_sm!=PDC_NO_COMP){

    /* select the proper mesh */
    mesh_id = apr_get_mesh_id(field_id);
    nreq = PDC_NREQ;
    
    /* get material number */
    el_mate =  mmr_el_groupID(mesh_id, El_id);
    
    /* find degree of polynomial and number of element scalar dofs */
    if(Pdeg_in>0) pdeg = Pdeg_in;
    else{
      apr_get_el_pdeg(field_id, El_id, &pdeg);
    }
    num_shap = apr_get_el_pdeg_numshap(field_id, El_id, &pdeg);
    num_dofs = num_shap*nreq;
    
    //printf("pdeg %d, num_shap=%d\n",pdeg,num_shap);
    
    max_dof_ent = *Nr_dof_ent;
    
#ifdef DEBUG
    if(max_dof_ent < 1){
      printf("Too small arrays List_dof_... passed to comp_el_stiff_mat\n");
      printf("%d < %d. Exiting !!!",max_dof_ent, *Nr_dof_ent);
      exit(-1);
    }
#endif
    
    
#ifdef DEBUG
    if(Nrdofs_loc == NULL || Stiff_mat == NULL || Rhs_vect == NULL){
printf("NULL arrays Stiff_mat and Rhs_vect in pdr_comp_stiff_el_mat. Exiting!");
      exit(-1);
    }
    if(*Nrdofs_loc<num_shap){
printf("Too small arrays Stiff_mat and Rhs_vect passed to comp_el_stiff_mat\n");
      printf("%d < %d. Exiting !!!", *Nrdofs_loc, num_shap);
      exit(-1);
    }

#endif


    /* get the most recent solution degrees of freedom */
    if(mmr_el_status(mesh_id,El_id)==MMC_ACTIVE){

      sol_vec_id = 1;
      apr_get_el_dofs(field_id, El_id, sol_vec_id, sol_dofs);
      
    }
    else{

     /*!!! coarse element dofs should be supplied by calling routine !!!*/

      for(i=0;i<num_dofs;i++) sol_dofs[i]=0;

    }


    if(Comp_sm==PDC_COMP_SM||Comp_sm==PDC_COMP_BOTH){

      /* initialize the matrices to zero */
      for(i=0;i<num_dofs*num_dofs;i++) Stiff_mat[i]=0.0;

    }

    if(Comp_sm==PDC_COMP_RHS||Comp_sm==PDC_COMP_BOTH){
    
      /* initialize the vector to zero */
      for(i=0;i<num_dofs;i++) Rhs_vect[i]=0.0; 

    }


    /* perform numerical integration of terms from the weak formulation */
    int diagonal[5]={1,1,0,0,0}; // diagonality of: M, A_ij, B_j, T_i and C  
    apr_num_int_el(Problem_id, field_id, El_id, Comp_sm, &pdeg, sol_dofs, NULL,
		   diagonal, Stiff_mat, Rhs_vect);


/* matrix displayed by rows, altghough stored by columns !!!!!!!!!*/
/*kbw
if(El_id>0){
  printf("Element %d: Standard stiffness matrix:\n",El_id);
  for (idofs=0;idofs<num_dofs;idofs++) { //for each row!!!!
    for (jdofs=0;jdofs<num_dofs;jdofs++) { // for each element in row !!!
      printf("%7.3lf",Stiff_mat[idofs+jdofs*num_dofs]);
    }
    printf("\n");
  }
  printf("Element %d: Rhs_vect:\n",El_id);
  for (idofs=0;idofs<num_dofs;idofs++) {
    printf("%7.3lf",Rhs_vect[idofs]);
  }
  printf("\n");
  //getchar();
 }
/*kew*/

    if(Rewr_dofs != NULL) *Rewr_dofs = 'F';
  
  } /* end if computing SM and/or RHSV */

  /* change the option compute SM and RHSV to rewrite SM and RHSV */
  if(Comp_sm!=PDC_NO_COMP) Comp_sm += 3;

  /* obligatory procedure to fill Lists of dof_ents and rewite SM and RHSV */
  /* the reason is to take into account POSSIBLE CONSTRAINTS (HANGING NODES) */
  apr_get_stiff_mat_data(field_id, El_id, Comp_sm, 'N', Pdeg_in, 0, Nr_dof_ent, 
	         List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
			   Nrdofs_loc, Stiff_mat, Rhs_vect);

/* matrix displayed by rows, altghough stored by columns !!!!!!!!!*/
/*kbw
if(Comp_sm!=PDC_NO_COMP && El_id>0){
  printf("Element %d: Modified stiffness matrix:\n",El_id);
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
  printf("\n");
  //getchar();
 }
/*kew*/

/*kbw
    printf("In pdr_com_el_stiff_mat: field_id %d, El_id %d, Comp_sm %d, Nr_dof_ent %d\n",
	   field_id, El_id, Comp_sm, *Nr_dof_ent);
    printf("For each block: \ttype, \tid, \tnrdofs\n");
    for(i=0;i<*Nr_dof_ent;i++){
      printf("\t\t\t%d\t%d\t%d\n",
	     List_dof_ent_type[i],List_dof_ent_id[i],List_dof_ent_nrdofs[i]);
    }
    //getchar();//getchar();
/*kew*/

  return(1);
}


/*------------------------------------------------------------
  pdr_comp_fa_stiff_mat - to construct a stiffness matrix and a load vector
                          for a face - special routine for discontinuous
                          Galerkin approximation fields
------------------------------------------------------------*/
int pdr_comp_fa_stiff_mat(/*returns: >=0 -success code, <0 -error code*/
  int Problem_id,     /* in: approximation field ID  */
  int Fa_id,        /* in: unique identifier of the face */
  int Comp_sm,       /* in: indicator for the scope of computations: */
                     /*   PDC_NO_COMP  - do not compute anything */
                     /*   PDC_COMP_SM - compute entries to stiff matrix only */
                     /*   PDC_COMP_RHS - compute entries to rhs vector only */
                     /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int Pdeg_in,     /* in: enforced degree of polynomial (if > 0 ) */
  int* Nr_dof_ent,   /* in: size of arrays List_dof_ent_... */
                     /* out: number of mesh entities with which dofs and */
                     /*      stiffness matrix blocks are associated */
  int* List_dof_ent_type, /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_id,   /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_loc,        /* in(optional): size of Stiff_mat and Rhs_vect */
                /* out(optional): actual number of dofs per integration entity*/
  double* Stiff_mat,      /* out(optional): stiffness matrix stored columnwise*/
  double* Rhs_vect,       /* out(optional): rhs vector */
  char* Rewr_dofs        /* out(optional): flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
  )
{

/* local variables */
  int name;		/* problem identifier */
  int fa_type;		/* type of face: quad or triangle */
  int fa_bc;	        /* bc flag for a face */
  int bc_type;	        /* type of bc*/
  int base_q;		/* type of basis functions */
  int nreq;		/* number of equations */
  int pdeg=0, num_shap=0;/* local dimension for neigbhor  */
  int el_mate=0;	/* material number for neigbhor  */
  int ngauss;		/* number of gauss points in 1 dimension */
  int face_neig[2];	/* list of neighbors */
  int neig_sides[2]={0,0};/* sides of face wrt neigs */
  int node_shift;
  double acoeff[4], bcoeff[2];/* to transform coordinates between faces */
  double loc_xg[6];	/* local coord of gauss points for neighbors */
  double xg[450];   	/* gauss points in 2D*/
  double wg[150];      	/* gauss weights in 2D*/
  double determ; 	/* determinant of jacobi matrix */
  double hsize=0; /* size of elements */
  double hf_size;        /* size of face */
  double area; 		/* area for integration rule */
  double time=0;          /* current time instant */
  double xcoor[3];/* global coord of gauss point */
  double vec_norm[3];	/* normal vector to the considered face */
  double u_val[PDC_NREQ]; /* computed solution */
  double u_x[PDC_NREQ];   /* gradient of computed solution */
  double u_y[PDC_NREQ];   /* gradient of computed solution */
  double u_z[PDC_NREQ];   /* gradient of computed solution */
  double base_phi[APC_MAXELVD];    /* basis functions */
  double base_dphix[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];  /* y-derivatives of basis function */
  double dofs_loc[APC_MAXELSD]={0}; /* element solution dofs */
  double anx[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double any[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double anz[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double bn[PDC_NREQ*PDC_NREQ];   /* coeff of PDE */
  double fval[PDC_NREQ]; /* coeff of PDE */
  double gval[PDC_NREQ]; /* coeff of PDE */
  double qn[PDC_NREQ];      /* source for the first neigbor */
  double norm_vel[PDC_NREQ]; /* normal velocity for each component of u */

  int el_nodes[MMC_MAXELVNO+1];        // list of nodes of El
  double node_coor[3*MMC_MAXELVNO];  // coord of nodes of El

  int i, ki, kk, ieq1, ieq2, time_dis=0;
  int iaux, jaux;
  int idofs, jdofs, ndofs_fa, ndofs, neig_id;
  double daux, faux, eaux, gaux, haux, penalty;


  int field_id, mesh_id;
  int sol_vec_id;       /* indicator for the solution dofs */


/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper mesh */
  field_id = Problem_id;
  mesh_id = apr_get_mesh_id(field_id);

  /* get type of face and bc flag */
  fa_type=mmr_fa_type(mesh_id,Fa_id);
  fa_bc=mmr_fa_bc(mesh_id,Fa_id);
  mmr_fa_area(mesh_id,Fa_id,&daux,NULL);
  hf_size = sqrt(daux);

  bc_type = pdr_get_bc_type(fa_bc);

  /* get approximation parameters */
  nreq=PDC_NREQ;

/* get neigbors list with corresponding neighbors' sides numbers*/
  mmr_fa_neig(mesh_id,Fa_id,face_neig,neig_sides,&node_shift,NULL,acoeff,bcoeff);

  /* get material number */
  neig_id = abs(face_neig[0]);
  el_mate =  mmr_el_groupID(mesh_id, neig_id);

  if(Pdeg_in>0) pdeg = Pdeg_in;
  else{
    /* find degree of polynomial and number of element scalar dofs */
    /*!!! need some trick for coarse solve with different Pdeg !!!*/
    apr_get_el_pdeg(field_id, neig_id, &pdeg);
  }

  num_shap = apr_get_el_pdeg_numshap(field_id, neig_id, &pdeg);
  ndofs = num_shap*nreq;


  if(Comp_sm!=PDC_NO_COMP){

    /* find vertices */
    mmr_el_node_coor(mesh_id,neig_id,el_nodes,node_coor);

   /* initialize the matrices to zero */
   ndofs_fa = ndofs;


#ifdef DEBUG
   if(*Nrdofs_loc < ndofs_fa ){
    printf("Too small arrays Stiff_mat and Rhs_vect passed to comp_el_stiff_mat\n");
    printf("%d < %d. Exiting !!!", *Nrdofs_loc, ndofs_fa);
    exit(-1);
  }
#endif

   for(i=0;i<ndofs_fa*ndofs_fa;i++) Stiff_mat[i]=0.0;
   for(i=0;i<ndofs_fa;i++) Rhs_vect[i]=0.0;

   base_q=apr_get_base_type(field_id,neig_id);

   /* prepare data for gaussian integration */
   /* !!!!! for triangular faces coordinates are standard [0,1][0,1] */
   /* !!!!! for quadrilateral faces coordinates are [-1,1][-1,1] - */
   /* !!!!! which means that they do not conform to element coordinates */
   /* !!!!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!!!!! */
   //   if(bc_type==PDC_BC_DIRI){
   //apr_set_quadr_2D_penalty(fa_type,base_q,&pdeg,&ngauss,xg,wg);
   //}
   //else{
     apr_set_quadr_2D(fa_type,base_q,&pdeg,&ngauss,xg,wg);
     //}
   
/* loop over integration points */
   for (ki=0;ki<ngauss;ki++) {

/* find coordinates within neighboring elements for a point on face */
   /* !!!!! for triangular faces coordinates are standard [0,1][0,1] */
   /* !!!!! for quadrilateral faces coordinates are [-1,1][-1,1] - */
   /* !!!!! which means that they do not conform to element coordinates */
   /* !!!!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!!!!! */
    mmr_fa_elem_coor(mesh_id,&xg[2*ki],face_neig,neig_sides,node_shift,
			acoeff,bcoeff,loc_xg);

/* at the gauss point for neig , compute basis functions */
    iaux = 3+neig_sides[0]; /* boundary data for face neig_sides[0] */

    determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base_q,
			      loc_xg,node_coor,dofs_loc,
			      base_phi,base_dphix,base_dphiy,base_dphiz,
			      xcoor,u_val,u_x,u_y,u_z,vec_norm);

/* get coefficients of convection-diffusion-reaction equations */
    pdr_fa_coeff(Fa_id,bc_type,neig_id,el_mate,
		   hsize,time,pdeg,
		   xcoor,vec_norm,u_val,u_x,u_y,u_z,
		   anx,any,anz,bn,fval,gval,qn,norm_vel);


/* coefficient for 2D numerical integration */
    area = determ*wg[ki];

    /*kbw
if(Fa_id>0){
printf("at integration point %d, coor: local %lf, %lf, weight %lf\n",
ki,xg[2*ki],xg[2*ki+1],wg[ki]);
printf("coor global %lf, %lf, %lf, determ %lf, area %lf\n",
xcoor[0],xcoor[1],xcoor[2],determ,area);
printf("normal %lf, %lf, %lf\n",
	vec_norm[0], vec_norm[1], vec_norm[2]);
printf("coord for neigh: %lf, %lf, %lf, hsize %lf\n",
loc_xg[0],loc_xg[1],loc_xg[2],hsize);
printf("weight %lf, integration coeff. %lf\n", wg[ki], area);
printf("%d shape functions and derivatives for neigh: \n",num_shap);
for(i=0;i<num_shap;i++){
  printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
  base_phi[i],base_dphix[i],base_dphiy[i],base_dphiz[i]);
}
  printf("solution %lf, der %lf, %lf, %lf\n",
	 u_val[0],u_x[0],u_y[0],u_z[0]);
  printf("diffusion coeff : %lf %lf %lf\n",anx[0],any[0],anz[0]);
  printf("convection coeff: %lf\n",bn[0]);
  printf("rhs coeff       : %lf %lf %lf\n",fval[0],gval[0],qn[0]);
  printf("penalty %lf : fval[0] %lf, hf_size %lf\n",
       penalty,fval[0],hf_size);

//getchar();
}
/*kew*/

    penalty = 1.0e7;
    
    if(bc_type==PDC_BC_DIRI){
      
      kk=0;
      
      for (jdofs=0;jdofs<num_shap;jdofs++) {
	for (idofs=0;idofs<num_shap;idofs++) {
	  
	  /* stiffness matrix */
	  Stiff_mat[kk+idofs] += 
	    /*penalty*/
	    penalty*base_phi[jdofs]*base_phi[idofs]
	    /*penalty*/
	    * area;
	  
	}/* idofs */
	
	kk+=num_shap;

      } /* jdofs */
      
      for (idofs=0;idofs<num_shap;idofs++) {
	
	
	Rhs_vect[idofs] +=
	  /*penalty*/
	  penalty*fval[0]*base_phi[idofs]
	  /*penalty*/
	  * area;
	
      }/* idofs */

    }
    else if(bc_type==PDC_BC_NEUM){
      
      for (idofs=0;idofs<num_shap;idofs++) {
	
	/* right hand side vector */
	Rhs_vect[idofs] += gval[0]*base_phi[idofs]*area;
	
      }/* idofs */
      
    } /* if Neumann BC */
    else if(bc_type==PDC_BC_MIXED){
      
      

    }/* if mixed type BC */

  } /* ki */


/*kbw
   if(Fa_id>0){
     printf("Stiffness matrix face=%d:\n",Fa_id);
     for (idofs=0;idofs<ndofs_fa;idofs++) {
       for (jdofs=0;jdofs<ndofs_fa;jdofs++) {
	 printf("%20.12lf",Stiff_mat[idofs+jdofs*ndofs_fa]);
       }
       printf("\n");
     }
     printf("Rhs_vect:\n");
     for (idofs=0;idofs<ndofs_fa;idofs++) {
       printf("%20.12lf",Rhs_vect[idofs]);
     }
     printf("\n");
     //getchar();
   }
/*kew*/

   if(Rewr_dofs != NULL) *Rewr_dofs = 'F';

/* matrix displayed by rows, altghough stored by columns !!!!!!!!!*/
/*kbw
if(Fa_id>0){
  int num_dofs=ndofs;
  printf("Face %d, Element %d: Standard stiffness matrix:\n",Fa_id,neig_id);
  for (idofs=0;idofs<num_dofs;idofs++) { //for each row!!!!
    for (jdofs=0;jdofs<num_dofs;jdofs++) { // for each element in row !!!
      printf("%11.3le",Stiff_mat[idofs+jdofs*num_dofs]);
    }
    printf("\n");
  }
  printf("Element %d: Rhs_vect:\n",neig_id);
  for (idofs=0;idofs<num_dofs;idofs++) {
    printf("%11.3le",Rhs_vect[idofs]);
  }
  printf("\n");
  //getchar();
 }
/*kew*/

  } /* end if computing entries to the stiffness matrix */

/*kbw
#ifdef DEBUG
    {
      int ibl;
      printf("leaving pdr_fa_stiff_mat:\n");
      printf("face %d, nrdofsbl %d\n", Fa_id, *Nr_dof_ent);
      for(ibl=0;ibl<*Nr_dof_ent; ibl++){
	printf("bl_id %d, bl_nrdofs %d\n",
	  List_dof_ent_id[ibl],List_dof_ent_nrdofs[ibl]);
      }
    }
#endif
/*kew*/

  /* change the option compute SM and RHSV to rewrite SM and RHSV */
  if(Comp_sm!=PDC_NO_COMP) Comp_sm += 3;

  /* obligatory procedure to fill Lists of dof_ents and rewite SM and RHSV */
  /* the reason is to take into account POSSIBLE CONSTRAINTS (HANGING NODES) */
  apr_get_stiff_mat_data(field_id,neig_id,Comp_sm,'N',Pdeg_in,0,Nr_dof_ent, 
	         List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
			   Nrdofs_loc, Stiff_mat, Rhs_vect);

/* matrix displayed by rows, altghough stored by columns !!!!!!!!!*/
/*kbw
if(Comp_sm!=PDC_NO_COMP && Fa_id>0){
  int num_dofs=ndofs;
  printf("Face %d, Element %d: Modified stiffness matrix:\n",Fa_id,neig_id);
  for (idofs=0;idofs<*Nrdofs_loc;idofs++) { //for each row!!!!
    for (jdofs=0;jdofs<*Nrdofs_loc;jdofs++) { // for each element in row !!!
      printf("%11.3le",Stiff_mat[idofs+jdofs*(*Nrdofs_loc)]);
    }
    printf("\n");
  }
  printf("Element %d: Rhs_vect:\n",neig_id);
  for (idofs=0;idofs<*Nrdofs_loc;idofs++) {
    printf("%11.3le",Rhs_vect[idofs]);
  }
  printf("\n");
  //getchar();
 }
/*kew*/

/*kbw
    printf("In pdr_comp_el_stiff_mat: field_id %d, El_id %d, Comp_sm %d, Nr_dof_ent %d\n",
	   field_id, Fa_id, Comp_sm, *Nr_dof_ent);
    printf("For each block: \ttype, \tid, \tnrdofs\n");
    for(i=0;i<*Nr_dof_ent;i++){
      printf("\t\t\t%d\t%d\t%d\n",
	     List_dof_ent_type[i],List_dof_ent_id[i],List_dof_ent_nrdofs[i]);
    }
    //getchar();//getchar();
/*kew*/

  return(1);
}


/*------------------------------------------------------------
  pdr_read_sol_dofs - to read a vector of dofs associated with a given
                   mesh entity from approximation field data structure
------------------------------------------------------------*/
int pdr_read_sol_dofs(/* returns: >=0 - success code, <0 - error code */
  int Problem_id,     /* in: solver ID (used to identify the subproblem) */
  int Dof_ent_type,
  int Dof_ent_id,
  int Nrdofs,
  double* Vect_dofs  /* in: dofs to be written */
  )
{

  int i;
  int vect_id = 0;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /*kbw
  printf("in pdr_read_sol_dofs before apr_read_ent_dofs\n");
  printf("problem_id %d, Dof_ent_type %d, Dof_ent_id %d, nrdofs %d\n",
	 Problem_id, Dof_ent_type, Dof_ent_id,  Nrdofs);
  /*kew*/

  apr_read_ent_dofs(Problem_id, Dof_ent_type, Dof_ent_id,  Nrdofs,
		    vect_id, Vect_dofs);

  /*kbw
  printf("in pdr_read_sol_dofs after apr_read_ent_dofs\n");
  for(i=0;i<Nrdofs;i++){
    printf("%10.6lf",Vect_dofs[i]);
  }
  printf("\n");
  /*kew*/


  return(1);
}

/*------------------------------------------------------------
  pdr_write_sol_dofs - to write a vector of dofs associated with a given
                   mesh entity to approximation field data structure
------------------------------------------------------------*/
int pdr_write_sol_dofs(/* returns: >=0 - success code, <0 - error code */
  int Problem_id,     /* in: solver ID (used to identify the subproblem) */
  int Dof_ent_type,
  int Dof_ent_id,
  int Nrdofs,
  double* Vect_dofs  /* in: dofs to be written */
  )
{

  int vect_id = 0;

/*++++++++++++++++ executable statements ++++++++++++++++*/


  apr_write_ent_dofs(Problem_id, Dof_ent_type, Dof_ent_id,  Nrdofs,
		    vect_id, Vect_dofs);

  return(1);
}


/*---------------------------------------------------------
  pdr_L2_proj_sol - to project solution between elements of different generations
----------------------------------------------------------*/
int pdr_L2_proj_sol(
  int Problem_id, /* in: problem ID */
  int El,		/* in: element number */
  int *Pdeg_vec,	/* in: element degree of approximation */
  double* Dofs,	/* out: workspace for degress of freedom of El */
  /* 	NULL - write to  data structure */
  int* El_from,	/* in: list of elements to provide function */
  int *Pdeg_vec_from,	/* in: degree of polynomial for each El_from */
  double* Dofs_from /* in: Dofs of El_from or...*/
  )
{

  int field_id = Problem_id;
  int i= -1; /* mode: -1 - projection from father to son */

  apr_L2_proj(field_id, i, El, Pdeg_vec, Dofs, 
	      El_from, Pdeg_vec_from, Dofs_from, NULL);


  return(0);
}


/*---------------------------------------------------------
pdr_renum_coeff - to return a coefficient being a basis for renumbering
----------------------------------------------------------*/
int pdr_renum_coeff(
  int Problem_id, /* in: problem ID */
  int Ent_type,	/* in: type of mesh entity */
  int Ent_id,	/* in: mesh entity ID */
  double* Ren_coeff  /* out: renumbering coefficient */
	)
{

/* auxiliary variables */
  int i, imat, nr_mat, mat_num;
  pdt_material *material; /* pointer to material data */

/*++++++++++++++++ executable statements ++++++++++++++++*/

  *Ren_coeff = 1.0;
  return(1);

/*   if(mat_num>0 && mat_num<=nr_mat) { */
/*     *Ren_coeff = material[mat_num-1].diff_coeff; */
/*   } */
/*   else { */
/*     *Ren_coeff = 1.0; */
/*   } */

/*   return(1); */
}


/*-----	-------------------------------------------------------
  pdr_get_ent_pdeg - to return the degree of approximation index
                      associated with a given mesh entity
------------------------------------------------------------*/
extern int pdr_get_ent_pdeg( /* returns: >0 - approximation index,
                                          <0 - error code */
  int Problem_id,     /* in: approximation field ID  */
  int Ent_type,      /* in: type of mesh entity */
  int Ent_id         /* in: mesh entity ID */
  )
{

  int field_id = Problem_id;
  return(apr_get_ent_pdeg(field_id, Ent_type, Ent_id));

}


/*---------------------------------------------------------
  pdr_dof_ent_sons - to return a list of dof entity sons
---------------------------------------------------------*/
int pdr_dof_ent_sons( /* returns: success >=0 or <0 - error code */
  int Problem_id,     /* in: problem ID  */
  int Ent_type,      /* in: type of mesh entity */
  int Ent_id,        /* in: mesh entity ID */
  int *Ent_sons     /* out: list of dof entity sons */
               	     /* 	Ent_sons[0] - number of sons */
  )
{

  int field_id, mesh_id, i;

  field_id = Problem_id;
  mesh_id = apr_get_mesh_id(field_id);
  if(Ent_type==PDC_ELEMENT){
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
  }
  else if(Ent_type == PDC_FACE){
    mmr_fa_fam(mesh_id, Ent_id, Ent_sons, NULL);
  }
  else if(Ent_type == PDC_EDGE){
    mmr_edge_sons(mesh_id, Ent_id, Ent_sons);
  }
  else{
    Ent_sons[0]=0;
  }

  return(0);
}

/*---------------------------------------------------------
  pdr_proj_sol_lev - to L2 project solution dofs between mesh levels
---------------------------------------------------------*/
int pdr_proj_sol_lev( /* returns: >=0 - success; <0 - error code*/
  int Problem_id, /* in: problem ID */
  int Solver_id,        /* in: solver data structure to be used */
  int Ilev_from,    /* in: level number to project from */
  double* Vec_from, /* in: vector of values to project */
  int Ilev_to,      /* in: level number to project to */
  double* Vec_to    /* out: vector of projected values */
  )
{

#ifdef DEBUG
  printf("No projection implemented in single level solver\n");
#endif

  return(0);
}

/*---------------------------------------------------------
  pdr_vec_norm - to compute a norm of global vector (in parallel)
---------------------------------------------------------*/
double pdr_vec_norm( /* returns: L2 norm of global Vector */
  int Problem_id, /* in: problem ID */
  int Solver_id,        /* in: solver data structure to be used */
  int Level_id,         /* in: level number */
  int Nrdof,            /* in: number of vector components */
  double* Vector        /* in: local part of global Vector */
  )
{

  const int IONE=1;
  double vec_norm = 0.0;
  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
#ifdef PARALLEL
  // simplified setting, only one problem and one field
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);
  vec_norm = appr_sol_vec_norm(field_id, Level_id, Nrdof, Vector);
#endif

  return(vec_norm);
}


/*---------------------------------------------------------
  pdr_sc_prod - to compute a scalar product of two global vectors 
---------------------------------------------------------*/
double pdr_sc_prod( /* retruns: scalar product of Vector1 and Vector2 */
  int Problem_id, /* in: problem ID */
  int Solver_id,        /* in: solver data structure to be used */
  int Level_id,         /* in: level number */
  int Nrdof,           /* in: number of vector components */
  double* Vector1,     /* in: local part of global Vector */
  double* Vector2      /* in: local part of global Vector */
  )
{

  const int IONE=1;
  double sc_prod = 0.0;
  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
#ifdef PARALLEL
  // simplified setting, only one problem and one field
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);
  sc_prod = appr_sol_sc_prod(field_id, Level_id, Nrdof, Vector1, Vector2);
#endif

  return(sc_prod);
}

/*---------------------------------------------------------
  pdr_create_exchange_tables - to create tables to exchange dofs 
---------------------------------------------------------*/
int pdr_create_exchange_tables( 
                      /* returns: >=0 -success code, <0 -error code */
  int Problem_id, /* in: problem ID */
  int Solver_id,        /* in: solver data structure to be used */
  int Level_id,       /* in: level ID */
  int Nr_dof_ent,     /* in: number of DOF entities in the level */
  /* all four subsequent arrays are indexed by block IDs with 1(!!!) offset */
  int* L_dof_ent_type,/* in: list of DOF entities associated with DOF blocks */
  int* L_dof_ent_id,  /* in: list of DOF entities associated with DOF blocks */
  int* L_bl_nrdof,    /* in: list of nrdofs for each dof block */
  int* L_bl_posg,     /* in: list of positions within the global */
                      /*     vector of dofs for each dof block */
  int* L_elem_to_bl,  /* in: list of DOF blocks associated with DOF entities */
  int* L_face_to_bl,  /* in: list of DOF blocks associated with DOF entities */
  int* L_edge_to_bl,  /* in: list of DOF blocks associated with DOF entities */
  int* L_vert_to_bl  /* in: list of DOF blocks associated with DOF entities */
  )
{

  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

#ifdef PARALLEL
  // simplified setting, only one problem and one field
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);

  appr_create_exchange_tables(field_id, Level_id, Nr_dof_ent, L_dof_ent_type,
			      L_dof_ent_id, L_bl_nrdof, L_bl_posg, 
			      L_elem_to_bl, L_face_to_bl,
			      L_edge_to_bl, L_vert_to_bl);
#endif

  return(0);

}

/*---------------------------------------------------------
  pdr_exchange_dofs - to exchange dofs between processors
---------------------------------------------------------*/
int pdr_exchange_dofs(
  int Problem_id, /* in: problem ID */
  int Solver_id,        /* in: solver data structure to be used */
  int Level_id,         /* in: level number */
  double* Vec_dofs  /* in: vector of dofs to be exchanged */
)
{

  int i, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

#ifdef PARALLEL
  // simplified setting, only one problem and one field
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);
  appr_exchange_dofs(field_id, Level_id, Vec_dofs);
#endif

  return(1);
}
