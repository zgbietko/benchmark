/************************************************************************
File pds_ns_supg_ale_time_integration.c - time stepping procedures

Contains definition of routines:
  pdr_ns_supg_ale_comp_sol_diff_norm - returns max difference between current and old
			  solutions vectors
  pdr_ns_supg_ale_time_integration - time integration driver

------------------------------
History:
	2002    - Krzysztof Banas (pobanas@cyf-kr.edu.pl) for conv-diff
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Pawe³ Cybu³ka (cybulka@agh.edu.pl, ticoo@wp.pl)
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include<string.h>

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
#include "mmph_intf.h"		/* USES */
/* interface for all parallel approximation modules */
#include "apph_intf.h"		/* USES */
/* interface for parallel communication modules */
#include "pch_intf.h"		/* USES */
#endif

/* problem module's types and functions */
#include "../include/pdh_ns_supg_ale.h"	/* USES & IMPLEMENTS */
/* types and functions related to ns_supg problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
// bc and material header files are included in problem header files
/* functions related to weak formulation (here for computing CFL number) */
#include "../../pdd_ns_supg/include/pdh_ns_supg_weakform.h" 

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */



/*------------------------------------------------------------
pdr_ns_supg_sol_diff_norm - returns max difference in current and old
			  solutions vectors
------------------------------------------------------------*/
void pdr_ns_supg_ale_sol_diff_norm(
  int Current, // in: current vector id 
  int Old,     // in: old vector id
  double* sol_diff_norm_ns_supg_p // norm of difference current-old for ns_supg
  )
{

  double sol_dofs_current[APC_MAXELSD];	/* solution dofs */
  double sol_dofs_old[APC_MAXELSD];	/* solution dofs */
  int field_id, mesh_id;
  int node_id = 0;
  double temp, norm_ns_supg = 0.0;
  int i;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;  // ns_supg problem
  i=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_ale_current_problem_id,i);
  mesh_id = apr_get_mesh_id(field_id);
  
  while ((node_id = mmr_get_next_node_all(mesh_id, node_id)) != 0) {
    if (apr_get_ent_pdeg(field_id, APC_VERTEX, node_id) > 0) {
      int numdofs = apr_get_ent_nrdofs(field_id, APC_VERTEX, node_id);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Current, sol_dofs_current);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Old, sol_dofs_old);
      i = 0; // for ns_supg we take into account velocities ONLY!!!
      for (i = 0; i < numdofs-1; ++i) {

/*kbw
  double norm=0.0;
  printf("node_id %d\ti %d\tcurrent %lf\told %lf\tnorm %lf\n",
	 node_id,i,sol_dofs_current[i],sol_dofs_old[i],norm);
  norm += (sol_dofs_current[i] - sol_dofs_old[i]) * (sol_dofs_current[i] - sol_dofs_old[i]);
/*kew*/

	temp = fabs(sol_dofs_current[i] - sol_dofs_old[i]);
	if (norm_ns_supg < temp)norm_ns_supg = temp;
      }
    }
  }
  
  
  
#ifdef PARALLEL
  pcr_allreduce_max_double(1, &norm_ns_supg, &temp);
  norm_ns_supg = temp;
#endif

  *sol_diff_norm_ns_supg_p = norm_ns_supg;
  
  return;
}

/*------------------------------------------------------------
pdr_ns_supg_ale_time_integration - time integration driver
------------------------------------------------------------*/
void pdr_ns_supg_ale_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
)
{

  int mesh_id;
  int field_ns_supg_id;
  int  solver_ns_supg_id;
  double sol_norm_uk_ns_supg, sol_norm_un_ns_supg;
  char autodump_filename[300];
  int iadapt = 0;
  char solver_ns_supg_filename[300];
  int nr_iter, nonl_iter;
  double conv_meas, conv_rate;
  int i, iaux;
  double cfl_min=1000000, cfl_max=0, cfl_ave=0.0, daux;

  /*  Navier_Stokes problem parameters */
  pdt_ns_supg_problem *problem_ns_supg = &pdv_ns_supg_problem;
  pdt_ns_supg_ctrls *ctrl_ns_supg = &pdv_ns_supg_problem.ctrl;
  pdt_ns_supg_times *time_ns_supg = &pdv_ns_supg_problem.time;
  pdt_ns_supg_nonls *nonl_ns_supg = &pdv_ns_supg_problem.nonl;
  pdt_ns_supg_linss *lins_ns_supg = &pdv_ns_supg_problem.lins;
  pdt_ns_supg_adpts *adpt_ns_supg = &pdv_ns_supg_problem.adpt;

  /* time step length adaptation based on specified CFL number */
  int cfl_control = time_ns_supg->CFL_control;
  double cfl_limit = time_ns_supg->CFL_limit;
  //double reference_time_step_length = time_ns_supg->reference_time_step_length;
  double time_step_length_mult = time_ns_supg->time_step_length_mult;


/*++++++++++++++++ executable statements ++++++++++++++++*/

  pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;

  // get mesh and field parameters
  i = 2; mesh_id = pdr_ctrl_i_params(pdv_ns_supg_ale_current_problem_id,i);
  pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;  // ns_supg problem
  field_ns_supg_id = pdr_ctrl_i_params(pdv_ns_supg_ale_current_problem_id,3);
/*
#ifdef TURBULENTFLOW
    int intStep=0;
  SetMesh(mesh_id);

  SetField(field_ns_supg_id);
  //SetTurbModel(pdv_settings->chlength);
  SetTurbModel(pdv_ns_supg_current_problem_id);

#endif //TURBULENTFLOW	
*/
  // when finished time loop reset interactively time integration parameters
  if (time_ns_supg->cur_time >= time_ns_supg->final_time) {
    
    fprintf(Interactive_output, 
	    "\nCurrent time: %lf is bigger than final time: %lf.\n\n", 
	    time_ns_supg->cur_time, time_ns_supg->final_time);
    
    fprintf(Interactive_output, 
	    "\nCurrent time-step: %d, current time step length: %lf.\n\n", 
	    time_ns_supg->cur_step, time_ns_supg->cur_dtime);
    
    if(Interactive_input == stdin){
      
#ifdef PARALLEL
      if (pcr_my_proc_id() == pcr_print_master()) {
#endif
	printf("How many steps to perform?\n");
	scanf("%d",&iaux);
#ifdef PARALLEL
      }
      pcr_bcast_int(pcr_print_master(), 1, &iaux);
#endif
      
#ifdef PARALLEL
      if (pcr_my_proc_id() == pcr_print_master()) {
#endif
	printf("Set new time step length (or CFL limit if < 0):\n");
	scanf("%lf",&daux);
#ifdef PARALLEL
      }
      pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
      if(daux>0){
	time_ns_supg->CFL_control = 0;
	time_ns_supg->CFL_limit = 1.e10;
	time_ns_supg->time_step_length_mult = 1.0;
	//time_ns_supg->reference_time_step_length = daux;
	//cfl_control = 0;
	//cfl_limit = 1.e10;
	//time_step_length_mult = 1.0;
	time_ns_supg->cur_dtime = daux;
      }
      else{
	time_ns_supg->CFL_control = 1;
	time_ns_supg->CFL_limit = -daux;
	//time_ns_supg->reference_time_step_length = time_ns_supg->cur_dtime;
	//cfl_control = 1;
	//cfl_limit = -daux;
#ifdef PARALLEL
	if (pcr_my_proc_id() == pcr_print_master()) {
#endif
	  printf("Set new time step length multiplier:\n");
	  scanf("%lf",&daux);
#ifdef PARALLEL
	}
	pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
	time_ns_supg->time_step_length_mult = daux;
	//time_step_length_mult = daux;
      }
      
    } else{
      
      fprintf(Interactive_output, "\nExiting!\n\n");
      exit(0);;
      
    }
    
    time_ns_supg->final_step = time_ns_supg->cur_step + iaux;
    
    time_ns_supg->final_time = time_ns_supg->cur_time + 
      iaux*time_ns_supg->cur_dtime;
  }
  
  if (time_ns_supg->cur_step >= time_ns_supg->final_step) {
    
    fprintf(Interactive_output, 
	    "\nCurrent time-step: %d is bigger than final step: %d\n\n", 
	    time_ns_supg->cur_step, time_ns_supg->final_step);
    
    fprintf(Interactive_output, 
	    "\nCurrent time: %lf, current time step length: %lf.\n\n", 
	    time_ns_supg->cur_time, time_ns_supg->cur_dtime);
    
    if(Interactive_input == stdin){
      
#ifdef PARALLEL
      if (pcr_my_proc_id() == pcr_print_master()) {
#endif
	printf("How many steps to perform?\n");
	scanf("%d",&iaux);
#ifdef PARALLEL
      }
      pcr_bcast_int(pcr_print_master(), 1, &iaux);
#endif
      
#ifdef PARALLEL
      if (pcr_my_proc_id() == pcr_print_master()) {
#endif
	printf("Set new time step length (or CFL limit if < 0):\n");
	scanf("%lf",&daux);
#ifdef PARALLEL
      }
      pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
      if(daux>0){
	time_ns_supg->CFL_control = 0;
	time_ns_supg->CFL_limit = 1.e10;
	time_ns_supg->time_step_length_mult = 1.0;
	//time_ns_supg->reference_time_step_length = daux;
	//cfl_control = 0;
	//cfl_limit = 1.e10;
	//time_step_length_mult = 1.0;
	time_ns_supg->cur_dtime = daux;
      }
      else{
	time_ns_supg->CFL_control = 1;
	time_ns_supg->CFL_limit = -daux;
	//time_ns_supg->reference_time_step_length = time_ns_supg->cur_dtime;
	//cfl_control = 1;
	//cfl_limit = -daux;
#ifdef PARALLEL
	if (pcr_my_proc_id() == pcr_print_master()) {
#endif
	  printf("Set new time step length multiplier:\n");
	  scanf("%lf",&daux);
#ifdef PARALLEL
	}
	pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
	time_ns_supg->time_step_length_mult = daux;
	//time_step_length_mult = daux;
      }
      
    } else{
      
      fprintf(Interactive_output, "\nExiting!\n\n");
      exit(0);;
      
    }
    
    time_ns_supg->final_step = time_ns_supg->cur_step + iaux;
    
    time_ns_supg->final_time = time_ns_supg->cur_time + 
      iaux*time_ns_supg->cur_dtime;
    
  } 
  
  /* print some info */
  fprintf(Interactive_output, "\nTime integration will stop whichever comes first:\n");
  fprintf(Interactive_output, "final_time: %lf\n", time_ns_supg->final_time);
  fprintf(Interactive_output, "final_timestep: %d\n", time_ns_supg->final_step);
  fprintf(Interactive_output, "error less than time_integration_tolerance: %lf\n\n", time_ns_supg->conv_meas);

#ifndef PARALLEL
  if(Interactive_input == stdin) {
	printf("Type [Ctrl-C] to manually break time integration.\n");
  }
#endif
  
  sprintf(solver_ns_supg_filename, "%s/%s", 
	  ctrl_ns_supg->work_dir, ctrl_ns_supg->solver_filename); 
  
  if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
#ifndef PARALLEL
    pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;  // ns_supg problem
    int problem_id = pdv_ns_supg_ale_current_problem_id;
    solver_ns_supg_id = sir_init(SIC_SEQUENTIAL, solver_ns_supg_filename,
			pdr_lins_i_params(problem_id, 2), // max_iter
			pdr_lins_i_params(problem_id, 3), // error_type
			pdr_lins_d_params(problem_id, 4), // error_tolerance
			pdr_lins_i_params(problem_id, 5)  // monitoring level
				 );
    ctrl_ns_supg->solver_id = solver_ns_supg_id;
    fprintf(Interactive_output, "Assigned solver ID: %d for NS_SUPG\n", 
	    solver_ns_supg_id);
#endif
#ifdef PARALLEL
    pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;  // ns_supg problem
    int problem_id = pdv_ns_supg_ale_current_problem_id;
    solver_ns_supg_id = sir_init(SIC_PARALLEL, solver_ns_supg_filename,
			pdr_lins_i_params(problem_id, 2), // max_iter
			pdr_lins_i_params(problem_id, 3), // error_type
			pdr_lins_d_params(problem_id, 4), // error_tolerance
			pdr_lins_i_params(problem_id, 5)  // monitoring level
				 ); 
    ctrl_ns_supg->solver_id = solver_ns_supg_id;
    fprintf(Interactive_output, "Assigned solver ID: %d for NS_SUPG\n", 
	    solver_ns_supg_id);
#endif
  }

  /* set parameter indicating new mesh */
  iadapt = 1;

  /* start loop over time steps */
  while (utv_SIGINT_not_caught) {

    /* update time step and current time */
    ++(time_ns_supg->cur_step);


    mmr_test_mesh_motion(3,time_ns_supg->cur_step,10,50,0.005,  3,0.5,0,   3.1,0.6,0.1,   3.1,0.4,0);

    time_ns_supg->prev_dtime = time_ns_supg->cur_dtime;

    // here possible time step length adaptations !!!
    // compute maximal, minimal and average CFL number for ns_supg_problem
    if(time_ns_supg->CFL_control == 1){

      cfl_limit = time_ns_supg->CFL_limit;
      time_step_length_mult = time_ns_supg->time_step_length_mult;

      fprintf(Interactive_output,
	      "\nCFL numbers before time step length control:\n");
      pdr_ns_supg_compute_CFL(PDC_NS_SUPG_ID, &cfl_min, &cfl_max, &cfl_ave);
      //fprintf(Interactive_output,
      //	    "CFL_min = %lf, CFL_max = %lf, CFL_average = %lf\n",
      //	    cfl_min,cfl_max,cfl_ave);
#ifdef PARALLEL
      {
	double cfl_max_new = cfl_max;
	pcr_allreduce_max_double(1,&cfl_max,&cfl_max_new);
	cfl_max = cfl_max_new;
      }
#endif
      fprintf(Interactive_output,
	      "CFL_min = %lf, CFL_max = %lf, CFL_average = %lf\n",
	      cfl_min,cfl_max,cfl_ave);

      // check whether CFL is not too big
      if(cfl_max > cfl_limit){
	time_ns_supg->cur_dtime = time_ns_supg->cur_dtime/time_step_length_mult;
      }
      // check whether CFL is not too small
      if(cfl_max < cfl_limit/(time_step_length_mult) ){
	if(cfl_max > 1.e-3){ // to exclude pure diffusion problems
	  time_ns_supg->cur_dtime=time_ns_supg->cur_dtime*time_step_length_mult;
	}
      }
    }

    time_ns_supg->cur_time += time_ns_supg->cur_dtime;
    
    fprintf(Interactive_output, "\n\nSolving time step %d (cur_dtime: %lf, cur_time (t^(n+1)): %lf)\n", 
      time_ns_supg->cur_step, time_ns_supg->cur_dtime, time_ns_supg->cur_time);
    
    // compute maximal, minimal and average CFL number for ns_supg_problem
    pdr_ns_supg_compute_CFL(PDC_NS_SUPG_ID, &cfl_min, &cfl_max, &cfl_ave);
#ifdef PARALLEL
    {
        double cfl_max_new = cfl_max;
        pcr_allreduce_max_double(1,&cfl_max,&cfl_max_new);
        cfl_max = cfl_max_new;
    }
#endif
    fprintf(Interactive_output,
	    "CFL_min = %lf, CFL_max = %lf, CFL_average = %lf\n",
	    cfl_min,cfl_max,cfl_ave);
    // compute maximal, minimal and average CFL number

    // rewrite solution: 
    // current from previous time step becomes previous for current time step
    // soldofs_1 are the most recent solution dofs
    // at the beginning of time step they are rewritten to soldofs_3
    // that holds the solution from the previous time step
    apr_rewr_sol(field_ns_supg_id, 1, 3);	//rewrite current -> u_n (3)	
  
    /* update time dependent boundary conditions for NS problem*/
    pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;
    pdr_ns_supg_update_timedep_bc(&problem_ns_supg->bc, 
				     time_ns_supg->cur_time);
   
    nonl_iter = 0;
    do {
      fprintf(Interactive_output, "\nSolving for nonlin. (nonl. iter step %d) convergence (time step %d).\n", nonl_iter+1, time_ns_supg->cur_step);
      
      // when forming linear system - soldofs_1 are equal to soldofs_2
      // after solving the system soldofs_1 are different than soldofs_2
      // before starting new solution soldofs_1 are rewritten to soldofs_2
      // simply: soldofs_1 are u_k+1, soldofs_2 are u_k and soldofs_3 are u_n
      apr_rewr_sol(field_ns_supg_id, 1, 2);	//rewrite current -> u_k (2)	

      if (iadapt == 1) {
#ifdef PARALLEL
	const int ione = 1;
	/* initiate exchange tables for DOFs - for two fields, one level */
	appr_init_exchange_tables(pcr_nr_proc(), pcr_my_proc_id(), 1, &ione);
#endif
	if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
	  sir_create(solver_ns_supg_id, PDC_NS_SUPG_ID); 
	}
	iadapt = 0;
      }
      
      if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
	
	int ini_guess = 1; // get initial guess from data structure
	nr_iter = pdr_lins_i_params(PDC_NS_SUPG_ID, 2);
	conv_meas = pdr_lins_d_params(PDC_NS_SUPG_ID, 4);
	int monitor =  pdr_lins_i_params(PDC_NS_SUPG_ID, 5);
	/*------------ CALLING ITERATIVE SOLVER ------------------*/
	sir_solve(solver_ns_supg_id, SIC_SOLVE, ini_guess, monitor, 
		  &nr_iter, &conv_meas, &conv_rate);
	fprintf(Interactive_output, 
		"\nAfter %d iterations of linear solver for ns_supg problem\n", 
		nr_iter); 
	fprintf(Interactive_output, 
		"Convergence measure: %lf, convergence rate %lf\n", 
		conv_meas, conv_rate); 

	
      } else {
#ifdef PARALLEL
          if(pcv_nr_proc > 1) {
              mf_log_err("Shared memory PARDISO called for solving in distributed memory computations!");
          }
#endif
	sir_direct_solve_lin_sys(PDC_NS_SUPG_ID, SIC_SEQUENTIAL, 
				 solver_ns_supg_filename);	
      }
      
      
      /* post-process solution using slope limiter */
      //if(slope) {
      //  iaux=pdr_slope_limit(problem_id); 
      // pdr_slope_limit for DG is archived in pdd_conv_diff/approx_dg/..._util.c
      //  if(iaux<0) { printf("\nError in slope!\n");getchar();}
      //}
      
      pdr_ns_supg_ale_sol_diff_norm(1,2,&sol_norm_uk_ns_supg);
      fprintf(Interactive_output, "\nNorm for ns_supg(u_k, prev. u_k): %lf\n", 
	      sol_norm_uk_ns_supg);
      
      ++nonl_iter;
      
      if (nonl_iter >= nonl_ns_supg->max_iter) {
	fprintf(Interactive_output, 
		"\nMax nonlinear iterations reached - breaking.\n");

#ifdef TURBULENTFLOW
	fprintf(Interactive_output, "test_tstart"); 
	
	if (intStep>0)
	  {
	    //RevStep();
	  }
	intStep=intStep+1;
	int jturb;
	for (jturb=0;jturb<1;jturb++)
	  {
	    Next(time_ns_supg->cur_time);
	  }
	fprintf(Interactive_output, "test_tstop"); 
	
	
	//		ConfStep();
	
#endif //TURBULENTFLOW
	break;
      }
      
      
    } while (sol_norm_uk_ns_supg > nonl_ns_supg->conv_meas );
    
    pdr_ns_supg_ale_sol_diff_norm(1,3,&sol_norm_un_ns_supg);
    fprintf(Interactive_output, "\nNorm ns_supg (u_n, prev. u_n): %lf\n", 
	    sol_norm_un_ns_supg);
    
    /* graphics data dumps */
    if (time_ns_supg->intv_graph > 0 && 
	time_ns_supg->cur_step % time_ns_supg->intv_graph == 0) {

      fprintf(Interactive_output, "(Writing field to disk (graphics)...)\n");
      
      /* sprintf(autodump_filename, "%s/%s_g%d.dmp", ctrl_ns_supg->work_dir, ctrl_ns_supg->field_dmp_filepattern, time_ns_supg->cur_step); */
      /* if (apr_write_field(field_id, PDC_NREQ, 1, time_ns_supg->intv_graph_accu, autodump_filename) < 0) */
      /* 	fprintf(Interactive_output, "Error in writing field data!\n"); */

#ifdef TURBULENTFLOW
      DumpParaviewC(time_ns_supg->cur_step);
#endif
      pdr_ns_supg_ale_write_paraview(Work_dir, 
			   Interactive_input, Interactive_output);
    }
    
    /* full data dumps (for restarting) */
    if (time_ns_supg->intv_dumpout > 0 
	&& time_ns_supg->cur_step % time_ns_supg->intv_dumpout == 0) {
      
      fprintf(Interactive_output, "\n(Writing field to disk - not implemented)\n");
      
      /* sprintf(autodump_filename, "%s/%s_f.dmp", ctrl_ns_supg->work_dir, ctrl_ns_supg->field_dmp_filepattern); */
      /* if (apr_write_field(field_id, PDC_NREQ, 0, 0, autodump_filename) < 0) */
      /* 	fprintf(Interactive_output, "Error in writing field data!\n"); */

    }
    
    /* check for stop conditions */
    
    /* stop when final time reached */
    if (time_ns_supg->cur_time >= time_ns_supg->final_time) {
      fprintf(Interactive_output, "\nFinal time reached. Stopping.\n");
      break;
    }    
    
    /* cur_dtime could change through simulation (time step adaptation) */
    /* thus we need to check also if cur_step not bigger than final_step */
    if (time_ns_supg->cur_step >= time_ns_supg->final_step) {
      fprintf(Interactive_output, "\nFinal step reached. Stopping.\n");
      break;
    }
    
    /* stop if convergence reached (for stationary problems) */
    if (sol_norm_un_ns_supg < time_ns_supg->conv_meas) {
      fprintf(Interactive_output, 
	      "\nNorm ns_supg (u_n, prev. u_n) below n_epsilon: %lf. Stopping.\n", 
	      time_ns_supg->conv_meas);
      break;
    }
    
    // when time for adaptation
    if( adpt_ns_supg->type>0 && adpt_ns_supg->interval>0 &&
	(time_ns_supg->cur_step+1)%adpt_ns_supg->interval==0 ) {
      
#ifdef PARALLEL
      /* free exchange tables for DOFs */
      appr_free_exchange_tables(1);
#endif
      
      if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
	/* free solver data structures */
	sir_free(solver_ns_supg_id);
      }
      pdr_ns_supg_ale_adapt(Work_dir,
			     Interactive_input, Interactive_output);
      /* indicate to recreate block structure */
      iadapt=1;
    }

	//// check if user want to break time_integration
	//if(Interactive_input == stdin) {
	//  int sec=0;
	//  char c=getc(stdin);
	//  fflush(stdout);
	//  if(c == 'q') {
	//	printf("\nPress [q] again to finalize current step and exit to menu: ");
	//	c=0;
	//	c=getchar();
	//	if(c == 'q') {
	//	  printf("\nBreaking time integration (user input)!");
	//	  break;
	//	}
	//}
	//}

	
  }  //end loop over timesteps
  
  
  if (iadapt == 0) {
#ifdef PARALLEL
    /* free exchange tables for DOFs */
    appr_free_exchange_tables(1);
#endif
    
    
    if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
      sir_free(solver_ns_supg_id); 
    }
    
  }
  
  if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
    sir_destroy(solver_ns_supg_id);
  }

  return;
}


