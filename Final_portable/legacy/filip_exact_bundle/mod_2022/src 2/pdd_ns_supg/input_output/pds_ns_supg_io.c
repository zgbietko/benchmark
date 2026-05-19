/************************************************************************
File pds_ns_supg_problem_io.c - problem data read

Contains definition of routines:
  pdr_ns_supg_dump_data - to write mesh and field data to files 

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
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
#include "pdh_control_intf.h"	/* IMPLEMENTS */

/* problem module's types and functions */
#include "../include/pdh_ns_supg.h" /* USES & IMPLEMENTS */
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
// bc and material header files are included in problem header files


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_ns_supg_dump_data
------------------------------------------------------------*/
int pdr_ns_supg_dump_data(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
)
{
  int mesh_id, field_id, iaux;
  char filename[300];

  // different files for different processors, different time steps

  // temporary sequential version
  mesh_id = pdr_ctrl_i_params(PDC_NS_SUPG_ID, 2);

  sprintf(filename, "%s/%s%d.dmp", Work_dir, 
	  pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern, 
	  pdv_ns_supg_problem.time.cur_step);  

  iaux = utr_io_export_mesh(Interactive_output,mesh_id, MMC_MOD_FEM_MESH_DATA, filename);
  if(iaux<0) {
    /* if error in writing data */
    fprintf(Interactive_output,"Error in writing mesh data!\n");
  }


  field_id = pdv_ns_supg_problem.ctrl.field_id;
  int nreq = pdv_ns_supg_problem.ctrl.nreq;

  sprintf(filename, "%s/%s%d.dmp", Work_dir, 
	  pdv_ns_supg_problem.ctrl.field_dmp_filepattern, 
	  pdv_ns_supg_problem.time.cur_step);  

  iaux = utr_io_export_field(field_id, nreq, 0, 0, filename);
  if(iaux<0) {
    /* if error in writing data */
    fprintf(Interactive_output,"Error in writing field data!\n");
  }

  /* sprintf(arg, "%s/%s.dmp", Work_dir, pdv_ns_supg_problem.ctrl.mesh_dmp_filepattern); */
  /* if (0 == strcmp(pdv_ns_supg_problem.ctrl.mesh_type, "j")) { */
  /*   if (mmr_export_mesh(pdv_ns_supg_problem.ctrl.mesh_id, MMC_MOD_FEM_MESH_DATA, arg) < 0) */
  /*     fprintf(Interactive_output, "Error in writing mesh data!\n"); */
  /* } else if (0 == strcmp(pdv_ns_supg_problem.ctrl.mesh_type, "t")) { */
  /*   if (mmr_export_mesh(pdv_ns_supg_problem.ctrl.mesh_id, MMC_MOD_FEM_TETRA_DATA, arg) < 0) */
  /*     fprintf(Interactive_output, "Error in writing mesh data!\n"); */
  /* } else if (0 == strcmp(pdv_ns_supg_problem.ctrl.mesh_type, "h")) { */
  /*   if (mmr_export_mesh(pdv_ns_supg_problem.ctrl.mesh_id, MMC_MOD_FEM_HYBRID_DATA, arg) < 0) */
  /*     fprintf(Interactive_output, "Error in writing mesh data!\n"); */
  /* } else if (0 == strcmp(pdv_ns_supg_problem.ctrl.mesh_type, "s")) { */
  /*   if (mmr_export_mesh(pdv_ns_supg_problem.ctrl.mesh_id, MMC_MOD_FEM_MESH_DATA, arg) < 0) */
  /*     fprintf(Interactive_output, "Error in writing mesh data!\n"); */
  /* } else if (0 == strcmp(pdv_ns_supg_problem.ctrl.mesh_type, "n")) { */
  /*   if (mmr_export_mesh(pdv_ns_supg_problem.ctrl.mesh_id, MMC_NASTRAN_DATA, arg) < 0) */
  /*     fprintf(Interactive_output, "Error in writing mesh data!\n"); */
  /* } else { */
  /*   if (mmr_export_mesh(pdv_ns_supg_problem.ctrl.mesh_id, MMC_MOD_FEM_MESH_DATA, arg) < 0) */
  /*     fprintf(Interactive_output, "Error in writing mesh data!\n"); */
  /* } */

  /* sprintf(arg, "%s/%s.dmp", Work_dir, pdv_ns_supg_problem.ctrl.field_dmp_filepattern); */
  /* if (apr_write_field(pdv_ns_supg_problem.ctrl.field_id, apr_get_nreq(pdv_ns_supg_problem.ctrl.field_id), 0, sigdigs, arg) < 0)	//!TODO its 7 not 0 in supg */

  return(iaux);
}

