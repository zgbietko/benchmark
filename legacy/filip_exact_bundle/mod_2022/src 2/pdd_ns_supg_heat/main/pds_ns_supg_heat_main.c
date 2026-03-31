/************************************************************************
File pds_ns_supg_heat_main.c - driver for ns_supg_heat module - approximation
      of Navier_Stokes equations with heat transfer and convection

Considered to be a model coupled problem with two ingredients:  
 ns_supg -> problem_id = PDC_NS_SUPG_ID (1) (ns_supg problem=module)
 heat -> problem_id = PDC_HEAT (2) (heat problem=module)

Contains definition of global variables - among them:
  pdv_ns_supg_problem - problem structure for ns_supg problem 
  pdv_heat_problem - problem structure for heat problem
					      
Contains definition of routines:
  main

Procedures local to main:
  pdr_ns_supg_heat_init - initialize both problems data
  pdr_ns_supg_heat_post_process
  pdr_ns_supg_heat_profile
  pdr_ns_supg_heat_initial_condition
  pdr_ns_supg_heat_bc_free
  pdr_ns_supg_heat_material_free

Implementation of pdh_intf.h:
  pdr_err_indi - to return error indicator for an element
Implementation of pdh_control_intf.h:
  pdr_get_problem_structure - to return pointer to problem structure
  pdr_ctrl_i_params - to return one of control parameters
  pdr_ctrl_d_params - to return one of control parameters

REMARK:
The code uses solution_1 for character reading problem in C - namely
scanf("%c", &var);getchar();
Possible solution_2 is:
scanf(" %c", &var);

------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2012    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)

*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include<string.h>

/* utilities - including simple time measurement library */
#include "uth_intf.h"		/* USES */
/* interface for all mesh manipulation modules */
#include "mmh_intf.h"		/* USES */
/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* interface for all solver modules */
#include "sih_intf.h"		/* USES */
/* problem dependent module interface */
#include "pdh_intf.h"		/* USES */
/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"	/* IMPLEMENTS */
#include "uth_log.h"
#include "uth_mesh.h"

#ifdef PARALLEL
/* interface of parallel mesh manipulation modules */
#include "mmph_intf.h"		/* USES */
/* interface for all parallel approximation modules */
#include "apph_intf.h"		/* USES */
/* interface for parallel communication modules */
#include "pch_intf.h"		/* USES */
#endif

/* visualization module */
#include "mod_fem_viewer.h"	/* USES */

/* problem module's types and functions */
#include "../include/pdh_ns_supg_heat.h"		/* USES */
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
// bc and material header files are included in problem header files
#include "../../pdd_heat/include/pdh_heat_bc.h"


#ifdef __cplusplus
extern "C" {
#endif

/**************************************/
/* GLOBAL CONSTANTS                   */
/**************************************/
/* Rules:
/* - constants always uppercase and start with PDC_ */

/* from pdh_inf.h */
const int PDC_ELEMENT = APC_ELEMENT;
const int PDC_FACE = APC_FACE;
const int PDC_EDGE = APC_EDGE;
const int PDC_VERTEX = APC_VERTEX;

const int PDC_NO_COMP = APC_NO_COMP;  /* do not compute stiff mat and rhs vect */
const int PDC_COMP_SM = APC_COMP_SM;  /* compute entries to stiff matrix only */
const int PDC_COMP_RHS = APC_COMP_RHS;/* compute entries to rhs vector only */
const int PDC_COMP_BOTH = APC_COMP_BOTH; /* compute entries for sm and rhsv */


/**************************************/
/* GLOBAL VARIABLES                   */
/**************************************/
/* Rules:
/* - name always begins with pdv_ */

/* time measurements */
double pdv_ns_supg_heat_timer_all = 0.0;
double pdv_ns_supg_heat_timer_pdr_comp_el_stiff_mat = 0.0;
double pdv_ns_supg_heat_timer_pdr_comp_fa_stiff_mat = 0.0;

// ID of the current problem
// on purpose initialized to 0 which is wrong value !
// later should be replaced by one of the two proper values:
// ns_supg -> problem_id = PDC_NS_SUPG_ID = 1
// heat -> problem_id = PDC_HEAT_ID = 2
int pdv_ns_supg_heat_current_problem_id = 0;	/* ID of the current problem */
// problem structure for ns_supg module
pdt_ns_supg_problem pdv_ns_supg_problem;
// problem structure for heat module
pdt_heat_problem pdv_heat_problem;


/***************************************/
/* DECLARATIONS OF INTERNAL PROCEDURES */
/***************************************/
/* Rules:
/* - name always begins with pdr_ns_supg_heat */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_ns_supg_heat_post_process - simple post-processing
------------------------------------------------------------*/
double pdr_ns_supg_heat_post_process(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/*------------------------------------------------------------
pdr_ns_supg_heat_profile - to dump a set of values along a line
------------------------------------------------------------*/
int pdr_ns_supg_heat_write_profile(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/*------------------------------------------------------------
pdr_heat_initial_condition - procedure passed as argument
  to field initialization routine in order to provide problem
  dependent initial condition data
------------------------------------------------------------*/
double pdr_heat_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Coor,   // point coordinates
  int Sol_comp_id // solution component
);

/*------------------------------------------------------------
pdr_ns_supg_initial_condition - procedure passed as argument
  to field initialization routine in order to provide problem
  dependent initial condition data
------------------------------------------------------------*/
double pdr_ns_supg_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Coor,   // point coordinates
  int Sol_comp_id // solution component
);

/*------------------------------------------------------------
 TODO: not implemented yet
------------------------------------------------------------*/
int  pdr_ns_supg_heat_bc_free();

/*------------------------------------------------------------
 TODO: not implemented yet
------------------------------------------------------------*/
int  pdr_ns_supg_heat_material_free();

/***************************************/
/* DEFINITIONS OF PROCEDURES */
/***************************************/

/*------------------------------------------------------------
main function
------------------------------------------------------------*/
int main(int argc, char **argv)
{
  FILE *interactive_input, *interactive_output;
  char work_dir[300];
  char interactive_input_name[300];
  char interactive_output_name[300], tmp[100];
  char c, cc, arg[300];
  int info, iaux;
  int field_id, mesh_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /****************************************/
  /* input/output settings                */
  /****************************************/

  if(argv[1]==NULL){
    strcpy(work_dir,".");
  } else {
    sprintf(work_dir,"%s",argv[1]);
  }

  utr_set_interactive(work_dir, argc, argv,
		      &interactive_input, &interactive_output);

  /****************************************/
  /* input/output settings - done         */
  /****************************************/

#ifdef DEBUG
  fprintf(interactive_output,"Starting program in debug mode.\n");
#endif
#ifdef DEBUG_MMM
  fprintf(interactive_output,"Starting mesh module in debug mode.\n");
#endif
#ifdef DEBUG_APM
  fprintf(interactive_output,"Starting approximation module in debug mode.\n");
#endif
#ifdef DEBUG_SIM
  fprintf(interactive_output,"Starting solver interface in debug mode.\n");
#endif
#ifdef DEBUG_LSM
  fprintf(interactive_output,"Starting linear solver (adapter) module in debug mode.\n");
#endif

  /******************************************************************/
  /* initialization of problem data (including mesh and two fields) */
  /******************************************************************/

  // initialize ns_supg and heat structures

  iaux=pdr_ns_supg_heat_init(work_dir,
			     interactive_input, interactive_output);
  if (iaux == EXIT_FAILURE) exit(-1);

  //printf("\n\tAS: heat_dtdt:\nfield_id : %d, mesh_id : %d, nr_sol : %d, name : %s, nreq : %d, solver_id : %d, ", pdv_heat_dtdt_problem.ctrl.field_id, pdv_heat_dtdt_problem.ctrl.mesh_id, pdv_heat_dtdt_problem.ctrl.nr_sol, pdv_heat_dtdt_problem.ctrl.name, pdv_heat_dtdt_problem.ctrl.nreq, pdv_heat_dtdt_problem.ctrl.solver_id);

  /****************************************/
  /* main menu loop                       */
  /****************************************/
  do {
#ifdef PARALLEL
    if (pcr_my_proc_id() == pcr_print_master()) {
#endif
      if (interactive_input == stdin) {
	do {
	  printf("\nChoose a command from the menu:\n");
	  printf("\tt - solve the problem (time integration)\n");
	  printf("\ts - solve the problem (time integration)\n");
	  printf("\te - compute error\n");
	  printf("\tp - postprocessing\n");
	  printf("\tg - launch graphic module\n");
	  printf("\tm - perform uniform mesh refinement \n");
	  printf("\ta - perform automatic mesh adaptation \n");
	  printf("\tr - print profile\n");
	  printf("\td - dump out data \n");
      printf("\tx - select format and export mesh\n");
	  printf("\tv - write ParaView graphics data \n");
	  printf("\tc - change control data \n");
	  printf("\tq - exit the program\n");
	  scanf(" %c", &c);
	}
	while (c != 's' && c != 't' && c != 'e' && c != 'p' && c != 'g' 
	        && c != 'm' && c != 'a' && c != 'r' && c != 'd' 
           && c != 'v' && c != 'c' && c != 'q' && c != 'q' && c !='x');
      } else {
	fscanf(interactive_input, "%c\n", &c);
      }
#ifdef PARALLEL
    }
    pcr_bcast_char(pcr_print_master(), 1, &c);
    //printf("After BCAST %c\n",c);
#endif

    /*------------------------------------------------------------*/
    if (c == 's' || c == 't') {

      utv_SIGINT_not_caught = 1;

      utr_io_result_set_filename(work_dir,"ns_supg_heat.csv");
      utr_io_result_clear_columns();
      utr_io_result_add_column(RESULT_STEP);
      utr_io_result_add_column(RESULT_CUR_TIME);
      utr_io_result_add_column(RESULT_DT);
      utr_io_result_add_column(RESULT_CFL_MIN);
      utr_io_result_add_column(RESULT_CFL_MAX);
      utr_io_result_add_column(RESULT_CFL_AVG);
      utr_io_result_add_column(RESULT_NON_LIN_STEP);
      utr_io_result_add_column(RESULT_N_DOFS);
      utr_io_result_add_column(RESULT_NORM_NS_SUPG);
      utr_io_result_add_column(RESULT_NORM_HEAT);
      utr_io_result_add_column(RESULT_OUT_FILENAME);
      utr_io_result_write_columns();

      pdv_ns_supg_heat_timer_all = 0.0;
      pdv_ns_supg_heat_timer_pdr_comp_el_stiff_mat = 0.0;
      pdv_ns_supg_heat_timer_pdr_comp_fa_stiff_mat = 0.0;

      mf_log_info("Beginning solution of coupled ns_supg-heat problem");

      pdv_ns_supg_heat_timer_all = time_clock();

      /*---------- main time integration procedure ------------*/
      pdr_ns_supg_heat_time_integration(work_dir, 
			   interactive_input, interactive_output);


      pdv_ns_supg_heat_timer_all = time_clock() - pdv_ns_supg_heat_timer_all;

      fprintf(interactive_output,"\nTime total: %lf\n", 
	      pdv_ns_supg_heat_timer_all);
      fprintf(interactive_output,"Time in pdr_comp_el_stiff_mat: %lf [%lf%%]\n", 
	      pdv_ns_supg_heat_timer_pdr_comp_el_stiff_mat, 
	      (pdv_ns_supg_heat_timer_pdr_comp_el_stiff_mat 
	       / pdv_ns_supg_heat_timer_all) * 100);
      fprintf(interactive_output,"Time in pdr_comp_fa_stiff_mat: %lf [%lf%%]\n", 
	      pdv_ns_supg_heat_timer_pdr_comp_fa_stiff_mat, 
	      (pdv_ns_supg_heat_timer_pdr_comp_fa_stiff_mat 
	       / pdv_ns_supg_heat_timer_all) * 100);

    }
    /*------------------------------------------------------------*/
    else if (c == 'e') {

      pdr_ns_supg_heat_ZZ_error(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'p') {

      pdr_ns_supg_heat_post_process(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'g') {
    	c = '\0';
    	init_mod_fem_viewer(argc,argv,interactive_output);
    }
    /*------------------------------------------------------------*/
    else if (c == 'm') {

      // perform manual mesh refinement using ns_supg as master problem 
      pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
      int ref_type = -1;
      pdr_ns_supg_heat_refine(pdv_ns_supg_heat_current_problem_id, ref_type,
			    interactive_output);
    }
    /*------------------------------------------------------------*/
    else if (c == 'a') {

      pdr_ns_supg_heat_adapt(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'r') {

      pdr_ns_supg_heat_write_profile(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'd') {

      pdr_ns_supg_heat_dump_data(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'x') {

        utr_menu_export_mesh(interactive_output,
                             interactive_input,
                             pdv_ns_supg_problem.ctrl.mesh_id,
                             work_dir,
                             pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern);

    }
    /*------------------------------------------------------------*/
    else if (c == 'v') {

      pdr_ns_supg_heat_write_paraview(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'c') {

      //fprintf(interactive_output, "\nChanging of control data not yet implemented.\n");
      fprintf(interactive_output, "\nSet new time step: ");
      fscanf(interactive_input, "%lf", &pdv_heat_problem.time.cur_dtime);
      pdv_ns_supg_problem.time.cur_dtime = pdv_heat_problem.time.cur_dtime;


    }
  }
  while (c != 'q');

  /* free allocated space */
  pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
  iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_current_problem_id,iaux);
  apr_free_field(field_id);

  /* pdv_ns_supg_heat_current_problem_id = PDC_HEAT_DTDT_ID; */
  /* iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_current_problem_id,iaux); */
  /* apr_free_field(field_id); */

  pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
  iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_current_problem_id,iaux);
  apr_free_field(field_id);

  mesh_id = apr_get_mesh_id(field_id);
  mmr_free_mesh(mesh_id);

  fclose(interactive_input);
  fclose(interactive_output);

  pdr_ns_supg_heat_bc_free();
  pdr_ns_supg_heat_material_free();

#ifdef PARALLEL
  pcr_exit_parallel();
#endif

  return (0);
}




/*------------------------------------------------------------
pdr_ns_supg_initial_condition
------------------------------------------------------------*/
double pdr_ns_supg_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Xcoor,   // point coordinates
  int Sol_comp_id // solution component
)
{

  return (0.0);
}

/*------------------------------------------------------------
pdr_heat_initial_condition
------------------------------------------------------------*/
double pdr_heat_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Xcoor,   // point coordinates
  int Sol_comp_id // solution component
)
{

/*kbw
  printf("specified initial temperature at point %lf, %lf, %lf : %lf\n",
	 Xcoor[0], Xcoor[1], Xcoor[2], 
	 pdv_heat_problem.ctrl.ambient_temperature); 
/*kew*/
/*kbw
  if(fabs(Xcoor[0]-0.6)<0.25 && fabs(Xcoor[1]-0.6)<0.25){
    return(1000.0);
  }
  else{
    return (pdv_heat_problem.ctrl.ambient_temperature);
  }
/*kew*/
    return (pdv_heat_problem.ctrl.ambient_temperature);
}

/*------------------------------------------------------------
pdr_ns_supg_heat_post_process
------------------------------------------------------------*/
double pdr_ns_supg_heat_post_process(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output)
{
  double x[3], xg[3];
  int pdeg;			/* degree of polynomial */
  int base;		/* type of basis functions for quadrilaterals */
  int num_shap;			/* number of element shape functions */
  int ndofs;			/* local dimension of the problem */
  double xcoor[3];		/* global coord of gauss point */
  double u_val[PDC_MAXEQ];	/* computed solution */
  double u_x[PDC_MAXEQ];		/* gradient of computed solution */
  double u_y[PDC_MAXEQ];		/* gradient of computed solution */
  double u_z[PDC_MAXEQ];		/* gradient of computed solution */
  double base_phi[APC_MAXELVD];	/* basis functions */
  double base_dphix[APC_MAXELVD];	/* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];	/* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];	/* y-derivatives of basis function */
  int el_nodes[MMC_MAXELVNO + 1];	/* list of nodes of El */
  double node_coor[3 * MMC_MAXELVNO];	/* coord of nodes of El */
  double dofs_loc[APC_MAXELSD];	/* element solution dofs */
  double dofs_loc2[APC_MAXELSD];	/* element solution dofs */
  int i, j, iel, ki, iaux, name, mat_num, nel, sol_vec_id, nreq;
  int list_el[20];
  int problem_id, field_id, mesh_id;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  fprintf(Interactive_output, "Select problem: 1 - ns_supg, 2 - heat:\n");
  fscanf(Interactive_input, "%d", &problem_id);
  
  i=3; field_id = pdr_ctrl_i_params(problem_id, i);
  /* select the corresponding mesh */
  mesh_id = apr_get_mesh_id(field_id);
  i=5; nreq = pdr_ctrl_i_params(problem_id, i);
  
  fprintf(Interactive_output, "Give global coordinates of a point (x,y,z):\n");
  fscanf(Interactive_input, "%lf", &x[0]);
  fscanf(Interactive_input, "%lf", &x[1]);
  fscanf(Interactive_input, "%lf", &x[2]);
  fprintf(Interactive_output, "x=%lf,y=%lf,z=%lf\n",x[0],x[1],x[2]);
  
  iaux = apr_sol_xglob(field_id, x, 1, list_el, xg, u_val, NULL, NULL, NULL,0);
  if(iaux==1){
    fprintf(Interactive_output, "\nSolution at point %.2lf %.2lf %.2lf in element %d:\n\n", xg[0], xg[1], xg[2], list_el[1]);
    for (j = 0; j < nreq; j++){
      fprintf(Interactive_output, "u_val[%d]=%lf\n", j, u_val[j]);
    }
  }
  else{ 
    printf("Local coordinates not found within a family in apr_sol_xglob\n");
  }
  
  return (0);
  
  fprintf(Interactive_output, "Give element number:\n");
  fscanf(Interactive_input, "%d", &nel);
  fprintf(Interactive_output, "Give local coordinates of a point (x,y,z):\n");
  fscanf(Interactive_input, "%lf", &x[0]);
  fscanf(Interactive_input, "%lf", &x[1]);
  fscanf(Interactive_input, "%lf", &x[2]);
  //fprintf(Interactive_output, "nel=%d, x=%lf, y=%lf, z=%lf\n",
  //        nel,x[0],x[1],x[2]);
    base = apr_get_base_type(field_id, nel);
    pdeg = apr_get_el_pdeg(field_id, nel, &pdeg);
    num_shap = apr_get_el_pdeg_numshap(field_id, nel, &pdeg);
    i=5; nreq = pdr_ctrl_i_params(problem_id, i);
    ndofs = nreq * num_shap;
    /* get the coordinates of the nodes of El in the right order */
    mmr_el_node_coor(mesh_id, nel, el_nodes, node_coor);
    /* get the most recent solution degrees of freedom */
    sol_vec_id = 1;
    for (j = 0; j < nreq; j++) {
      for (i = 0; i < el_nodes[0]; i++) {
	apr_read_ent_dofs(field_id, APC_VERTEX, el_nodes[i + 1], nreq, sol_vec_id, dofs_loc);
	dofs_loc2[j * num_shap + i] = dofs_loc[j];
      }
    }
    /* calculations with jacobian but not on the boundary */
    iaux = 2;
    apr_elem_calc_3D(iaux, nreq, &pdeg, base, x, node_coor, dofs_loc2, 
		     base_phi, base_dphix, base_dphiy, base_dphiz, 
		     xcoor, u_val, u_x, u_y, u_z, NULL);
    fprintf(Interactive_output, 
	    "\nSolution at point %.2lf %.2lf %.2lf in element %d:\n\n", 
	    x[0], x[1], x[2], nel);
    for (j = 0; j < nreq; j++){
      fprintf(Interactive_output, "u_val[%d]=%lf\n", j, u_val[j]);
    }
    fprintf(Interactive_output, 
	    "\nGlobal coordinates of the point:  %.2lf %.2lf %.2lf:\n\n",
	    xcoor[0], xcoor[1], xcoor[2]);
  
  return (1);
}

/*------------------------------------------------------------
pdr_ns_supg_heat_write_profile
------------------------------------------------------------*/
int pdr_ns_supg_heat_write_profile(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
  )
{
  double x1[3], x2[3];
  int solNr; // solution component ID
  int nSol; // number of solution components
  int nPoints; // number of points along the line
  int problem_id, field_id, mesh_id, i;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  fprintf(Interactive_output, "Select problem: 1 - ns_supg, 2 - heat:\n");
  fscanf(Interactive_input, "%d", &problem_id);
  
  pdv_ns_supg_heat_current_problem_id = problem_id;
  i=3; field_id = pdr_ctrl_i_params(problem_id, i);
  /* select the corresponding mesh */
  mesh_id = apr_get_mesh_id(field_id);
  //i=5; nSol = pdr_ctrl_i_params(problem_id, i);
  nSol = apr_get_nreq(field_id);

  fprintf(Interactive_output, "Give solution component number (>=0):\n");
  fscanf(Interactive_input, "%d", &solNr);
  fprintf(Interactive_output, "Give number of points (>0):\n");
  fscanf(Interactive_input, "%d", &nPoints);
  fprintf(Interactive_output, "Give global coordinates of a point1 (x,y,z):\n");
  fscanf(Interactive_input, "%lf", &x1[0]);
  fscanf(Interactive_input, "%lf", &x1[1]);
  fscanf(Interactive_input, "%lf", &x1[2]);
  fprintf(Interactive_output, "Give global coordinates of a point2 (x,y,z):\n");
  fscanf(Interactive_input, "%lf", &x2[0]);
  fscanf(Interactive_input, "%lf", &x2[1]);
  fscanf(Interactive_input, "%lf", &x2[2]);
  apr_get_profile(Interactive_output, field_id, solNr, nSol, x1, x2, nPoints);

  return 1;
}

/*---------------------------------------------------------
pdr_err_indi - to return error indicator for an element
----------------------------------------------------------*/
double pdr_err_indi(		/* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int Mode,	/* in: mode of operation */
  int El	/* in: element number */
    )
{

  if (Mode == PDC_ADAPT_EXPL) {
    
    return pdr_ns_supg_heat_err_indi_explicit(Problem_id, El);
    
  } else if (Mode == PDC_ADAPT_ZZ) {
    
    return pdr_ns_supg_heat_err_indi_ZZ(Problem_id, El);
    
  } else {
    
    printf("Unknown error indicator in pdr_err_indi!\n");
    
  }
  
  return (0.0);
}

/*------------------------------------------------------------
  pdr_get_problem_structure - to return pointer to problem structure
------------------------------------------------------------*/
void* pdr_get_problem_structure(int Problem_id)
{
  if(Problem_id==PDC_NS_SUPG_ID){
    return (&pdv_ns_supg_problem);
  } else if(Problem_id==PDC_HEAT_ID){
    return (&pdv_heat_problem);
  } else{
    printf("Wrong problem_id in pdr_get_problem_structure!");
    exit(1);
  }
  
  return (NULL);
}

/*------------------------------------------------------------
pdr_ctrl_i_params - to return one of control parameters
------------------------------------------------------------*/
int pdr_ctrl_i_params(int Problem_id, int Num)
{

  pdt_heat_ctrls *ctrl_heat = &pdv_heat_problem.ctrl;
  pdt_ns_supg_ctrls *ctrl_ns_supg = &pdv_ns_supg_problem.ctrl;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    
    if (Num == 1) {
      return (ctrl_ns_supg->name);
    } else if (Num == 2) {
      return (ctrl_ns_supg->mesh_id);
    } else if (Num == 3) {
      return (ctrl_ns_supg->field_id);
    } else if (Num == 4) {
      return (ctrl_ns_supg->nr_sol);
    } else if (Num == 5) {
      return (ctrl_ns_supg->nreq);
    } else if (Num == 6) {
      return (ctrl_ns_supg->solver_id);
    } else {
      
    }
  } else if(Problem_id==PDC_HEAT_ID) {
    
    if (Num == 1) {
      return (ctrl_heat->name);
    } else if (Num == 2) {
      return (ctrl_heat->mesh_id);
    } else if (Num == 3) {
      return (ctrl_heat->field_id);
    } else if (Num == 4) {
      return (ctrl_heat->nr_sol);
    } else if (Num == 5) {
      return (ctrl_heat->nreq);
    } else if (Num == 6) {
      return (ctrl_heat->solver_id);
    } else {
      
    }
    
    
  }
  
  return (-1);
}

/*------------------------------------------------------------
pdr_ctrl_d_params - to return one of control parameters
------------------------------------------------------------*/
double pdr_ctrl_d_params(int Problem_id, int Num)
{
  pdt_heat_ctrls *ctrl_heat = &pdv_heat_problem.ctrl;
  pdt_ns_supg_ctrls *ctrl_ns_supg = &pdv_ns_supg_problem.ctrl;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
 
    if (Num == 10) {
      return (ctrl_ns_supg->gravity_field[0]);
    } else if (Num == 11) {
      return (ctrl_ns_supg->gravity_field[1]);
    } else if (Num == 12) {
      return (ctrl_ns_supg->gravity_field[2]);
    } else if (Num == 20) {
      return (ctrl_ns_supg->ref_temperature);
    }
  } else if(Problem_id==PDC_HEAT_ID){
    if (Num == 20) {
      return (ctrl_heat->ref_temperature);
    }
  }

  return(0.0);
}

/*------------------------------------------------------------
pdr_adapt_i_params - to return parameters of adaptation
------------------------------------------------------------*/
int pdr_adapt_i_params(int Problem_id, int Num)
{

  pdt_heat_adpts *adpts_heat = &pdv_heat_problem.adpt;
  pdt_ns_supg_adpts *adpts_ns_supg = &pdv_ns_supg_problem.adpt;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    if (Num == 1)
      return (adpts_ns_supg->type);
    else if (Num == 2)
      return (adpts_ns_supg->interval);
    else if (Num == 3)
      return (adpts_ns_supg->maxgen);
    else if (Num == 7)
      return (adpts_ns_supg->monitor);
    else {
      printf("Wrong parameter number in adapt_i_params!");
      exit(1);
    }
  } else if(Problem_id==PDC_HEAT_ID){
    if (Num == 1)
      return (adpts_heat->type);
    else if (Num == 2)
      return (adpts_heat->interval);
    else if (Num == 3)
      return (adpts_heat->maxgen);
    else if (Num == 7)
      return (adpts_heat->monitor);
    else {
      printf("Wrong parameter number in adapt_i_params!");
      exit(1);
    }
  } else{
    printf("Wrong problem_id in adapt_i_params!");
    exit(1);
  }
  

  return (-1);
}


/*------------------------------------------------------------
pdr_adapt_d_params - to return parameters of adaptation
------------------------------------------------------------*/
double pdr_adapt_d_params(int Problem_id, int Num)
{


  pdt_heat_adpts *adpts_heat = &pdv_heat_problem.adpt;
  pdt_ns_supg_adpts *adpts_ns_supg = &pdv_ns_supg_problem.adpt;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    if (Num == 5)
      return (adpts_ns_supg->eps);
    else if (Num == 6)
      return (adpts_ns_supg->ratio);
    else {
      printf("Wrong parameter number in adapt_d_params!");
      exit(1);
    }
  } else if(Problem_id==PDC_HEAT_ID){
    if (Num == 5)
      return (adpts_heat->eps);
    else if (Num == 6)
      return (adpts_heat->ratio);
    else {
      printf("Wrong parameter number in adapt_d_params!");
      exit(1);
    }
  } else{
    printf("Wrong problem_id in adapt_i_params!");
    exit(1);
  }

  return (-1);
}

/*------------------------------------------------------------
pdr_time_i_params - to return parameters of timeation
------------------------------------------------------------*/
int pdr_time_i_params(int Problem_id, int Num)
{

  pdt_heat_times *times_heat = &pdv_heat_problem.time;
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    if (Num == 1)
      return (times_ns_supg->type);
    else if (Num == 3)
      return (times_ns_supg->cur_step);
    else if (Num == 4)
      return (times_ns_supg->final_step);
    else if (Num == 9)
      return (times_ns_supg->conv_type);
    else if (Num == 11)
      return (times_ns_supg->monitor);
    else {
      printf("Wrong parameter number in time_i_params!");
      exit(1);
    }
  } else if(Problem_id==PDC_HEAT_ID){
    if (Num == 1)
      return (times_heat->type);
    else if (Num == 3)
      return (times_heat->cur_step);
    else if (Num == 4)
      return (times_heat->final_step);
    else if (Num == 9)
      return (times_heat->conv_type);
    else if (Num == 11)
      return (times_heat->monitor);
    else {
      printf("Wrong parameter number in time_i_params!");
      exit(1);
    }
  } else{
    printf("Wrong problem_id in time_i_params!");
    exit(1);
  }
  

  return (-1);
}


/*------------------------------------------------------------
pdr_time_d_params - to return parameters of timeation
------------------------------------------------------------*/
double pdr_time_d_params(int Problem_id, int Num)
{


  pdt_heat_times *times_heat = &pdv_heat_problem.time;
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    if (Num == 2)
      return (times_ns_supg->alpha);
    else {
      printf("Wrong parameter number in time_d_params!");
      exit(1);
    }
  } else if(Problem_id==PDC_HEAT_ID){
    if (Num == 2)
      return (times_heat->alpha);
    else {
      printf("Wrong parameter number in time_d_params!");
      exit(1);
    }
  } else{
    printf("Wrong problem_id in time_i_params!");
    exit(1);
  }

  return (-1);
}

/*---------------------------------------------------------
pdr_set_time_i_params - to change parameters of time discretization
---------------------------------------------------------*/
void pdr_set_time_i_params( 
        int Problem_id,	     /* in: data structure to be used  */
	int Num,             /* in: parameter number in control structure */
	int Value            /* in: parameter value */
	)
{
/* auxiliary variables */
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;
  pdt_heat_times *times_heat = &pdv_heat_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  if(Problem_id==PDC_NS_SUPG_ID){

    if(Num==1) times_ns_supg->type=Value;
    else if(Num==2) times_ns_supg->cur_step=Value;
    else if(Num==3) times_ns_supg->final_step=Value;
    else if(Num==8) times_ns_supg->conv_type=Value;
    else if(Num==10) times_ns_supg->monitor=Value;
    else if(Num==11) times_ns_supg->intv_dumpout=Value;
    else if(Num==12) times_ns_supg->intv_graph=Value;
    else {
      printf("Wrong parameter number in set_time_i_params!");
      exit(1);
    }

  }
  else if(Problem_id==PDC_HEAT_ID){

    if(Num==1) times_heat->type=Value;
    else if(Num==2) times_heat->cur_step=Value;
    else if(Num==3) times_heat->final_step=Value;
    else if(Num==8) times_heat->conv_type=Value;
    else if(Num==10) times_heat->monitor=Value;
    else if(Num==11) times_heat->intv_dumpout=Value;
    else if(Num==12) times_heat->intv_graph=Value;
    else {
      printf("Wrong parameter number in set_time_i_params!");
      exit(1);
    }

  }

  return;
}

/*---------------------------------------------------------
pdr_set_time_d_params - to change parameters of time discretization
---------------------------------------------------------*/
void pdr_set_time_d_params( 
        int Problem_id,	     /* in: data structure to be used  */
	int Num,             /* in: parameter number in control structure */
	double Value         /* in: parameter value */
	)
{
/* auxiliary variables */
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;
  pdt_heat_times *times_heat = &pdv_heat_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  if(Problem_id==PDC_NS_SUPG_ID){

    if(Num==4) times_ns_supg->cur_time=Value;
    else if(Num==5) times_ns_supg->final_time=Value;
    else if(Num==6) times_ns_supg->cur_dtime=Value;
    else if(Num==7) times_ns_supg->prev_dtime=Value;
    else if(Num==9) times_ns_supg->conv_meas=Value;
    else {
      printf("Wrong parameter number in set_time_d_params!");
      exit(1);
    }

  }
  else if(Problem_id==PDC_HEAT_ID){

    if(Num==4) times_heat->cur_time=Value;
    else if(Num==5) times_heat->final_time=Value;
    else if(Num==6) times_heat->cur_dtime=Value;
    else if(Num==7) times_heat->prev_dtime=Value;
    else if(Num==9) times_heat->conv_meas=Value;
    else {
      printf("Wrong parameter number in set_time_d_params!");
      exit(1);
    }

  }

  return;
}


/*---------------------------------------------------------
pdr_lins_i_params - to return parameters of linear equations solver
---------------------------------------------------------*/
int pdr_lins_i_params( /* returns: integer linear solver parameter */
	int Problem_id,	/* in: data structure to be used  */
	int Num         /* in: parameter number in control structure */
	)
{
/* auxiliary variables */
  pdt_ns_supg_linss *linss_ns_supg = &pdv_ns_supg_problem.lins;
  pdt_heat_linss *linss_heat = &pdv_heat_problem.lins;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  if(Problem_id==PDC_NS_SUPG_ID){

    if(Num==1) return(linss_ns_supg->type);
    else if(Num==2) return(linss_ns_supg->max_iter);
    else if(Num==3) return(linss_ns_supg->conv_type);
    else if(Num==5) return(linss_ns_supg->monitor);
    else {
      printf("Wrong parameter number in lins_i_params!");
      exit(-1);
    }

  }
  else if(Problem_id==PDC_HEAT_ID){

    if(Num==1) return(linss_heat->type);
    else if(Num==2) return(linss_heat->max_iter);
    else if(Num==3) return(linss_heat->conv_type);
    else if(Num==5) return(linss_heat->monitor);
    else {
      printf("Wrong parameter number in lins_i_params!");
      exit(-1);
    }

  }

/* error condition - that point should not be reached */
  return(-1);
}

/*---------------------------------------------------------
pdr_lins_d_params - to return parameters of linear equations solver
---------------------------------------------------------*/
double pdr_lins_d_params( /* returns: real linear solver parameter */
	int Problem_id,	/* in: data structure to be used  */
	int Num         /* in: parameter number in control structure */
	)
{
/* auxiliary variables */
  pdt_ns_supg_linss *linss_ns_supg = &pdv_ns_supg_problem.lins;
  pdt_heat_linss *linss_heat = &pdv_heat_problem.lins;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  if(Problem_id==PDC_NS_SUPG_ID){


    if(Num==4) return(linss_ns_supg->conv_meas);
    else {
      printf("Wrong parameter number in lins_d_params!");
      exit(-1);
    }
    
  }
  else if(Problem_id==PDC_HEAT_ID){


    if(Num==4) return(linss_heat->conv_meas);
    else {
      printf("Wrong parameter number in lins_d_params!");
      exit(-1);
    }
    
  }

/* error condition - that point should not be reached */
  return(-1);
}


/*---------------------------------------------------------
pdr_change_data - to change some of control data 
---------------------------------------------------------*/
void pdr_change_data(
	int Problem_id	/* in: data structure to be used  */
        )
{

  /******************************* WARNING ******************************/
  /* in its current version the procedure changes ns_supg and heat data */
  /* separately - consistency must be ensured by the user, e.g.         */
  /* ns_supg->dtime must be equal heat->dtime                           */

/* local variables */
  pdt_ns_supg_adpts *adpts_ns_supg = &pdv_ns_supg_problem.adpt;
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;
  pdt_ns_supg_linss *linss_ns_supg = &pdv_ns_supg_problem.lins;
  pdt_ns_supg_ctrls *ctrls_ns_supg = &pdv_ns_supg_problem.ctrl;
  pdt_heat_adpts *adpts_heat = &pdv_heat_problem.adpt;
  pdt_heat_times *times_heat = &pdv_heat_problem.time;
  pdt_heat_linss *linss_heat = &pdv_heat_problem.lins;
  pdt_heat_ctrls *ctrls_heat = &pdv_heat_problem.ctrl;
  char c, d, pans[100]; /* string variable to read menu */

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  if(Problem_id==PDC_NS_SUPG_ID){

    do {

      do {
/* define a menu */
        printf("\nChoose a group of data:\n");
        printf("\tc - general control data \n"); 
        printf("\tt - time integration parameters \n"); 
        printf("\ta - adaptation parameters \n"); 
        printf("\tl - linear solver parameters \n"); 
        printf("\tq - quit changing data for problem %d\n",Problem_id);

        scanf("%s",pans);getchar();
      } while ( *pans != 'c' && *pans != 't' && *pans != 'a' 
             && *pans != 'l' && *pans != 'q' && *pans != 'q' );

      c = *pans;

      if(c=='c'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\tv - dynamic viscosity\n"); 
            printf("\tq - quit changing general control data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 's' && *pans != 's' && *pans != 's' 
               && *pans != 'l' && *pans != 'c' && *pans != 'q' );

          d = *pans;

	  if(d=='v'){

            printf("Old value: %lf, new value: ", 
		   ctrls_ns_supg->dynamic_viscosity);
            scanf("%lg",&ctrls_ns_supg->dynamic_viscosity); getchar();

          }

        } while(d != 'q');

      }
      else if(c=='t'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\ta - method identifier\n"); 
            printf("\tc - current time-step number\n"); 
            printf("\td - final time-step number\n"); 
            printf("\te - current time-step length\n"); 
            printf("\tf - previous time-step length\n"); 
            printf("\tg - current time\n"); 
            printf("\th - final time\n"); 
            printf("\ti - convergence in time criterion number\n"); 
            printf("\tj - convergence in time treshold value\n"); 
            printf("\tk - implicitness parameter alpha (theta)\n"); 
            printf("\tq - quit changing time integration data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 'a' && *pans != 'c' && *pans != 'c' 
                 && *pans != 'd' && *pans != 'e' && *pans != 'f' 
                 && *pans != 'g' && *pans != 'h' && *pans != 'i' 
                 && *pans != 'j' && *pans != 'k' && *pans != 'q' );

          d = *pans;

          if(d=='a'){

            printf("Old value: %d, new value: ",times_ns_supg->type);
            scanf("%d",&times_ns_supg->type); getchar();

          }
          else if(d=='c'){

            printf("Old value: %d, new value: ",times_ns_supg->cur_step);
            scanf("%d",&times_ns_supg->cur_step); getchar();

          }
          else if(d=='d'){

            printf("Old value: %d, new value: ",times_ns_supg->final_step);
            scanf("%d",&times_ns_supg->final_step); getchar();

          }
          else if(d=='e'){

            printf("Old value: %lg, new value: ",times_ns_supg->cur_dtime);
            scanf("%lg",&times_ns_supg->cur_dtime); getchar();

          }
          else if(d=='f'){

            printf("Old value: %lg, new value: ",times_ns_supg->prev_dtime);
            scanf("%lg",&times_ns_supg->prev_dtime); getchar();

          }
          else if(d=='g'){

            printf("Old value: %lg, new value: ",times_ns_supg->cur_time);
            scanf("%lg",&times_ns_supg->cur_time); getchar();

          }
          else if(d=='h'){

            printf("Old value: %lg, new value: ",times_ns_supg->final_time);
            scanf("%lg",&times_ns_supg->final_time); getchar();

          }
          else if(d=='i'){

            printf("Old value: %d, new value: ",times_ns_supg->conv_type);
            scanf("%d",&times_ns_supg->conv_type); getchar();

          }
          else if(d=='j'){

            printf("Old value: %lg, new value: ",times_ns_supg->conv_meas);
            scanf("%lg",&times_ns_supg->conv_meas); getchar();

          }
          else if(d=='k'){

            printf("Old value: %lg, new value: ", 
		   times_ns_supg->alpha);
            scanf("%lg",&times_ns_supg->alpha); getchar();

          }

        } while(d != 'q');

      }
      else if(c=='a'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\tt - strategy number\n"); 
            printf("\ti - time interval between adaptations\n"); 
            printf("\tm - maximal generation level for elements\n"); 
            printf("\te - global treshold value for adaptation\n"); 
            printf("\tr - ratio for indicating derefinements\n"); 
            printf("\tq - quit changing adaptation data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 't' && *pans != 'i' && *pans != 'm' && *pans != 'd'
               && *pans != 'e' && *pans != 'r' && *pans != 'q' );

          d = *pans;

          if(d=='t'){

            printf("Old value: %d, new value: ",adpts_ns_supg->type);
            scanf("%d",&adpts_ns_supg->type); getchar();

          }
          else if(d=='i'){

            printf("Old value: %d, new value: ",adpts_ns_supg->interval);
            scanf("%d",&adpts_ns_supg->interval); getchar();

          }
          else if(d=='m'){

            printf("Old value: %d, new value: ",adpts_ns_supg->maxgen);
            scanf("%d",&adpts_ns_supg->maxgen); getchar();

          }
          else if(d=='e'){

            printf("Old value: %lg, new value: ",adpts_ns_supg->eps);
            scanf("%lg",&adpts_ns_supg->eps); getchar();

          }
          else if(d=='r'){

            printf("Old value: %lg, new value: ",adpts_ns_supg->ratio);
            scanf("%lg",&adpts_ns_supg->ratio); getchar();

          }

        } while(d != 'q');

      }

      else if(c=='l'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\tt - solver type\n"); 
            printf("\ti - maximal number of iterations\n"); 
            printf("\tc - convergence criterion number\n"); 
            printf("\te - convergence treshold value\n"); 
            printf("\tm - monitoring level\n"); 
            printf("\tq - quit changing linear solver data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 't' && *pans != 'm' && *pans != 'c' 
                 && *pans != 'e' && *pans != 'p' && *pans != 'k' 
                 && *pans != 'b' && *pans != 'q' && *pans != 'q' );

          d = *pans;

          if(d=='t'){

            printf("Old value: %d, new value: ",linss_ns_supg->type);
            scanf("%d",&linss_ns_supg->type); getchar();

          }
          else if(d=='i'){

            printf("Old value: %d, new value: ",linss_ns_supg->max_iter);
            scanf("%d",&linss_ns_supg->max_iter); getchar();

          }
          else if(d=='c'){

            printf("Old value: %d, new value: ",linss_ns_supg->conv_type);
            scanf("%d",&linss_ns_supg->conv_type); getchar();

          }
          else if(d=='e'){

            printf("Old value: %lg, new value: ",linss_ns_supg->conv_meas);
            scanf("%lg",&linss_ns_supg->conv_meas); getchar();

          }
          else if(d=='m'){

            printf("Old value: %d, new value: ",linss_ns_supg->monitor);
            scanf("%d",&linss_ns_supg->monitor); getchar();

          }

        } while(d != 'q');

      }

    } while(c != 'q');

  }
/* select the proper problem data structure */
  else if(Problem_id==PDC_HEAT_ID){

    do {

      do {
/* define a menu */
        printf("\nChoose a group of data:\n");
        printf("\tc - general control data \n"); 
        printf("\tt - time integration parameters \n"); 
        printf("\ta - adaptation parameters \n"); 
        printf("\tl - linear solver parameters \n"); 
        printf("\tq - quit changing data for problem %d\n",Problem_id);

        scanf("%s",pans);getchar();
      } while ( *pans != 'c' && *pans != 't' && *pans != 'a' 
             && *pans != 'l' && *pans != 'q' && *pans != 'q' );

      c = *pans;

      if(c=='c'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\tv - \n"); 
            printf("\tq - quit changing general control data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 'v' && *pans != 'v' && *pans != 'v' 
               && *pans != 'v' && *pans != 'v' && *pans != 'q' );

          d = *pans;

	  if(d=='v'){

            //printf("Old value: %lf, new value: ", 
	    //	   ctrls_heat->);
            //scanf("%lg",&ctrls_heat->); getchar();

          }

        } while(d != 'q');

      }
      else if(c=='t'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\ta - method identifier\n"); 
            printf("\tc - current time-step number\n"); 
            printf("\td - final time-step number\n"); 
            printf("\te - current time-step length\n"); 
            printf("\tf - previous time-step length\n"); 
            printf("\tg - current time\n"); 
            printf("\th - final time\n"); 
            printf("\ti - convergence in time criterion number\n"); 
            printf("\tj - convergence in time treshold value\n"); 
            printf("\tk - implicitness parameter alpha (theta)\n"); 
            printf("\tq - quit changing time integration data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 'a' && *pans != 'c' && *pans != 'c' 
                 && *pans != 'd' && *pans != 'e' && *pans != 'f' 
                 && *pans != 'g' && *pans != 'h' && *pans != 'i' 
                 && *pans != 'j' && *pans != 'k' && *pans != 'q' );

          d = *pans;

          if(d=='a'){

            printf("Old value: %d, new value: ",times_heat->type);
            scanf("%d",&times_heat->type); getchar();

          }
          else if(d=='c'){

            printf("Old value: %d, new value: ",times_heat->cur_step);
            scanf("%d",&times_heat->cur_step); getchar();

          }
          else if(d=='d'){

            printf("Old value: %d, new value: ",times_heat->final_step);
            scanf("%d",&times_heat->final_step); getchar();

          }
          else if(d=='e'){

            printf("Old value: %lg, new value: ",times_heat->cur_dtime);
            scanf("%lg",&times_heat->cur_dtime); getchar();

          }
          else if(d=='f'){

            printf("Old value: %lg, new value: ",times_heat->prev_dtime);
            scanf("%lg",&times_heat->prev_dtime); getchar();

          }
          else if(d=='g'){

            printf("Old value: %lg, new value: ",times_heat->cur_time);
            scanf("%lg",&times_heat->cur_time); getchar();

          }
          else if(d=='h'){

            printf("Old value: %lg, new value: ",times_heat->final_time);
            scanf("%lg",&times_heat->final_time); getchar();

          }
          else if(d=='i'){

            printf("Old value: %d, new value: ",times_heat->conv_type);
            scanf("%d",&times_heat->conv_type); getchar();

          }
          else if(d=='j'){

            printf("Old value: %lg, new value: ",times_heat->conv_meas);
            scanf("%lg",&times_heat->conv_meas); getchar();

          }
          else if(d=='k'){

            printf("Old value: %lg, new value: ", 
		   times_heat->alpha);
            scanf("%lg",&times_heat->alpha); getchar();

          }

        } while(d != 'q');

      }
      else if(c=='a'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\tt - strategy number\n"); 
            printf("\ti - time interval between adaptations\n"); 
            printf("\tm - maximal generation level for elements\n"); 
            printf("\te - global treshold value for adaptation\n"); 
            printf("\tr - ratio for indicating derefinements\n"); 
            printf("\tq - quit changing adaptation data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 't' && *pans != 'i' && *pans != 'm' && *pans != 'd'
               && *pans != 'e' && *pans != 'r' && *pans != 'q' );

          d = *pans;

          if(d=='t'){

            printf("Old value: %d, new value: ",adpts_heat->type);
            scanf("%d",&adpts_heat->type); getchar();

          }
          else if(d=='i'){

            printf("Old value: %d, new value: ",adpts_heat->interval);
            scanf("%d",&adpts_heat->interval); getchar();

          }
          else if(d=='m'){

            printf("Old value: %d, new value: ",adpts_heat->maxgen);
            scanf("%d",&adpts_heat->maxgen); getchar();

          }
          else if(d=='e'){

            printf("Old value: %lg, new value: ",adpts_heat->eps);
            scanf("%lg",&adpts_heat->eps); getchar();

          }
          else if(d=='r'){

            printf("Old value: %lg, new value: ",adpts_heat->ratio);
            scanf("%lg",&adpts_heat->ratio); getchar();

          }

        } while(d != 'q');

      }

      else if(c=='l'){

        do {

          do {
/* define a menu */
            printf("\nChoose variable to change:\n");
            printf("\tt - solver type\n"); 
            printf("\ti - maximal number of iterations\n"); 
            printf("\tc - convergence criterion number\n"); 
            printf("\te - convergence treshold value\n"); 
            printf("\tm - monitoring level\n"); 
            printf("\tq - quit changing linear solver data\n");

            scanf("%s",pans);getchar();
          } while ( *pans != 't' && *pans != 'm' && *pans != 'c' 
                 && *pans != 'e' && *pans != 'p' && *pans != 'k' 
                 && *pans != 'b' && *pans != 'q' && *pans != 'q' );

          d = *pans;

          if(d=='t'){

            printf("Old value: %d, new value: ",linss_heat->type);
            scanf("%d",&linss_heat->type); getchar();

          }
          else if(d=='i'){

            printf("Old value: %d, new value: ",linss_heat->max_iter);
            scanf("%d",&linss_heat->max_iter); getchar();

          }
          else if(d=='c'){

            printf("Old value: %d, new value: ",linss_heat->conv_type);
            scanf("%d",&linss_heat->conv_type); getchar();

          }
          else if(d=='e'){

            printf("Old value: %lg, new value: ",linss_heat->conv_meas);
            scanf("%lg",&linss_heat->conv_meas); getchar();

          }
          else if(d=='m'){

            printf("Old value: %d, new value: ",linss_heat->monitor);
            scanf("%d",&linss_heat->monitor); getchar();

          }

        } while(d != 'q');

      }

    } while(c != 'q');

  }

  return;
}

/*------------------------------------------------------------
pdr_ns_supg_heat_init - read problem data
------------------------------------------------------------*/
int pdr_ns_supg_heat_init(
  char *Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
)
{

  FILE *testforfile;
  char filename[300], arg[300];
  int nr_sol; // number of solution vectors - determined by time integration
  int mesh_id, field_id, iaux;

  // ns_supg uses two problem structures - one from ns_supg
  pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
  pdr_ns_supg_problem_clear(&pdv_ns_supg_problem);

  // and the second from heat
  pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
  pdr_heat_problem_clear(&pdv_heat_problem);


  // first we read ns_supg problem structure
  pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
  nr_sol = 3;
  strcpy(pdv_ns_supg_problem.ctrl.work_dir, Work_dir);
  pdv_ns_supg_problem.ctrl.interactive_input = Interactive_input;
  pdv_ns_supg_problem.ctrl.interactive_output = Interactive_output;
  sprintf(filename, "%s", "problem_ns_supg.dat");

  pdr_ns_supg_problem_read(Work_dir, filename, Interactive_output,
			   &pdv_ns_supg_problem, nr_sol);


  // second we read heat problem structure
  pdv_ns_supg_heat_current_problem_id = PDC_HEAT_ID;
  nr_sol = 3;
  strcpy(pdv_heat_problem.ctrl.work_dir, Work_dir);
  pdv_heat_problem.ctrl.interactive_input = Interactive_input;
  pdv_heat_problem.ctrl.interactive_output = Interactive_output;
  sprintf(filename, "%s", "problem_heat.dat");
  pdr_heat_problem_read(Work_dir, filename, Interactive_output,
			   &pdv_heat_problem, nr_sol);

  // check data 

  fprintf(Interactive_output, 
	  "\nNS_SUPG problem %d and HEAT problem %d settings :\n",
	  pdv_ns_supg_problem.ctrl.name, pdv_heat_problem.ctrl.name);

  fprintf(Interactive_output, "\nCONTROL PARAMETERS:\n");

  fprintf(Interactive_output, "\n\tmesh_type:\t\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_type);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.mesh_type,
		 pdv_ns_supg_problem.ctrl.mesh_type)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    exit(-1);
    //printf("HEAT problem parameter overridden.\n");
    //strcpy(pdv_heat_problem.ctrl.mesh_type, pdv_ns_supg_problem.ctrl.mesh_type);
  }

  fprintf(Interactive_output, "\tmesh_file_in:\t\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_filename);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.mesh_filename,
		 pdv_ns_supg_problem.ctrl.mesh_filename)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    exit(-1);
    //printf("HEAT problem parameter overridden.\n");
    //strcpy(pdv_heat_problem.ctrl.mesh_filename, 
    //	   pdv_ns_supg_problem.ctrl.mesh_filename);
  }

  fprintf(Interactive_output, "\n\tns_supg field_file_in:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.field_filename);
  fprintf(Interactive_output, "\theat field_file_in:\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.field_filename);


  fprintf(Interactive_output, "\tns_supg mesh_file_out:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.mesh_dmp_filepattern,
		 pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    exit(-1);
    //printf("HEAT problem parameter overridden.\n");
    //strcpy(pdv_heat_problem.ctrl.mesh_dmp_filepattern, 
    //	   pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern);
  }
  fprintf(Interactive_output, "\tns_supg field_file_out:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.field_dmp_filepattern);
  fprintf(Interactive_output, "\theat field_file_out:\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.field_dmp_filepattern);

  fprintf(Interactive_output,"\n\tns_supg penalty for Dirichlet BCs:\t%lf\n",
	  pdv_ns_supg_problem.ctrl.penalty);

  fprintf(Interactive_output,"\theat penalty for Dirichlet BCs:\t\t%lf\n",
	  pdv_heat_problem.ctrl.penalty);

  fprintf(Interactive_output, "\tns_supg gravity:\t\t\t%lf %lf %lf\n", 
	  pdv_ns_supg_problem.ctrl.gravity_field[0], 
	  pdv_ns_supg_problem.ctrl.gravity_field[1], 
	  pdv_ns_supg_problem.ctrl.gravity_field[2]);

  /****************************************/
  /* initialization of material data      */
  /****************************************/
  fprintf(Interactive_output, "\n\tmaterials_file:\t\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.material_filename);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.material_filename,
		  pdv_ns_supg_problem.ctrl.material_filename)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    exit(-1);
    //printf("HEAT problem parameter overridden.\n");
    //strcpy(pdv_heat_problem.ctrl.material_filename, 
    //	   pdv_ns_supg_problem.ctrl.material_filename);
  }
  
  if(strlen(pdv_ns_supg_problem.ctrl.material_filename)==0){

    // e.g. for non-dimensional form of NS equations material files are not used
    pdv_ns_supg_problem.ctrl.ref_temperature = -1.0;

  }
  else{
    
    // for non-dimensional form of NS equations materials are not used
    iaux = pdr_ns_supg_material_read(pdv_ns_supg_problem.ctrl.work_dir, 
				     pdv_ns_supg_problem.ctrl.material_filename, 
                     pdv_ns_supg_problem.ctrl.interactive_output);
    if (iaux == EXIT_FAILURE) exit(-1);

//    fprintf(Interactive_output, "\tnumber of materials:\t\t\t%d\n",
//	    pdv_ns_supg_problem.materials.materials_num);
  
//    fprintf(Interactive_output, "\nNS_SUPG material database read\n");

  }

  if(pdv_ns_supg_problem.ctrl.ref_temperature <= 0.0){

    // reference temperature used to indicate whether material database is used
    fprintf(Interactive_output, "\n\tns_supg density:\t\t%lf\n",
	    pdv_ns_supg_problem.ctrl.density);
    fprintf(Interactive_output, "\tns_supg dynamic viscosity:\t\t%lf\n", 
	    pdv_ns_supg_problem.ctrl.dynamic_viscosity);
    
  }
  else{

    fprintf(Interactive_output, "\tns_supg density and dynamic viscosity from material database\n");
    pdv_ns_supg_problem.ctrl.density = -1.0;
    pdv_ns_supg_problem.ctrl.dynamic_viscosity = -1.0;
    
    fprintf(Interactive_output, "\tns_supg reference_temperature:\t\t%lf\n", 
	    pdv_ns_supg_problem.ctrl.ref_temperature);

  }

  fprintf(Interactive_output, "\tns_supg reference_length:\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.ref_length);
  fprintf(Interactive_output, "\tns_supg reference_velocity:\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.ref_velocity);

  if(pdv_ns_supg_problem.ctrl.ref_temperature <= 0.0){
    fprintf(Interactive_output, "\tns_supg Reynolds number:\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.ref_length*pdv_ns_supg_problem.ctrl.ref_velocity*pdv_ns_supg_problem.ctrl.density/pdv_ns_supg_problem.ctrl.dynamic_viscosity);
  }

  /****************************************/
  /* initialization of heat material data */
  /****************************************/
  if(strlen(pdv_heat_problem.ctrl.material_filename)==0){

    // e.g. for non-dimensional form of heat equation material files are not used
    pdv_heat_problem.ctrl.ref_temperature = -1.0;

  }
  else{

    iaux = pdr_heat_material_read(pdv_heat_problem.ctrl.work_dir, 
				  pdv_heat_problem.ctrl.material_filename, 
                  pdv_heat_problem.ctrl.interactive_output);
    if (iaux == EXIT_FAILURE) exit(-1);
        
    fprintf(Interactive_output, "\nHEAT materials database read\n");
    
  }

  // ns_supg and heat can have different reference temperatures!!! 
  // reference temperatures are used to indicate whether material data are constant
  if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){

    fprintf(Interactive_output, "\theat material data not temperature dependent\n");
    fprintf(Interactive_output, "\n\theat thermal_conductivity:\t\t%lf\n",
	    pdv_heat_problem.ctrl.thermal_conductivity);
    fprintf(Interactive_output, "\n\theat density:\t\t%lf\n",
	    pdv_heat_problem.ctrl.density);

    if(pdv_heat_problem.ctrl.density != pdv_ns_supg_problem.ctrl.density){
      printf("Inconsistency between ns_supg and heat for density! Exiting.\n");
      exit(-1);
    }

    fprintf(Interactive_output, "\n\theat specific_heat:\t\t%lf\n",
	    pdv_heat_problem.ctrl.specific_heat);
   
  }
  else{

    fprintf(Interactive_output, "\theat material data from material database\n");
    pdv_heat_problem.ctrl.thermal_conductivity = -1.0;
    pdv_heat_problem.ctrl.density = -1.0;
    pdv_heat_problem.ctrl.specific_heat = -1.0;
    
    fprintf(Interactive_output, "\theat reference_temperature:\t\t%lf\n", 
	    pdv_heat_problem.ctrl.ref_temperature);

  }

  fprintf(Interactive_output, "\theat ambient temperature:\t\t%lf\n", 
	  pdv_heat_problem.ctrl.ambient_temperature);


  fprintf(Interactive_output, "\nTIME INTEGRATION PARAMETERS:\n");

  fprintf(Interactive_output, "\ttype:\t\t\t\t\t%d\n", 
	  pdv_ns_supg_problem.time.type);
  if(pdv_heat_problem.time.type != pdv_ns_supg_problem.time.type){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.type = pdv_ns_supg_problem.time.type;
  }
  fprintf(Interactive_output, "\timplicitness parameter:\t\t\t%lf\n\n", 
	  pdv_ns_supg_problem.time.alpha);
  if(pdv_heat_problem.time.alpha != pdv_ns_supg_problem.time.alpha){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.alpha = pdv_ns_supg_problem.time.alpha;
  }

  fprintf(Interactive_output, "\tcurrent timestep:\t\t\t%d\n", 
	  pdv_ns_supg_problem.time.cur_step);
  if(pdv_heat_problem.time.cur_step != pdv_ns_supg_problem.time.cur_step){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.cur_step = pdv_ns_supg_problem.time.cur_step;
  }
  fprintf(Interactive_output, "\tcurrent time:\t\t\t\t%lf\n", 
	  pdv_ns_supg_problem.time.cur_time);
  if(pdv_heat_problem.time.cur_time != pdv_ns_supg_problem.time.cur_time){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.cur_time = pdv_ns_supg_problem.time.cur_time;
  }
  fprintf(Interactive_output, "\n\tcurrent timestep_length:\t\t%lf\n", 
	  pdv_ns_supg_problem.time.cur_dtime);
  if(pdv_heat_problem.time.cur_dtime != pdv_ns_supg_problem.time.cur_dtime){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.cur_dtime = pdv_ns_supg_problem.time.cur_dtime;
  }
  fprintf(Interactive_output, "\tprevious timestep_length:\t\t%lf\n\n", 
	  pdv_ns_supg_problem.time.prev_dtime);
  if(pdv_heat_problem.time.prev_dtime != pdv_ns_supg_problem.time.prev_dtime){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.prev_dtime = pdv_ns_supg_problem.time.prev_dtime;
  }

  fprintf(Interactive_output, "\tfinal time:\t\t\t\t%lf\n", 
	  pdv_ns_supg_problem.time.final_time);
  if(pdv_heat_problem.time.final_time != pdv_ns_supg_problem.time.final_time){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.final_time = pdv_ns_supg_problem.time.final_time;
  }
  fprintf(Interactive_output, "\tfinal timestep:\t\t\t\t%d\n", 
	  pdv_ns_supg_problem.time.final_step);
  if(pdv_heat_problem.time.final_step != pdv_ns_supg_problem.time.final_step){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.final_step = pdv_ns_supg_problem.time.final_step;
  }

  fprintf(Interactive_output, "\n\tns_supg convergence criterion type:\t%d\n", 
	  pdv_ns_supg_problem.time.conv_type);
  fprintf(Interactive_output, "\tns_supg error tolerance (n-epsilon):\t%lf\n", 
	  pdv_ns_supg_problem.time.conv_meas);
  fprintf(Interactive_output, "\n\theat convergence criterion type:\t%d\n", 
	  pdv_heat_problem.time.conv_type);
  fprintf(Interactive_output, "\theat error tolerance (n-epsilon):\t%lf\n\n", 
	  pdv_heat_problem.time.conv_meas);

  fprintf(Interactive_output, "\tmonitoring level:\t\t\t%d\n\n", 
	  pdv_ns_supg_problem.time.monitor);
  if(pdv_heat_problem.time.monitor != pdv_ns_supg_problem.time.monitor){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.monitor = pdv_ns_supg_problem.time.monitor;
  }

  fprintf(Interactive_output, "\tgraph_dump_intv:\t\t\t%d\n", 
	  pdv_ns_supg_problem.time.intv_graph);
  if(pdv_heat_problem.time.intv_graph != pdv_ns_supg_problem.time.intv_graph){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.intv_graph = pdv_ns_supg_problem.time.intv_graph;
  }
  fprintf(Interactive_output, "\tfull_dump_intv:\t\t\t\t%d\n\n", 
	  pdv_ns_supg_problem.time.intv_dumpout);
  if(pdv_heat_problem.time.intv_dumpout!=pdv_ns_supg_problem.time.intv_dumpout){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.time.intv_dumpout = pdv_ns_supg_problem.time.intv_dumpout;
  }

  fprintf(Interactive_output, "\nNONLINEAR SOLVER PARAMETERS:\n");

  fprintf(Interactive_output, "\ttype:\t\t\t\t\t%d\n", 
	  pdv_ns_supg_problem.nonl.type);
  if(pdv_heat_problem.nonl.type != pdv_ns_supg_problem.nonl.type){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.nonl.type = pdv_ns_supg_problem.nonl.type;
  }

  fprintf(Interactive_output, "\tmax_nonl_iter:\t\t\t\t%d\n", 
	  pdv_ns_supg_problem.nonl.max_iter);
  if(pdv_heat_problem.nonl.max_iter != pdv_ns_supg_problem.nonl.max_iter){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.nonl.max_iter = pdv_ns_supg_problem.nonl.max_iter;
  }

  fprintf(Interactive_output, "\n\tns_supg convergence criterion type:\t%d\n", 
	  pdv_ns_supg_problem.nonl.conv_type);
  fprintf(Interactive_output, "\tns_supg error tolerance (k-epsilon):\t%lf\n", 
	  pdv_ns_supg_problem.nonl.conv_meas);
  fprintf(Interactive_output, "\tns_supg monitoring level:\t\t%d\n", 
	  pdv_ns_supg_problem.nonl.monitor);

  fprintf(Interactive_output, "\n\theat convergence criterion type:\t%d\n", 
	  pdv_heat_problem.nonl.conv_type);
  fprintf(Interactive_output, "\theat error tolerance (k-epsilon):\t%lf\n", 
	  pdv_heat_problem.nonl.conv_meas);
  fprintf(Interactive_output, "\theat monitoring level:\t\t\t%d\n\n", 
	  pdv_heat_problem.nonl.monitor);


  fprintf(Interactive_output, "\nLINEAR SOLVER PARAMETERS:\n");

  fprintf(Interactive_output, "\tns_supg solver type:\t\t\t%d\n", 
	  pdv_ns_supg_problem.lins.type);

  fprintf(Interactive_output, "\tns_supg solver_file:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.solver_filename);

  if(pdv_ns_supg_problem.lins.type!=0){
    fprintf(Interactive_output, "\n\tns_supg max_lins_iter:\t\t\t%d\n", 
	    pdv_ns_supg_problem.lins.max_iter);
    fprintf(Interactive_output, "\tns_supg convergence criterion type:\t%d\n", 
	    pdv_ns_supg_problem.lins.conv_type);
    fprintf(Interactive_output, "\tns_supg error tolerance:\t\t%.15lf  %.15lf\n", 
	    pdv_ns_supg_problem.lins.conv_meas,
	    pdr_lins_d_params(PDC_NS_SUPG_ID, 4));
  }

  fprintf(Interactive_output, "\tns_supg monitoring level:\t\t%d\n", 
	  pdv_ns_supg_problem.lins.monitor);

  fprintf(Interactive_output, "\n\theat solver type:\t\t\t%d\n", 
	  pdv_heat_problem.lins.type);

  fprintf(Interactive_output, "\theat solver_file:\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.solver_filename);

  if(pdv_heat_problem.lins.type!=0){
    fprintf(Interactive_output, "\n\theat max_lins_iter:\t\t\t%d\n", 
	    pdv_heat_problem.lins.max_iter);
    fprintf(Interactive_output, "\theat convergence criterion type:\t%d\n", 
	    pdv_heat_problem.lins.conv_type);
    fprintf(Interactive_output, "\theat error tolerance:\t\t\t%.15lf  %.15lf\n", 
	    pdv_heat_problem.lins.conv_meas,
	    pdr_lins_d_params(PDC_NS_SUPG_ID, 4));
  }

  fprintf(Interactive_output, "\theat monitoring level:\t\t\t%d\n\n", 
	  pdv_heat_problem.lins.monitor);


  fprintf(Interactive_output, "\nADAPTATION PARAMETERS:\n");

  fprintf(Interactive_output, "\tns_supg adapt_type:\t\t\t%d\n", 
	  pdv_ns_supg_problem.adpt.type);
  fprintf(Interactive_output, "\tns_supg adapt_interval:\t\t\t%d\n", 
	  pdv_ns_supg_problem.adpt.interval);
  fprintf(Interactive_output, "\tns_supg adapt_eps:\t\t\t%lf\n", 
	  pdv_ns_supg_problem.adpt.eps);
  fprintf(Interactive_output, "\tns_supg adapt_ratio:\t\t\t%lf\n\n", 
	  pdv_ns_supg_problem.adpt.ratio);

  fprintf(Interactive_output, "\tns_supg monitoring level:\t\t%d\n\n", 
	  pdv_ns_supg_problem.adpt.monitor);

  fprintf(Interactive_output, "\theat adapt_type:\t\t\t%d\n", 
	  pdv_heat_problem.adpt.type);
  fprintf(Interactive_output, "\theat adapt_interval:\t\t\t%d\n", 
	  pdv_heat_problem.adpt.interval);
  fprintf(Interactive_output, "\theat adapt_eps:\t\t\t\t%lf\n", 
	  pdv_heat_problem.adpt.eps);
  fprintf(Interactive_output, "\theat adapt_ratio:\t\t\t%lf\n\n", 
	  pdv_heat_problem.adpt.ratio);

  fprintf(Interactive_output, "\theat monitoring level:\t\t\t%d\n\n", 
	  pdv_heat_problem.adpt.monitor);


  /****************************************/
  /* initialization of bc data            */
  /****************************************/

  iaux = pdr_ns_supg_bc_read(pdv_ns_supg_problem.ctrl.work_dir, 
			     pdv_ns_supg_problem.ctrl.bc_filename, 
			     pdv_ns_supg_problem.ctrl.interactive_output, 
			     &pdv_ns_supg_problem.bc);
  if (iaux == EXIT_FAILURE) exit(-1);

  fprintf(Interactive_output, "\nns_supg boundary conditions configuration file:\t%s\n", 
	  pdv_ns_supg_problem.ctrl.bc_filename);
  fprintf(Interactive_output, "\tnumber of pressure pins:\t\t%d\n", 
	  pdr_ns_supg_get_pressure_pins_count(&pdv_ns_supg_problem.bc));
  fprintf(Interactive_output, "\tnumber of velocity pins:\t\t%d\n", 
	  pdr_ns_supg_get_velocity_pins_count(&pdv_ns_supg_problem.bc));
  fprintf(Interactive_output, "\tnumber of BCs:\t\t\t\t%d\n\n", 
	  pdr_ns_supg_get_bc_assign_count(&pdv_ns_supg_problem.bc));

  fprintf(Interactive_output, "NS_SUPG BC OK\n");
  
  iaux = pdr_heat_bc_read(pdv_heat_problem.ctrl.work_dir, 
			     pdv_heat_problem.ctrl.bc_filename, 
			     pdv_heat_problem.ctrl.interactive_output, 
			     &pdv_heat_problem.bc);
  if (iaux == EXIT_FAILURE) exit(-1);

  fprintf(Interactive_output, "\nheat boundary conditions configuration file:\t%s\n", 
	  pdv_heat_problem.ctrl.bc_filename);
  fprintf(Interactive_output, "\tnumber of BCs:\t\t\t\t%d\n\n", 
	  pdr_heat_get_bc_assign_count(&pdv_heat_problem.bc));


  fprintf(Interactive_output, "HEAT BC OK\n");
  
  /****************************************/
  /* initialization of mesh data          */
  /****************************************/

  // mesh is read using ns_supg problem parameters (there is one mesh for
  // both problems)
  pdv_ns_supg_heat_current_problem_id = PDC_NS_SUPG_ID;
  mesh_id = utr_initialize_mesh( Interactive_output, Work_dir, 
				pdv_ns_supg_problem.ctrl.mesh_type[0], pdv_ns_supg_problem.ctrl.mesh_filename);
  pdv_ns_supg_problem.ctrl.mesh_id = mesh_id;
  pdv_heat_problem.ctrl.mesh_id = mesh_id;



  /// Initialziation of contact bc data
  if(pdv_heat_problem.bc.bc_contact_count > 0) {

      int * bc_nums = (int*) calloc(pdv_heat_problem.bc.bc_contact_count,sizeof(int));
      int * ids = (int*) calloc(2*pdv_heat_problem.bc.bc_contact_count,sizeof(int));
      int * types = (int*) calloc(pdv_heat_problem.bc.bc_contact_count,sizeof(int));

      int i=0;
      for(; i < pdv_heat_problem.bc.bc_contact_count; ++i) {
          bc_nums[i] = pdv_heat_problem.bc.bc_contact[i].bnum;
          types[i] = pdv_heat_problem.bc.bc_contact[i].type;
          ids[2*i] = pdv_heat_problem.bc.bc_contact[i].IDs[0];
          ids[2*i+1] = pdv_heat_problem.bc.bc_contact[i].IDs[1];
      }

      int n_inserted = utr_mesh_insert_BC_contact(Work_dir,
												  pdv_heat_problem.ctrl.mesh_id,
                                                  pdv_heat_problem.bc.bc_contact_count,
                                                  bc_nums,
                                                  ids,
                                                  (utt_mesh_bc_type*) types);

      mf_check(n_inserted == pdv_heat_problem.bc.bc_contact_count,
               "Not all contact BCs ware inserted correctly (%d from %d)",
               n_inserted,pdv_heat_problem.bc.bc_contact_count);

      free(bc_nums);
      free(ids);
      free(types);
  }



#ifdef DEBUG
  {
  int currfa = 0;
  int fa_bnum;
  /* check if every boundary has been assigned boundary condtion */
  while (currfa = mmr_get_next_face_all(pdv_heat_problem.ctrl.mesh_id, 
					currfa)) {
    fa_bnum = mmr_fa_bc(pdv_heat_problem.ctrl.mesh_id, currfa);
    //fprintf(Interactive_output, "BC HEAT %d set for boundary %d\n", 
    //	    pdr_heat_get_bc_type(&pdv_heat_problem.bc, fa_bnum), fa_bnum);
    if (fa_bnum > 0) {
      if ((pdr_heat_get_bc_type(&pdv_heat_problem.bc, fa_bnum) == BC_HEAT_NONE)) {
	fprintf(Interactive_output, "BC HEAT not set for boundary:\t%d\n", fa_bnum);
	fprintf(Interactive_output, "Check bc config file - Exiting.\n");
	exit(-1);
      }
    }
  }
  }
#endif

#ifdef DEBUG
  {
  int currfa = 0;
  int fa_bnum;
  /* check if every boundary has been assigned boundary condtion */
  while (currfa = mmr_get_next_face_all(pdv_ns_supg_problem.ctrl.mesh_id, 
					currfa)) {
    fa_bnum = mmr_fa_bc(pdv_ns_supg_problem.ctrl.mesh_id, currfa);
    //fprintf(Interactive_output, "BC NS_SUPG %d set for boundary %d\n", 
    //	    pdr_ns_supg_get_bc_type(&pdv_ns_supg_problem.bc, fa_bnum), fa_bnum);
    if (fa_bnum > 0) {
      if ((pdr_ns_supg_get_bc_type(&pdv_ns_supg_problem.bc, fa_bnum) == BC_NS_SUPG_NONE)) {
	fprintf(Interactive_output, "BC NS_SUPG not set for boundary:\t%d\n", fa_bnum);
	fprintf(Interactive_output, "Check bc config file - Exiting.\n");
	exit(-1);
      }
    }
  }
  }
#endif

  mmr_set_max_gen(mesh_id, pdv_ns_supg_problem.adpt.maxgen);
  mmr_set_max_gen_diff(mesh_id, 1); // one irregularity of meshes enforced

  fprintf(Interactive_output, "\nAfter reading initial mesh data.\n\n");
  fprintf(Interactive_output, 
	  "Mesh entities (number of active, maximal index):\n");
  fprintf(Interactive_output, "Elements: nrel %d, nmel %d\n", 
	  mmr_get_nr_elem(mesh_id), mmr_get_max_elem_id(mesh_id));
  fprintf(Interactive_output, "Faces:    nrfa %d, nmfa %d\n", 
	  mmr_get_nr_face(mesh_id), mmr_get_max_face_id(mesh_id));
  fprintf(Interactive_output, "Edges:    nred %d, nmed %d\n", 
	  mmr_get_nr_edge(mesh_id), mmr_get_max_edge_id(mesh_id));
  fprintf(Interactive_output, "Nodes:    nrno %d, nmno %d\n", 
	  mmr_get_nr_node(mesh_id), mmr_get_max_node_id(mesh_id));

  fprintf(Interactive_output, 
	  "\nMaximal generation level set to %d, maximal generation difference set to %d\n",
	  mmr_get_max_gen(mesh_id), mmr_get_max_gen_diff(mesh_id));

  /****************************************/
  /* initialization of approximation field data */
  /****************************************/
  int pdeg = 1;
  
  // first ns_supg field
  if (strcmp(pdv_ns_supg_problem.ctrl.field_filename,"z")==0){
	  
    fprintf(Interactive_output, 
	    "\nInitializing ns_supg field with 0\n"); 
    //  - for standard continuous basis functions
    // 'z' - for zeroing field values
	
    pdv_ns_supg_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		               pdv_ns_supg_problem.ctrl.mesh_id, 
			       pdv_ns_supg_problem.ctrl.nreq, 
			       pdv_ns_supg_problem.ctrl.nr_sol, pdeg,NULL,NULL);
				   
				    
  }
  else if (strcmp(pdv_ns_supg_problem.ctrl.field_filename,"i")==0) {
    fprintf(Interactive_output, 
	    "\nInitializing ns_supg field with initial_condition function\n"); 
  // 's' - for standard continuous basis functions
  // 'i' - for initializing using function pdr_ns_supg_heat_initial_condition
    pdv_ns_supg_problem.ctrl.field_id = utr_initialize_field(
			       Interactive_output, 's', 'i', 
		               pdv_ns_supg_problem.ctrl.mesh_id, 
			       pdv_ns_supg_problem.ctrl.nreq, 
			       pdv_ns_supg_problem.ctrl.nr_sol, pdeg, NULL, 
			       pdr_ns_supg_initial_condition);
			
  }
  else{
	
    sprintf(arg, "%s/%s", Work_dir, pdv_ns_supg_problem.ctrl.field_filename);
    testforfile = fopen(arg, "r");
    if (testforfile != NULL) {
      fclose(testforfile);
      fprintf(Interactive_output, "Input field file %s.", 
	      pdv_ns_supg_problem.ctrl.field_filename);
  // 's' - for standard continuous basis functions
  // 'i' - for initializing using function pdr_ns_supg_heat_initial_condition
      pdv_ns_supg_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'r',
                               pdv_ns_supg_problem.ctrl.mesh_id, 
                               pdv_ns_supg_problem.ctrl.nreq, 
                               pdv_ns_supg_problem.ctrl.nr_sol, pdeg, arg,NULL);
						
    } else {
      fprintf(Interactive_output, 
	      "Input field file %s not found - setting field to 0\n", 
	      pdv_ns_supg_problem.ctrl.field_filename);
      // 's' - for standard continuous basis functions
      // 'z' - for zeroing field values
      pdv_ns_supg_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		               pdv_ns_supg_problem.ctrl.mesh_id, 
			       pdv_ns_supg_problem.ctrl.nreq, 
			       pdv_ns_supg_problem.ctrl.nr_sol, pdeg,NULL,NULL);
    }
  }
  apr_check_field(pdv_ns_supg_problem.ctrl.field_id);


  // second heat field
  
 
  if (strcmp(pdv_heat_problem.ctrl.field_filename,"z")==0){
    fprintf(Interactive_output, 
	    "\nInitializing heat field with 0\n"); 
    // 's' - for standard continuous basis functions
    // 'z' - for zeroing field values
    pdv_heat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		               pdv_heat_problem.ctrl.mesh_id, 
			       pdv_heat_problem.ctrl.nreq, 
			       pdv_heat_problem.ctrl.nr_sol, pdeg,NULL,NULL);
  }
  else if (strcmp(pdv_heat_problem.ctrl.field_filename,"i")==0) {
    fprintf(Interactive_output, 
	    "\nInitializing heat field with initial_condition function\n"); 
  // 's' - for standard continuous basis functions
  // 'i' - for initializing using function pdr_heat_heat_initial_condition
    pdv_heat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'i', 
		               pdv_heat_problem.ctrl.mesh_id, 
			       pdv_heat_problem.ctrl.nreq, 
			       pdv_heat_problem.ctrl.nr_sol, pdeg, NULL, 
			       pdr_heat_initial_condition);
  }
  else{
    sprintf(arg, "%s/%s", Work_dir, pdv_heat_problem.ctrl.field_filename);
    testforfile = fopen(arg, "r");
    if (testforfile != NULL) {
      fclose(testforfile);
      fprintf(Interactive_output, "\nInput field file %s.", 
	      pdv_heat_problem.ctrl.field_filename);
      // 's' - for standard continuous basis functions
      // 'i' - for initializing using function pdr_heat_heat_initial_condition
      pdv_heat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'r',
                               pdv_heat_problem.ctrl.mesh_id, 
                               pdv_heat_problem.ctrl.nreq, 
                               pdv_heat_problem.ctrl.nr_sol, pdeg, arg,NULL);
    } else {
      fprintf(Interactive_output, 
	      "\nInput field file %s not found - setting field to 0\n", 
	      pdv_heat_problem.ctrl.field_filename);
      // 's' - for standard continuous basis functions
      // 'z' - for zeroing field values
      pdv_heat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		               pdv_heat_problem.ctrl.mesh_id, 
			       pdv_heat_problem.ctrl.nreq, 
			       pdv_heat_problem.ctrl.nr_sol, pdeg,NULL,NULL);
    }
  }
  apr_check_field(pdv_heat_problem.ctrl.field_id);

  // third heat_dtdt field
  /* fprintf(Interactive_output,  */
  /* 	    "\nInitializing heat_dtdt field with 0\n");  */
  /* pdv_heat_dtdt_problem.ctrl.field_id = utr_initialize_field( */
  /*                           Interactive_output, 's', 'z',  */
  /* 			    pdv_heat_dtdt_problem.ctrl.mesh_id,  */
  /* 			    pdv_heat_dtdt_problem.ctrl.nreq,  */
  /* 			    pdv_heat_dtdt_problem.ctrl.nr_sol, pdeg,NULL,NULL); */

  /* fprintf(Interactive_output, "\n\tAS: num_dof = % d", apr_get_ent_nrdofs(pdv_heat_dtdt_problem.ctrl.field_id,APC_VERTEX,1)); */
  /* fprintf(Interactive_output, "\n\tAS: nreq = % d", apr_get_nreq(pdv_heat_dtdt_problem.ctrl.field_id)); */
  /* fprintf(Interactive_output, "\n\tAS: numshap = % d", apr_get_ent_numshap(pdv_heat_dtdt_problem.ctrl.field_id, APC_VERTEX, 1)); */
  /* fprintf(Interactive_output, "\n\tAS: pdeg = % d", apr_get_ent_pdeg(pdv_heat_dtdt_problem.ctrl.field_id, APC_VERTEX, 1)); */
  /* fprintf(Interactive_output, "\n\tAS: field_id = %d, mesh_id = %d, nreq = %d, nr_sol = %d, pdeg = %d\n", */
  /* 	  pdv_heat_dtdt_problem.ctrl.field_id, */
  /* 	  pdv_heat_dtdt_problem.ctrl.mesh_id,  */
  /* 	  pdv_heat_dtdt_problem.ctrl.nreq,  */
  /* 	  pdv_heat_dtdt_problem.ctrl.nr_sol, pdeg); */

  /* apr_check_field(pdv_heat_dtdt_problem.ctrl.field_id); */

return(0);
}


int  pdr_ns_supg_heat_bc_free()
{

  return(0);
}

int  pdr_ns_supg_heat_material_free()
{

  return(0);
}


#ifdef __cplusplus
}
#endif
