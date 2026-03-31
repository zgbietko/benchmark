/************************************************************************
File pdh_ns_supg_ale.h - problem module's types and functions
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_ns_supg_weakform.h)

Contains problem module defines (see below)


Contains declarations of routines:
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_ns_supg_ale_problem.h)
  pdr_ns_supg_ale_time_integration - time integration driver (in time_integration)
  pdr_ns_supg_ale_error - to compute estimated norm of error (in adapt) 
  pdr_ns_supg_ale_adapt - to enforce adaptation strategy for ns problem (in adapt)
  pdr_ns_supg_ale_dump_data - dump data to files (in input_output)
  pdr_ns_supg_ale_write_paraview - to write graphics data to file (in input_output)

utilities:
  pdr_ns_supg_ale_post_process
  pdr_ns_supg_ale_write_profile
  pdr_ns_supg_ale_initial_condition

------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#ifndef PDH_NS_SUPG_ALE
#define PDH_NS_SUPG_ALE

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
// bc and material header files are included in problem header files

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

#define PDC_NS_SUPG_ID  1

/**************************************/
/* TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always witn pdt_ */


/**************************************/
/* GLOBAL VARIABLES                   */
/**************************************/
/* Rules:
/* - name always begins with pdv_ */
/* - constants always uppercase and start with PDC_ */

extern double pdv_ns_supg_ale_timer_all;
extern double pdv_ns_supg_ale_timer_pdr_comp_el_stiff_mat;
extern double pdv_ns_supg_ale_timer_pdr_comp_fa_stiff_mat;


// ID of the current problem
extern int pdv_ns_supg_ale_current_problem_id;	/* ID of the current problem */
// problem structure for ns_supg module
extern pdt_ns_supg_problem pdv_ns_supg_problem;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */


/**-----------------------------------------------------------
pdr_ns_supg_ale_time_integration - time integration driver
------------------------------------------------------------*/  
extern void pdr_ns_supg_ale_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_ale_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
extern double pdr_ns_supg_ale_error(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_ale_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
extern int pdr_ns_supg_ale_adapt(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_ale_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_ns_supg_ale_ZZ_error(		
	   /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
);

/**--------------------------------------------------------
pdr_ns_supg_ale_err_indi_ZZ - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_ns_supg_ale_err_indi_ZZ(		
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**--------------------------------------------------------
pdr_ns_supg_ale_err_indi_explicit - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_ns_supg_err_ale_indi_explicit(	
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**-----------------------------------------------------------
pdr_ns_supg_ale_dump_data - dump data to files
------------------------------------------------------------*/
extern int pdr_ns_supg_ale_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**--------------------------------------------------------
  pdr_ns_supg_ale_write_paraview - to write graphics data to a disk file
----------------------------------------------------------*/
extern int pdr_ns_supg_ale_write_paraview(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );


  // IN: MAIN/PDS_NS_SUPG_UTIL.C

/**-----------------------------------------------------------
pdr_ns_supg_ale_post_process - simple post-processing
------------------------------------------------------------*/
double pdr_ns_supg_ale_post_process(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**-----------------------------------------------------------
pdr_ns_supg_ale_write_profile - to dump a set of values along a line
------------------------------------------------------------*/
int pdr_ns_supg_ale_write_profile(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**-----------------------------------------------------------
pdr_ns_supg_ale_initial_condition - procedure passed as argument
  to field initialization routine in order to provide problem
  dependent initial condition data
------------------------------------------------------------*/
double pdr_ns_supg_ale_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Coor,   // point coordinates
  int Sol_comp_id // solution component
);


#ifdef __cplusplus
}
#endif

#endif
