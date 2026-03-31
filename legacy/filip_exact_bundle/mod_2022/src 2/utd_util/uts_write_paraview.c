/************************************************************************
File uts_write_paraview.c - paraview output generation

Contains definition of routines:
  utr_write_paraview_mesh - to dump mesh in Paraview format 
  utr_write_paraview_partmesh - to dump mesh in Paraview format includiong partition info
  utr_write_paraview_field - to dump field in Paraview format

------------------------------
History:
	08.2012 - Kamil Wachala, kamil.wachala@gmail.com
	05.2011 - Kazimierz Michalik, kamich@agh.edu.pl, initial version
	06.2011 - KGB, pobanas@cyf-kr.edu.pl 
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

#include "pdh_intf.h"
#include "mmh_intf.h"
#include "aph_intf.h"
#include "uth_intf.h"

typedef struct{
  double dofs[PDC_MAXEQ];
}PVSolInfo;

/*---------------------------------------------------------
/// utr_write_paraview_mesh - to dump mesh in Paraview format
---------------------------------------------------------*/
int utr_write_paraview_mesh(int Mesh_id, FILE  *File)
{
	utr_write_paraview_partmesh(Mesh_id, File, NULL);
	return 0;
}

/*---------------------------------------------------------
/// utr_write_paraview_partmesh - to dump mesh in Paraview format includiong partition info
---------------------------------------------------------*/
void utr_write_paraview_partmesh(int Mesh_id, FILE  *File, int *Part)
{
  int i,el_id=0, el_type;
  int max_node_id, ino;
  int numElems;
  double nodeCoor[3];
  int numConn = 0;
  int el_nodes[10];
  
/*++++++++++++++++ executable statements ++++++++++++++++*/

  max_node_id = mmr_get_max_node_id(Mesh_id);
  numElems = mmr_get_nr_elem(Mesh_id);
 
	fprintf(File,"# vtk DataFile Version 3.1\n");
	fprintf(File,"crated by MODFEM\n");
	fprintf(File,"ASCII\n");
	fprintf(File,"DATASET UNSTRUCTURED_GRID\n");
	fprintf(File,"POINTS %d double\n",max_node_id);

  /* loop over vertices-nodes */
  for(ino=1; ino<=max_node_id; ino++){
	if(mmr_node_status(Mesh_id,ino)==MMC_ACTIVE){
	  mmr_node_coor(Mesh_id, ino, nodeCoor);
	  fprintf(File,"%.12lg %.12lg %.12lg\n",nodeCoor[0], nodeCoor[1], nodeCoor[2]);
	}
	else{
	  fprintf(File,"0.0 0.0 0.0\n");
	}
  }

  /* loop over elements */
  el_id=0;
  while((el_id=mmr_get_next_act_elem(Mesh_id, el_id))!=0){
	numConn += mmr_el_node_coor(Mesh_id, el_id, NULL, NULL);
  }
  
  fprintf(File,"CELLS %d %d\n",numElems, numConn+numElems);

  /* loop over elements */
  el_id=0;
  while((el_id=mmr_get_next_act_elem(Mesh_id, el_id))!=0){
	mmr_el_node_coor(Mesh_id, el_id, el_nodes, NULL);
	if(el_nodes[0] == 6){ // PRISM
	  fprintf(File,"6 %d %d %d %d %d %d\n", 
		  el_nodes[1]-1, el_nodes[2]-1, el_nodes[3]-1, 
		  el_nodes[4]-1, el_nodes[5]-1, el_nodes[6]-1);
	}
	else if(el_nodes[0] == 4){ //TETRA
	  fprintf(File,"4 %d %d %d %d\n", el_nodes[1]-1, el_nodes[2]-1, 
		  el_nodes[3]-1, el_nodes[4]-1);
	}
	else{
	  fprintf(File, "ERROR: WRONG ELEMENT TYPE");
	}
  }
  
  fprintf(File,"CELL_TYPES %d\n",numElems);
  el_id=0;
  while((el_id=mmr_get_next_act_elem(Mesh_id, el_id))!=0){
	el_type = mmr_el_type(Mesh_id, el_id);
	if(el_type==MMC_PRISM) fprintf(File,"13\n"); // PRISM
	if(el_type==MMC_TETRA) fprintf(File,"10\n"); // TETRA
  }
  
	if(Part != NULL)
	{	  
		fprintf(File,"CELL_DATA %d\n", numElems);
		fprintf(File,"SCALARS ProcessorsDistribuation float\n");
		fprintf(File,"LOOKUP_TABLE default\n");
	  
		for(i=0; i<numElems; i++)
		{
				fprintf(File,"%d\n",Part[i]);
		}
	}
}

/*---------------------------------------------------------
/// utr_write_paraview_field - to dump field in Paraview format
---------------------------------------------------------*/
int utr_write_paraview_field(
  int Field_id, 
  FILE *File,
  int* Dofs_write // dofs to write: dofs_write[0] - number of dofs
				   //                dofs_write[i] - IDs of dofs to write
)
{

  int i,nrdofs, idofs;
  PVSolInfo * solInfos;
  int numNodes, max_node_id, numElems, nreq, ino, idof, el_id, nrnodes;
  int mesh_id, el_type;
  int el_nodes[10];
  double el_dofs[APC_MAXELSD];  /* solution dofs in an element */

/*++++++++++++++++ executable statements ++++++++++++++++*/

  mesh_id = apr_get_mesh_id(Field_id);  

  max_node_id = mmr_get_max_node_id(mesh_id);
  numElems = mmr_get_nr_elem(mesh_id);
  
  nreq=apr_get_nreq(Field_id);
  if(nreq>PDC_MAXEQ){
	printf("Number of equations %d > maximum allowed in uts_dump_paraview %d\n",
	   nreq,PDC_MAXEQ);
	printf(" Recompile uts_write_paraview with greater PDC_MAXEQ. Exiting!\n");
	exit(0);
  }

  // dof_entities (nodes) are numbered from 1 to max_node_id
  solInfos = (PVSolInfo*)malloc((max_node_id+1) * sizeof(PVSolInfo));
  for(ino=0;ino<=max_node_id;ino++){   
	for(idof=0;idof<nreq;idof++){
	  solInfos[ino].dofs[idof]=0.0;
	}
  }

  /* loop over elements */
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
	printf("node %d, value %.12lg\n",node_id, solInfos[node_id].dofs[0]);
/*kew*/
	dof_counter++;
	  }
	}
  }
  
  for(i=1;i<=max_node_id;i++){
	
	for(idofs = 1; idofs <= Dofs_write[0]; idofs++){
	  fprintf(File,"%.12lg\n",solInfos[i].dofs[Dofs_write[idofs]]);
	}
	fprintf(File,"\n");
  }

  free(solInfos);
  
  return 0;
}



/*---------------------------------------------------------
/// utr_write_paraview_bc - to dump boundary conditions 'field' in Paraview format
---------------------------------------------------------*/
int utr_write_paraview_bc(
  int Mesh_id, 
  FILE *File
)
{

  int i,faceId,bc=0;
  int * nodeBCs;
  int numNodes, max_node_id, numFaces;
  int fNodes[5]={0};

  double el_dofs[APC_MAXELSD];  /* solution dofs in an element */

/*++++++++++++++++ executable statements ++++++++++++++++*/

  max_node_id = mmr_get_max_node_id(Mesh_id);
  numFaces = mmr_get_nr_face(Mesh_id);

  // dof_entities (nodes) are numbered from 1 to max_node_id
  nodeBCs = (int*)malloc((max_node_id+1) * sizeof(int));
  memset(nodeBCs,0,(max_node_id+1)* sizeof(int));
  // gather bc info for nodes
  faceId=0;
  while(faceId=mmr_get_next_face_all(Mesh_id,faceId)) {
	bc=mmr_fa_bc(Mesh_id,faceId);
	if(bc != 0) {
		mmr_fa_node_coor(Mesh_id,faceId,fNodes,NULL);
		for(i=1; i <= fNodes[0]; ++i) {
			if(nodeBCs[fNodes[i]] < bc) {
				nodeBCs[fNodes[i]] = bc;
			}
		}
	}
  }

  for(i=1;i<=max_node_id;i++){
	fprintf(File,"%d\n\n",nodeBCs[i]);
  }

  free(nodeBCs);
  
  return 0;
}


/******************* OLD INTERFACE *********************************/


/// \file uts_dump_paraview
/// \brief generic routines for dumping mesh and/or field into .vtu files (ParaView format).
///
/// int utr_write_paraview_mesh - to dump mesh in Paraview format 
/// int utr_write_paraview_field - to dump field in Paraview format
///-----------------------------------------------
/// History:
/// 05.2011 - Kazimierz Michalik, kamich@agh.edu.pl, initial version
/// 06.2011 - KGB, pobanas@cyf-kr.edu.pl - utr_write_paraview_std_lin



// utr_write_paraview_std_lin - to dump std_lin mesh and field in Paraview format
/** \param Mesh_id - id of the mesh associated with given field Field_id
	\param Field_id - id of the field to dump
	\param Filename - c-string with name of the file to write on disk
	\param Desc - c-string c-array with name of the field values
	(at least Desc[0]!= NULL should be passed)
*/
int utr_write_paraview_std_lin(int Mesh_id, int Field_id, char *Filename)
{
  FILE *fp;
  int i,ino, el_type, el_id, nrnodes, nreq, nrdofs, idof; //, node_id
  int numElems, max_node_id;
  double nodeCoor[3];
  int numConn = 0;
  int el_nodes[10];
  
  PVSolInfo * solInfos;
  double el_dofs[APC_MAXELSD];  /* solution dofs in an element */

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* open the output file */
  fp = fopen(Filename, "w");
  if(fp==NULL) {
	printf("Cannot open file '%s' for Paraview mesh data\n",Filename);
	return(-1);
  } 

  max_node_id = mmr_get_max_node_id(Mesh_id);
  numElems = mmr_get_nr_elem(Mesh_id);
  
  fprintf(fp,"# vtk DataFile Version 2.0\n");
  fprintf(fp,"Navier-Stokes: pdd_navstokes\n");
  fprintf(fp,"ASCII\n");
  fprintf(fp,"DATASET UNSTRUCTURED_GRID\n");
  fprintf(fp,"POINTS %d double\n",max_node_id);

  /* loop over vertices-nodes */
  for(ino=1; ino<=max_node_id; ino++){
	if(mmr_node_status(Mesh_id,ino)==MMC_ACTIVE){
	  mmr_node_coor(Mesh_id, ino, nodeCoor);
	  fprintf(fp,"%.12lg %.12lg %.12lg\n",nodeCoor[0], nodeCoor[1], nodeCoor[2]);
	}
	else{
	  fprintf(fp,"0.0 0.0 0.0\n");
	}
  }

  /* loop over elements */
  el_id=0;
  while((el_id=mmr_get_next_act_elem(Mesh_id, el_id))!=0){
	numConn += mmr_el_node_coor(Mesh_id, el_id, NULL, NULL);
  }
  
  fprintf(fp,"CELLS %d %d\n",numElems, numConn+numElems);

  /* loop over elements */
  el_id=0;
  while((el_id=mmr_get_next_act_elem(Mesh_id, el_id))!=0){
	mmr_el_node_coor(Mesh_id, el_id, el_nodes, NULL);
	if(el_nodes[0] == 6){ // PRISM
	  fprintf(fp,"6 %d %d %d %d %d %d\n", 
		  el_nodes[1]-1, el_nodes[2]-1, el_nodes[3]-1, 
		  el_nodes[4]-1, el_nodes[5]-1, el_nodes[6]-1);
	}
	else if(el_nodes[0] == 4){ //TETRA
	  fprintf(fp,"4 %d %d %d %d\n", el_nodes[1]-1, el_nodes[2]-1, 
		  el_nodes[3]-1, el_nodes[4]-1);
	}
	else{
	  fprintf(fp, "ERROR: WRONG ELEMENT TYPE");
	}
  }
  
  fprintf(fp,"CELL_TYPES %d\n",numElems);
  el_id=0;
  while((el_id=mmr_get_next_act_elem(Mesh_id, el_id))!=0){
	el_type = mmr_el_type(Mesh_id, el_id);
	if(el_type==MMC_PRISM) fprintf(fp,"13\n"); // PRISM
	if(el_type==MMC_TETRA) fprintf(fp,"10\n"); // TETRA
  }
  
  /* writing field data */

  nreq=apr_get_nreq(Field_id);
  if(nreq>PDC_MAXEQ){
	printf("Number of equations %d > maximum allowed in uts_dump_paraview %d\n",
	   nreq,PDC_MAXEQ);
	printf(" Recompile uts_dump_paraview with greater PDC_MAXEQ. Exiting!\n");
	exit(0);
  }

  solInfos = (PVSolInfo*)malloc((max_node_id+1) * sizeof(PVSolInfo));
  for(ino=0;ino<=max_node_id;ino++){   
	for(idof=0;idof<nreq;idof++){
	  solInfos[ino].dofs[idof]=0.0;
	}
  }

  /* loop over elements */
  el_id=0;
  while((el_id=mmr_get_next_act_elem(Mesh_id, el_id))!=0){
	int dof_counter = 0;
	nrnodes = mmr_el_node_coor(Mesh_id, el_id, el_nodes, NULL);
	apr_get_el_dofs(Field_id, el_id, 1, el_dofs);
	for(ino=1;ino<=nrnodes;ino++){
	  nrdofs=apr_get_ent_nrdofs(Field_id,APC_VERTEX,ino);
	  for(idof=0;idof<nreq;idof++){
	int node_id = el_nodes[ino];
	solInfos[node_id].dofs[idof]=el_dofs[dof_counter];
	//Fk-for testing material
	//solInfos[node_id].dofs[idof]=mmr_el_groupID(Mesh_id, el_id);
/*kbw
	printf("node %d, value %.12lg\n",node_id, solInfos[node_id].dofs[0]);
/*kew*/
	dof_counter++;
	  }
	}
  }
  
  // NASTY HACKS - SHOULD BE MUCH MORE INPUT CONTROL PARAMETERS
  
  fprintf(fp,"POINT_DATA %d\n", max_node_id);
  
  if(nreq==1){ // Laplace
	
	fprintf(fp, "SCALARS field_value double 1\n");
	fprintf(fp,"LOOKUP_TABLE default\n");
	
	for(i=1;i<=max_node_id;i++){
	  fprintf(fp,"%.12lg\n",solInfos[i].dofs[0]);
/*kbw
	  printf("node %d, value %.12lg\n",i, solInfos[i].dofs[0]);
/*kew*/
	}
	
  }
  else if(nreq==3){ // elasticity
	
	fprintf(fp, "VECTORS displacement double\n");
	for(i=1;i<=max_node_id;i++){
	  fprintf(fp,"%.12lg %.12lg %.12lg\n",
		  solInfos[i].dofs[0], solInfos[i].dofs[1], solInfos[i].dofs[2]);
	}

  }
  else if(nreq==4){ // NS
	
	fprintf(fp, "SCALARS pressure double 1\n");
	fprintf(fp,"LOOKUP_TABLE default\n");
	for(i=1;i<=max_node_id;i++){
	  fprintf(fp,"%.12lg\n",solInfos[i].dofs[nreq-1]);
	}
	
	fprintf(fp, "VECTORS velocity double\n");
	for(i=1;i<=max_node_id;i++){
	  fprintf(fp,"%.12lg %.12lg %.12lg\n",
		  solInfos[i].dofs[0], solInfos[i].dofs[1], solInfos[i].dofs[2]);
	}
	
  }
  else if(nreq==5){ // NS+THERMO
	
	fprintf(fp, "SCALARS pressure double 1\n");
	fprintf(fp,"LOOKUP_TABLE default\n");
	for(i=1;i<=max_node_id;i++){
	  fprintf(fp,"%.12lg\n",solInfos[i].dofs[nreq-2]);
	}
	
	fprintf(fp, "VECTORS velocity double\n");
	for(i=1;i<=max_node_id;i++){
	  fprintf(fp,"%.12lg %.12lg %.12lg\n",
		  solInfos[i].dofs[0], solInfos[i].dofs[1], solInfos[i].dofs[2]);
	}
	
	fprintf(fp, "SCALARS temperature double 1\n");
	fprintf(fp,"LOOKUP_TABLE default\n");
	for(i=1;i<=max_node_id;i++){
	  fprintf(fp,"%.12lg\n",solInfos[i].dofs[nreq-1]);
	}
	
  }
  else{
	
	printf("write_Paraview too stupid to handle %d solution vector components\n",
	   nreq);
	
  }
  
  fclose(fp);
  
  free(solInfos);

  return 0;
}


typedef struct{
  int elNodes[7];
  int numNodes;
  int elType;
}PVElInfo;

/// utr_write_paraview_mesh - to dump mesh in Paraview format
/** \param Mesh_id - id of the mesh to dump
	\param Filename - c-string with name of the file to write(dump) on disk
*/
int utr_write_paraview_mesh_old(int Mesh_id, char *Filename)
{
  FILE *fp;
  int i,el_id=0;
  PVElInfo * elInfos;
  int numNodes;
  int numElems;
  double nodeCoor[3];
  int numConn = 0;
  

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* open the output file */
  fp = fopen(Filename, "w");
  if(fp==NULL) {
	printf("Cannot open file '%s' for Paraview mesh data\n",Filename);
	return(-1);
  } 

 
  numNodes = mmr_get_nr_node(Mesh_id);
  numElems = mmr_get_nr_elem(Mesh_id);
  elInfos = (PVElInfo*)malloc((numElems+1) * sizeof(PVElInfo));
  
  
  
  fprintf(fp,"# vtk DataFile Version 2.0\n");
  fprintf(fp,"Navier-Stokes: pdd_navstokes\n");
  fprintf(fp,"ASCII\n");
  fprintf(fp,"DATASET UNSTRUCTURED_GRID\n");
  fprintf(fp,"POINTS %d double\n",numNodes);
  for(i=1;i<=numNodes;i++){
	mmr_node_coor(Mesh_id, i, nodeCoor);
	fprintf(fp,"%.12lg %.12lg %.12lg\n",nodeCoor[0], nodeCoor[1], nodeCoor[2]);
  }
  for(i=1,el_id=0;(el_id=mmr_get_next_act_elem(Mesh_id,el_id)) > 0;i++){
	mmr_el_node_coor(Mesh_id, el_id, elInfos[i].elNodes, NULL);
	//apr_get_el_dofs(Field_id, i, 1, elInfos[i].solDofs);
	elInfos[i].numNodes = elInfos[i].elNodes[0];
	if(elInfos[i].numNodes == 6)
	  elInfos[i].elType = 13;
	else if(elInfos[i].numNodes == 4)
	  elInfos[i].elType = 10;
	else{
	  elInfos[i].elType = -1;
	}
	numConn += elInfos[i].numNodes;
  }
  
  fprintf(fp,"CELLS %d %d\n",numElems, numConn+numElems);
  
  for(i=1;i<=numElems;i++){
	  if(elInfos[i].elType == 13) //PRISM
	fprintf(fp,"6 %d %d %d %d %d %d\n", elInfos[i].elNodes[1]-1, elInfos[i].elNodes[2]-1, elInfos[i].elNodes[3]-1, elInfos[i].elNodes[4]-1, elInfos[i].elNodes[5]-1, elInfos[i].elNodes[6]-1);
	  else if(elInfos[i].elType == 10) //TETRA
	fprintf(fp,"4 %d %d %d %d\n", elInfos[i].elNodes[1]-1, elInfos[i].elNodes[2]-1, elInfos[i].elNodes[3]-1, elInfos[i].elNodes[4]-1);
	  else
	fprintf(fp, "ERROR: WRONG ELEMENT TYPE");
  }
  
  fprintf(fp,"CELL_TYPES %d\n",numElems);
  for(i=1;i<=numElems;i++){
	fprintf(fp,"%d\n", elInfos[i].elType);
  }
  
  fclose(fp);
  free(elInfos);

  return 0;
}

/// utr_write_paraview_field - to dump field in Paraview format
/** \param Mesh_id - id of the mesh associated with given field Field_id
	\param Field_id - id of the field to dump
	\param Filename - c-string with name of the file to write on disk
	\param Desc - c-string c-array with name of the field values
	(at least Desc[0]!= NULL should be passed)
 */
int utr_write_paraview_field_old(int Mesh_id, int Field_id, char *Filename, char ** Desc)
{
  FILE *fp;
  int i,nrdofs;
  PVSolInfo * solInfos;
  int numNodes;


/*++++++++++++++++ executable statements ++++++++++++++++*/

/* open the output file */
  fp = fopen(Filename, "w");
  if(fp==NULL) {
	printf("Cannot open file '%s' for Paraview field data\n",Filename);
	return(-1);
  } 

  numNodes = mmr_get_nr_node(Mesh_id);
  solInfos = (PVSolInfo*)malloc((numNodes+1) * sizeof(PVSolInfo));
  
  fprintf(fp,"POINT_DATA %d\n", numNodes);
  fprintf(fp,"%s",Desc[0]);
  fprintf(fp,"LOOKUP_TABLE default\n");
  
  
  
  for(i=1;i<=numNodes;i++){
	nrdofs=apr_get_ent_nrdofs(Field_id,APC_VERTEX,i);
	apr_read_ent_dofs(Field_id,APC_VERTEX, i, nrdofs, 1, solInfos[i].dofs);
  }
  
  for(i=1;i<=numNodes;i++){
	fprintf(fp,"%.12lg\n",solInfos[i].dofs[0]);
  }
  
  if(nrdofs > 1) {
	fprintf(fp,"%s",Desc[1]);
	for(i=1;i<=numNodes;i++){
	fprintf(fp,"%.12lg %.12lg %.12lg\n",solInfos[i].dofs[0], solInfos[i].dofs[1], solInfos[i].dofs[2]);
	}
  }
  fclose(fp);
  
  free(solInfos);

  return 0;
}
/// utr_merge_paraview_mesh_field - to merge previously dumped Paraview mesh and Paraview field files
/// into one .vtu file.
/** \param meshFile - c-string with Paraview mesh filename
	\param fieldFile - c-string with Paraview field filename
	\param mergedFile - c-string with name of the file to write on disk
 */
int utr_merge_paraview_mesh_field(char * meshFile, char * fieldFile, char * mergedFile)
{
	char call[255]={0};
	sprintf(call, "cat %s %s > %s",meshFile,fieldFile,mergedFile);
	system(call);
	return 0;
}
