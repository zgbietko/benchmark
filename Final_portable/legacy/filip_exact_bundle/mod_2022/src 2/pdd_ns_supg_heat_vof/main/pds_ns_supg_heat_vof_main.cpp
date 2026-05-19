/************************************************************************
File pds_ns_supg_heat_vof_main.c - driver for ns_supg_heat_vof module - approximation
      of Navier_Stokes equations with heat transfer, convection and volume of fluid

Considered to be a model coupled problem with following ingredients:
 ns_supg -> problem_id = PDC_NS_SUPG_ID (1) (ns_supg problem=module)
 heat -> problem_id = PDC_HEAT (2) (heat problem=module)
 heat_dtdt -> problem_id = PDC_HEAT_DTDT (3) (heating cooling problem=no module)
 vof  -> problem_id = PDC_VOF (4) (vof problem=module)
 mat -> problem_id = PDC_MAT (5) (material problem=no module)

Contains definition of global variables - among them:
  pdv_ns_supg_problem - problem structure for ns_supg problem 
  pdv_heat_problem - problem structure for heat problem
  pdv_heat_dtdt_problem - structure for heating-cooling problem
  pdv_vof_problem - structure for volume of fluid problem
  pdv_mat_problem - structure for material problem
					      
Contains definition of routines:
  main

Procedures local to main:
  pdr_ns_supg_heat_vof_init - initialize problems data
  pdr_ns_supg_heat_vof_post_process
  pdr_ns_supg_heat_vof_profile
  pdr_ns_supg_heat_vof_initial_condition
  pdr_ns_supg_heat_vof_bc_free
  pdr_ns_supg_heat_vof_material_free

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
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include<string.h>
#include<signal.h>

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
#ifdef MOD_FEM_VIEWER
#include "mod_fem_viewer.h"	/* USES */
#endif

/* problem module's types and functions */
#include "../include/pdh_ns_supg_heat_vof.h"		/* USES */
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
//#include "../../pdd_vof/include/pdh_vof_problem.h" 
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
double pdv_ns_supg_heat_vof_timer_all = 0.0;
double pdv_ns_supg_heat_vof_timer_pdr_comp_el_stiff_mat = 0.0;
double pdv_ns_supg_heat_vof_timer_pdr_comp_fa_stiff_mat = 0.0;

// ID of the current problem
// on purpose initialized to 0 which is wrong value !
// later should be replaced by one of the four proper values:
// ns_supg -> problem_id = PDC_NS_SUPG_ID = 1
// heat -> problem_id = PDC_HEAT_ID = 2
// heat_dtdt -> problem_id = PDC_HEAT_DTDT_ID = 3
// vof -> problem_id = PDC_VOF_ID = 4
// mat -> problem_id = PDC_MAT_ID = 5
int pdv_ns_supg_heat_vof_current_problem_id = 0;	/* ID of the current problem */
// problem structure for ns_supg module
pdt_ns_supg_problem pdv_ns_supg_problem;
// problem structure for heat module
pdt_heat_problem pdv_heat_problem;
// problem structure for heating-cooling
pdt_heat_dtdt_problem pdv_heat_dtdt_problem;
// problem structure for vof module
pdt_vof_problem pdv_vof_problem;
// problem structure for material
pdt_mat_problem pdv_mat_problem;

// flag indicating that SIGINT was not caught
int pdv_ns_supg_SIGINT_not_caught;

#ifdef PARALLEL
  int pdv_nr_proc;
  int pdv_my_proc_id;
#endif

/***************************************/
/* DECLARATIONS OF INTERNAL PROCEDURES */
/***************************************/
/* Rules:
/* - name always begins with pdr_ns_supg_heat_vof */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_ns_supg_heat_vof_post_process - simple post-processing
------------------------------------------------------------*/
double pdr_ns_supg_heat_vof_post_process(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/*------------------------------------------------------------
pdr_ns_supg_heat_vof_profile - to dump a set of values along a line
------------------------------------------------------------*/
int pdr_ns_supg_heat_vof_write_profile(
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
pdr_vof_initial_condition - procedure passed as argument
  to field initialization routine in order to provide problem
  dependent initial condition data
------------------------------------------------------------*/
double pdr_vof_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Coor,   // point coordinates
  int Sol_comp_id // solution component
);

/*------------------------------------------------------------
pdr_initialize_mesh_mat - mesh material initialization routine
------------------------------------------------------------*/
int pdr_initialize_mesh_mat(
  int Mesh_id, // mesh_id - each problem should know its mesh id
  int (*Fun_p)(double*) // pointer to function that provides
  // problem dependent initial material numbers
);


/*------------------------------------------------------------
pdr_mat_initial_condition - procedure passed as argument
  to mesh initialization routine in order to provide problem
  dependent initial material numbers
------------------------------------------------------------*/
double pdr_mat_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *X_coor,   // point coordinates
  int Sol_comp_id // solution component
);

/*------------------------------------------------------------
pdr_material_init - procedure passed as argument
  to mesh initialization routine in order to provide problem
  dependent initial material numbers
------------------------------------------------------------*/
int pdr_material_init(
  double *X_coor   // point coordinates
);

/*------------------------------------------------------------
 TODO: not implemented yet
------------------------------------------------------------*/
int  pdr_ns_supg_heat_vof_bc_free();

/*------------------------------------------------------------
 TODO: not implemented yet
------------------------------------------------------------*/
int  pdr_ns_supg_heat_vof_material_free();

/***************************************/
/* DEFINITIONS OF PROCEDURES */
/***************************************/
/****************************************/
/* Ctrl+C handling functtion            */
/****************************************/
void pdr_ns_supg_ctrl_c_signal_handler(int param)
{
  char c=0;

  signal(SIGINT,SIG_DFL);
  
  printf("\nCought Ctrl+C signal!\nPress key and confirm it by pressing [Enter]:\n[c] or [q] to quit now\n[m] to finalize current step and display menu\n[r] or any other key to resume(default) : ");
  c=getchar();
  switch(c) {
  case 3: // Ctrl-C code
  case '^':
  case 'q':
  case 'c': {
	printf("\nExiting application!\n");
	exit(-1);
  }
	break;
  case 'm': {
	pdv_ns_supg_SIGINT_not_caught = 0;
	printf("\nFinalizing current progress and entering menu...");
  }
	break;
  case 27: // esc
  default: {
	printf("\nResuming execution...");
	pdv_ns_supg_SIGINT_not_caught = 1;
  }
	break;
  } //!switch

  signal(SIGINT,pdr_ns_supg_ctrl_c_signal_handler);
  
  return;
}

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

/* initial setting for interactive_input and interactive_output */
  sprintf(interactive_input_name,"%s/input_interactive.txt",work_dir);
/*kbw
  printf("input file: %s\n", interactive_input_name);
/*kew*/
  interactive_input = fopen(interactive_input_name,"r");
  if(interactive_input==NULL){
    interactive_input = stdin;
    interactive_output = stdout;
    
    /****************************************/
    /* Ctrl+C signal handling (only stdin)  */
    signal(SIGINT,pdr_ns_supg_ctrl_c_signal_handler);
    /****************************************/  
    

  } else {
    fscanf(interactive_input,"%s\n",tmp);
    if(strcmp(tmp,"STDOUT")==0 ||
       strcmp(tmp,"stdout")==0) interactive_output = stdout;
    else{
/*kbw
  printf("output file: %s\n", interactive_output_name);
/*kew*/
      sprintf(interactive_output_name,"%s/%s", work_dir, tmp);
    }
  }
/*kbw
  printf("output file: %s\n", interactive_output_name);
/*kew*/

#ifdef PARALLEL
/* first initiate parallel execution */
  info = pcr_init_parallel(&argc, argv, work_dir, 
			   interactive_output_name, &interactive_output,
			   &pdv_nr_proc, &pdv_my_proc_id);
/*kbw
  printf("output file: %s\n", interactive_output_name);
/*kew*/

  if(info) {
    fprintf(interactive_output,
	  "Unable to initiate parallel communication module! Exiting.\n");
    exit(0);
  }
/*kbw
  fprintf(interactive_output,
	  "\nInitiated parallel communication: nr_proc %d, my_proc_id %d\n", 
	  pdv_nr_proc, pdv_my_proc_id);
/*kew*/
  if(pdv_my_proc_id==pcr_print_master()){
    if(interactive_input == stdin){
      printf("\nWARNING! Input from STDIN in PARALLEL mode!\n");
      printf("May lead to deadlock in batch execution!\n");
      //printf("Press any key to continue.\n");
      //getchar();getchar();
    }
  }
#endif

#ifndef PARALLEL
  if(interactive_output != stdout){
    interactive_output = fopen(interactive_output_name,"w");
/*kbw
  printf("output file: %s\n", interactive_output_name);
/*kew*/
  }
#endif

  if(interactive_input == NULL || interactive_output == NULL){
    fprintf(stderr, 
"Cannot establish interactive input and/or interactive output. Exiting\n");
    exit(0);
  }

  if(strcmp(work_dir,".")==0){
    fprintf(interactive_output, 
"No special working directory specified in command line\n");
    fprintf(interactive_output, 
"Current directory: %s , assumed as working directory\n", work_dir);
  } else {
    fprintf(interactive_output, 
"Working directory specified in command line: %s\n", work_dir);
  }

  if(interactive_input == stdin){
    fprintf(interactive_output, 
"Interactive input from terminal (stdin)\nInteractive output to stdout\n");
  } else {
    if(interactive_output == stdout){
      fprintf(interactive_output, 
	      "Interactive input from file: %s\nInteractive output to stdout\n",
	      interactive_input_name);
    } else {
      fprintf(interactive_output, 
	      "Interactive input from file: %s\n",
	      interactive_input_name);
      fprintf(interactive_output, 
	      "Interactive output to file: %s\n",
	      interactive_output_name);
    }
  }

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

  // initialize ns_supg, heat and vof structures
  iaux=pdr_ns_supg_heat_vof_init(work_dir,
			     interactive_input, interactive_output);
  if (iaux == EXIT_FAILURE) exit(-1);

  // create Voronoi diagram for cell nodes
  //pdr_vof_problem_voronoi(&pdv_vof_problem);


  //printf("\n\tAS: heat_dtdt:\nfield_id : %d, mesh_id : %d, nr_sol : %d, name : %s, nreq : %d, solver_id : %d, ", pdv_heat_dtdt_problem.ctrl.field_id, pdv_heat_dtdt_problem.ctrl.mesh_id, pdv_heat_dtdt_problem.ctrl.nr_sol, pdv_heat_dtdt_problem.ctrl.name, pdv_heat_dtdt_problem.ctrl.nreq, pdv_heat_dtdt_problem.ctrl.solver_id);

  /****************************************/
  /* main menu loop                       */
  /****************************************/
  do {
#ifdef PARALLEL
    if (pdv_my_proc_id == pcr_print_master()) {
#endif
      if (interactive_input == stdin) {
	do {
	  printf("\nChoose a command from the menu:\n");
	  printf("\ts - solve the problem (time integration)\n");
	  printf("\te - compute error\n");
	  printf("\tp - postprocessing\n");
#ifdef MOD_FEM_VIEWER
	  printf("\tg - visualize (MODFEMVIEWER)\n");
#endif
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
	        && c != 'v' && c != 'c' && c != 'q' && c != 'q');
      } else {
	fscanf(interactive_input, "%c\n", &c);
      }
#ifdef PARALLEL
    }
    pcr_bcast_char(pcr_print_master(), 1, &c);
    //printf("After BCAST %c\n",c);
#endif

    /*------------------------------------------------------------*/
    if (c == 's') {
		//for(int i=0; i<pdv_ns_supg_problem.adpt.maxgen; ++ i) {
// 		for(int i=0; i<2; ++ i) {
// 		  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
// 		  int ref_type = -2;
// 		  pdr_ns_supg_heat_vof_refine(pdv_ns_supg_heat_vof_current_problem_id, ref_type,
// 				  interactive_output);
// 		  // initialization of material in mesh structure */
// 		  pdr_initialize_mesh_mat(pdv_mat_problem.ctrl.mesh_id, pdr_material_init);
// 		  apr_set_ini_con(PDC_NS_SUPG_ID, pdr_ns_supg_initial_condition);
// 		  apr_set_ini_con(PDC_HEAT_ID, pdr_heat_initial_condition);
// 		  apr_set_ini_con(PDC_VOF_ID, pdr_vof_initial_condition);
// 		  apr_set_ini_con(PDC_MAT_ID, pdr_mat_initial_condition);
// 		}

// 		for(int i=0; i<=2 /*pdv_vof_problem.adpt.maxgen*/; ++ i) {
// 		fprintf(interactive_output, "\nUniform refinement(%d)", i+1);
// 		pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
// 		int ref_type = -1;
// 		pdr_ns_supg_heat_vof_refine(pdv_ns_supg_heat_vof_current_problem_id, 
// 		  ref_type, interactive_output);
// 		// initialization of material in mesh structure */
// 		pdr_initialize_mesh_mat(pdv_mat_problem.ctrl.mesh_id, 
// 		  pdr_material_init);
// 		apr_set_ini_con(PDC_NS_SUPG_ID, pdr_ns_supg_initial_condition);
// 		apr_set_ini_con(PDC_HEAT_ID, pdr_heat_initial_condition);
// 		apr_set_ini_con(PDC_VOF_ID, pdr_vof_initial_condition);
// 		apr_set_ini_con(PDC_MAT_ID, pdr_mat_initial_condition);
// 	  }

	  for(int i=0; i<2 /*pdv_vof_problem.adpt.maxgen*/; ++ i) {
		fprintf(interactive_output, "\nInterphase refinement(%d)", i+1);
		pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
		int ref_type = -2;
		pdr_ns_supg_heat_vof_refine(pdv_ns_supg_heat_vof_current_problem_id, 
		  ref_type, interactive_output);
		// initialization of material in mesh structure */
		pdr_initialize_mesh_mat(pdv_mat_problem.ctrl.mesh_id, 
		  pdr_material_init);
		apr_set_ini_con(PDC_NS_SUPG_ID, pdr_ns_supg_initial_condition);
		apr_set_ini_con(PDC_HEAT_ID, pdr_heat_initial_condition);
		apr_set_ini_con(PDC_VOF_ID, pdr_vof_initial_condition);
		apr_set_ini_con(PDC_MAT_ID, pdr_mat_initial_condition);
	  }
	  pdr_ns_supg_heat_vof_write_paraview(work_dir, 
		interactive_input, interactive_output);

	  pdv_ns_supg_SIGINT_not_caught = 1;

      pdv_ns_supg_heat_vof_timer_all = 0.0;
      pdv_ns_supg_heat_vof_timer_pdr_comp_el_stiff_mat = 0.0;
      pdv_ns_supg_heat_vof_timer_pdr_comp_fa_stiff_mat = 0.0;

      fprintf(interactive_output, 
	      "\nBeginning solution of coupled ns_supg-heat problem\n\n");

      pdv_ns_supg_heat_vof_timer_all = time_clock();

      /*---------- main time integration procedure ------------*/
      pdr_ns_supg_heat_vof_time_integration(work_dir, 
			   interactive_input, interactive_output);


      pdv_ns_supg_heat_vof_timer_all = time_clock() - pdv_ns_supg_heat_vof_timer_all;

      fprintf(interactive_output,"\nTime total: %lf\n", 
	      pdv_ns_supg_heat_vof_timer_all);
      fprintf(interactive_output,"Time in pdr_comp_el_stiff_mat: %lf [%lf%%]\n", 
	      pdv_ns_supg_heat_vof_timer_pdr_comp_el_stiff_mat, 
	      (pdv_ns_supg_heat_vof_timer_pdr_comp_el_stiff_mat 
	       / pdv_ns_supg_heat_vof_timer_all) * 100);
      fprintf(interactive_output,"Time in pdr_comp_fa_stiff_mat: %lf [%lf%%]\n", 
	      pdv_ns_supg_heat_vof_timer_pdr_comp_fa_stiff_mat, 
	      (pdv_ns_supg_heat_vof_timer_pdr_comp_fa_stiff_mat 
	       / pdv_ns_supg_heat_vof_timer_all) * 100);

    }
    /*------------------------------------------------------------*/
    else if (c == 'e') {

      pdr_ns_supg_heat_vof_ZZ_error(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'p') {

      pdr_ns_supg_heat_vof_post_process(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'g') {

#ifdef MOD_FEM_VIEWER
      fprintf(interactive_output, "\nStarting visualization\n");
      mod_fem_viewer(argc, argv);
#else
      fprintf(interactive_output, "\nVisualization module not present\n");
#endif

    }
    /*------------------------------------------------------------*/
    else if (c == 'm') {

      // perform manual mesh refinement using ns_supg as master problem 
      pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
      int ref_type = -1;
      pdr_ns_supg_heat_vof_refine(pdv_ns_supg_heat_vof_current_problem_id, ref_type,
			    interactive_output);
      // initialization of material in mesh structure */
      pdr_initialize_mesh_mat(pdv_mat_problem.ctrl.mesh_id, pdr_material_init);
      apr_set_ini_con(PDC_NS_SUPG_ID, pdr_ns_supg_initial_condition);
      apr_set_ini_con(PDC_HEAT_ID, pdr_heat_initial_condition);
      apr_set_ini_con(PDC_VOF_ID, pdr_vof_initial_condition);
      apr_set_ini_con(PDC_MAT_ID, pdr_mat_initial_condition);
    }
    /*------------------------------------------------------------*/
    else if (c == 'a') {

      pdr_ns_supg_heat_vof_adapt(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'r') {

      pdr_ns_supg_heat_vof_write_profile(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'd') {

      pdr_ns_supg_heat_vof_dump_data(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'v') {

      pdr_ns_supg_heat_vof_write_paraview(work_dir, 
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
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;
  iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,iaux);
  apr_free_field(field_id);

  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_DTDT_ID;
  iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,iaux);
  apr_free_field(field_id);

  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
  iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,iaux);
  apr_free_field(field_id);

  pdv_ns_supg_heat_vof_current_problem_id = PDC_VOF_ID;
  iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,iaux);
  apr_free_field(field_id);

//  pdv_ns_supg_heat_vof_current_problem_id = PDC_MAT_ID;
//  iaux=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,iaux);
//  apr_free_field(field_id);

  mesh_id = apr_get_mesh_id(field_id);
  mmr_free_mesh(mesh_id);

  fclose(interactive_input);
  fclose(interactive_output);

  pdr_ns_supg_heat_vof_bc_free();
  pdr_ns_supg_heat_vof_material_free();

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
  /* if(fabs(Xcoor[0]-0.5)<0.05){ */
  /*   return(1000.0); */
  /* } */
  /* else{ */
  /*   return (pdv_heat_problem.ctrl.ambient_temperature); */
  /* } */
  /* if(fabs(Xcoor[2]-0.5)<0.05){ // initial condition for cube 1x1x1 */
  /*   return(1.0); */
  /* } */
  /* else{ */
  /*   return (0.0); */
  /* } */

    return (pdv_heat_problem.ctrl.ambient_temperature);
}

/*------------------------------------------------------------
pdr_vof_initial_condition
------------------------------------------------------------*/
double pdr_vof_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Xcoor,   // point coordinates
  int Sol_comp_id // solution component
)
{

/*kbw
  printf("specified initial vof at point %lf, %lf, %lf : %lf\n",
	 Xcoor[0], Xcoor[1], Xcoor[2], 
	 pdv_heat_problem.ctrl.ambient_temperature); 
/*kew*/
  if( Xcoor[2] > 0.5 ) // initial condition for cube 1x1x1
    return(0.0);
  else
    return(1.0);
}

/*------------------------------------------------------------
pdr_mat_initial_condition
------------------------------------------------------------*/
double pdr_mat_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Xcoor,   // point coordinates
  int Sol_comp_id // solution component
)
{

/*kbw
  printf("specified initial vof at point %lf, %lf, %lf : %lf\n",
	 Xcoor[0], Xcoor[1], Xcoor[2], 
	 pdv_heat_problem.ctrl.ambient_temperature); 
/*kew*/
  if( Xcoor[1] > 0.0 ) // initial condition for cube 1x1x1
    return(1.0);
  else
    return(0.0);
}

/*------------------------------------------------------------
pdr_initialize_mesh_mat - mesh material initialization routine
------------------------------------------------------------*/
int pdr_initialize_mesh_mat(
  int Mesh_id, // mesh_id - each problem should know its mesh id
  int (*Fun_p)(double*) // pointer to function that provides
  // problem dependent initial material numbers
)
{
  int nel, el_nodes[MMC_MAXELVNO+1];
  double node_coor[3 * MMC_MAXELVNO];

  nel = 0;
  while( (nel = mmr_get_next_act_elem(Mesh_id, nel)) != 0 ) {
    mmr_el_node_coor(Mesh_id, nel, el_nodes, node_coor);
    mmr_el_set_groupID(Mesh_id, nel, (*Fun_p)(node_coor));
  }

  return(1);
}

/*------------------------------------------------------------
pdr_material_init
------------------------------------------------------------*/
int pdr_material_init(
  double *Xcoor   // point coordinates
)
{

/*kbw
  printf("specified initial vof at point %lf, %lf, %lf : %lf\n",
	 Xcoor[0], Xcoor[1], Xcoor[2], 
	 pdv_heat_problem.ctrl.ambient_temperature); 
/*kew*/
  if(Xcoor[1]<0.0)
    return(0);
  /* else if(Xcoor[2]<-0.0010) */
  /*   return (1.0); */
  else
    return(1);
}

/*------------------------------------------------------------
pdr_ns_supg_heat_vof_post_process
------------------------------------------------------------*/
double pdr_ns_supg_heat_vof_post_process(
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

  //AS: unknown cell_id
  iaux = apr_sol_xglob(field_id, x, 1, list_el, xg, u_val, NULL, NULL, NULL, 0);
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
pdr_ns_supg_heat_vof_write_profile
------------------------------------------------------------*/
int pdr_ns_supg_heat_vof_write_profile(
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
  
  pdv_ns_supg_heat_vof_current_problem_id = problem_id;
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
    
    return pdr_ns_supg_heat_vof_err_indi_explicit(Problem_id, El);
    
  } else if (Mode == PDC_ADAPT_ZZ) {
    
    return pdr_ns_supg_heat_vof_err_indi_ZZ(Problem_id, El);
    
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
  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      return (&pdv_ns_supg_problem);
    case PDC_HEAT_ID:
      return (&pdv_heat_problem);
    case PDC_HEAT_DTDT_ID:
      return (&pdv_heat_dtdt_problem);
    case PDC_VOF_ID:
      return (&pdv_vof_problem);
    case PDC_MAT_ID:
      return (&pdv_mat_problem);
    default:
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
  pdt_heat_dtdt_ctrls *ctrl_heat_dtdt = &pdv_heat_dtdt_problem.ctrl;
  pdt_ns_supg_ctrls *ctrl_ns_supg = &pdv_ns_supg_problem.ctrl;
  pdt_vof_ctrls *ctrl_vof = &pdv_vof_problem.ctrl;
  pdt_mat_ctrls *ctrl_mat = &pdv_mat_problem.ctrl;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 1:  return (ctrl_ns_supg->name);
        case 2:  return (ctrl_ns_supg->mesh_id);
        case 3:  return (ctrl_ns_supg->field_id);
        case 4:  return (ctrl_ns_supg->nr_sol);
        case 5:  return (ctrl_ns_supg->nreq);
        case 6:  return (ctrl_ns_supg->solver_id);
        default:
	  printf("Wrong parameter number in ctrl_i_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 1:  return (ctrl_heat->name);
        case 2:  return (ctrl_heat->mesh_id);
        case 3:  return (ctrl_heat->field_id);
        case 4:  return (ctrl_heat->nr_sol);
        case 5:  return (ctrl_heat->nreq);
        case 6:  return (ctrl_heat->solver_id);
        default:
	  printf("Wrong parameter number in ctrl_i_params!");
	  exit(1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 1:  return (ctrl_vof->name);
        case 2:  return (ctrl_vof->mesh_id);
        case 3:  return (ctrl_vof->field_id);
        case 4:  return (ctrl_vof->nr_sol);
        case 5:  return (ctrl_vof->nreq);
        case 6:  return (ctrl_vof->solver_id);
        default:
	  printf("Wrong parameter number in ctrl_i_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_DTDT_ID:
      switch(Num) {
        case 1:  return (ctrl_heat_dtdt->name);
        case 2:  return (ctrl_heat_dtdt->mesh_id);
        case 3:  return (ctrl_heat_dtdt->field_id);
        case 4:  return (ctrl_heat_dtdt->nr_sol);
        case 5:  return (ctrl_heat_dtdt->nreq);
        case 6:  return (ctrl_heat_dtdt->solver_id);
        default:
	  printf("Wrong parameter number in ctrl_i_params!");
	  exit(1);
      }
      break;
    case PDC_MAT_ID:
      switch(Num) {
        case 1:  return (ctrl_mat->name);
        case 2:  return (ctrl_mat->mesh_id);
        case 3:  return (ctrl_mat->field_id);
        case 4:  return (ctrl_mat->nr_sol);
        case 5:  return (ctrl_mat->nreq);
        case 6:  return (ctrl_mat->solver_id);
        default:
	  printf("Wrong parameter number in ctrl_i_params!");
	  exit(1);
      }
      break;
    default:
      printf("Wrong problem_id in ctr_i_params!");
      exit(1);
  }
  
  return (-1);
}

/*------------------------------------------------------------
pdr_ctrl_d_params - to return one of control parameters
------------------------------------------------------------*/
double pdr_ctrl_d_params(int Problem_id, int Num)
{
  pdt_heat_ctrls *ctrl_heat = &pdv_heat_problem.ctrl;
  pdt_vof_ctrls *ctrl_vof = &pdv_vof_problem.ctrl;
  pdt_heat_dtdt_ctrls *ctrl_dtdt_heat = &pdv_heat_dtdt_problem.ctrl;
  pdt_ns_supg_ctrls *ctrl_ns_supg = &pdv_ns_supg_problem.ctrl;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 10: return(ctrl_ns_supg->gravity_field[0]);
        case 11: return(ctrl_ns_supg->gravity_field[1]);
        case 12: return(ctrl_ns_supg->gravity_field[2]);
        case 20: return(ctrl_ns_supg->ref_temperature);
        default:
	  printf("Wrong parameter number in ctrl_d_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(20) {
        case 20: return(ctrl_heat->ref_temperature);
        default:
	  printf("Wrong parameter number in ctrl_d_params!");
	  exit(1);
      }
      break;
    default:
      printf("Wrong problem_id in ctr_d_params!");
      exit(1);
  }

  return(0.0);
}

/*------------------------------------------------------------
pdr_adapt_i_params - to return parameters of adaptation
------------------------------------------------------------*/
int pdr_adapt_i_params(int Problem_id, int Num)
{

  pdt_heat_adpts *adpts_heat = &pdv_heat_problem.adpt;
  pdt_vof_adpts *adpts_vof = &pdv_vof_problem.adpt;
  pdt_heat_dtdt_adpts *adpts_heat_dtdt = &pdv_heat_dtdt_problem.adpt;
  pdt_ns_supg_adpts *adpts_ns_supg = &pdv_ns_supg_problem.adpt;
  pdt_mat_adpts *adpts_mat = &pdv_mat_problem.adpt;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 1:
	  return(adpts_ns_supg->type);
        case 2:
	  return(adpts_ns_supg->interval);
        case 3:
          return(adpts_ns_supg->maxgen);
        case 7:
	  return(adpts_ns_supg->monitor);
        default:
	  printf("Wrong parameter number in adapt_i_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 1:
	  return(adpts_heat->type);
        case 2:
          return(adpts_heat->interval);
        case 3:
	  return(adpts_heat->maxgen);
        case 7:
          return(adpts_heat->monitor);
        default:
	  printf("Wrong parameter number in adapt_i_params!");
	  exit(1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 1:
	  return(adpts_vof->type);
        case 2:
          return(adpts_vof->interval);
        case 3:
	  return(adpts_vof->maxgen);
        case 7:
          return(adpts_vof->monitor);
        default:
	  printf("Wrong parameter number in adapt_i_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_DTDT_ID:
      switch(Num) {
        case 1:
	  return(adpts_heat_dtdt->type);
        case 2:
	  return(adpts_heat_dtdt->interval);
        case 3:
	  return(adpts_heat_dtdt->maxgen);
        case 7:
	  return(adpts_heat_dtdt->monitor);
        default:
	  printf("Wrong parameter number in adapt_i_params!");
	  exit(1);
      }
      break;
    case PDC_MAT_ID:
      switch(Num) {
        case 1:
	  return(adpts_mat->type);
        case 2:
          return(adpts_mat->interval);
        case 3:
	  return(adpts_mat->maxgen);
        case 7:
          return(adpts_mat->monitor);
        default:
	  printf("Wrong parameter number in adapt_i_params!");
	  exit(1);
      }
      break;
    default:
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
  pdt_vof_adpts *adpts_vof = &pdv_vof_problem.adpt;
  pdt_heat_dtdt_adpts *adpts_heat_dtdt = &pdv_heat_dtdt_problem.adpt;
  pdt_ns_supg_adpts *adpts_ns_supg = &pdv_ns_supg_problem.adpt;
  pdt_mat_adpts *adpts_mat = &pdv_mat_problem.adpt;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 5:
	  return (adpts_ns_supg->eps);
        case 6:
	  return (adpts_ns_supg->ratio);
        default:
	  printf("Wrong parameter number in adapt_d_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 5:
	  return (adpts_heat->eps);
        case 6:
	  return (adpts_heat->ratio);
        default:
	  printf("Wrong parameter number in adapt_d_params!");
	  exit(1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 5:
	  return (adpts_vof->eps);
        case 6:
	  return (adpts_vof->ratio);
        default:
	  printf("Wrong parameter number in adapt_d_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_DTDT_ID:
      switch(Num) {
        case 5:
	  return (adpts_heat_dtdt->eps);
        case 6:
	  return (adpts_heat_dtdt->ratio);
        default:
	  printf("Wrong parameter number in adapt_d_params!");
	  exit(1);
      }
      break;
    case PDC_MAT_ID:
      switch(Num) {
        case 5:
	  return (adpts_mat->eps);
        case 6:
	  return (adpts_mat->ratio);
        default:
	  printf("Wrong parameter number in adapt_d_params!");
	  exit(1);
      }
      break;
    default:
      printf("Wrong problem_id in adapt_d_params!");
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
  pdt_vof_times *times_vof = &pdv_vof_problem.time;
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 1:
	  return (times_ns_supg->type);
        case 3:
	  return (times_ns_supg->cur_step);
        case 4:
	  return (times_ns_supg->final_step);
        case 9:
	  return (times_ns_supg->conv_type);
        case 11:
	  return (times_ns_supg->monitor);
        default:
	  printf("Wrong parameter number in time_i_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 1:
	  return (times_heat->type);
        case 3:
	  return (times_heat->cur_step);
        case 4:
	  return (times_heat->final_step);
        case 9:
	  return (times_heat->conv_type);
        case 11:
	  return (times_heat->monitor);
        default:
	  printf("Wrong parameter number in time_i_params!");
	  exit(1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 1:
	  return (times_vof->type);
        case 3:
	  return (times_vof->cur_step);
        case 4:
	  return (times_vof->final_step);
        case 9:
	  return (times_vof->conv_type);
        case 11:
	  return (times_vof->monitor);
        default:
	  printf("Wrong parameter number in time_i_params!");
	  exit(1);
      }
      break;
    default:
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
  pdt_vof_times *times_vof = &pdv_vof_problem.time;
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 2:
	  return (times_ns_supg->alpha);
        default:
	  printf("Wrong parameter number in time_d_params!");
	exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 2:
	  return (times_heat->alpha);
        default:
	  printf("Wrong parameter number in time_d_params!");
	  exit(1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 2:
	  return (times_vof->alpha);
        default:
	  printf("Wrong parameter number in time_d_params!");
	  exit(1);
      }
      break;
    default:
      printf("Wrong problem_id in time_d_params!");
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
  pdt_vof_times *times_vof = &pdv_vof_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
	case 1:  times_ns_supg->type=Value;
	  break;
        case 2:  times_ns_supg->cur_step=Value;
	  break;
        case 3:  times_ns_supg->final_step=Value;
	  break;
        case 8:  times_ns_supg->conv_type=Value;
	  break;
        case 10: times_ns_supg->monitor=Value;
	  break;
        case 11: times_ns_supg->intv_dumpout=Value;
	  break;
        case 12: times_ns_supg->intv_graph=Value;
	  break;
        default:
	  printf("Wrong parameter number in set_time_i_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 1:  times_heat->type=Value;
	  break;
        case 2:  times_heat->cur_step=Value;
	  break;
       case 3:  times_heat->final_step=Value;
	 break;
        case 8: times_heat->conv_type=Value;
	  break;
        case 10: times_heat->monitor=Value;
	  break;
        case 11: times_heat->intv_dumpout=Value;
	  break;
        case 12: times_heat->intv_graph=Value;
	  break;
        default:
	    printf("Wrong parameter number in set_time_i_params!");
	  exit(1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 1:  times_vof->type=Value;
	  break;
        case 2:  times_vof->cur_step=Value;
	  break;
        case 3:  times_vof->final_step=Value;
	  break;
        case 8: times_vof->conv_type=Value;
	  break;
        case 10: times_vof->monitor=Value;
	  break;
        case 11: times_vof->intv_dumpout=Value;
	  break;
        case 12: times_vof->intv_graph=Value;
	  break;
        default:
	    printf("Wrong parameter number in set_time_i_params!");
	  exit(1);
      }
      break;
    default:
      printf("Wrong problem_id in set_time_i_params!");
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
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;
  pdt_heat_times *times_heat = &pdv_heat_problem.time;
  pdt_vof_times *times_vof = &pdv_vof_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 4:  times_ns_supg->cur_time=Value;
	  break;
        case 5:  times_ns_supg->final_time=Value;
	  break;
        case 6:  times_ns_supg->cur_dtime=Value;
	  break;
        case 7:  times_ns_supg->prev_dtime=Value;
	  break;
        case 9:  times_ns_supg->conv_meas=Value;
	  break;
        default:
	  printf("Wrong parameter number in set_time_d_params!");
	  exit(1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 4:  times_heat->cur_time=Value;
	  break;
        case 5:  times_heat->final_time=Value;
	  break;
        case 6:  times_heat->cur_dtime=Value;
	  break;
        case 7:  times_heat->prev_dtime=Value;
	 break;
        case 9:  times_heat->conv_meas=Value;
	  break;
        default:
	  printf("Wrong parameter number in set_time_d_params!");
	  exit(1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 4:  times_vof->cur_time=Value;
	  break;
        case 5:  times_vof->final_time=Value;
	  break;
        case 6:  times_vof->cur_dtime=Value;
	  break;
        case 7:  times_vof->prev_dtime=Value;
	 break;
        case 9:  times_vof->conv_meas=Value;
	  break;
        default:
	  printf("Wrong parameter number in set_time_d_params!");
	  exit(1);
      }
      break;
    default:
      printf("Wrong problem_id in set_time_d_params!");
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
  pdt_ns_supg_linss *linss_ns_supg = &pdv_ns_supg_problem.lins;
  pdt_heat_linss *linss_heat = &pdv_heat_problem.lins;
  pdt_vof_linss *linss_vof = &pdv_vof_problem.lins;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 1: return(linss_ns_supg->type);
        case 2: return(linss_ns_supg->max_iter);
        case 3: return(linss_ns_supg->conv_type);
        case 5: return(linss_ns_supg->monitor);
        default:
	  printf("Wrong parameter number in lins_i_params!");
	  exit(-1);

      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 1: return(linss_heat->type);
        case 2: return(linss_heat->max_iter);
        case 3: return(linss_heat->conv_type);
        case 5: return(linss_heat->monitor);
        default:
	  printf("Wrong parameter number in lins_i_params!");
	  exit(-1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 1: return(linss_vof->type);
        case 2: return(linss_vof->max_iter);
        case 3: return(linss_vof->conv_type);
        case 5: return(linss_vof->monitor);
        default:
	  printf("Wrong parameter number in lins_i_params!");
	  exit(-1);
      }
      break;
    default:
      printf("Wrong problem_id in lins_i_params!");
      exit(1);
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
  pdt_vof_linss *linss_vof = &pdv_vof_problem.lins;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  switch(Problem_id) {
    case PDC_NS_SUPG_ID:
      switch(Num) {
        case 4:  return(linss_ns_supg->conv_meas);
        default:
	  printf("Wrong parameter number in lins_d_params!");
	  exit(-1);
      }
      break;
    case PDC_HEAT_ID:
      switch(Num) {
        case 4:  return(linss_heat->conv_meas);
        default:
	  printf("Wrong parameter number in lins_d_params!");
	  exit(-1);
      }
      break;
    case PDC_VOF_ID:
      switch(Num) {
        case 4:  return(linss_vof->conv_meas);
        default:
	  printf("Wrong parameter number in lins_d_params!");
	  exit(-1);
      }
      break;
    default:
      printf("Wrong problem_id in lins_d_params!");
      exit(1);
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
  pdt_vof_adpts *adpts_vof = &pdv_vof_problem.adpt;
  pdt_vof_times *times_vof = &pdv_vof_problem.time;
  pdt_vof_linss *linss_vof = &pdv_vof_problem.lins;
  pdt_vof_ctrls *ctrls_vof = &pdv_vof_problem.ctrl;
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
pdr_ns_supg_heat_vof_init - read problem data
------------------------------------------------------------*/
int pdr_ns_supg_heat_vof_init(
  char *Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
)
{

  FILE *testforfile;
  char filename[300], arg[300];
  int nr_sol; // number of solution vectors - determined by time integration
  int mesh_id, field_id, iaux;
  int i;

  // ns_supg_heat_vof uses following problem structures
  // first from ns_supg
  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
  pdr_ns_supg_problem_clear(&pdv_ns_supg_problem);

  // second from heat
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;
  pdr_heat_problem_clear(&pdv_heat_problem);

  // third from heating-cooling
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_DTDT_ID;
  pdr_heat_dtdt_problem_clear(&pdv_heat_dtdt_problem);

  // fourth from volume of fluid
  pdv_ns_supg_heat_vof_current_problem_id = PDC_VOF_ID;
  pdr_vof_problem_clear(&pdv_vof_problem);

  // fifth from material
  pdv_ns_supg_heat_vof_current_problem_id = PDC_MAT_ID;
  pdr_mat_problem_clear(&pdv_mat_problem);

  // first we read ns_supg problem structure
  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
  nr_sol = 3;
  strcpy(pdv_ns_supg_problem.ctrl.work_dir, Work_dir);
  pdv_ns_supg_problem.ctrl.interactive_input = Interactive_input;
  pdv_ns_supg_problem.ctrl.interactive_output = Interactive_output;
  sprintf(filename, "%s", "problem_ns_supg.dat");

  pdr_ns_supg_problem_read(Work_dir, filename, Interactive_output,
			   &pdv_ns_supg_problem, nr_sol);


  // second we read heat problem structure
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;
  nr_sol = 3;
  strcpy(pdv_heat_problem.ctrl.work_dir, Work_dir);
  pdv_heat_problem.ctrl.interactive_input = Interactive_input;
  pdv_heat_problem.ctrl.interactive_output = Interactive_output;
  sprintf(filename, "%s", "problem_heat.dat");
  pdr_heat_problem_read(Work_dir, filename, Interactive_output,
			   &pdv_heat_problem, nr_sol);

  // third we read dtdt problem structure
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_DTDT_ID;
  nr_sol = 3;
  strcpy(pdv_heat_dtdt_problem.ctrl.mesh_type, pdv_heat_problem.ctrl.mesh_type);
  pdv_heat_dtdt_problem.ctrl.name = pdv_heat_problem.ctrl.name;
  pdv_heat_dtdt_problem.ctrl.nr_sol = nr_sol;
  pdv_heat_dtdt_problem.ctrl.nreq = PDC_HEAT_DTDT_NREQ;

  // fourth we read vof problem structure
  pdv_ns_supg_heat_vof_current_problem_id = PDC_VOF_ID;
  nr_sol = 3;
  strcpy(pdv_vof_problem.ctrl.work_dir, Work_dir);
  pdv_vof_problem.ctrl.interactive_input = Interactive_input;
  pdv_vof_problem.ctrl.interactive_output = Interactive_output;
  sprintf(filename, "%s", "problem_vof.dat");
  pdr_vof_problem_read(Work_dir, filename, Interactive_output, &pdv_vof_problem, nr_sol);

  // fifth we read mat problem structure
  pdv_ns_supg_heat_vof_current_problem_id = PDC_MAT_ID;
  nr_sol = 3;
  strcpy(pdv_mat_problem.ctrl.mesh_type, pdv_vof_problem.ctrl.mesh_type);
  pdv_mat_problem.ctrl.interactive_input = Interactive_input;
  pdv_mat_problem.ctrl.interactive_output = Interactive_output;
  pdv_mat_problem.ctrl.name = pdv_vof_problem.ctrl.name;
  pdv_mat_problem.ctrl.nr_sol = nr_sol;
  strcpy(pdv_mat_problem.ctrl.field_filename, pdv_vof_problem.ctrl.field_filename);
  pdv_mat_problem.ctrl.nreq = PDC_MAT_NREQ;

  // check data (comparison data for ns_supg & heat)
  fprintf(Interactive_output, 
	  "\nNS_SUPG problem %d and HEAT problem %d settings :\n",
	  pdv_ns_supg_problem.ctrl.name, pdv_heat_problem.ctrl.name);

  fprintf(Interactive_output, "\nCONTROL PARAMETERS:\n");

  fprintf(Interactive_output, "\tmesh_type:\t\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_type);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.mesh_type,
		 pdv_ns_supg_problem.ctrl.mesh_type)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    strcpy(pdv_heat_problem.ctrl.mesh_type, pdv_ns_supg_problem.ctrl.mesh_type);
  }

  fprintf(Interactive_output, "\tmesh_file_in:\t\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_filename);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.mesh_filename,
		 pdv_ns_supg_problem.ctrl.mesh_filename)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    strcpy(pdv_heat_problem.ctrl.mesh_filename, 
	   pdv_ns_supg_problem.ctrl.mesh_filename);
  }

  fprintf(Interactive_output, "\n\tns_supg field_file_in:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.field_filename);
  fprintf(Interactive_output, "\theat field_file_in:\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.field_filename);
  fprintf(Interactive_output, "\n\tmaterials_file:\t\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.material_filename);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.material_filename,
		 pdv_ns_supg_problem.ctrl.material_filename)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    strcpy(pdv_heat_problem.ctrl.material_filename, 
	   pdv_ns_supg_problem.ctrl.material_filename);
  }
  fprintf(Interactive_output, "\n\tns_supg bc_file:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.bc_filename);
  fprintf(Interactive_output, "\theat bc_file:\t\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.bc_filename);


  fprintf(Interactive_output, "\tns_supg mesh_file_out:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern);
  if(0 !=  strcmp(pdv_heat_problem.ctrl.mesh_dmp_filepattern,
		 pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern)){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    strcpy(pdv_heat_problem.ctrl.mesh_dmp_filepattern, 
	   pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern);
  }
  fprintf(Interactive_output, "\tns_supg field_file_out:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.field_dmp_filepattern);
  fprintf(Interactive_output, "\theat field_file_out:\t\t\t%s\n", 
	  pdv_heat_problem.ctrl.field_dmp_filepattern);

  fprintf(Interactive_output,"\n\tpenalty for Dirichlet BCs:\t\t%lf\n\n",
	  pdv_ns_supg_problem.ctrl.penalty);
  if(pdv_heat_problem.ctrl.penalty != pdv_ns_supg_problem.ctrl.penalty){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.ctrl.penalty = pdv_ns_supg_problem.ctrl.penalty;
  }

  fprintf(Interactive_output, "\tns_supg gravity:\t\t\t%lf %lf %lf\n\n", 
	  pdv_ns_supg_problem.ctrl.gravity_field[0], 
	  pdv_ns_supg_problem.ctrl.gravity_field[1], 
	  pdv_ns_supg_problem.ctrl.gravity_field[2]);

  fprintf(Interactive_output, "\treference_temperature:\t\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.ref_temperature);
  if(pdv_heat_problem.ctrl.ref_temperature != 
                                   pdv_ns_supg_problem.ctrl.ref_temperature){
    printf("Inconsistency between ns_supg and heat for the above parameter!\n");
    printf("HEAT problem parameter overridden.\n");
    pdv_heat_problem.ctrl.ref_temperature = 
                                   pdv_ns_supg_problem.ctrl.ref_temperature;
  }
  fprintf(Interactive_output, "\theat ambient temperature:\t\t%lf\n", 
	  pdv_heat_problem.ctrl.ambient_temperature);

  fprintf(Interactive_output, "\n\tns_supg reference_density:\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.ref_density);
  fprintf(Interactive_output, "\tns_supg reference_length:\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.ref_length);
  fprintf(Interactive_output, "\n\tns_supg reference_velocity:\t\t%lf\n\n", 
	  pdv_ns_supg_problem.ctrl.ref_velocity);
  fprintf(Interactive_output, "\tns_supg dynamic viscosity:\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.dynamic_viscosity);
  fprintf(Interactive_output, "\tns_supg Reynolds number:\t\t%lf\n\n", 
	  pdv_ns_supg_problem.ctrl.reynolds_number);

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
    pdv_vof_problem.time.cur_step = pdv_ns_supg_problem.time.cur_step;
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
  /* initialization of material data      */
  /****************************************/

  // for non-dimensional form of NS equations materials are not used
  iaux = pdr_ns_supg_material_read(pdv_ns_supg_problem.ctrl.work_dir, 
				   pdv_ns_supg_problem.ctrl.material_filename, 
				   pdv_ns_supg_problem.ctrl.interactive_output, 
				   &pdv_ns_supg_problem.materials);
  if (iaux == EXIT_FAILURE) exit(-1);
  fprintf(Interactive_output, "NS_SUPG materials OK\n");

  iaux = pdr_heat_material_read(pdv_heat_problem.ctrl.work_dir, 
				   pdv_heat_problem.ctrl.material_filename, 
				   pdv_heat_problem.ctrl.interactive_output, 
				   &pdv_heat_problem.materials);
  if (iaux == EXIT_FAILURE) exit(-1);
  fprintf(Interactive_output, "HEAT materials OK\n");
  
  iaux = pdr_vof_material_read(pdv_vof_problem.ctrl.work_dir, 
				   pdv_vof_problem.ctrl.material_filename, 
				   pdv_vof_problem.ctrl.interactive_output, 
				   &pdv_vof_problem.materials);
  if (iaux == EXIT_FAILURE) exit(-1);
  fprintf(Interactive_output, "VOF materials OK\n");

  for ( i = 1; i <= pdv_vof_problem.ctrl.materials_used[0]; ++i) {
    // material number starts counting from 0
    if ( pdv_vof_problem.ctrl.materials_used[i] >= pdv_vof_problem.materials.materials_num ) {
	fprintf(Interactive_output, "There is no such material %d in database\n", pdv_vof_problem.ctrl.materials_used[i]);
	exit(-1);
    }
  }
  
  
  /****************************************/
  /* initialization of bc data            */
  /****************************************/
  
  iaux = pdr_ns_supg_bc_read(pdv_ns_supg_problem.ctrl.work_dir, 
			     pdv_ns_supg_problem.ctrl.bc_filename, 
			     pdv_ns_supg_problem.ctrl.interactive_output, 
			     &pdv_ns_supg_problem.bc);
  if (iaux == EXIT_FAILURE) exit(-1);
  fprintf(Interactive_output, "NS_SUPG BC OK\n");
  
  iaux = pdr_heat_bc_read(pdv_heat_problem.ctrl.work_dir, 
			     pdv_heat_problem.ctrl.bc_filename, 
			     pdv_heat_problem.ctrl.interactive_output, 
			     &pdv_heat_problem.bc);
  if (iaux == EXIT_FAILURE) exit(-1);
  fprintf(Interactive_output, "HEAT BC OK\n");
  
  iaux = pdr_vof_bc_read(pdv_vof_problem.ctrl.work_dir, 
			     pdv_vof_problem.ctrl.bc_filename, 
			     pdv_vof_problem.ctrl.interactive_output, 
			     &pdv_vof_problem.bc);
  if (iaux == EXIT_FAILURE) exit(-1);
  fprintf(Interactive_output, "VOF BC OK\n");
  
  fprintf(Interactive_output, "\nBoundary conditions configuration file:\t%s\n", 
	  pdv_ns_supg_problem.ctrl.bc_filename);
  fprintf(Interactive_output, "\tNumber of pressure pins:\t\t%d\n", 
	  pdr_ns_supg_get_pressure_pins_count(&pdv_ns_supg_problem.bc));
  fprintf(Interactive_output, "\tNumber of velocity pins:\t\t%d\n", 
	  pdr_ns_supg_get_velocity_pins_count(&pdv_ns_supg_problem.bc));
  fprintf(Interactive_output, "\tNumber of BCs:\t\t\t\t%d\n\n", 
	  pdr_ns_supg_get_bc_assign_count(&pdv_ns_supg_problem.bc));

  fprintf(Interactive_output, "\nMaterials configuration file:\t\t\t%s\n",
	  pdv_ns_supg_problem.ctrl.material_filename);
  fprintf(Interactive_output, "\tNumber of materials:\t\t\t%d", 
	  pdv_ns_supg_problem.materials.materials_num);
  fprintf(Interactive_output, "\n\tNumber of materials in the model :\t%d\n", 
	  pdv_vof_problem.ctrl.materials_used[0]);

  /****************************************/
  /* initialization of mesh data          */
  /****************************************/

  // mesh is read using ns_supg problem parameters (there is one mesh for
  // both problems)
  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
  mesh_id = utr_initialize_mesh( Interactive_output, Work_dir, 
				pdv_ns_supg_problem.ctrl.mesh_type[0], pdv_ns_supg_problem.ctrl.mesh_filename);
  pdv_ns_supg_problem.ctrl.mesh_id = mesh_id;
  pdv_heat_problem.ctrl.mesh_id = mesh_id;
  pdv_heat_dtdt_problem.ctrl.mesh_id = mesh_id;
  pdv_vof_problem.ctrl.mesh_id = mesh_id;
  pdv_mat_problem.ctrl.mesh_id = mesh_id;

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

  /************************************************/
  /* initialization of material in mesh structure */
  /************************************************/

  pdr_initialize_mesh_mat(pdv_mat_problem.ctrl.mesh_id, pdr_material_init);

  /****************************************/
  /* initialization of approximation field data */
  /****************************************/
  int pdeg = 1;

  // first ns_supg field
  if (strcmp(pdv_ns_supg_problem.ctrl.field_filename,"z")==0){
    fprintf(Interactive_output, 
	    "\nInitializing ns_supg field with 0\n"); 
    // 's' - for standard continuous basis functions
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
  // 'i' - for initializing using function pdr_ns_supg_initial_condition
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
  // 'i' - for initializing using function pdr_ns_supg_initial_condition
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
  // 'i' - for initializing using function pdr_heat_initial_condition
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
      // 'i' - for initializing using function pdr_heat_initial_condition
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
  fprintf(Interactive_output, 
	    "\nInitializing heat_dtdt field with 0\n"); 
  pdv_heat_dtdt_problem.ctrl.field_id = utr_initialize_field(
                            Interactive_output, 's', 'z', 
			    pdv_heat_dtdt_problem.ctrl.mesh_id, 
			    pdv_heat_dtdt_problem.ctrl.nreq, 
			    pdv_heat_dtdt_problem.ctrl.nr_sol, pdeg,NULL,NULL);

//  fprintf(Interactive_output, "\n\tAS: num_dof = % d", apr_get_ent_nrdofs(pdv_heat_dtdt_problem.ctrl.field_id,APC_VERTEX,1));
//  fprintf(Interactive_output, "\n\tAS: nreq = % d", apr_get_nreq(pdv_heat_dtdt_problem.ctrl.field_id));
//  fprintf(Interactive_output, "\n\tAS: numshap = % d", apr_get_ent_numshap(pdv_heat_dtdt_problem.ctrl.field_id, APC_VERTEX, 1));
//  fprintf(Interactive_output, "\n\tAS: pdeg = % d", apr_get_ent_pdeg(pdv_heat_dtdt_problem.ctrl.field_id, APC_VERTEX, 1));
/*  fprintf(Interactive_output, "\n\tAS: field_id = %d, mesh_id = %d, nreq = %d, nr_sol = %d, pdeg = %d\n",
	  pdv_heat_dtdt_problem.ctrl.field_id,
	  pdv_heat_dtdt_problem.ctrl.mesh_id, 
	  pdv_heat_dtdt_problem.ctrl.nreq, 
	  pdv_heat_dtdt_problem.ctrl.nr_sol, pdeg); */

  apr_check_field(pdv_heat_dtdt_problem.ctrl.field_id);

  // fourth vof field
  if (strcmp(pdv_vof_problem.ctrl.field_filename,"z")==0){
    fprintf(Interactive_output, 
	    "\nInitializing vof field with 0\n"); 
    // 's' - for standard continuous basis functions
    // 'z' - for zeroing field values
    pdv_vof_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		               pdv_vof_problem.ctrl.mesh_id, 
			       pdv_vof_problem.ctrl.nreq, 
			       pdv_vof_problem.ctrl.nr_sol, pdeg,NULL,NULL);
  }
  else if (strcmp(pdv_vof_problem.ctrl.field_filename,"i")==0) {
    fprintf(Interactive_output, 
	    "\nInitializing vof field with initial_condition function\n"); 
  // 's' - for standard continuous basis functions
  // 'i' - for initializing using function pdr_vof_initial_condition
    pdv_vof_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'i', 
		               pdv_vof_problem.ctrl.mesh_id, 
			       pdv_vof_problem.ctrl.nreq, 
			       pdv_vof_problem.ctrl.nr_sol, pdeg, NULL, 
			       pdr_vof_initial_condition);
  }
  else{
    sprintf(arg, "%s/%s", Work_dir, pdv_vof_problem.ctrl.field_filename);
    testforfile = fopen(arg, "r");
    if (testforfile != NULL) {
      fclose(testforfile);
      fprintf(Interactive_output, "\nInput field file %s.", 
	      pdv_vof_problem.ctrl.field_filename);
      // 's' - for standard continuous basis functions
      // 'i' - for initializing using function pdr_vof_initial_condition
      pdv_vof_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'r',
                               pdv_vof_problem.ctrl.mesh_id, 
                               pdv_vof_problem.ctrl.nreq, 
                               pdv_vof_problem.ctrl.nr_sol, pdeg, arg,NULL);
    } else {
      fprintf(Interactive_output, 
	      "\nInput field file %s not found - setting field to 0\n", 
	      pdv_vof_problem.ctrl.field_filename);
      // 's' - for standard continuous basis functions
      // 'z' - for zeroing field values
      pdv_vof_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		               pdv_vof_problem.ctrl.mesh_id, 
			       pdv_vof_problem.ctrl.nreq, 
			       pdv_vof_problem.ctrl.nr_sol, pdeg,NULL,NULL);
    }
  }
  apr_check_field(pdv_vof_problem.ctrl.field_id);

  // fifth mat field
  if (strcmp(pdv_mat_problem.ctrl.field_filename,"z")==0){
    fprintf(Interactive_output, 
	    "\nInitializing mat field with 0\n"); 
    // 's' - for standard continuous basis functions
    // 'z' - for zeroing field values
    pdv_mat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		          pdv_mat_problem.ctrl.mesh_id, 
			       pdv_mat_problem.ctrl.nreq, 
			       pdv_mat_problem.ctrl.nr_sol, pdeg,NULL,NULL);
  }
  else if (strcmp(pdv_mat_problem.ctrl.field_filename,"i")==0) {
    fprintf(Interactive_output, 
	    "\nInitializing mat field with initial_condition function\n"); 
  // 's' - for standard continuous basis functions
  // 'i' - for initializing using function pdr_vof_initial_condition
    pdv_mat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'i', 
		          pdv_mat_problem.ctrl.mesh_id, 
			       pdv_mat_problem.ctrl.nreq, 
			       pdv_mat_problem.ctrl.nr_sol, pdeg, NULL, 
			       pdr_mat_initial_condition);
  }
  else{
    sprintf(arg, "%s/%s", Work_dir, pdv_mat_problem.ctrl.field_filename);
    testforfile = fopen(arg, "r");
    if (testforfile != NULL) {
      fclose(testforfile);
      fprintf(Interactive_output, "\nInput field file %s.", 
	      pdv_mat_problem.ctrl.field_filename);
      // 's' - for standard continuous basis functions
      // 'i' - for initializing using function pdr_vof_initial_condition
      pdv_mat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'r',
                               pdv_mat_problem.ctrl.mesh_id, 
                               pdv_mat_problem.ctrl.nreq, 
                               pdv_mat_problem.ctrl.nr_sol, pdeg, arg,NULL);
    } else {
      fprintf(Interactive_output, 
	      "\nInput field file %s not found - setting field to 0\n", 
	      pdv_mat_problem.ctrl.field_filename);
      // 's' - for standard continuous basis functions
      // 'z' - for zeroing field values
      pdv_mat_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		          pdv_mat_problem.ctrl.mesh_id, 
			       pdv_mat_problem.ctrl.nreq, 
			       pdv_mat_problem.ctrl.nr_sol, pdeg,NULL,NULL);
    }
  }
  apr_check_field(pdv_mat_problem.ctrl.field_id);

  /* fprintf(Interactive_output, "\n\tAS: num_dof = % d", apr_get_ent_nrdofs(pdv_mat_problem.ctrl.field_id,APC_VERTEX,1)); */
  /* fprintf(Interactive_output, "\n\tAS: nreq = % d", apr_get_nreq(pdv_mat_problem.ctrl.field_id)); */
  /* fprintf(Interactive_output, "\n\tAS: numshap = % d", apr_get_ent_numshap(pdv_mat_problem.ctrl.field_id, APC_VERTEX, 1)); */
  /* fprintf(Interactive_output, "\n\tAS: pdeg = % d", apr_get_ent_pdeg(pdv_mat_problem.ctrl.field_id, APC_VERTEX, 1)); */
  /* fprintf(Interactive_output, "\n\tAS: field_id = %d, mesh_id = %d, nreq = %d, nr_sol = %d, pdeg = %d\n", */
  /* 	  pdv_mat_problem.ctrl.field_id, */
  /* 	  pdv_mat_problem.ctrl.mesh_id,  */
  /* 	  pdv_mat_problem.ctrl.nreq,  */
  /* 	  pdv_mat_problem.ctrl.nr_sol, pdeg); */

return(0);
}


int  pdr_ns_supg_heat_vof_bc_free()
{

  return(0);
}

int  pdr_ns_supg_heat_vof_material_free()
{

  return(0);
}
