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
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* types and functions related to materials handling */
#include "../../pdd_ns_supg/include/pdh_ns_supg_materials.h"	/* IMPLEMENTS */
/* problem dependent module interface */
#include "../../pdd_ns_supg_heat_vof/include/pdh_ns_supg_heat_vof.h"	/* USES */
#include "uth_log.h"
/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_ns_supg_material_query - gets material data
------------------------------------------------------------*/
int pdr_ns_supg_material_query(
  const utt_material_query_params * Params,
  utt_material_query_result * Result)
{
  int i, j;
  pdt_ns_supg_material_data *material;
  double v1, v2, t1, t2;
  double a, b;
  int problem_id, field_id, iaux;
  double u_val[PDC_NS_SUPG_MAXEQ];
  double *VOF = (double *) malloc(pdv_vof_problem.ctrl.materials_used[0] * sizeof(double));
  double *density = (double *) malloc(pdv_vof_problem.ctrl.materials_used[0] * sizeof(double));
  double Xloc[3], Xglob[3];
  int list_el[20];
  int numdofs;


  problem_id = PDC_VOF_ID;
  i=3; field_id = pdr_ctrl_i_params(problem_id, i);
	switch( Params->query_type ) {
        case QUERY_POINT:
			list_el[0] = Params->cell_id;
			for( i=0; i<3; ++i) {
				Xglob[i] = Params->xg[i];
			}
			iaux = apr_sol_xglob(field_id, Xglob, 1, list_el, Xloc, u_val, NULL, NULL, NULL, 1);
		  break;
        case QUERY_NODE:
// 		  numdofs = apr_get_ent_nrdofs(field_id, APC_VERTEX, Params->node_id);
// 		  apr_read_ent_dofs(field_id, APC_VERTEX, Params->node_id, numdofs, 1, u_val);
			u_val[0] = Params->aux;
		  break;
		default:
			printf("\nWrong VOF query type");
			exit(1);
	}

  //AS: VOF=1 for the point occupied with first material on the list of
  //materials (pdv_vof_problem.ctrl.materials_used), and vice versa
  VOF[1] = u_val[0];
  VOF[0] = 1.0 - u_val[0];
  
  //check( (VOF[0] > 0.0) || (VOF[1] > 0.0) );

/*if ( (u_val[0] > 0.1) && (u_val[1] < 0.9) )
  {
    printf("\n\tVOF = %f", u_val[0]);
    printf("\n\tXglob = (%f,%f,%f)\n\tXloc = (%f,%f,%f)", Params->xg[0], Params->xg[1], Params->xg[2], Xloc[0], Xloc[1], Xloc[2]);
  }
*/
//  for( i=0; i<pdv.vof_problem.ctrl.materials_used[0]; i++) {
//    material_idx[i] = pdv.vof_problem.ctrl.materials_used[i];
//  }


  //density
  // Wang_Tsai_IntJHeatMassTrans_2001
  // Francois_in_JCompPhys_2006
  // property weighted by volume fraction
  Result->density = 0.0;
  for( j=0; j < pdv_vof_problem.ctrl.materials_used[0]; ++j ) {
    material = &Materials_db->material_data[pdv_vof_problem.ctrl.materials_used[j+1]];
    if (material->density_num == 1) {
      density[j] = VOF[j] * material->atT_density[0];
      Result->density += VOF[j] * material->atT_density[0];
    } else {
      if (Params->temperature <= material->Tfor_density[0]) {
	density[j] = VOF[j] * material->atT_density[0];
	Result->density += VOF[j] * material->atT_density[0];
      } else if (Params->temperature >= material->Tfor_density[material->density_num - 1]) {
	density[j] = VOF[j] * material->atT_density[0];
	Result->density += VOF[j] * material->atT_density[material->density_num - 1];
      } else
	for (i = 0; i < material->density_num - 1; ++i) {
	  if (Params->temperature <= material->Tfor_density[i + 1]) {
	    t1 = material->Tfor_density[i];
	    t2 = material->Tfor_density[i + 1];
	    v1 = material->atT_density[i];
	    v2 = material->atT_density[i + 1];
	    a = (v1 - v2) / (t1 - t2);
	    b = v1 - a * t1;
	    density[j] = VOF[j] * (a * (Params->temperature) + b);
	    Result->density += VOF[j] * (a * (Params->temperature) + b);
	    break;
	  }
	}
    }
  }

  //viscosity
  // Francois_in_JCompPhys_2006
  // property weighted by volume fraction
  Result->dynamic_viscosity = 0.0;
  for( j=0; j < pdv_vof_problem.ctrl.materials_used[0]; ++j ) {
    material = &Materials_db->material_data[pdv_vof_problem.ctrl.materials_used[j+1]];
    if (material->dynamic_viscosity_num == 1)
      Result->dynamic_viscosity += VOF[j] * material->atT_dynamic_viscosity[0];
    else {
      if (Params->temperature <= material->Tfor_dynamic_viscosity[0])
	Result->dynamic_viscosity += VOF[j] * material->atT_dynamic_viscosity[0];
      else if (Params->temperature >= 
	      material->Tfor_dynamic_viscosity[material->dynamic_viscosity_num-1])
	Result->dynamic_viscosity += VOF[j] * 
	  material->atT_dynamic_viscosity[material->dynamic_viscosity_num - 1];
      else
	for (i = 0; i < material->dynamic_viscosity_num - 1; ++i) {
	  if (Params->temperature <= material->Tfor_dynamic_viscosity[i + 1]) {
	    t1 = material->Tfor_dynamic_viscosity[i];
	    t2 = material->Tfor_dynamic_viscosity[i + 1];
	    v1 = material->atT_dynamic_viscosity[i];
	    v2 = material->atT_dynamic_viscosity[i + 1];
	    a = (v1 - v2) / (t1 - t2);
	    b = v1 - a * t1;
	    Result->dynamic_viscosity += VOF[j] * (a * (Params->temperature) + b);
	    break;
	  }
	}
    }
  }

  //thermal_conductivity
  /* if (material->thermal_conductivity_num == 1) */
  /*   Result->thermal_conductivity = material->atT_thermal_conductivity[0]; */
  /* else { */
  /*   if (Params->temperature <= material->Tfor_thermal_conductivity[0]) */
  /*     Result->thermal_conductivity = material->atT_thermal_conductivity[0]; */
  /*   else if (Params->temperature >=  */
  /* 	     material->Tfor_thermal_conductivity[material->thermal_conductivity_num - 1]) */
  /*     Result->thermal_conductivity =  */
  /* 	material->atT_thermal_conductivity[material->thermal_conductivity_num-1]; */
  /*   else */
  /*     for (i = 0; i < material->thermal_conductivity_num - 1; ++i) { */
  /* 	if (Params->temperature <= material->Tfor_thermal_conductivity[i + 1]) { */
  /* 	  t1 = material->Tfor_thermal_conductivity[i]; */
  /* 	  t2 = material->Tfor_thermal_conductivity[i + 1]; */
  /* 	  v1 = material->atT_thermal_conductivity[i]; */
  /* 	  v2 = material->atT_thermal_conductivity[i + 1]; */
  /* 	  a = (v1 - v2) / (t1 - t2); */
  /* 	  b = v1 - a * t1; */
  /* 	  Result->thermal_conductivity = a * (Params->temperature) + b; */
  /* 	  break; */
  /* 	} */
  /*     } */
  /* } */


  //specific_heat
  /* if (material->specific_heat_num == 1) */
  /*   Result->specific_heat = material->atT_specific_heat[0]; */
  /* else { */
  /*   if (Params->temperature <= material->Tfor_specific_heat[0]) */
  /*     Result->specific_heat = material->atT_specific_heat[0]; */
  /*   else if (Params->temperature >=  */
  /* 	     material->Tfor_specific_heat[material->specific_heat_num - 1]) */
  /*     Result->specific_heat =  */
  /* 	material->atT_specific_heat[material->specific_heat_num - 1]; */
  /*   else */
  /*     for (i = 0; i < material->specific_heat_num - 1; ++i) { */
  /* 	if (Params->temperature <= material->Tfor_specific_heat[i + 1]) { */
  /* 	  t1 = material->Tfor_specific_heat[i]; */
  /* 	  t2 = material->Tfor_specific_heat[i + 1]; */
  /* 	  v1 = material->atT_specific_heat[i]; */
  /* 	  v2 = material->atT_specific_heat[i + 1]; */
  /* 	  a = (v1 - v2) / (t1 - t2); */
  /* 	  b = v1 - a * t1; */
  /* 	  Result->specific_heat = a * (Params->temperature) + b; */
  /* 	  break; */
  /* 	} */
  /*     } */
  /* } */

  //thermal_expansion_coefficient
  /* if (material->thermal_expansion_coefficient_num == 1) */
  /*   Result->thermal_expansion_coefficient =  */
  /*     material->atT_thermal_expansion_coefficient[0]; */
  /* else { */
  /*   if (Params->temperature <= material->Tfor_thermal_expansion_coefficient[0]) */
  /*     Result->thermal_expansion_coefficient =  */
  /* 	material->atT_thermal_expansion_coefficient[0]; */
  /*   else if (Params->temperature >=  */
  /* 	     material->Tfor_thermal_expansion_coefficient[material->thermal_expansion_coefficient_num - 1]) */
  /*     Result->thermal_expansion_coefficient =  */
  /* 	material->atT_thermal_expansion_coefficient[material->thermal_expansion_coefficient_num - 1]; */
  /*   else */
  /*     for (i = 0; i < material->thermal_expansion_coefficient_num - 1; ++i) { */
  /* 	if (Params->temperature <=  */
  /* 	    material->Tfor_thermal_expansion_coefficient[i + 1]) { */
  /* 	  t1 = material->Tfor_thermal_expansion_coefficient[i]; */
  /* 	  t2 = material->Tfor_thermal_expansion_coefficient[i + 1]; */
  /* 	  v1 = material->atT_thermal_expansion_coefficient[i]; */
  /* 	  v2 = material->atT_thermal_expansion_coefficient[i + 1]; */
  /* 	  a = (v1 - v2) / (t1 - t2); */
  /* 	  b = v1 - a * t1; */
  /* 	  Result->thermal_expansion_coefficient = a * (Params->temperature) + b; */
  /* 	  break; */
  /* 	} */
  /*     } */
  /* } */

  //electrical_resistivity
  /* if (material->electrical_resistivity_num == 1) */
  /*   Result->electrical_resistivity = material->atT_electrical_resistivity[0]; */
  /* else { */
  /*   if (Params->temperature <= material->Tfor_electrical_resistivity[0]) */
  /*     Result->electrical_resistivity = material->atT_electrical_resistivity[0]; */
  /*   else if (Params->temperature >=  */
  /* 	     material->Tfor_electrical_resistivity[material->electrical_resistivity_num - 1]) */
  /*     Result->electrical_resistivity =  */
  /* 	material->atT_electrical_resistivity[material->electrical_resistivity_num - 1]; */
  /*   else */
  /*     for (i = 0; i < material->electrical_resistivity_num - 1; ++i) { */
  /* 	if (Params->temperature <=  */
  /* 	    material->Tfor_electrical_resistivity[i + 1]) { */
  /* 	  t1 = material->Tfor_electrical_resistivity[i]; */
  /* 	  t2 = material->Tfor_electrical_resistivity[i + 1]; */
  /* 	  v1 = material->atT_electrical_resistivity[i]; */
  /* 	  v2 = material->atT_electrical_resistivity[i + 1]; */
  /* 	  a = (v1 - v2) / (t1 - t2); */
  /* 	  b = v1 - a * t1; */
  /* 	  Result->electrical_resistivity = a * (Params->temperature) + b; */
  /* 	  break; */
  /* 	} */
  /*     } */
  /* } */

  //enthalpy
  /* if (material->enthalpy_num == 1) */
  /*   Result->enthalpy = material->atT_enthalpy[0]; */
  /* else { */
  /*   if (Params->temperature <= material->Tfor_enthalpy[0]) */
  /*     Result->enthalpy = material->atT_enthalpy[0]; */
  /*   else if (Params->temperature >=  */
  /* 	     material->Tfor_enthalpy[material->enthalpy_num - 1]) */
  /*     Result->enthalpy = material->atT_enthalpy[material->enthalpy_num - 1]; */
  /*   else */
  /*     for (i = 0; i < material->enthalpy_num - 1; ++i) { */
  /* 	if (Params->temperature <= material->Tfor_enthalpy[i + 1]) { */
  /* 	  t1 = material->Tfor_enthalpy[i]; */
  /* 	  t2 = material->Tfor_enthalpy[i + 1]; */
  /* 	  v1 = material->atT_enthalpy[i]; */
  /* 	  v2 = material->atT_enthalpy[i + 1]; */
  /* 	  a = (v1 - v2) / (t1 - t2); */
  /* 	  b = v1 - a * t1; */
  /* 	  Result->enthalpy = a * (Params->temperature) + b; */
  /* 	  break; */
  /* 	} */
  /*     } */
  /* } */

  // VOF material data
  // property weighted by volume fraction
  Result->VOF = 0.0;
  for( j=0; j < pdv_vof_problem.ctrl.materials_used[0]; ++j ) {
    material = &Materials_db->material_data[pdv_vof_problem.ctrl.materials_used[j+1]];
    if (material->VOF_num == 1)
      Result->VOF += VOF[j] * material->atT_VOF[0];
    else {
      if (Params->temperature <= material->Tfor_VOF[0])
	Result->VOF += VOF[j] * material->atT_VOF[0];
      else if (Params->temperature >= material->Tfor_VOF[material->VOF_num - 1])
	Result->VOF += VOF[j] * material->atT_VOF[material->VOF_num - 1];
      else
	for (i = 0; i < material->VOF_num - 1; ++i) {
	  if (Params->temperature <= material->Tfor_VOF[i + 1]) {
	    t1 = material->Tfor_VOF[i];
	    t2 = material->Tfor_VOF[i + 1];
	    v1 = material->atT_VOF[i];
	    v2 = material->atT_VOF[i + 1];
	    a = (v1 - v2) / (t1 - t2);
	    b = v1 - a * t1;
	    Result->VOF += VOF[j] * (a * (Params->temperature) + b);
	    break;
	  }
	}
    }
  }

  //dg_dT
  // property weighted by volume fraction
  Result->dg_dT = 0.0;
  for( j=0; j < pdv_vof_problem.ctrl.materials_used[0]; ++j ) {
    material = &Materials_db->material_data[pdv_vof_problem.ctrl.materials_used[j+1]];
    if (material->dg_dT_num == 1)
      Result->dg_dT += VOF[j] * material->atT_dg_dT[0];
    else {
      if (Params->temperature <= material->Tfor_dg_dT[0])
	Result->dg_dT += VOF[j] * material->atT_dg_dT[0];
      else if (Params->temperature >= material->Tfor_dg_dT[material->dg_dT_num - 1])
	Result->dg_dT += VOF[j] * material->atT_dg_dT[material->dg_dT_num - 1];
      else
	for (i=0; i < material->dg_dT_num - 1; ++i) {
	  if (Params->temperature <= material->Tfor_dg_dT[i + 1]) {
	    t1 = material->Tfor_dg_dT[i];
	    t2 = material->Tfor_dg_dT[i + 1];
	    v1 = material->atT_dg_dT[i];
	    v2 = material->atT_dg_dT[i + 1];
	    a = (v1 - v2) / (t1 - t2);
	    b = v1 - a * t1;
	    Result->dg_dT += VOF[j] * (a * (Params->temperature) + b);
	    break;
	  }
	}
    }
  }

  //Result->temp_solidus = material->temp_solidus;
  //Result->temp_liquidus = material->temp_liquidus;
  //Result->temp_vaporization = material->temp_vaporization;
  //Result->latent_heat_of_fusion = material->latent_heat_of_fusion;
  //Result->latent_heat_of_vaporization = material->latent_heat_of_vaporization;

  mf_check(Result->density > 0,"Density below 0!");
  mf_check(Result->dynamic_viscosity > 0, "Dynamic viscosity below 0!");
  
  return 0;
}
