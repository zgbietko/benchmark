/*************************************************************
File contains procedures:
  itr_proj_sol_lev - to L2 project solution dofs between mesh levels
  it_par_vec_norm - to compute a norm of global vector in parallel
  it_par_sc_prod - to compute a scalar product of two global vectors 
  it_par_exchange_dofs - to exchange dofs between processors

In the current implementation the library requires to be linked with
the fem code providing the functions called in this file and declared
in file "it_bliter_fem_intf.h":
  fem_proj_sol_lev - to L2 project solution dofs between mesh levels
  fem_vec_norm - to compute a norm of global vector (possibly in parallel)
  fem_sc_prod - to compute a scalar product of two global vectors ( -||- ) 
  fem_exchange_dofs - to exchange dofs between processors

It can be made more elegant by defining a data structure with function
pointers for the above functions. The pointers will be filled with
values during the execution.

History:
	05.2013 - Krzysztof Banas, initial version		

*************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>

/* internal data structures and functions headers */
#include "./include/it_mkb.h"

/* interface with the FEM code - routines called by the solver */
#include "./lsh_mkb_fem_intf.h"

/* LAPACK procedures */
#include "lin_alg_intf.h"


/*---------------------------------------------------------
  itr_proj_sol_lev - to L2 project solution dofs between mesh levels
---------------------------------------------------------*/
int itr_proj_sol_lev( /* returns: 1 - success; <=0 - error code*/
	int Solver_id,        /* in: solver data structure to be used */
	int Ilev_from,    /* in: level number to project from */
        double* Vec_from, /* in: vector of values to project */
	int Ilev_to,      /* in: level number to project to */
        double* Vec_to    /* out: vector of projected values */
	)
{


  if(itv_solver[Solver_id].nr_level > 1){
    fem_proj_sol_lev(Solver_id,Ilev_from,Vec_from,Ilev_to,Vec_to);
  }

  return(1);
}

/*---------------------------------------------------------
  it_par_vec_norm - to compute a norm of global vector in parallel
---------------------------------------------------------*/
double it_par_vec_norm( /* returns: L2 norm of global Vector */
  int Nrdof,            /* in: number of vector components */
  double* Vector        /* in: local part of global Vector */
  )
{

  int IONE=1; double vec_norm;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  if(itv_solver[itv_cur_solver_id].parallel == ITC_PARALLEL){

    vec_norm = fem_vec_norm(itv_cur_solver_id,
			itv_solver[itv_cur_solver_id].cur_level,
			Nrdof, Vector);

  }
  else{

    vec_norm = dnrm2_(&Nrdof, Vector, &IONE);

  }

  return(vec_norm);

}


/*---------------------------------------------------------
  it_par_sc_prod - to compute a scalar product of two global vectors 
---------------------------------------------------------*/
double it_par_sc_prod( /* retruns: scalar product of Vector1 and Vector2 */
  int Nrdof,           /* in: number of vector components */
  double* Vector1,     /* in: local part of global Vector */
  double* Vector2      /* in: local part of global Vector */
  )
{

  int IONE=1; double sc_prod;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  if(itv_solver[itv_cur_solver_id].parallel == ITC_PARALLEL){

    sc_prod = fem_sc_prod(itv_cur_solver_id,
		       itv_solver[itv_cur_solver_id].cur_level,
		       Nrdof, Vector1, Vector2);

  }
  else{

    sc_prod = ddot_(&Nrdof, Vector1, &IONE, Vector2, &IONE);
  
  }

  return(sc_prod);
}


/*---------------------------------------------------------
  it_par_exchange_dofs - to exchange dofs between processors
---------------------------------------------------------*/
int it_par_exchange_dofs(
  double* Vec_dofs  /* in: vector of dofs to be exchanged */
)
{

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(itv_solver[itv_cur_solver_id].parallel == ITC_PARALLEL){

    fem_exchange_dofs(itv_cur_solver_id,
		      itv_solver[itv_cur_solver_id].cur_level,
		      Vec_dofs);
  }

  return(1);
}
