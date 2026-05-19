#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

#include "pdh_intf.h"
#include "mmh_intf.h"
#include "aph_intf.h"

#include "pdh_elast.h"


typedef struct {
    int elNodes[7];
    int numNodes;
    int elType;
} PVElInfo;

typedef struct {
    double dofs[4];
} PVSolInfo;


int writeParaviewMesh(int Mesh_id, char *Filename)
{
    FILE *fp;
    int i;
    PVElInfo *elInfos;
    int numNodes;
    int numElems;
    double nodeCoor[3];
    int numConn = 0;


/*++++++++++++++++ executable statements ++++++++++++++++*/

/* open the output file */
    fp = fopen(Filename, "w");
    if (fp == NULL) {
	printf("Cannot open file '%s' for Paraview mesh data\n", Filename);
	return (-1);
    }


    numNodes = mmr_mesh_i_params(Mesh_id, 1);
    numElems = mmr_mesh_i_params(Mesh_id, 13);
    elInfos = (PVElInfo *) malloc((numElems + 1) * sizeof(PVElInfo));



    fprintf(fp, "# vtk DataFile Version 2.0\n");
    fprintf(fp, "Elasticity: pdd_elast\n");
    fprintf(fp, "ASCII\n");
    fprintf(fp, "DATASET UNSTRUCTURED_GRID\n");
    fprintf(fp, "POINTS %d double\n", numNodes);
    for (i = 1; i <= numNodes; i++) {
	mmr_node_coor(Mesh_id, i, nodeCoor);
	fprintf(fp, "%.12lg %.12lg %.12lg\n", nodeCoor[0], nodeCoor[1], nodeCoor[2]);
    }

    for (i = 1; i <= numElems; i++) {
	mmr_el_node_coor(Mesh_id, i, elInfos[i].elNodes, NULL);
	//apr_get_el_dofs(Field_id, i, 1, elInfos[i].solDofs);
	elInfos[i].numNodes = elInfos[i].elNodes[0];
	if (elInfos[i].numNodes == 6)
	    elInfos[i].elType = 13;
	else if (elInfos[i].numNodes == 4)
	    elInfos[i].elType = 10;
	else
	    elInfos[i].elType = -1;

	numConn += elInfos[i].numNodes;
    }

    fprintf(fp, "CELLS %d %d\n", numElems, numConn + numElems);

    for (i = 1; i <= numElems; i++) {
	if (elInfos[i].elType == 13)	//PRISM
	    fprintf(fp, "6 %d %d %d %d %d %d\n", elInfos[i].elNodes[1] - 1, elInfos[i].elNodes[2] - 1, elInfos[i].elNodes[3] - 1, elInfos[i].elNodes[4] - 1, elInfos[i].elNodes[5] - 1, elInfos[i].elNodes[6] - 1);
	else if (elInfos[i].elType == 10)	//TETRA
	    fprintf(fp, "4 %d %d %d %d\n", elInfos[i].elNodes[1] - 1, elInfos[i].elNodes[2] - 1, elInfos[i].elNodes[3] - 1, elInfos[i].elNodes[4] - 1);
	else
	    fprintf(fp, "ERROR: WRONG ELEMENT TYPE");
    }

    fprintf(fp, "CELL_TYPES %d\n", numElems);
    for (i = 1; i <= numElems; i++) {
	fprintf(fp, "%d\n", elInfos[i].elType);
    }

    fclose(fp);
    free(elInfos);

    return 0;
}


int writeParaviewField(int Mesh_id, int Field_id, char *Filename)
{
    FILE *fp;
    int i,j,k;
    PVSolInfo *solInfos;
    int numNodes, *check;
    int pdeg,constr[6];
    double tmpdofs1[4],tmpdofs2[4];

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* open the output file */
    fp = fopen(Filename, "w");
    if (fp == NULL) {
	printf("Cannot open file '%s' for Paraview field data\n", Filename);
	return (-1);
    }

    numNodes = mmr_mesh_i_params(Mesh_id, 1);
    solInfos = (PVSolInfo *) malloc((numNodes + 1) * sizeof(PVSolInfo));
    check = (int *) malloc((numNodes + 1) * sizeof(int));

    fprintf(fp, "POINT_DATA %d\n", numNodes);
//    fprintf(fp, "SCALARS pressure double 1\n");
//    fprintf(fp, "LOOKUP_TABLE default\n");

    //zle czyta wezly zawieszone!!

    for (i = 1; i <= numNodes; i++) {
    	pdeg=apr_get_ent_pdeg(Field_id,APC_VERTEX,i);
    	if(pdeg!=0){
    		//printf("Pdeg=0; wezeł %d\n",i);
    		check[i] = apr_read_ent_dofs(Field_id, APC_VERTEX, i, 3, 1, solInfos[i].dofs);
    	}
    	else
    	{
    		for(k=0;k<4;k++)
    	    	tmpdofs2[k]=0;
    		apr_get_constr_data(Field_id,i,constr);
    	    //printf("Constr[0]=%d\n",constr[0]);
    	    for(j=1;j<=constr[0];j++)
    	    {
    	    	check[i] = 1;
    	    	apr_read_ent_dofs(Field_id, APC_VERTEX, constr[j], 3, 1, tmpdofs1);
    	    	for(k=0;k<4;k++)
    	    		tmpdofs2[k]+=tmpdofs1[k];
    	    }
    	    for(k=0;k<4;k++)
    	    	solInfos[i].dofs[k]=tmpdofs2[k]/constr[0];
    	    	//printf("%d\t",constr[j]);
    	    //printf("\n");


    	}

    }

//    for (i = 1; i <= numNodes; i++) {
//	if (check[i] != 2)
//	    fprintf(fp, "%.12lg\n", solInfos[i].dofs[3]);
//    }

    fprintf(fp, "VECTORS velocity double\n");
    for (i = 1; i <= numNodes; i++) {
	if (check[i] != 2)
	    fprintf(fp, "%.12lg %.12lg %.12lg\n", solInfos[i].dofs[0], solInfos[i].dofs[1], solInfos[i].dofs[2]);
    }

    fclose(fp);

    free(solInfos);
    free(check);

    return 0;
}


int mergeParaviewMeshAndField(int Mesh_id, int Field_id, char *Filename)
{
		FILE *fp;
	    int i,j,k,l;
	    PVElInfo *elInfos;
	    PVSolInfo *solInfos;
	    int numNodes, *check;
	    int numElems;
	    int nummaxNodes,nummaxElems;
	    double nodeCoor[3];
	    int numConn = 0;
	    int pdeg,constr[6];
	    double tmpdofs1[4],tmpdofs2[4];

	/*++++++++++++++++ executable statements ++++++++++++++++*/

	/* open the output file */
	    fp = fopen(Filename, "w");
	    if (fp == NULL) {
		printf("Cannot open file '%s' for Paraview mesh data\n", Filename);
		return (-1);
	    }

//	    i=0;
//	    numNodes=0;
//	    while((i=mmr_get_next_node_all(Mesh_id, i))!=0)
//	    {
//	    	if(mmr_node_status(Mesh_id,i)==1)
//	    		numNodes++;
//	    }
//
//	    i=0;
//	    numElems=0;
//	    while((i=mmr_get_next_act_elem(Mesh_id, i))!=0)
//	    {
//	    	numElems++;
//	    }

	    numNodes = mmr_mesh_i_params(Mesh_id, 2);
	    numElems = mmr_mesh_i_params(Mesh_id, 13);

	    //printf("numNodes=%d\n",numNodes);
	    //printf("numElems=%d\n",numElems);

	    elInfos = (PVElInfo *) malloc((numElems + 1) * sizeof(PVElInfo));

	    fprintf(fp, "# vtk DataFile Version 2.0\n");
	    fprintf(fp, "Elasticity: pdd_elast\n");
	    fprintf(fp, "ASCII\n");
	    fprintf(fp, "DATASET UNSTRUCTURED_GRID\n");
	    fprintf(fp, "POINTS %d double\n", numNodes);

	    for(i=1;i<=numNodes;i++)
	    {
	    	if(mmr_node_status(Mesh_id,i)==1)
	    	{
				mmr_node_coor(Mesh_id, i, nodeCoor);
				fprintf(fp, "%.12lg %.12lg %.12lg\n", nodeCoor[0], nodeCoor[1], nodeCoor[2]);
	    	}
	    	else
	    	{
	    		mmr_node_coor(Mesh_id, i, nodeCoor);
	    		//printf("Zaszlo dla i=%d\n",i);
	    		fprintf(fp, "%.12lg %.12lg %.12lg\n", nodeCoor[0], nodeCoor[1], nodeCoor[2]);
	    	}

	    }

	    i=0;
	    j=0;
	   	while((i=mmr_get_next_act_elem(Mesh_id, i))!=0)
	    {
				mmr_el_node_coor(Mesh_id, i, elInfos[j].elNodes, NULL);
				//apr_get_el_dofs(Field_id, i, 1, elInfos[i].solDofs);
				elInfos[j].numNodes = elInfos[j].elNodes[0];
				if (elInfos[j].numNodes == 6)
					elInfos[j].elType = 13;
				else if (elInfos[j].numNodes == 4)
					elInfos[j].elType = 10;
				else
					elInfos[j].elType = -1;

				numConn += elInfos[j].numNodes;
				j++;
	    }

	    fprintf(fp, "CELLS %d %d\n", numElems, numConn + numElems);
	    //printf("CELLS %d %d\n", numElems, numConn + numElems);

	    i=0;
	    j=0;
	   	while((i=mmr_get_next_act_elem(Mesh_id, i))!=0)
	    	{
				if (elInfos[j].elType == 13)	//PRISM
					fprintf(fp, "6 %d %d %d %d %d %d\n", elInfos[j].elNodes[1] - 1, elInfos[j].elNodes[2] - 1, elInfos[j].elNodes[3] - 1, elInfos[j].elNodes[4] - 1, elInfos[j].elNodes[5] - 1, elInfos[j].elNodes[6] - 1);
				else if (elInfos[j].elType == 10)	//TETRA
					fprintf(fp, "4 %d %d %d %d\n", elInfos[j].elNodes[1] - 1, elInfos[j].elNodes[2] - 1, elInfos[j].elNodes[3] - 1, elInfos[j].elNodes[4] - 1);
				else
					//fprintf(fp, "6 %d %d %d %d %d %d\n", elInfos[i].elNodes[1] - 1, elInfos[i].elNodes[2] - 1, elInfos[i].elNodes[3] - 1, elInfos[i].elNodes[4] - 1, elInfos[i].elNodes[5] - 1, elInfos[i].elNodes[6] - 1);
					fprintf(fp, "ERROR: WRONG ELEMENT TYPE");
				j++;
	    	}

	    fprintf(fp, "CELL_TYPES %d\n", numElems);
	    i=0;
	    j=0;
	   	while((i=mmr_get_next_act_elem(Mesh_id, i))!=0)
	    		fprintf(fp, "%d\n", elInfos[j].elType);

	    //Field

	    solInfos = (PVSolInfo *) malloc((numNodes + 1) * sizeof(PVSolInfo));
	    check = (int *) malloc((numNodes + 1) * sizeof(int));

	    fprintf(fp, "POINT_DATA %d\n", numNodes);
	//    fprintf(fp, "SCALARS pressure double 1\n");
	//    fprintf(fp, "LOOKUP_TABLE default\n");
	    l=0;
	    for(i=1;i<=numNodes;i++)
	    {
	    	if(mmr_node_status(Mesh_id,i)==1)
	    	{
				pdeg=apr_get_ent_pdeg(Field_id,APC_VERTEX,i);
				if(pdeg!=0){
					//printf("Pdeg=0; wezeł %d\n",i);
					check[l] = apr_read_ent_dofs(Field_id, APC_VERTEX, i, 3, 1, solInfos[l].dofs);
				}
				else
				{
					for(k=0;k<4;k++)
						tmpdofs2[k]=0;
					apr_get_constr_data(Field_id,i,constr);
					//printf("Constr[0]=%d\n",constr[0]);
					for(j=1;j<=constr[0];j++)
					{
						check[l] = 1;
						apr_read_ent_dofs(Field_id, APC_VERTEX, constr[j], 3, 1, tmpdofs1);
						for(k=0;k<4;k++)
							tmpdofs2[k]+=tmpdofs1[k];
					}
					for(k=0;k<4;k++)
						solInfos[l].dofs[k]=tmpdofs2[k]/constr[0];
						//printf("%d\t",constr[j]);
					//printf("\n");


				}
	    	}
	    	else
	    	{
	    		check[l]=2;
	    		//printf("\nWezel i=%d check==2 dla l=%d\n",i,l);
	    	}
	    	l++;
	    }

	//    for (i = 1; i <= numNodes; i++) {
	//	if (check[i] != 2)
	//	    fprintf(fp, "%.12lg\n", solInfos[i].dofs[3]);
	//    }

	    fprintf(fp, "VECTORS velocity double\n");
	    i=0;
	    j=0;
	    for(i=1;i<=numNodes;i++)
	    {
	    	//if(mmr_node_status(Mesh_id,i)==1)
	    	//{
	    		if (check[j] != 2)
	    			fprintf(fp, "%.12lg %.12lg %.12lg\n", solInfos[j].dofs[0], solInfos[j].dofs[1], solInfos[j].dofs[2]);
	    		else
	    			fprintf(fp, "%.12lg %.12lg %.12lg\n", 0.0, 0.0, 0.0);

	    		j++;

			//}
	    }

	    fclose(fp);

	    free(elInfos);
	    free(solInfos);
	    free(check);

	    return 0;








//    char call[1000] = { 0 };
//
//    sprintf(call, "cat %s %s > %s", meshFile, fieldFile, vtuFile);
//
//    system(call);
//
//    return 1;
}
