/************************************************************************
File pdh_vof_problem.h - problem module's types and functions

Contains problem module defines (see below)

Contains definition of types:
  pdt_vof_ctrls
  pdt_vof_times
  pdt_vof_nonls
  pdt_vof_linss
  pdt_vof_adpts
  pdt_vof_problem - aggregates above ones
  
Procedures:
  pdr_vof_problem_clear - clear problem data
  pdr_vof_problem_read - read problem data
  pdr_vof_problem_voronoi - create Voronoi diagram for cell nodes
  pdr_vof_get_vof_at_point - to provide VOF at point
  pdr_vof_get_mat_at_point - to provide material at point
------------------------------
History:
        initial version - Krzysztof Banas
        2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
        2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#ifndef PDH_VOF_PROBLEM
#define PDH_VOF_PROBLEM

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to boundary conditions handling */
#include "pdh_vof_bc.h" 

/* types and functions related to materials handling */
#include "pdh_vof_materials.h"

/**************************************/
/* DEFINES                            */
/**************************************/
/* Rules:
/* - always uppercase */
/* - name starts with PDC_ */

#ifdef __cplusplus
extern "C" {
#endif


#define PDC_VOF_MAXEQ 1
#define PDC_VOF_NREQ 1
#define PDC_MAT_NREQ 1

#define PDC_VOF_NCOMP 1   //number of material components, at the moment sulfur only
#define PDC_VOF_ERROR_INDI (1.0E-1)
/**************************************/
/* TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always with pdt_ */

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
  // int     base;      /* parameter specifying the type of basis functions */
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
  //  double  ref_temperature;
  //  double  ambient_temperature;
  int     *materials_used; /* indexes of materials used in the model, materials_used[0] - materials count */
} pdt_vof_ctrls;

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
  //  double  ref_temperature;
  //  double  ambient_temperature;  
} pdt_mat_ctrls;

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
  int 	 intv_graph_accu; /* auto graphics dumpout accuracy */
  /*** PROBLEM SPECIFIC - CAN BE MODIFIED ***/
} pdt_vof_times;

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
} pdt_vof_nonls;

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
} pdt_vof_linss;

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
} pdt_vof_adpts;

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
} pdt_mat_adpts;

/* problem definition data structure */
typedef struct {
    pdt_vof_ctrls  ctrl;    /* structure with control parameters */
    pdt_vof_times  time;   /* structure with time integration parameters */
    pdt_vof_nonls  nonl;   /* structure with nonlinear solver parameters */
    pdt_vof_linss  lins;   /* structure with linear solver parameters */
    pdt_vof_adpts  adpt;    /* structure with adaptation parameters */
    pdt_vof_materials materials; /* main structure containing materials data */
    pdt_vof_bc bc; /* main structure containing bc data */
} pdt_vof_problem;

/* problem definition data structure */
typedef struct {
    pdt_mat_ctrls  ctrl;    /* structure with control parameters */
    pdt_mat_adpts  adpt;    /* structure with adaptation parameters */
} pdt_mat_problem;



/**-----------------------------------------------------------
pdr_vof_problem_clear - clear problem data
------------------------------------------------------------*/
int pdr_vof_problem_clear(pdt_vof_problem *Problem);

/**-----------------------------------------------------------
pdr_mat_problem_clear - clear problem data
------------------------------------------------------------*/
extern  int pdr_mat_problem_clear(pdt_mat_problem *Problem);

/**-----------------------------------------------------------
pdr_vof_problem_read - read problem data
------------------------------------------------------------*/
int pdr_vof_problem_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_vof_problem *Problem,
  int Nr_sol // nr_sol is time integration dependent - specified by main
							 );

/**-----------------------------------------------------------
pdr_vof_problem_voronoi - create Voronoi diagram for cell nodes
------------------------------------------------------------*/
extern int pdr_vof_problem_voronoi(pdt_vof_problem *Problem);

/**-----------------------------------------------------------
pdr_vof_problem_vof - update vof field
------------------------------------------------------------*/
int pdr_vof_problem_vof(pdt_vof_problem *Problem);

/**-----------------------------------------------------------
pdr_vof_get_vof_at_point - to provide the vof and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_vof_weakform.c)
------------------------------------------------------------*/
int pdr_vof_get_vof_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Vof, // volume of fluid
  double *DVof_dx, // x-derivative of temperature
  double *DVof_dy, // y-derivative of temperature
  double *DVof_dz // z-derivative of temperature
);

/**-----------------------------------------------------------
pdr_vof_get_mat_at_point - to provide the material and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_vof_weakform.c)
------------------------------------------------------------*/
int pdr_vof_get_mat_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Mat, // material
  double *DMat_dx, // x-derivative of temperature
  double *DMat_dy, // y-derivative of temperature
  double *DMat_dz // z-derivative of temperature
);

#ifdef __cplusplus
}
#endif

#endif
