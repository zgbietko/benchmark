/************************************************************************
File pdh_ns_supg_heat.h - problem module's types and functions

Contains problem module defines (see below)


Contains declarations of routines:
  pdr_ns_supg_heat_init - to initialize problem data 
  pdr_ns_supg_heat_time_integration - time integration driver
  pdr_ns_supg_heat_error - to compute estimated norm of error  
  pdr_ns_supg_heat_adapt - to enforce adaptation strategy for a given problem 
  pdr_ns_supg_heat_dump_data - dump data to files
  pdr_ns_supg_heat_write_paraview - to write graphics data to a disk file

------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#ifndef PDH_NS_SUPG_HEAT
#define PDH_NS_SUPG_HEAT

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
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

// IDs of component problems:
#define PDC_NS_SUPG_ID   1
#define PDC_HEAT_ID      2

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

extern double pdv_ns_supg_heat_timer_all;
extern double pdv_ns_supg_heat_timer_pdr_comp_el_stiff_mat;
extern double pdv_ns_supg_heat_timer_pdr_comp_fa_stiff_mat;

// ID of the current problem
// on purpose initialized to 0 which is wrong value !
// later should be replaced by one of the two proper values:
// ns_supg -> problem_id = 1
// heat -> problem_id = 2
extern int pdv_ns_supg_heat_current_problem_id;	/* ID of the current problem */
// problem structure for ns_supg module
extern pdt_ns_supg_problem pdv_ns_supg_problem;
// problem structure for heat module
extern pdt_heat_problem pdv_heat_problem;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
  pdr_ns_supg_heat_init - to initialize problem data 
                          (including mesh and two fields)
----------------------------------------------------------*/
extern int pdr_ns_supg_heat_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
  );

/**-----------------------------------------------------------
pdr_ns_supg_heat_time_integration - time integration driver
------------------------------------------------------------*/  
extern void pdr_ns_supg_heat_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_heat_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
extern double pdr_ns_supg_heat_error(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_heat_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
extern int pdr_ns_supg_heat_adapt(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_heat_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_ns_supg_heat_ZZ_error(		
	   /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
);

/**--------------------------------------------------------
pdr_ns_supg_heat_err_indi_ZZ - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_ns_supg_heat_err_indi_ZZ(		
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**--------------------------------------------------------
pdr_ns_supg_heat_err_indi_explicit - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_ns_supg_heat_err_indi_explicit(	
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**-----------------------------------------------------------
pdr_ns_supg_heat_dump_data - dump data to files
------------------------------------------------------------*/
extern int pdr_ns_supg_heat_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**--------------------------------------------------------
  pdr_ns_supg_heat_write_paraview - to write graphics data to a disk file
----------------------------------------------------------*/
extern int pdr_ns_supg_heat_write_paraview(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );



#ifdef __cplusplus
}
#endif

#endif
