/************************************************************************
File pds_ns_supg_materials.c - definition of functions related to materials
			   handling

Contains definition of routines:
  pdr_ns_supg_material_query - gets material data

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

/* types and functions related to materials handling */
#include "../include/pdh_ns_supg_materials.h"	/* IMPLEMENTS */
#include "uth_mat.h"

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_ns_supg_material_query - gets material data
------------------------------------------------------------*/
int pdr_ns_supg_material_query(const utt_material_query_params *Params,
  utt_material_query_result *Result)
{
	utr_mat_init_query_result(Result);

	Result->dynamic_viscosity = UTC_MAT_QUERY_RESULT_REQUIRED;
	Result->density = UTC_MAT_QUERY_RESULT_REQUIRED;
	
	return utr_material_query(Params, Result);
}
