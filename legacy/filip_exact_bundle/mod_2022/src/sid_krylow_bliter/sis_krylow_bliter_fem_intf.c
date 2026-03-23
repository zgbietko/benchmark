/*************************************************************
File contains procedures:
 fem_proj_sol_lev - to L2 project solution dofs between mesh levels
 fem_vec_norm - to compute a norm of global vector in parallel
 fem_sc_prod - to compute a scalar product of two global vectors
 fem_exchange_dofs - to exchange dofs between processors

History:
	05.2001 - Krzysztof Banas, initial version		

*************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>

/* problem dependent module with implementation of pdr_ routines */
#include "pdh_intf.h"

/* general solver interface */
#include "sih_intf.h"

/* linear solver (adapter) required interface */
#include "../lsd_mkb/lsh_mkb_fem_intf.h"

/* specific declarations for krylow_bliter */
#include "./sih_krylow_bliter.h"

/* LAPACK procedures */
#include "lin_alg_intf.h"


/*---------------------------------------------------------
  fem_proj_sol_lev - to L2 project solution dofs between mesh levels
---------------------------------------------------------*/
int fem_proj_sol_lev( /* returns: >=0 - success; <0 - error code*/
  int Solver_id,        /* in: solver data structure to be used */
  int Ilev_from,    /* in: level number to project from */
  double* Vec_from, /* in: vector of values to project */
  int Ilev_to,      /* in: level number to project to */
  double* Vec_to    /* out: vector of projected values */
  )
{

#ifdef DEBUG_SIM
  printf("No projection implemented in single level solver\n");
#endif

  return(0);
}

/*---------------------------------------------------------
  fem_vec_norm - to compute a norm of global vector (in parallel)
---------------------------------------------------------*/
double fem_vec_norm( /* returns: L2 norm of global Vector */
  int Solver_id,        /* in: solver data structure to be used */
  int Level_id,         /* in: level number */
  int Nrdof,            /* in: number of vector components */
  double* Vector        /* in: local part of global Vector */
  )
{

  int IONE=1; int problem_id; double vec_norm;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  if(siv_solver[Solver_id].parallel){

    problem_id = siv_solver[Solver_id].problem_id;
    vec_norm = pdr_vec_norm(problem_id, Solver_id, Level_id, Nrdof, Vector);

  } else {

    vec_norm = dnrm2_(&Nrdof, Vector, &IONE);

  }

  return(vec_norm);
}


/*---------------------------------------------------------
  fem_sc_prod - to compute a scalar product of two global vectors 
---------------------------------------------------------*/
double fem_sc_prod( /* retruns: scalar product of Vector1 and Vector2 */
  int Solver_id,        /* in: solver data structure to be used */
  int Level_id,         /* in: level number */
  int Nrdof,           /* in: number of vector components */
  double* Vector1,     /* in: local part of global Vector */
  double* Vector2      /* in: local part of global Vector */
  )
{

  int IONE=1; int problem_id; double sc_prod;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  if(siv_solver[Solver_id].parallel){

    problem_id = siv_solver[Solver_id].problem_id;
    sc_prod = pdr_sc_prod(problem_id,Solver_id,Level_id,Nrdof,Vector1,Vector2);

  } else {

    sc_prod = ddot_(&Nrdof, Vector1, &IONE, Vector2, &IONE);

  }

  return(sc_prod);
}


/*---------------------------------------------------------
  fem_exchange_dofs - to exchange dofs between processors
---------------------------------------------------------*/
int fem_exchange_dofs(
  int Solver_id,        /* in: solver data structure to be used */
  int Level_id,         /* in: level number */
  double* Vec_dofs  /* in: vector of dofs to be exchanged */
)
{

  int problem_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

//!!! If renumbering used - should be applied here !!! 

  if(siv_solver[Solver_id].parallel){
    
    problem_id = siv_solver[Solver_id].problem_id;
    pdr_exchange_dofs(problem_id, Solver_id, Level_id, Vec_dofs);
    
  }

  return(1);
}
