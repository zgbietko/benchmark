/************************************************************************
File pds_heat_materials.c- definition of functions related to materials
			   handling

Contains definition of routines:
  pdr_heat_material_query - gets material data

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#include <string.h>
#include <stdlib.h>

/* types and functions related to materials handling */
#include "../include/pdh_heat_materials.h"	/* IMPLEMENTS */
//#include "uth_log.h"

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_heat_material_query - gets material data
------------------------------------------------------------*/
int pdr_heat_material_query(const utt_material_query_params *Params,
  utt_material_query_result *Result)
{
  utr_mat_init_query_result(Result);

  Result->density = UTC_MAT_QUERY_RESULT_REQUIRED;
  Result->thermal_conductivity = UTC_MAT_QUERY_RESULT_REQUIRED;
  Result->specific_heat = UTC_MAT_QUERY_RESULT_REQUIRED;

  return utr_material_query(Params,Result);
}
