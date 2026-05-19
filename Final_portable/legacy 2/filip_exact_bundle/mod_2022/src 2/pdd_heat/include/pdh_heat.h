/************************************************************************
File pdh_heat.h - problem module's types and functions
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_heat_weakform.h)

Contains problem module defines (see below)


Contains declarations of routines:
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_heat_problem.h)
  pdr_heat_init - to initialize problem data 
  pdr_heat_time_integration - time integration driver
  pdr_heat_error - to compute estimated norm of error  
  pdr_heat_adapt - to enforce adaptation strategy for a given problem 
  pdr_heat_dump_data - dump data to files
  pdr_heat_write_paraview - to write graphics data to a disk file

------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#ifndef PDH_HEAT
#define PDH_HEAT

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to problem structures */
#include "pdh_heat_problem.h" 
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

extern double pdv_heat_timer_all;
extern double pdv_heat_timer_pdr_comp_el_stiff_mat;
extern double pdv_heat_timer_pdr_comp_fa_stiff_mat;

// ID of the current problem
extern int pdv_heat_current_problem_id;	/* ID of the current problem */
// problem structure for heat module
extern pdt_heat_problem pdv_heat_problem;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
  pdr_heat_init - to initialize problem data 
----------------------------------------------------------*/
int pdr_heat_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
  );

/**-----------------------------------------------------------
pdr_heat_time_integration - time integration driver
------------------------------------------------------------*/  
extern void pdr_heat_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_heat_error - to compute estimated norm of error  
usually based on recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
extern double pdr_heat_error(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_heat_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_heat_ZZ_error(		
	   /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
);

/**--------------------------------------------------------
pdr_heat_err_indi_ZZ - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_heat_err_indi_ZZ(		
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**--------------------------------------------------------
pdr_heat_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
extern int pdr_heat_adapt(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**-----------------------------------------------------------
pdr_heat_dump_data - dump data to files
------------------------------------------------------------*/
extern int pdr_heat_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**--------------------------------------------------------
  pdr_heat_write_paraview - to write graphics data to a disk file
----------------------------------------------------------*/
extern int pdr_heat_write_paraview(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );

#ifdef __cplusplus
}
#endif

#endif
