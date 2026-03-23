/************************************************************************
File pdh_ns_supg_materials.h - types and functions related to materials 
			   handling - for HMT problem

Contains declarations of routines:
  pdr_ns_supg_material_read - reads materials file and fills structures
  pdr_ns_supg_material_query - gets material data
  pdr_ns_supg_material_free - free materials structures

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)

*************************************************************************/

#ifndef PDH_NS_SUPG_MATERIALS
#define PDH_NS_SUPG_MATERIALS

#include <stdio.h>

#include "uth_mat.h"

#ifdef __cplusplus
extern "C" 
{
#endif


///**************************************/
///* TYPES                              */
///**************************************/
///* Rules:
///* - type name starts always with pdt_ */

///* internal structure filled from materials file - not for direct access!*/
//typedef struct {
//	int matnum;
//    char name[20];
  
//    double *Tfor_dynamic_viscosity;
//    double *atT_dynamic_viscosity;
//    int dynamic_viscosity_num;
    
//    double *Tfor_density;
//    double *atT_density;
//    int density_num;
    
//  //double *Tfor_thermal_conductivity;
//  //double *atT_thermal_conductivity;
//  //int thermal_conductivity_num;
    
//  //double *Tfor_specific_heat;
//  //double *atT_specific_heat;
//  //int specific_heat_num;
    
//  //double *Tfor_enthalpy;
//  //double *atT_enthalpy;
//  //int enthalpy_num;
    
//  //double *Tfor_thermal_expansion_coefficient;
//  //double *atT_thermal_expansion_coefficient;
//  //int thermal_expansion_coefficient_num;
    
//  //double *Tfor_electrical_resistivity;
//  //double *atT_electrical_resistivity;
//  //int electrical_resistivity_num;

//    double *Tfor_VOF;
//    double *atT_VOF;
//    int VOF_num;

//    double *Tfor_dg_dT;
//    double *atT_dg_dT;
//    int dg_dT_num;

//    double temp_solidus;
//    double temp_liquidus;
//    double temp_vaporization;

//    double latent_heat_of_fusion;
//    double latent_heat_of_vaporization;

//} pdt_ns_supg_material_data;

///* main structure containing materials data */
//typedef struct{
//  int materials_num;
//  char **material_names;
//  pdt_ns_supg_material_data *material_data;
//  //TODO: cache
//} pdt_ns_supg_materials;

//typedef enum {
//	QUERY_NS_SUPG_NODE,
//	QUERY_NS_SUPG_POINT
//} pdt_ns_supg_query_type;

///* input for pdr_material_query */
//typedef struct{
//  char * name; //if searching by idx (idx >= 0) set to anything
//  int material_idx; //set -1 if you want to search by name
//  double temperature;
//  double xg[3];
//  pdt_ns_supg_query_type query_type;
//  int cell_id;
//  int node_id;
//	double aux;
//  //in future: elem no., coordinates etc.
//} pdt_ns_supg_material_query_params;

///* output of pdr_material_query */
//typedef struct{
//    char name[20];
//    double dynamic_viscosity;
//    double density;
//    //double thermal_conductivity;
//    //double specific_heat;
//    //double enthalpy;
//    //double thermal_expansion_coefficient;
//    //double electrical_resistivity;
//    double VOF;
//    double dg_dT;
//    double temp_solidus;
//    double temp_liquidus;
//    double temp_vaporization;
//    double latent_heat_of_fusion;
//    double latent_heat_of_vaporization;
//} pdt_ns_supg_material_query_result;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
pdr_ns_supg_material_read - reads material file and fills pdt_materials structure
---------------------------------------------------------*/
int pdr_ns_supg_material_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output
  //pdt_ns_supg_materials *Materials_db /* out: materials structure to be filled */
  );
  
/**--------------------------------------------------------
pdr_ns_supg_material_query - gets material data stored in pdt_materials
---------------------------------------------------------*/ 
int pdr_ns_supg_material_query(
  //const pdt_ns_supg_materials *Materials_db, /* in: materials structure */
  const utt_material_query_params *Params, /* in: query parameters */
  utt_material_query_result *Result /* out: material data */
  );
  
/**--------------------------------------------------------
pdr_ns_supg_material_free - release memory
---------------------------------------------------------*/  
//int pdr_ns_supg_material_free(pdt_ns_supg_materials *Materials_db);

#ifdef __cplusplus
}
#endif

#endif
