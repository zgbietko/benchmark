/************************************************************************
File pds_ns_supg_heat_vof_paraview_io.c - paraview output generation

Contains definition of routines:
  pdr_ns_supg_heat_vof_write_paraview

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
        2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

/* interface for all mesh manipulation modules */
#include "mmh_intf.h" /* USES */
/* interface for all approximation modules */
#include "aph_intf.h" /* USES */
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
#include "../../pdd_vof/include/pdh_vof_problem.h" 
// bc and material header files are included in problem header files


/**************************************/
/* LOCAL TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always with pdt_ */

typedef struct{
  double dofs[PDC_MAXEQ];
}pdt_sol_info;

typedef struct{
  int elNodes[7];
  int numNodes;
  int elType;
}PVElInfo;

typedef struct{
  double dofs[4];
}PVSolInfo;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/// pdr_ns_supg_heat_vof_write_paraview_field - to dump temperature dependent field in Paraview format
/**     \param Mesh_id - id of the mesh associated with given field Field_id
	\param Field_id - id of the field to dump
	\param Filename - c-string with name of the file to write on disk
	\param Desc - c-string c-array with name of the field values
	(at least Desc[0]!= NULL should be passed)
 */

int pdr_ns_supg_heat_vof_write(
  int Field_id, 
  FILE *File,
  int* Dofs_write // dofs to write: Dofs_write[0] - number of dofs
                   //                Dofs_write[i] - IDs of dofs to write
)
{
  int i, j, nrdofs, idofs;
  PVSolInfo * solInfos;
  PVSolInfo * solInfos_VOF;
  PVSolInfo * solInfos_mat;
  int numNodes, max_node_id, numElems, nreq, ino, idof, el_id, nrnodes;
  int mesh_id, el_type;
  int el_nodes[10];
  double el_dofs[APC_MAXELSD];  /* solution dofs in an element */
  pdt_heat_material_query_params qparams;
  pdt_heat_material_query_result qresult;
  //AS: pdt_heat_material_query_result *qresult_patch;
  pdt_ns_supg_material_query_params qparams_ns_supg;
  pdt_ns_supg_material_query_result qresult_ns_supg;
  pdt_vof_material_query_params qparams_vof;
  pdt_vof_material_query_result qresult_vof;
  //AS: pdt_ns_supg_material_query_result *qresult_ns_supg_patch;
  int Field_mat = pdr_ctrl_i_params(PDC_MAT_ID, 3);
  int Field_VOF = pdr_ctrl_i_params(PDC_VOF_ID, 3);
  //int nr_patches;
  //utt_patches *pdv_patches;
  double xcoor[3];

  //qparams_ns_supg.material_idx=0;
  qparams_ns_supg.name = "";

  mesh_id = apr_get_mesh_id(Field_id);
  max_node_id = mmr_get_max_node_id(mesh_id);
  numElems = mmr_get_nr_elem(mesh_id);  
  nreq=apr_get_nreq(Field_id);
  if(nreq>PDC_MAXEQ){
    printf("Number of equations %d > maximum allowed in uts_dump_paraview %d\n",
	   nreq,PDC_MAXEQ);
    printf(" Recompile pdr_ns_supg_heat_vof_write with greater PDC_MAXEQ. Exiting!\n");
    exit(0);
  }

  // dof_entities (nodes) are numbered from 1 to max_node_id
  // Field_id allocation
  solInfos = (PVSolInfo*)malloc((max_node_id+1) * sizeof(PVSolInfo));
  for(ino=0;ino<=max_node_id;ino++){   
    for(idof=0;idof<nreq;idof++){
      solInfos[ino].dofs[idof]=0.0;
    }
  }

  // dof_entities (nodes) are numbered from 1 to max_node_id
  // Field_VOF allocation
  solInfos_VOF = (PVSolInfo*)malloc((max_node_id+1) * sizeof(PVSolInfo));
  for(ino=0;ino<=max_node_id;ino++){   
    for(idof=0;idof<nreq;idof++){
      solInfos_VOF[ino].dofs[idof]=0.0;
    }
  }

  // dof_entities (nodes) are numbered from 1 to max_node_id
  // material field alocation
  solInfos_mat = (PVSolInfo*)malloc((max_node_id+1) * sizeof(PVSolInfo));
  for(ino=0;ino<=max_node_id;ino++){   
    for(idof=0;idof<nreq;idof++){
      solInfos_mat[ino].dofs[idof]=0.0;
    }
  }

  /* loop over elements to read the Field_id */
	el_id=0;
	while((el_id=mmr_get_next_act_elem(mesh_id, el_id))!=0){
		int dof_counter = 0;
		nrnodes = mmr_el_node_coor(mesh_id, el_id, el_nodes, NULL);
		apr_get_el_dofs(Field_id, el_id, 1, el_dofs);
		for(ino=1;ino<=nrnodes;ino++){
			nrdofs=apr_get_ent_nrdofs(Field_id,APC_VERTEX,ino);
			for(idof=0;idof<nreq;idof++){
				int node_id = el_nodes[ino];
				solInfos[node_id].dofs[idof]=el_dofs[dof_counter];
				//Fk-for testing material
				//solInfos[node_id].dofs[idof]=mmr_el_groupID(mesh_id, el_id);
				/*kbw
				printf("node %d, value %.12lg\n",node_id, solInfos[node_id].dofs[idof]);
				/*kew*/
				dof_counter++;
			}
		}
  }

  /* loop over elements to read the Field_VOF */
  nreq=apr_get_nreq(Field_VOF);
	el_id=0;
	while((el_id=mmr_get_next_act_elem(mesh_id, el_id))!=0){
		int dof_counter = 0;
		nrnodes = mmr_el_node_coor(mesh_id, el_id, el_nodes, NULL);
		apr_get_el_dofs(Field_VOF, el_id, 1, el_dofs);
		for(ino=1;ino<=nrnodes;ino++){
			nrdofs=apr_get_ent_nrdofs(Field_VOF,APC_VERTEX,ino);
			for(idof=0;idof<nreq;idof++){
				int node_id = el_nodes[ino];
				solInfos_VOF[node_id].dofs[idof]=el_dofs[dof_counter];
				dof_counter++;
			}
		}
  }

  /* loop over elements to read the material field Field_mat */
  nreq=apr_get_nreq(Field_mat);
  el_id=0;
  while((el_id=mmr_get_next_act_elem(mesh_id, el_id))!=0){
    int dof_counter = 0;
    nrnodes = mmr_el_node_coor(mesh_id, el_id, el_nodes, NULL);
    apr_get_el_dofs(Field_mat, el_id, 1, el_dofs);
    for(ino=1;ino<=nrnodes;ino++){
      nrdofs=apr_get_ent_nrdofs(Field_mat,APC_VERTEX,ino);
      for(idof=0;idof<nreq;idof++){
			int node_id = el_nodes[ino];
			solInfos_mat[node_id].dofs[idof]=el_dofs[dof_counter];
			dof_counter++;
      }
    }
  }

  pdt_vof_problem *problem_vof = (pdt_vof_problem *)pdr_get_problem_structure(PDC_VOF_ID);
  pdt_heat_problem *problem_heat = (pdt_heat_problem *)pdr_get_problem_structure(PDC_HEAT_ID);
  pdt_ns_supg_problem *problem_ns_supg = (pdt_ns_supg_problem *)pdr_get_problem_structure(PDC_NS_SUPG_ID);

  //qparams.material_idx = 0;
  qparams.name = "";

  // create patches for material data query
  // for all fields, the same patch
  //nr_patches = utr_create_patches_small(PDC_NS_SUPG_ID, &pdv_patches);

  // write from heat problem
  fprintf(File, "SCALARS density double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){
    // first cell with node_id
//     qparams.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams.query_type = QUERY_HEAT_NODE;
		//qparams.node_id = i;
      qparams.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_heat_material_query(&problem_heat->materials, &qparams, &qresult);
      fprintf(File,"%.12lg\n",qresult.density);
    }
    fprintf(File,"\n");
//	}
  }

  //qparams.material_idx = 0;

  // write from heat problem
  fprintf(File, "SCALARS thermal_conductivity double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){    
    // first cell with node_id
//     qparams.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams.query_type = QUERY_HEAT_NODE;
		//qparams.node_id = i;
      qparams.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_heat_material_query(&problem_heat->materials, &qparams, &qresult);
      fprintf(File,"%.12lg\n",qresult.thermal_conductivity);
    }
    fprintf(File,"\n");
//	}
  }

  // write from heat problem
  fprintf(File, "SCALARS specific_heat double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){    
    // first cell with node_id
//     qparams.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams.query_type = QUERY_HEAT_NODE;
		//qparams.node_id = i;
      qparams.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_heat_material_query(&problem_heat->materials, &qparams, &qresult);
      fprintf(File,"%.12lg\n",qresult.specific_heat);
    }
    fprintf(File,"\n");
//	}
  }

  // write from heat problem
  fprintf(File, "SCALARS enthalpy double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){    
    // first cell with node_id
//     qparams.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams.query_type = QUERY_HEAT_NODE;
		//qparams.node_id = i;
      qparams.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_heat_material_query(&problem_heat->materials, &qparams, &qresult);
      fprintf(File,"%.12lg\n",qresult.enthalpy);
    }
    fprintf(File,"\n");
//	}
  }

  // write from heat problem
  fprintf(File, "SCALARS thermal_expansion_coefficient double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){    
    // first cell with node_id
//     qparams.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams.query_type = QUERY_HEAT_NODE;
		//qparams.node_id = i;
      qparams.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_heat_material_query(&problem_heat->materials, &qparams, &qresult);
      fprintf(File,"%.12lg\n",qresult.thermal_expansion_coefficient);
    }
    fprintf(File,"\n");
//	}
  }

  // write from heat problem
  fprintf(File, "SCALARS electrical_resistivity double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){    
    // first cell with node_id
//     qparams.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams.query_type = QUERY_HEAT_NODE;
		//qparams.node_id = i;
      qparams.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_heat_material_query(&problem_heat->materials, &qparams, &qresult);
      fprintf(File,"%.12lg\n",qresult.electrical_resistivity);
    }
    fprintf(File,"\n");
//	}
  }

  // write VOF material data from heat problem
  fprintf(File, "SCALARS VOF(md) double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){    
    // first cell with node_id
//     qparams.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams.query_type = QUERY_HEAT_NODE;
		//qparams.node_id = i;
      qparams.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_heat_material_query(&problem_heat->materials, &qparams, &qresult);
      fprintf(File,"%.12lg\n",qresult.VOF);
    }
    fprintf(File,"\n");
//	}
  }

  // write from ns_supg problem
  fprintf(File, "SCALARS dynamic_viscosity double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){
    // first cell with node_id
//     qparams_ns_supg.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams_ns_supg.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams_ns_supg.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams_ns_supg.query_type = QUERY_NS_SUPG_NODE;
		//qparams_ns_supg.node_id = i;
      qparams_ns_supg.aux = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_ns_supg_material_query(&problem_ns_supg->materials, &qparams_ns_supg, &qresult_ns_supg);
      fprintf(File,"%.12lg\n",qresult_ns_supg.dynamic_viscosity);
    }
    fprintf(File,"\n");
//	}
  }

  // write sulfur content from vof problem
  fprintf(File, "SCALARS sulfur double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){
    // first cell with node_id
//     qparams_vof.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams_vof.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams_vof.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams_vof.query_type = QUERY_VOF_NODE;
      qparams_vof.VOF_fd = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_vof_material_query(&problem_vof->materials, &qparams_vof, &qresult_vof);
      fprintf(File,"%.12lg\n",qresult_vof.av_comp);
    }
    fprintf(File,"\n");
//   }
  }

  // write from vof problem
  fprintf(File, "SCALARS gamma double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){
    // first cell with node_id
//     qparams_vof.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams_vof.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams_vof.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams_vof.query_type = QUERY_VOF_NODE;
      qparams_vof.VOF_fd = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_vof_material_query(&problem_vof->materials, &qparams_vof, &qresult_vof);
      fprintf(File,"%.12lg\n",qresult_vof.gamma);
    }
    fprintf(File,"\n");
//   }
  }

  // write from vof problem
  fprintf(File, "SCALARS dg_dT double 1\n");
  fprintf(File, "LOOKUP_TABLE default\n");
  for(i=1;i<=max_node_id;i++){
//     qparams_vof.cell_id = pdv_patches[i].elems[0];
//     mmr_node_coor(mesh_id, i, xcoor);
//     for( j=0; j<3; j++ ){
//       qparams_vof.xg[j] = xcoor[j];
//     }
//	if ( apr_get_ent_pdeg(PDC_VOF_ID, APC_VERTEX, i) > 0 ) {
    for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
      qparams_vof.temperature = solInfos[i].dofs[Dofs_write[idofs]];
		qparams_vof.query_type = QUERY_VOF_NODE;
      qparams_vof.VOF_fd = solInfos_VOF[i].dofs[Dofs_write[idofs]];
      pdr_vof_material_query(&problem_vof->materials, &qparams_vof, &qresult_vof);
      fprintf(File,"%.12lg\n",qresult_vof.dg_dT);
    }
    fprintf(File,"\n");
//   }
  }

  // write from mesh
//   fprintf(File, "SCALARS material_idx double 1\n");
//   fprintf(File, "LOOKUP_TABLE default\n");
//   for(i=1;i<=max_node_id;i++) {
//     for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
//       fprintf(File,"%d\n", mmr_el_groupID(mesh_id, i));
//     }
//     fprintf(File,"\n");
//   }

  //free(pdv_patches);
  free(solInfos);
  free(solInfos_VOF);
  free(solInfos_mat);

  return 0;
}

/*---------------------------------------------------------
pdr_ns_supg_heat_vof_write_paraview - write mesh in ParaView format
---------------------------------------------------------*/
int pdr_ns_supg_heat_vof_write_paraview(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
)
{
  FILE *fp;
  int i;
  int num_nodes;
  int num_elems;
  double node_coor[3];
  int num_conn = 0;
  int mesh_id, field_id;
  char filename[300];
  int dofs_write[PDC_MAXEQ+1];

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* open the output file */
  sprintf(filename, "%s/%s%d.vtk", Work_dir, "ns_supg_heat_vof_",
	  pdv_ns_supg_problem.time.cur_step);  
  fp = fopen(filename, "w");
  if (fp == NULL) {
      fprintf(Interactive_output, "Cannot open file '%s' for Paraview mesh data\n", filename);
      return (-1);
  } else {
    fprintf(Interactive_output, "Opened file '%s' for Paraview mesh data\n", filename);
  }

  mesh_id = pdr_ctrl_i_params(PDC_NS_SUPG_ID, 2);

/*  fprintf(fp, "# vtk DataFile Version 2.0\n");
  fprintf(fp, "Navier-Stokes with Heat convection: pdd_ns_supg_heat\n");
  fprintf(fp, "ASCII\n");
*/
  utr_write_paraview_mesh(mesh_id, fp);

  // write ns_supg pressure field data
  int max_node_id = mmr_get_max_node_id(mesh_id);
  fprintf(fp, "POINT_DATA %d\n", max_node_id);
  fprintf(fp, "SCALARS pressure double 1\n");
  fprintf(fp, "LOOKUP_TABLE default\n");
  dofs_write[0]=1;
  dofs_write[1]=3; // pressure is the fourth component (3rd counting from 0)
  utr_write_paraview_field(pdv_ns_supg_problem.ctrl.field_id, 
			   fp, dofs_write);
  
  // write ns_supg velocity field data
  fprintf(fp, "VECTORS velocity double\n");
  dofs_write[0]=3;
  dofs_write[1]=0;dofs_write[2]=1;dofs_write[3]=2; // velocity components
  
  utr_write_paraview_field(pdv_ns_supg_problem.ctrl.field_id, 
			   fp, dofs_write);
  
  // write temperature field data
  fprintf(fp, "SCALARS temperature double 1\n");
  fprintf(fp,"LOOKUP_TABLE default\n");
  dofs_write[0]=1;
  dofs_write[1]=0; // temperature is the only component
  utr_write_paraview_field(pdv_heat_problem.ctrl.field_id, 
			   fp, dofs_write);

  // write heating-cooling rate
  fprintf(fp, "SCALARS dT_dt double 1\n");
  fprintf(fp, "LOOKUP_TABLE default\n");
  dofs_write[0]=1;
  dofs_write[1]=1; // heating-cooling rate is the only component
  utr_write_paraview_field(pdv_heat_dtdt_problem.ctrl.field_id, 
			   fp, dofs_write);
  
  // write VOF Field Data
  fprintf(fp, "SCALARS VOF double 1\n");
  fprintf(fp,"LOOKUP_TABLE default\n");

  dofs_write[0]=1;
  dofs_write[1]=0; // vof is the only component
  
  utr_write_paraview_field(pdv_vof_problem.ctrl.field_id, 
			   fp, dofs_write);

  // write mat field data
  fprintf(fp, "SCALARS G double 1\n");
  fprintf(fp,"LOOKUP_TABLE default\n");

  dofs_write[0]=1;
  dofs_write[1]=0; // mat is the only component
  
  utr_write_paraview_field(pdv_mat_problem.ctrl.field_id, 
			   fp, dofs_write);


  // other options directly using field data structures as in 
  // utr_write_paraview_field
  // pdr_ns_supg_heat_write writes temperature dependent fields:
  // density, 

  // temperature dependent coefficients
  dofs_write[0]=1;
  dofs_write[1]=0; // temperature is the only component
  pdr_ns_supg_heat_vof_write(pdv_heat_problem.ctrl.field_id, fp, dofs_write);

  // write latent heat of fusion factor
  /* fprintf(fp, "SCALARS latent_heat_factor double 1\n"); */
  /* fprintf(fp, "LOOKUP_TABLE default\n"); */
  /* dofs_write[0]=1; */
  /* dofs_write[1]=0; // fusion factor is the only component */
  /* utr_write_paraview_field(pdv_heat_dtdt_problem.ctrl.field_id,  */
  /* 			   fp, dofs_write); */

  fclose(fp);
  
  return 0;
}

/* THE CODE BELOW IS LEFT AS AN EXAMPLE FOR EXPLICIT WRITING OF PARAVIEW FILES */
/* ITS PARTS CAN BE ADAPTED TO THE PROBLEM SOLVED AND MOVED ABOVE */ 

/*---------------------------------------------------------
pdr_paraview_write_field - write field in ParaView format
---------------------------------------------------------*/
/* int pdr_paraview_write_field(int Problem_id, int Field_id, char *Filename) */
/* { */
/*     FILE *fp; */
/*     int i; */
/*     int mesh_id; */
/*     pdt_sol_info *sol_infos; */
/*     int num_nodes, *check; */
/*     /\* material data (queries) *\/ */
/*     pdt_material_query_params qparams; */
/*     pdt_material_query_result qresult; */
/*     /\* material data *\/ */
/*     double viscosity, tconductivity, specific_heat, texpansion, density, enthalpy, eresistivity, VOF; */
          

/*   pdt_problem *problem = &pdv_problems[Problem_id]; */
/*   pdt_ctrls *ctrls = &pdv_problems[Problem_id].ctrl; */

/* /\* open the output file *\/ */
/*     fp = fopen(Filename, "w"); */
/*     if (fp == NULL) { */
/* 	fprintf(ctrls->interactive_output, "Cannot open file '%s' for Paraview field data\n", Filename); */
/* 	return (-1); */
/*     } */

/*     mesh_id = apr_get_mesh_id(Field_id); */
/*     //num_nodes = mmr_mesh_i_params(mesh_id, 1); */
/*     num_nodes = mmr_get_nr_node(mesh_id); */
/*     sol_infos = (pdt_sol_info *) malloc((num_nodes + 1) * sizeof(pdt_sol_info)); */
/*     check = (int *) malloc((num_nodes + 1) * sizeof(int)); */

/*     for (i = 1; i <= num_nodes; i++) { */
/* 	check[i] = apr_read_ent_dofs(Field_id, APC_VERTEX, i, 5, 1, sol_infos[i].dofs); */
/*     } */

/*     fprintf(fp, "POINT_DATA %d\n", num_nodes); */
/*     fprintf(fp, "SCALARS pressure double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/* 	if (check[i] != 2) */
/* 	    fprintf(fp, "%.12lg\n", sol_infos[i].dofs[3]); */
/*     } */
    
/*     fprintf(fp, "SCALARS temperature double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2) */
/*             fprintf(fp, "%.12lg\n", sol_infos[i].dofs[4]); */
/*     }  */

/*     //1.set query parameters (which material and temperature) */
/*     qparams.material_idx = 0;   //query by material index ... */
/*     qparams.name = "";          //... not by material name //TODO: get material by mesh coordinates */

/*     fprintf(fp, "SCALARS density double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             density = qresult.density; */
/*             fprintf(fp, "%.12lg\n", density); */
/* 	} */
/*     } */

/*     fprintf(fp, "SCALARS viscosity double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             viscosity = qresult.viscosity; */
/*             fprintf(fp, "%.12lg\n", viscosity); */
/* 	} */
/*     } */

/*     fprintf(fp, "SCALARS thermal_conductivity double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             tconductivity = qresult.thermal_conductivity; */
/*             fprintf(fp, "%.12lg\n", tconductivity); */
/* 	} */
/*     } */

/*     fprintf(fp, "SCALARS specific_heat double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             specific_heat = qresult.specific_heat; */
/*             fprintf(fp, "%.12lg\n", specific_heat); */
/* 	} */
/*     } */

/*     fprintf(fp, "SCALARS thermal_expansion_coefficient double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             texpansion = qresult.thermal_expansion_coefficient; */
/*             fprintf(fp, "%.12lg\n", texpansion); */
/* 	} */
/*     } */

/*     fprintf(fp, "SCALARS enthalpy double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             enthalpy = qresult.enthalpy; */
/*             fprintf(fp, "%.12lg\n", enthalpy); */
/* 	} */
/*     } */

/*     fprintf(fp, "SCALARS electrical_resistivity double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             eresistivity = qresult.electrical_resistivity; */
/*             fprintf(fp, "%.12lg\n", eresistivity); */
/* 	} */
/*     } */

/*     fprintf(fp, "SCALARS VOF double 1\n"); */
/*     fprintf(fp, "LOOKUP_TABLE default\n"); */

/*     for (i = 1; i <= num_nodes; i++) { */
/*         if (check[i] != 2){ */
/*             qparams.temperature = sol_infos[i].dofs[4]; //temperature from last iteration */
/*             //2.get query results */
/*             pdr_material_query(&problem->materials, &qparams, &qresult); */
/*             //3.set values to those obtained with query */
/*             VOF = qresult.VOF; */
/*             fprintf(fp, "%.12lg\n", VOF); */
/* 	} */
/*     } */

/*     fprintf(fp, "VECTORS velocity double\n"); */
/*     for (i = 1; i <= num_nodes; i++) { */
/* 	if (check[i] != 2) */
/* 	    fprintf(fp, "%.12lg %.12lg %.12lg\n", sol_infos[i].dofs[0], sol_infos[i].dofs[1], sol_infos[i].dofs[2]); */
/*     } */

/*     fclose(fp); */

/*     free(sol_infos); */
/*     free(check); */

/*     return 0; */
/* } */
