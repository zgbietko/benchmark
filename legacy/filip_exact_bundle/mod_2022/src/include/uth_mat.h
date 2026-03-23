#ifndef UTS_MAT_H
#define UTS_MAT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 \defgroup UTM_MAT Materials Utilities
 \ingroup UTM
*
* The materials submodule of the utilities module is responsible for \link utr_mat_read reading the material file(s)  \endlink
* and \link utr_material_query providing access to the exisiting database \endlink.
* The default and preferred way of accessing the materials is to make use of \link utr_material_query material query function \endlink with \link mmr_el_groupID groupID \endlink
* as a parameter.
*
* The correct way of using the material database is as follows:
*
* At program initialisation:
* 1. \link utr_mat_read Read the material file(s) \endlink at least once.
*   - the \link utc_mat_database_filename main material base \endlink will be automaticly readed
*   - provided material file should contain information about \link utr_mat_get_matID group-to-material \endlink and \link utr_mat_get_blockID group-to-block \endlink information.
*     example blocks with those informations could look like this:
*
*    group_to_materials_assignment: (
*    {
*    material_number = 10;
*    groups = (15,11);
*    },
*    {
*    material_number = 5;
*    groups = ( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 );
*    });
*
*    group_to_blocks_assignment: (
*    {
*    block_number = 1;
*    groups = (15,11);
*    },
*    {
*    block_number = 35;
*    groups = ( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 );
*    });
*
* During computations:
* 2. Get the \link mmr_el_groupID groupID of the element \endlink you are analysing.
* 3. Create \link utt_material_query_params structure with params \endlink of the material query.
* - obligatory paramteres are:
*   + \link utt_material_query_params.group_idx groupID \endlink of the element you are querying
*   + \link utt_material_query_params.temperature current temperature \endlink if you want to use material properties depending on temperature
* - Additional parameters are:
*   + \link utt_query_type type of query \endlink
*   + \link utt_material_query_params.xg coordinates of point of interest \endlink
* 4. Create \link utt_material_query_result query result \endlink and initialize it with \link utr_mat_init_query_result \endlink.
* 5. Select material properties that you want to obtain:
*  - for each desired field of initialized \link utt_material_query_result query result structure \endlink set it to the value \link UTC_MAT_QUERY_RESULT_REQUIRED \endlink.
* 6. Finally call \link utr_material_query \endlink providing query parameters from step 3 and query result from step 4-5 as arguments.
* 7. As function successfully returns fields from the \link utt_material_query_result result \endlink previously assigned with
* \link UTC_MAT_QUERY_RESULT_REQUIRED \endlink will store information about selected material properties.
*
* Examples of the material query usage can be found in \link pdr_heat_material_query heat module \endlink, \link pdr_ns_supg_material_query ns_supg module \endlink
* and \link pdr_ns_supg_heat_vof_material_query ns_supg-heat-vof supermodule \endlink.
*
* NOTE: The materials submodule provides also \link utr_mat_get_material direct access \endlink to the \link utt_material_data raw material data \endlink,
* but it is intended to be an additional feature for special purposes,
* and using this other way is much more cumbersome during computations.
*
 @{
 */

/// Filename of the default built-in material database linked with executable.
/// The default location of this file is the directory of the executable.
/// If given problem require to provide additional materials, then the material file
/// given in problem file should be modyfied.
extern const char* utc_mat_database_filename;

/** internal structure filled from materials file - not for direct access!*/
typedef struct {

  int matnum;
  char name[20];

  double *Tfor_dynamic_viscosity;
  double *atT_dynamic_viscosity;
  void*	expression_of_dynamic_viscosity_from_T;
  int dynamic_viscosity_num;

  double *Tfor_density;
  double *atT_density;
  int density_num;

  double *Tfor_thermal_conductivity;
  double *atT_thermal_conductivity;
  int thermal_conductivity_num;

  double *Tfor_specific_heat;
  double *atT_specific_heat;
  int specific_heat_num;

  double *Tfor_enthalpy;
  double *atT_enthalpy;
  int enthalpy_num;

  double *Tfor_thermal_expansion_coefficient;
  double *atT_thermal_expansion_coefficient;
  int thermal_expansion_coefficient_num;

  double *Tfor_electrical_resistivity;
  double *atT_electrical_resistivity;
  int electrical_resistivity_num;

  double *Tfor_VOF;
  double *atT_VOF;
  int VOF_num;

  double *Tfor_dg_dT;
  double *atT_dg_dT;
  int dg_dT_num;

  double temp_solidus;
  double temp_liquidus;
  double temp_vaporization;

  double latent_heat_of_fusion;
  double latent_heat_of_vaporization;

} utt_material_data;

typedef enum {
    QUERY_NODE,
    QUERY_POINT
} utt_query_type;

/** input for pdr_material_query */
typedef struct{
  char * name; //if searching by idx (idx >= 0) set to anything
  int group_idx; //set -1 if you want to search by name
  double temperature;
  double xg[3];
  utt_query_type query_type;
  int cell_id;
  int node_id;
    double aux;
  //in future: elem no., coordinates etc.
} utt_material_query_params;

typedef struct{
    char name[20];
    double dynamic_viscosity;
    double density;
    double thermal_conductivity;
    double specific_heat;
    double enthalpy;
    double thermal_expansion_coefficient;
    double electrical_resistivity;
    double VOF;
    double dg_dT;
    double temp_solidus;
    double temp_liquidus;
    double temp_vaporization;
    double latent_heat_of_fusion;
    double latent_heat_of_vaporization;
} utt_material_query_result;

#define UTC_MAT_QUERY_RESULT_NOT_NEEDED -1.0
#define UTC_MAT_QUERY_RESULT_REQUIRED -3.0


typedef enum {
    UTE_GROUP_ID_NOT_ASSIGNED = -1,
    UTE_GROUP_ID_DEFAULT_BLOCK = 0

} ute_mat_groupID_flag;


typedef enum {
    UTE_SUCCESS = 0,
    UTE_FAIL = -1,
    UTE_REF_TEMP_MUST_BE_GEQ_ZERO = -2,
    UTE_NOT_NEED_MATERIAL_DATABASE = 1
} ute_mat_read_result;

// Main functions

double utr_temp2block_id(int id);
int utr_temp2block_size();

ute_mat_read_result utr_mat_read(const char *Work_dir,
                 const char *Filename,
                 FILE *Interactive_output);

void utr_mat_init_query_result(utt_material_query_result* result);

int utr_material_query(const utt_material_query_params *Params,
                         utt_material_query_result *Result);

int utr_mat_get_matID(int groupID);

int utr_mat_get_blockID(int groupID);

int utr_mat_get_n_materials();

void utr_mat_clear_all();

// Additional functions

//int utr_read_mat_and_block_mapID(config_setting_t *root_setting);

const utt_material_data* utr_mat_get_material(const int groupID);

const utt_material_data *utr_mat_get_material_by_matID(int matID);

/** @} */ // end of group


#ifdef __cplusplus
}
#endif

#endif //UTS_MAT_H
