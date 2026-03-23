#ifndef UTS_MAT_CPP
#define UTS_MAT_CPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include "uth_log.h"
#include "uth_mat.h"
#include "fastmathparser/exprtk.hpp"

#ifdef __cplusplus
extern "C"
{
#endif


const char* utc_mat_database_filename = "materials_database.dat";


const int utc_mat_database_max_reserved_id = 100;

// LOCAL STRUCUTES
/* main structure containing materials data */
typedef struct{
  std::vector<int> material_ids;
  std::map<int, utt_material_data> material_data;
} utt_materials;



// LOCAL VARIABLES
utt_materials utv_materials;
std::set<std::string> utv_read_material_files;

//
std::vector<double>    utv_temp2block;
std::vector<int>    utv_group2material;
std::vector<int>    utv_group2block;



void utr_mat_init_material(utt_material_data* mat)
{
    if(mat != NULL) {
        memset(mat,0,sizeof(utt_material_data));
    }
}

void utr_mat_clear_material(utt_material_data* mat)
{
    if(mat != NULL) {
        free(mat->Tfor_density);
        free(mat->atT_density);

        free(mat->Tfor_thermal_conductivity);
        free(mat->atT_thermal_conductivity);

        free(mat->Tfor_specific_heat);
        free(mat->atT_specific_heat);

        free(mat->Tfor_enthalpy);
        free(mat->atT_enthalpy);

        free(mat->Tfor_thermal_expansion_coefficient);
        free(mat->atT_thermal_expansion_coefficient);


        free(mat->Tfor_electrical_resistivity);
        free(mat->atT_electrical_resistivity);

        free(mat->Tfor_VOF);
        free(mat->atT_VOF);

        free(mat->Tfor_dg_dT);
        free(mat->atT_dg_dT);
    }
}

ute_mat_read_result utr_mat_read_assigments( const char *Work_dir,
                   const char *Filename,
                   FILE *Interactive_output)
{
    config_t cfg;
    config_setting_t *root_setting;
    const char *str;
    char mapping_file[300];


    ute_mat_read_result ret_val = UTE_SUCCESS;

    //printf("%s\n",Work_dir);
    //printf("%s\n",Filename);
    sprintf(mapping_file, "%s/%s", Work_dir, Filename);
    //printf("%s\n",material_file);

    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, mapping_file)) {
        //fprintf(Interactive_output, "Material file error: %s:%d - %s\n", config_error_file(&cfg),
        //        config_error_line(&cfg), config_error_text(&cfg));
        //mf_log_err("Could not open file! (%s) ", mapping_file);
        mf_log_err("Material file error: %s:%d - %s\n", config_error_file(&cfg),
                        config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return(UTE_FAIL);
      }

    root_setting = config_lookup(&cfg, "group_to_materials_assignment");
    if(root_setting != NULL)
    {
        int count = config_setting_length(root_setting);
        int setting_length;

        mf_log_info("Number of assigments: %d",count);

        if(count > utv_group2material.size()) {
            utv_group2material.resize(count+1,UTE_GROUP_ID_NOT_ASSIGNED);

            for(int i = 0; i < count; ++i){
                config_setting_t *material = config_setting_get_elem(root_setting, i);
                config_setting_t *setting;

                int matnum=0;

                // MATERIAL NUMBER
                if(NULL != (setting = config_setting_get_member(material, "material_number"))){
                    matnum  = (int)config_setting_get_int(setting);
                }
                else{
                    mf_fatal_err("In 'group_to_materials_assignment' in file %s missing 'material_number' in assigment %d ",
                                 mapping_file,i);
                }

                // groups
                if(NULL != (setting = config_setting_get_member(material, "groups"))){

					if (config_setting_is_list(setting) == CONFIG_TRUE) {
						setting_length = config_setting_length(setting);
						for (int j = 0; j < setting_length; ++j) {
							int  groupID = config_setting_get_int_elem(setting, j);

							if (groupID >= utv_group2material.size()) {
								utv_group2material.resize(groupID + 1, UTE_GROUP_ID_NOT_ASSIGNED);
							}

							if (utv_group2material[groupID] != UTE_GROUP_ID_NOT_ASSIGNED) {
								mf_log_warn("Duplicate 'group_to_materials_assignment' for groupID=%d ", groupID);
							}

							utv_group2material[groupID] = matnum;

							mf_log_info("Assigning group %d to material %d", groupID, matnum);
						}
					}
					else {
						int  groupID = config_setting_get_int(setting);

						if (groupID >= utv_group2material.size()) {
							utv_group2material.resize(groupID + 1, UTE_GROUP_ID_NOT_ASSIGNED);
						}

						if (utv_group2material[groupID] != UTE_GROUP_ID_NOT_ASSIGNED) {
							mf_log_warn("Duplicate 'group_to_materials_assignment' for groupID=%d ", groupID);
						}

						utv_group2material[groupID] = matnum;

						mf_log_info("Assigning group %d to material %d", groupID, matnum);

					}
                }
                else {
                    mf_log_err("In 'group_to_materials_assignment' in file %s missing 'groups' in assigment %d ", mapping_file,i);
                }
                
            }
        }
        mf_log_info("'group_to_materials_assignment' read! ");
    }

    config_destroy(&cfg);



    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, mapping_file)) {
        //fprintf(Interactive_output, "Material file error: %s:%d - %s\n", config_error_file(&cfg),
        //        config_error_line(&cfg), config_error_text(&cfg));
        mf_log_err("Could not open file! (%s) ", mapping_file);
        config_destroy(&cfg);
        return(UTE_FAIL);
      }

    root_setting = config_lookup(&cfg, "group_to_blocks_assignment");
    if(root_setting != NULL)
    {
        int count = config_setting_length(root_setting);
        int setting_length;

        mf_log_info("Number of assigments: %d",count);

        if(count > utv_group2block.size()) {
            utv_group2block.resize(count+1,UTE_GROUP_ID_NOT_ASSIGNED);

            for(int i = 0; i < count; ++i){
                config_setting_t *material = config_setting_get_elem(root_setting, i);
                config_setting_t *setting;

                int blocknum=0;
				double startTemp = 0.0;

                // MATERIAL NUMBER
                if(NULL != (setting = config_setting_get_member(material, "block_number"))){
                    blocknum  = (int)config_setting_get_int(setting);
                }
                else{
                    mf_fatal_err("In 'group_to_blocks_assignment' in file %s missing 'block_number' in assigment %d ",
                                 mapping_file,i);
                }
				
				// Init Temp
                if(NULL != (setting = config_setting_get_member(material, "init_temp"))){
					
					startTemp = 0.0;
                    startTemp  = (double)config_setting_get_float(setting);
					utv_temp2block.push_back(double(blocknum));
					utv_temp2block.push_back(startTemp);
					
                }
                else{
                    mf_log_warn("In 'group_to_blocks_assignment' in file %s missing 'init_temp' in assigment %d ",
                                 mapping_file,i);
                }

                // groups
                if(NULL != (setting = config_setting_get_member(material, "groups"))){

					if (config_setting_is_list(setting) == CONFIG_TRUE) {
						setting_length = config_setting_length(setting);
						for (int j = 0; j < setting_length; ++j) {
							int  groupID = config_setting_get_int_elem(setting, j);

							if (groupID >= utv_group2block.size()) {
								utv_group2block.resize(groupID + 1, UTE_GROUP_ID_NOT_ASSIGNED);
							}

							if (utv_group2block[groupID] != UTE_GROUP_ID_NOT_ASSIGNED) {
								mf_log_warn("Duplicate 'group_to_blocks_assignment' for groupID=%d ", groupID);
							}

							utv_group2block[groupID] = blocknum;

							mf_log_info("Assigning group %d to block %d", groupID, blocknum);
						}
					}
					else {
						int  groupID = config_setting_get_int(setting);

						if (groupID >= utv_group2block.size()) {
							utv_group2block.resize(groupID + 1, UTE_GROUP_ID_NOT_ASSIGNED);
						}

						if (utv_group2block[groupID] != UTE_GROUP_ID_NOT_ASSIGNED) {
							mf_log_warn("Duplicate 'group_to_blocks_assignment' for groupID=%d ", groupID);
						}

                        utv_group2block[groupID] = blocknum;

						mf_log_info("Assigning group %d to block %d", groupID, blocknum);

					}
                }
                else {
                    mf_log_err("In 'group_to_blocks_assignment' in file %s missing 'groups' in assigment %d ", mapping_file,i);
                }
                
            }
        }
        mf_log_info("'group_to_blocks_assignment' read! ");
    }


    config_destroy(&cfg);


    return(ret_val);
}


ute_mat_read_result utr_mat_read_material_file( const char *Work_dir,
                   const char *Filename,
                   FILE *Interactive_output)
{

	
	utr_mat_read_assigments(Work_dir,Filename,Interactive_output);
    //utr_mat_read_assigments(Work_dir,"materials_database.dat",Interactive_output);


    utv_read_material_files.insert(Filename);

    config_t cfg;
    config_setting_t *root_setting;
    const char *str;
    char material_file[300];

    int constant_thermal_conductivity=1;
    int constant_density=1;
    int constant_specific_heat=1;
    int no_other_parameters=1;
    int constant_viscosity=1;

    ute_mat_read_result ret_val = UTE_SUCCESS;

    //printf("%s\n",Work_dir);
    //printf("%s\n",Filename);
    sprintf(material_file, "%s/%s", Work_dir, Filename);
    //printf("%s\n",material_file);

    mf_log_info("Materials configuration file: %s",material_file);

    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, material_file)) {
        //fprintf(Interactive_output, "Material file error: %s:%d - %s\n", config_error_file(&cfg),
        //        config_error_line(&cfg), config_error_text(&cfg));
        mf_log_err("Could not open material file! (%s) ", material_file);
        config_destroy(&cfg);
        return(UTE_FAIL);
      }

    root_setting = config_lookup(&cfg, "materials");
    if(root_setting != NULL)
    {
        utt_materials* Materials_db = &utv_materials;



        int count = config_setting_length(root_setting);
        int i, j;
        int setting_length;
        const char *name;

        mf_log_info("Number of materials: %d",count);

        for(i = 0; i < count; ++i){
            config_setting_t *material = config_setting_get_elem(root_setting, i);
            config_setting_t *setting;

            // MATERIAL NUMBER
            if(NULL != (setting = config_setting_get_member(material, "material_number"))){
              int matnum  = (int)config_setting_get_int(setting);

              if( Materials_db->material_ids.end() !=
                      std::find( Materials_db->material_ids.begin(),
                                 Materials_db->material_ids.end(),
                                 matnum ) ) {
                      mf_fatal_err("Material with id (%d) already exist!", matnum );
              }

//////////////////////////////  //////////////////////////////  //////////////////////////////
/// Built-in material database uses id (0-utc_mat_database_max_reserved_id).
/// Users can define own materials with ids bigger than utc_mat_database_max_reserved_id.
//////////////////////////////  //////////////////////////////  //////////////////////////////
              if(strcmp(Filename,utc_mat_database_filename)) {
                if(matnum <= utc_mat_database_max_reserved_id) {
                    mf_log_err("Problem material file (%s) uses id (%d) reserved for built-in material database (0-%d)",
                                 Filename,matnum,utc_mat_database_max_reserved_id);
                }
             }
             else { // this is main material database
                  if(matnum > utc_mat_database_max_reserved_id) {
                      mf_log_err("Material from database uses id (%d) reserved for user materials(bigger than %d)",
                                   matnum,utc_mat_database_max_reserved_id);
                  }
             }
//////////////////////////////  //////////////////////////////  //////////////////////////////
              Materials_db->material_ids.push_back(matnum);
            }
            else{
              int matnum = Materials_db->material_data.size();
              mf_log_warn("Material without 'material_number' asssigned to material %d", matnum);
              Materials_db->material_ids.push_back(matnum);
            }

            utt_material_data& mat_data = Materials_db->material_data[ Materials_db->material_ids.back() ];
            utr_mat_init_material(& mat_data);

            mat_data.matnum = Materials_db->material_ids.back();

            // MATERIAL NAME
            config_setting_lookup_string(material, "name", &name);
            strcpy(mat_data.name, name);

			// DENSITY
			if (NULL != (setting = config_setting_get_member(material, "density"))){

				if (config_setting_is_list(setting) == CONFIG_TRUE)
				{
					setting_length = config_setting_length(setting);
					mat_data.Tfor_density =
						(double*)malloc(sizeof(double)*setting_length);
					mat_data.atT_density =
						(double*)malloc(sizeof(double)*setting_length);
					mat_data.density_num = setting_length;
					for (j = 0; j < setting_length; ++j)
					{
						mat_data.Tfor_density[j] =
							(double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0);
						mat_data.atT_density[j] =
							(double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1);
					}

					//              // for temperature dependance reference temperature must be > 0
					//              //TODO: move to check problem module
					//              if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){
					//                fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent visocisty. Exiting \n");
					//                config_destroy(&cfg);
					//                exit(-1);
					//                //return(EXIT_FAILURE);
					//              }
					ret_val = UTE_REF_TEMP_MUST_BE_GEQ_ZERO;

					constant_density = 0;

				}
				else if (NULL != config_setting_get_string(setting))
				{
					mat_data.density_num = 1;
					mat_data.Tfor_density = (double*)malloc(sizeof(double));
					mat_data.expression_of_dynamic_viscosity_from_T = (void*) new exprtk::expression<double>();

					std::string expression_string(config_setting_get_string(setting));	
					exprtk::symbol_table<double>	symbol_table;
					exprtk::expression<double>	&	expression = *((exprtk::expression<double>*)mat_data.expression_of_dynamic_viscosity_from_T);

					symbol_table.add_variable("T", mat_data.Tfor_density[0]);
					symbol_table.add_constants();
					expression.register_symbol_table(symbol_table);
					
					exprtk::parser<double>			parser;
					parser.compile(expression_string, expression);
				}
				else
				{
					mat_data.density_num = 1;
					mat_data.Tfor_density = (double*)malloc(sizeof(double));
					mat_data.atT_density = (double*)malloc(sizeof(double));
					mat_data.Tfor_density[0] = -1.0;
					mat_data.atT_density[0] =
						(double)config_setting_get_float(setting);


				}
			}

            // THERMAL_COND.
            if(NULL != (setting = config_setting_get_member(material, "thermal_conductivity"))){

              if(config_setting_is_list(setting) == CONFIG_TRUE)
                {
              setting_length = config_setting_length(setting);
              mat_data.Tfor_thermal_conductivity =
                (double*) malloc(sizeof(double)*setting_length);
              mat_data.atT_thermal_conductivity =
                (double*) malloc(sizeof(double)*setting_length);
              mat_data.thermal_conductivity_num = setting_length;
              for(j = 0; j < setting_length; ++j)
                {
                  mat_data.Tfor_thermal_conductivity[j] =
                    (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 0);
                  mat_data.atT_thermal_conductivity[j]  =
                    (double)config_setting_get_float_elem(config_setting_get_elem(setting, j), 1);
                }

//              // for temperature dependance reference temperature must be > 0
//              if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){
//                fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent thermal_conductivity. Exiting \n");
//                config_destroy(&cfg);
//                exit(-1);
//                //return(EXIT_FAILURE);
//              }
              ret_val = UTE_REF_TEMP_MUST_BE_GEQ_ZERO;

              constant_thermal_conductivity = 0;

                }
              else
                {
              mat_data.thermal_conductivity_num = 1;
              mat_data.Tfor_thermal_conductivity =
                (double*) malloc(sizeof(double));
              mat_data.atT_thermal_conductivity =
                (double*) malloc(sizeof(double));
              mat_data.Tfor_thermal_conductivity[0] = -1.0;
              mat_data.atT_thermal_conductivity[0] =
                (double)config_setting_get_float(setting);
                }
            }

            // SPECYFIC HEAT
            if(NULL != (setting = config_setting_get_member(material, "specific_heat"))){

              if(config_setting_is_list(setting) == CONFIG_TRUE)
                {
              setting_length = config_setting_length(setting);
              mat_data.Tfor_specific_heat =
                (double*) malloc(sizeof(double)*setting_length);
              mat_data.atT_specific_heat =
                (double*) malloc(sizeof(double)*setting_length);
              mat_data.specific_heat_num = setting_length;
              for(j = 0; j < setting_length; ++j)
                {
                  mat_data.Tfor_specific_heat[j] =
                    (double)config_setting_get_float_elem(config_setting_get_elem(setting,j), 0);
                  mat_data.atT_specific_heat[j]  =
                    (double)config_setting_get_float_elem(config_setting_get_elem(setting,j), 1);
                }

              // for temperature dependance reference temperature must be > 0
//              if(pdv_heat_problem.ctrl.ref_temperature <= 0.0){
//                fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent specific heat. Exiting \n");
//                config_destroy(&cfg);
//                exit(-1);
//                //return(EXIT_FAILURE);
//              }
              ret_val = UTE_REF_TEMP_MUST_BE_GEQ_ZERO;

              constant_specific_heat = 0;

                }
              else
                {
              mat_data.specific_heat_num = 1;
              mat_data.Tfor_specific_heat =
                (double*) malloc(sizeof(double));
              mat_data.atT_specific_heat =
                (double*) malloc(sizeof(double));
              mat_data.Tfor_specific_heat[0] = -1.0;
              mat_data.atT_specific_heat[0]  =
                (double)config_setting_get_float(setting);
                }
            }

            /////////////////////////// NS_SUPG/////////////////////////

            if(NULL != (setting = config_setting_get_member(material, "dynamic_viscosity"))){

              if(config_setting_is_list(setting) == CONFIG_TRUE)
                {
              setting_length = config_setting_length(setting);
              //printf("\n\tAS: dynamic viscosity - setting_length =%d\n", setting_length);
              mat_data.Tfor_dynamic_viscosity =
                (double*) malloc(sizeof(double)*setting_length);
              mat_data.atT_dynamic_viscosity =
                (double*) malloc(sizeof(double)*setting_length);
              mat_data.dynamic_viscosity_num = setting_length;
              for(j = 0; j < setting_length; ++j)
                {
                  mat_data.Tfor_dynamic_viscosity[j] =
                    (double)config_setting_get_float_elem(
                                config_setting_get_elem(setting, j), 0);
                  mat_data.atT_dynamic_viscosity[j]  =
                    (double)config_setting_get_float_elem(
                                config_setting_get_elem(setting, j), 1);
                }

              // for temperature dependance reference temperature must be > 0
              //if(pdv_ns_supg_problem.ctrl.ref_temperature <= 0.0){
//                fprintf(Interactive_output, "ref_temperature <= 0.0 for temperature dependent visocisty. Exiting \n");
//                config_destroy(&cfg);
//                exit(-1);
//                //return(EXIT_FAILURE);
                  ret_val = UTE_REF_TEMP_MUST_BE_GEQ_ZERO;


              constant_viscosity = 0;

                }
              else
                {

              mat_data.dynamic_viscosity_num = 1;
              mat_data.Tfor_dynamic_viscosity =
                (double*) malloc(sizeof(double));
              mat_data.atT_dynamic_viscosity =
                (double*) malloc(sizeof(double));
              mat_data.Tfor_dynamic_viscosity[0] = -1.0;
              mat_data.atT_dynamic_viscosity[0]  =
                (double)config_setting_get_float(setting);

                }

            }


        }

        // REGISTERING deleting function
        atexit(utr_mat_clear_all);

        config_destroy(&cfg);

          if(constant_thermal_conductivity==1 && constant_density==1
                  && constant_specific_heat==1 && no_other_parameters==1 && count==1){

            if(ret_val== UTE_SUCCESS) {
                ret_val = UTE_NOT_NEED_MATERIAL_DATABASE;
                mf_log_warn("Materials database not needed!");
            }
          }

    }

    mf_log_info("Materials read! ");

    return(ret_val);
}

// INTERFACE FUNCTIONS
void utr_mat_init_query_result(utt_material_query_result* result) {
    if(result!= NULL) {
        result->dynamic_viscosity = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->density = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->thermal_conductivity = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->specific_heat = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->enthalpy = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->thermal_expansion_coefficient = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->electrical_resistivity = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->VOF = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->dg_dT = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->temp_solidus = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->temp_liquidus = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->temp_vaporization = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->latent_heat_of_fusion = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
        result->latent_heat_of_vaporization = UTC_MAT_QUERY_RESULT_NOT_NEEDED;
    }
}


ute_mat_read_result utr_mat_read(const char *Work_dir,
                   const char *Filename,
                   FILE *Interactive_output)
{
    // If file already read ignore it.
    if( utv_read_material_files.find(Filename) != utv_read_material_files.end()) {
        return UTE_SUCCESS;
    }
    else if(utv_read_material_files.empty()) {
        // read main database material file
        utr_mat_read_material_file(Work_dir,utc_mat_database_filename,Interactive_output);
    }

    return utr_mat_read_material_file(Work_dir,Filename,Interactive_output);

//    utr_mat_read_material_file(Work_dir,Filename,Interactive_output);
//    return utr_mat_read_material_file(Work_dir,utc_mat_database_filename,Interactive_output);


}




int utr_material_query(const utt_material_query_params *Params,
                         utt_material_query_result *Result)
{
    int i;
    //int material_idx = -1;
    const utt_material_data *material=utr_mat_get_material(Params->group_idx);
    

	
	double v1, v2, t1, t2;
    double a, b;

    strcpy(Result->name, material->name);
    //result->queryparams = params;

    if(Result->dynamic_viscosity == UTC_MAT_QUERY_RESULT_REQUIRED) {
        //viscosity
        if (material->dynamic_viscosity_num == 1)
            Result->dynamic_viscosity = material->atT_dynamic_viscosity[0];
        else {
		

		
            if (Params->temperature <= material->Tfor_dynamic_viscosity[0])
                Result->dynamic_viscosity = material->atT_dynamic_viscosity[0];
            else if (Params->temperature >=
                     material->Tfor_dynamic_viscosity[material->dynamic_viscosity_num-1])
                Result->dynamic_viscosity =
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
                        Result->dynamic_viscosity = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //density
    if(Result->density == UTC_MAT_QUERY_RESULT_REQUIRED ) {
		if (material->density_num == 1) {
			if (material->expression_of_dynamic_viscosity_from_T != NULL) {
				material->Tfor_density[0] = Params->temperature;
                // calculating value using given formula
				Result->density = ((exprtk::expression<double>*)material->expression_of_dynamic_viscosity_from_T)->value();
			}
			else {
				Result->density = material->atT_density[0];
			}
		}
        else {
            if (Params->temperature <= material->Tfor_density[0])
                Result->density = material->atT_density[0];
            else if (Params->temperature >=
                     material->Tfor_density[material->density_num - 1])
                Result->density = material->atT_density[material->density_num - 1];
            else
                for (i = 0; i < material->density_num - 1; ++i) {
                    if (Params->temperature <= material->Tfor_density[i + 1]) {
                        t1 = material->Tfor_density[i];
                        t2 = material->Tfor_density[i + 1];
                        v1 = material->atT_density[i];
                        v2 = material->atT_density[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->density = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //thermal_conductivity
    if(Result->thermal_conductivity == UTC_MAT_QUERY_RESULT_REQUIRED) {
        if (material->thermal_conductivity_num == 1)
            Result->thermal_conductivity = material->atT_thermal_conductivity[0];
        else {
            if (Params->temperature <= material->Tfor_thermal_conductivity[0])
                Result->thermal_conductivity = material->atT_thermal_conductivity[0];
            else if (Params->temperature >=
                     material->Tfor_thermal_conductivity[material->thermal_conductivity_num - 1])
                Result->thermal_conductivity =
                        material->atT_thermal_conductivity[material->thermal_conductivity_num-1];
            else
                for (i = 0; i < material->thermal_conductivity_num - 1; ++i) {
                    if (Params->temperature <= material->Tfor_thermal_conductivity[i + 1]) {
                        t1 = material->Tfor_thermal_conductivity[i];
                        t2 = material->Tfor_thermal_conductivity[i + 1];
                        v1 = material->atT_thermal_conductivity[i];
                        v2 = material->atT_thermal_conductivity[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->thermal_conductivity = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //specific_heat
    if(Result->specific_heat == UTC_MAT_QUERY_RESULT_REQUIRED) {
        if (material->specific_heat_num == 1)
            Result->specific_heat = material->atT_specific_heat[0];
        else {
            if (Params->temperature <= material->Tfor_specific_heat[0])
                Result->specific_heat = material->atT_specific_heat[0];
            else if (Params->temperature >=
                     material->Tfor_specific_heat[material->specific_heat_num - 1])
                Result->specific_heat =
                        material->atT_specific_heat[material->specific_heat_num - 1];
            else
                for (i = 0; i < material->specific_heat_num - 1; ++i) {
                    if (Params->temperature <= material->Tfor_specific_heat[i + 1]) {
                        t1 = material->Tfor_specific_heat[i];
                        t2 = material->Tfor_specific_heat[i + 1];
                        v1 = material->atT_specific_heat[i];
                        v2 = material->atT_specific_heat[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->specific_heat = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //thermal_expansion_coefficient
    if(Result->thermal_expansion_coefficient == UTC_MAT_QUERY_RESULT_REQUIRED) {
        if (material->thermal_expansion_coefficient_num == 1)
            Result->thermal_expansion_coefficient =
                    material->atT_thermal_expansion_coefficient[0];
        else {
            if (Params->temperature <= material->Tfor_thermal_expansion_coefficient[0])
                Result->thermal_expansion_coefficient =
                        material->atT_thermal_expansion_coefficient[0];
            else if (Params->temperature >=
                     material->Tfor_thermal_expansion_coefficient[material->thermal_expansion_coefficient_num - 1])
                Result->thermal_expansion_coefficient =
                        material->atT_thermal_expansion_coefficient[material->thermal_expansion_coefficient_num - 1];
            else
                for (i = 0; i < material->thermal_expansion_coefficient_num - 1; ++i) {
                    if (Params->temperature <=
                            material->Tfor_thermal_expansion_coefficient[i + 1]) {
                        t1 = material->Tfor_thermal_expansion_coefficient[i];
                        t2 = material->Tfor_thermal_expansion_coefficient[i + 1];
                        v1 = material->atT_thermal_expansion_coefficient[i];
                        v2 = material->atT_thermal_expansion_coefficient[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->thermal_expansion_coefficient = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //electrical_resistivity
    if(Result->electrical_resistivity == UTC_MAT_QUERY_RESULT_REQUIRED) {
        if (material->electrical_resistivity_num == 1)
            Result->electrical_resistivity = material->atT_electrical_resistivity[0];
        else {
            if (Params->temperature <= material->Tfor_electrical_resistivity[0])
                Result->electrical_resistivity = material->atT_electrical_resistivity[0];
            else if (Params->temperature >=
                     material->Tfor_electrical_resistivity[material->electrical_resistivity_num - 1])
                Result->electrical_resistivity =
                        material->atT_electrical_resistivity[material->electrical_resistivity_num - 1];
            else
                for (i = 0; i < material->electrical_resistivity_num - 1; ++i) {
                    if (Params->temperature <=
                            material->Tfor_electrical_resistivity[i + 1]) {
                        t1 = material->Tfor_electrical_resistivity[i];
                        t2 = material->Tfor_electrical_resistivity[i + 1];
                        v1 = material->atT_electrical_resistivity[i];
                        v2 = material->atT_electrical_resistivity[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->electrical_resistivity = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //enthalpy
    if(Result->enthalpy == UTC_MAT_QUERY_RESULT_REQUIRED) {
        if (material->enthalpy_num == 1)
            Result->enthalpy = material->atT_enthalpy[0];
        else {
            if (Params->temperature <= material->Tfor_enthalpy[0])
                Result->enthalpy = material->atT_enthalpy[0];
            else if (Params->temperature >=
                     material->Tfor_enthalpy[material->enthalpy_num - 1])
                Result->enthalpy = material->atT_enthalpy[material->enthalpy_num - 1];
            else
                for (i = 0; i < material->enthalpy_num - 1; ++i) {
                    if (Params->temperature <= material->Tfor_enthalpy[i + 1]) {
                        t1 = material->Tfor_enthalpy[i];
                        t2 = material->Tfor_enthalpy[i + 1];
                        v1 = material->atT_enthalpy[i];
                        v2 = material->atT_enthalpy[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->enthalpy = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //VOF */
    if(Result->VOF == UTC_MAT_QUERY_RESULT_REQUIRED) {
        if (material->VOF_num == 1)
            Result->VOF = material->atT_VOF[0];
        else {
            if (Params->temperature <= material->Tfor_VOF[0])
                Result->VOF = material->atT_VOF[0];
            else if (Params->temperature >= material->Tfor_VOF[material->VOF_num - 1])
                Result->VOF = material->atT_VOF[material->VOF_num - 1];
            else
                for (i = 0; i < material->VOF_num - 1; ++i) {
                    if (Params->temperature <= material->Tfor_VOF[i + 1]) {
                        t1 = material->Tfor_VOF[i];
                        t2 = material->Tfor_VOF[i + 1];
                        v1 = material->atT_VOF[i];
                        v2 = material->atT_VOF[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->VOF = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    //dg_dT */
    if(Result->dg_dT == UTC_MAT_QUERY_RESULT_REQUIRED) {
        if (material->dg_dT_num == 1)
            Result->dg_dT = material->atT_dg_dT[0];
        else {
            if (Params->temperature <= material->Tfor_dg_dT[0])
                Result->dg_dT = material->atT_dg_dT[0];
            else if (Params->temperature >= material->Tfor_dg_dT[material->dg_dT_num - 1])
                Result->dg_dT = material->atT_dg_dT[material->dg_dT_num - 1];
            else
                for (i=0; i < material->dg_dT_num - 1; ++i) {
                    if (Params->temperature <= material->Tfor_dg_dT[i + 1]) {
                        t1 = material->Tfor_dg_dT[i];
                        t2 = material->Tfor_dg_dT[i + 1];
                        v1 = material->atT_dg_dT[i];
                        v2 = material->atT_dg_dT[i + 1];
                        a = (v1 - v2) / (t1 - t2);
                        b = v1 - a * t1;
                        Result->dg_dT = a * (Params->temperature) + b;
                        break;
                    }
                }
        }
    }

    if(Result->temp_solidus == UTC_MAT_QUERY_RESULT_REQUIRED) {
        Result->temp_solidus = material->temp_solidus;
    }
    if(Result->temp_liquidus  == UTC_MAT_QUERY_RESULT_REQUIRED) {
        Result->temp_liquidus = material->temp_liquidus;
    }
    if(Result->temp_vaporization  == UTC_MAT_QUERY_RESULT_REQUIRED) {
        Result->temp_vaporization = material->temp_vaporization;
    }
    if(Result->latent_heat_of_fusion == UTC_MAT_QUERY_RESULT_REQUIRED) {
        Result->latent_heat_of_fusion = material->latent_heat_of_fusion;
    }
    if(Result->latent_heat_of_vaporization == UTC_MAT_QUERY_RESULT_REQUIRED) {
        Result->latent_heat_of_vaporization = material->latent_heat_of_vaporization;
    }
    return 0;
}



int utr_mat_get_matID(int groupID)
{
    int material_idx = UTE_GROUP_ID_NOT_ASSIGNED;


    if(groupID < utv_group2material.size()) {
        material_idx = utv_group2material[groupID];
    }

    if(material_idx ==  UTE_GROUP_ID_NOT_ASSIGNED) {
        mf_log_warn("Requested 'material_number' for groupID=%d which is not assigned!",groupID );

		
		
        for (int i = 0; i < utv_materials.material_data.size(); ++i) {
            if(utv_materials.material_ids[i] == groupID){
          material_idx = utv_materials.material_ids[i];
          break;
            }
        }

		
		
        if(groupID >= utv_group2material.size()) {
            utv_group2material.resize(groupID+1,UTE_GROUP_ID_NOT_ASSIGNED);
        }
        // Assigning permanently to avoid next searches.
        utv_group2material[groupID] = material_idx;
        mf_log_info("Group %d assigned to material %d",groupID, material_idx);

    }
    //mf_log_info("Group_id(%d), Material_id(%d)",groupID,material_idx);
    mf_check(material_idx !=UTE_GROUP_ID_NOT_ASSIGNED,"Material not found for given Group_id (%d) ",groupID);

    return material_idx;
	
}

int utr_mat_get_blockID(int groupID)
{
    mf_check(groupID > 0, "GroupID should be > 0 (is %d)", groupID);

    int blockID = UTE_GROUP_ID_NOT_ASSIGNED;

    if(groupID < utv_group2block.size()) {
        blockID = utv_group2block[groupID];
    }

    if(blockID ==  UTE_GROUP_ID_NOT_ASSIGNED) {
        mf_log_warn("Requested blockID for groupID=%d which is not assigned!", groupID);
        blockID = UTE_GROUP_ID_DEFAULT_BLOCK;
        mf_log_info("Group %d assigned to default block %d",groupID, blockID);
    }

    return blockID;
}

double utr_temp2block_id(int id){
	
	double ID = -1.0;
	
    if(id < utv_temp2block.size()) {
        ID = utv_temp2block[id];
    }	
	
    if(ID ==  -1.0) {
        mf_log_warn("Requested utr_temp2block_id for id =%d which is not assigned!", id);
    }	
	
	return ID; 
	
}

int utr_temp2block_size(){
	
	return utv_temp2block.size(); 
	
}

int utr_mat_get_n_materials()
{
    return utv_materials.material_data.size();
}

void utr_mat_clear_all()
{
    utv_materials.material_data.clear();
    utv_materials.material_ids.clear();
    utv_group2block.clear();
    utv_group2material.clear();
    utv_read_material_files.clear();
	utv_temp2block.clear();

}

const utt_material_data* utr_mat_get_material(const int groupID)
{
    mfp_check_debug(utr_mat_get_matID(groupID) >= 0,"Material id incorrect(%d)",utr_mat_get_matID(groupID) );

//    mfp_check_debug( ( utv_materials.material_data.find(utr_mat_get_matID(groupID)) !=  utv_materials.material_data.end() ),
//                    "Material id incorrect(%d)",
//                     utr_mat_get_matID(groupID) );

    return & ( utv_materials.material_data[utr_mat_get_matID(groupID)] );
}

const utt_material_data* utr_mat_get_material_by_matID(int matID)
{
    mfp_check_debug(matID >= 0,"Material id incorrect(%d)",matID);
    mfp_check_debug(matID < utv_materials.material_data.size(),"Material id incorrect(%d)",matID);

    return & ( utv_materials.material_data[matID] );
}

#ifdef __cplusplus
}
#endif

#endif //UTS_MAT_CPP
