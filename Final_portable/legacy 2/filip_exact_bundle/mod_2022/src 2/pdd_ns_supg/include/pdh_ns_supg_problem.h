/************************************************************************
File pdh_ns_supg_problem.h - problem module's type

Contains problem module defines (see below)

Contains definition of types:
  pdt_ns_supg_ctrls
  pdt_ns_supg_times
  pdt_ns_supg_nonls
  pdt_ns_supg_linss
  pdt_ns_supg_adpts
  pdt_ns_supg_problem - aggregates above ones
  
------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#ifndef PDH_NS_SUPG_PROBLEM
#define PDH_NS_SUPG_PROBLEM

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to boundary conditions handling */
#include "pdh_ns_supg_bc.h" 

/* types and functions related to materials handling */
#include "pdh_ns_supg_materials.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/**************************************/
/* DEFINES                            */
/**************************************/
/* Rules:
/* - always uppercase */
/* - name starts with PDC_ */

// maximal number of equations (solution components) in component modules
#define PDC_NS_SUPG_MAXEQ 4
#define PDC_NS_SUPG_NREQ 4


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
// for continuous basis functions problem->ctrl.base is not used
  //int     base;         /* parameter specifying the type of basis functions */
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
  double  penalty; // for boundary conditions
  double  gravity_field[3];  
  double  ref_temperature; // if <=0 - constant density and viscosity are used
  double  density;  //material property (this value used only for ref_temperature <=0 )
  double  dynamic_viscosity; //material property (this value used only for ref_temperature <=0 )
  double  ref_length;   // should match problem definition
  double  ref_velocity; // should match problem definition
  //double  reynolds_number; // never specified, always computed
} pdt_ns_supg_ctrls;

/* structure with time integration parameters */
typedef struct {
  /*** GENERIC - DO NOT CHANGE !!! ***/
  int    type;          /* type of time integration scheme */
                        /*      0 - no time integration */
                        /*      1 - alpha (a.k.a. theta) scheme  */
  double alpha;	/* implicitnes parameter alpha (a.k.a. theta)*/	

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
  int 	 graph_accu;    /* auto graphics dumpout accuracy */
  /*** PROBLEM SPECIFIC - CAN BE MODIFIED ***/
  int CFL_control;
  double CFL_limit;
  double time_step_length_mult;
  double reference_time_step_length;
} pdt_ns_supg_times;

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
} pdt_ns_supg_nonls;

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
} pdt_ns_supg_linss;

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
} pdt_ns_supg_adpts;

/* problem definition data structure */
typedef struct {
    pdt_ns_supg_ctrls  ctrl;    /* structure with control parameters */
    pdt_ns_supg_times  time;   /* structure with time integration parameters */
    pdt_ns_supg_nonls  nonl;   /* structure with nonlinear solver parameters */
    pdt_ns_supg_linss  lins;   /* structure with linear solver parameters */
    pdt_ns_supg_adpts  adpt;    /* structure with adaptation parameters */
    //pdt_ns_supg_materials materials; /* structure containing materials data */
    pdt_ns_supg_bc bc; /* structure containing bc data */
} pdt_ns_supg_problem;
  

/**-----------------------------------------------------------
pdr_ns_supg_problem_clear - clear problem data
------------------------------------------------------------*/
extern  int pdr_ns_supg_problem_clear(pdt_ns_supg_problem *Problem);

  /**-----------------------------------------------------------
pdr_ns_supg_problem_read - read problem data
------------------------------------------------------------*/
extern int pdr_ns_supg_problem_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_ns_supg_problem *Problem,
  int Nr_sol // nr_sol is time integration dependent - specified by main
							 );
  
#ifdef __cplusplus
}
#endif

#endif
