/************************************************************************
File pdh_forming.h - problem module's types and functions
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_forming_weakform.h)

Contains problem module defines (see below)


Contains declarations of routines:
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_forming_problem.h)
  pdr_forming_time_integration - time integration driver (in time_integration)
  pdr_forming_error - to compute estimated norm of error (in adapt) 
  pdr_forming_adapt - to enforce adaptation strategy for forming problem (in adapt)
  pdr_forming_dump_data - dump data to files (in input_output)
  pdr_forming_write_paraview - to write graphics data to file (in input_output)

utilities:
  pdr_forming_post_process
  pdr_forming_write_profile
  pdr_forming_initial_condition

------------------------------
History:
	initial version - Krzysztof Banas (ns_supg)
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
    2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
    2015    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl) (forming)
*************************************************************************/

#ifndef PDH_FORMING
#define PDH_FORMING

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to problem structures */
#include "pdh_forming_problem.h" 
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

#define PDC_FORMING_ID  10

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

extern double pdv_forming_timer_all;
extern double pdv_forming_timer_pdr_comp_el_stiff_mat;
extern double pdv_forming_timer_pdr_comp_fa_stiff_mat;


// ID of the current problem
extern int pdv_forming_current_problem_id;	/* ID of the current problem */
// problem structure for forming module
extern pdt_forming_problem pdv_forming_problem;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */


/**-----------------------------------------------------------
pdr_forming_time_integration - time integration driver
------------------------------------------------------------*/  
extern void pdr_forming_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_forming_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
extern double pdr_forming_error(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_forming_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
extern int pdr_forming_adapt(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_forming_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_forming_ZZ_error(		
	   /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
);

/**--------------------------------------------------------
pdr_forming_err_indi_ZZ - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_forming_err_indi_ZZ(		
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**--------------------------------------------------------
pdr_forming_err_indi_explicit - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_forming_err_indi_explicit(	
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**-----------------------------------------------------------
pdr_forming_dump_data - dump data to files
------------------------------------------------------------*/
extern int pdr_forming_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**--------------------------------------------------------
  pdr_forming_write_paraview - to write graphics data to a disk file
----------------------------------------------------------*/
extern int pdr_forming_write_paraview(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );


  // IN: MAIN/PDS_FORMING_UTIL.C

/**-----------------------------------------------------------
pdr_forming_post_process - simple post-processing
------------------------------------------------------------*/
double pdr_forming_post_process(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**-----------------------------------------------------------
pdr_forming_profile - to dump a set of values along a line
------------------------------------------------------------*/
int pdr_forming_write_profile(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**-----------------------------------------------------------
pdr_forming_initial_condition - procedure passed as argument
  to field initialization routine in order to provide problem
  dependent initial condition data
------------------------------------------------------------*/
double pdr_forming_initial_condition(
  int Field_id, // field_id - each problem should know its field id
  double *Coor,   // point coordinates
  int Sol_comp_id // solution component
);


#ifdef __cplusplus
}
#endif

#endif
