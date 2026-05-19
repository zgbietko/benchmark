/************************************************************************
File pds_heat_materials_io.c - definition of functions related to materials  
			   read and management.

Contains definition of routines:
  pdr_heat_material_read - read materials data from config file
  pdr_heat_material_free - free materials resources

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>

#include "../include/pdh_heat.h" /* USES */
/* types and functions related to materials handling */
#include "../include/pdh_heat_materials.h" /* IMPLEMENTS */
#include "uth_mat.h"


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_heat_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
//pdr_heat_material_free - free materials resources
//------------------------------------------------------------*/
//int pdr_heat_material_free(pdt_heat_materials *Materials_db)
//{
//  return 0; //TODO
//}

/*------------------------------------------------------------
pdr_heat_material_read - read materials data from config file
------------------------------------------------------------*/
int pdr_heat_material_read(char *Work_dir,
  char *Filename,
  FILE *Interactive_output)
{

    ute_mat_read_result result = utr_mat_read(Work_dir,Filename,Interactive_output);

    switch(result) {
    case UTE_FAIL:
        break;
    case UTE_REF_TEMP_MUST_BE_GEQ_ZERO:
                if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){
                  fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent specific heat. Exiting \n");
                  exit(-1);
                  //return(EXIT_FAILURE);
                }
        break;
    case UTE_NOT_NEED_MATERIAL_DATABASE:
            pdv_heat_problem.ctrl.ref_temperature = -1.0;
            pdv_heat_problem.ctrl.thermal_conductivity =
                utr_mat_get_material_by_matID(0)->atT_thermal_conductivity[0];
            pdv_heat_problem.ctrl.density =
                utr_mat_get_material_by_matID(0)->atT_density[0];
            pdv_heat_problem.ctrl.specific_heat =
                utr_mat_get_material_by_matID(0)->atT_specific_heat[0];
        break;
    case UTE_SUCCESS:
    default:
        break;
    }

    return result;


//  config_t cfg;
//  config_setting_t *root_setting;
//  const char *str;
//  char material_file[300];
  
//  int constant_thermal_conductivity=1;
//  int constant_density=1;
//  int constant_specific_heat=1;
//  int no_other_parameters=1;
  
//  //printf("%s\n",Work_dir);
//  //printf("%s\n",Filename);
//  sprintf(material_file, "%s/%s", Work_dir, Filename);
//  //printf("%s\n",material_file);
  
//  config_init(&cfg);
  
//  /* Read the file. If there is an error, report it and exit. */
//  if(! config_read_file(&cfg, material_file))
//    {
//      //    fprintf(Interactive_output, "Material file error: %s:%d - %s\n",
//      //	    config_error_file(&cfg),
//      //            config_error_line(&cfg), config_error_text(&cfg));
//      config_destroy(&cfg);
//      return(EXIT_FAILURE);
//    }
  
  
//  root_setting = config_lookup(&cfg, "materials");
//  if(root_setting != NULL)
//    {
//      int count = config_setting_length(root_setting);
//      int i, j;
//      int setting_length;
//      const char *name;
         
     
//      Materials_db->materials_num = count;
//      Materials_db->material_names = (char**) malloc (sizeof(char*)*count);
//      Materials_db->material_data =  (pdt_heat_material_data*) malloc(sizeof(pdt_heat_material_data)*count);
      
      
//      for(i = 0; i < count; ++i)
//	{
//	  config_setting_t *material = config_setting_get_elem(root_setting, i);
//	  config_setting_t *setting;
	
//	  if(NULL != (setting = config_setting_get_member(material, "material_number"))){
//	    Materials_db->material_data[i].matnum  = (int)config_setting_get_int(setting);

//	  }
//	  else{
//	    Materials_db->material_data[i].matnum  = i;
//	  }
	  
//	  config_setting_lookup_string(material, "name", &name);
	  
//	  Materials_db->material_names[i] = (char*)malloc(sizeof(char)*(strlen(name)+1));
//	  strcpy(Materials_db->material_names[i], name);
//	  strcpy(Materials_db->material_data[i].name, name);
	  
//	  if(NULL != (setting = config_setting_get_member(material, "density"))){
	    
//	    if(config_setting_is_list(setting) == CONFIG_TRUE)
//	      {
//		setting_length = config_setting_length(setting);
//		Materials_db->material_data[i].Tfor_density =
//		  (double*) malloc(sizeof(double)*setting_length);
//		Materials_db->material_data[i].atT_density =
//		  (double*) malloc(sizeof(double)*setting_length);
//		Materials_db->material_data[i].density_num = setting_length;
//		for(j = 0; j < setting_length; ++j)
//		  {
//		    Materials_db->material_data[i].Tfor_density[j] =
//		      (double)config_setting_get_float_elem(config_setting_get_elem(setting,j), 0);
//		    Materials_db->material_data[i].atT_density[j]  =
//		      (double)config_setting_get_float_elem(config_setting_get_elem(setting,j), 1);
//		  }

//		// for temperature dependance reference temperature must be > 0
//		if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){
//		  fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent visocisty. Exiting \n");
//		  config_destroy(&cfg);
//		  exit(-1);
//		  //return(EXIT_FAILURE);
//		}

//		constant_density = 0;

//	      }
//	    else
//	      {
//		Materials_db->material_data[i].density_num = 1;
//		Materials_db->material_data[i].Tfor_density = (double*) malloc(sizeof(double));
//		Materials_db->material_data[i].atT_density = (double*) malloc(sizeof(double));
//		Materials_db->material_data[i].Tfor_density[0] = -1.0;
//		Materials_db->material_data[i].atT_density[0]  =
//		  (double)config_setting_get_float(setting);


//	      }
//	  }
	  
	  
//	  if(NULL != (setting = config_setting_get_member(material, "thermal_conductivity"))){
	    
//	    if(config_setting_is_list(setting) == CONFIG_TRUE)
//	      {
//		setting_length = config_setting_length(setting);
//		Materials_db->material_data[i].Tfor_thermal_conductivity =
//		  (double*) malloc(sizeof(double)*setting_length);
//		Materials_db->material_data[i].atT_thermal_conductivity =
//		  (double*) malloc(sizeof(double)*setting_length);
//		Materials_db->material_data[i].thermal_conductivity_num = setting_length;
//		for(j = 0; j < setting_length; ++j)
//		  {
//		    Materials_db->material_data[i].Tfor_thermal_conductivity[j] =
//		      (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0);
//		    Materials_db->material_data[i].atT_thermal_conductivity[j]  =
//		      (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1);
//		  }

//		// for temperature dependance reference temperature must be > 0
//		if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){
//		  fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent thermal_conductivity. Exiting \n");
//		  config_destroy(&cfg);
//		  exit(-1);
//		  //return(EXIT_FAILURE);
//		}

//		constant_thermal_conductivity = 0;

//	      }
//	    else
//	      {
//		Materials_db->material_data[i].thermal_conductivity_num = 1;
//		Materials_db->material_data[i].Tfor_thermal_conductivity =
//		  (double*) malloc(sizeof(double));
//		Materials_db->material_data[i].atT_thermal_conductivity =
//		  (double*) malloc(sizeof(double));
//		Materials_db->material_data[i].Tfor_thermal_conductivity[0] = -1.0;
//		Materials_db->material_data[i].atT_thermal_conductivity[0] =
//		  (double)config_setting_get_float(setting);
//	      }
//	  }


//	  if(NULL != (setting = config_setting_get_member(material, "specific_heat"))){
	    
//	    if(config_setting_is_list(setting) == CONFIG_TRUE)
//	      {
//		setting_length = config_setting_length(setting);
//		Materials_db->material_data[i].Tfor_specific_heat =
//		  (double*) malloc(sizeof(double)*setting_length);
//		Materials_db->material_data[i].atT_specific_heat =
//		  (double*) malloc(sizeof(double)*setting_length);
//		Materials_db->material_data[i].specific_heat_num = setting_length;
//		for(j = 0; j < setting_length; ++j)
//		  {
//		    Materials_db->material_data[i].Tfor_specific_heat[j] =
//		      (double)config_setting_get_float_elem(config_setting_get_elem(setting,j), 0);
//		    Materials_db->material_data[i].atT_specific_heat[j]  =
//		      (double)config_setting_get_float_elem(config_setting_get_elem(setting,j), 1);
//		  }

//		// for temperature dependance reference temperature must be > 0
//		if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){
//		  fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent specific heat. Exiting \n");
//		  config_destroy(&cfg);
//		  exit(-1);
//		  //return(EXIT_FAILURE);
//		}

//		constant_specific_heat = 0;

//	      }
//	    else
//	      {
//		Materials_db->material_data[i].specific_heat_num = 1;
//		Materials_db->material_data[i].Tfor_specific_heat =
//		  (double*) malloc(sizeof(double));
//		Materials_db->material_data[i].atT_specific_heat =
//		  (double*) malloc(sizeof(double));
//		Materials_db->material_data[i].Tfor_specific_heat[0] = -1.0;
//		Materials_db->material_data[i].atT_specific_heat[0]  =
//		  (double)config_setting_get_float(setting);
//	      }
//	  }
	  
//	  // SPECIAL PARAMETERS FOR SPECIAL PROBLEMS - should not be required for all cases !!!!!
	  
//	  //no_other_parameters=1; // indicate material database must be used due to additional
	  
//	  /* 	if(NULL != (setting = config_setting_get_member(material, "enthalpy"))){ */
     
//    /*   if(config_setting_is_list(setting) == CONFIG_TRUE) */
//    /*   { */
//    /*     setting_length = config_setting_length(setting); */
//    /*     Materials_db->material_data[i].Tfor_enthalpy = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].atT_enthalpy = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].enthalpy_num = setting_length; */
//    /*     for(j = 0; j < setting_length; ++j) */
//    /*     { */
//    /*       Materials_db->material_data[i].Tfor_enthalpy[j] = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0); */
//    /*       Materials_db->material_data[i].atT_enthalpy[j]  = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1); */
//    /*     } */
//    /*   } */
//    /*   else */
//    /*   { */
//    /*     Materials_db->material_data[i].enthalpy_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_enthalpy = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_enthalpy = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_enthalpy[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_enthalpy[0]  = (double)config_setting_get_float(setting);         */
//    /*   }   */
//    /* } */
//    /* 	else{ */
//    /*     Materials_db->material_data[i].enthalpy_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_enthalpy = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_enthalpy = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_enthalpy[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_enthalpy[0]  = 0.0; 	 */
//    /* 	} */
//    /* 	if(NULL != (setting = config_setting_get_member(material, "thermal_expansion_coefficient"))){ */
     
//    /*   if(config_setting_is_list(setting) == CONFIG_TRUE) */
//    /*   { */
//    /*     setting_length = config_setting_length(setting); */
//    /*     Materials_db->material_data[i].Tfor_thermal_expansion_coefficient = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].atT_thermal_expansion_coefficient = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].thermal_expansion_coefficient_num = setting_length; */
//    /*     for(j = 0; j < setting_length; ++j) */
//    /*     { */
//    /*       Materials_db->material_data[i].Tfor_thermal_expansion_coefficient[j] = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0); */
//    /*       Materials_db->material_data[i].atT_thermal_expansion_coefficient[j]  = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1); */
//    /*     } */
//    /*   } */
//    /*   else */
//    /*   { */
//    /*     Materials_db->material_data[i].thermal_expansion_coefficient_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_thermal_expansion_coefficient = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_thermal_expansion_coefficient = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_thermal_expansion_coefficient[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_thermal_expansion_coefficient[0]  = (double)config_setting_get_float(setting);         */
//    /*   }   */
//    /* } */
//    /* 	else{ */
//    /*     Materials_db->material_data[i].thermal_expansion_coefficient_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_thermal_expansion_coefficient = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_thermal_expansion_coefficient = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_thermal_expansion_coefficient[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_thermal_expansion_coefficient[0]  = 0.0;	 */
	
//    /* 	} */
//    /* 	if(NULL != (setting = config_setting_get_member(material, "electrical_resistivity"))){ */
   
//    /*   if(config_setting_is_list(setting) == CONFIG_TRUE) */
//    /*   { */
//    /*     setting_length = config_setting_length(setting); */
//    /*     Materials_db->material_data[i].Tfor_electrical_resistivity = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].atT_electrical_resistivity = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].electrical_resistivity_num = setting_length; */
//    /*     for(j = 0; j < setting_length; ++j) */
//    /*     { */
//    /*       Materials_db->material_data[i].Tfor_electrical_resistivity[j] = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0); */
//    /*       Materials_db->material_data[i].atT_electrical_resistivity[j]  = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1); */
//    /*     } */
//    /*   } */
//    /*   else */
//    /*   { */
//    /*     Materials_db->material_data[i].electrical_resistivity_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_electrical_resistivity = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_electrical_resistivity = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_electrical_resistivity[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_electrical_resistivity[0]  = (double)config_setting_get_float(setting);         */
//    /*   }   */
//    /* } */
//    /* 	else{ */
//    /*     Materials_db->material_data[i].electrical_resistivity_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_electrical_resistivity = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_electrical_resistivity = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_electrical_resistivity[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_electrical_resistivity[0]  = 0.0;	 */
//    /* 	} */
//    /* 	if(NULL != (setting = config_setting_get_member(material, "VOF"))){ */
 
//    /*   if(config_setting_is_list(setting) == CONFIG_TRUE) */
//    /*   { */
//    /*     setting_length = config_setting_length(setting); */
//    /*     Materials_db->material_data[i].Tfor_VOF = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].atT_VOF = (double*) malloc(sizeof(double)*setting_length); */
//    /*     Materials_db->material_data[i].VOF_num = setting_length; */
//    /*     for(j = 0; j < setting_length; ++j) */
//    /*     { */
//    /*       Materials_db->material_data[i].Tfor_VOF[j] = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0); */
//    /*       Materials_db->material_data[i].atT_VOF[j]  = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1); */
//    /*     } */
//    /*   } */
//    /*   else */
//    /*   { */
//    /*     Materials_db->material_data[i].VOF_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_VOF = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_VOF = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_VOF[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_VOF[0]  = (double)config_setting_get_float(setting); */
//    /*   } */
//    /* } */
//    /* 	else{ */
//    /*     Materials_db->material_data[i].VOF_num = 1; */
//    /*     Materials_db->material_data[i].Tfor_VOF = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].atT_VOF = (double*) malloc(sizeof(double)); */
//    /*     Materials_db->material_data[i].Tfor_VOF[0] = -1.0; */
//    /*     Materials_db->material_data[i].atT_VOF[0]  = 0.0;	 */
//    /* 	} */
//      /* setting = config_setting_get_member(material, "dg_dT"); */
//      /* if(config_setting_is_list(setting) == CONFIG_TRUE) */
//      /* { */
//      /*   printf("\n\tAS: material file = %s", material_file); */
//      /*   setting_length = config_setting_length(setting); */
//      /*   printf("\n\tAS: setting_length dg_dT = %d", setting_length); */
//      /*   Materials_db->material_data[i].Tfor_dg_dT = (double*) malloc(sizeof(double)*setting_length); */
//      /*   Materials_db->material_data[i].atT_dg_dT = (double*) malloc(sizeof(double)*setting_length); */
//      /*   Materials_db->material_data[i].dg_dT_num = setting_length; */
//      /*   for(j = 0; j < setting_length; ++j) */
//      /*   { */
//      /*     Materials_db->material_data[i].Tfor_dg_dT[j] = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0); */
//      /*     Materials_db->material_data[i].atT_dg_dT[j]  = (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1); */
//      /*   } */
//      /* } */
//      /* else */
//      /* { */
//      /*   Materials_db->material_data[i].dg_dT_num = 1; */
//      /*   printf("\n\tAS: setting_length dg_dT = %d", Materials_db->material_data[i].dg_dT_num); */
//      /*   Materials_db->material_data[i].Tfor_dg_dT = (double*) malloc(sizeof(double)); */
//      /*   Materials_db->material_data[i].atT_dg_dT = (double*) malloc(sizeof(double)); */
//      /*   Materials_db->material_data[i].Tfor_dg_dT[0] = -1.0; */
//      /*   Materials_db->material_data[i].atT_dg_dT[0]  = (double)config_setting_get_float(setting); */
//      /* } */

//    /* 	if(NULL != (setting = config_setting_get_member(material, "temp_solidus"))){ */
//    /* 	  Materials_db->material_data[i].temp_solidus  = (double)config_setting_get_float(setting); */
//    /* 	} */
//    /* 	else{ */
//    /* 	  Materials_db->material_data[i].temp_solidus  = 0.0; */
//    /* 	} */
	
//    /* if(NULL != (setting = config_setting_get_member(material, "temp_liquidus"))){     */

//    /*   Materials_db->material_data[i].temp_liquidus  = (double)config_setting_get_float(setting); */
//    /* 	} */
//    /* 	else{Materials_db->material_data[i].temp_liquidus  = 0.0;} */
//    /* if(NULL != (setting = config_setting_get_member(material, "temp_vaporization"))){  */

//    /*   Materials_db->material_data[i].temp_vaporization  = (double)config_setting_get_float(setting); */
//    /* 	} */
//    /* 	else{Materials_db->material_data[i].temp_vaporization  = 0.0;} */
//    /* 	if(NULL != (setting = config_setting_get_member(material, "latent_heat_of_fusion"))){ */

//    /*   Materials_db->material_data[i].latent_heat_of_fusion  = (double)config_setting_get_float(setting); */
//    /* 	} */
//    /* 	else{Materials_db->material_data[i].latent_heat_of_fusion  = 0.0;} */
//    /* 	if(NULL != (setting = config_setting_get_member(material, "latent_heat_of_vaporization"))){ */

//    /*   Materials_db->material_data[i].latent_heat_of_vaporization  = (double)config_setting_get_float(setting); */
//    /* 	} */
//    /* 	else{Materials_db->material_data[i].latent_heat_of_vaporization  = 0.0;} */
          
//    }

//      if(constant_thermal_conductivity==1 && constant_density==1
//              && constant_specific_heat==1 && no_other_parameters==1 && count==1){
	
//	// we do not need material database
//	pdv_heat_problem.ctrl.ref_temperature = -1.0;
//	pdv_heat_problem.ctrl.thermal_conductivity =
//	  Materials_db->material_data[0].atT_thermal_conductivity[0];
//	pdv_heat_problem.ctrl.density =
//	  Materials_db->material_data[0].atT_density[0];
//	pdv_heat_problem.ctrl.specific_heat =
//	  Materials_db->material_data[0].atT_specific_heat[0];
	
//      }
      
//  }

//  config_destroy(&cfg);
//  return(EXIT_SUCCESS);
}
