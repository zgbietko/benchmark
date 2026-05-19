/************************************************************************
File pds_ns_supg_main.c - driver for ns_supg module - approximation
      of Navier_Stokes equations

Contains definition of global variables - among them:
  pdv_ns_supg_problem - problem structure for ns_supg problem 
					      
Contains definition of routines:
  main
  pdr_ns_supg_init - initialize problem data
 

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

*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include<string.h>

/* USED DATA STRUCTURES AND INTERFACES FROM OTHER MODULES */

/* utilities - including simple time measurement library */
#include "uth_intf.h"

/* interface for all mesh manipulation modules */
#include "mmh_intf.h"	

/* interface for all approximation modules */
#include "aph_intf.h"	

#ifdef PARALLEL
/* interface of parallel mesh manipulation modules */
#include "mmph_intf.h"
/* interface for all parallel approximation modules */
#include "apph_intf.h"
/* interface for parallel communication modules */
#include "pch_intf.h"
#endif

/* interface for thread management modules */
#include "tmh_intf.h"

/* interface for all solver modules */
#include "sih_intf.h"	

/* visualization module */
#include "mod_fem_viewer.h"

#include "uth_log.h"

/* USED AND IMPLEMENTED PROBLEM DEPENDENT DATA STRUCTURES AND INTERFACES */

/* problem dependent module interface */
#include "pdh_intf.h"	

/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"

/* problem module's types and functions */
#include "../include/pdh_ns_supg.h"	
/* types and functions related to problem structures */
#include "../include/pdh_ns_supg_problem.h" 
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


// ID of the current problem
// on purpose initialized to 0 which is wrong value !
// later should be replaced by one of the two proper values:
// ns_supg -> problem_id = PDC_NS_SUPG_ID = 1
int pdv_ns_supg_current_problem_id=PDC_NS_SUPG_ID;/* ID of the current problem */
// problem structure for ns_supg module
pdt_ns_supg_problem pdv_ns_supg_problem;

  /* time measurements */
  double timer_all = 0.0;
  double pdv_ns_supg_timer_pdr_comp_el_stiff_mat = 0.0;
  double pdv_ns_supg_timer_pdr_comp_fa_stiff_mat = 0.0;

/***************************************/
/* DECLARATIONS OF INTERNAL PROCEDURES */
/***************************************/
/* Rules:
/* - name always begins with pdr_ns_supg */
/* - argument names start uppercase */
/*---------------------------------------------------------
  pdr_ns_supg_init - to initialize problem data 
                          (including mesh and two fields)
----------------------------------------------------------*/
extern int pdr_ns_supg_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
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
  /* initialization of problem data (including mesh and field) */
  /******************************************************************/

  // initialize ns_supg  structure
  pdr_ns_supg_init(work_dir, interactive_input, interactive_output);


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
	  printf("\tm - perform manual mesh adaptation \n");
	  printf("\ta - perform automatic mesh adaptation \n");
	  printf("\tr - print profile\n");
	  printf("\td - dump out data \n");
	  printf("\tv - write ParaView graphics data \n");
	  printf("\tc - change control data \n");
	  printf("\th - perform random test of h-adaptivity\n");
	  printf("\tq - exit the program\n");
	  scanf(" %c", &c);
	}
	while (c != 't' && c != 's' && c != 'e' && c != 'p' && c != 'g' 
	        && c != 'm' && c != 'a' && c != 'r' && c != 'd' 
	        && c != 'v' && c != 'c' && c != 'h' && c != 'q');
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
      timer_all = 0.0;


      utr_io_result_set_filename(work_dir,"ns_supg.csv");
      utr_io_result_clear_columns();
      utr_io_result_add_column(RESULT_STEP);
      utr_io_result_add_column(RESULT_CUR_TIME);
      utr_io_result_add_column(RESULT_OUT_FILENAME);
      utr_io_result_write_columns();

      mf_log_info("Beginning solution of ns_supg problem");

      timer_all = time_clock();

      pdr_ns_supg_time_integration(work_dir, 
			   interactive_input, interactive_output);

      timer_all = time_clock() - timer_all;

      fprintf(interactive_output,"\nTime total: %lf\n", 
	      timer_all);

    }
    /*------------------------------------------------------------*/
    else if (c == 'e') {

      pdr_ns_supg_ZZ_error(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'p') {

      pdr_ns_supg_post_process(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if(c=='k'){
        c='\0';
        init_mod_fem_viewer(argc,argv,interactive_output);
    }
    /*------------------------------------------------------------*/
    else if (c == 'm') {

      // perform manual mesh refinement using ns_supg problem structure
      pdv_ns_supg_current_problem_id = PDC_NS_SUPG_ID;
      utr_manual_refinement(pdv_ns_supg_current_problem_id, 
			    work_dir, interactive_input, interactive_output);
    }
    /*------------------------------------------------------------*/
    else if (c == 'a') {

      pdr_ns_supg_adapt(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'r') {

      pdr_ns_supg_write_profile(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'd') {

      pdr_ns_supg_dump_data(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'v') {

      pdr_ns_supg_write_paraview(work_dir, 
			   interactive_input, interactive_output);

    }
    /*------------------------------------------------------------*/
    else if (c == 'c') {

      fprintf(interactive_output, "\nChanging of control data not yet implemented.\n");


    }
    /*------------------------------------------------------------*/
    else if (c == 'h') {

      // perform test for mesh using ns_supg problem structure
      pdv_ns_supg_current_problem_id = PDC_NS_SUPG_ID;
      utr_test_refinements(pdv_ns_supg_current_problem_id, work_dir, 
			   interactive_input, interactive_output );


    }
  }
  while (c != 'q');

  /* free allocated space */
  pdv_ns_supg_current_problem_id = PDC_NS_SUPG_ID;
  iaux=3; field_id =  pdr_ctrl_i_params(pdv_ns_supg_current_problem_id,iaux);
  apr_free_field(field_id);

  mesh_id = apr_get_mesh_id(field_id);
  mmr_free_mesh(mesh_id);

  fclose(interactive_input);
  fclose(interactive_output);

  //pdr_ns_supg_bc_free();
  //pdr_ns_supg_material_free();

#ifdef PARALLEL
  pcr_exit_parallel();
#endif

  return (0);
}



/*------------------------------------------------------------
pdr_ns_supg_init - read problem data
------------------------------------------------------------*/
int pdr_ns_supg_init(
  char *Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
)
{

  FILE *testforfile;
  char filename[300], arg[300];
  int nr_sol; // number of solution vectors - determined by time integration
  int mesh_id, field_id, iaux;

  pdv_ns_supg_current_problem_id = PDC_NS_SUPG_ID;
  pdr_ns_supg_problem_clear(&pdv_ns_supg_problem);

  strcpy(pdv_ns_supg_problem.ctrl.work_dir, Work_dir);
  pdv_ns_supg_problem.ctrl.interactive_input = Interactive_input;
  pdv_ns_supg_problem.ctrl.interactive_output = Interactive_output;

  nr_sol = 3;
  sprintf(filename, "%s", "problem_ns_supg.dat");


  pdr_ns_supg_problem_read(Work_dir, filename, Interactive_output,
			   &pdv_ns_supg_problem, nr_sol);


    // check data 

  fprintf(Interactive_output, "\nNS_SUPG problem %d settings :\n",
	  pdv_ns_supg_problem.ctrl.name);

  fprintf(Interactive_output, "\nCONTROL PARAMETERS:\n");

  fprintf(Interactive_output, "\n\tmesh_type:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_type);
  fprintf(Interactive_output, "\tmesh_file_in:\t\t\t%s\n\n", 
	  pdv_ns_supg_problem.ctrl.mesh_filename);

  fprintf(Interactive_output, "\tfield_file_in:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.field_filename);


  fprintf(Interactive_output, "\tmesh_file_out:\t\t\t%s\n", 
	  pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern);
  fprintf(Interactive_output, "\tfield_file_out:\t\t\t%s\n\n", 
	  pdv_ns_supg_problem.ctrl.field_dmp_filepattern);

  fprintf(Interactive_output,"\n\tpenalty for Dirichlet BCs:\t%lf\n\n",
	  pdv_ns_supg_problem.ctrl.penalty);

  fprintf(Interactive_output, "\tgravity:\t\t\t%lf %lf %lf\n\n", 
	  pdv_ns_supg_problem.ctrl.gravity_field[0], 
	  pdv_ns_supg_problem.ctrl.gravity_field[1], 
	  pdv_ns_supg_problem.ctrl.gravity_field[2]);

  /****************************************/
  /* initialization of material data      */
  /****************************************/
  if(strlen(pdv_ns_supg_problem.ctrl.material_filename)==0){

    // e.g. for non-dimensional form of NS equations material files are not used
    pdv_ns_supg_problem.ctrl.ref_temperature = -1.0;

  }
  else{

    iaux =  pdr_ns_supg_material_read(pdv_ns_supg_problem.ctrl.work_dir, 
				      pdv_ns_supg_problem.ctrl.material_filename, 
                      pdv_ns_supg_problem.ctrl.interactive_output);
    if (iaux == EXIT_FAILURE) exit(-1);

//    fprintf(Interactive_output, "\nmaterials configuration file:\t\t\t%s\n",
//	    pdv_ns_supg_problem.ctrl.material_filename);
//    fprintf(Interactive_output, "\tnumber of materials:\t\t\t%d\n\n",
//	    pdv_ns_supg_problem.materials.materials_num);
  
//    fprintf(Interactive_output, "NS_SUPG material database read\n");

  }

  // reference temperature used to indicate whether material database is used
  if(pdv_ns_supg_problem.ctrl.ref_temperature <= 0.0){

    fprintf(Interactive_output, "\tdensity and dynamic viscosity not temperature dependent\n");
    fprintf(Interactive_output, "\n\tdensity:\t\t\t%lf\n",
	    pdv_ns_supg_problem.ctrl.density);
    fprintf(Interactive_output, "\tdynamic viscosity:\t\t%lf\n", 
	    pdv_ns_supg_problem.ctrl.dynamic_viscosity);
    
  }
  else{

    fprintf(Interactive_output, "\tdensity and dynamic viscosity from material database\n");
    pdv_ns_supg_problem.ctrl.density = -1.0;
    pdv_ns_supg_problem.ctrl.dynamic_viscosity = -1.0;
    
    fprintf(Interactive_output, "\treference_temperature:\t\t%lf\n", 
	    pdv_ns_supg_problem.ctrl.ref_temperature);

  }

  fprintf(Interactive_output, "\treference_length:\t\t%lf\n", 
	  pdv_ns_supg_problem.ctrl.ref_length);
  fprintf(Interactive_output, "\n\treference_velocity:\t\t%lf\n\n", 
	  pdv_ns_supg_problem.ctrl.ref_velocity);

  if(pdv_ns_supg_problem.ctrl.ref_temperature <= 0.0){
    fprintf(Interactive_output, "\tReynolds number:\t\t%lf\n\n", 
	  pdv_ns_supg_problem.ctrl.ref_length*pdv_ns_supg_problem.ctrl.ref_velocity*pdv_ns_supg_problem.ctrl.density/pdv_ns_supg_problem.ctrl.dynamic_viscosity);
  }


  fprintf(Interactive_output, "\nTIME INTEGRATION PARAMETERS:\n");

  fprintf(Interactive_output, "\ttype:\t\t\t\t%d\n", 
	  pdv_ns_supg_problem.time.type);
  fprintf(Interactive_output, "\timplicitness parameter:\t\t%lf\n\n", 
	  pdv_ns_supg_problem.time.alpha);

  fprintf(Interactive_output, "\tcurrent timestep:\t\t%d\n", 
	  pdv_ns_supg_problem.time.cur_step);
  fprintf(Interactive_output, "\tcurrent time:\t\t\t%lf\n", 
	  pdv_ns_supg_problem.time.cur_time);
  fprintf(Interactive_output, "\tcurrent timestep_length:\t%lf\n", 
	  pdv_ns_supg_problem.time.cur_dtime);
  fprintf(Interactive_output, "\tprevious timestep_length:\t%lf\n\n", 
	  pdv_ns_supg_problem.time.prev_dtime);

  fprintf(Interactive_output, "\tfinal time:\t\t\t%lf\n", 
	  pdv_ns_supg_problem.time.final_time);
  fprintf(Interactive_output, "\tfinal timestep:\t\t\t%d\n", 
	  pdv_ns_supg_problem.time.final_step);
  fprintf(Interactive_output, "\tconvergence criterion type:\t%d\n", 
	  pdv_ns_supg_problem.time.conv_type);
  fprintf(Interactive_output, "\terror tolerance (n-epsilon):\t%lf\n\n", 
	  pdv_ns_supg_problem.time.conv_meas);

  fprintf(Interactive_output, "\tmonitoring level:\t\t%d\n\n", 
	  pdv_ns_supg_problem.time.monitor);

  fprintf(Interactive_output, "\tgraph_dump_intv:\t\t%d\n", 
	  pdv_ns_supg_problem.time.intv_graph);
  fprintf(Interactive_output, "\tfull_dump_intv:\t\t\t%d\n\n", 
	  pdv_ns_supg_problem.time.intv_dumpout);

  fprintf(Interactive_output, "\nNONLINEAR SOLVER PARAMETERS:\n");

  fprintf(Interactive_output, "\ttype:\t\t\t\t%d\n", 
	  pdv_ns_supg_problem.nonl.type);
  fprintf(Interactive_output, "\tmax_nonl_iter:\t\t\t%d\n", 
	  pdv_ns_supg_problem.nonl.max_iter);
  fprintf(Interactive_output, "\tconvergence criterion type:\t%d\n", 
	  pdv_ns_supg_problem.nonl.conv_type);
  fprintf(Interactive_output, "\terror tolerance (k-epsilon):\t%lf\n\n", 
	  pdv_ns_supg_problem.nonl.conv_meas);

  fprintf(Interactive_output, "\tmonitoring level:\t\t%d\n\n", 
	  pdv_ns_supg_problem.nonl.monitor);


  fprintf(Interactive_output, "\nLINEAR SOLVER PARAMETERS:\n");

  fprintf(Interactive_output, "\ttype:\t\t\t\t%d\n", 
	  pdv_ns_supg_problem.lins.type);

  fprintf(Interactive_output, "\tsolver_file:\t\t\t%s\n\n", 
	  pdv_ns_supg_problem.ctrl.solver_filename);

  if(pdv_ns_supg_problem.lins.type!=0){
    fprintf(Interactive_output, "\tmax_lins_iter:\t\t\t%d\n", 
	    pdv_ns_supg_problem.lins.max_iter);
    fprintf(Interactive_output, "\tconvergence criterion type:\t%d\n", 
	    pdv_ns_supg_problem.lins.conv_type);
    fprintf(Interactive_output, "\terror tolerance:\t\t%lf\n\n", 
	    pdv_ns_supg_problem.lins.conv_meas);
  }

  fprintf(Interactive_output, "\tmonitoring level:\t\t%d\n\n", 
	  pdv_ns_supg_problem.lins.monitor);


  fprintf(Interactive_output, "\nADAPTATION PARAMETERS:\n");

  fprintf(Interactive_output, "\tadapt_type:\t\t\t%d\n", 
	  pdv_ns_supg_problem.adpt.type);
  fprintf(Interactive_output, "\tadapt_interval:\t\t\t%d\n", 
	  pdv_ns_supg_problem.adpt.interval);
  fprintf(Interactive_output, "\tadapt_eps:\t\t\t%lf\n", 
	  pdv_ns_supg_problem.adpt.eps);
  fprintf(Interactive_output, "\tadapt_ratio:\t\t\t%lf\n\n", 
	  pdv_ns_supg_problem.adpt.ratio);

  fprintf(Interactive_output, "\tmonitoring level:\t\t%d\n\n", 
	  pdv_ns_supg_problem.adpt.monitor);
  
  
  /****************************************/
  /* initialization of bc data            */
  /****************************************/
  
  iaux = pdr_ns_supg_bc_read(pdv_ns_supg_problem.ctrl.work_dir, 
			     pdv_ns_supg_problem.ctrl.bc_filename, 
			     pdv_ns_supg_problem.ctrl.interactive_output, 
			     &pdv_ns_supg_problem.bc);
  if (iaux == EXIT_FAILURE) exit(-1);
  
  fprintf(Interactive_output, "\nboundary conditions configuration file:\t%s\n", 
	  pdv_ns_supg_problem.ctrl.bc_filename);
  fprintf(Interactive_output, "\tnumber of pressure pins:\t%d\n", 
	  pdr_ns_supg_get_pressure_pins_count(&pdv_ns_supg_problem.bc));
  fprintf(Interactive_output, "\tnumber of velocity pins:\t%d\n", 
	  pdr_ns_supg_get_velocity_pins_count(&pdv_ns_supg_problem.bc));
  fprintf(Interactive_output, "\tnumber of BCs:\t\t\t%d\n\n", 
	  pdr_ns_supg_get_bc_assign_count(&pdv_ns_supg_problem.bc));

  fprintf(Interactive_output, "NS_SUPG BC OK\n");
  

  /****************************************/
  /* initialization of mesh data          */
  /****************************************/

  pdv_ns_supg_current_problem_id = PDC_NS_SUPG_ID;
  mesh_id = utr_initialize_mesh(Interactive_output, Work_dir, 
				pdv_ns_supg_problem.ctrl.mesh_type[0], 
				pdv_ns_supg_problem.ctrl.mesh_filename);
  pdv_ns_supg_problem.ctrl.mesh_id = mesh_id;

#ifdef DEBUG
  {
  int currfa = 0;
  int fa_bnum;
  /* check if every boundary has been assigned boundary condtion */
  while (currfa = mmr_get_next_face_all(pdv_ns_supg_problem.ctrl.mesh_id, 
					currfa)) {
    fa_bnum = mmr_fa_bc(pdv_ns_supg_problem.ctrl.mesh_id, currfa);
    if (fa_bnum > 0) {
      if ((pdr_ns_supg_get_bc_type(&pdv_ns_supg_problem.bc, fa_bnum) == BC_NS_SUPG_NONE)) {
	fprintf(Interactive_output, "BC not set for boundary:\t%d\n", fa_bnum);
	fprintf(Interactive_output, "Check bc config file - Exiting.\n");
	exit(-1);
      }
    }
  }
  }
#endif

  mmr_set_max_gen(mesh_id, pdv_ns_supg_problem.adpt.maxgen);
  mmr_set_max_gen_diff(mesh_id, 1); // one irregularity of meshes enforced

  fprintf(Interactive_output, "\nAfter reading initial data.\n");
  fprintf(Interactive_output, "Mesh (number of active, maximal index):\n");
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
    // 's' - for standard continuous basis functions
    // 'z' - for zeroing field values
    pdv_ns_supg_problem.ctrl.field_id = utr_initialize_field(
                               Interactive_output, 's', 'z', 
		               pdv_ns_supg_problem.ctrl.mesh_id, 
			       pdv_ns_supg_problem.ctrl.nreq, 
			       pdv_ns_supg_problem.ctrl.nr_sol, pdeg,NULL,NULL);
  }
  else if (strcmp(pdv_ns_supg_problem.ctrl.field_filename,"i")==0) {
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

  return(0);
}

#ifdef TURBULENTFLOW
void GetLocalViscosity(int elementId,int GaussPointId, double* viscLocal,int problem_id)
{
	pdt_ns_supg_problem *problem = 
		(pdt_ns_supg_problem *)pdr_get_problem_structure(problem_id);
	*viscLocal = problem->ctrl.dynamic_viscosity;
}
#endif //TURBULENTFLOW
