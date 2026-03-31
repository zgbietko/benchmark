/************************************************************************
File pds_test_ls_dg_intf.c - interface between the problem dependent 
         module for testing using Laplace's equation, the discontinuous Galerkin 
         approximation  and linear solver modules (direct and iterative)

Contains definitions of routines:   

  pdr_get_list_ent - to return the list of integration entities (entities
                  for which stiffnes matrices and load vectors will be provided)
                  end DOF entities (entities with associated DOFs)
  pdr_comp_stiff_mat - to provide a solver with a stiffness matrix 
                      and a load vector corresponding to the specified
                      mesh entity, together with information on how to
                      assemble entries into the global stiffness matrix
                      and the global load vector
  pdr_assemble_local_stiff_mat - to assemble an element stiffness matrix
                                   to the global SM
  pdr_read_sol_dofs - to read a vector of dofs associated with a given 
                   mesh entity from approximation field data structure
  pdr_write_sol_dofs - to write a vector of dofs associated with a given 
                   mesh entity to approximation field data structure

  pdr_get_ent_pdeg - to return the degree of approximation index 
                      associated with a given mesh entity
  pdr_dof_ent_sons - to return a list of dof entity sons

  pdr_proj_sol_lev - to project solution between mesh levels
  pdr_vec_norm - to compute a norm of global vector in parallel
  pdr_sc_prod - to compute a scalar product of two global vectors
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
#include "pdh_control_intf.h"

#include "uth_intf.h"

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
const int PDC_VERTEX  = APC_VERTEX ;

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
	/* GHOST ENTITIES HAVE NEGATIVE TYPE !!! */
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
  int* List_dof_ent_nrdof_fine,/* in: list of no of dofs for 'dof' entity */
  int Nrdof_glob_fine,  /* in: global number of degrees of freedom (unknowns) */
  int Max_dof_per_ent_fine, /* in: maximal number of dofs per dof entity */
  int* Pdeg_coarse_p,  /* in: degree of approximation for coarse space */
  int* Nr_int_ent_p,    /* out: number of integration entitites */
  int** List_int_ent_type,/* out: list of types of integration entitites */
  int** List_int_ent_id,  /* out: list of IDs of integration entitites */
  int* Nr_dof_ent_p,    /* out: number of dof entities (entities with which there
		              are dofs associated by the given approximation) */
  int** List_dof_ent_type,/* out: list of types of integration entitites */
  int** List_dof_ent_id,  /* out: list of IDs of integration entitites */
  int** List_dof_ent_nrdof,/* out: list of no of dofs for 'dof' entity */
  int* Nrdof_glob,    /* out: global number of degrees of freedom (unknowns) */
  int* Max_dof_per_ent/* out: maximal number of dofs per dof entity */
  )
{


  int solver_id, field_id, mesh_id;
  int i, iaux, ient, nel, nfa, nreq;
  int nrdofgl, nr_int_ent, nr_dof_ent;
  int nrdofloc, max_dof_ent_dof;
  int ient_fine, int_ent_id, int_ent_type, dof_ent_id, dof_ent_type;
  int father, father_old, pdeg_coarse;
  int* temp_list_elem, * temp_list_face;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* associate the suitable field with the solver */
  /* here there is only one problem one field and one solver */
  solver_id = Problem_id;
  field_id = solver_id;
  mesh_id = apr_get_mesh_id(field_id);
  
  pdeg_coarse = *Pdeg_coarse_p;
  nreq = apr_get_nreq(field_id);

/*ok_kbw*/
  printf("Constructing coarse mesh from the fine mesh: Problem_id %d\n",
	 Problem_id);
  printf("Nr_int_ent_fine %d, Nr_dof_ent_fine %d, Nrdof_glob_fine %d\n",
	 Nr_int_ent_fine, Nr_dof_ent_fine, Nrdof_glob_fine);
  printf("Max_dof_per_ent_fine %d, pdeg_coarse %d\n",
	 Max_dof_per_ent_fine, pdeg_coarse);
/*kew*/

  father_old = -1; nr_int_ent = 0; 
  temp_list_face = (int *) malloc( (Nr_int_ent_fine+1)*sizeof(int) );
  /* in a loop over fine level integration and dof entities */
  /*    - create lists of integration and dof entities for coarse level */
  /*    - find data related to coarse entities */
  for(ient_fine=0; ient_fine<Nr_int_ent_fine; ient_fine++){

    int_ent_id = List_int_ent_id_fine[ient_fine];
    int_ent_type = List_int_ent_type_fine[ient_fine];

/*kbw
    printf("In intent fine: ient %d, id %d, type %d\n",
	   ient_fine, int_ent_id, int_ent_type);
/*kew*/
 
    if(int_ent_type == PDC_ELEMENT){

      father=mmr_el_fam(mesh_id,int_ent_id,NULL,NULL);

/*kbw
      printf("father: %d (previous %d)\n", father, father_old);
/*kew*/

      if(father==0){

	temp_list_face[nr_int_ent]=int_ent_id;
	nr_int_ent++;

      }
      /* we assume families are listed together !!!!*/
      else if(father != father_old){
      
	temp_list_face[nr_int_ent]=father;
	nr_int_ent++;
	father_old = father;
      
      } 
    }
    else if(int_ent_type == PDC_FACE){

      father = mmr_fa_fam(mesh_id, int_ent_id, NULL, NULL);

/*kbw
      printf("father: %d (previous %d)\n", father, father_old);
/*kew*/

      if(father==0){
	int face_neig[2];
	/* check whether neighbors are initial mesh elements */
	mmr_fa_eq_neig(mesh_id, int_ent_id, face_neig, NULL, NULL);
	if(mmr_el_fam(mesh_id, face_neig[0], NULL, NULL)==0){

/*kbw
	  printf("face %d, neig %d, father_neig %d\n", int_ent_id, 
		 face_neig[0], mmr_el_fam(mesh_id, face_neig[0], NULL, NULL));
/*kew*/

	  temp_list_face[nr_int_ent]=int_ent_id;
	  nr_int_ent++;
	}  
      }
      else{

	if(father != father_old){
      
	  temp_list_face[nr_int_ent]=father;
	  nr_int_ent++;
	  father_old = father;
      
	} 


      }
    }
    else{

      printf("Unknown int_ent type %d in get_list_ent_coarse. Exiting!\n",
	     int_ent_type);
	exit(-1);

    }
  }

  father_old = -1; nr_dof_ent = 0;
  temp_list_elem = (int *) malloc( (Nr_dof_ent_fine+1)*sizeof(int));
  /* in a loop over fine level integration and dof entities */
  /*    - create lists of integration and dof entities for coarse level */
  /*    - find data related to coarse entities */
  for(ient_fine=0; ient_fine<Nr_dof_ent_fine; ient_fine++){

    dof_ent_id = List_dof_ent_id_fine[ient_fine];
    dof_ent_type = List_dof_ent_type_fine[ient_fine];

/*kbw
    printf("In dofent fine: ient %d, id %d, type %d\n",
	   ient_fine, dof_ent_id, dof_ent_type);
/*kew*/
 
    if(dof_ent_type == PDC_ELEMENT){

      father=mmr_el_fam(mesh_id,dof_ent_id,NULL,NULL);

/*kbw
      printf("father: %d (previous %d)\n", father, father_old);
/*kew*/

      if(father==0){

	temp_list_elem[nr_dof_ent]=dof_ent_id;
	nr_dof_ent++;

      }
      /* we assume families are listed together !!!!*/
      else if(father != father_old){
      
	temp_list_elem[nr_dof_ent]=father;
	nr_dof_ent++;
	father_old = father;
      
      } 
    }
    else{

      printf("Unknown dof_ent type %d in get_list_ent_coarse. Exiting!\n",
	     dof_ent_type);
	exit(-1);

    }

  }

/*kbw
  printf("%d coarse elements:\n", nr_dof_ent);
  for(i=0;i<nr_dof_ent; i++){
    printf("%d  ", temp_list_elem[i]);
    if(temp_list_elem[i] != temp_list_face[i]){
      printf("Not DG - really?\n");
    }
  }
  printf("\n");
  printf("%d coarse faces:\n", nr_int_ent-nr_dof_ent);
  for(i=nr_dof_ent;i<nr_int_ent; i++){
    printf("%d  ", temp_list_face[i]);
  }
  printf("\n");
/*kew*/

  if(pdeg_coarse <=0){
    iaux = temp_list_elem[0];
    if(mmr_el_status(mesh_id,iaux)==MMC_ACTIVE){ 
      pdeg_coarse = apr_get_ent_pdeg(field_id, APC_ELEMENT, iaux);
    }
    else{ 
      int istop=0;
      int sons[10];
      while(istop==0){
	mmr_el_fam(mesh_id,iaux,sons,NULL);
	for(i=1;i<=sons[0];i++){
	  if(mmr_el_status(mesh_id,sons[i])==MMC_ACTIVE){

	    pdeg_coarse = apr_get_ent_pdeg(field_id, APC_ELEMENT, sons[i]);;

/*kbw
	    printf("original element %d, antecedent %d, pdeg %d, nrdofloc %d\n",
		   nel, sons[i], pdeg_coarse, nrdofloc);
/*kew*/

	    istop=1;
	    break;
	  }
	}
	iaux = sons[1];
      }

    }
  }



/*kbw
  printf("nr_int_ent %d, nr_dof_ent %d\n", nr_int_ent, nr_dof_ent);
/*kew*/

  /* prepare arrays */

  *List_int_ent_type = (int *) malloc( (nr_int_ent+1)*sizeof(int) );
  *List_int_ent_id = (int *) malloc( (nr_int_ent+1)*sizeof(int) );
  *List_dof_ent_type = (int *) malloc( (nr_dof_ent+1)*sizeof(int) );
  *List_dof_ent_id = (int *) malloc( (nr_dof_ent+1)*sizeof(int) );
  *List_dof_ent_nrdof = (int *) malloc( (nr_dof_ent+1)*sizeof(int) );

  if((*List_dof_ent_nrdof)==NULL){
    printf("Not enough space for allocating lists in pdr_get_list_ent! Exiting\n");
    exit(-1);
  } 



  nrdofgl = 0; max_dof_ent_dof = 0;
  for(ient=0;ient<nr_dof_ent; ient++){

    nel = temp_list_elem[ient];
    (*List_int_ent_type)[ient]=PDC_ELEMENT;
    (*List_int_ent_id)[ient]=nel;
      
    nrdofloc = nreq*apr_get_el_pdeg_numshap(field_id, nel, &pdeg_coarse);
    (*List_dof_ent_type)[ient]=PDC_ELEMENT;
    (*List_dof_ent_id)[ient]=nel;
    (*List_dof_ent_nrdof)[ient]=nrdofloc;
      
    if(nrdofloc>max_dof_ent_dof) max_dof_ent_dof = nrdofloc;
    nrdofgl += nrdofloc;
      
/*kbw
	  printf(" element %d ient %d, nrdofloc %d, nrdofgl %d\n", 
		 nel, ient, nrdofloc, nrdofgl);
/*kew*/ 


    
    
  } /* end for all elements */

  
  /* loop over faces - integration entities in DG */
  for(ient=nr_dof_ent;ient<nr_int_ent; ient++){
    nfa = temp_list_face[ient];

    /* add face to the list of integration entities */
    (*List_int_ent_type)[ient]=PDC_FACE;
    (*List_int_ent_id)[ient]=nfa;

/*kbw
	  printf("face %d ient %d\n", nfa, ient);
/*kew*/ 


  } /* end loop over all faces: nfa */ 



    *Nr_int_ent_p = nr_int_ent;
    *Nr_dof_ent_p = nr_dof_ent;
    *Nrdof_glob = nrdofgl;
    *Max_dof_per_ent = max_dof_ent_dof;
    *Pdeg_coarse_p = pdeg_coarse;

/*kbw
    printf("In pdr_get_list_ent\n");
    printf("nr_int_ent %d\n",*Nr_int_ent_p);
    for(i=0;i<nr_int_ent;i++)  printf("type %d, id %d\n",
	 (*List_int_ent_type)[i],(*List_int_ent_id)[i]);
    printf("\nNr_dof_ent_p %d, Nrdof_glob %d, Max_dof_per_ent %d\n",
	   *Nr_dof_ent_p, *Nrdof_glob, *Max_dof_per_ent); 
    for(i=0;i<nr_dof_ent;i++)  printf("type %d, id %d, nrdof %d\n",
	(*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdof)[i]);

    getchar(); getchar();
/*kew*/

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
			       L_dof_ent_id,L_dof_ent_nrdofs, 
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
  int* List_dof_ent_nrdof,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_loc,        /* in(optional): size of Stiff_mat and Rhs_vect */
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
  int* List_dof_ent_nrdof,/* out: list of no of dofs for 'dof' entity */
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
  int* List_dof_ent_nrdof,/* out: list of no of dofs for 'dof' entity */
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

/* in higher order approximation pdeg may be a vector - in DG single number */
  if(Pdeg_vec==NULL) pdeg = 0;
  else pdeg = Pdeg_vec[0];

/*kbw
    printf("in pdr_comp_stiff_mat: problem_id %d, int_ent: type %d, id %d\n", 
	   Problem_id, Int_ent_type, Int_ent_id);
/*kew*/

  if( Int_ent_type == PDC_ELEMENT ){

    pdr_comp_el_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg,
			  Nr_dof_ent, List_dof_ent_type, 
			  List_dof_ent_id, List_dof_ent_nrdof,
			  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs); 


  }
  else if(Int_ent_type==PDC_FACE){

    pdr_comp_fa_stiff_mat(Problem_id, Int_ent_id, Comp_sm, pdeg,
			  Nr_dof_ent, List_dof_ent_type, 
			  List_dof_ent_id, List_dof_ent_nrdof,
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
  int* List_dof_ent_nrdof,/* out: list of no of dofs for 'dof' entity */
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
  double sol_dofs[APC_MAXELSD]; /* solution dofs */

  int field_id, mesh_id;  
  int kk, idofs, jdofs;
  int i,j,k;
  int max_dof_ent, max_nrdofs;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper mesh */
  field_id = Problem_id;
  mesh_id = apr_get_mesh_id(field_id);

/* get material number */
  el_mate =  mmr_el_groupID(mesh_id, El_id);

  /* find degree of polynomial and number of element scalar dofs */
  if(Pdeg_in>0) pdeg = Pdeg_in;
  else{
#ifdef DEBUG
    /* check input for DG approximation */
    if(mmr_el_status(mesh_id,El_id)!=MMC_ACTIVE){
      printf("Asking for pdeg of inactive element in comp_el_stiff_mat !\n");
      exit(-1);
    }
#endif
      pdeg = apr_get_ent_pdeg(field_id, APC_ELEMENT, El_id);
  }
  num_shap = apr_get_el_pdeg_numshap(field_id, El_id, &pdeg);

  max_dof_ent = *Nr_dof_ent;
  *Nr_dof_ent = 1;

#ifdef DEBUG
  if(max_dof_ent < 1){
    printf("Too small arrays List_dof_... passed to comp_el_stiff_mat\n");
    printf("%d < %d. Exiting !!!",max_dof_ent, *Nr_dof_ent);
    exit(-1);
  }
#endif

  List_dof_ent_type[0] = PDC_ELEMENT;
  List_dof_ent_id[0] = El_id;
  /* scalar problem nrdofs = numshap */
  List_dof_ent_nrdof[0] = num_shap;

/*kbw
    printf("In pdr_com_el_stiff_mat: field_id %d, El_id %d, Comp_sm %d, Nr_dof_ent %d\n",
	   field_id, El_id, Comp_sm, *Nr_dof_ent);
    printf("For each block: \ttype, \tid, \tnrdof\n");
    for(i=0;i<*Nr_dof_ent;i++){
      printf("\t\t\t%d\t%d\t%d\n",
	     List_dof_ent_type[i],List_dof_ent_id[i],List_dof_ent_nrdof[i]);
    }
    getchar();getchar();
/*kew*/

  if(Comp_sm!=PDC_NO_COMP){

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


    *Nrdofs_loc = num_shap;
    int num_dofs = num_shap;

    /* get the most recent solution degrees of freedom */
    if(mmr_el_status(mesh_id,El_id)==MMC_ACTIVE){

      /* select solution vector to be used */
      sol_vec_id = 1;
      apr_read_ent_dofs(field_id,APC_ELEMENT,El_id,num_shap,sol_vec_id,sol_dofs);
    }
    else{

      /*!!! coarse element dofs should be supplied by calling routine !!!*/
      for(i=0;i<num_shap;i++) sol_dofs[i]=0;

    }

    if(Comp_sm==PDC_COMP_SM||Comp_sm==PDC_COMP_BOTH){

      /* initialize the matrices to zero */
      for(i=0;i<num_dofs*num_dofs;i++) Stiff_mat[i]=0.0;

    }

    if(Comp_sm==PDC_COMP_RHS||Comp_sm==PDC_COMP_BOTH){
    
      /* initialize the vector to zero */
      for(i=0;i<num_dofs;i++) Rhs_vect[i]=0.0; 

    }

    int diagonal[5]={0,1,0,0,0}; // diagonality of: M, A_ij, B_j, T_i and C  
    // coefficient matrices returned to apr_num_int_el by pdr_el_coeff
    /* perform numerical integration of terms from the weak formualation */
    /* pdeg - taken from approximation data structures (&pdeg==NULL) */
    apr_num_int_el(Problem_id,field_id,El_id, Comp_sm, NULL, sol_dofs, sol_dofs,
		   diagonal, Stiff_mat, Rhs_vect);

/*kbw
    if(El_id>0){
      printf("Element %d: Stiffness matrix:\n",El_id);
      for (idofs=0;idofs<num_shap*PDC_NREQ;idofs++) {
	for (jdofs=0;jdofs<num_shap*PDC_NREQ;jdofs++) {
	  printf("%20.12lf",Stiff_mat[idofs+jdofs*num_shap*PDC_NREQ]);        
	} 
	printf("\n");
      }
      printf("Element %d: Rhs_vect:\n",El_id);
      for (idofs=0;idofs<num_shap*PDC_NREQ;idofs++) {
	printf("%20.12lf",Rhs_vect[idofs]);
      }
      printf("\n");
      getchar(); 
    } 
/*kew*/

    if(Rewr_dofs != NULL) *Rewr_dofs = 'F';

  } /* end if computing SM and/or RHSV */

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
  int* List_dof_ent_nrdof,/* out: list of no of dofs for 'dof' entity */
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
  int base_1, base_2;	/* type of basis functions */
  int nreq;		/* number of equations */
  int pdeg1=0, num_shap1=0;/* local dimension for neigbhor 1 */
  int pdeg2=0, num_shap2=0;/* local dimension for neigbhor 2 */
  int el_mate1=0;	/* material number for neigbhor 1 */
  int el_mate2=0;	/* material number for neigbhor 2 */
  int ngauss;		/* number of gauss points in 1 dimension */
  int face_neig[2];	/* list of neighbors */
  int neig_sides[2]={0,0};/* sides of face wrt neigs */
  int node_shift;
  double acoeff[4], bcoeff[2];/* to transform coordinates between faces */
  double loc_xg[6];	/* local coord of gauss points for neighbors */
  double xg[450];   	/* gauss points in 2D*/
  double wg[150];      	/* gauss weights in 2D*/
  double determ; 	/* determinant of jacobi matrix */
  double hsize1, hsize2; /* size of elements */
  double hf_size;        /* size of face */
  double area; 		/* area for integration rule */
  double time;          /* current time instant */
  double xcoor[3],xcoor2[3];/* global coord of gauss point */
  double vec_norm[3];	/* normal vector to the considered face */
  double u_val1[PDC_NREQ]; /* computed solution */
  double u_x1[PDC_NREQ];   /* gradient of computed solution */
  double u_y1[PDC_NREQ];   /* gradient of computed solution */
  double u_z1[PDC_NREQ];   /* gradient of computed solution */
  double u_val2[PDC_NREQ]; /* computed solution */
  double u_x2[PDC_NREQ];   /* gradient of computed solution */
  double u_y2[PDC_NREQ];   /* gradient of computed solution */
  double u_z2[PDC_NREQ];   /* gradient of computed solution */
  double base_phi1[APC_MAXELVD];    /* basis functions */
  double base_dphix1[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy1[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz1[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_phi2[APC_MAXELVD];    /* basis functions */
  double base_dphix2[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy2[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz2[APC_MAXELVD];  /* y-derivatives of basis function */
  int el_nodes1[MMC_MAXELVNO+1];        /* list of nodes of El */
  int el_nodes2[MMC_MAXELVNO+1];        /* list of nodes of El */
  double node_coor1[3*MMC_MAXELVNO];  /* coord of nodes of El */
  double node_coor2[3*MMC_MAXELVNO];  /* coord of nodes of El */
  double dofs_loc1[APC_MAXELSD]; /* element solution dofs */
  double dofs_loc2[APC_MAXELSD]; /* element solution dofs */
  double anx1[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double any1[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double anz1[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double bn1[PDC_NREQ*PDC_NREQ];   /* coeff of PDE */
  double fval1[PDC_NREQ]; /* coeff of PDE */
  double gval1[PDC_NREQ]; /* coeff of PDE */
  double qn1[PDC_NREQ];      /* source for the first neigbor */
  double anx2[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double any2[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double anz2[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double bn2[PDC_NREQ*PDC_NREQ];   /* coeff of PDE */
  double fval2[PDC_NREQ]; /* coeff of PDE */
  double gval2[PDC_NREQ]; /* coeff of PDE */
  double qn2[PDC_NREQ];      /* source for the second neigbor */
  double norm_vel[PDC_NREQ]; /* normal velocity for each component of u */
  double norm_vel1[PDC_NREQ]; /* normal velocity for each component of u */
  double norm_vel2[PDC_NREQ]; /* normal velocity for each component of u */

  int i, ki, kk, ieq1, ieq2, time_dis;
  int iaux, jaux;
  int idofs, jdofs, ndofs_fa, ndofs1, ndofs2, neig1_id, neig2_id;
  double daux, faux, eaux, gaux, haux, penalty;


  int field_id, mesh_id;  
  int sol_vec_id;       /* indicator for the solution dofs */


/*++++++++++++++++ executable statements ++++++++++++++++*/

  time=0.0;

#ifdef DEBUG
  if(*Nr_dof_ent<2){ // two elements = dof entities per face
    printf("Too small arrays List_dof_... passed to comp_fa_stiff_mat\n");
    printf("%d < 2. Exiting !!!", *Nr_dof_ent);
    exit(-1);
  }
#endif

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
  mmr_fa_neig(mesh_id,Fa_id,face_neig,neig_sides,&node_shift,
					NULL,acoeff,bcoeff);

  /* get material number */
  neig1_id = abs(face_neig[0]);
  base_1=apr_get_base_type(field_id, neig1_id);
  el_mate1 =  mmr_el_groupID(mesh_id, neig1_id);

  if(Pdeg_in>0) pdeg1 = Pdeg_in;
  else{
    /* find degree of polynomial and number of element scalar dofs */
    /*!!! need some trick for coarse solve with different Pdeg !!!*/
#ifdef DEBUG
    /* check input for DG approximation */
    if(mmr_el_status(mesh_id,neig1_id)!=MMC_ACTIVE){
      printf("Asking for pdeg of inactive element in comp_fa_stiff_mat !\n");
      exit(-1);
    }
#endif
    pdeg1 = apr_get_ent_pdeg(field_id, PDC_ELEMENT, neig1_id);
  }
  num_shap1 = apr_get_el_pdeg_numshap(field_id,neig1_id,&pdeg1);
  ndofs1 = num_shap1*nreq;

  *Nr_dof_ent = 1;
  List_dof_ent_type[0] = PDC_ELEMENT;
  List_dof_ent_id[0] = neig1_id;
  List_dof_ent_nrdof[0] = ndofs1;

  if(Comp_sm!=PDC_NO_COMP){
    mmr_el_node_coor(mesh_id,neig1_id,el_nodes1,node_coor1);

    /* get the most recent solution degrees of freedom */
    /*!!! coarse element dofs should be supplied by calling routine !!!*/
    //sol_vec_id = 1;
    //apr_read_ent_dofs(field_id,PDC_ELEMENT,neig1_id,ndofs1,sol_vec_id,
    //	      dofs_loc1);
  }


/* if not on the wall, i.e. if(bc_type==INTERNAL) */
  if(face_neig[1]!=0){

    neig2_id = abs(face_neig[1]);

    base_2=apr_get_base_type(field_id, neig2_id);
    el_mate2 =  mmr_el_groupID(mesh_id, neig2_id);

    if(Pdeg_in>0) pdeg2 = Pdeg_in;
    else{
  /* find degree of polynomial and number of element scalar dofs */
  /*!!! need some trick for coarse solve with different Pdeg !!!*/
#ifdef DEBUG
      /* check input for DG approximation */
      if(mmr_el_status(mesh_id,neig2_id)!=MMC_ACTIVE){
	printf("Asking for pdeg of inactive element in comp_fa_stiff_mat !\n");
	exit(-1);
      }
#endif
      pdeg2 = apr_get_ent_pdeg(field_id, PDC_ELEMENT, neig2_id);
    }
    num_shap2 = apr_get_el_pdeg_numshap(field_id,neig2_id,&pdeg2);
    ndofs2 = num_shap2*nreq;

    *Nr_dof_ent = 2;
    List_dof_ent_type[1] = PDC_ELEMENT;
    List_dof_ent_id[1] = neig2_id;
    List_dof_ent_nrdof[1] = ndofs2;

    if(Comp_sm!=PDC_NO_COMP){
      mmr_el_node_coor(mesh_id,neig2_id,el_nodes2,node_coor2);

      /* get the most recent solution degrees of freedom */
      /*!!! coarse element dofs should be supplied by calling routine !!!*/
      //sol_vec_id = 1;
      //apr_read_ent_dofs(field_id,PDC_ELEMENT,neig2_id,ndofs2,sol_vec_id,
      //	      dofs_loc2);
    }

  } /* end if second neighbor not boundary */ 
  else{
    pdeg2 = 0;
    num_shap2 = 0;
    ndofs2 = 0;
  }

  /*kbw 
#ifdef DEBUG
    printf("In pdr_comp_fa_stiff_mat: field_id %d, mesh_id %d, Fa_id %d, Comp_sm %d\n",
	   field_id, mesh_id, Fa_id, Comp_sm);
    printf("For each block: \ttype, \tid, \tnrdof\n");
    for(i=0;i<*Nr_dof_ent;i++){
      printf("\t\t\t%d\t%d\t%d\n",
	     List_dof_ent_type[i],List_dof_ent_id[i],List_dof_ent_nrdof[i]);
    }
#endif
  /*kew*/

  if(Comp_sm!=PDC_NO_COMP){

   /* initialize the matrices to zero */
   ndofs_fa = ndofs1+ndofs2;

#ifdef DEBUG
   if(*Nrdofs_loc < ndofs_fa ){
    printf("Too small arrays Stiff_mat and Rhs_vect passed to comp_el_stiff_mat\n");
    printf("%d < %d. Exiting !!!", *Nrdofs_loc, ndofs_fa);
    exit(-1);
  }
#endif

   *Nrdofs_loc = ndofs_fa;

   for(i=0;i<ndofs_fa*ndofs_fa;i++) Stiff_mat[i]=0.0;
   for(i=0;i<ndofs_fa;i++) Rhs_vect[i]=0.0; 
    
    /* prepare data for gaussian integration */
   if(face_neig[1]!=0){
     apr_set_quadr_2D(fa_type,base_1,&pdeg1, &iaux,NULL,NULL);
     apr_set_quadr_2D(fa_type,base_2,&pdeg2, &jaux,NULL,NULL);
     if(iaux>jaux) apr_set_quadr_2D(fa_type,base_1,&pdeg1,&ngauss,xg,wg); 
     else apr_set_quadr_2D(fa_type,base_2,&pdeg2,&ngauss,xg,wg);
   }
   else apr_set_quadr_2D(fa_type,base_1,&pdeg1,&ngauss,xg,wg);
   
/*kb!!!!!!!!!!!!!!*/
   penalty=1.0;    
/*kb!!!!!!!!!!!!!!*/

   if(base_1==APC_BASE_COMPLETE_DG){
     if(face_neig[1]!=0) penalty *= ut_min(pdeg1,pdeg2);
     else penalty *= pdeg1;
   }
   else{
     if(face_neig[1]!=0){
       daux=ut_min(pdeg1/100,pdeg1%100);
       eaux=ut_min(pdeg2/100,pdeg2%100);
       penalty*=ut_min(daux,eaux);
     }
     else penalty*=ut_min(pdeg1/100,pdeg1%100);
   }
   

    /*kbw 
#ifdef DEBUG
    if(Fa_id>0){
      printf("in stiff_face: face %d, type %d, bc_type %d, NREQ %d, base-type %d, node_shift %d\n",
	     Fa_id,fa_type, bc_type,nreq,base_1, node_shift);
      printf("neigh1: %d, side %d, pdeg %d, num_shap %d\n",
	     face_neig[0],neig_sides[0],pdeg1,num_shap1);
      for(i=0;i<el_nodes1[0];i++){
	printf("node %d (global - %d): x - %f, y - %f, z - %f\n",
	       i,el_nodes1[i+1],node_coor1[3*i],
	       node_coor1[3*i+1],node_coor1[3*i+2]);
      }
      if(face_neig[1]!=0){
	printf("neigh2: %d, side %d, pdeg %d, num_shap %d\n",
	       face_neig[1],neig_sides[1],pdeg2,num_shap2);
	for(i=0;i<el_nodes2[0];i++){
	  printf("node %d (global - %d): x - %f, y - %f, z - %f\n",
		 i,el_nodes2[i+1],node_coor2[3*i],
		 node_coor2[3*i+1],node_coor2[3*i+2]);
	}
      }
      printf("transformation coefficients: a %lf, b %lf\n",acoeff,bcoeff);
      printf("Gauss points:\n");
      for (ki=0;ki<ngauss;ki++) {
	printf("Xg: %lf, %lf, Weight: %lf\n", xg[2*ki], xg[2*ki+1], wg[ki]);
      }
      getchar();
    }
#endif
  /*kew*/

 
/* loop over integration points */
   for (ki=0;ki<ngauss;ki++) {

/* find coordinates within neighboring elements for a point on face */
    mmr_fa_elem_coor(mesh_id,&xg[2*ki],face_neig,neig_sides,node_shift,
			acoeff,bcoeff,loc_xg);

/* at the gauss point for neig 1, compute basis functions */
    iaux = 3+neig_sides[0]; /* boundary data for face neig_sides[0] */

    determ = apr_elem_calc_3D(iaux, nreq, &pdeg1, base_1,
			      loc_xg,node_coor1,dofs_loc1,
			      base_phi1,base_dphix1,base_dphiy1,base_dphiz1,
			      xcoor,u_val1,u_x1,u_y1,u_z1,vec_norm);

/* get coefficients of convection-diffusion-reaction equations */
    pdr_fa_coeff(Fa_id,bc_type,neig1_id,el_mate1,
		   hsize1,time,pdeg1,
		   xcoor,vec_norm,u_val1,u_x1,u_y1,u_z1,
		   anx1,any1,anz1,bn1,fval1,gval1,qn1,norm_vel1);

/* make the normal velocity returned by the first neighbor default */
    for(i=0;i<PDC_NREQ;i++) norm_vel[i]=norm_vel1[i];

    if(face_neig[1]!=0) {

/* if first neighbor is big, we need proper determinant of Jacobian */
      if(face_neig[0]<0){
        iaux = 3+neig_sides[1]; /* boundary data for face neig_sides[1] */

        determ = apr_elem_calc_3D(iaux, nreq, &pdeg2, base_2,
				  &loc_xg[3],node_coor2,dofs_loc2,
				  base_phi2,base_dphix2,base_dphiy2,base_dphiz2,
				  xcoor2,u_val2,u_x2,u_y2,u_z2,NULL);
      }
      else{
        iaux = 2; /* no boundary data */
        apr_elem_calc_3D( iaux, nreq, &pdeg2, base_2,
			   &loc_xg[3],node_coor2,dofs_loc2,
			   base_phi2,base_dphix2,base_dphiy2,base_dphiz2,
			   xcoor2,u_val2,u_x2,u_y2,u_z2,NULL);
      }

/* get coefficients from the second neighbor */
      pdr_fa_coeff(Fa_id,bc_type,abs(face_neig[1]),
		    el_mate2,hsize2,time,pdeg2,
		    xcoor,vec_norm,u_val2,u_x2,u_y2,u_z2,
		    anx2,any2,anz2,bn2,fval2,gval2,qn2,norm_vel2);


#ifdef DEBUG
      for(i=0;i<PDC_NREQ;i++){
	if(bc_type==PDC_BC_MIXED && norm_vel1[i] * norm_vel2[i] < -1e-6){
	printf("opposite signs for normal velocities on face %d:\n",Fa_id);
	printf("%lf != %lf\n",norm_vel1[i],norm_vel2[i]);
	}
      }
#endif

      for(i=0;i<PDC_NREQ;i++) norm_vel[i] = 0.5 * ( norm_vel1[i] + norm_vel2[i] );

#ifdef DEBUG
      for(i=0;i<3;i++){
        if(fabs(xcoor2[i]-xcoor[i])>1e-9){
          printf("%lf != %lf\n",xcoor[i],xcoor2[i]);
          printf("Coord in neigh1 != coord in neigh2 for face %d!\n",Fa_id);
          return(-1);
        }
      }
#endif

    } /* end if exist second neighbor */

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
printf("NORMAL velocities: \neig1= %lf, neig2= %lf, average= %lf\n",
	 norm_vel1[0],norm_vel2[0],norm_vel[0]);
printf("coord for neigh1: %lf, %lf, %lf, hsize %lf\n",
loc_xg[0],loc_xg[1],loc_xg[2],hsize1);
if(face_neig[1]!=0)
  printf("coord for neigh2: %lf, %lf, %lf, hsize %lf\n",
loc_xg[3],loc_xg[4],loc_xg[5],hsize2);
printf("weight %lf, integration coeff. %lf\n", wg[ki], area);
printf("%d shape functions and derivatives for neigh1: \n",num_shap1);
for(i=0;i<num_shap1;i++){
  printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
  base_phi1[i],base_dphix1[i],base_dphiy1[i],base_dphiz1[i]);
}
if(face_neig[1]!=0){
 printf("%d shape functions and derivatives for neigh2: \n",num_shap2);
 for(i=0;i<num_shap2;i++){
  printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
  base_phi2[i],base_dphix2[i],base_dphiy2[i],base_dphiz2[i]);
 }
}
  printf("solution %lf, der %lf, %lf, %lf\n", 
	 u_val1[0],u_x1[0],u_y1[0],u_z1[0]);
  printf("diffusion coeff : %lf %lf %lf\n",anx1[0],any1[0],anz1[0]);
  printf("convection coeff: %lf\n",bn1[0]);
  printf("rhs coeff       : %lf %lf %lf\n",fval1[0],gval1[0],qn1[0]);
  if(face_neig[1]!=0){
  printf("solution for neigh2: %lf, der %lf, %lf, %lf\n", 
	 u_val2[0],u_x2[0],u_y2[0],u_z2[0]);
    printf("diffusion coeff : %lf %lf %lf\n",anx2[0],any2[0],anz2[0]);
    printf("convection coeff: %lf\n",bn2[0]);
    printf("rhs coeff       : %lf %lf %lf\n",fval2[0],gval2[0],qn2[0]);
  }
  printf("penalty %lf : fval1[0] %lf, hf_size %lf\n",
       penalty,fval1[0],hf_size);
 
getchar();
}
/*kew*/ 


    if(bc_type==PDC_INTERIOR){

      for(ieq2=0;ieq2<PDC_NREQ;ieq2++){
        for(ieq1=0;ieq1<PDC_NREQ;ieq1++){

          kk=(ndofs_fa*ieq2+ieq1)*num_shap1;

          for (jdofs=0;jdofs<num_shap1;jdofs++) {
            for (idofs=0;idofs<num_shap1;idofs++) {

/* stiffness matrices */
              Stiff_mat[kk+idofs] += (
			        (
	 base_phi1[jdofs] * (
            anx1[ieq1*PDC_NREQ+ieq2] * base_dphix1[idofs] +
            any1[ieq1*PDC_NREQ+ieq2] * base_dphiy1[idofs] +
            anz1[ieq1*PDC_NREQ+ieq2] * base_dphiz1[idofs]
                          )	  
	-base_phi1[idofs] * (
            anx1[ieq1*PDC_NREQ+ieq2] * base_dphix1[jdofs] +
            any1[ieq1*PDC_NREQ+ieq2] * base_dphiy1[jdofs] +
            anz1[ieq1*PDC_NREQ+ieq2] * base_dphiz1[jdofs]
                          ) 
				) *  0.5
/*penalty*/
	 + penalty*base_phi1[jdofs]*base_phi1[idofs]/hf_size
/*penalty*/
				       ) * area;

            }/* idofs */

            kk+=ndofs_fa;

          } /* jdofs */

          kk=ndofs_fa*(ndofs1+ieq2*num_shap2)+ieq1*num_shap1;

          for (jdofs=0;jdofs<num_shap2;jdofs++) {
            for (idofs=0;idofs<num_shap1;idofs++) {

              Stiff_mat[kk+idofs] += (
			        (
	-base_phi2[jdofs] * (
            anx1[ieq1*PDC_NREQ+ieq2] * base_dphix1[idofs] +
            any1[ieq1*PDC_NREQ+ieq2] * base_dphiy1[idofs] +
            anz1[ieq1*PDC_NREQ+ieq2] * base_dphiz1[idofs]
                          )	  
	-base_phi1[idofs] * (
            anx2[ieq1*PDC_NREQ+ieq2] * base_dphix2[jdofs] +
            any2[ieq1*PDC_NREQ+ieq2] * base_dphiy2[jdofs] +
            anz2[ieq1*PDC_NREQ+ieq2] * base_dphiz2[jdofs]
                          ) 
				) *  0.5
/*penalty*/
	 - penalty*base_phi2[jdofs]*base_phi1[idofs]/hf_size
/*penalty*/
				       ) * area;

            }/* idofs */

            kk+=ndofs_fa;

          } /* jdofs */

          kk=ndofs_fa*ieq2*num_shap1+ndofs1+ieq1*num_shap2;

          for (jdofs=0;jdofs<num_shap1;jdofs++) {
            for (idofs=0;idofs<num_shap2;idofs++) {

              Stiff_mat[kk+idofs] += (
			        (
	 base_phi1[jdofs] * (
            anx2[ieq1*PDC_NREQ+ieq2] * base_dphix2[idofs] +
            any2[ieq1*PDC_NREQ+ieq2] * base_dphiy2[idofs] +
            anz2[ieq1*PDC_NREQ+ieq2] * base_dphiz2[idofs]
                          )	  
	 +base_phi2[idofs] * (
            anx1[ieq1*PDC_NREQ+ieq2] * base_dphix1[jdofs] +
            any1[ieq1*PDC_NREQ+ieq2] * base_dphiy1[jdofs] +
            anz1[ieq1*PDC_NREQ+ieq2] * base_dphiz1[jdofs]
                          ) 
				) *  0.5 
/*penalty*/
	 - penalty*base_phi1[jdofs]*base_phi2[idofs]/hf_size
/*penalty*/
				       ) * area;


            }/* idofs */

            kk+=ndofs_fa;

          } /* jdofs */

          kk=ndofs_fa*(ndofs1+ieq2*num_shap2)+ ndofs1+ieq1*num_shap2;

          for (jdofs=0;jdofs<num_shap2;jdofs++) {
            for (idofs=0;idofs<num_shap2;idofs++) {

              Stiff_mat[kk+idofs] += (
			        (
	-base_phi2[jdofs] * (
            anx2[ieq1*PDC_NREQ+ieq2] * base_dphix2[idofs] +
            any2[ieq1*PDC_NREQ+ieq2] * base_dphiy2[idofs] +
            anz2[ieq1*PDC_NREQ+ieq2] * base_dphiz2[idofs]
                          )	  
	+base_phi2[idofs] * (
            anx2[ieq1*PDC_NREQ+ieq2] * base_dphix2[jdofs] +
            any2[ieq1*PDC_NREQ+ieq2] * base_dphiy2[jdofs] +
            anz2[ieq1*PDC_NREQ+ieq2] * base_dphiz2[jdofs]
                          ) 
				) *  0.5
/*penalty*/
	 + penalty*base_phi2[jdofs]*base_phi2[idofs]/hf_size
/*penalty*/
				       ) * area;

            }/* idofs */

            kk+=ndofs_fa;

          } /* jdofs */

        }/* ieq1 */
      } /* ieq2 */

    }
    else if(bc_type==PDC_BC_DIRI){

      for(ieq2=0;ieq2<PDC_NREQ;ieq2++){
        for(ieq1=0;ieq1<PDC_NREQ;ieq1++){

/* fortran-like style - columnwise storage of matrices */
          kk=(ieq2*PDC_NREQ*num_shap1+ieq1)*num_shap1;

          for (jdofs=0;jdofs<num_shap1;jdofs++) {
            for (idofs=0;idofs<num_shap1;idofs++) {

/* stiffness matrix */ 
              Stiff_mat[kk+idofs] += (
			(
		(
	    anx1[ieq1*PDC_NREQ+ieq2]*base_dphix1[idofs]
	   +any1[ieq1*PDC_NREQ+ieq2]*base_dphiy1[idofs]
	   +anz1[ieq1*PDC_NREQ+ieq2]*base_dphiz1[idofs]
		)*base_phi1[jdofs] +
		(
	   -anx1[ieq1*PDC_NREQ+ieq2]*base_dphix1[jdofs]
	   -any1[ieq1*PDC_NREQ+ieq2]*base_dphiy1[jdofs]
	   -anz1[ieq1*PDC_NREQ+ieq2]*base_dphiz1[jdofs]
		)*base_phi1[idofs]
			) 
/*penalty*/
	 + penalty*base_phi1[jdofs]*base_phi1[idofs]/hf_size
/*penalty*/
                                         ) * area;

            }/* idofs */

            kk+=num_shap1*PDC_NREQ;

          } /* jdofs */
        }/* ieq1 */
      } /* ieq2 */

      for(ieq1=0;ieq1<PDC_NREQ;ieq1++){
        for (idofs=0;idofs<num_shap1;idofs++) {

/* right hand side vector */
          daux=0.0;faux=0.0;eaux=0.0;gaux=0.0;
          for(ieq2=0;ieq2<PDC_NREQ;ieq2++){
            daux +=
           	  anx1[ieq1*PDC_NREQ+ieq2]  * fval1[ieq2];
            faux +=
           	  any1[ieq1*PDC_NREQ+ieq2]  * fval1[ieq2];
            gaux +=
           	  anz1[ieq1*PDC_NREQ+ieq2]  * fval1[ieq2];
	  }

          Rhs_vect[ieq1*num_shap1+idofs] += (
		daux*base_dphix1[idofs] +
		faux*base_dphiy1[idofs] +
		gaux*base_dphiz1[idofs] 
/*penalty*/
	 + penalty*fval1[ieq1]*base_phi1[idofs]/hf_size
/*penalty*/
					) * area;

        }/* ieq1 */
      }/* idofs */

    }
    else if(bc_type==PDC_BC_NEUM){

      for(ieq1=0;ieq1<PDC_NREQ;ieq1++){
        kk=ieq1*num_shap1;

        for (idofs=0;idofs<num_shap1;idofs++) {

/* right hand side vector */
          Rhs_vect[kk] += gval1[ieq1]*base_phi1[idofs]*area;
          kk++;

        }/* idofs */
      }/* ieq1 */
    } /* if Neumann BC */
    else if(bc_type==PDC_BC_MIXED){

      for(ieq1=0;ieq1<PDC_NREQ;ieq1++){

/* fortran-like style - columnwise storage of matrices */
	kk=ieq1*num_shap1;

	for (jdofs=0;jdofs<num_shap1;jdofs++) {
	  for (idofs=0;idofs<num_shap1;idofs++) {

/* stiffness matrix */ 
	    Stiff_mat[kk+idofs] += (
/* normal velocity norm_vel gives proportionality constant between */
/* the difference in values and the flux */
	 - norm_vel[ieq1] * base_phi1[jdofs]*base_phi1[idofs] 
/*kb!!! penalty?
	 + penalty/hf_size * base_phi1[jdofs]*base_phi1[idofs]
/*kb!!! penalty*/
	                              ) * area;

	  }/* idofs */

	  kk+=num_shap1*PDC_NREQ;

	} /* jdofs */
      }/* ieq1 */

      for(ieq1=0;ieq1<PDC_NREQ;ieq1++){
        for (idofs=0;idofs<num_shap1;idofs++) {

/* right hand side vector */

          Rhs_vect[ieq1*num_shap1+idofs] += (
	 - norm_vel[ieq1] *fval1[ieq1]*base_phi1[idofs]
/*kb!!! penalty?
	 + penalty/hf_size *fval1[ieq1]*base_phi1[idofs]
/*kb!!! penalty*/
					) * area;

        }/* ieq1 */
      }/* idofs */

    }/* if mixed type BC */
 
  } /* ki */


/*kbw
   if(Fa_id>0){
     printf("Stiffness matrix:\n");
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
     getchar();  
   }
/*kew*/

   if(Rewr_dofs != NULL) *Rewr_dofs = 'F';

  } /* end if computing entries to the stiffness matrix */

/*kbw
#ifdef DEBUG
    {
      int ibl;
      printf("leaving pdr_fa_stiff_mat:\n");
      printf("face %d, nrdofbl %d\n", Fa_id, *Nr_dof_ent);
      for(ibl=0;ibl<*Nr_dof_ent; ibl++){
	printf("bl_id %d, bl_nrdof %d\n",
	  List_dof_ent_id[ibl],List_dof_ent_nrdof[ibl]);
      }
    }
#endif
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
  int Nrdof,
  double* Vect_dofs  /* in: dofs to be written */
  )
{

  int i;
  int vect_id = 0;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /*kbw
  printf("in pdr_read_sol_dofs before apr_read_ent_dofs\n");
  printf("problem_id %d, Dof_ent_type %d, Dof_ent_id %d, nrdof %d\n",
	 Problem_id, Dof_ent_type, Dof_ent_id,  Nrdof);
  /*kew*/

  apr_read_ent_dofs(Problem_id, Dof_ent_type, Dof_ent_id,  Nrdof,
		    vect_id, Vect_dofs);

  /*kbw
  printf("in pdr_read_sol_dofs after apr_read_ent_dofs\n");
  for(i=0;i<Nrdof;i++){
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
  int Nrdof,
  double* Vect_dofs  /* in: dofs to be written */
  )
{

  int vect_id = 0;

/*++++++++++++++++ executable statements ++++++++++++++++*/


  apr_write_ent_dofs(Problem_id, Dof_ent_type, Dof_ent_id,  Nrdof,
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

  return(1);

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


/*------------------------------------------------------------
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

  // BELOW IS THE OLD VERSION THAT SHOULD BE ADAPTEED TO NEW STRUCTURE OF THE CODE

/* /\*--------------------------------------------------------- */
/* pbr_proj_sol - to L2 project dofs vectors between meshes */
/* ---------------------------------------------------------*\/ */
/* int pbr_proj_sol( /\* returns: 1 - success; <=0 - error code*\/ */
/* 	int *Solv,        /\* in: solver data structure to be used *\/ */
/* 	int Ilev_from,    /\* in: level number to project from *\/ */
/*         double* Vec_from, /\* in: vector of values to project *\/ */
/* 	int Ilev_to,      /\* in: level number to project to *\/ */
/*         double* Vec_to    /\* out: vector of projected values *\/ */
/* 	) */
/* { */

/* /\* auxiliary variables *\/ */
/*   static int precomp=0; */
/*   static double shap_fun_precomp[MAX_NUM_ANC][MAX_EL_DOFS][MAX_EL_DOFS]; */
/*   IT_LEVELS *it_level_coarse, *it_level_fine; */
/*   int i, iaux, iel, ielaux, nel, nelaux, base_q, *Prob, nr_levels; */
/*   int nreq, posglob_c, posglob_f, pdeg_coarse, ndof_coarse, ndof_fine; */
/*   int idofs, jdofs, ison, fath, el_sons[MAX_NUM_ANC+1], pdeg_fine; */
/*   double shap_fun_proj[MAX_EL_DOFS]; */

/* /\*++++++++++++++++ executable statements ++++++++++++++++*\/ */

/* /\* get pointer to problem number *\/  */
/*   Prob = &It_solver[*Solv].Prob; */
  
/* /\* set number of levels *\/ */
/*   nr_levels = It_solver[*Solv].It_nr_level; */

/* /\* we will perform L2 projections of shape functions from coarse to fine *\/ */
/* /\* elements and use obtained coefficient matrices to perform actual *\/ */
/* /\* projections of dofs *\/ */

/* /\* projecting from coarser to finer grid *\/ */
/*   if(Ilev_from<Ilev_to) { */

/*     it_level_coarse = &It_solver[*Solv].It_level[Ilev_from]; */
/*     it_level_fine = &It_solver[*Solv].It_level[Ilev_to]; */

/*   } */
/*   else{ */
    
/*     it_level_coarse = &It_solver[*Solv].It_level[Ilev_to]; */
/*     it_level_fine = &It_solver[*Solv].It_level[Ilev_from]; */
    
/*   } */
  
/* /\* check *\/ */
/*   if(abs(it_level_coarse->Mesh_gen-it_level_fine->Mesh_gen)>1){ */
/*     printf("Cannot project with too much difference between generations!\n"); */
/*     return(0); */
/*   } */

/* /\* get necessary problem parameters *\/ */
/*   i=3; nreq=dgfr_dgfem_i_params(Prob,i); */
/*   i=6; base_q=dgfr_dgfem_i_params(Prob,i); */
  
/* /\* for each element active in a coarse mesh*\/ */
/*   for(iel=1;iel<=it_level_coarse->Mesh_elems[0];iel++){ */

/*     ielaux=it_level_coarse->Mesh_elems[iel]; */

/* /\* position in global vectors of unknowns *\/ */
/*     posglob_c = it_level_coarse->Block[iel]->Posg; */

/* /\* number of dofs associated with iel in the coarse mesh *\/ */
/*     pdeg_coarse = it_level_coarse->Pdeg_coarse; */
    
/*     ndof_coarse = nreq*dgfr_pdeg_numdofs(pdeg_coarse,base_q, */
/* 					 dgfr_el_type(Prob,ielaux)); */
    
/* /\* first check whether the element do not belong to both meshes *\/ */
/*     if(it_level_fine->Elem_nums[ielaux]>0){ */
      
/*       nel = it_level_fine->Elem_nums[ielaux]; */

/* /\* check *\/ */
/*       if(it_level_fine->Mesh_elems[nel]!=ielaux){ */
/* 	printf("Error in renumbering of elements in proj_sol!\n"); */
/* 	return(0); */
/*       } */
      

/* /\* position in global vectors of unknowns *\/ */
/*       posglob_f = it_level_fine->Block[nel]->Posg; */

/* /\* projecting from coarser to finer grid *\/ */
/*       if(Ilev_from<Ilev_to) { */
/* 	for(i=0;i<ndof_coarse;i++) Vec_to[posglob_f+i] */
/* 				     = Vec_from[posglob_c+i]; */
/*       }  */
/* /\* if projecting from  finer to coarser mesh *\/ */
/*       else if(Ilev_to<Ilev_from) { */
/* 	for(i=0;i<ndof_coarse;i++) Vec_to[posglob_c+i] */
/* 				     = Vec_from[posglob_f+i]; */
/*       } */
      
/*     } */
/*     else{ /\* element do not belong to both meshes *\/ */

/* /\* find list of descendents belonging to the next mesh *\/ */
/*       dgfr_el_fam(Prob,ielaux,el_sons,NULL); */

/* /\* coefficient matrix has not been computed yet *\/ */
/*       if(precomp==0){ */

/* /\* loop over all sons *\/ */
/* 	for(ison=0;ison<el_sons[0];ison++){ */
	
/* 	  nelaux=el_sons[ison+1]; */
/* 	  nel = it_level_fine->Elem_nums[nelaux]; */

/* /\* position in global vector of unknowns *\/ */
/* 	  posglob_f = it_level_fine->Block[nel]->Posg; */

/* /\* number of dofs associated with iel in the fine mesh *\/ */
/* 	  if(Ilev_from<Ilev_to) { */
/* 	    if(Ilev_to<nr_levels-1) pdeg_fine = it_level_fine->Pdeg_coarse; */
/* 	    else pdeg_fine = dgfr_el_pdeg(Prob,nelaux); */
/* 	  } */
/* 	  else if(Ilev_from>Ilev_to) { */
/* 	    if(Ilev_from<nr_levels-1) pdeg_fine = it_level_fine->Pdeg_coarse; */
/* 	    else pdeg_fine = dgfr_el_pdeg(Prob,nelaux); */
/* 	  } */

/* 	  ndof_fine = nreq*dgfr_pdeg_numdofs(pdeg_fine,base_q, */
/* 					     dgfr_el_type(Prob,nelaux)); */

/* /\* for all degrees of freedom of a coarse element *\/ */
/* 	  for(idofs=0;idofs<ndof_coarse;idofs++) { */

/* /\* coefficients will be computed for every coarse shape function *\/ */
/* 	    double dofs_temp[MAX_EL_DOFS]; */
/* 	    for(jdofs=0;jdofs<ndof_coarse;jdofs++) dofs_temp[jdofs]=0.0; */
/* 	    dofs_temp[idofs]=1.0; */

/* /\* L2 project: to fine element (nelaux) with dofs in shap_fun_precomp *\/ */
/* /\*             from coarse element (ielaux) with dofs dofs_temp *\/ */
/* 	    i = -1; */
/* 	    dgfr_L2_proj(Prob,i, */
/* 			 nelaux,pdeg_fine, */
/* 			 shap_fun_precomp[ison][idofs], */
/* 			 &ielaux,&pdeg_coarse,dofs_temp); */
		
/* 	    precomp=pdeg_coarse*10000+pdeg_fine*10+base_q; */
/* 	  } */
/* 	} */
/*       } /\* end if coefficinets have not been precomputed yet *\/ */

/* /\* when rewriting from fine mesh - initialize coarse dofs *\/ */
/*       if(Ilev_from>Ilev_to)  */
/* 	for(idofs=0;idofs<ndof_coarse;idofs++) Vec_to[posglob_c+idofs] = 0.0; */

/* /\* loop over all sons *\/ */
/*       for(ison=0;ison<el_sons[0];ison++){ */
	
/* 	nelaux=el_sons[ison+1]; */
/* 	nel = it_level_fine->Elem_nums[nelaux]; */

/* /\* position in global vector of unknowns *\/ */
/* 	posglob_f = it_level_fine->Block[nel]->Posg; */

/* /\* number of dofs associated with iel in the fine mesh *\/ */
/* 	if(Ilev_from<Ilev_to) { */
/* 	  if(Ilev_to<nr_levels-1) pdeg_fine = it_level_fine->Pdeg_coarse; */
/* 	  else pdeg_fine = dgfr_el_pdeg(Prob,nelaux); */
/* 	} */
/* 	else if(Ilev_from>Ilev_to) { */
/* 	  if(Ilev_from<nr_levels-1) pdeg_fine = it_level_fine->Pdeg_coarse; */
/* 	  else pdeg_fine = dgfr_el_pdeg(Prob,nelaux); */
/* 	} */

/* 	ndof_fine = nreq*dgfr_pdeg_numdofs(pdeg_fine,base_q, */
/* 					   dgfr_el_type(Prob,nelaux)); */

/* /\* use precomputed coefficients to project dofs *\/ */
/* 	if(precomp==pdeg_coarse*10000+pdeg_fine*10+base_q){ */

/* /\* if projecting from coarser to finer mesh *\/ */
/* 	  if(Ilev_to>Ilev_from){ */

/* 	    for(jdofs=0;jdofs<ndof_fine;jdofs++){ */
/* 	      Vec_to[posglob_f+jdofs] = 0.0; */
/* 	      for(idofs=0;idofs<ndof_coarse;idofs++) { */
		
/* 		Vec_to[posglob_f+jdofs] += Vec_from[posglob_c+idofs]  *  */
/* 		  shap_fun_precomp[ison][idofs][jdofs];  */
/* 	      } */
/* 	    } */
/* 	  } */
/* /\* if projecting from  finer to coarser mesh *\/ */
/* 	  else if(Ilev_to<Ilev_from){ */

/* /\*kbw */
/* printf("PROJECTING: from %d, posglob %d, to %d, posglob %d\n", */
/* nelaux, posglob_f, ielaux, posglob_c); */
/* /\*kbw*\/ */

/* 	    for(idofs=0;idofs<ndof_coarse;idofs++) { */
/* 	      for(jdofs=0;jdofs<ndof_fine;jdofs++){ */
		
/* 		Vec_to[posglob_c+idofs] += Vec_from[posglob_f+jdofs]  *  */
/* 		  shap_fun_precomp[ison][idofs][jdofs];  */

/* /\*kbw */
/* printf("idofs %d jdofs %d vec_to %lf, vec_from %lf, coeff %lf\n", */
/* idofs, jdofs, Vec_to[posglob_c+idofs], Vec_from[posglob_f+jdofs], */
/* shap_fun_precomp[ison][idofs][jdofs]); */
/* /\*kbw*\/ */

/* 	      } */
/* 	    } */
/* /\*kbw */
/* getchar(); */
/* /\*kbw*\/ */

/* 	  } /\* end if projecting from finer to coarse *\/ */
	  
/* 	} */
/* 	else{ */
/* 	  printf("Precomputed coefficients do not match pdeg_coarse %d \ */
/* pdeg_fine %d and base_q %d \n", */
/* 		 pdeg_coarse,pdeg_fine,base_q); */
/* 	  return(-1); */
/* 	} */


/*       } /\* end loop over sons: ison *\/ */

/*     } /\* end if element does not exist in both meshes *\/ */
    
/*   } /\* end loop over all elements of a "to" mesh *\/ */

/*   return(1); */
/* } */


// ANOTHER VERSION WITH SLIGHTLY DIFFERENT INTERFACE

/* /\*--------------------------------------------------------- */
/*   pdr_proj_sol_lev - to L2 project a solution between mesh levels */
/* ---------------------------------------------------------*\/ */
/* int pdr_proj_sol_lev( /\* returns: 1 - success; <=0 - error code*\/ */
/*   int Solver_id,       /\* in: solver ID *\/ */
/*   int Action,         /\* in: control flag to indicate action, values: *\/ */
/*                        /\*     FEC_RESTRICT or FEC_PROLONGATE *\/  */
/*   int Ilev_coarse,     /\* in: coarse grid level number *\/ */
/*   int* L_nrdof_coarse, /\* in: coarse grid list of nrdofs for each dof block *\/ */
/*   int* L_posg_coarse,  /\* in: coarse grid list of positions within the global *\/ */
/*                        /\*     vector of dofs for each dof block *\/ */
/*   double* Vec_coarse,  /\* in: global vector of dofs for coarse grid *\/ */
/*   int Ilev_fine,       /\* in: fine grid level number *\/ */
/*   int* L_nrdof_fine,   /\* in: fine grid list of nrdofs for each dof block *\/ */
/*   int* L_posg_fine,    /\* in: fine grid list of positions within the global *\/ */
/*                        /\*     vector of dofs for each dof block *\/ */
/*   double* Vec_fine     /\* out: vector of projected values *\/ */
/*   ) */
/* { */

/* #define MAX_NUM_ANC     MMC_MAXELSONS  /\* maximal number of sons for element *\/ */
/* #define MAX_EL_DOFS     APC_MAXELSD    /\* maximal number of dofs for element *\/ */
/* #define MAX_PROJ_COMB   2 /\* maximal number of pdeg combinations *\/ */

/*   /\* pointer to renumbering arrays structure for a given level *\/ */
/*   pdt_renum_level *p_renum_coarse,  *p_renum_fine; */
 
/*   /\* auxiliary variables *\/ */
/*   static int nr_precomp=0; */
/*   static int precomp_ind[MAX_PROJ_COMB]; */
/*   static double  */
/*     shap_fun_precomp[MAX_PROJ_COMB][MAX_NUM_ANC][MAX_EL_DOFS][MAX_EL_DOFS]; */
/*   int i, iaux, iel, iel_coarse, iel_fine, base_q, field_id, mesh_id; */
/*   int posglob_coarse, posglob_fine, pdeg_coarse, pdeg_fine; */
/*   int ndof_coarse, ndof_fine, ibl_coarse, ibl_fine; */
/*   int idofs, jdofs, index, ison, fath, el_sons[MAX_NUM_ANC+1]; */
/*   double shap_fun_proj[MAX_EL_DOFS]; */

/* /\*++++++++++++++++ executable statements ++++++++++++++++*\/ */


/* /\* we will perform L2 projections of shape functions from coarse to fine *\/ */
/* /\* elements and use obtained coefficient matrices to perform actual *\/ */
/* /\* projections of dofs *\/ */

/*   field_id = pdv_renum[Solver_id].field_id; */
/*   mesh_id = apr_get_mesh_id(field_id); */
/*   base_q=apr_get_base_type(field_id); */

/*   p_renum_coarse = &pdv_renum[Solver_id].levels[Ilev_coarse]; */
/*   p_renum_fine =   &pdv_renum[Solver_id].levels[Ilev_fine]; */

/* /\* for each element active in a coarse mesh*\/ */
/*   for(ibl_coarse=1;ibl_coarse<=p_renum_coarse->nr_dof_ent;ibl_coarse++){ */

/*     iel_coarse = p_renum_coarse->l_dof_ent_id[ibl_coarse]; */

/*     if(ibl_coarse != p_renum_coarse->l_dof_bl_id[iel_coarse]){ */
/*       	  printf("pdr_L2proj: error in renumbering !!!\n"); */
/* 	  exit(-1); */
/*     } */

/* /\* position in global vectors of unknowns *\/ */
/*     posglob_coarse = L_posg_coarse[ibl_coarse]; */

/* /\* number of dofs associated with iel in the coarse mesh *\/ */
/*     ndof_coarse = L_nrdof_coarse[ibl_coarse]; */

/*     if(mmr_el_status(mesh_id,iel_coarse)==MMC_ACTIVE){ */
/*       pdeg_coarse = apr_get_ent_pdeg(field_id, APC_ELEMENT, iel_coarse); */
/*     } */
/*     else pdeg_coarse = apr_get_pdeg_coarse(field_id); */

/* #ifdef DEBUG */
/*     { */
/*       int ndof; */
/*       ndof = apr_get_pdeg_nrdofs(field_id,pdeg_coarse); */
/*       if(ndof!= ndof_coarse){ */
/* 	printf("wrong nrdof in pdr_proj_sol, iel %d, ibl %d, pdeg %d != %d\n", */
/* 	       iel_coarse, ibl_coarse, ndof,  ndof_coarse); */
/* 	exit(-1); */
/*       } */
/*     } */
/* #endif    */

/* /\* first check whether the elements do not belong to both meshes *\/ */
/*     if(p_renum_fine->l_dof_bl_id[iel_coarse]>0){ */
      
/*       ibl_fine = p_renum_fine->l_dof_bl_id[iel_coarse]; */

/* #ifdef DEBUG */
/* /\* check *\/ */
/*       if(p_renum_fine->l_dof_ent_id[ibl_fine]!=iel_coarse){ */
/* 	printf("Error in renumbering of elements in proj_sol!\n"); */
/* 	exit(-1); */
/*       } */
/*       if(L_nrdof_fine[ibl_fine]!=ndof_coarse){ */
/* 	printf("Error in number of dofs in proj_sol!\n"); */
/* 	exit(-1); */
/*       } */
/* #endif    */
      
/* /\* position in global vectors of unknowns *\/ */
/*       posglob_fine = L_posg_fine[ibl_fine]; */



/* /\* projecting from coarser to finer grid *\/ */
/*       if(Action==FEC_PROLONGATE) { */

/* /\*kbw */
/* #ifdef DEBUG */
/* 	printf("Projecting from iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_coarse, Ilev_coarse, ibl_coarse, posglob_coarse, */
/* 	       ndof_coarse); */
/* 	printf("Projecting to iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_fine, Ilev_fine, ibl_fine, posglob_fine, */
/* 	       ndof_fine); */
/* #endif    */
/* /\*kew*\/ */

/* 	for(i=0;i<ndof_coarse;i++) Vec_fine[posglob_fine+i] */
/* 				     = Vec_coarse[posglob_coarse+i]; */

/* /\*kbw */
/* #ifdef DEBUG */
/* 	for(i=0;i<ndof_coarse;i++) printf("%20.15lf",  */
/* 					  Vec_fine[posglob_fine+i]); */
/* #endif    */
/* /\*kew*\/ */

/*       }  */
/* /\* if projecting from  finer to coarser mesh *\/ */
/*       else if(Action==FEC_RESTRICT) { */
/* /\*kbw */
/* #ifdef DEBUG */
/* 	printf("Projecting from iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_fine, Ilev_fine, ibl_fine, posglob_fine, */
/* 	       ndof_fine); */
/* 	printf("Projecting to iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_coarse, Ilev_coarse, ibl_coarse, posglob_coarse, */
/* 	       ndof_coarse); */
/* #endif    */
/* /\*kew*\/ */
/* 	for(i=0;i<ndof_coarse;i++) Vec_coarse[posglob_coarse+i] */
/* 				     = Vec_fine[posglob_fine+i]; */
/* /\*kbw */
/* #ifdef DEBUG */
/* 	for(i=0;i<ndof_coarse;i++) printf("%20.15lf",  */
/* 					  Vec_fine[posglob_fine+i]); */
/* #endif    */
/* /\*kew*\/ */
/*       } */
/*       else{ */
/* 	printf("Unknown action specified in proj_sol!\n"); */
/* 	exit(-1); */
/*       } */

/*     } */
/*     else{ /\* element do not belong to both meshes *\/ */

/* /\* when rewriting from fine mesh - initialize coarse dofs *\/ */
/*       if(Action==FEC_RESTRICT) { */
/* 	for(i=0;i<ndof_coarse;i++) Vec_coarse[posglob_coarse+i] = 0.0; */
/*       } */

/*       /\* find list of descendents belonging to the next mesh *\/ */
/*       mmr_el_fam(mesh_id,iel_coarse,el_sons,NULL); */

/*       /\* loop over all sons *\/ */
/*       for(ison=0;ison<el_sons[0];ison++){ */
	
/* 	iel_fine=el_sons[ison+1]; */

/* 	ibl_fine = p_renum_fine->l_dof_bl_id[iel_fine]; */

/* 	/\* position in global vectors of unknowns *\/ */
/* 	posglob_fine = L_posg_fine[ibl_fine]; */

/* 	/\* number of dofs associated with iel in the coarse mesh *\/ */
/* 	ndof_fine = L_nrdof_fine[ibl_fine]; */

/* 	if(mmr_el_status(mesh_id,iel_fine)==MMC_ACTIVE){ */
/* 	  pdeg_fine = apr_get_ent_pdeg(field_id, APC_ELEMENT, iel_fine); */
/* 	} */
/* 	else pdeg_fine = apr_get_pdeg_coarse(field_id); */

/* /\*kbw */
/* 	printf("el_c %d (bl %d, posg %d, nrdof %d), p_c %d, \n", */
/* 	      iel_coarse, ibl_coarse, posglob_coarse, ndof_coarse, pdeg_coarse); */
/* 	printf("son %d, el_f %d (bl %d, posg %d, nrdof %d), p_f %d\n", */
/* 	       ison, iel_fine, ibl_fine, posglob_fine, ndof_fine, pdeg_fine);  */
/* /\*kew*\/ */

/* #ifdef DEBUG */
/* 	{ */
/* 	  int ndof; */
/* 	  ndof = apr_get_pdeg_nrdofs(field_id,pdeg_fine); */
/* 	  if(ndof!= ndof_fine){ */
/* 	    printf("Error in nrdof in pdr_proj_sol\n"); */
/* 	    exit(-1); */
/* 	  } */
/* 	} */
/* #endif    */

/* 	/\* check whether coefficient arrays have been percomputed *\/ */
/* 	index = -1; */
/* 	iaux = pdeg_coarse*10000+pdeg_fine*10+base_q; */
/* 	for(i=0;i<nr_precomp;i++){ */
/* 	  if(precomp_ind[i]==iaux) { */
/* 	    index = i; */
/* 	    break; */
/* 	  } */
/* 	} */

/* 	/\* coefficient matrix has not been computed yet *\/ */
/* 	if(index<0){ */

/* 	  int is, iel_temp, pdeg_temp; */
/* 	  double dofs_temp[MAX_EL_DOFS]; */

/* 	  precomp_ind[nr_precomp]=iaux; */
/* 	  index = nr_precomp; */
/* 	  nr_precomp++; */

/* 	  /\* loop over all sons *\/ */
/* 	  for(is=0;is<el_sons[0];is++){ */
	    
/* 	    iel_temp=el_sons[is+1]; */

/* 	    if(mmr_el_status(mesh_id,iel_temp)==MMC_ACTIVE){ */
/* 	      pdeg_temp = apr_get_ent_pdeg(field_id, APC_ELEMENT, iel_temp); */
/* 	    } */
/* 	    else pdeg_temp = apr_get_pdeg_coarse(field_id); */

/* /\* for all degrees of freedom of a coarse element *\/ */
/* 	    for(idofs=0;idofs<ndof_coarse;idofs++) { */

/* /\* coefficients will be computed for every coarse shape function *\/ */
/* 	      for(jdofs=0;jdofs<ndof_coarse;jdofs++) dofs_temp[jdofs]=0.0; */
/* 	      dofs_temp[idofs]=1.0; */

/* /\* L2 project: to fine element (iel_fine) with dofs in shap_fun_precomp *\/ */
/* /\*             from coarse element (iel_coarse) with dofs dofs_temp *\/ */
/* 	      i = -1; */
/* 	      apr_L2_proj(field_id,i, */
/* 			  iel_temp,pdeg_temp, */
/* 			  shap_fun_precomp[index][is][idofs], */
/* 			  &iel_coarse,&pdeg_coarse,dofs_temp, NULL); */
		
/* 	    } /\* end for all coarse shape functions: idofs *\/ */
/* 	  } /\* end for all sons: is *\/ */
/* 	} /\* end if coefficinets have not been precomputed yet *\/ */

/* 	/\* if projecting from coarser to finer mesh *\/ */
/* 	if(Action==FEC_PROLONGATE) { */

/* /\*kbw */
/* #ifdef DEBUG */
/* 	printf("Projecting from iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_coarse, Ilev_coarse, ibl_coarse, posglob_coarse, */
/* 	       ndof_coarse); */
/* 	printf("Projecting to iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_fine, Ilev_fine, ibl_fine, posglob_fine, */
/* 	       ndof_fine); */
/* 	printf("index %d, ison %d\n", index, ison); */
/* #endif    */
/* /\*kew*\/ */

/* 	  for(jdofs=0;jdofs<ndof_fine;jdofs++){ */
/* 	    Vec_fine[posglob_fine+jdofs] = 0.0; */
/* 	    for(idofs=0;idofs<ndof_coarse;idofs++) { */
	      
/* 	      Vec_fine[posglob_fine+jdofs] += Vec_coarse[posglob_coarse+idofs]   */
/* 		              * shap_fun_precomp[index][ison][idofs][jdofs];  */

/* /\*kbw */
/* #ifdef DEBUG */
/* 	      printf("i %d, j %d, v_f %15.12lf, v_c %15.12lf, coef %15.12lf\n", */
/* 		     idofs,jdofs, */
/* 		     Vec_fine[posglob_fine+jdofs], */
/* 		     Vec_coarse[posglob_coarse+idofs], */
/* 		     shap_fun_precomp[index][ison][idofs][jdofs]); */
/* #endif    */
/* /\*kew*\/ */

/* 	    } */
/* 	  } */
/* 	}  */
/* 	/\* if projecting from  finer to coarser mesh *\/ */
/* 	else if(Action==FEC_RESTRICT) { */

/* /\*kbw */
/* #ifdef DEBUG */
/* 	printf("Projecting from iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_fine, Ilev_fine, ibl_fine, posglob_fine, */
/* 	       ndof_fine); */
/* 	printf("Projecting to iel %d, level %d, ibl %d, posg %d, ndof %d\n", */
/* 	       iel_coarse, Ilev_coarse, ibl_coarse, posglob_coarse, */
/* 	       ndof_coarse); */
/* 	printf("index %d, ison %d\n", index, ison); */
/* #endif    */
/* /\*kew*\/ */

/* 	  for(idofs=0;idofs<ndof_coarse;idofs++) { */
/* 	    for(jdofs=0;jdofs<ndof_fine;jdofs++){ */
		
/* 	      Vec_coarse[posglob_coarse+idofs] += Vec_fine[posglob_fine+jdofs]  */
/* 		                 * shap_fun_precomp[index][ison][idofs][jdofs];  */

/* /\*kbw */
/* #ifdef DEBUG */
/* 	      printf("i %d, j %d, v_f %15.12lf, v_c %15.12lf, coef %15.12lf\n", */
/* 		     idofs,jdofs, */
/* 		     Vec_coarse[posglob_coarse+idofs], */
/* 		     Vec_fine[posglob_fine+jdofs], */
/* 		     shap_fun_precomp[index][ison][idofs][jdofs]); */
/* #endif    */
/* /\*kew*\/ */

/* 	    } */
/* 	  } */
/* /\*kbw */
/* getchar(); */
/* /\*kew*\/ */

/* 	} /\* end if projecting from finer to coarse *\/ */
	  
/*       } /\* end loop over sons: ison *\/ */

/*     } /\* end if element does not exist in both meshes *\/ */
    
/*   } /\* end loop over all elements of a coarse mesh *\/ */

/*   return(1); */
/* } */

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
