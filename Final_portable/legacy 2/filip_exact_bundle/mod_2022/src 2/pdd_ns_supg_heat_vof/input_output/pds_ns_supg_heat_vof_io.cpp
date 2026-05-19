/************************************************************************
File pds_ns_supg_heat_vof_problem_io.c - problem data read

Contains definition of routines:
  pdr_ns_supg_heat_vof_dump_data - to write mesh and field data to files 

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utilities - including simple time measurement library */
#include "uth_intf.h"		/* USES */
/* interface for all mesh manipulation modules */
#include "mmh_intf.h"		/* USES */
/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* interface for all solver modules */
#include "sih_intf.h"		/* USES */
/* problem dependent module interface */
#include "pdh_intf.h"		/* USES */
/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"	/* USES */

/* problem module's types and functions */
#include "../include/pdh_ns_supg_heat_vof.h" /* USES & IMPLEMENTS */
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
#include "../../pdd_ns_supg/include/pdh_ns_supg_materials.h" 
#include "../../pdd_heat/include/pdh_heat_materials.h" 
#include "../../pdd_vof/include/pdh_vof_materials.h"
// bc and material header files are included in problem header files

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_ns_supg_vof_material_query - gets material data for node belonging to patch
------------------------------------------------------------*/
int pdr_ns_supg_vof_material_query(
  const int node_id,
  const utt_patches * pdv_patches,
  const pdt_ns_supg_materials * Materials_db, 
  pdt_ns_supg_material_query_params * Params, 
  pdt_ns_supg_material_query_result ** Result_p)
{
  int i, j;
  pdt_ns_supg_material_query_result *Result;
  pdt_ns_supg_problem *problem_ns_supg = (pdt_ns_supg_problem *)pdr_get_problem_structure(PDC_NS_SUPG_ID);
 /*  //if ( Result != NULL) free(Result); */
 /*  for (i=0; i < pdv_patches[node_id].nr_elems; ++i) { */
 /*    Result[i] = (pdt_ns_supg_material_query_result *)malloc(sizeof(pdt_ns_supg_material_query_result)); */
 /*  } */
  Result = (pdt_ns_supg_material_query_result *)malloc((pdv_patches[node_id].nr_elems+1)*sizeof(pdt_ns_supg_material_query_result));

  for (i=0; i < pdv_patches[node_id].nr_elems; ++i) {
    Params->material_idx = mmr_el_groupID(problem_ns_supg->ctrl.mesh_id, pdv_patches[node_id].elems[i]);
    pdr_ns_supg_material_query(&problem_ns_supg->materials, Params, &(Result[i]));
 }
  *Result_p = Result;
  return (0);
}

/*------------------------------------------------------------
pdr_heat_vof_material_query - gets material data for node belonging to patch
------------------------------------------------------------*/
int pdr_heat_vof_material_query(
  const int node_id,
  const utt_patches * pdv_patches,
  const pdt_heat_materials * Materials_db, 
  pdt_heat_material_query_params * Params, 
  pdt_heat_material_query_result ** Result_p)
{
  int i, j;
  pdt_heat_material_query_result *Result;
  pdt_heat_problem *problem_heat = (pdt_heat_problem *)pdr_get_problem_structure(PDC_HEAT_ID);
  //if ( Result != NULL) free(Result);
  //for (i=0; i < pdv_patches[node_id].nr_elems; ++i) {
  //  (*Result_p)[i] = (pdt_heat_material_query_result *)malloc(sizeof(pdt_heat_material_query_result));
  //}
  Result = (pdt_heat_material_query_result *)malloc((pdv_patches[node_id].nr_elems+1)*sizeof(pdt_heat_material_query_result));

  for (i=0; i < pdv_patches[node_id].nr_elems; ++i) {
    Params->material_idx = mmr_el_groupID(problem_heat->ctrl.mesh_id, pdv_patches[node_id].elems[i]);
    pdr_heat_material_query(&problem_heat->materials, Params, &(Result[i]));
  }
  *Result_p = Result;
  return (0);
}

/*------------------------------------------------------------
pdr_ns_supg_heat_vof_dump_data - to write mesh and field data to files 
------------------------------------------------------------*/
int pdr_ns_supg_heat_vof_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
)
{
  int mesh_id, field_id, iaux;
  char filename[300];

  // as usual NS_SUPG is the default problem

  // different fields for ns_supg, heat and vof, different files for different processors, different files for different time steps
  // temporary sequential version
  mesh_id = pdr_ctrl_i_params(PDC_NS_SUPG_ID, 2);

  // writing mesh
  sprintf(filename, "%s/%s%d.dmp", Work_dir, 
	  pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern, 
	  pdv_ns_supg_problem.time.cur_step);  

  iaux = mmr_export_mesh(mesh_id, MMC_MOD_FEM_MESH_DATA, filename);
  if(iaux<0) {
    /* if error in writing data */
    fprintf(Interactive_output,"Error in writing mesh data!\n");
  }

  // writing ns_supg field
  field_id = pdv_ns_supg_problem.ctrl.field_id;
  int nreq = pdv_ns_supg_problem.ctrl.nreq;

  sprintf(filename, "%s/%s%d.dmp", Work_dir, 
	  pdv_ns_supg_problem.ctrl.field_dmp_filepattern, 
	  pdv_ns_supg_problem.time.cur_step);  

  iaux = utr_io_export_field(field_id, nreq, 0, 0, filename);
  if(iaux<0) {
    /* if error in writing data */
    fprintf(Interactive_output,"Error in writing ns_supg field data!\n");
  }

  // writing heat field
  field_id = pdv_heat_problem.ctrl.field_id;
  nreq = pdv_heat_problem.ctrl.nreq;

  sprintf(filename, "%s/%s%d.dmp", Work_dir, 
	  pdv_heat_problem.ctrl.field_dmp_filepattern, 
	  pdv_heat_problem.time.cur_step);  

  iaux = utr_io_export_field(field_id, nreq, 0, 0, filename);
  if(iaux<0) {
    /* if error in writing data */
    fprintf(Interactive_output,"Error in writing heat field data!\n");
  }

  // writing vof field
  field_id = pdv_vof_problem.ctrl.field_id;
  nreq = pdv_vof_problem.ctrl.nreq;

  sprintf(filename, "%s/%s%d.dmp", Work_dir, 
	  pdv_vof_problem.ctrl.field_dmp_filepattern, 
	  pdv_vof_problem.time.cur_step);  

  iaux = utr_io_export_field(field_id, nreq, 0, 0, filename);
  if(iaux<0) {
    /* if error in writing data */
    fprintf(Interactive_output,"Error in writing vof field data!\n");
  }

   //!!!!!!!!!!
  /* pdt_ns_supg_problem * problem_ns_supg = (pdt_ns_supg_problem *) */
  /*   pdr_get_problem_structure(PDC_NS_SUPG_ID); */
  /* pdt_heat_problem *problem_heat = (pdt_heat_problem *)  */
  /*   pdr_get_problem_structure(PDC_HEAT_ID); */

/*   sprintf(arg, "%s/%s%d.dmp", Work_dir, problem_ns_supg->ctrl.mesh_dmp_filepattern, problem_ns_supg->time.cur_step); */
/*   if (0 == strcmp(problem_ns_supg->ctrl.mesh_type, "j")) { */
/*     if (mmr_export_mesh(problem_ns_supg->ctrl.mesh_id, MMC_MOD_FEM_MESH_DATA, arg) < 0) */
/*       fprintf(Interactive_output, "Error in writing mesh data!\n"); */
/*   } else if (0 == strcmp(problem_ns_supg->ctrl.mesh_type, "t")) { */
/*     if (mmr_export_mesh(problem_ns_supg->ctrl.mesh_id, MMC_MOD_FEM_TETRA_DATA, arg) < 0) */
/*       fprintf(Interactive_output, "Error in writing mesh data!\n"); */
/*   } else if (0 == strcmp(problem_ns_supg->ctrl.mesh_type, "h")) { */
/*     if (mmr_export_mesh(problem_ns_supg->ctrl.mesh_id, MMC_MOD_FEM_HYBRID_DATA, arg) < 0) */
/*       fprintf(Interactive_output, "Error in writing mesh data!\n"); */
/*   } else if (0 == strcmp(problem_ns_supg->ctrl.mesh_type, "s")) { */
/*     if (mmr_export_mesh(problem_ns_supg->ctrl.mesh_id, MMC_MOD_FEM_MESH_DATA, arg) < 0) */
/*       fprintf(Interactive_output, "Error in writing mesh data!\n"); */
/*   } else if (0 == strcmp(problem_ns_supg->ctrl.mesh_type, "n")) { */
/*     if (mmr_export_mesh(problem_ns_supg->ctrl.mesh_id, MMC_NASTRAN_DATA, arg) < 0) */
/*       fprintf(Interactive_output, "Error in writing mesh data!\n"); */
/*   } else { */
/*     if (mmr_export_mesh(problem_ns_supg->ctrl.mesh_id, MMC_MOD_FEM_MESH_DATA, arg) < 0) */
/*       fprintf(Interactive_output, "Error in writing mesh data!\n"); */
/*   } */

/*   sprintf(arg, "%s/%s%d.dmp", Work_dir, problem_ns_supg->ctrl.field_dmp_filepattern, problem_ns_supg->time.cur_step); */
/*   if (utr_io_export_field(problem_ns_supg->ctrl.field_id, apr_get_nreq(problem_ns_supg->ctrl.field_id), 0, 0, arg) < 0)	//!TODO its 7 not 0 in supg */

/*   sprintf(arg, "%s/%s%d.dmp", Work_dir, problem_heat->ctrl.field_dmp_filepattern */
/* problem_heat->time.cur_step); */
/*   if (utr_io_export_field(problem_heat->ctrl.field_id, apr_get_nreq(problem_heat->ctrl.field_id), 0, 0, arg) < 0)	//!TODO its 7 not 0 in supg */

  return(0);
}
