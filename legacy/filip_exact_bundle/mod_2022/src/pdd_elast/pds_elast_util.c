#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

/* interface for general purpose utilities - for all problem dependent modules*/
//#include "uth_intf.h"

/* header file of the problem dependent module for Laplace's equation */
#include "./pdh_elast.h"


/*** utilities ***/

void utr_vec_points(
		double* point_a, 	/* in: point a */
		double* point_b, 	/* in: point b */
		double* vec_c	/* out: vector ab */
		)
{
	vec_c[0]=point_b[0]-point_a[0];
	vec_c[1]=point_b[1]-point_a[1];
	vec_c[2]=point_b[2]-point_a[2];
}

double utr_vec_scal(
		double* vec_a, 	/* in: vector a */
		double* vec_b 	/* in: vector b */
		)
{
	double scal;
	scal=vec_a[0]*vec_b[0]+vec_a[1]*vec_b[1]+vec_a[2]*vec_b[2];

	return scal;
}

//function SameSide(p1,p2, a,b)
//    cp1 = CrossProduct(b-a, p1-a)
//    cp2 = CrossProduct(b-a, p2-a)
//    if DotProduct(cp1, cp2) >= 0 then return true
//    else return false
//
//function PointInTriangle(p, a,b,c)
//    if SameSide(p,a, b,c) and SameSide(p,b, a,c)
//        and SameSide(p,c, a,b) then return true
//    else return false

int sameside(
		double *p1,
		double *p2,
		double *a,
		double *b
		)
{
		double sum1[3],sum2[3],cp1[3],cp2[3];

		utr_vec_points(a,b,sum1);
		utr_vec_points(a,p1,sum2);
		utr_vec3_prod(sum1,sum2,cp1);
		utr_vec_points(a,p2,sum2);
		utr_vec3_prod(sum1,sum2,cp2);
		if(utr_vec_scal(cp1,cp2)>=0)
			return 1;
		else
			return 0;
}

int pointintriangle(
		double *p,
		double *a,
		double *b,
		double *c
		)
{
	if (sameside(p,a, b,c) && sameside(p,b, a,c) && sameside(p,c, a,b))
		return 1;
	else
		return 0;
}

int writeGrains(int Mesh_id, int Field_id, int *Grains, int Counter, char *Filename)
{
	FILE *fp;
	int nnode,nel,numelem,max,i,j,flag,licz,flag2,flag3,pdeg;
	double nodeCoor[3];
	int elnodes[7];
	double mn=pdv_problem.gr.multip;
	int edges[10];
	int neig[6];
	int sons[3],nodes[3];
	int liczel;
	typedef struct{
	  int id;
	  int a;
	  int b;
	  int c;
	}elem;

	elem **new_el;

	/* open the output file */
	fp = fopen(Filename, "w");
	if (fp == NULL)
	{
		printf("Cannot open file '%s'\n", Filename);
		return (-1);
	}
	fprintf(fp,"*Part, name=PART-%d\n",Counter);
	fprintf(fp,"*Node\n");
	nnode = mmr_mesh_i_params(Mesh_id, 2);
	numelem = mmr_get_max_elem_id(Mesh_id);

	new_el = (elem **)malloc((numelem+1)*sizeof(elem *));
	for(i=1;i<=numelem;i++)
		new_el[i]=(elem *)malloc(2*sizeof(elem));

//	for(i=1;i<=nnode;i++)
//	{
//		pdeg=apr_get_ent_pdeg(Field_id,0,i);
//		printf("Node %d pdeg %d\n",i,pdeg);
//	}

	liczel=numelem+1;
	nel=0;
	while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
	{
		for(i=0;i<2;i++)
			new_el[nel][i].a=0;
		for(i=0;i<2;i++)
			new_el[nel][i].b=0;
		for(i=0;i<2;i++)
			new_el[nel][i].c=0;

		licz=0;
		mmr_el_eq_neig(Mesh_id,nel,neig,NULL);
		//printf("Sasiedzi elementu %d:\n",iel);
		for(i=1;i<=neig[0];i++)
		{
			if(mmr_el_status(Mesh_id,neig[i])==-1)
			{
				//printf("%d - %d\n",neig[i],mmr_el_status(mesh_id,neig[i]));
				licz++;
			}
		}
		//printf("licz=%d\n",licz);
		if(licz==1)
		{
			//printf("Element %d sklada sie z: ",nel);
			mmr_el_node_coor(Mesh_id,nel,elnodes,NULL);
			//printf("Wezly elementu: %d, %d, %d\n",elnodes[1],elnodes[2],elnodes[3]);
			//third=elnodes[1];
			mmr_el_edges(Mesh_id,nel,edges);
			for(j=1;j<=3;j++)
			{
				//printf("%d ",edges[j]);
				if(mmr_edge_status(Mesh_id,edges[j])==-1)
				{
					mmr_edge_sons(Mesh_id,edges[j],sons);
					mmr_edge_nodes(Mesh_id,edges[j],nodes);
					//printf("Wezly edgea %d i %d\n",nodes[0],nodes[1]);
					for(i=1;i<=3;i++)
					{
						if(elnodes[i]!=nodes[0]&&elnodes[i]!=nodes[1])
						{
							new_el[nel][0].a=elnodes[i];
							new_el[nel][1].a=elnodes[i];
							//printf("i=%d\n",i);
							if(i==1)
							{
								new_el[nel][0].c=elnodes[3];
								new_el[nel][1].b=elnodes[2];
							}
							if(i==2)
							{
								new_el[nel][0].c=elnodes[1];
								new_el[nel][1].b=elnodes[3];
							}
							if(i==3)
							{
								new_el[nel][0].c=elnodes[2];
								new_el[nel][1].b=elnodes[1];
							}
						}
					}
					//printf("Synami edge %d sa: %d i %d\n",edges[j],sons[0],sons[1]);
					mmr_edge_nodes(Mesh_id,sons[0],nodes);
					new_el[nel][0].b=nodes[1];
					new_el[nel][1].c=nodes[1];
					//printf("Zawieszony jest %d wezel!\n",nodes[1]);
					//mmr_edge_nodes(Mesh_id,sons[1],nodes);
					//new_el[nel][1].b=nodes[0];
					//new_el[nel][1].c=nodes[1];

					new_el[nel][0].id=liczel++;
					new_el[nel][1].id=liczel++;

				}
			}
			//printf("Element %d o wezlach %d,%d,%d rozklada sie na %d o wezlach %d,%d,%d oraz %d o wezlach %d,%d,%d\n",nel,elnodes[1],elnodes[2],elnodes[3],new_el[nel][0].id,new_el[nel][0].a,new_el[nel][0].b,new_el[nel][0].c,new_el[nel][1].id,new_el[nel][1].a,new_el[nel][1].b,new_el[nel][1].c);
			//printf("\n");
		}

	}

	//printf("numNodes=%d\n",nnode);
	//printf("numElems=%d\n",nel);
	for(i=1;i<=nnode;i++)
	{
		mmr_node_coor(Mesh_id, i, nodeCoor);
		if(nodeCoor[2]==0)
			fprintf(fp, "\t%d,\t%.10lf,\t%.10lf\n",i,nodeCoor[0]*mn, nodeCoor[1]*mn);
	}
	fprintf(fp,"\n");
	fprintf(fp,"*Element, type=CPE3\n");
	liczel=numelem+1;
	nel=0;
	while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
	{
		if(new_el[nel][0].a==0)
		{
			mmr_el_node_coor(Mesh_id, nel, elnodes, NULL);
			fprintf(fp,"\t%d,\t%d,\t%d,\t%d\n",nel,elnodes[1],elnodes[2],elnodes[3]);
		}
		else
		{
			fprintf(fp,"\t%d,\t%d,\t%d,\t%d\n",new_el[nel][0].id,new_el[nel][0].a,new_el[nel][0].b,new_el[nel][0].c);
			fprintf(fp,"\t%d,\t%d,\t%d,\t%d\n",new_el[nel][1].id,new_el[nel][1].a,new_el[nel][1].b,new_el[nel][1].c);
		}
	}

	fprintf(fp,"\n");
	for(i=1;i<=2;i++)
	{
		licz=1;
		flag=0;
		nel=0;
		while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
		{
			flag2=0;
			if((flag==0))
			{
				fprintf(fp,"\n\n*Elset, elset=M%d\n",i);
				flag++;
			}
			if(mmr_el_groupID(Mesh_id,nel)==i)
			{
				if(flag==1)
				{
					if(new_el[nel][0].a==0)
					{
						fprintf(fp,"%d",nel);
						flag++;
						licz++;
					}
					else
					{
						fprintf(fp,"%d, %d",new_el[nel][0].id,new_el[nel][1].id);
						flag++;
						licz+=2;
					}
				}
				else
				{
					if(new_el[nel][0].a==0)
					{
						fprintf(fp,", %d",nel);
						licz++;
					}
					else
					{
						if(licz%10==0)
						{
							fprintf(fp,", %d,\n%d",new_el[nel][0].id,new_el[nel][1].id);
							licz+=2;
						}
						else
						{
							fprintf(fp,", %d, %d",new_el[nel][0].id,new_el[nel][1].id);
							licz+=2;
						}
					}
				}
				flag2=1;
			}
			if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag2==1))
			{
				fprintf(fp,",\n");
				flag=1;
			}
		}
	}

	/*
	max=Grains[0];
	for(i=1;i<=numelem;i++)
	{
		if(Grains[i]>=max)
		   max=Grains[i];
	}
	//printf("max=%d\n",max);
	//flag=0;
	fprintf(fp,"\n");
	for(i=1;i<=max;i++)
	{
		licz=1;
		flag=0;
		for(j=1;j<=numelem;j++)
		{
			flag2=0;
			if((Grains[j]==i)&&(flag==0))
			{
				fprintf(fp,"\n\n*Elset, elset=M%d\n",i);
				flag++;
			}
			if(Grains[j]==i)
			{
				if(flag==1)
				{
					fprintf(fp,"%d",j);
					flag++;
				}
				else
					fprintf(fp,", %d",j);
				licz++;
				flag2=1;
			}
			if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag2==1))
			{
				fprintf(fp,",\n");
				flag=1;
			}
		}
	}
	/**/
	fprintf(fp,"\n\n");
	fprintf(fp,"*Nset, nset=N1\n");
	licz=1;
	flag2=0;
	for(i=1;i<=nnode;i++)
	{
		flag=0;
		mmr_node_coor(Mesh_id, i, nodeCoor);
		if(nodeCoor[1]==0.0&&nodeCoor[2]==0.0)
		{
			if(flag2==0)
				fprintf(fp, "%d",i);
			else
				fprintf(fp, ", %d",i);
			licz++;
			flag=1;
			flag2=1;
		}
		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
		{
			fprintf(fp,",\n");
			flag2=0;
		}
	}

	fprintf(fp,"\n\n");
	fprintf(fp,"*Nset, nset=N2\n");
	licz=1;
	flag2=0;
	for(i=1;i<=nnode;i++)
	{
		flag=0;
		mmr_node_coor(Mesh_id, i, nodeCoor);
		if(nodeCoor[0]==1.0&&nodeCoor[2]==0.0)
		{
			if(flag2==0)
				fprintf(fp, "%d",i);
			else
				fprintf(fp, ", %d",i);
			licz++;
			flag=1;
			flag2=1;
		}
		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
		{
			fprintf(fp,",\n");
			flag2=0;
		}
	}

	fprintf(fp,"\n\n");
	fprintf(fp,"*Nset, nset=N3\n");
	licz=1;
	flag2=0;
	for(i=1;i<=nnode;i++)
	{
		flag=0;
		mmr_node_coor(Mesh_id, i, nodeCoor);
		if(nodeCoor[1]==1.0&&nodeCoor[2]==0.0)
		{
			if(flag2==0)
				fprintf(fp, "%d",i);
			else
				fprintf(fp, ", %d",i);
			licz++;
			flag=1;
			flag2=1;
		}
		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
		{
			fprintf(fp,",\n");
			flag2=0;
		}
	}

	fprintf(fp,"\n\n");
	fprintf(fp,"*Nset, nset=N4\n");
	licz=1;
	flag2=0;
	for(i=1;i<=nnode;i++)
	{
		flag=0;
		mmr_node_coor(Mesh_id, i, nodeCoor);
		if(nodeCoor[0]==0.0&&nodeCoor[2]==0.0)
		{
			if(flag2==0)
				fprintf(fp, "%d",i);
			else
				fprintf(fp, ", %d",i);
			licz++;
			flag=1;
			flag2=1;
		}
		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
		{
			fprintf(fp,",\n");
			flag2=0;
		}
	}
	fprintf(fp,"\n\n");
	for(i=1;i<=2;i++)
	{
		fprintf(fp,"** Section: Section-%d-M%d\n",i,i);
		fprintf(fp,"*Solid Section, elset=M%d, material=A\n",i);
		fprintf(fp,"1.,\n");
	}


	fclose(fp);
	return 1;
}

int writeGrains3D(int Mesh_id, int Field_id, int *Grains, int Counter, char *Filename)
{
	FILE *fp;
	int nnode,nel,numelem,max,i,j,flag,licz,flag2,flag3,pdeg;
	double nodeCoor[3];
	int elnodes[7];
	double mn=pdv_problem.gr.multip;
	int edges[10];
	int neig[6];
	int sons[6],nodes[6];
	int liczel;
	typedef struct{
	  int id;
	  int a;
	  int b;
	  int c;
	  int d;
	  int e;
	  int f;
	}elem;

	elem **new_el;

	/* open the output file */
	fp = fopen(Filename, "w");
	if (fp == NULL)
	{
		printf("Cannot open file '%s'\n", Filename);
		return (-1);
	}
	//fprintf(fp,"*Part, name=PART-%d\n",Counter);
	fprintf(fp,"*Node\n");
	nnode = mmr_mesh_i_params(Mesh_id, 2);
	numelem = mmr_get_max_elem_id(Mesh_id);

	new_el = (elem **)malloc((numelem+1)*sizeof(elem *));
	for(i=1;i<=numelem;i++)
		new_el[i]=(elem *)malloc(2*sizeof(elem));

//	for(i=1;i<=nnode;i++)
//	{
//		pdeg=apr_get_ent_pdeg(Field_id,0,i);
//		printf("Node %d pdeg %d\n",i,pdeg);
//	}

	liczel=numelem+1;
	nel=0;
	while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
	{
		for(i=0;i<2;i++)
			new_el[nel][i].a=0;
		for(i=0;i<2;i++)
			new_el[nel][i].b=0;
		for(i=0;i<2;i++)
			new_el[nel][i].c=0;
		for(i=0;i<2;i++)
			new_el[nel][i].d=0;
		for(i=0;i<2;i++)
			new_el[nel][i].e=0;
		for(i=0;i<2;i++)
			new_el[nel][i].f=0;

		licz=0;
		mmr_el_eq_neig(Mesh_id,nel,neig,NULL);
		//printf("Sasiedzi elementu %d:\n",iel);
		for(i=1;i<=neig[0];i++)
		{
			if(mmr_el_status(Mesh_id,neig[i])==-1)
			{
				//printf("%d - %d\n",neig[i],mmr_el_status(mesh_id,neig[i]));
				licz++;
			}
		}
		//printf("licz=%d\n",licz);
		if(licz==1)
		{
			//printf("Element %d sklada sie z: ",nel);
			mmr_el_node_coor(Mesh_id,nel,elnodes,NULL);
			//printf("Wezly elementu: %d, %d, %d\n",elnodes[1],elnodes[2],elnodes[3],elnodes[4],elnodes[5],elnodes[6]);
			//third=elnodes[1];
			mmr_el_edges(Mesh_id,nel,edges);
			for(j=1;j<=3;j++)
			{
				printf("%d ",edges[j]);
				if(mmr_edge_status(Mesh_id,edges[j])==-1)
				{
					mmr_edge_sons(Mesh_id,edges[j],sons);
					mmr_edge_nodes(Mesh_id,edges[j],nodes);
					//printf("Wezly edgea %d i %d\n",nodes[0],nodes[1]);
					for(i=1;i<=3;i++)
					{
						if(elnodes[i]!=nodes[0]&&elnodes[i]!=nodes[1])
						{
							new_el[nel][0].a=elnodes[i];
							new_el[nel][1].a=elnodes[i];
							new_el[nel][0].d=elnodes[i+3];
							new_el[nel][1].d=elnodes[i+3];
							//printf("i=%d\n",i);
							if(i==1)
							{
								new_el[nel][0].c=elnodes[3];
								new_el[nel][1].b=elnodes[2];

								new_el[nel][0].f=elnodes[6];
								new_el[nel][1].e=elnodes[5];
							}
							if(i==2)
							{
								new_el[nel][0].c=elnodes[1];
								new_el[nel][1].b=elnodes[3];

								new_el[nel][0].f=elnodes[4];
								new_el[nel][1].e=elnodes[6];
							}
							if(i==3)
							{
								new_el[nel][0].c=elnodes[2];
								new_el[nel][1].b=elnodes[1];

								new_el[nel][0].f=elnodes[5];
								new_el[nel][1].e=elnodes[4];
							}
						}
					}
					//printf("Synami edge %d sa: %d i %d\n",edges[j],sons[0],sons[1]);
					mmr_edge_nodes(Mesh_id,sons[0],nodes);
					new_el[nel][0].b=nodes[1];
					new_el[nel][1].c=nodes[1];

					//3D
					mmr_edge_sons(Mesh_id,edges[j+3],sons);
					mmr_edge_nodes(Mesh_id,sons[0],nodes);
					new_el[nel][0].e=nodes[1];
					new_el[nel][1].f=nodes[1];

					//printf("Zawieszony jest %d wezel!\n",nodes[1]);
					//mmr_edge_nodes(Mesh_id,sons[1],nodes);
					//new_el[nel][1].b=nodes[0];
					//new_el[nel][1].c=nodes[1];

					new_el[nel][0].id=liczel++;
					new_el[nel][1].id=liczel++;

				}
			}
			//printf("Element %d o wezlach %d,%d,%d rozklada sie na %d o wezlach %d,%d,%d oraz %d o wezlach %d,%d,%d\n",nel,elnodes[1],elnodes[2],elnodes[3],new_el[nel][0].id,new_el[nel][0].a,new_el[nel][0].b,new_el[nel][0].c,new_el[nel][1].id,new_el[nel][1].a,new_el[nel][1].b,new_el[nel][1].c);
			//printf("\n");
		}

	}

	//printf("numNodes=%d\n",nnode);
	//printf("numElems=%d\n",nel);
	for(i=1;i<=nnode;i++)
	{
		mmr_node_coor(Mesh_id, i, nodeCoor);
		//if(nodeCoor[2]==0)
		fprintf(fp, "\t%d,\t%.10lf,\t%.10lf,\t%.10lf\n",i,nodeCoor[0]*mn, nodeCoor[1]*mn, nodeCoor[2]*mn);
	}
	fprintf(fp,"\n");
	fprintf(fp,"*Element, type=C3D6\n");
	liczel=numelem+1;
	nel=0;
	while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
	{
		if(new_el[nel][0].a==0)
		{
			mmr_el_node_coor(Mesh_id, nel, elnodes, NULL);
			fprintf(fp,"\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d\n",nel,elnodes[1],elnodes[2],elnodes[3],elnodes[4],elnodes[5],elnodes[6]);
		}
		else
		{
			fprintf(fp,"\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d\n",new_el[nel][0].id,new_el[nel][0].a,new_el[nel][0].b,new_el[nel][0].c,new_el[nel][0].d,new_el[nel][0].e,new_el[nel][0].f);
			fprintf(fp,"\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d\n",new_el[nel][1].id,new_el[nel][1].a,new_el[nel][1].b,new_el[nel][1].c,new_el[nel][1].d,new_el[nel][1].e,new_el[nel][1].f);
		}
	}

	fprintf(fp,"\n");
	for(i=1;i<=2;i++)
	{
		licz=1;
		flag=0;
		nel=0;
		while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
		{
			flag2=0;
			if((flag==0))
			{
				fprintf(fp,"\n\n*Elset, elset=M%d\n",i);
				flag++;
			}
			if(mmr_el_groupID(Mesh_id,nel)==i)
			{
				if(flag==1)
				{
					if(new_el[nel][0].a==0)
					{
						fprintf(fp,"%d",nel);
						flag++;
						licz++;
					}
					else
					{
						fprintf(fp,"%d, %d",new_el[nel][0].id,new_el[nel][1].id);
						flag++;
						licz+=2;
					}
				}
				else
				{
					if(new_el[nel][0].a==0)
					{
						fprintf(fp,", %d",nel);
						licz++;
					}
					else
					{
						if(licz%10==0)
						{
							fprintf(fp,", %d,\n%d",new_el[nel][0].id,new_el[nel][1].id);
							licz+=2;
						}
						else
						{
							fprintf(fp,", %d, %d",new_el[nel][0].id,new_el[nel][1].id);
							licz+=2;
						}
					}
				}
				flag2=1;
			}
			if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag2==1))
			{
				fprintf(fp,",\n");
				flag=1;
			}
		}
	}

	/*
	max=Grains[0];
	for(i=1;i<=numelem;i++)
	{
		if(Grains[i]>=max)
		   max=Grains[i];
	}
	//printf("max=%d\n",max);
	//flag=0;
	fprintf(fp,"\n");
	for(i=1;i<=max;i++)
	{
		licz=1;
		flag=0;
		for(j=1;j<=numelem;j++)
		{
			flag2=0;
			if((Grains[j]==i)&&(flag==0))
			{
				fprintf(fp,"\n\n*Elset, elset=M%d\n",i);
				flag++;
			}
			if(Grains[j]==i)
			{
				if(flag==1)
				{
					fprintf(fp,"%d",j);
					flag++;
				}
				else
					fprintf(fp,", %d",j);
				licz++;
				flag2=1;
			}
			if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag2==1))
			{
				fprintf(fp,",\n");
				flag=1;
			}
		}
	}
	/**/
//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N1\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[1]==0.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//
//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N2\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[0]==1.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//
//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N3\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[1]==1.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//
//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N4\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[0]==0.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//	fprintf(fp,"\n\n");
//	for(i=1;i<=2;i++)
//	{
//		fprintf(fp,"** Section: Section-%d-M%d\n",i,i);
//		fprintf(fp,"*Solid Section, elset=M%d, material=A\n",i);
//		fprintf(fp,"1.,\n");
//	}


	fclose(fp);
	return 1;
}

int writeGrains3Dconstr(int Mesh_id, int Field_id, int *Grains, int Counter, char *Filename)
{
	FILE *fp;
	int nnode,nel,numelem,max,i,j,flag,licz,flag2,flag3,pdeg;
	double nodeCoor[3];
	int elnodes[7];
	double mn=pdv_problem.gr.multip;
	int edges[10];
	int neig[6];
	int sons[3],nodes[3];
	int liczel;

	/* open the output file */
	fp = fopen(Filename, "w");
	if (fp == NULL)
	{
		printf("Cannot open file '%s'\n", Filename);
		return (-1);
	}
	fprintf(fp,"*Node\n");
	nnode = mmr_mesh_i_params(Mesh_id, 2);
	numelem = mmr_get_max_elem_id(Mesh_id);

//	for(i=1;i<=nnode;i++)
//	{
//		pdeg=apr_get_ent_pdeg(Field_id,0,i);
//		printf("Node %d pdeg %d\n",i,pdeg);
//	}

	//printf("numNodes=%d\n",nnode);
	//printf("numElems=%d\n",nel);
	for(i=1;i<=nnode;i++)
	{
		mmr_node_coor(Mesh_id, i, nodeCoor);
		fprintf(fp, "\t%d,\t%.10lf,\t%.10lf,\t%.10lf\n",i,nodeCoor[0]*mn, nodeCoor[1]*mn, nodeCoor[2]*mn);
	}
	fprintf(fp,"\n");
	fprintf(fp,"*Element, type=C3D6\n");
	liczel=numelem+1;
	nel=0;
	while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
	{
			mmr_el_node_coor(Mesh_id, nel, elnodes, NULL);
			fprintf(fp,"\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d\n",nel,elnodes[1],elnodes[2],elnodes[3],elnodes[4],elnodes[5],elnodes[6]);
	}

	fprintf(fp,"\n");
	for(i=1;i<=2;i++)
	{
		licz=1;
		flag=0;
		nel=0;
		while((nel=mmr_get_next_act_elem(Mesh_id, nel))!=0)
		{
			flag2=0;
			if((flag==0))
			{
				fprintf(fp,"\n\n*Elset, elset=M%d\n",i);
				flag++;
			}
			if(mmr_el_groupID(Mesh_id,nel)==i)
			{
				if(flag==1)
				{
					fprintf(fp,"%d",nel);
					flag++;
					licz++;

				}
				else
				{
					fprintf(fp,", %d",nel);
					licz++;
				}
				flag2=1;
			}
			if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag2==1))
			{
				fprintf(fp,",\n");
				flag=1;
			}
		}
	}

//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N1\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[1]==0.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//
//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N2\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[0]==1.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//
//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N3\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[1]==1.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//
//	fprintf(fp,"\n\n");
//	fprintf(fp,"*Nset, nset=N4\n");
//	licz=1;
//	flag2=0;
//	for(i=1;i<=nnode;i++)
//	{
//		flag=0;
//		mmr_node_coor(Mesh_id, i, nodeCoor);
//		if(nodeCoor[0]==0.0&&nodeCoor[2]==0.0)
//		{
//			if(flag2==0)
//				fprintf(fp, "%d",i);
//			else
//				fprintf(fp, ", %d",i);
//			licz++;
//			flag=1;
//			flag2=1;
//		}
//		if(((licz-1)%10==0)&&((licz-1)!=0)&&(flag==1))
//		{
//			fprintf(fp,",\n");
//			flag2=0;
//		}
//	}
//	fprintf(fp,"\n\n");
//	for(i=1;i<=2;i++)
//	{
//		fprintf(fp,"** Section: Section-%d-M%d\n",i,i);
//		fprintf(fp,"*Solid Section, elset=M%d, material=A\n",i);
//		fprintf(fp,"1.,\n");
//	}


	fclose(fp);
	return 1;
}


