/************************************************************************
File pdh_forming_materials.h - types and functions related to materials 
			   handling - for Forming problem

Contains declarations of routines:
  pdr_forming_material_read - reads materials file and fills structures
  pdr_forming_material_query - gets material data
  pdr_forming_material_free - free materials structures

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl) (ns_supg)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
    2015    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl) (forming)
*************************************************************************/

#ifndef PDH_FORMING_MATERIALS
#define PDH_FORMING_MATERIALS

#include <stdio.h>

#include "uth_mat.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/**--------------------------------------------------------
pdr_forming_material_read - reads material file and fills pdt_materials structure
---------------------------------------------------------*/
int pdr_forming_material_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output
  //pdt_forming_materials *Materials_db /* out: materials structure to be filled */
  );
  
/**--------------------------------------------------------
pdr_forming_material_query - gets material data stored in pdt_materials
---------------------------------------------------------*/ 
int pdr_forming_material_query(
  //const pdt_forming_materials *Materials_db, /* in: materials structure */
  const utt_material_query_params *Params, /* in: query parameters */
  utt_material_query_result *Result /* out: material data */
  );
  
/**--------------------------------------------------------
pdr_forming_material_free - release memory
---------------------------------------------------------*/  
//int pdr_forming_material_free(pdt_forming_materials *Materials_db);

#ifdef __cplusplus
}
#endif

#endif
