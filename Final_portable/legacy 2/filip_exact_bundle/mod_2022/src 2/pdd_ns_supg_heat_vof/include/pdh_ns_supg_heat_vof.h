/************************************************************************
File pdh_ns_supg_heat_vof.h - problem module's types and functions

Contains problem module defines (see below)


Contains declarations of routines:
  pdr_ns_supg_heat_vof_init - to initialize problem data 
  pdr_ns_supg_heat_vof_time_integration - time integration driver
  pdr_ns_supg_heat_vof_error - to compute estimated norm of error  
  pdr_ns_supg_heat_vof_adapt - to enforce adaptation strategy for a given problem 
  pdr_ns_supg_heat_vof_dump_data - dump data to files
  pdr_ns_supg_heat_vof_write_paraview - to write graphics data to a disk file

------------------------------
History:
	initial version - Krzysztof Banas
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#ifndef PDH_NS_SUPG_HEAT_VOF
#define PDH_NS_SUPG_HEAT_VOF

#include <stdio.h>

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
#include "../../pdd_vof/include/pdh_vof_problem.h"

// TODO_AS #include "../../pdd_vof/include/pdh_vof_problem.h"
// bc and material header files are included in problem header files

/**************************************/
/* DEFINES                            */
/**************************************/
/* Rules:
/* - always uppercase */
/* - name starts with PDC_ */

// IDs of component problems:
#define PDC_NS_SUPG_ID   1
#define PDC_HEAT_ID      2
#define PDC_HEAT_DTDT_ID 3
#define PDC_VOF_ID       4
#define PDC_MAT_ID       5

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

extern double pdv_ns_supg_heat_vof_timer_all;
extern double pdv_ns_supg_heat_vof_timer_pdr_comp_el_stiff_mat;
extern double pdv_ns_supg_heat_vof_timer_pdr_comp_fa_stiff_mat;

// ID of the current problem
// on purpose initialized to 0 which is wrong value !
// later should be replaced by one of the two proper values:
// ns_supg -> problem_id = 1
// heat -> problem_id = 2
// heat_dtdt -> problem_id = 3
// heat_vof problem_id = 4
// heat_mat -> problem_id = 5
extern int pdv_ns_supg_heat_vof_current_problem_id;	/* ID of the current problem */
// problem structure for ns_supg module
extern pdt_ns_supg_problem pdv_ns_supg_problem;
// problem structure for heat module
extern pdt_heat_problem pdv_heat_problem;
// problem structure for heating-cooling
extern pdt_heat_dtdt_problem pdv_heat_dtdt_problem;
// problem structure for volume of fluid
extern pdt_vof_problem pdv_vof_problem;
// problem structure for material
extern pdt_mat_problem pdv_mat_problem;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
  pdr_ns_supg_heat_vof_init - to initialize problem data 
                          (including mesh and two fields)
----------------------------------------------------------*/
extern int pdr_ns_supg_heat_vof_init(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
  );

/**-----------------------------------------------------------
pdr_ns_supg_heat_vof_time_integration - time integration driver
------------------------------------------------------------*/  
extern void pdr_ns_supg_heat_vof_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_heat_vof_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
extern double pdr_ns_supg_heat_vof_error(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_heat_vof_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
extern int pdr_ns_supg_heat_vof_adapt(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output);

/**--------------------------------------------------------
pdr_ns_supg_heat_vof_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_ns_supg_heat_vof_ZZ_error(		
	   /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
);

/**--------------------------------------------------------
pdr_ns_supg_heat_vof_err_indi_ZZ - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_ns_supg_heat_vof_err_indi_ZZ(		
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**--------------------------------------------------------
pdr_ns_supg_heat_vof_err_indi_explicit - to return error indicator for an element,
        based on ZZ first derivative recovery 
----------------------------------------------------------*/
extern double pdr_ns_supg_heat_vof_err_indi_explicit(	
                        /* returns error indicator for an element */
  int Problem_id,	/* in: data structure to be used  */
  int El	/* in: element number */
    );

/**-----------------------------------------------------------
pdr_ns_supg_heat_vof_dump_data - dump data to files
------------------------------------------------------------*/
extern int pdr_ns_supg_heat_vof_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
);

/**--------------------------------------------------------
  pdr_ns_supg_heat_vof_write_paraview - to write graphics data to a disk file
----------------------------------------------------------*/
extern int pdr_ns_supg_heat_vof_write_paraview(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );

/**-----------------------------------------------------------
pdr_heat_vof_material_query - gets material data for node belonging to patch
------------------------------------------------------------*/
extern int pdr_heat_vof_material_query(
  const int node_id,
  const utt_patches *pdv_patches, 
  utt_material_query_params *Params,
  utt_material_query_result **Result);

/**-----------------------------------------------------------
pdr_ns_supg_vof_material_query - gets material data for node belonging to patch
------------------------------------------------------------*/
extern int pdr_ns_supg_vof_material_query(
  const int node_id,
  const utt_patches *pdv_patches,
  utt_material_query_params *Params,
  utt_material_query_result **Result);

/**--------------------------------------------------------
pdr_ns_supg_heat_vof_refine - to enforce mesh refinement for ns_supg+heat+vof problem 
---------------------------------------------------------*/
extern int pdr_ns_supg_heat_vof_refine(  /* returns: >0 - success, <=0 - failure */
  int Problem_id, // leading problem ID
  int Ref_type, // type of refinement 
  FILE *Interactive_output
);

/**-----------------------------------------------------------
pdr_dtdt_problem_clear - clear problem data
------------------------------------------------------------*/
extern int pdr_heat_dtdt_problem_clear(pdt_heat_dtdt_problem *Problem);

#endif
