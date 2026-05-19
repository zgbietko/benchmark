/************************************************************************
File pds_ns_supg_main.c - utilities for ns_supg module - approximation
      of Navier_Stokes equations

Implementation of pdh_ns_supg.h:
  pdr_ns_supg_post_process
  pdr_ns_supg_write_profile
  pdr_ns_supg_initial_condition

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
#ifdef MOD_FEM_VIEWER
#include "mod_fem_viewer.h"
#endif


/* USED AND IMPLEMENTED PROBLEM DEPENDENT DATA STRUCTURES AND INTERFACES */

/* problem dependent module interface */
#include "pdh_intf.h"	

/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"

/* problem module's types and functions */
#include "../include/pdh_ns_supg.h"	
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
// bc and material header files are included in problem header files




/*------------------------------------------------------------
pdr_ns_supg_initial_condition
------------------------------------------------------------*/
double pdr_ns_supg_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Coor,   // point coordinates
  int Sol_comp_id // solution component
)
{

  return (0.0);
}

/*------------------------------------------------------------
pdr_ns_supg_post_process
------------------------------------------------------------*/
double pdr_ns_supg_post_process(
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

  problem_id = PDC_NS_SUPG_ID;
  
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
pdr_ns_supg_write_profile
------------------------------------------------------------*/
int pdr_ns_supg_write_profile(
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

  problem_id = PDC_NS_SUPG_ID;
  
  pdv_ns_supg_current_problem_id = problem_id;
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
    
    return pdr_ns_supg_err_indi_explicit(Problem_id, El);
    
  } else if (Mode == PDC_ADAPT_ZZ) {
    
    return pdr_ns_supg_err_indi_ZZ(Problem_id, El);
    
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
    
  }
  
  return (-1);
}

/*------------------------------------------------------------
pdr_ctrl_d_params - to return one of control parameters
------------------------------------------------------------*/
double pdr_ctrl_d_params(int Problem_id, int Num)
{
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

  }

  return(0.0);
}

/*------------------------------------------------------------
pdr_adapt_i_params - to return parameters of adaptation
------------------------------------------------------------*/
int pdr_adapt_i_params(int Problem_id, int Num)
{

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

  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    if (Num == 1)      return (times_ns_supg->type);
    else if (Num == 3)      return (times_ns_supg->cur_step);
    else if (Num == 4)      return (times_ns_supg->final_step);
    else if (Num == 9)      return (times_ns_supg->conv_type);
    else if (Num == 10)      return (times_ns_supg->monitor);
    else if(Num==11) return(times_ns_supg->intv_dumpout);
    else if(Num==12) return(times_ns_supg->intv_graph);
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


  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Problem_id==PDC_NS_SUPG_ID){
    if (Num == 2)
      return (times_ns_supg->alpha);
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

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  if(Problem_id==PDC_NS_SUPG_ID){

    if(Num==2) times_ns_supg->alpha=Value;
    else if(Num==4) times_ns_supg->cur_time=Value;
    else if(Num==5) times_ns_supg->final_time=Value;
    else if(Num==6) times_ns_supg->cur_dtime=Value;
    else if(Num==7) times_ns_supg->prev_dtime=Value;
    else if(Num==9) times_ns_supg->conv_meas=Value;
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

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper problem data structure */
  if(Problem_id==PDC_NS_SUPG_ID){


    if(Num==4) return(linss_ns_supg->conv_meas);
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

/* local variables */
  pdt_ns_supg_adpts *adpts_ns_supg = &pdv_ns_supg_problem.adpt;
  pdt_ns_supg_times *times_ns_supg = &pdv_ns_supg_problem.time;
  pdt_ns_supg_linss *linss_ns_supg = &pdv_ns_supg_problem.lins;
  pdt_ns_supg_ctrls *ctrls_ns_supg = &pdv_ns_supg_problem.ctrl;
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

  return;
}

