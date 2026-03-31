/************************************************************************
File pds_ns_supg_heat_adapt.c - adaptation functions for ns_supg_heat module

procedures in this module use the procedures from component ns_supg and heat 
modules - combining their results in a suitable way

Contains declarations of routines:
  pdr_ns_supg_heat_adapt - to enforce adaptation strategy for a given problem 
  pdr_ns_supg_heat_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
  pdr_ns_supg_heat_err_indi_explicit
  pdr_ns_supg_heat_err_indi_ZZ - to return error indicator for an element,
        based on ZZ first derivative recovery 

------------------------------
History:
        02.2002 - Krzysztof Banas, initial version
        03.2011 - Filip Krużel, navier-stokes
        06.2011 - KGB, interoperability with utd_util routines
        01.2012 - Krzysztof Banas, pobanas@cyf-kr.edu.pl
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"		/* USES */
/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"	/* IMPLEMENTS */
/* interface of the mesh manipulation module */
#include "mmh_intf.h"		/* USES */
/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* general purpose utilities */
#include "uth_intf.h"		/* USES */
/* interface for linear algebra packages */
#include "lin_alg_intf.h"	/* USES */

/* problem module's types and functions */
#include "../include/pdh_ns_supg_heat.h"	/* USES & IMPLEMENTS*/
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
// bc and material header files are included in problem header files

/* global variable for the patch data structure used in derivative recovery */
utt_patches *pdv_patches;
int pdv_ns_supg_heat_switch_adapt=0;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */



/*---------------------------------------------------------
pdr_ns_supg_heat_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
int pdr_ns_supg_heat_adapt(  /* returns: >0 - success, <=0 - failure */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output)
{

/*   double eps;		/\* eps is assumed to be a global tolerance level *\/ */
/*   double el_eps;	/\* coefficient for choosing elements to adapt *\/ */
/*   double fam_error;	/\* family error *\/ */
/*   double ratio;		/\* ratio of errors for derefinements *\/ */
/*   double *error_indi;	/\* array with elements errors *\/ */
/*   double sum_error, average_error; */
/*   int nr_elem; */
/*   int nr_ref, *list_ref;/\* number and list of elements to refine *\/ */
/*   int nr_deref, *list_deref;/\* number and list of elements to derefine *\/ */
/*   int nmel,nrel;/\* number of elements: total, active *\/ */
/*   int nmfa,nrfa;/\* number of faces: total, active *\/ */
/*   int nmed,nred;/\* number of edges: total, active *\/ */
/*   int nmno,nrno;/\* number of nodes: total, active *\/ */
/*   int father, elsons[MMC_MAXELSONS+1]; /\* family information *\/ */

/* /\* auxiliary variables *\/ */
/*   int i, iel, iaux, name, ison;  */
/*   int iprint=0; */
/*   double daux, clock_time; */
/*   int ino, nno, mesh_id, field_id; */
/*   int type, problem_id, nr_patches; */

  pdt_ns_supg_times *time_ns_supg = &pdv_ns_supg_problem.time; 

  int iprint=0;
  int type, problem_id, nr_patches;
  int i, ino, nno, mesh_id, field_id, sol_vec_id;
  double daux, clock_time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  printf("\nStarting adaptation in pdr_ns_supg_heat_adapt.\n");

  //we usually employ ns_supg as default problem 
  /* printf("\nAssuming ns_supg as master problem in pdr_ns_supg_heat_adapt.\n"); */
  /* pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID; */
  /* problem_id = PDC_NS_SUPG_ID; */

  /* pdv_ns_supg_heat_switch_adapt = 1 - pdv_ns_supg_heat_switch_adapt; */
  /* if ( pdv_ns_supg_heat_switch_adapt ){ */
  /* //if(time_ns_supg->cur_step%2 == 0){ */
  /*   printf("\nEven time step: HEAT as master problem.\n"); */
  /*   pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID; */
  /*   problem_id = PDC_HEAT_ID; */

  /* } */
  /* else{ */
  /*   printf("\nOdd time step: NS_SUPG as master problem.\n"); */
  /*   pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID; */
  /*   problem_id = PDC_NS_SUPG_ID; */

  /* } */

//  printf("\nAssuming ns_supg as master problem in pdr_ns_supg_heat_adapt.\n");
  pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
  problem_id = PDC_NS_SUPG_ID;

  //printf("\nAssuming HEAT as master problem in pdr_ns_supg_heat_adapt!!!\n");
  //pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
  //problem_id = PDC_HEAT_ID;


  /* field_id = pdr_ctrl_i_params(problem_id, 3); */
  /* mesh_id = apr_get_mesh_id(field_id); */

/* get adaptation parameters */
  i=1; type=pdr_adapt_i_params(problem_id,i);
  printf("Adapt type %d\n", type);
  //i=5; eps=pdr_adapt_d_params(problem_id,i);
  //i=6; ratio=pdr_adapt_d_params(problem_id,i);
  //i=7; iprint=pdr_adapt_i_params(problem_id,i);

  // type == -2 - uniform derefinement - may be performed by utr_adapt
  // type == -1 - uniform refinement - may be performed by utr_adapt
  // type == 0 - no adaptations
  // type > 0  - adaptive refinement with the following 
  // most popular types encoded in include/pdh_intf.h as:
  //  PDC_ADAPT_EXACT = 1 - adaptations based on the knowledge of exact solution
  //  PDC_ADAPT_ZZ    = 2 - adaptations based on Zienkiewicz-Zhu error estimate
  //  PDC_ADAPT_EXPL  = 3 - adaptations based on explicit residual error estimate
  if (type == PDC_ADAPT_EXACT) {
    printf("No exact solution for ns_supg_heat problem!.\n");
    printf("Change adaptation type in input file to: \n");
    printf("PDC_ADAPT_ZZ = 2 or PDC_ADAPT_EXPL  = 3 (not working now) \n");
  }
  else if (type == PDC_ADAPT_ZZ) {

    pdv_ns_supg_heat_switch_adapt = 1 - pdv_ns_supg_heat_switch_adapt;
    if ( pdv_ns_supg_heat_switch_adapt ){
      //if(time_ns_supg->cur_step%2 == 0){
      printf("\nEven time step: HEAT as master problem.\n");
      pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
      problem_id = PDC_HEAT_ID;

    }
    else{
      printf("\nOdd time step: NS_SUPG as master problem.\n");
      pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
      problem_id = PDC_NS_SUPG_ID;

    }
    field_id = pdr_ctrl_i_params(problem_id, 3);
    mesh_id = apr_get_mesh_id(field_id);

    /* get adaptation parameters */
    i=1; type=pdr_adapt_i_params(problem_id,i);

    // create patches for nno_old nodes
    clock_time = time_clock();
    //nr_patches = utr_create_patches(field_id, &pdv_patches);
    nr_patches = utr_create_patches_small(field_id, &pdv_patches);
    clock_time = time_clock() - clock_time;
    printf("New patch creation (ZZ) - time %lf\n", clock_time);


    // recover all first derivatives of all solution components
    clock_time = time_clock();

    // !!! it is assumed that the most recent dofs are in the last vector
    //int sol_vec_id = apr_get_nr_sol(field_id);
    // !!! it is assumed that the most recent dofs are in the first vector
    sol_vec_id = 1;
    // recover all first derivatives of all solution components
    //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
    utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

    clock_time = time_clock() - clock_time;
    printf("Derivatives recovery (ZZ) - time %lf\n", clock_time);

    // next steps:
    // 1. rewrite ns_supg derivatives to local data structures
    // 2. recover derivatives to heat equations
    // 3. rewrite heat derivatives to local data structures (may be ommited)
    // 4. prepare data structures for pdr_err_indi to return error indicators
    //    for the whole combined ns_supg + heat problem

    // enforce mesh refinement for ns_supg+heat problem 
    pdr_ns_supg_heat_refine(problem_id, type, Interactive_output); 

    // free the space - only nno_old patches; no patches for new nodes !!! */
    for (ino = 1; ino <= nr_patches; ino++) {
      if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
    }
    free(pdv_patches);

  }
  // common adaptation for flow (+10) and heat (+20) fields
  else if (type == (PDC_ADAPT_ZZ+10+20)) {

    printf("\nZienkiewicz-Zhou adaptation based on ns_supg and heat fields.\n");

    // flow problem
    printf("\nAssuming ns_supg as master problem in pdr_ns_supg_heat_adapt.\n");
    pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
    problem_id = PDC_NS_SUPG_ID;
    field_id = pdr_ctrl_i_params(problem_id, 3);
    mesh_id = apr_get_mesh_id(field_id);
    // create patches for nno_old nodes
    clock_time = time_clock();
    nr_patches = utr_create_patches_small(field_id, &pdv_patches);
    clock_time = time_clock() - clock_time;
    printf("New patch creation for fluid flow - time %lf\n", clock_time);
    // recover all first derivatives of all solution components
    clock_time = time_clock();

    // !!! it is assumed that the most recent dofs are in the last vector
    //int sol_vec_id = apr_get_nr_sol(field_id);
    // !!! it is assumed that the most recent dofs are in the first vector
    sol_vec_id = 1;
    // recover all first derivatives of all solution components
    //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
    utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

    clock_time = time_clock() - clock_time;
    printf("Derivatives recovery for fluid flow - time %lf\n", clock_time);
    // enforce mesh refinement for ns_supg problem 
    pdr_ns_supg_heat_refine(problem_id, PDC_ADAPT_ZZ, Interactive_output); 
    // free the space - only nno_old patches; no patches for new nodes !!! */
    for (ino = 1; ino <= nr_patches; ino++) {
      if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
    }
    free(pdv_patches);

    // heat problem
    printf("\nAssuming heat as master problem in pdr_ns_supg_heat_adapt.\n");
    pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
    problem_id = PDC_HEAT_ID;
    field_id = pdr_ctrl_i_params(problem_id, 3);
    mesh_id = apr_get_mesh_id(field_id);
    // create patches for nno_old nodes
    clock_time = time_clock();
    nr_patches = utr_create_patches_small(field_id, &pdv_patches);
    clock_time = time_clock() - clock_time;
    printf("New patch creation for heat field - time %lf\n", clock_time);
    // recover all first derivatives of all solution components
    clock_time = time_clock();

    // !!! it is assumed that the most recent dofs are in the last vector
    //int sol_vec_id = apr_get_nr_sol(field_id);
    // !!! it is assumed that the most recent dofs are in the first vector
    sol_vec_id = 1;
    // recover all first derivatives of all solution components
    //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
    utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

    clock_time = time_clock() - clock_time;
    printf("Derivatives recovery for heat field - time %lf\n", clock_time);
    // enforce mesh refinement for heat problem 
    pdr_ns_supg_heat_refine(problem_id, PDC_ADAPT_ZZ, Interactive_output); 
    // free the space - only nno_old patches; no patches for new nodes !!! */
    for (ino = 1; ino <= nr_patches; ino++) {
      if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
    }
    free(pdv_patches);

  }
  // common adaptation for flow (+10), heat (+20) and heating-cooling rate (+40) fields
  else if (type == (PDC_ADAPT_ZZ+10+20+40)) {

    printf("\nZienkiewicz-Zhou adaptation based on ns_supg, heat and heating velocity fields.\n");

    // flow problem
    printf("\nAssuming ns_supg as master problem in pdr_ns_supg_heat_adapt.\n");
    pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
    problem_id = PDC_NS_SUPG_ID;
    field_id = pdr_ctrl_i_params(problem_id, 3);
    mesh_id = apr_get_mesh_id(field_id);
    // create patches for nno_old nodes
    clock_time = time_clock();
    nr_patches = utr_create_patches_small(field_id, &pdv_patches);
    clock_time = time_clock() - clock_time;
    printf("New patch creation for fluid flow - time %lf\n", clock_time);
    // recover all first derivatives of all solution components
    clock_time = time_clock();

    // !!! it is assumed that the most recent dofs are in the last vector
    //int sol_vec_id = apr_get_nr_sol(field_id);
    // !!! it is assumed that the most recent dofs are in the first vector
    sol_vec_id = 1;
    // recover all first derivatives of all solution components
    //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
    utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

    clock_time = time_clock() - clock_time;
    printf("Derivatives recovery for fluid flow - time %lf\n", clock_time);
    // enforce mesh refinement for ns_supg problem 
    pdr_ns_supg_heat_refine(problem_id, PDC_ADAPT_ZZ, Interactive_output); 
    // free the space - only nno_old patches; no patches for new nodes !!! */
    for (ino = 1; ino <= nr_patches; ino++) {
      if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
    }
    free(pdv_patches);

    // heat problem
    printf("\nAssuming heat as master problem in pdr_ns_supg_heat_adapt.\n");
    pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
    problem_id = PDC_HEAT_ID;
    field_id = pdr_ctrl_i_params(problem_id, 3);
    mesh_id = apr_get_mesh_id(field_id);
    // create patches for nno_old nodes
    clock_time = time_clock();
    nr_patches = utr_create_patches_small(field_id, &pdv_patches);
    clock_time = time_clock() - clock_time;
    printf("New patch creation for heat field - time %lf\n", clock_time);
    // recover all first derivatives of all solution components
    clock_time = time_clock();

    // !!! it is assumed that the most recent dofs are in the last vector
    //int sol_vec_id = apr_get_nr_sol(field_id);
    // !!! it is assumed that the most recent dofs are in the first vector
    sol_vec_id = 1;
    // recover all first derivatives of all solution components
    //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
    utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

    clock_time = time_clock() - clock_time;
    printf("Derivatives recovery for heat field - time %lf\n", clock_time);
    // enforce mesh refinement for heat problem 
    pdr_ns_supg_heat_refine(problem_id, PDC_ADAPT_ZZ, Interactive_output); 
    // free the space - only nno_old patches; no patches for new nodes !!! */
    for (ino = 1; ino <= nr_patches; ino++) {
      if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
    }
    free(pdv_patches);

    /* // heating velocity problem */
    /* printf("\nAssuming heating velocity as master problem in pdr_ns_supg_heat_adapt.\n"); */
    /* pdv_ns_supg_heat_current_problem_id = PDC_HEAT_DTDT_ID; */
    /* problem_id = PDC_HEAT_DTDT_ID; */
    /* field_id = pdr_ctrl_i_params(problem_id, 3); */
    /* mesh_id = apr_get_mesh_id(field_id); */
    /* // create patches for nno_old nodes */
    /* clock_time = time_clock(); */
    /* nr_patches = utr_create_patches_small(field_id, &pdv_patches); */
    /* clock_time = time_clock() - clock_time; */
    /* printf("New patch creation for heat field - time %lf\n", clock_time); */
    /* // recover all first derivatives of all solution components */
    /* clock_time = time_clock(); */

    /* // !!! it is assumed that the most recent dofs are in the last vector */
    /* //int sol_vec_id = apr_get_nr_sol(field_id); */
    /* // !!! it is assumed that the most recent dofs are in the first vector */
    /* sol_vec_id = 1; */
    /* // recover all first derivatives of all solution components */
    /* //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches); */
    /* utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches); */

    /* clock_time = time_clock() - clock_time; */
    /* printf("Derivatives recovery for heating velocity field - time %lf\n", clock_time); */
    /* // enforce mesh refinement for heating velocity problem  */
    /* pdr_ns_supg_heat_refine(problem_id, PDC_ADAPT_ZZ, Interactive_output);  */
    /* // free the space - only nno_old patches; no patches for new nodes !!! *\/ */
    /* for (ino = 1; ino <= nr_patches; ino++) { */
    /*   if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv); */
    /* } */
    /* free(pdv_patches); */


  }
  else{
    printf("Wrong adapt_type in adapt!");
    // ?????

  }

/*kbw
  nno=0;
  while((nno=mmr_get_next_node_all(mesh_id, nno))!=0){
    double f,f_x,f_y,f_z,xcoor[3],eaux;
    printf("\nReal derivatives for node %d :\n",nno);
    mmr_node_coor(mesh_id, nno, xcoor);
    pdr_exact_sol(iaux,xcoor[0],xcoor[1],xcoor[2],daux,
                    &f,&f_x,&f_y,&f_z,&eaux);
    printf("%8.4lf%8.4lf%8.4lf", f_x, f_y, f_z);
    printf("\nRecovered derivatives for node %d :\n",nno);
    for(ider=0;ider<nr_deriv;ider++)
      {
        printf("%8.4lf", pdv_patches[nno].deriv[ider]);
      }
    printf("\n");
  }
/*kew*/

// enforce mesh refinement for ns_supg+heat problem 
  /* pdr_ns_supg_heat_refine(problem_id, type, Interactive_output);  */


  /* if ((type == PDC_ADAPT_ZZ)) { */

  /*   // free the space - only nno_old patches; no patches for new nodes !!! *\/ */
  /*   for (ino = 1; ino <= nr_patches; ino++) { */
  /*     if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv); */
  /*   } */
  /*   free(pdv_patches); */

  /* }				// end if ZZ adaptation */

  return (1);
}


/*---------------------------------------------------------
pdr_ns_supg_heat_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_ns_supg_heat_ZZ_error(		
	   /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
)
{

/* local variables */
  double err_ZZ = 0.0, el_err = 0.0;	/* error norms */
  double err_ZZ_LDC = 0.0;	/* error norms */
  int i, j, k, mesh_id, nreq, nr_deriv, nr_patches, ino, nno, nel;
  double clock_time;
  int problem_id, name, field_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  // we usually employ ns_supg as default problem 
  pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
  problem_id = PDC_NS_SUPG_ID;
  field_id = pdr_ctrl_i_params(problem_id, 3);
  mesh_id = apr_get_mesh_id(field_id);

  nreq = apr_get_nreq(field_id);
  nr_deriv = 3 * nreq;

  // create patches for nno_old nodes
  clock_time = time_clock();
  //nr_patches = utr_create_patches(field_id, &pdv_patches);
  nr_patches = utr_create_patches_small(field_id, &pdv_patches);
  clock_time = time_clock()-clock_time;
  fprintf(Interactive_output, "Patch creation - time %lf\n", clock_time);


  // recover all first derivatives of all solution components
  clock_time = time_clock();

  // !!! it is assumed that the most recent dofs are in the last vector
  //int sol_vec_id = apr_get_nr_sol(field_id);
  // !!! it is assumed that the most recent dofs are in the first vector
  int sol_vec_id = 1;
  // recover all first derivatives of all solution components
  //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
  utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

  clock_time = time_clock()-clock_time;
  fprintf(Interactive_output, "Derivative recovery - time %lf\n", clock_time);

/*kbw
  nno=0;
  while((nno=mmr_get_next_node_all(mesh_id, nno))!=0){
    double f,f_x,f_y,f_z,xcoor[3],eaux;
    printf("\nReal derivatives for node %d :\n",nno);
    mmr_node_coor(mesh_id, nno, xcoor);
    pdr_exact_sol(iaux,xcoor[0],xcoor[1],xcoor[2],daux,
                    &f,&f_x,&f_y,&f_z,&eaux);
    printf("%8.4lf%8.4lf%8.4lf", f_x, f_y, f_z);
    printf("\nRecovered derivatives for node %d :\n",nno);
    for(ider=0;ider<nr_deriv;ider++)
      {
        printf("%8.4lf", pdv_patches[nno].deriv[ider]);
      }
    printf("\n");
  }
/*kew*/

  nel = 0;
  err_ZZ = 0;
  err_ZZ_LDC = 0;
  while ((nel = mmr_get_next_act_elem(mesh_id, nel)) != 0) {
    el_err = pdr_ns_supg_heat_err_indi_ZZ(field_id, nel);
    
#ifdef DEBUG
    if (el_err < -10e200 || el_err > 10e200) {
      printf("in active element %d - error_indi %20.15lf. Exiting! \n",
	     nel,el_err);
      exit(-1);
    }
#endif
    
    err_ZZ += el_err;
    
    /*kbw
      printf("in active element %d - error_indi %20.15lf \n",
      nel,el_err);
      err_ZZ += pdr_ns_supg_heat_err_indi_ZZ(Field_id,nel);
      /*kew */
    
  }
  err_ZZ = sqrt(err_ZZ);
  fprintf(Interactive_output,"\nZienkiewicz-Zhu error estimator = %lf\n",err_ZZ);

  // free the space */
  for (ino = 1; ino <= nr_patches; ino++) {
    if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
  }
  free(pdv_patches);

  return (err_ZZ);

}

/*---------------------------------------------------------
pdr_ns_supg_heat_err_indi_ZZ - to return error indicator for an element
----------------------------------------------------------*/
double pdr_ns_supg_heat_err_indi_ZZ(/* returns error indicator for an element */
  int Problem_id, 
  int El	/* in: element number */
    )
{
  double err = 0;
  int i, ki, j, k, ieq;
  int problem_id_tmp, field_id, mesh_id, pdeg, num_shap, sol_vec_id, iaux, nreq;
  int base;		/* type of basis functions  */
  int el_nodes[MMC_MAXELVNO + 1];	/* list of nodes of El */
  double node_coor[3 * MMC_MAXELVNO];	/* coord of nodes of El */
  double dofs_loc[APC_MAXELSD];	/* element solution dofs */
  double xcoor[3];		/* global coord of gauss point */
  double u_val[PDC_MAXEQ];	/* computed solution */
  double u_x[PDC_MAXEQ];		/* gradient of computed solution */
  double u_y[PDC_MAXEQ];		/* gradient of computed solution */
  double u_z[PDC_MAXEQ];		/* gradient of computed solution */
  double base_phi[APC_MAXELVD];	/* basis functions */
  double base_dphix[APC_MAXELVD];	/* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];	/* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];	/* y-derivatives of basis function */
  double determ;
  int ngauss;			/* number of gauss points */
  double xg[3000];		/* coordinates of gauss points in 3D */
  double wg[1000];		/* gauss weights */
  double vol;			/* volume for integration rule */
  double deriv_el_nodes[3][9];	/* derivatives at element nodes */
  double deriv_at_point[3];
  int nr_comp;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
    problem_id_tmp = PDC_NS_SUPG_ID;
  }
  else{
    pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
    problem_id_tmp = PDC_HEAT_ID;
  }

/* get formulation parameters */
  //i=1; name=pdr_ctrl_i_params(problem_id_tmp,i);
  i=2; mesh_id=pdr_ctrl_i_params(problem_id_tmp,i);
  i=3; field_id=pdr_ctrl_i_params(problem_id_tmp,i);

  if(Problem_id==PDC_NS_SUPG_ID){
    nreq = 4; 
    nr_comp = 3; // we consider only velocities for ns_supg error indicator
  }
  else{
    nreq = 1;
    nr_comp = nreq;
  }

/* get adaptation parameters */
  //i=1; type=pdr_adapt_i_params(problem_id_tmp,i);
  //i=5; eps=pdr_adapt_d_params(problem_id_tmp,i);
  //i=6; ratio=pdr_adapt_d_params(problem_id_tmp,i);
  //i=7; iprint=pdr_adapt_i_params(problem_id_tmp,i);


  /* find degree of polynomial and number of element scalar dofs */
  apr_get_el_pdeg(field_id, El, &pdeg);

  /* get the coordinates of the nodes of El in the right order */
  mmr_el_node_coor(mesh_id, El, el_nodes, node_coor);

  /* prepare data for gaussian integration */
  base = apr_get_base_type(field_id, El);
  apr_set_quadr_3D(base, &pdeg, &ngauss, xg, wg);

  /* get the most recent solution degrees of freedom */
  sol_vec_id = 1;
  apr_get_el_dofs(field_id, El, sol_vec_id, dofs_loc);

  // initialize element error
  err = 0.0;

  // for each solution component
  for (ieq = 0; ieq < nr_comp; ieq++) {

    // get 3 derivatives 
    for (j = 0; j < 3; j++) {
      k = 0;
      for (i = 1; i <= el_nodes[0]; i++) {
	deriv_el_nodes[j][k] = pdv_patches[el_nodes[i]].deriv[3 * ieq + j];
	k++;
      }
    }
    for (j = 0; j < 3; j++) deriv_at_point[j] = 0.0;


    for (ki = 0; ki < ngauss; ki++) {
      /* at the gauss point, compute basis functions, determinant etc */
      iaux = 2;	/* calculations with jacobian but not on the boundary */
      determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, &xg[3 * ki], node_coor, 
				dofs_loc, base_phi, base_dphix, base_dphiy, 
				base_dphiz, xcoor, u_val, u_x, u_y, u_z, NULL);
      vol = determ * wg[ki];


      // get derivatives for ieq unknown at gauss points
      for (i = 0; i < 3; i++) {
	iaux = 1;
	apr_elem_calc_3D(iaux, 1, &pdeg, base, &xg[3 * ki], node_coor, 
			 &deriv_el_nodes[i][0], base_phi, base_dphix, 
			 base_dphiy, base_dphiz, xcoor, &deriv_at_point[i], 
			 NULL, NULL, NULL, NULL);
      }

      err += vol * 
	((deriv_at_point[0] - u_x[ieq]) * (deriv_at_point[0] - u_x[ieq]) + 
	 (deriv_at_point[1] - u_y[ieq]) * (deriv_at_point[1] - u_y[ieq]) + 
	 (deriv_at_point[2] - u_z[ieq]) * (deriv_at_point[2] - u_z[ieq])
	  );

    }				// end loop over gauss points

  }				// end loop over solution components

  //  assert(err > -10e200);
  //assert(err < 10e200);
  return (err);

}


/*---------------------------------------------------------
pdr_ns_supg_heat_err_indi_explicit - to return error indicator for an element
  // NOT IMPLEMENTED 
----------------------------------------------------------*/
double pdr_ns_supg_heat_err_indi_explicit(	
                             /* returns error indicator for an element */
  int Problem_id, 
  int El	/* in: element number */
    )
{

  return (0.0);
}

/*---------------------------------------------------------
pdr_ns_supg_heat_refine - to enforce mesh refinement for ns_supg+heat problem 
---------------------------------------------------------*/
int pdr_ns_supg_heat_refine(  /* returns: >0 - success, <=0 - failure */
  int Problem_id, // leading problem ID
  int Ref_type, // type of refinement 
  FILE *Interactive_output
)
{

  double eps;		/* eps is assumed to be a global tolerance level */
  double el_eps;	/* coefficient for choosing elements to adapt */
  double fam_error;	/* family error */
  double ratio;		/* ratio of errors for derefinements */
  double *error_indi;	/* array with elements errors */
  double sum_error, average_error, max_error;
  int nr_elem;
  int nr_ref, *list_ref;/* number and list of elements to refine */
  int nr_deref, *list_deref;/* number and list of elements to derefine */
  int nmel,nrel;/* number of elements: total, active */
  int nmfa,nrfa;/* number of faces: total, active */
  int nmed,nred;/* number of edges: total, active */
  int nmno,nrno;/* number of nodes: total, active */
  int father, elsons[MMC_MAXELSONS+1]; /* family information */

/* auxiliary variables */
  int i, iel, iaux, name, ison; 
  int iprint=0;
  double daux, clock_time;
  int ino, nno, mesh_id, field_id;
/*  heating-cooling problem parameters */
  /* int num_dofs, num_eq; */
  /* int Current; */
  /* double sol_dofs[APC_MAXELSD];	/\* solution dofs *\/ */
  /* pdt_heat_problem *problem_heat = &pdv_heat_problem; */
  /* pdt_heat_dtdt_problem *problem_heat_dtdt = &pdv_heat_dtdt_problem; */
  /* pdt_heat_dtdt_ctrls *ctrl_heat_dtdt = &pdv_heat_dtdt_problem.ctrl; */
  /* pdt_heat_material_query_params query_params; */
  /* pdt_heat_material_query_result query_result; */

/*++++++++++++++++ executable statements ++++++++++++++++*/

  // enforce default refinement (similar to utr_adapt and utr_manual_refinement) 
  // for Ref_type == -1 - uniform refinement
  // for Ref_type > 0  - adaptive refinement: element error indicators are 
  //             computed by pdr_err_indi, elements with error greater then 
  //             eps*average_error are broken, families of elements (sons
  //             of a single father) with total error less than 
  //             ratio*eps*average_error are clustered back
  // eps is specified as the first of ADAPT_TOLERANCE_REF_UNREF
  // ratio is specified as the second of ADAPT_TOLERANCE_REF_UNREF
  // (there is one more hook - if eps is less than 0.1 it is considered
  // as global parameter and elements with error greater then eps are broken)
  
  field_id = pdr_ctrl_i_params(Problem_id, 3);
  mesh_id = apr_get_mesh_id(field_id);

/* get adaptation parameters */
  i=5; eps=pdr_adapt_d_params(Problem_id,i);
  i=6; ratio=pdr_adapt_d_params(Problem_id,i);
  i=7; iprint=pdr_adapt_i_params(Problem_id,i);

/* get necessary mesh parameters */
  nrno = mmr_get_nr_node(mesh_id);
  nmno = mmr_get_max_node_id(mesh_id);
  nred = mmr_get_nr_edge(mesh_id);
  nmed = mmr_get_max_edge_id(mesh_id);
  nrfa = mmr_get_nr_face(mesh_id);
  nmfa = mmr_get_max_face_id(mesh_id);
  nrel = mmr_get_nr_elem(mesh_id);
  nmel = mmr_get_max_elem_id(mesh_id);


  if(iprint>1){
    fprintf(Interactive_output,"\nBefore adaptation.\n");
    fprintf(Interactive_output,"Parameters (number of active, maximal index):\n");
    fprintf(Interactive_output,"Elements: nrel %d, nmel %d\n", nrel, nmel);
    fprintf(Interactive_output,"Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa); 
    fprintf(Interactive_output,"Edges:    nred %d, nmed %d\n", nred, nmed); 
    fprintf(Interactive_output,"Nodes:    nrno %d, nmno %d\n", nrno, nmno); 
  }

  /* uniform refinement */
  if (Ref_type == -1) {
    
    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);
    
    /* perform uniform refinement */
    i=apr_refine(mesh_id,MMC_DO_UNI_REF);
    
    /* project DOFS between generations */
    iaux=-1;
	/* project DOFS between generations */
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

    // !!!!! WE HAVE TO PROJECT DOFS FOR THE SECOND PROBLEM AS WELL !!!
    if(Problem_id==PDC_NS_SUPG_ID){
      int field_temp = pdr_ctrl_i_params(PDC_HEAT_ID, 3);
      apr_proj_dof_ref(field_temp, iaux, nmel, nmfa, nmed, nmno);
      
      //int field_dtdt = pdr_ctrl_i_params(PDC_HEAT_DTDT_ID, 3);
      //apr_proj_dof_ref(field_dtdt, iaux, nmel, nmfa, nmed, nmno);
      // TODO: what if other fields also needs to be extended?
    }
    else{
      int field_temp = pdr_ctrl_i_params(PDC_NS_SUPG_ID, 3);
      apr_proj_dof_ref(field_temp, iaux, nmel, nmfa, nmed, nmno);
      
      //int field_dtdt = pdr_ctrl_i_params(PDC_HEAT_DTDT_ID, 3);
      //apr_proj_dof_ref(field_dtdt, iaux, nmel, nmfa, nmed, nmno);
      // TODO: what if other fields also needs to be extended?
    }
    
    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);
    
    apr_check_field(field_id);
	
  }/* end uniform refinement */
  else {/* adaptive refinement with error indicators */


    /* allocate space for array with error indicators */
    error_indi=utr_dvector(nmel+1,"error_indi in adapt");
    nr_ref=0;
    list_ref=utr_ivector(nmel+1,"list_ref in adapt");
    nr_deref=0;
    list_deref=utr_ivector(nmel+1,"list_deref in adapt");
    
/*kbw
      printf("In adapt: Ref_type %d, eps %lf, ratio %lf\n",
      Ref_type,  eps, ratio);
/*kew*/

    sum_error = 0.0; nr_elem=0; max_error = 0.0;
/* compute error indicators for all active elements */
    iel=0;
    while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0){
      
      error_indi[iel]=pdr_err_indi(Problem_id, Ref_type, iel);
      sum_error += error_indi[iel];
      if(error_indi[iel]>max_error) max_error = error_indi[iel];
      nr_elem++;
      
/*kbw
printf("in active element %d - error_indi %20.15lf \n",
iel,error_indi[iel]);
/*kew*/

    }

    average_error = sum_error/nr_elem;
/*kbw
	printf("In utr_adapt: total error %lf (sqrt %lf), average error %lf\n",
	   sum_error, sqrt(sum_error), average_error);
/*kew*/


/* here: eps<0.1 denotes global tolerance */
    if(eps<0.1) el_eps = eps;
/* set element limit for errors (for equidistribution principle) */
    else if(eps<1.0) el_eps = eps*max_error;
    else el_eps = eps*average_error;

/* create lists of elements for refinements and derefinements */
/* loop over all active elements */
    iel=0;
    while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0) {
  
/*kbw
printf("in active element %d - error_indi %20.15lf <> %20.15lf\n",
iel,error_indi[iel],el_eps);
/*kew*/

/* heating-cooling adaptation indicator */
      /* pdv_ns_supg_heat_current_problem_id = PDC_HEAT_DTDT_ID;  // heating-cooling problem */
      /* i=3; field__id = pdr_ctrl_i_params(pdv_ns_supg_heat_current_problem_id,i); */
      /* num_eq = apr_get_ent_nrdofs(field_id, APC_VERTEX, iel); */
      /* apr_read_ent_dofs(field_id, APC_VERTEX, iel, num_eq, Current, sol_dofs); */
      /* pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;  // heat problem */
      /* i=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_current_problem_id,i); */
      /* num_eq = apr_get_ent_nrdofs(field_id, APC_VERTEX, iel); */
      /* apr_read_ent_dofs(field_id, APC_VERTEX, iel, num_eq, Current, sol_dofs); */
      /* num_dofs = 0; // temperature numdofs = 1 */
      // CHECK WHETHER MATERIAL DATABASE USED AT ALL
      /* query_params.material_idx = 0;	//material by idx */
      /* query_params.name = ""; */
      /* query_params.temperature = sol_dofs[num_dofs]; */
      /* pdr_heat_material_query(&problem_heat->materials,  */
      /* 				  &query_params, &query_result); */
  /* && */
  /* 	 sol_dofs[num_dofs]>query_result.temp_solidus){ */



/* if error small enough and the family not yet considered */
      if(mmr_el_fam(mesh_id,iel,NULL,NULL)>0 &&
	 error_indi[iel]<ratio*el_eps && error_indi[iel]>-1){

/* find father and sons */
	father=mmr_el_fam(mesh_id,iel,NULL,NULL);
	mmr_el_fam(mesh_id,father,elsons,NULL);

/* check whether all sons are active and compute the error for the family*/
	iaux=0; fam_error=0.0;
	for(ison=1;ison<=elsons[0];ison++){
	  if(mmr_el_status(mesh_id,elsons[ison])<=0) { 
	    iaux=1; 
	    break; 
	  }
	  else {
	    fam_error+=error_indi[elsons[ison]];
	    /* indicate son should not be considered again */
	    if(error_indi[elsons[ison]]<el_eps) 
	      error_indi[elsons[ison]] = -2;
	    
/*kbw
printf("in active element %d, father %d, son %d (%d)\n",
iel,father,elsons[ison],ison);
printf("in active element %d - error_indi %20.15lf <> %20.15lf\n",
elsons[ison],error_indi[elsons[ison]],el_eps);
/*kew*/

	  }
	}

/* if all sons active and family error is smaller than the limit */
	if(iaux==0 && fam_error<ratio*el_eps){

/* if derefinement is not excluded because of irregularity constrained */   
	  if(apr_limit_deref(field_id, iel)==APC_DEREF_ALLOWED){	    

/*kbw
  printf("element %d (%d) to derefine: fam_error %15.12lf < %15.12lf\n",
  elsons[1],nr_deref,fam_error,ratio*el_eps);
/*kew*/
		
   
	    /* put first son (elsons[1]) on list to derefine */
	    list_deref[nr_deref]=elsons[1];
	    nr_deref++;
	  }

	} /* end if all sons active and family error smaller than the limit */

      } /* end if error small in element */ 
      else if(error_indi[iel]>el_eps){

	  /*kbw
printf("element %d (%d) to refine: error_indi %15.12lf > %15.12lf\n",
iel,nr_ref,error_indi[iel],el_eps);
/*kew*/

	if(apr_limit_ref(field_id, iel)==APC_REF_ALLOWED){
 /* && */
 /* 	   sol_dofs[num_dofs]>query_result.temp_solidus){	     */
	  /* put iel on list to refine */
	  list_ref[nr_ref]=iel;
	  nr_ref++;

	}

      }

    } /* end loop over active elements */

/*kbw
	printf("List to derefine (%d elements): \n",nr_deref);
	for(iel=0;iel<nr_deref;iel++) printf("%d ",list_deref[iel]);
	printf("\n");
	//getchar();
/*kew*/

	/* DEREFINEMENTS */


/*kbw
	printf("List to derefine after exchange (%d elements): \n",nr_deref);
	for(iel=0;iel<nr_deref;iel++) printf("%d ",list_deref[iel]);
	printf("\n");
	//getchar();
/*kew*/

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);
    
    /* perform derefinements */
    for(iel=0;iel<nr_deref;iel++) {
      if(mmr_el_status(mesh_id, list_deref[iel])==MMC_ACTIVE){
	if(apr_limit_deref(field_id, list_deref[iel])==APC_DEREF_ALLOWED){  
	  iaux=apr_derefine(mesh_id,list_deref[iel]);
	  if(iaux<0) {
	    printf("Unsuccessful derefinement of El %d. Exiting!\n", 
		   list_deref[iel]);
	      exit(-1);
	  }
	}
      }
    }
    
	
    /* project DOFS between generations */
    iaux=-1;
    /* project DOFS between generations */
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);
    
    // !!!!! WE HAVE TO PROJECT DOFS FOR THE SECOND PROBLEM AS WELL !!!
    if(Problem_id==PDC_NS_SUPG_ID){
      int field_temp = pdr_ctrl_i_params(PDC_HEAT_ID, 3);
      apr_proj_dof_ref(field_temp, iaux, nmel, nmfa, nmed, nmno);

      //int field_dtdt = pdr_ctrl_i_params(PDC_HEAT_DTDT_ID, 3);
      //apr_proj_dof_ref(field_dtdt, iaux, nmel, nmfa, nmed, nmno);
      // TODO: what if other fields also needs to be extended?
    }
    else{
      int field_temp = pdr_ctrl_i_params(PDC_NS_SUPG_ID, 3);
      apr_proj_dof_ref(field_temp, iaux, nmel, nmfa, nmed, nmno);

      //int field_dtdt = pdr_ctrl_i_params(PDC_HEAT_DTDT_ID, 3);
      //apr_proj_dof_ref(field_dtdt, iaux, nmel, nmfa, nmed, nmno);
      // TODO: what if other fields also needs to be extended?
    }

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);
    

    /* REFINEMENTS */

/*kbw
	printf("List to refine before update (%d elements): \n",nr_ref);
	for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
	printf("\n");
	getchar();
/*kew*/


/*kbw
	  printf("List to refine after exchange (%d elements): \n",nr_ref);
	  for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
	  printf("\n");
	  //getchar();
/*kew*/

	/* update list of refined elements due to irregularity constraint 
	mmr_update_ref_list(mesh_id, &nr_ref, list_ref);
	*/

/*kbw
	printf("List to refine after update (%d elements): \n",nr_ref);
	for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
	printf("\n");
	//getchar();
/*kew*/

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);
    
    /* next perform refinements */
    for(iel=0;iel<nr_ref;iel++) {
      if(mmr_el_status(mesh_id, list_ref[iel])==MMC_ACTIVE){
	if(apr_limit_ref(field_id, list_ref[iel])==APC_REF_ALLOWED){
 /* && */
 /* 	   sol_dofs[num_dofs]>query_result.temp_solidus){	     */
	  iaux=apr_refine(mesh_id,list_ref[iel]);
	  if(iaux<0) {
	    printf("Unsuccessful derefinement of El %d. Exiting!\n", 
		   list_deref[iel]);
	      exit(-1);
	  }
	}
      }
    }

 
    /* project DOFS between generations */
    iaux=-1;
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);
    
    // !!!!! WE HAVE TO PROJECT DOFS FOR THE SECOND PROBLEM AS WELL !!!
    if(Problem_id==PDC_NS_SUPG_ID){
      int field_temp = pdr_ctrl_i_params(PDC_HEAT_ID, 3);
      apr_proj_dof_ref(field_temp, iaux, nmel, nmfa, nmed, nmno);

      //int field_dtdt = pdr_ctrl_i_params(PDC_HEAT_DTDT_ID, 3);
      //apr_proj_dof_ref(field_dtdt, iaux, nmel, nmfa, nmed, nmno);
      // TODO: what if other fields also needs to be extended?
    }
    else{
      int field_temp = pdr_ctrl_i_params(PDC_NS_SUPG_ID, 3);
      apr_proj_dof_ref(field_temp, iaux, nmel, nmfa, nmed, nmno);

      //int field_dtdt = pdr_ctrl_i_params(PDC_HEAT_DTDT_ID, 3);
      //apr_proj_dof_ref(field_dtdt, iaux, nmel, nmfa, nmed, nmno);
      // TODO: what if other fields also needs to be extended?
    }

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);
    
    apr_check_field(field_id);
    
    
    /* free the space */
    free(error_indi);
    free(list_ref);
    free(list_deref);
    

  } /* end if not uniform refinements */
  
  /* get necessary mesh parameters */
  nrno = mmr_get_nr_node(mesh_id);
  nmno = mmr_get_max_node_id(mesh_id);
  nred = mmr_get_nr_edge(mesh_id);
  nmed = mmr_get_max_edge_id(mesh_id);
  nrfa = mmr_get_nr_face(mesh_id);
  nmfa = mmr_get_max_face_id(mesh_id);
  nrel = mmr_get_nr_elem(mesh_id);
  nmel = mmr_get_max_elem_id(mesh_id);
  
  if(iprint>1){
    fprintf(Interactive_output,"\nAfter adaptation.\n");
    fprintf(Interactive_output,"Parameters (number of active, maximal index):\n");
    fprintf(Interactive_output,"Elements: nrel %d, nmel %d\n", nrel, nmel);
    fprintf(Interactive_output,"Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa); 
    fprintf(Interactive_output,"Edges:    nred %d, nmed %d\n", nred, nmed); 
    fprintf(Interactive_output,"Nodes:    nrno %d, nmno %d\n", nrno, nmno); 
  }

  return(0);
}
