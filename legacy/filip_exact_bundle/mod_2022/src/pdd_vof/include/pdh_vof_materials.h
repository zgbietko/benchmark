/************************************************************************
File pdh_vof_materials.h - types and functions related to materials 
			   handling

Contains declarations of routines:
  pdr_vof_material_read - reads materials file and fills structures
  pdr_vof_material_query - gets material data
  pdr_vof_material_free - free materials structures

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#ifndef PDH_VOF_MATERIALS_H
#define PDH_VOF_MATERIALS_H

#include <stdio.h>


/**************************************/
/* TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always with pdt_ */

/* internal structure filled from materials file - not for direct access!*/
typedef struct {
  char name[20];
  
  /* double *Tfor_dynamic_viscosity;     */
  /* double *atT_dynamic_viscosity; */
  /* int dynamic_viscosity_num; */
  
  /* double *Tfor_density; */
  /* double *atT_density; */
  /* int density_num; */
  
  /* double *Tfor_thermal_conductivity; */
  /* double *atT_thermal_conductivity; */
  /* int thermal_conductivity_num; */
  
  /* double *Tfor_specific_heat; */
  /* double *atT_specific_heat; */
  /* int specific_heat_num; */
  
  /* double *Tfor_enthalpy; */
  /* double *atT_enthalpy; */
  /* int enthalpy_num; */
  
  /* double *Tfor_thermal_expansion_coefficient; */
  /* double *atT_thermal_expansion_coefficient; */
  /* int thermal_expansion_coefficient_num; */
  
  /* double *Tfor_electrical_resistivity; */
  /* double *atT_electrical_resistivity; */
  /* int electrical_resistivity_num; */

  double *Tfor_VOF;
  double *atT_VOF;
  int VOF_num;
  
  /* double *Tfor_dg_dT; */
  /* double *atT_dg_dT; */
  /* int dg_dT_num; */
  
  double temp_solidus;
  double temp_liquidus;
  double temp_vaporization;
  
  /* double latent_heat_of_fusion; */
  /* double latent_heat_of_vaporization; */
  
  double *composition;
  int composition_num;

} pdt_vof_material_data;

/* main structure containing materials data */
typedef struct{
  int materials_num;
  char **material_names;
  pdt_vof_material_data *material_data;
  //TODO: cache
} pdt_vof_materials;

typedef enum {
	QUERY_VOF_NODE,
	QUERY_VOF_POINT
} pdt_vof_query_type;

/* input for pdr_material_query */
typedef struct{
  char * name; //if searching by idx (idx >= 0) set to anything
  int material_idx; //set -1 if you want to search by name
  double temperature;
  double *composition;	//initial content of the element in each material
  double VOF_fd;
  double xg[3];
  pdt_vof_query_type query_type;
  int node_id;
  int cell_id;
  //in future: elem no., coordinates etc.
} pdt_vof_material_query_params;

/* output of pdr_material_query */
typedef struct{
    char name[20];
    /* //double dynamic_viscosity; */
    /* double thermal_conductivity; */
    /* double specific_heat; */
    /* double density; */
    /* double enthalpy; */
    /* double thermal_expansion_coefficient; */
    /* double electrical_resistivity; */
//     double VOF_md;
    double av_comp;	//averaged content of the element after mixing
    double dg_dT;
    double gamma;
    double temp_solidus;
    double temp_liquidus;
    double temp_vaporization;
    /* double latent_heat_of_fusion; */
    /* double latent_heat_of_vaporization; */
} pdt_vof_material_query_result;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
pdr_vof_material_read - reads material file and fills pdt_materials structure
---------------------------------------------------------*/
int pdr_vof_material_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_vof_materials *Materials_db /* out: materials structure to be filled */
  );
  
/**--------------------------------------------------------
pdr_vof_material_query - gets material data stored in pdt_materials
---------------------------------------------------------*/ 
int pdr_vof_material_query(
  const pdt_vof_materials *Materials_db, /*in:materials structure to read from*/
  const pdt_vof_material_query_params *Params, /* in: query parameters */
  pdt_vof_material_query_result *Result /* out: material data */
  );
  
/**--------------------------------------------------------
pdr_vof_material_free - release memory
---------------------------------------------------------*/  
int pdr_vof_material_free(pdt_vof_materials *Materials_db);

#endif
