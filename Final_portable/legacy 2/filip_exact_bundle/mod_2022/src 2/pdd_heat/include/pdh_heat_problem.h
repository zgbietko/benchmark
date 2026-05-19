/************************************************************************
File pdh_heat_problem.h - problem module's types and functions

Contains problem module defines (see below)

Contains definition of types:
  pdt_heat_ctrls
  pdt_heat_times
  pdt_heat_nonls
  pdt_heat_linss
  pdt_heat_adpts
  pdt_heat_problem - aggregates above ones
  
Procedures:
  pdr_heat_problem_clear - clear problem data
  pdr_heat_problem_read - read problem data


------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#ifndef PDH_HEAT_PROBLEM
#define PDH_HEAT_PROBLEM

#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to boundary conditions handling */
#include "pdh_heat_bc.h" 

/* types and functions related to materials handling */
#include "pdh_heat_materials.h"

/**************************************/
/* DEFINES                            */
/**************************************/
/* Rules:
/* - always uppercase */
/* - name starts with PDC_ */

#define PDC_HEAT_MAXEQ 1
#define PDC_HEAT_NREQ 1

	// PDC_HEAT_SBC - Stefan-Boltzmann constant [W / m^2 K^4]
#define PDC_HEAT_SBC 5.670373E-08


/**************************************/
/* TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always witn pdt_ */

/* structure with control parameters */
typedef struct {
  /*** GENERIC - DO NOT CHANGE !!! ***/
  int     name;         /* name (identifier for problem dependent routines) */
  int     mesh_id;      /* ID of the associated mesh */
  int     field_id;     /* ID of the associated approximation field */
  int     nr_sol;       /* number of solution vectors stored by approximation */
                        /* module (for time dependent/nonlinear problems) */
  int     nreq;         /* number of equations (solution components) */
  int     solver_id;    /* ID of the associated solver */
  // for continuous standard shape functions global base is not used
  // int     base;         /* parameter specifying the type of basis functions */
                        /* interpreted by particular approximation modules */
  char    mesh_type[2];
  
  char    work_dir[300];
  FILE*   interactive_input;
  FILE*   interactive_output;
  char    mesh_filename[300];
  char    field_filename[300];
  char    material_filename[300];
  char    bc_filename[300];
  char    solver_filename[300];
  char    field_dmp_filepattern[50];
  char    mesh_dmp_filepattern[50];
  /*** PROBLEM SPECIFIC - CAN BE MODIFIED ***/
  double  penalty; 
  double  ref_temperature;
  double  ambient_temperature; 
  double  thermal_conductivity;
  double  density;
  double  specific_heat; 
} pdt_heat_ctrls;


/* structure with time integration parameters */
typedef struct {
  /*** GENERIC - DO NOT CHANGE !!! ***/
  int    type;          /* type of time integration scheme */
                        /*      0 - no time integration */
                        /*      1 - alpha scheme  */
  double	alpha;	/* implicitnes parameter alpha */	

  int    cur_step;      /* current time-step number */
  double cur_time;      /* current time */
  double cur_dtime;     /* current time-step length */
  double prev_dtime;    /* previous time-step length */

  int    final_step;    /* time-step number to stop simulation */
  double final_time;    /* time to stop simulation */
  
  int    conv_type;     /* convergence criterion number */
  double conv_meas;     /* convergence measure */
  int    monitor;       /* monitoring level: */
                        /*   PDC_SILENT      0  */
                        /*   PDC_ERRORS      1  */
                        /*   PDC_INFO        2  */
                        /*   PDC_ALLINFO     3  */
  int    intv_dumpout;  /* interval (in time steps) for dumping out data */
  int    intv_graph;    /* interval (in time steps) for graphics output */
  int 	 graph_accu; /* auto graphics dumpout accuracy */
  /*** PROBLEM SPECIFIC - CAN BE MODIFIED ***/
  int CFL_control;
  double CFL_limit;
  double time_step_length_mult;
  double reference_time_step_length;
} pdt_heat_times;

/* structure with nonlinear solver control parameters */
typedef struct {
  /*** GENERIC - DO NOT CHANGE !!! ***/
  int    type;          /* method identifier */
                        /*      0 - problem is linear */
  int    max_iter;      /* maximal iteration number */
  int    conv_type;     /* convergence criterion number */
  double conv_meas;     /* convergence measure */
  int    monitor;       /* monitoring level: */
                        /*   PDC_SILENT      0  */
                        /*   PDC_ERRORS      1  */
                        /*   PDC_INFO        2  */
                        /*   PDC_ALLINFO     3  */
  /*** PROBLEM SPECIFIC - CAN BE MODIFIED ***/
} pdt_heat_nonls;

/* structure with linear solver control parameters */
typedef struct {
  /*** GENERIC - DO NOT CHANGE !!! ***/
  int    type;          /* method identifier */
                        /*       0 - direct solver (?) */
                        /*       1 - precondittioned GMRES */
                        /*       2 - multigrid preconditioned  GMRES */
                        /*      10 - standard iterations */
                        /*      20 - V-cycle multigrid */
  int    max_iter;      /* maximal iteration number */
  int    conv_type;     /* convergence criterion number */
                        /*      0 - relative to initial residual */
                        /*      1 - absolute residual */
                        /*      2 - relative to rhs */
  double conv_meas;     /* convergence measure */
  int    monitor;       /* monitoring level: */
                        /*   PDC_SILENT      0  */
                        /*   PDC_ERRORS      1  */
                        /*   PDC_INFO        2  */
                        /*   PDC_ALLINFO     3  */
  /*** PROBLEM SPECIFIC - CAN BE MODIFIED ***/
} pdt_heat_linss;

/* structure with adaptation parameters */
typedef struct {
  /*** GENERIC - DO NOT CHANGE !!! ***/
  int    type;          /* strategy number for adaptation */
  int    interval;      /* number of time steps between adaptations */
  int    maxgen;        /* maximum generation level for elements */
  double eps;           /* coefficient for choosing elements to adapt */
  double ratio;         /* ratio of errors for derefinements */
  int    monitor;       /* monitoring level: */
                        /*   PDC_SILENT      0  */
                        /*   PDC_ERRORS      1  */
                        /*   PDC_INFO        2  */
                        /*   PDC_ALLINFO     3  */
  /*** PROBLEM SPECIFIC - CAN BE MODIFIED ***/
} pdt_heat_adpts;


/* problem definition data structure */
typedef struct {
    pdt_heat_ctrls  ctrl;    /* structure with control parameters */
    pdt_heat_times  time;   /* structure with time integration parameters */
    pdt_heat_nonls  nonl;   /* structure with nonlinear solver parameters */
    pdt_heat_linss  lins;   /* structure with linear solver parameters */
    pdt_heat_adpts  adpt;    /* structure with adaptation parameters */
    //pdt_heat_materials materials; /* main structure containing materials data */
    pdt_heat_bc bc; /* main structure containing bc data */
} pdt_heat_problem;


/**-----------------------------------------------------------
pdr_heat_problem_clear - clear problem data
------------------------------------------------------------*/
extern  int pdr_heat_problem_clear(pdt_heat_problem *Problem);

/**--------------------------------------------------------
pdr_ns_supg_heat_refine - to enforce mesh refinement for ns_supg+heat problem 
---------------------------------------------------------*/
int pdr_ns_supg_heat_refine(  /* returns: >0 - success, <=0 - failure */
  int Problem_id, // leading problem ID
  int Ref_type, // type of refinement 
  FILE *Interactive_output
);

/**-----------------------------------------------------------
pdr_heat_get_bc_assign_count - get num of boundaries with conditions set in file
------------------------------------------------------------*/
int pdr_heat_get_bc_assign_count(const pdt_heat_bc * Bc_db);

/**-----------------------------------------------------------
pdr_heat_problem_read - read problem data
------------------------------------------------------------*/
extern int pdr_heat_problem_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_heat_problem *Problem,
  int Nr_sol // nr_sol is time integration dependent - specified by main
							 );

#ifdef __cplusplus
}
#endif


#endif
