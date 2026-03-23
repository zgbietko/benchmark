/************************************************************************
File pds_test_ls_dg_intf.c - interface between the problem dependent
         module for testing using Laplace's equation, the discontinuous Galerkin
         approximation  and linear solver modules (direct and iterative)

Contains definitions of routines:

  pdr_get_list_int_ent - to return the list of integration entities (entities
                  for which stiffnes matrices and load vectors will be provided)
  pdr_init_levels - to provide information on mesh levels for the solver
  pdr_create_level - to return lists of mesh entities': types, IDs and
                     the associated numbers od dofs, for all entities
		     for which entries to the global
                     stiffness matrix and the load vector will
                     be provided for a given mesh level
  pdr_comp_stiff_mat - to provide a solver with a stiffness matrix
                      and a load vector corresponding to the specified
                      mesh entity, together with information on how to
                      assemble entries into the global stiffness matrix
                      and the global load vector
  pdr_read_sol_dofs - to read a vector of dofs associated with a given
                   mesh entity from approximation field data structure
  pdr_write_sol_dofs - to write a vector of dofs associated with a given
                   mesh entity to approximation field data structure
  pdr_proj_sol_lev - to project solution between mesh levels
  pdr_get_ent_pdeg - to return the degree of approximation index
                      associated with a given mesh entity
  pdr_dof_ent_sons - to return a list of dof entity sons

  pdr_vec_norm - to compute a norm of global vector in parallel
  pdr_sc_prod - to compute a scalar product of two global vectors
  pdr_create_exchange_tables - to create tables to exchange dofs
  pdr_exchange_dofs - to exchange dofs between processors
------------------------------
History:
	02.2002 - Krzysztof Banas, initial version
	10.2010 - Filip Krużel, elasticity
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

/* header file of the problem dependent module for Laplace's equation */
#include "./pdh_elast.h"

/* interface of the problem dependent module for fe solvers */
#include "pdh_intf.h"

/* interface of the mesh manipulation module */
#include "mmh_intf.h"

/* interface for approximation modules */
#include "aph_intf.h"

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
  int* Nrdof_glob,    /* out: global number of degrees of freedom (unknowns) */
  int* Max_dof_per_ent /* out: maximal number of dofs per dof entity */
  )
{

  int solver_id, field_id, mesh_id;
  int i, iaux, nel, nfa, int_ent, dof_ent, nr_elem, nr_face, nr_node, nno;
  int nrdofsgl, nrintent, nrdofent;
  int nreq;
  int el_id;


/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* associate the suitable field with the solver */
  /* here there is only one problem one field and one solver */
  solver_id = Problem_id;
  field_id = solver_id;
  mesh_id = apr_get_mesh_id(field_id);

  nreq = PDC_NREQ;

  /* prepare arrays */
  nr_elem = mmr_get_nr_elem(mesh_id);
  nr_face = mmr_get_nr_face(mesh_id);
  nr_node= mmr_get_nr_node(mesh_id);

  *List_int_ent_type = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );
  *List_int_ent_id = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );

  *List_dof_ent_type = (int *) malloc( (nr_node+1)*sizeof(int) );
  *List_dof_ent_id = (int *) malloc( (nr_node+1)*sizeof(int) );
  *List_dof_ent_nrdofs = (int *) malloc( (nr_node+1)*sizeof(int) );

  if((*List_dof_ent_nrdofs)==NULL){
   printf("Not enough space for allocating lists in pdr_get_list_ent! Exiting\n");
   exit(-1);
 }

  nrdofsgl = 0;

  int_ent=0; dof_ent=0;

  /* loop over elements - integration entities*/

  nel=0;
  while((nel=mmr_get_next_elem_all(mesh_id, nel))!=0){

    if(mmr_el_status(mesh_id,nel)==MMC_ACTIVE ) {

      int_ent++;
      (*List_int_ent_type)[int_ent-1]=PDC_ELEMENT;
      (*List_int_ent_id)[int_ent-1]=nel;

    } // end if element included for integration

  } // end for all elements

 /*loop over faces*/

  nfa=0;
  while((nfa=mmr_get_next_face_all(mesh_id, nfa))!=0)
    {
      if(mmr_fa_status(mesh_id,nfa)==MMC_ACTIVE)
	{
	  if(mmr_fa_bc(mesh_id, nfa)>0)
	    {
	      int_ent++;
	      (*List_int_ent_type)[int_ent-1]=PDC_FACE;
	      (*List_int_ent_id)[int_ent-1]=nfa;
	    }
	}
    }


/*loop over vertexes - dof entities*/

  nno=0;
    while((nno=mmr_get_next_node_all(mesh_id,nno))!=0)
      {
  /*kbw
      printf("In pdr_get_list_ent: nno %d, pdeg %d, dof_ent %d, nrdofsgl %d\n",
      nno,apr_get_ent_pdeg(field_id,APC_VERTEX,nno),dof_ent, nrdofsgl);
  /*kew*/
        if(apr_get_ent_pdeg(field_id,APC_VERTEX,nno)>0 )
  	{
  	  /* real node pdeg>0 */
  	  (*List_dof_ent_type)[dof_ent]=PDC_VERTEX;
  	  (*List_dof_ent_id)[dof_ent]=nno;
  	  (*List_dof_ent_nrdofs)[dof_ent]=PDC_NREQ;
  	  dof_ent++;
  	  nrdofsgl+=nreq;

  	}
        else if(apr_get_ent_pdeg(field_id,APC_VERTEX,nno)==0){
  	/* constrained node pdeg==0 */
        }
      }


  *Nr_int_ent = int_ent;
  *Nrdof_glob = nrdofsgl;
  *Max_dof_per_ent = nreq;
  *Nr_dof_ent = dof_ent;

/*kbw
    printf("In pdr_get_list_ent\n");
    printf("nrelem %d, nrface %d, nr_int_ent %d\n",
	   nr_elem, nr_face, *Nr_int_ent);
    for(i=0;i<int_ent;i++)  printf("type %d, id %d\n",
	 (*List_int_ent_type)[i],(*List_int_ent_id)[i]);
    printf("\nNr_dof_ent %d, Nrdof_glob %d, Max_dof_per_ent %d\n",
	   *Nr_dof_ent, *Nrdof_glob, *Max_dof_per_ent);
    for(i=0;i<dof_ent;i++)  printf("type %d, id %d, nrdof %d\n",
	(*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdof)[i]);
/*kew*/

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

  return(1);
}



/* two internal routines - one for elements, one for faces */
int pdr_comp_el_stiff_mat_elast(
                     /*returns: >=0 -success code, <0 -error code*/
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

int pdr_comp_fa_stiff_mat_elast(
		      /*returns: >=0 -success code, <0 -error code*/
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
  int *Pdeg_vec,        /* in: enforced degree of polynomial (if > 0 ) */
  int* Nr_dof_ent,   /* in: size of arrays, */
                          /* out: number of mesh entities with which dofs and */
                          /*      stiffness matrix blocks are associated */
  int* List_dof_ent_type, /* out: list of types for 'dof' entities */
  int* List_dof_ent_id,   /* out: list of ids for 'dof' entities */
  int* List_dof_ent_nrdof,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_loc,        /* in(optional): size of Stiff_mat and Rhs_vect */
                /* out(optional): actual number of dofs per integration entity*/
  double* Stiff_mat,      /* out(optional): stiffness matrix stored columnwise*/
  double* Rhs_vect,       /* outpds_elast_ls_std_intf.c(optional): rhs vector */
  char* Rewr_dofs         /* out(optional): flag to rewrite or sum up entries */
                          /*   'T' - true, rewrite entries when assembling */
                          /*   'F' - false, sum up entries when assembling */
  )
{

  int pdeg;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* element degree of approximation for linear prisms is a single number */
  if(Pdeg_vec==NULL) pdeg = 0;
  else pdeg = Pdeg_vec[0];

/*kbw
    printf("in pdr_comp_stiff_mat: problem_id %d, int_ent: type %d, id %d, enforced pdeg %d\n",
	   Problem_id, Int_ent_type, Int_ent_id, pdeg);
/*kew*/

  if( Int_ent_type == PDC_ELEMENT ){

    // Two choices: standard procedure that uses apr_num_int_el
/*     pdr_comp_el_stiff_mat(Problem_id, Int_ent_id, Comp_sm, Pdeg, */
/* 			  Nr_dof_ent, List_dof_ent_type, */
/* 			  List_dof_ent_id, List_dof_ent_nrdof, */
/* 			  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs); */

    //              or local problem routine:
    pdr_comp_el_stiff_mat_elast(Problem_id, Int_ent_id, Comp_sm, pdeg,
			  Nr_dof_ent, List_dof_ent_type,
			  List_dof_ent_id, List_dof_ent_nrdof,
			  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs);


  }
  else if(Int_ent_type==PDC_FACE){

    // Two choices: standard procedure that uses apr_num_int_el
/*     pdr_comp_fa_stiff_mat(Problem_id, Int_ent_id, Comp_sm, Pdeg, */
/* 			  Nr_dof_ent, List_dof_ent_type, */
/* 			  List_dof_ent_id, List_dof_ent_nrdof, */
/* 			  Nrdofs_loc, Stiff_mat, Rhs_vect, Rewr_dofs); */

    //              or local problem routine:
    pdr_comp_fa_stiff_mat_elast(Problem_id, Int_ent_id, Comp_sm, pdeg,
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
  int Pdeg_vec,        /* in: enforced degree of polynomial (if != NULL ) */
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
  int nreq;		/* number of equations = solution components */
  int el_mate;		/* element material */
  int sol_vec_id;       /* indicator for the solution dofs */
  int num_shap;         /* number of element shape functions */
  double sol_dofs[APC_MAXELSD]; /* solution dofs */

  int field_id, mesh_id;
  int kk, idofs, jdofs,num_dofs;
  int i,j,k;
  int max_dof_ent, max_nrdofs, glob_nr_dofs;
  int el_nodes[MMC_MAXELVNO+1];        // list of nodes of El
  double node_coor[3*MMC_MAXELVNO];  // coord of nodes of El



/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper mesh */
  field_id = Problem_id;
  mesh_id = apr_get_mesh_id(field_id);

/* get material number */
  el_mate =  mmr_el_groupID(mesh_id, El_id);

  /* set the number of element shape functions and scalar dofs */
  num_shap = 6;
  nreq = PDC_NREQ;
  num_dofs = num_shap*nreq;

  //printf("pdeg %d, num_shap=%d\n",pdeg,num_shap);

  max_dof_ent = *Nr_dof_ent;

  *Nr_dof_ent = num_shap;

  mmr_el_node_coor(mesh_id,El_id,el_nodes,node_coor);
  for(i=0;i<num_shap;i++)
    {
      List_dof_ent_type[i] = PDC_VERTEX;
      List_dof_ent_id[i] = el_nodes[i+1];
      List_dof_ent_nrdof[i] = nreq;
    }

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
    if(*Nrdofs_loc<num_dofs){
printf("Too small arrays Stiff_mat and Rhs_vect passed to comp_el_stiff_mat\n");
      printf("%d < %d. Exiting !!!", *Nrdofs_loc, num_dofs);
      exit(-1);
    }

#endif

    /* no dependence on the current solution in linear elasticity */
    for(i=0;i<num_dofs;i++) sol_dofs[i]=0;

    if(Comp_sm==PDC_COMP_SM||Comp_sm==PDC_COMP_BOTH){

      /* initialize the matrices to zero */
      for(i=0;i<num_dofs*num_dofs;i++) Stiff_mat[i]=0.0;

    }

    if(Comp_sm==PDC_COMP_RHS||Comp_sm==PDC_COMP_BOTH){
    
      /* initialize the vector to zero */
      for(i=0;i<num_dofs;i++) Rhs_vect[i]=0.0; 

    }

    int diagonal[5]={0,0,0,0,0}; // diagonality of: M, A_ij, B_j, T_i and C  
    // coefficient matrices returned to apr_num_int_el by pdr_el_coeff
    /* perform numerical integration of terms from the weak formualation */
    /* pdeg - taken from approximation data structures (&pdeg==NULL) */
    //FK wylaczam
    //apr_num_int_el(Problem_id,field_id,El_id, Comp_sm, NULL, sol_dofs, sol_dofs,
    //	   diagonal, Stiff_mat, Rhs_vect);



    /*kbw
        if(El_id>0){
          printf("Element %d: Stiffness matrix:\n",El_id);
          for (idofs=0;idofs<num_dofs*PDC_NREQ;idofs++) {
    	for (jdofs=0;jdofs<num_dofs*PDC_NREQ;jdofs++) {
    	  printf("%7.3lf",Stiff_mat[idofs+jdofs*num_dofs*PDC_NREQ]);
    	}
    	printf("\n");
          }
          printf("Element %d: Rhs_vect:\n",El_id);
          for (idofs=0;idofs<num_dofs*PDC_NREQ;idofs++) {
    	printf("%7.3lf",Rhs_vect[idofs]);
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
  int* Pdeg_vec,     /* in: enforced degree of polynomial (if != NULL ) */
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
//
///* local variables */
//
//    int fa_type;		/* type of face: quad or triangle */
//    int base_q;		/* type of basis functions */
//    int nreq;		/* number of equations */
//    int num_shap=0;/* local dimension for neigbhor 1 */
//    int el_mate=0;	/* material number for neigbhor 1 */
//    int face_neig[2];	/* list of neighbors */
//    int neig_sides[2]={0,0};/* sides of face wrt neigs */
//    int node_shift;
//    double acoeff[4], bcoeff[2];/* to transform coordinates between faces */
//    double hf_size;        /* size of face */
//    int i, kk;
//    int idofs, jdofs,neig_id, num_dofs;
//    double daux;
//
//  double *a_int=NULL;
//  double *s_int=NULL;
//
//  int field_id, mesh_id;
//  int sol_vec_id;       /* indicator for the solution dofs */
//
//  int el_nodes[MMC_MAXELVNO+1];        // list of nodes of El
//  double node_coor[3*MMC_MAXELVNO];  // coord of nodes of El
//
//
///*++++++++++++++++ executable statements ++++++++++++++++*/
//
//  /* select the proper mesh */
//  field_id = Problem_id;
//  mesh_id = apr_get_mesh_id(field_id);
//
//  /* get type of face and bc flag */
//  fa_type=mmr_fa_type(mesh_id,Fa_id);
//  mmr_fa_area(mesh_id,Fa_id,&daux,NULL);
//  hf_size = sqrt(daux);
//
//  /* get approximation parameters */
//  base_q=apr_get_base_type(field_id);
//  nreq=PDC_NREQ;
//
///* get neighbors list with corresponding neighbors' sides numbers*/
//  mmr_fa_neig(mesh_id,Fa_id,face_neig,neig_sides,&node_shift,NULL,acoeff,bcoeff);
//
//  /* get material number */
//  neig_id = abs(face_neig[0]);
//  el_mate =  mmr_el_groupID(mesh_id, neig_id);
//
//  apr_get_el_pdeg(field_id, neig_id, &i);
//  num_shap = apr_get_el_pdeg_numshap(field_id, &i); ;
//  num_dofs = num_shap*nreq;
//
//  /* trick: for a face we return the whole element stiffness matrix, hence
//     the number of associated dof entities is 6 (num_shap) not 4 */
//  *Nr_dof_ent = num_shap;
//
//  mmr_el_node_coor(mesh_id,neig_id,el_nodes,node_coor);
//  for(i=0;i<num_shap;i++)
//    {
//      List_dof_ent_type[i] = PDC_VERTEX;
//      List_dof_ent_id[i] = el_nodes[i+1];
//      List_dof_ent_nrdof[i] = nreq;
//    }
//
//  if(Comp_sm!=PDC_NO_COMP){
//
//    if(a_int!=NULL)
//      {
//	free(a_int);
//      }
//    a_int = (double *) malloc(num_dofs*num_dofs*sizeof(double));
//
//    if(s_int!=NULL)
//      {
//	free(s_int);
//      }
//    s_int = (double *) malloc(num_dofs*sizeof(double));
//
//    /* perform numerical integration of terms from the weak formualation */
//    /* pdeg - taken from approximation data structures (&pdeg==NULL) */
//    apr_num_int_fa(field_id,Fa_id,Comp_sm,NULL,&a_int,&s_int);
//
//
//    if(Comp_sm==PDC_COMP_SM||Comp_sm==PDC_COMP_BOTH){
//
//      /* initialize the matrices to zero */
//      for(i=0;i<num_dofs*num_dofs;i++) Stiff_mat[i]=0.0;
//
//      kk=0;
//      for (jdofs=0;jdofs<num_dofs;jdofs++) {
// 	for (idofs=0;idofs<num_dofs;idofs++) {
//
// 	  /* simple integral for elasticity equations */
// 	  Stiff_mat[kk+idofs] += a_int[kk+idofs];
//
// 	}/* idofs */
// 	kk+=num_dofs;
//       } /* jdofs */
//
//     }
//
//     if(Comp_sm==PDC_COMP_RHS||Comp_sm==PDC_COMP_BOTH){
//
//       /* initialize the vector to zero */
//       for(i=0;i<num_dofs;i++) Rhs_vect[i]=0.0;
//
//       kk=0;
//       for (idofs=0;idofs<num_dofs;idofs++) {
//
// 	/* right hand side vector - for body forces */
// 	Rhs_vect[kk] += s_int[kk];
//
// 	kk++;
//       }/* idofs */
//
//     }
//
//
//     if(Rewr_dofs != NULL) *Rewr_dofs = 'F';
//
//     /*kbw
//
//     printf("\n");
//
//     if(Fa_id>0){
//       printf("Face %d: Stiffness matrix:\n",Fa_id);
//       for (idofs=0;idofs<num_dofs;idofs++) {
// 	for (jdofs=0;jdofs<num_dofs;jdofs++) {
// 	  printf("%7.3lf",Stiff_mat[idofs+jdofs*num_dofs*PDC_NREQ]);
// 	}
// 	printf("\n");
//       }
//       printf("Face %d: Rhs_vect:\n",Fa_id);
//       for (idofs=0;idofs<num_dofs;idofs++) {
// 	printf("%7.3lf",Rhs_vect[idofs]);
//       }
//       printf("\n");
//       getchar();
//     }
// /*kew*/
//
//
//
//
//   } /* end if computing SM and/or RHSV */
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
  int *Pdeg,	/* in: element degree of approximation */
  double* Dofs,	/* out: workspace for degress of freedom of El */
  /* 	NULL - write to  data structure */
  int* El_from,	/* in: list of elements to provide function */
  int* Pdeg_from,	/* in: degree of polynomial for each El_from */
  double* Dofs_from /* in: Dofs of El_from or...*/
  )
{

  int field_id = Problem_id;
  int i= -1; /* mode: -1 - projection from father to son */

  apr_L2_proj(field_id, i, El, Pdeg, Dofs, El_from, Pdeg_from, Dofs_from, NULL);

  return 1;
}


/*---------------------------------------------------------
pdr_renum_coeff - to return a coefficient being a basis for renumbering
----------------------------------------------------------*/
int pdr_renum_coeff(
  int Problem_id,	/* in: problem ID */
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
  else{
    Ent_sons[0]=0;
  }

  return(0);
}

/*------------------------------------------------------------
  pdr_comp_el_stiff_mat_elast - to construct a stiffness matrix and
                          a load vector for an element
------------------------------------------------------------*/
int pdr_comp_el_stiff_mat_elast(/*returns: >=0 -success code, <0 -error code*/
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
  double sol_dofs[APC_MAXELSD]; /* solution dofs */


  int field_id, mesh_id, nreq, num_dofs;
  int kk, idofs, jdofs;
  int i,j,k,ki,iaux,nai,naj;
  int max_dof_ent, max_nrdofs, glob_nr_dofs;
  int el_nodes[MMC_MAXELVNO+1];        // list of nodes of El
  double node_coor[3*MMC_MAXELVNO];  // coord of nodes of El

/*   static int pdeg_old=0; /\* indicator for recomputing quadrature data *\/ */
/*   static int base_q;		/\* type of basis functions for quadrilaterals *\/ */
/*   static int ngauss;            /\* number of gauss points *\/ */
/*   static double xg[3000];   	 /\* coordinates of gauss points in 3D *\/ */
/*   static double wg[1000];       /\* gauss weights *\/ */

/* #pragma omp threadprivate (pdeg_old) */
/* #pragma omp threadprivate (base_q) */
/* #pragma omp threadprivate (ngauss) */
/* #pragma omp threadprivate (xg) */
/* #pragma omp threadprivate (wg) */

  // to make OpenMP working
  int pdeg_old=-1; /* indicator for recomputing quadrature data */
  int base_q;		/* type of basis functions for quadrilaterals */
  int ngauss;            /* number of gauss points */
  double xg[3000];   	 /* coordinates of gauss points in 3D */
  double wg[1000];       /* gauss weights */

  double determ;        /* determinant of jacobi matrix */
  double hsize;         /* size of an element */
  double vol;           /* volume for integration rule */
  double xcoor[3];      /* global coord of gauss point */
  double u_val[PDC_NREQ]; /* computed solution */
  double u_x[PDC_NREQ];   /* gradient of computed solution */
  double u_y[PDC_NREQ];   /* gradient of computed solution */
  double u_z[PDC_NREQ];   /* gradient of computed solution */
  double base_phi[APC_MAXELVD];    /* basis functions */
  double base_dphix[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];  /* y-derivatives of basis function */
  double Eyoung,Poisini;

  double sv, g, gprl, rl;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper mesh */
  field_id = Problem_id;
  mesh_id = apr_get_mesh_id(field_id);

/* get material number */
  el_mate =  mmr_el_groupID(mesh_id, El_id);

  /* find degree of polynomial and number of element scalar dofs */
  /* Pdeg_in is now a number - we are in linear approximation!!! */
  if(Pdeg_in>0) pdeg = Pdeg_in;
  else{
    apr_get_el_pdeg(field_id, El_id, &pdeg);
  }
  num_shap = apr_get_el_pdeg_numshap(field_id,El_id,&pdeg);

/* check parameter num_shap */
#ifdef DEBUG
  if(num_shap!=6){
    printf("Wrong num_shap from apr_shape_fun_3D in linear approximation%d\n",
		num_shap);
    exit(-1);
  }
#endif

  /* set the number of element shape functions and scalar dofs */
  nreq = PDC_NREQ;
  num_dofs = num_shap*nreq;

  //printf("pdeg %d, num_shap=%d\n",pdeg,num_shap);

  max_dof_ent = *Nr_dof_ent;

/*kbw
    printf("In pdr_com_el_stiff_mat: field_id %d, El_id %d, Comp_sm %d, Nr_dof_ent %d\n",
	   field_id, El_id, Comp_sm, *Nr_dof_ent);
    getchar();getchar();
/*kew*/

  if(Comp_sm!=PDC_NO_COMP){


#ifdef DEBUG
    if(Nrdofs_loc == NULL || Stiff_mat == NULL || Rhs_vect == NULL){
printf("NULL arrays Stiff_mat and Rhs_vect in pdr_comp_stiff_el_mat. Exiting!");
      exit(-1);
    }
    if(*Nrdofs_loc<num_dofs){
printf("Too small arrays Stiff_mat and Rhs_vect passed to comp_el_stiff_mat\n");
      printf("%d < %d. Exiting !!!", *Nrdofs_loc, num_dofs);
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


 /* prepare data for gaussian integration */
  if(pdeg!=pdeg_old){
    base_q=apr_get_base_type(field_id,El_id);
    apr_set_quadr_3D(base_q, &pdeg, &ngauss, xg, wg);
    pdeg_old = pdeg;
  }

/*kbw
    printf("In num_int_el: field_id %d, mesh_id %d,  element %d\n",
	   field_id, mesh_id, El_id);
    printf("pdeg %d, ngauss %d\n",pdeg, ngauss);
    printf("NREQ %d, ndof %d, local_dim %d\n",nreq, num_dofs, num_dofs);
    printf("%d nodes with coordinates:\n",el_nodes[0]);
    for(i=0;i<el_nodes[0];i++){
      printf("node %d (global - %d): x - %f, y - %f, y - %f\n",
	     i, el_nodes[i+1],
	     node_coor[3*i],node_coor[3*i+1],node_coor[3*i+2]);
    }
    printf("DOFS:\n");
    for(i=0;i<num_dofs;i++) printf("%20.15lf",sol_dofs[i]);
    printf("\n");
    getchar();
/*kew*/

  	for(k=0;k<matnum;k++)
  	{
  		if(pdv_material[k].id==el_mate)
  		{
  			Eyoung=pdv_material[k].Eyoung;
  			Poisini=pdv_material[k].Poisni;
  			//printf("Eyoung=%.2lf, Poisni=%.2lf\n",Eyoung,Poisni);
  		}
  	}

  //printf("Eyoung=%lf,Poisni=%lf\n",Eyoung,Poisini);

  g=Eyoung/(2*(1+Poisini));
  rl=Poisini*Eyoung/((1-2*Poisini)*(1+Poisini));
  gprl=g+rl;

  mmr_el_node_coor(mesh_id,El_id,el_nodes,node_coor);

  /* initialize the matrices to zero */
  for(i=0;i<num_dofs*num_dofs;i++) Stiff_mat[i]=0.0;

  for (ki=0;ki<ngauss;ki++) {

    /* at the gauss point, compute basis functions, determinant etc*/
    iaux = 2; /* calculations with jacobian but not on the boundary */
    determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base_q,
			       &xg[3*ki],node_coor,sol_dofs,
			       base_phi,base_dphix,base_dphiy,base_dphiz,
			       xcoor,u_val,u_x,u_y,u_z,NULL);

    vol = determ * wg[ki];


    /*kbw
      printf("at gauss point %d, local coor %lf, %lf, %lf\n",
	     ki,xg[3*ki],xg[3*ki+1],xg[3*ki+2]);
      printf("global coor %lf %lf %lf\n",xcoor[0],xcoor[1],xcoor[2]);
      printf("weight %lf, determ %lf, coeff %lf\n",
	     wg[ki],determ,vol);
      printf("%d shape functions and derivatives: \n", num_shap);
      for(i=0;i<num_shap;i++){
	printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
	       base_phi[i],base_dphix[i],base_dphiy[i],base_dphiz[i]);
      }
      printf("solution and derivatives: \n");
      printf("un - %lf, der: x - %lf, y - %lf, z - %lf\n",
	     *u_val,*u_x,*u_y,*u_z);
      getchar();
   /*kew*/


   if(Comp_sm==PDC_COMP_SM||Comp_sm==PDC_COMP_BOTH){


     sv=0;
     kk=0;
     naj=0;
     for (jdofs=0;jdofs<num_shap;jdofs++) {
       nai=0;
       for (idofs=0;idofs<num_shap;idofs++) {
	 sv=g*(base_dphix[jdofs]*base_dphix[idofs]+
	       base_dphiy[jdofs]*base_dphiy[idofs]);
	 sv+=g*base_dphiz[jdofs]*base_dphiz[idofs];

	 Stiff_mat[(jdofs*kk)+nai]+=
	   (gprl*base_dphix[jdofs]*base_dphix[idofs] + sv)*vol;
	 Stiff_mat[jdofs*kk+nai+1]+=
	   (rl*base_dphix[jdofs] *base_dphiy[idofs] +
	    g*base_dphiy[jdofs]*base_dphix[idofs])*vol;
	 Stiff_mat[jdofs*kk+nai+2]+=
	   (rl*base_dphix[jdofs] *base_dphiz[idofs] +
	    g*base_dphiz[jdofs]*base_dphix[idofs])*vol;

	 Stiff_mat[(jdofs)*kk+nai+num_dofs]+=
	   (rl*base_dphiy[jdofs]*base_dphix[idofs]+
	    g*base_dphix[jdofs]*base_dphiy[idofs])*vol;
	 Stiff_mat[(jdofs)*kk+nai+1+num_dofs]+=
	   (gprl*base_dphiy[jdofs]*base_dphiy[idofs]+sv)*vol;
	 Stiff_mat[(jdofs)*kk+nai+2+num_dofs]+=
	   (rl*base_dphiy[jdofs] *base_dphiz[idofs] +
	    g*base_dphiz[jdofs]*base_dphiy[idofs])*vol;

	 Stiff_mat[(jdofs)*kk+nai+num_dofs*2]+=
	   (rl*base_dphiz[jdofs] *base_dphix[idofs] +
	    g*base_dphix[jdofs]*base_dphiz[idofs])*vol;
	 Stiff_mat[(jdofs)*kk+nai+1+num_dofs*2]+=
	   (rl*base_dphiz[jdofs] *base_dphiy[idofs] +
	    g*base_dphiy[jdofs]*base_dphiz[idofs])*vol;
	 Stiff_mat[(jdofs)*kk+nai+2+num_dofs*2]+=
	   (gprl*base_dphiz[jdofs] *base_dphiz[idofs]+sv)*vol;

	 nai+=nreq;
       }/* idofs */
       naj+=nreq;
       kk=nreq*num_dofs;
     } /* jdofs */

   } /* end if computing SM */

   if(Comp_sm==PDC_COMP_RHS||Comp_sm==PDC_COMP_BOTH){

      /* initialize the matrices to zero */
      for(i=0;i<num_dofs;i++) Rhs_vect[i]=0.0;

/*      kk=0; */
/*      for (idofs=0;idofs<num_shap;idofs++) */
/*        { */

/* 	 Rhs_vect[idofs*kk]   =1 * base_phi[idofs] * vol; */
/* 	 Rhs_vect[idofs*kk+1] =1 * base_phi[idofs] * vol; */
/* 	 Rhs_vect[idofs*kk+2] =1 * base_phi[idofs] * vol; */

/*        }/\* idofs *\/ */

   } /* end if computing RHSV */

  } /* end loop over integration points: ki */


  //printf("\n\nTime=%lf\n",time);

	/*kbw
    FILE *plik;
    plik=fopen("outputAKT.txt","a");
    kk=0;
    fprintf(plik,"\nMatrix for element %d:\n",El_id);
    for (jdofs=0;jdofs<num_dofs;jdofs++)
    {
    	for (idofs=0;idofs<num_dofs;idofs++)
    	{
    		fprintf(plik," %.15lf",Stiff_mat[kk+idofs]);
    	}
		kk+=num_dofs;
		fprintf(plik,"\n");
    }

    fprintf(plik,"\nRhs_vect for element:\n");
    for (idofs=0;idofs<num_dofs;idofs++)
    {
    	fprintf(plik," %.15lf",Rhs_vect[idofs]);
	}
	fprintf(plik,"\n\n");
	fclose(plik);
	/*kew*/

    if(Rewr_dofs != NULL) *Rewr_dofs = 'F';


  } /* end if computing SM and/or RHSV */

  /* change the option compute SM and RHSV to rewrite SM and RHSV */
  if(Comp_sm!=PDC_NO_COMP) Comp_sm += 3;

  /* obligatory procedure to fill Lists of dof_ents and rewite SM and RHSV */
  /* the reason is to take into account POSSIBLE CONSTRAINTS (HANGING NODES) */
  apr_get_stiff_mat_data(field_id, El_id, Comp_sm, 'N', Pdeg_in, NULL, Nr_dof_ent,
  	         List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
  			   Nrdofs_loc, Stiff_mat, Rhs_vect);

    /* matrix displayed by rows, altghough stored by columns !!!!!!!!!*/
    /*kbw
    FILE *plik;
    plik=fopen("outputAKT.txt","a");
    if(Comp_sm!=PDC_NO_COMP && El_id>0){
      fprintf(plik,"\nElement %d: Modified stiffness matrix:\n",El_id);
      for (idofs=0;idofs<*Nrdofs_loc;idofs++) { //for each row!!!!
        for (jdofs=0;jdofs<*Nrdofs_loc;jdofs++) { // for each element in row !!!
          fprintf(plik,"%.15lf",Stiff_mat[idofs+jdofs*(*Nrdofs_loc)]);
        }
        fprintf(plik,"\n");
      }
      fprintf(plik,"Element %d: Rhs_vect:\n",El_id);
      for (idofs=0;idofs<*Nrdofs_loc;idofs++) {
        fprintf(plik,"%.15lf",Rhs_vect[idofs]);
      }
      fprintf(plik,"\n");
      //getchar();
     }
    fclose(plik);
    /**/

  return(1);
}



/*------------------------------------------------------------
  pdr_comp_fa_stiff_mat_elast - to construct a stiffness matrix and
                          a load vector for a face
------------------------------------------------------------*/
int pdr_comp_fa_stiff_mat_elast(/*returns: >=0 -success code, <0 -error code*/
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

    int fa_type;		/* type of face: quad or triangle */
    int fa_bc;	        /* bc flag for a face */
    int base_q;		/* type of basis functions */
    int nreq;		/* number of equations */
    int pdeg=0, num_shap=0;/* local dimension for neigbhor 1 */
    int el_mate=0;	/* material number for neigbhor 1 */
    int face_neig[2];	/* list of neighbors */
    int num_fa_nodes;
    int fa_nodes[4];
    double fa_node_coor[3*MMC_MAXELVNO];  // coord of nodes of Fa
    int neig_sides[2]={0,0};/* sides of face wrt neigs */
    double xg_el[6];	/* local coord of gauss points for neighbors */
    int node_shift;
    double acoeff[4], bcoeff[2];/* to transform coordinates between faces */
    double hf_size;        /* size of face */
    int i, ieq, kk;
    int idofs, jdofs, ndofs1,neig_id, iaux, num_dofs,ki,bc_type,nai,j;
    double daux, vec_norm[3], area, penalty,penmod;

  int field_id, mesh_id;
  int sol_vec_id;       /* indicator for the solution dofs */

  int el_nodes[MMC_MAXELVNO+1];        // list of nodes of El
  double node_coor[3*MMC_MAXELVNO];  // coord of nodes of El

  int ngauss;            /* number of gauss points */
  double xg[3000];   	 /* coordinates of gauss points in 3D */
  double wg[1000];       /* gauss weights */


  double determ;        /* determinant of jacobi matrix */
  double hsize;         /* size of an element */
  double vol;           /* volume for integration rule */
  double xcoor[3];      /* global coord of gauss point */
  double u_val_hat[PDC_NREQ]; /* specified solution for Dirichlet BC */
  double u_val[PDC_NREQ]; /* computed solution */
  double u_x[PDC_NREQ];   /* gradient of computed solution */
  double u_y[PDC_NREQ];   /* gradient of computed solution */
  double u_z[PDC_NREQ];   /* gradient of computed solution */
  double base_phi[APC_MAXELVD];    /* basis functions */
  double base_dphix[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];  /* y-derivatives of basis function */
  double sol_dofs[APC_MAXELSD]; /* solution dofs */
  double wsp[3]={1,1,1};

  double sv, g, gprl, rl;

  /* coefficients from PDE */
  double gval[PDC_NREQ];   /* rhs  Neumann data  */


/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper mesh */
  field_id = Problem_id;
  mesh_id = apr_get_mesh_id(field_id);

  /* get type of face and bc flag */
  fa_type=mmr_fa_type(mesh_id,Fa_id);
  mmr_fa_area(mesh_id,Fa_id,&daux,NULL);
  hf_size = sqrt(daux);

  fa_bc=mmr_fa_bc(mesh_id,Fa_id);
  bc_type = pdr_get_bc_type(fa_bc);

 // printf("fa_bc=%d, bc_type=%d\n",fa_bc,bc_type);

  /* get approximation parameters */
  nreq=PDC_NREQ;

  /* get neighbors list with corresponding neighbors' sides numbers*/
  mmr_fa_neig(mesh_id,Fa_id,face_neig,neig_sides,&node_shift,NULL,acoeff,bcoeff);

  //printf("For face %d face_neig=%d %d\t neig_sides=%d %d\n",Fa_id,face_neig[0],face_neig[1],neig_sides[0],neig_sides[1]);

  /* get material number */
  neig_id = abs(face_neig[0]);
  el_mate =  mmr_el_groupID(mesh_id, neig_id);

  apr_get_el_pdeg(field_id, neig_id, &pdeg);
  num_shap = apr_get_el_pdeg_numshap(field_id,neig_id,&pdeg);
  num_dofs = num_shap*nreq;

  base_q=apr_get_base_type(field_id,neig_id);

  mmr_el_node_coor(mesh_id,neig_id,el_nodes,node_coor);

  /*kbw
    printf("In pdr_comp_fa_stiff_mat: field_id %d, mesh_id %d, Comp_sm %d\n",
	   field_id, mesh_id, Comp_sm);
    printf("Fa_id %d, Fa_type %d, size %lf, Fa_bc %d, bc_type %d\n",
	   Fa_id, fa_type, hf_size, fa_bc, bc_type);
    printf("elem %d, el_side %d, el_mate %d, num_shap %d, num_dofs %d\n",
	   neig_id, neig_sides[0], el_mate, num_shap, num_dofs);
    printf("For each block: \ttype, \tid, \tnrdof\n");
    for(i=0;i<*Nr_dof_ent;i++){
      printf("\t\t\t%d\t%d\t%d\n",
	     List_dof_ent_type[i],List_dof_ent_id[i],List_dof_ent_nrdof[i]);
    }
  /*kew*/

  if(Comp_sm!=PDC_NO_COMP){

    if(Comp_sm==PDC_COMP_SM||Comp_sm==PDC_COMP_BOTH){

      /* initialize the matrices to zero */
      for(i=0;i<num_dofs*num_dofs;i++) Stiff_mat[i]=0.0;
      /* initialize the vector to zero */
      for(i=0;i<num_dofs;i++) Rhs_vect[i]=0.0;

      if(bc_type==PDC_BC_DIRI){

    	  for(i=0;i<bcnum;i++)
    	  {
    		  if(fa_bc==pdv_bc[i].type)
    		  {
    			  u_val_hat[0]=pdv_bc[i].vec[0];
    			  u_val_hat[1]=pdv_bc[i].vec[1];
    			  u_val_hat[2]=pdv_bc[i].vec[2];
    			  break;
    		  }
    	  }

    penalty=1.0e9;
    penmod=0;
	for(i=0;i<matnum;i++)
	{
		penmod+=pdv_material[i].Eyoung;
	}
	penalty=penalty*penmod;

	if(fa_type==MMC_TRIA) num_fa_nodes=3;
	else if(fa_type==MMC_QUAD) num_fa_nodes=4;

	if(neig_sides[0]==0){
	  fa_nodes[0]=0; fa_nodes[1]=1; fa_nodes[2]=2;
	}
	else if(neig_sides[0]==1){
	  fa_nodes[0]=3; fa_nodes[1]=4; fa_nodes[2]=5;
	}
	else if(neig_sides[0]==2){
	  fa_nodes[0]=0; fa_nodes[1]=1; fa_nodes[2]=4; fa_nodes[3]=3;
	}
	else if(neig_sides[0]==3){
	  fa_nodes[0]=1; fa_nodes[1]=2; fa_nodes[2]=5; fa_nodes[3]=4;
	}
	else if(neig_sides[0]==4){
	  fa_nodes[0]=2; fa_nodes[1]=0; fa_nodes[2]=3; fa_nodes[3]=5;
	}

	//kk=(num_dofs*ieq+ieq)*num_shap;

	for (idofs=0;idofs<num_fa_nodes;idofs++) {

	  kk = (num_dofs*fa_nodes[idofs]+fa_nodes[idofs])*nreq;

	  for(ieq=0;ieq<PDC_NREQ;ieq++){

	    j = ieq*num_dofs + ieq;
	    Stiff_mat[kk+j] += penalty;
	    Rhs_vect[fa_nodes[idofs]*nreq + ieq] += u_val_hat[ieq]*penalty;
/*kbw
	    printf("SM(%d,%d)=%d==%d: %lf; LV(%d): %lf\n",
		   fa_nodes[idofs]*nreq+ieq, fa_nodes[idofs]*nreq+ieq,
		   (fa_nodes[idofs]*nreq+ieq)*num_dofs+fa_nodes[idofs]*nreq+ieq,
		   kk+j, Stiff_mat[kk+j],
		   fa_nodes[idofs]*nreq+ieq, Rhs_vect[fa_nodes[idofs]*nreq+ieq]
		   );
/*kew*/

	  }
	}
      }
      else{

	/* prepare data for gaussian integration */
	apr_set_quadr_2D(fa_type,base_q, &pdeg,&ngauss,xg,wg);

	//printf("\nFace %d ma %d bokow\n",Fa_id,fa_type);

	/* loop over integration points */
	for (ki=0;ki<ngauss;ki++) {

	  /* find coordinates within neighboring elements for a point on face */
	  mmr_fa_elem_coor(mesh_id,&xg[2*ki],face_neig,neig_sides,node_shift, acoeff,bcoeff,xg_el);

	  /* at the gauss point for neig , compute basis functions */
	  iaux = 3+neig_sides[0]; /* boundary data for face neig_sides[0] */

	  //printf("For fa %d neig_sides[0]=%d iaux=%d\n",Fa_id,neig_sides[0],iaux);

	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base_q,
				    xg_el,node_coor,sol_dofs,
				    base_phi,base_dphix,base_dphiy,base_dphiz,
				    xcoor,u_val,u_x,u_y,u_z,vec_norm);

	  /* coefficient for 2D numerical integration */
	  area = determ*wg[ki];

/*kbw
	  if(Fa_id>0){
	    printf("at integration point %d, coor: local %lf, %lf, weight %lf\n"
		   ,ki,xg[2*ki],xg[2*ki+1],wg[ki]);
	    printf("coor global %lf, %lf, %lf, determ %lf, area %lf\n",
		   xcoor[0],xcoor[1],xcoor[2],determ,area);
	    printf("normal %lf, %lf, %lf\n",
		   vec_norm[0], vec_norm[1], vec_norm[2]);
	    printf("coord for neigh: %lf, %lf, %lf, hsize %lf\n",
		   xg_el[0],xg_el[1],xg_el[2],hsize);
	    printf("weight %lf, integration coeff. %lf\n", wg[ki], area);
	    printf("%d shape functions and derivatives for neigh: \n",num_shap);
	    for(i=0;i<num_shap;i++){
	      printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
		     base_phi[i],base_dphix[i],base_dphiy[i],base_dphiz[i]);
	    }
	    printf("solution %lf, der %lf, %lf, %lf\n",
		   u_val[0],u_x[0],u_y[0],u_z[0]);
	    printf("penalty %lf, hf_size %lf\n",
		   penalty,hf_size);

	    getchar();
	  }
/*kew*/


//	if(bc_type==APC_BC_DIRI){

	  /* we need Gauss-Lobatto integration or
	     direct substitution to Stiffness matrix and Rhs vector */
/* 	  printf("Dirichlet BC not implemented yet!!!\n"); */

/* 	  penalty=1; */
/* 	  for (jdofs=0;jdofs<num_shap;jdofs++) */
/* 	    { */
/* 	      for(nai=0;nai<nreq;nai++) */
/* 		{ */
/* 		  j=(jdofs*nreq)+nai; */
/* 		  Stiff_mat[j*num_dofs+j]+= */
/* 		    penalty*base_phi[jdofs]*base_phi[jdofs]*area;  */
/* 		} */
/* 	    } */

/* 	  int uval[3]; */
/* 	  uval[0]=0; */
/* 	  uval[1]=1; */
/* 	  uval[2]=0; */

/* 	  kk=0; */
/*           for (idofs=0;idofs<num_shap;idofs++) { */

/* 	    /\* right hand side vector *\/ */

/* 	    Rhs_vect[kk] += penalty * uval[0] * base_phi[idofs] * area; */
/* 	    Rhs_vect[kk+1] += penalty * uval[1] * base_phi[idofs] * area; */
/* 	    Rhs_vect[kk+2] += penalty * uval[2] * base_phi[idofs] * area; */
/* 	    kk+=nreq; */


/* 	  }/\* idofs *\/ */

//	}
//	else

	  if(bc_type==PDC_BC_NEUM){

    	  /*get bc data from structure*/
		  //printf("\nwew sciana %d z neumannem \n",Fa_id);

    	  for(i=0;i<bcnum;i++)
    	  {
    		  if(fa_bc==pdv_bc[i].type)
    		  {
    			  gval[0]=pdv_bc[i].vec[0];
    			  gval[1]=pdv_bc[i].vec[1];
    			  gval[2]=pdv_bc[i].vec[2];
    			  break;
    		  }
    	  }

	    if(Comp_sm==PDC_COMP_RHS||Comp_sm==PDC_COMP_BOTH){

	      kk=0;
	      for (idofs=0;idofs<num_shap;idofs++) {

		/* right hand side vector */
		Rhs_vect[kk] += gval[0]*base_phi[idofs]*area;
		Rhs_vect[kk+1] += gval[1]*base_phi[idofs]*area;
		Rhs_vect[kk+2] += gval[2]*base_phi[idofs]*area;
		kk+=nreq;

	      }/* idofs */

	    }/* if computing RHS */

	  } /* if Neumann BC */
	  else if(bc_type==PDC_BC_MIXED){

	    printf("MIXED BC not implemented yet!!!\n");


	  }/* if mixed type BC */

	} /* ki */

      } /* if not BC_DIRI */

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



/*kbw

      printf("\n");

     if(Fa_id==6){
	printf("Face %d: Stiffness matrix:\n",Fa_id);
	for (idofs=0;idofs<num_dofs;idofs++) {
	  for (jdofs=0;jdofs<num_dofs;jdofs++) {
	    printf("%7.3lf",Stiff_mat[idofs+jdofs*num_dofs]);
	  }
	  printf("\n");
	}
	printf("Face %d: Rhs_vect:\n",Fa_id);
	for (idofs=0;idofs<num_dofs;idofs++) {
	  printf("%7.3lf",Rhs_vect[idofs]);
	}
	printf("\n");
	getchar();
 }
/*kew*/

    } /* end if computing entries to the stiffness matrix */

  } /* end if computing SM and/or RHSV */

  if(Rewr_dofs != NULL) *Rewr_dofs = 'F';

  /* change the option compute SM and RHSV to rewrite SM and RHSV */
  if(Comp_sm!=PDC_NO_COMP) Comp_sm += 3;

  /* obligatory procedure to fill Lists of dof_ents and rewite SM and RHSV */
  /* the reason is to take into account POSSIBLE CONSTRAINTS (HANGING NODES) */
  apr_get_stiff_mat_data(field_id, neig_id, Comp_sm, 'N', Pdeg_in, NULL, Nr_dof_ent,
  	         List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
  			   Nrdofs_loc, Stiff_mat, Rhs_vect);
  /*
	  FILE *plik;
	  plik=fopen("faceAKT.txt","a");
		if(Comp_sm!=PDC_NO_COMP){
	  fprintf(plik,"\nSTIFF_mat face2 %d:\n",Fa_id);
	  kk=0;
	  for (jdofs=0;jdofs<*Nrdofs_loc;jdofs++)
	  {
		for (idofs=0;idofs<*Nrdofs_loc;idofs++)
		{
			fprintf(plik," %6.15lf",Stiff_mat[kk+idofs]);
		}
		kk+=*Nrdofs_loc;
		fprintf(plik,"\n");
	  }

	  fprintf(plik,"\nRHS_vect2:\n");
	  for (idofs=0;idofs<*Nrdofs_loc;idofs++)
	  {
		fprintf(plik," %6.3lf",Rhs_vect[idofs]);
	  }
	  fprintf(plik,"\n\n\n");
		}
	  fclose(plik);
  /**/
    return(1);
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
