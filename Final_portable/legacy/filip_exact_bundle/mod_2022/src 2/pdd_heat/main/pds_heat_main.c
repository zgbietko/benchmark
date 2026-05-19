/************************************************************************
File pds_heat_main.c - driver for heat module - approximation
      of heat transfer and convection

Contains definitions of global variables - among them:
  pdv_heat_problem - problem structure for heat problem
					      
Contains definition of routines:
  main

Procedures local to main:
  pdr_heat_init - initialize problem data
  pdr_heat_post_process
  pdr_heat_profile
  pdr_heat_initial_condition

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
	2014    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)

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
#include "../include/pdh_heat.h"		/* USES */
/* types and functions related to problem structures */
#include "../include/pdh_heat_problem.h" 
// bc and material header files are included in problem header files


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
double pdv_heat_timer_all = 0.0;
double pdv_heat_timer_pdr_comp_el_stiff_mat = 0.0;
double pdv_heat_timer_pdr_comp_fa_stiff_mat = 0.0;

// ID of the current problem
// on purpose initialized to 0 which is wrong value !
// later should be replaced by one of the two proper values:
int pdv_heat_current_problem_id = 0;	/* ID of the current problem */
// problem structure for heat module
pdt_heat_problem pdv_heat_problem;


/***************************************/
/* DECLARATIONS OF INTERNAL PROCEDURES */
/***************************************/
/* Rules:
/* - name always begins with pdr_heat */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_heat_post_process - simple post-processing
------------------------------------------------------------*/
double pdr_heat_post_process(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/*------------------------------------------------------------
pdr_heat_profile - to dump a set of values along a line
------------------------------------------------------------*/
int pdr_heat_write_profile(
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
  int i, info, iaux;
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

  // initialize  heat structures
  
  iaux=pdr_heat_init(work_dir, interactive_input, interactive_output);
  if (iaux == EXIT_FAILURE) exit(-1);

  int problem_id = pdv_heat_current_problem_id;
  i=2; mesh_id=pdr_ctrl_i_params(problem_id,i);
  i=3; field_id = pdr_ctrl_i_params(problem_id,i);  
  i=4; int nr_sol = pdr_ctrl_i_params(problem_id,i);
  i=5; int nreq = pdr_ctrl_i_params(problem_id,i);
  // currently supported fields:
  // STANDARD_LINEAR - for continuous, vector, linear approximations
  // DG_SCALAR_PRISM - for discontinuous, scalar, high order approximations
  // apr_module_introduce(field_module_name);

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
	  printf("\ts - solve single heat problem \n");
	  printf("\te - compute error (ZZ aproximation)\n");
	  printf("\tp - postprocessing\n");
	  printf("\tg - launch graphic module\n");
	  printf("\tm - perform uniform mesh refinement \n");
	  printf("\ta - perform automatic mesh adaptation \n");
	  printf("\tr - print profile\n");
	  printf("\td - dump out data \n");
	  printf("\tv - write ParaView graphics data \n");
	  printf("\tc - change control data \n");
	  printf("\tq - exit the program\n");
	  scanf(" %c", &c);
	}
	while (c != 's' && c != 'e' && c != 'p' && c != 'g' 
	        && c != 'm' && c != 'a' && c != 'r' && c != 'd' 
	        && c != 'v' && c != 'c' && c != 't' && c != 'q');
      } else {
	fscanf(interactive_input, "%c\n", &c);
      }
#ifdef PARALLEL
    }
    pcr_bcast_char(pcr_print_master(), 1, &c);
    //printf("After BCAST %c\n",c);
#endif

    /*------------------------------------------------------------*/
    if (c == 't') {

      utv_SIGINT_not_caught = 1;

      pdv_heat_timer_all = 0.0;
      pdv_heat_timer_pdr_comp_el_stiff_mat = 0.0;
      pdv_heat_timer_pdr_comp_fa_stiff_mat = 0.0;

      fprintf(interactive_output, 
	      "\nBeginning time integration of heat problem\n\n");

      pdv_heat_timer_all = time_clock();

      /*---------- main time integration procedure ------------*/
      pdr_heat_time_integration(work_dir, 
			   interactive_input, interactive_output);


      pdv_heat_timer_all = time_clock() - pdv_heat_timer_all;

      fprintf(interactive_output,"\nTime total: %lf\n", 
	      pdv_heat_timer_all);
      fprintf(interactive_output,"Time in pdr_comp_el_stiff_mat: %lf [%lf%%]\n", 
	      pdv_heat_timer_pdr_comp_el_stiff_mat, 
	      (pdv_heat_timer_pdr_comp_el_stiff_mat 
	       / pdv_heat_timer_all) * 100);
      fprintf(interactive_output,"Time in pdr_comp_fa_stiff_mat: %lf [%lf%%]\n", 
	      pdv_heat_timer_pdr_comp_fa_stiff_mat, 
	      (pdv_heat_timer_pdr_comp_fa_stiff_mat 
	       / pdv_heat_timer_all) * 100);

    }
    /*------------------------------------------------------------*/
    if (c == 's') {
      
      utv_SIGINT_not_caught = 1;
      
      pdv_heat_timer_all = 0.0;
      pdv_heat_timer_pdr_comp_el_stiff_mat = 0.0;
      pdv_heat_timer_pdr_comp_fa_stiff_mat = 0.0;
      
#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif
      fprintf(interactive_output, 
	      "\nBeginning solution of a single heat problem\n\n");
#ifdef PARALLEL
    }
#endif
      
      pdv_heat_timer_all = time_clock();
      
#ifdef PARALLEL
      /* initiate exchange tables for DOFs - for two fields, one level */
      int ione = 1;
      appr_init_exchange_tables(pcr_nr_proc(), pcr_my_proc_id(), field_id, &ione);
#endif


      /*---------- main solution procedure ------------*/

      char solver_heat_filename[300];;

      if(pdr_lins_i_params(problem_id, 1) > 0){ // iterative solver	

	int max_iter = -1;
	int error_type = -1;
	double error_tolerance = -1;
	int monitoring_level = -1;
	
	// when no parameter file passed - take control parameters from problem input file
	if(0==strlen(pdv_heat_problem.ctrl.solver_filename)){

	  strcpy(solver_heat_filename, pdv_heat_problem.ctrl.solver_filename);
	  max_iter = pdr_lins_i_params(problem_id, 2); // max_iter
	  error_type = pdr_lins_i_params(problem_id, 3); // error_type
	  error_tolerance = pdr_lins_d_params(problem_id, 4); // error_tolerance
	  monitoring_level = pdr_lins_i_params(problem_id, 5);  // monitoring level

/*kbw
	  fprintf(Interactive_output, "heat solver parameters: maxiter %d, error_type %d, error_meas %.15lf, monitor %d\n",
		  pdr_lins_i_params(problem_id, 2), // max_iter
		  pdr_lins_i_params(problem_id, 3), // error_type
		  pdr_lins_d_params(problem_id, 4), // error_tolerance
		  pdr_lins_i_params(problem_id, 5)  // monitoring level
		  );
/*kew*/

	}
	else {

	  sprintf(solver_heat_filename, "%s/%s", 
		  pdv_heat_problem.ctrl.work_dir, pdv_heat_problem.ctrl.solver_filename);

	}
	
	int parallel = SIC_SEQUENTIAL;
#ifdef PARALLEL
	parallel = SIC_PARALLEL;
#endif

	// in a single call to sir_solve_lin_sys

	//sir_solve_lin_sys(problem_id, parallel, solver_heat_filename,  max_iter,
	//		  error_type, error_tol, monitoring_level);
      

        // or using separate calls to steps in solution procedure
	
/*kbw*/
    fprintf(interactive_output, "initializing heat solver (file %s)\n", solver_heat_filename);
    fprintf(interactive_output, "parameters: parallel %d, maxiter %d, error_type %d, error_meas %.15lf, monitor %d\n", 
	    parallel, max_iter,  error_type,  error_tolerance, monitoring_level);
/*kbw*/

	int solver_heat_id = sir_init(parallel, solver_heat_filename,  max_iter,
				      error_type, error_tolerance, monitoring_level);
	
	pdv_heat_problem.ctrl.solver_id = solver_heat_id; // !!! for callback procedures !!!
	sir_create(solver_heat_id, problem_id); 
	
	int ini_guess = 0; //  no initial guess from data structure 
	int nr_iter = max_iter;
	double conv_meas = error_tolerance;
	int monitor =  monitoring_level;
	double conv_rate;
	/*------------ CALLING ITERATIVE SOLVER ------------------*/
	sir_solve(solver_heat_id, SIC_SOLVE, ini_guess, monitor, 
		  &nr_iter, &conv_meas, &conv_rate);
	fprintf(interactive_output, 
		"\nAfter %d iterations of linear solver for heat problem\n", 
		nr_iter); 
	fprintf(interactive_output, 
		"Convergence measure: %15.12lf, convergence rate %lf\n", 
		conv_meas, conv_rate); 

	sir_free(solver_heat_id);

	sir_destroy(solver_heat_id);
	
#ifdef PARALLEL
/* free exchange tables for DOFs - for one field = one solver */
	appr_free_exchange_tables(1);
#endif

      } else {
	
	sir_direct_solve_lin_sys(problem_id, SIC_SEQUENTIAL, solver_heat_filename);	

      }
      
      
      pdv_heat_timer_all = time_clock() - pdv_heat_timer_all;
      
      fprintf(interactive_output,"\nTime total: %lf\n", 
	      pdv_heat_timer_all);
      fprintf(interactive_output,"Time in pdr_comp_el_stiff_mat: %lf [%lf%%]\n", 
	      pdv_heat_timer_pdr_comp_el_stiff_mat, 
	      (pdv_heat_timer_pdr_comp_el_stiff_mat 
	       / pdv_heat_timer_all) * 100);
      fprintf(interactive_output,"Time in pdr_comp_fa_stiff_mat: %lf [%lf%%]\n", 
	      pdv_heat_timer_pdr_comp_fa_stiff_mat, 
	      (pdv_heat_timer_pdr_comp_fa_stiff_mat 
	       / pdv_heat_timer_all) * 100);

    }
    /*------------------------------------------------------------*/
    else if (c == 'e') {

      pdr_heat_ZZ_error(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'p') {

      pdr_heat_post_process(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'g') {
    	c = '\0';
    	init_mod_fem_viewer(argc,argv,interactive_output);
    }
    /*------------------------------------------------------------*/
    else if (c == 'm') {

      // perform manual mesh refinement
      int ref_type = -1;
      pdr_heat_refine(pdv_heat_current_problem_id, ref_type,
			    interactive_output);
    }
    /*------------------------------------------------------------*/
    else if (c == 'a') {

      pdr_heat_adapt(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'r') {

      pdr_heat_write_profile(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'd') {

      pdr_heat_dump_data(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'v') {

      pdr_heat_write_paraview(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'c') {

      fprintf(interactive_output, "\nSet new time step: ");
      fscanf(interactive_input, "%lf", &pdv_heat_problem.time.cur_dtime);

      pdr_change_data(pdv_heat_current_problem_id);


    }
  }
  while (c != 'q');

  /* free allocated space */
  iaux=3; field_id = pdr_ctrl_i_params(pdv_heat_current_problem_id,iaux);
  apr_free_field(field_id);

  /* HEAT_DTDT_ID; */
  /* iaux=3; field_id = pdr_ctrl_i_params(pdv_heat_current_problem_id,iaux); */
  /* apr_free_field(field_id); */

  mesh_id = apr_get_mesh_id(field_id);
  mmr_free_mesh(mesh_id);

  fclose(interactive_input);
  fclose(interactive_output);

  //pdr_heat_bc_free();
  //pdr_heat_material_free();

#ifdef PARALLEL
  pcr_exit_parallel();
#endif

  return (0);
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
pdr_heat_post_process
------------------------------------------------------------*/
double pdr_heat_post_process(
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

  problem_id = pdv_heat_current_problem_id;
  
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
pdr_heat_write_profile
------------------------------------------------------------*/
int pdr_heat_write_profile(
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

  problem_id = pdv_heat_current_problem_id;
  
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
    
    return pdr_heat_err_indi_explicit(Problem_id, El);
    
  } else if (Mode == PDC_ADAPT_ZZ) {
    
    return pdr_heat_err_indi_ZZ(Problem_id, El);
    
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
  return (&pdv_heat_problem);
}

/*------------------------------------------------------------
pdr_ctrl_i_params - to return one of control parameters
------------------------------------------------------------*/
int pdr_ctrl_i_params(int Problem_id, int Num)
{

  pdt_heat_ctrls *ctrl_heat = &pdv_heat_problem.ctrl;

/*++++++++++++++++ executable statements ++++++++++++++++*/

    
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
    
    
  
  return (-1);
}

/*------------------------------------------------------------
pdr_ctrl_d_params - to return one of control parameters
------------------------------------------------------------*/
double pdr_ctrl_d_params(int Problem_id, int Num)
{
  pdt_heat_ctrls *ctrl_heat = &pdv_heat_problem.ctrl;

/*++++++++++++++++ executable statements ++++++++++++++++*/

    if (Num == 20) {
      return (ctrl_heat->ref_temperature);
    }

  return(0.0);
}

/*------------------------------------------------------------
pdr_adapt_i_params - to return parameters of adaptation
------------------------------------------------------------*/
int pdr_adapt_i_params(int Problem_id, int Num)
{

  pdt_heat_adpts *adpts_heat = &pdv_heat_problem.adpt;

/*++++++++++++++++ executable statements ++++++++++++++++*/

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
  

  return (-1);
}


/*------------------------------------------------------------
pdr_adapt_d_params - to return parameters of adaptation
------------------------------------------------------------*/
double pdr_adapt_d_params(int Problem_id, int Num)
{


  pdt_heat_adpts *adpts_heat = &pdv_heat_problem.adpt;

/*++++++++++++++++ executable statements ++++++++++++++++*/

    if (Num == 5)
      return (adpts_heat->eps);
    else if (Num == 6)
      return (adpts_heat->ratio);
    else {
      printf("Wrong parameter number in adapt_d_params!");
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

/*++++++++++++++++ executable statements ++++++++++++++++*/

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
 

  return (-1);
}


/*------------------------------------------------------------
pdr_time_d_params - to return parameters of timeation
------------------------------------------------------------*/
double pdr_time_d_params(int Problem_id, int Num)
{


  pdt_heat_times *times_heat = &pdv_heat_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

    if (Num == 2)
      return (times_heat->alpha);
    else {
      printf("Wrong parameter number in time_d_params!");
      exit(1);
    }
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
  pdt_heat_times *times_heat = &pdv_heat_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/


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
  pdt_heat_times *times_heat = &pdv_heat_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/


    if(Num==4) times_heat->cur_time=Value;
    else if(Num==5) times_heat->final_time=Value;
    else if(Num==6) times_heat->cur_dtime=Value;
    else if(Num==7) times_heat->prev_dtime=Value;
    else if(Num==9) times_heat->conv_meas=Value;
    else {
      printf("Wrong parameter number in set_time_d_params!");
      exit(1);
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
  pdt_heat_linss *linss_heat = &pdv_heat_problem.lins;

/*++++++++++++++++ executable statements ++++++++++++++++*/


    if(Num==1) return(linss_heat->type);
    else if(Num==2) return(linss_heat->max_iter);
    else if(Num==3) return(linss_heat->conv_type);
    else if(Num==5) return(linss_heat->monitor);
    else {
      printf("Wrong parameter number in lins_i_params!");
      exit(-1);
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
  pdt_heat_linss *linss_heat = &pdv_heat_problem.lins;

/*++++++++++++++++ executable statements ++++++++++++++++*/


    if(Num==4) return(linss_heat->conv_meas);
    else {
      printf("Wrong parameter number in lins_d_params!");
      exit(-1);
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


/* local variables */
  pdt_heat_adpts *adpts_heat = &pdv_heat_problem.adpt;
  pdt_heat_times *times_heat = &pdv_heat_problem.time;
  pdt_heat_linss *linss_heat = &pdv_heat_problem.lins;
  pdt_heat_ctrls *ctrls_heat = &pdv_heat_problem.ctrl;
  char c, d, pans[100]; /* string variable to read menu */

/*++++++++++++++++ executable statements ++++++++++++++++*/


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


  return;
}

/*------------------------------------------------------------
pdr_heat_init - read problem data
------------------------------------------------------------*/
int pdr_heat_init(
  char *Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
)
{

  FILE *testforfile;
  char filename[300], arg[300];
  int nr_sol; // number of solution vectors - determined by time integration
  int mesh_id, field_id, iaux;


  pdr_heat_problem_clear(&pdv_heat_problem);

  nr_sol = 3;
  strcpy(pdv_heat_problem.ctrl.work_dir, Work_dir);
  pdv_heat_problem.ctrl.interactive_input = Interactive_input;
  pdv_heat_problem.ctrl.interactive_output = Interactive_output;
  sprintf(filename, "%s", "problem_heat.dat");
  pdr_heat_problem_read(Work_dir, filename, Interactive_output,
			   &pdv_heat_problem, nr_sol);

  // check data 

  fprintf(Interactive_output, 
	  "\nHEAT problem %d settings :\n", pdv_heat_problem.ctrl.name);

  fprintf(Interactive_output, "\nCONTROL PARAMETERS:\n");

  fprintf(Interactive_output, "\tmesh_type:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.mesh_type);
  fprintf(Interactive_output, "\tmesh_file_in:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.mesh_filename);
  fprintf(Interactive_output, "\tfield_file_in:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.field_filename);
  fprintf(Interactive_output, "\tbc_file:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.bc_filename);


  fprintf(Interactive_output, "\tmesh_file_out:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.mesh_dmp_filepattern);
  fprintf(Interactive_output, "\tfield_file_out:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.field_dmp_filepattern);

  fprintf(Interactive_output,"\n\tpenalty for Dirichlet BCs:\t\t%lf\n\n",
	  pdv_heat_problem.ctrl.penalty);


  /****************************************/
  /* initialization of material data      */
  /****************************************/
  if(strlen(pdv_heat_problem.ctrl.material_filename)==0){

    // e.g. for non-dimensional form of NS equations material files are not used
    pdv_heat_problem.ctrl.ref_temperature = -1.0;

  }
  else{

    iaux = pdr_heat_material_read(pdv_heat_problem.ctrl.work_dir, 
				  pdv_heat_problem.ctrl.material_filename, 
                  pdv_heat_problem.ctrl.interactive_output);
    if (iaux == EXIT_FAILURE) exit(-1);
    
    fprintf(Interactive_output, "\nmaterials configuration file:\t\t\t%s\n",
	    pdv_heat_problem.ctrl.material_filename);
//    fprintf(Interactive_output, "\tnumber of materials:\t\t\t%d\n\n",
//	    pdv_heat_problem.materials.materials_num);
    
    fprintf(Interactive_output, "HEAT material database read\n");
    
  }

  // reference temperature used to indicate whether material database is used
  if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){

    fprintf(Interactive_output, "\tmaterial data not temperature dependent\n");
    fprintf(Interactive_output, "\n\tthermal_conductivity:\t\t%lf\n",
	    pdv_heat_problem.ctrl.thermal_conductivity);
    fprintf(Interactive_output, "\n\tdensity:\t\t%lf\n",
	    pdv_heat_problem.ctrl.density);
    fprintf(Interactive_output, "\n\tspecific_heat:\t\t%lf\n",
	    pdv_heat_problem.ctrl.specific_heat);
   
  }
  else{

    fprintf(Interactive_output, "\tmaterial data from material database\n");
    pdv_heat_problem.ctrl.thermal_conductivity = -1.0;
    pdv_heat_problem.ctrl.density = -1.0;
    pdv_heat_problem.ctrl.specific_heat = -1.0;
    
    fprintf(Interactive_output, "\treference_temperature:\t\t\t%lf\n", 
	    pdv_heat_problem.ctrl.ref_temperature);

  }

  fprintf(Interactive_output, "\tambient temperature:\t\t\t%lf\n", 
	  pdv_heat_problem.ctrl.ambient_temperature);

  fprintf(Interactive_output, "\nTIME INTEGRATION PARAMETERS:\n");

  fprintf(Interactive_output, "\ttype:\t\t\t\t\t%d\n", 
	  pdv_heat_problem.time.type);
  fprintf(Interactive_output, "\timplicitness parameter:\t\t\t%lf\n\n", 
	  pdv_heat_problem.time.alpha);

  fprintf(Interactive_output, "\tcurrent timestep:\t\t\t%d\n", 
	  pdv_heat_problem.time.cur_step);
  fprintf(Interactive_output, "\tcurrent time:\t\t\t\t%lf\n", 
	  pdv_heat_problem.time.cur_time);
  fprintf(Interactive_output, "\n\tcurrent timestep_length:\t\t%lf\n", 
	  pdv_heat_problem.time.cur_dtime);
  fprintf(Interactive_output, "\tprevious timestep_length:\t\t%lf\n\n", 
	  pdv_heat_problem.time.prev_dtime);

  fprintf(Interactive_output, "\tfinal time:\t\t\t\t%lf\n", 
	  pdv_heat_problem.time.final_time);
  fprintf(Interactive_output, "\tfinal timestep:\t\t\t\t%d\n", 
	  pdv_heat_problem.time.final_step);

  fprintf(Interactive_output, "\n\tconvergence criterion type:\t\t%d\n", 
	  pdv_heat_problem.time.conv_type);
  fprintf(Interactive_output, "\terror tolerance (n-epsilon):\t\t%lf\n\n", 
	  pdv_heat_problem.time.conv_meas);

  fprintf(Interactive_output, "\tmonitoring level:\t\t\t%d\n\n", 
	  pdv_heat_problem.time.monitor);

  fprintf(Interactive_output, "\tgraph_dump_intv:\t\t\t%d\n", 
	  pdv_heat_problem.time.intv_graph);
  fprintf(Interactive_output, "\tfull_dump_intv:\t\t\t\t%d\n\n", 
	  pdv_heat_problem.time.intv_dumpout);

  fprintf(Interactive_output, "\nNONLINEAR SOLVER PARAMETERS:\n");

  fprintf(Interactive_output, "\ttype:\t\t\t\t\t%d\n", 
	  pdv_heat_problem.nonl.type);

  fprintf(Interactive_output, "\tmax_nonl_iter:\t\t\t\t%d\n", 
	  pdv_heat_problem.nonl.max_iter);

  fprintf(Interactive_output, "\n\tconvergence criterion type:\t\t%d\n", 
	  pdv_heat_problem.nonl.conv_type);
  fprintf(Interactive_output, "\terror tolerance (k-epsilon):\t\t%lf\n", 
	  pdv_heat_problem.nonl.conv_meas);
  fprintf(Interactive_output, "\tmonitoring level:\t\t\t%d\n\n", 
	  pdv_heat_problem.nonl.monitor);


  fprintf(Interactive_output, "\nLINEAR SOLVER PARAMETERS:\n");

  fprintf(Interactive_output, "\n\tsolver type:\t\t\t\t%d\n", 
	  pdv_heat_problem.lins.type);

  fprintf(Interactive_output, "\tsolver_file:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.solver_filename);

  if(pdv_heat_problem.lins.type!=0){
    fprintf(Interactive_output, "\n\tmax_lins_iter:\t\t\t\t%d\n", 
	    pdv_heat_problem.lins.max_iter);
    fprintf(Interactive_output, "\tconvergence criterion type:\t\t%d\n", 
	    pdv_heat_problem.lins.conv_type);
    fprintf(Interactive_output, "\terror tolerance:\t\t\t%.15lf\n", 
	    pdv_heat_problem.lins.conv_meas);
  }

  fprintf(Interactive_output, "\tmonitoring level:\t\t\t%d\n\n", 
	  pdv_heat_problem.lins.monitor);


  fprintf(Interactive_output, "\nADAPTATION PARAMETERS:\n");

  fprintf(Interactive_output, "\tadapt_type:\t\t\t\t%d\n", 
	  pdv_heat_problem.adpt.type);
  fprintf(Interactive_output, "\tadapt_interval:\t\t\t\t%d\n", 
	  pdv_heat_problem.adpt.interval);
  fprintf(Interactive_output, "\tadapt_eps:\t\t\t\t%lf\n", 
	  pdv_heat_problem.adpt.eps);
  fprintf(Interactive_output, "\tadapt_ratio:\t\t\t\t%lf\n\n", 
	  pdv_heat_problem.adpt.ratio);

  fprintf(Interactive_output, "\tmonitoring level:\t\t\t%d\n\n", 
	  pdv_heat_problem.adpt.monitor);
  
  
  /****************************************/
  /* initialization of bc data            */
  /****************************************/
  
  iaux = pdr_heat_bc_read(pdv_heat_problem.ctrl.work_dir, 
			     pdv_heat_problem.ctrl.bc_filename, 
			     pdv_heat_problem.ctrl.interactive_output, 
			     &pdv_heat_problem.bc);
  if (iaux == EXIT_FAILURE) exit(-1);

  fprintf(Interactive_output, "\nboundary conditions configuration file:\t\t%s\n", 
	  pdv_heat_problem.ctrl.bc_filename);
  fprintf(Interactive_output, "\tnumber of BCs:\t\t\t\t%d\n\n", 
	  pdr_heat_get_bc_assign_count(&pdv_heat_problem.bc));


  fprintf(Interactive_output, "HEAT BC OK\n");
  

  /****************************************/
  /* initialization of mesh data          */
  /****************************************/

  mesh_id = utr_initialize_mesh( Interactive_output, Work_dir, 
				pdv_heat_problem.ctrl.mesh_type[0], 
				 pdv_heat_problem.ctrl.mesh_filename);
  pdv_heat_problem.ctrl.mesh_id = mesh_id;

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


  mmr_set_max_gen(mesh_id, pdv_heat_problem.adpt.maxgen);
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

