/************************************************************************
File pdh_vof.h - problem module's types and functions
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_vof_weakform.h)

Contains problem module defines (see below)


Contains declarations of routines:
 BY ASSUMPTION NOT CALLED BY OTHER MODULES' FUNCTIONS
 (functions called by other modules are in pdh_vof_problem.h)
  pdr_vof_init - to initialize problem data 
  pdr_vof_time_integration - time integration driver
  pdr_vof_error - to compute estimated norm of error  
  pdr_vof_adapt - to enforce adaptation strategy for a given problem 
  pdr_vof_dump_data - dump data to files
  pdr_vof_write_paraview - to write graphics data to a disk file

------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#ifndef PDH_VOF
#define PDH_VOF

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to problem structures */
#include "pdh_vof_problem.h" 
// bc and material header files are included in problem header files

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


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
  pdr_vof_init - to initialize problem data 
----------------------------------------------------------*/
int pdr_vof_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
  );

/**-----------------------------------------------------------
pdr_vof_time_integration - time integration driver
------------------------------------------------------------*/  
extern void pdr_vof_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_vof_error - to compute estimated norm of error  
usually based on recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
extern double pdr_vof_error(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_vof_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
extern int pdr_vof_adapt(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**-----------------------------------------------------------
pdr_vof_dump_data - dump data to files
------------------------------------------------------------*/
void pdr_vof_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**--------------------------------------------------------
  pdr_vof_write_paraview - to write graphics data to a disk file
----------------------------------------------------------*/
int pdr_vof_write_paraview(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );


#endif
