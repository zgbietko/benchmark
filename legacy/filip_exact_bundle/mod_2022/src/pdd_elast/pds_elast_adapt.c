/************************************************************************
File contains routines:
  pdr_adapt - to enforce adaptation strategy for a given problem
  pdr_err_indi - to return error indicator for an element (internal procedure)
  pdr_create_patch - to recover patch needed for Zienkiewicz-Zhu error esimator
  pdr_get_zzhu_error - to get the value of error
  pdr_zzhu_error - to compute Zienkiewicz-Zhu error estimator

------------------------------
History:
	02.2002 - Krzysztof Banas, initial version
	10.2010 - Filip Krużel, elasticity
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>

/* header files for the problem dependent module for Conv_Diff's equation */
#include "./pdh_elast.h"

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* interface of the mesh manipulation module */
#include "mmh_intf.h"

/* interface for all approximation modules */
#include "aph_intf.h"

/* general purpose utilities */
#include "uth_intf.h"

/* global variable for the patch data structure used in derivative recovery */
utt_patches *pdv_patches;

pdt_patches_old *lista;
double **exdofs=NULL;
int exsize=0;
int nrnode=0;

/*---------------------------------------------------------
  ut_dvector - to allocate a double vector: name[0..ncom-1]:
  name=ut_dvector(ncom,error_text)
---------------------------------------------------------*/
double *ut_dvector( /* return: pointer to allocated vector */
	int ncom,  	/* in: number of components */
	char error_text[]/* in: error text to be printed */
	);

/*---------------------------------------------------------
ut_ivector - to allocate an integer vector: name[0..ncom-1]:
                  name=ut_ivector(ncom,error_text)
---------------------------------------------------------*/
int *ut_ivector(    /* return: pointer to allocated vector */
	int ncom, 	/* in: number of components */
	char error_text[]/* in: error text to be printed */
	);

int pdr_create_patch(
  int Field_id
)
{
  int i,j,k,l,m,n,z, nel,mesh_id,nr_dof_ent_loc,idofent,dof_ent_id,nno;
  int l_dof_ent_types[300], l_dof_ent_ids[300], l_dof_ent_nrdofs[300];
  int *tmp,*tmp2,*tmp3, *tmp5,*tmp6;
  double *tmp4,time;
  int el_nodes[MMC_MAXELVNO+1];        /* list of nodes of El */
  double node_coor[3*MMC_MAXELVNO];  /* coord of nodes of El */
  int max,min;
  
  mesh_id = apr_get_mesh_id(Field_id);
  
  nno=mmr_get_max_node_id(mesh_id);
  
  nrnode=nno;
  
  //FK set gen
  mmr_set_max_gen(Field_id, 15);
  mmr_set_max_gen_diff(Field_id, 1);


  lista=(pdt_patches_old *)malloc((nno+2)*sizeof(pdt_patches_old));
  for(i=1;i<=nno;i++)
    lista[i].patch=(int *)malloc(300*sizeof(int));
  
  for(i=1;i<=nno;i++)
    lista[i].sigma=(double *)malloc(7*sizeof(double));
  
  for(i=1;i<=nno;i++)
    lista[i].posglob=(int *)malloc((nno+1)*sizeof(int));
  //nodes ids of patch for node j
  
  for(j=1;j<=nno;j++)
    {
      for(i=0;i<300;i++)
	lista[j].patch[i]=0;
      for(i=0;i<7;i++)
	lista[j].sigma[i]=0.0;
    }
  
  //printf("Create patch\n");
  
  nel=0;
  while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0)
    {
      pdr_comp_stiff_mat(Field_id, PDC_ELEMENT,nel, PDC_NO_COMP, NULL, &nr_dof_ent_loc, l_dof_ent_types, l_dof_ent_ids, l_dof_ent_nrdofs, NULL, NULL, NULL, NULL);
      
      for(idofent=0; idofent<nr_dof_ent_loc; idofent++)
	{
	  dof_ent_id = l_dof_ent_ids[idofent];
	  i = utr_put_list(nel,lista[dof_ent_id].patch, 300);
	} // end loop over dof_ents of a given int_ent
    } //end loop over int_ent
  
  //printf("\n");
  //	  time=time_clock();
  
  j=0;
  while((j=mmr_get_next_node_all(mesh_id, j))!=0)
    {
/*kbw
      printf("Computing patch %d %\r",(int)((double)j/nno*100));
/*kew*/ 
      i=0;
      while(lista[j].patch[i]!=0)
	{
	  i++;
	}
      lista[j].size=i; //number of elements in patch
/*kbw
printf("For vertex %d number of elements in the patch %d\n",j,i);
/*kew*/ 
      if(i==0)
	continue;
      
      tmp=(int *)malloc((lista[j].size*48+1)*sizeof(int));
      tmp2=(int *)malloc((lista[j].size*48+1)*sizeof(int));
      tmp3=(int *)malloc((lista[j].size*48+1)*sizeof(int));
      tmp4=(double *)malloc((lista[j].size*48+1)*sizeof(double));
      tmp5=(int *)malloc((lista[j].size*48+1)*sizeof(int));
      tmp6=(int *)malloc((lista[j].size*48+1)*sizeof(int));
      
      z=0;
      for(i=0;i<lista[j].size;i++)//for each element in patch
	{
	  apr_get_el_constr_data(Field_id,lista[j].patch[i],tmp,tmp2,tmp3,tmp4);
	  
	  l=0;
	  for(k=1;k<=tmp[0];k++)
	    l+=tmp2[k];
	  for(m=0;m<l;m++)
	    {
	      tmp5[z]=tmp3[m];
	      z++;
	    }
	}//i
      
      for (i = 0; i<z; i++)
	for (m=0; m<z-1; m++)
	  {
	    if (tmp5[m] > tmp5[m+1])
	      {
		min = tmp5[m+1];
		tmp5[m+1] = tmp5[m];
		tmp5[m] = min;
	      }
	  }
      
      max=tmp5[0];
      for(i=1;i<z;i++)
	{
	  if(tmp5[i]==max)
	    {
	      tmp5[i]=-1;
	    }
	  if(tmp5[i]!=-1)
	    max=tmp5[i];
	  else
	    continue;
	}
      n=0;
      for(i=0;i<z;i++)
	{
	  if(tmp5[i]!=-1)
	    {
	      lista[j].posglob[n]=tmp5[i];
	      n++;
	    }
	}
      
      lista[j].size_stiff_mat=n;
      
      free(tmp);
      free(tmp2);
      free(tmp3);
      free(tmp4);
      free(tmp5);
      free(tmp6);
      /*
      printf("Patch dla elementu %d - rozmiar %d\n",j,lista[j].size);
      for(i=0;i<lista[j].size;i++)
    	  printf(" %d",lista[j].patch[i]);
      printf("\n");
	  */


    }//j
  //printf("\n");
  
  //	time=time_clock()-time;
  //	printf("Time=%lf\n",time);
  
  return 1;
}

int pdr_free_list()
{
  int i;
  
  for(i=1;i<=nrnode;i++)
    free(lista[i].patch);
  
  for(i=1;i<=nrnode;i++)
    free(lista[i].sigma);
  
  for(i=1;i<=nrnode;i++)
    free(lista[i].posglob);
  
  free(lista);
  
  return 1;
}



int pdr_get_zzhu_error(
		       int Field_id
		       )
{
  double err;
  int nel=0,mesh_id,i,j,nno,mat,num_dofs;
  double i1,i2,i3;
  err=0;
  
  mesh_id=apr_get_mesh_id(Field_id);
  nno=mmr_get_max_node_id(mesh_id);
  
  pdr_zzhu_error(Field_id);
  
  err=0;
  while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0)
    {
      err+=pdr_err_indi(Field_id,2,nel);
    }
  err=sqrt(err);
  //printf("\n\nZienkiewicz-Zhu error estimator = %lf\n\n",err);

  return 1;
}

int pdr_write_sigma(
		    int field_id
		    )
{
  char name[100];
  double i1,i2,i3;
  int i,j,num_dofs,mesh_id;
  //zapis do pliku sigm
  
  mesh_id=apr_get_mesh_id(field_id);
  
  pdr_zzhu_error(field_id);
  
  FILE *sigmafile;

if(pdv_problem.sigma==1)
  sprintf(name,"%s/field_sigma.dmp",work_dir);
else
  sprintf(name,"%s/field_epsilon.dmp",work_dir);
  
  sigmafile=fopen(name,"w");
  j=0;
  num_dofs=0;
  while((j=mmr_get_next_node_all(mesh_id, j))!=0)
    {
      if(lista[j].size==0)
	continue;
      num_dofs++;
    }
  fprintf(sigmafile,"3 1\n");
  fprintf(sigmafile,"%d\n",num_dofs);
  
  j=0;
  while((j=mmr_get_next_node_all(mesh_id, j))!=0)
    {
      if(lista[j].size==0)
	continue;
      //liczyc niezmienniki i zapisywac do pliku
      
      i1=lista[j].sigma[0]+lista[j].sigma[1]+lista[j].sigma[2];
      i2=lista[j].sigma[1]*lista[j].sigma[2] - lista[j].sigma[5]*lista[j].sigma[5]+lista[j].sigma[0]*lista[j].sigma[2] - lista[j].sigma[4]*lista[j].sigma[4]+lista[j].sigma[0]*lista[j].sigma[1] - lista[j].sigma[3]*lista[j].sigma[3];
      i3=lista[j].sigma[0]*lista[j].sigma[1]*lista[j].sigma[2]+2*lista[j].sigma[3]*lista[j].sigma[4]*lista[j].sigma[5]-lista[j].sigma[0]*lista[j].sigma[5]*lista[j].sigma[5]-lista[j].sigma[1]*lista[j].sigma[4]*lista[j].sigma[4]-lista[j].sigma[2]*lista[j].sigma[3]*lista[j].sigma[3];
      
      fprintf(sigmafile,"%d %lf %lf %lf\n",j,i1,i2,i3);
    }
  
  fclose(sigmafile);
  return 1;
}


int pdr_zzhu_error(
		   int Field_id
		   )
{

  int base_q;		/* type of basis functions for quadrilaterals */
  int pdeg,mesh_id,nreq,sol_vec_id,i,j,k,l,nno,ki,iaux;
  int el_nodes[MMC_MAXELVNO+1];        /* list of nodes of El */
  double node_coor[3*MMC_MAXELVNO];  /* coord of nodes of El */
  double *dofs_loc; /* element solution dofs */
  dofs_loc=(double *)malloc(sizeof(double)*APC_MAXELVD);
  double xcoor[3];      /* global coord of gauss point */
  double u_val[PDC_NREQ]; /* computed solution */
  double u_x[PDC_NREQ];   /* gradient of computed solution */
  double u_y[PDC_NREQ];   /* gradient of computed solution */
  double u_z[PDC_NREQ];   /* gradient of computed solution */
  double *base_phi;    /* basis functions */
  base_phi=(double *)malloc(sizeof(double)*APC_MAXELVD);
  double *base_dphix;  /* x-derivatives of basis function */
  base_dphix=(double *)malloc(sizeof(double)*APC_MAXELVD);
  double *base_dphiy;  /* y-derivatives of basis function */
  base_dphiy=(double *)malloc(sizeof(double)*APC_MAXELVD);
  double *base_dphiz;  /* y-derivatives of basis function */
  base_dphiz=(double *)malloc(sizeof(double)*APC_MAXELVD);
  double determ;
  double Wsp;
  double Sigma[7],Epsilon[7];
  int ngauss;           /* number of gauss points */
  double *xg;   	/* coordinates of gauss points in 3D */
  xg=(double *)malloc(sizeof(double)*3000);
  double *wg;      /* gauss weights */
  wg=(double *)malloc(sizeof(double)*1000);
  double vol;           /* volume for integration rule */
  double *stiff_loc;				/* stiffness matrix for local problem */
  stiff_loc=(double *)malloc(sizeof(double)*PDC_NREQ*APC_MAXELVD*PDC_NREQ*APC_MAXELVD);
  //double stiff_loc2[PDC_NREQ*APC_MAXELVD*PDC_NREQ*APC_MAXELVD];
  double *f_loc; /* rhs vector for local problem */
  double *f_loc1; /* rhs vector for local problem */
  double *f_loc2; /* rhs vector for local problem */
  double *f_loc3; /* rhs vector for local problem */
  double *f_loc4; /* rhs vector for local problem */
  double *f_loc5; /* rhs vector for local problem */
  f_loc=(double *)malloc(sizeof(double)*PDC_NREQ*APC_MAXELVD);
  f_loc1=(double *)malloc(sizeof(double)*PDC_NREQ*APC_MAXELVD);
  f_loc2=(double *)malloc(sizeof(double)*PDC_NREQ*APC_MAXELVD);
  f_loc3=(double *)malloc(sizeof(double)*PDC_NREQ*APC_MAXELVD);
  f_loc4=(double *)malloc(sizeof(double)*PDC_NREQ*APC_MAXELVD);
  f_loc5=(double *)malloc(sizeof(double)*PDC_NREQ*APC_MAXELVD);
  double E[7],S[7];

  double *stiff_patch;
  double **f_patch; //6 razy??
  
  int kk,ieq,idofs,jdofs,num_shap,ndofs,ndofs1;
  int Comp_sm=APC_REWR_BOTH;
  int Comp_sm2=APC_REWR_RHS;
  int Nr_dof_ent;
  int Nrdofs_loc=24;
  int* List_dof_ent_type; /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_id;   /* out: list of no of dofs for 'dof' entity */
  int* List_dof_ent_nrdofs;/* out: list of no of dofs for 'dof' entity */
  int *tabtrans;
  int constr[5];
  
  int ipiv[APC_MAXELSD];
  /* constatnts */
  int ione=1;
  double err,err2;
  int mat;
  double Eyoung=0,Poisni=0;
  
  /* get formulation parameters */

  nreq = apr_get_nreq(Field_id);
  mesh_id = apr_get_mesh_id(Field_id);

  pdr_create_patch(Field_id);
  
  nno=mmr_get_max_node_id(mesh_id);
  
/*kbw
    for(j=1;j<=nno;j++)
    {
    printf("For vertex %d number of elements in the patch %d\n",j,i);
    
    for(i=0;i<lista[j].size;i++)
    printf("%d\t", lista[j].patch[i]);
    printf("\n");
    
    printf("Patch has %d vertices\n", lista[j].size_stiff_mat);
    
    for(i=0;i<lista[j].size_stiff_mat;i++)
    printf("%d\t", lista[j].posglob[i]);
    printf("\n");
    }
/*kew*/ 

  List_dof_ent_type=(int *)malloc(25*sizeof(int));
  List_dof_ent_id=(int *)malloc(25*sizeof(int));
  List_dof_ent_nrdofs=(int *)malloc(25*sizeof(int));
  
  j=0;
  while((j=mmr_get_next_node_all(mesh_id, j))!=0)
    {
      if(lista[j].size==0)
	continue;
      
      stiff_patch=(double *)malloc(lista[j].size_stiff_mat*lista[j].size_stiff_mat*sizeof(double));
      
      for(k=0;k<lista[j].size_stiff_mat*lista[j].size_stiff_mat;k++)
	stiff_patch[k]=0.0;
      
      f_patch=(double **)malloc(6*sizeof(double *));
      for(k=0;k<6;k++)
	f_patch[k]=(double *)malloc(lista[j].size_stiff_mat*sizeof(double));
      
      for(l=0;l<6;l++)
	for(k=0;k<lista[j].size_stiff_mat;k++)
	  f_patch[l][k]=0.0;
      
      ndofs1=lista[j].size_stiff_mat;
      
      //Loop over elements in patch
      i=0;
      while(lista[j].patch[i]!=0)
	{
	  /* find degree of polynomial and number of element scalar dofs */
	  apr_get_el_pdeg(Field_id, lista[j].patch[i], &pdeg);
	  
	  /* get the coordinates of the nodes of El in the right order */
	  mmr_el_node_coor(mesh_id,lista[j].patch[i],el_nodes,node_coor);
	  
	  num_shap = apr_get_el_pdeg_numshap(Field_id,lista[j].patch[i], &pdeg);
	  //printf("Nowy num_shap=%d\n",num_shap);
	  ndofs = num_shap*1;
	  
	  mat=mmr_el_groupID(mesh_id,lista[j].patch[i]);
	  
	  //printf("Element %d material %d\n",lista[j].patch[i],mat);
	  for(k=0;k<matnum;k++)
	    {
	      if(pdv_material[k].id==mat)
		{
		  Eyoung=pdv_material[k].Eyoung;
		  Poisni=pdv_material[k].Poisni;
		  //printf("Eyoung=%.2lf, Poisni=%.2lf\n",Eyoung,Poisni);
		}
	    }
	  
	  /* initialize the matrices to zero */
	  for(k=0;k<ndofs*ndofs;k++)
	    {
	      stiff_loc[k]=0.0;
	      //stiff_loc2[k]=0.0;
	    }
	  for(k=0;k<ndofs;k++)
	    {
	      f_loc[k]=0.0;
	      f_loc1[k]=0.0;
	      f_loc2[k]=0.0;
	      f_loc3[k]=0.0;
	      f_loc4[k]=0.0;
	      f_loc5[k]=0.0;
	    }
	  
	  base_q=apr_get_base_type(Field_id,lista[j].patch[i]);
	  //printf("Normalne base_q=%d",base_q);
	  /* prepare data for gaussian integration */
	  apr_set_quadr_3D(base_q, &pdeg, &ngauss, xg, wg);
	  
	  /* get the most recent solution degrees of freedom */
	  sol_vec_id = 1;
	  apr_get_el_dofs(Field_id, lista[j].patch[i], sol_vec_id, dofs_loc);
	  
	  for (ki=0;ki<ngauss;ki++)
	  {
	      
	      /* at the gauss point, compute basis functions, determinant etc*/
	      iaux = 2; /* calculations with jacobian but not on the boundary */
	      
	      determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base_q, &xg[3*ki],node_coor,dofs_loc, base_phi,base_dphix,base_dphiy,base_dphiz, xcoor,u_val,u_x,u_y,u_z,NULL);
	      
	      vol = determ * wg[ki];
	      
	      Epsilon[0]=u_x[0]; //Exx
	      Epsilon[1]=u_y[1]; //Eyy
	      Epsilon[2]=u_z[2]; //Ezz
	      Epsilon[3]=1/2.*(u_y[0]+u_x[1]); //Exy
	      Epsilon[4]=1/2.*(u_z[0]+u_x[2]); //Exz
	      Epsilon[5]=1/2.*(u_z[1]+u_y[2]); //Eyz
	      
	      //  printf("Epsilony: Exx=%lf,Eyy=%lf,Ezz=%lf,Exy=%lf,Exz=%lf,Eyz=%lf\n",Epsilon[0],Epsilon[1],Epsilon[2],Epsilon[3],Epsilon[4],Epsilon[5]);
	      
	      if(pdv_problem.sigma==1)
	      {
			Wsp=Eyoung/((1+Poisni)*(1-2*Poisni));

			Sigma[0]=(1-Poisni)*Epsilon[0]+Poisni*Epsilon[1]+Poisni*Epsilon[2];
			Sigma[0]=Wsp*Sigma[0]; //Sxx

			Sigma[1]=Poisni*Epsilon[0]+(1-Poisni)*Epsilon[1]+Poisni*Epsilon[2];
			Sigma[1]=Wsp*Sigma[1]; //Syy

			Sigma[2]=Poisni*Epsilon[0]+Poisni*Epsilon[1]+(1-Poisni)*Epsilon[2];
			Sigma[2]=Wsp*Sigma[2]; //Szz

			Sigma[3]=2*(1/2.-Poisni)*Epsilon[3];
			Sigma[3]=Wsp*Sigma[3]; //Sxy

			Sigma[4]=2*(1/2.-Poisni)*Epsilon[4];
			Sigma[4]=Wsp*Sigma[4]; //Sxz

			Sigma[5]=2*(1/2.-Poisni)*Epsilon[5];
			Sigma[5]=Wsp*Sigma[5]; //Syz
	      }
	      else
	      {
	    	for(k=0;k<6;k++)
	    		Sigma[k]=Epsilon[k];
	      }
	      //printf("Sxx=%lf,Syy=%lf,Szz=%lf,Sxy=%lf,Sxz=%lf,Syz=%lf\n",Sigma[0],Sigma[1],Sigma[2],Sigma[3],Sigma[4],Sigma[5]);

	      for(ieq=0;ieq<1;ieq++)
	      {
			kk=(ieq*ndofs+ieq)*num_shap;
			for (jdofs=0;jdofs<num_shap;jdofs++) {
			  for (idofs=0;idofs<num_shap;idofs++) {
				/* mass matrix */
				stiff_loc[kk+idofs] += base_phi[jdofs] * base_phi[idofs] * vol;
			  }/* idofs */
			  kk+=ndofs;
			} /* jdofs */
	      }/* ieq */


	      kk=0;
	      for (idofs=0;idofs<num_shap;idofs++)
	      {
			f_loc[kk] += Sigma[0] * base_phi[idofs] * vol;
			kk++;
	      }

	      kk=0;
	      for (idofs=0;idofs<num_shap;idofs++)
	      {
	    	f_loc1[kk] += Sigma[1] * base_phi[idofs] * vol;
			kk++;
	      }
	      
	      kk=0;
	      for (idofs=0;idofs<num_shap;idofs++)
	      {
	    	f_loc2[kk] += Sigma[2] * base_phi[idofs] * vol;
	    	kk++;
	      }
	      
	      kk=0;
	      for (idofs=0;idofs<num_shap;idofs++)
	      {
	    	f_loc3[kk] += Sigma[3] * base_phi[idofs] * vol;
	    	kk++;
	      }
	      
	      kk=0;
	      for (idofs=0;idofs<num_shap;idofs++)
	      {
	    	f_loc4[kk] += Sigma[4] * base_phi[idofs] * vol;
	    	kk++;
	      }

	      kk=0;
	      for (idofs=0;idofs<num_shap;idofs++)
	      {
	    	f_loc5[kk] += Sigma[5] * base_phi[idofs] * vol;
		    kk++;
	      }
	      
	    }//ngauss
	  
	  /*kbw
	    kk=0;
	    printf("\nMacierz Element %d:\n",lista[j].patch[i]);
	    for (jdofs=0;jdofs<ndofs;jdofs++)
	    {
	    for (idofs=0;idofs<ndofs;idofs++)
	    {
	    printf(" %.4lf",stiff_loc[kk+idofs]);
	    }
	    kk+=ndofs;
	    printf("\n");
	    }
	    
	    printf("\nMacierz S:\n");
	    for (idofs=0;idofs<ndofs;idofs++)
	    {
	    printf(" %.4lf",f_loc[idofs]);
	    }
	    printf("\n\n");
	    /*kew*/
	  
	  Nrdofs_loc=24;
	  
	  /* obligatory procedure to fill Lists of dof_ents and rewite SM and RHSV */
	  apr_get_stiff_mat_data(Field_id, lista[j].patch[i], Comp_sm, 'N', 0, 1 , &Nr_dof_ent,
				 List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
				 &Nrdofs_loc, stiff_loc, f_loc);
	  
	  apr_get_stiff_mat_data(Field_id, lista[j].patch[i], Comp_sm2, 'N', 0, 1 , &Nr_dof_ent,
				 List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
				 &Nrdofs_loc, stiff_loc, f_loc1);
	  
	  apr_get_stiff_mat_data(Field_id, lista[j].patch[i], Comp_sm2, 'N', 0, 1 , &Nr_dof_ent,
				 List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
				 &Nrdofs_loc, stiff_loc, f_loc2);
	  
	  apr_get_stiff_mat_data(Field_id, lista[j].patch[i], Comp_sm2, 'N', 0, 1 , &Nr_dof_ent,
				 List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
				 &Nrdofs_loc, stiff_loc, f_loc3);
	  
	  apr_get_stiff_mat_data(Field_id, lista[j].patch[i], Comp_sm2, 'N', 0, 1 , &Nr_dof_ent,
				 List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
				 &Nrdofs_loc, stiff_loc, f_loc4);
	  
	  apr_get_stiff_mat_data(Field_id, lista[j].patch[i], Comp_sm2, 'N', 0, 1 , &Nr_dof_ent,
				 List_dof_ent_type, List_dof_ent_id, List_dof_ent_nrdofs,
				 &Nrdofs_loc, stiff_loc, f_loc5);
	  
	  
	  tabtrans=(int*)malloc((Nrdofs_loc+1)*sizeof(int));
	  
	  for(k=0;k<Nrdofs_loc;k++)
	  {
	      for(l=0;l<lista[j].size_stiff_mat;l++)
	      {
	    	  if(List_dof_ent_id[k]==lista[j].posglob[l])
	    		  tabtrans[k]=l;
	      }
	  }
	  
	  //assembling
	  //printf("\nLocal->patch\n");
	  for(idofs=0;idofs<Nrdofs_loc;idofs++)
	  {
	     for(jdofs=0;jdofs<Nrdofs_loc;jdofs++)
	     {
	    	 stiff_patch[tabtrans[idofs]+lista[j].size_stiff_mat*tabtrans[jdofs]]+=stiff_loc[idofs+jdofs*Nrdofs_loc];   //tu ma byc +=
	    	 //printf("[%d][%d](%d)->[%d][%d]\n",idofs,jdofs,idofs+Nrdofs_loc*jdofs,tabtrans[idofs],tabtrans[jdofs]);
	     }
	  }
	  
	  for(idofs=0;idofs<Nrdofs_loc;idofs++)
	    f_patch[0][tabtrans[idofs]]+=f_loc[idofs];
	  for(idofs=0;idofs<Nrdofs_loc;idofs++)
	    f_patch[1][tabtrans[idofs]]+=f_loc1[idofs];
	  for(idofs=0;idofs<Nrdofs_loc;idofs++)
	    f_patch[2][tabtrans[idofs]]+=f_loc2[idofs];
	  for(idofs=0;idofs<Nrdofs_loc;idofs++)
	    f_patch[3][tabtrans[idofs]]+=f_loc3[idofs];
	  for(idofs=0;idofs<Nrdofs_loc;idofs++)
	    f_patch[4][tabtrans[idofs]]+=f_loc4[idofs];
	  for(idofs=0;idofs<Nrdofs_loc;idofs++)
	    f_patch[5][tabtrans[idofs]]+=f_loc5[idofs];
	  
	  /*fk
	    printf("\nPatch for node:%d has %d nodes\n",j, lista[j].size_stiff_mat);
	    
	    for(k=0;k<lista[j].size_stiff_mat;k++)
	    printf("%d\t", lista[j].posglob[k]);
	    printf("\n");
	    
	    printf("Nrdofs_loc=%d\nList_dof_ent_id:\n",Nrdofs_loc);
	    for (idofs=0;idofs<Nrdofs_loc;idofs++) { //for each row!!!!
	    printf("%d\t",List_dof_ent_id[idofs]);
	    }
	    printf("\n");
	    
	    printf("Transfer table\n:");
	    for (idofs=0;idofs<Nrdofs_loc;idofs++)
	    printf("%d\t",tabtrans[idofs]);
	    printf("\n");
	    
	    printf("\nElement %d: Modified stiffness matrix:\n",lista[j].patch[i]);
	    for (idofs=0;idofs<Nrdofs_loc;idofs++) { //for each row!!!!
	    for (jdofs=0;jdofs<Nrdofs_loc;jdofs++) { // for each element in row !!!
	    printf("%8.4lf",stiff_loc[idofs+jdofs*(Nrdofs_loc)]);
	    }
	    printf("\n");
	    }
	    
	    printf("\nPatch for node %d stiffness matrix:\n",j);
	    for (idofs=0;idofs<lista[j].size_stiff_mat;idofs++) { //for each row!!!!
	    for (jdofs=0;jdofs<lista[j].size_stiff_mat;jdofs++) { // for each element in row !!!
	    printf("%8.4lf",stiff_patch[idofs][jdofs]);
	    }
	    printf("\n");
	    }
	    
	    printf("Element %d: Rhs_vect:\n",lista[j].patch[i]);
	    for (idofs=0;idofs<Nrdofs_loc;idofs++)
	    printf("%8.4lf",f_loc[idofs]);
	    printf("\n");
	    for (idofs=0;idofs<Nrdofs_loc;idofs++)
	    printf("%8.4lf",f_loc1[idofs]);
	    printf("\n");
	    for (idofs=0;idofs<Nrdofs_loc;idofs++)
	    printf("%8.4lf",f_loc2[idofs]);
	    printf("\n");
	    for (idofs=0;idofs<Nrdofs_loc;idofs++)
	    printf("%8.4lf",f_loc3[idofs]);
	    printf("\n");
	    for (idofs=0;idofs<Nrdofs_loc;idofs++)
	    printf("%8.4lf",f_loc4[idofs]);
	    printf("\n");
	    for (idofs=0;idofs<Nrdofs_loc;idofs++)
	    printf("%8.4lf",f_loc5[idofs]);
	    printf("\n");
	    
	  /**/
	  
	  free(tabtrans);
	  i++;
	  
	} //loop over elements in patch
      
    ndofs1=lista[j].size_stiff_mat;
      /*
	 printf("\nPatch for node %d stiffness matrix:\n",j);
	 for (idofs=0;idofs<lista[j].size_stiff_mat;idofs++) { //for each row!!!!
	 for (jdofs=0;jdofs<lista[j].size_stiff_mat;jdofs++) { // for each element in row !!!
	 printf("%8.4lf",stiff_patch[idofs+jdofs*ndofs1]);
	 }
	 printf("\n");
	 }
	 /*
	 printf("\nRhs Patch for node %d:\n",j);
	 for (idofs=0;idofs<6;idofs++) { //for each row!!!!
	 for (jdofs=0;jdofs<lista[j].size_stiff_mat;jdofs++) { // for each element in row !!!
	 printf("%8.4lf",f_patch[idofs][jdofs]);
	 }
	 printf("\n");
	 }
	 /**/
      dgetrf_(&ndofs1,&ndofs1,stiff_patch,&ndofs1,ipiv,&iaux);
      /*
	 printf("\nSfaktoryzowany Patch for node %d stiffness matrix:\n",j);
	 for (idofs=0;idofs<lista[j].size_stiff_mat;idofs++) { //for each row!!!!
	 for (jdofs=0;jdofs<lista[j].size_stiff_mat;jdofs++) { // for each element in row !!!
	 printf("%8.4lf",stiff_patch[idofs+jdofs*ndofs1]);
	 }
	 printf("\n");
	 }
	 
      */
      for(idofs=0;idofs<6;idofs++)
	dgetrs_("N",&ndofs1,&ione,stiff_patch,&ndofs1,ipiv,&f_patch[idofs][0],&ndofs1,&iaux);
      /*
	printf("\nObliczone wektory for node %d stiffness matrix:\n",j);
	for (idofs=0;idofs<6;idofs++)
	{ //for each row!!!!
	for (jdofs=0;jdofs<lista[j].size_stiff_mat;jdofs++)
	{ // for each element in row !!!
	printf("%8.4lf",f_patch[idofs][jdofs]);
	}
	printf("\n");
	}
	/**/
      for (jdofs=0;jdofs<lista[j].size_stiff_mat;jdofs++)
	{
	  if(lista[j].posglob[jdofs]==j)
	    {
	      for (idofs=0;idofs<6;idofs++)
		{
		  lista[j].sigma[idofs]=f_patch[idofs][jdofs];
		}
	    }
	}
      /*
	printf("\nSigmy dla %d:\n",j);
	for (idofs=0;idofs<6;idofs++)
	printf("%8.4lf",lista[j].sigma[idofs]);
	printf("\n");
	/**/
      //printf("\n");
    }//petla po j
  
  j=0;
  while((j=mmr_get_next_node_all(mesh_id, j))!=0)
    {
      if(lista[j].size==0)
	{
	  //printf("node=%d\n",j);
	  apr_get_constr_data(Field_id,j,constr);
	  /*
	    printf("node %d jest zawieszony na %d nodach:\n",j,constr[0]);
	    for(i=1;i<=constr[0];i++)
	    printf("%d\t",constr[i]);
	    printf("\n");
	    /**/
	  for(jdofs=1;jdofs<=constr[0];jdofs++)
	    {
	      for (idofs=0;idofs<6;idofs++)
		{
		  lista[j].sigma[idofs]+=1./constr[0]*lista[constr[jdofs]].sigma[idofs];
		  //printf("lista[%d].sigma[%d](%lf)+=1./%d(%lf)*lista[%d].sigma[%d](%lf)\n",j,idofs,lista[j].sigma[idofs],constr[0],1./constr[0],constr[jdofs],idofs,lista[constr[jdofs]].sigma[idofs]);
		  
		}
	    }
	}
    }
  
  free(List_dof_ent_type);
  free(List_dof_ent_id);
  free(List_dof_ent_nrdofs);
  free(f_loc);
  free(f_loc1);
  free(f_loc2);
  free(f_loc3);
  free(f_loc4);
  free(f_loc5);
  free(stiff_loc);
  free(wg);
  free(xg);
  free(base_phi);
  free(base_dphix);
  free(base_dphiy);
  free(base_dphiz);
  free(dofs_loc);
  free(stiff_patch);
  for(i=0;i<6;i++)
    free(f_patch[i]);
  free(f_patch);
  
  return 1;
}

/*---------------------------------------------------------
pdr_err_indi - to return error indicator for an element,
	in the current version based on the knowledge of
	the exact solution
----------------------------------------------------------*/
double pdr_err_indi (	/* returns error indicator for an element */
        int Problem_id,	/* in: data structure to be used  */
	int Mode,	/* in: mode of operation */
	int El		/* in: element number */
)
{
  double ni=0,err[6],err2,ni2=0;
  int i,ki,j,k,mat;
  int field_id,mesh_id,pdeg,num_shap,sol_vec_id,iaux,nreq;
  int base_q;		/* type of basis functions for quadrilaterals */
  int el_nodes[MMC_MAXELVNO+1];        /* list of nodes of El */
  double node_coor[3*MMC_MAXELVNO];  /* coord of nodes of El */
  double dofs_loc[APC_MAXELSD]; /* element solution dofs */
  double xcoor[3];      /* global coord of gauss point */
  double u_val[PDC_NREQ]; /* computed solution */
  double u_x[PDC_NREQ];   /* gradient of computed solution */
  double u_y[PDC_NREQ];   /* gradient of computed solution */
  double u_z[PDC_NREQ];   /* gradient of computed solution */
  double base_phi[APC_MAXELVD];    /* basis functions */
  double base_dphix[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];  /* y-derivatives of basis function */
  double determ;
  double Wsp;
  double Sigma[7],Epsilon[7];
  int ngauss;           /* number of gauss points */
  double xg[3000];   	/* coordinates of gauss points in 3D */
  double wg[1000];      /* gauss weights */
  double vol;           /* volume for integration rule */
  double Eyoung=0,Poisni=0;
  double sigmael[7][7];
  double sigmagw[7];
  double sr=0,src=0,srsigm=0,kus=0;
  double area;
  
  field_id = Problem_id;
  mesh_id = apr_get_mesh_id(field_id);
  base_q=apr_get_base_type(field_id,El);
  nreq = apr_get_nreq(field_id);
  
  for(j=0;j<7;j++)
    sigmagw[j]=0.0;
  
  //printf("\nErr indi Element %d:\n",El);
  
  /* find degree of polynomial and number of element scalar dofs */
  apr_get_el_pdeg(field_id, El, &pdeg);
  
  /* get the coordinates of the nodes of El in the right order */
  mmr_el_node_coor(mesh_id,El,el_nodes,node_coor);
  
  num_shap = apr_get_el_pdeg_numshap(field_id,El, &pdeg);
  //ndofs = num_shap*1;*
  
  /* prepare data for gaussian integration */
  apr_set_quadr_3D(base_q, &pdeg, &ngauss, xg, wg);
  
  //printf("Nowy ngauss=%d, pdeg=%d\n",ngauss,pdeg);

  /* get the most recent solution degrees of freedom */
  sol_vec_id = 1;
  apr_get_el_dofs(field_id, El, sol_vec_id, dofs_loc);
  
  mat=mmr_el_groupID(mesh_id,El);
  
  //printf("Element %d material %d\n",lista[j].patch[i],mat);
  for(k=0;k<matnum;k++)
    {
      if(pdv_material[k].id==mat)
	{
	  Eyoung=pdv_material[k].Eyoung;
	  Poisni=pdv_material[k].Poisni;
	  //printf("Eyoung=%.2lf, Poisni=%.2lf\n",Eyoung,Poisni);
	}
    }
  
  for(j=0;j<6;j++)
    {
      k=0;
      for(i=1;i<=el_nodes[0];i++)
	{
	  sigmael[j][k]=lista[el_nodes[i]].sigma[j];
	  //printf("Sigmael[%d][%d]=lista[%d].sigma[%d](%lf)\n",j,k,el_nodes[i],j,lista[el_nodes[i]].sigma[j]);
	  k++;
	}
    }
  
  for (ki=0;ki<ngauss;ki++)
    {
      /* at the gauss point, compute basis functions, determinant etc*/
      iaux = 2; /* calculations with jacobian but not on the boundary */
      
      determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base_q, &xg[3*ki],node_coor,dofs_loc, base_phi,base_dphix,base_dphiy,base_dphiz, xcoor,u_val,u_x,u_y,u_z,NULL);
      area=determ*wg[ki];
      
      Epsilon[0]=u_x[0]; //Exx
      Epsilon[1]=u_y[1]; //Eyy
      Epsilon[2]=u_z[2]; //Ezz
      Epsilon[3]=1/2.*(u_y[0]+u_x[1]); //Exy
      Epsilon[4]=1/2.*(u_z[0]+u_x[2]); //Exz
      Epsilon[5]=1/2.*(u_z[1]+u_y[2]); //Eyz
      
      //printf("Nowe Epsilony: Exx=%lf,Eyy=%lf,Ezz=%lf,Exy=%lf,Exz=%lf,Eyz=%lf\n",Epsilon[0],Epsilon[1],Epsilon[2],Epsilon[3],Epsilon[4],Epsilon[5]);
      
      if(pdv_problem.sigma==1)
      {
	
	Wsp=Eyoung/((1+Poisni)*(1-2*Poisni));
	
	Sigma[0]=(1-Poisni)*Epsilon[0]+Poisni*Epsilon[1]+Poisni*Epsilon[2];
	Sigma[0]=Wsp*Sigma[0]; //Sxx
	
	Sigma[1]=Poisni*Epsilon[0]+(1-Poisni)*Epsilon[1]+Poisni*Epsilon[2];
	Sigma[1]=Wsp*Sigma[1]; //Syy
	
	Sigma[2]=Poisni*Epsilon[0]+Poisni*Epsilon[1]+(1-Poisni)*Epsilon[2];
	Sigma[2]=Wsp*Sigma[2]; //Szz
	
	Sigma[3]=2*(1/2.-Poisni)*Epsilon[3];
	Sigma[3]=Wsp*Sigma[3]; //Sxy
	
	Sigma[4]=2*(1/2.-Poisni)*Epsilon[4];
	Sigma[4]=Wsp*Sigma[4]; //Sxz
	
	Sigma[5]=2*(1/2.-Poisni)*Epsilon[5];
	Sigma[5]=Wsp*Sigma[5]; //Syz
      }
      else
      {
    	  for(k=0;k<6;k++)
    		  Sigma[k]=Epsilon[k];
      }
      
      for(i=0;i<6;i++)
	determ = apr_elem_calc_3D(iaux, 1, &pdeg, base_q, &xg[3*ki], node_coor,&sigmael[i][0],base_phi,base_dphix,base_dphiy,base_dphiz, xcoor,&sigmagw[i],NULL,NULL,NULL,NULL);
      
      //error z wzoru bez macierzy D
      err2=0;
      for(i=0;i<6;i++)
	{
	  err2+=(sigmagw[i]-Sigma[i])*(sigmagw[i]-Sigma[i]);
	}
      
      ni2+=area*err2;
      //ni2=ni2*area;
      
      //error z mathcada zgodnie z wzorem z artykulu nr 3
      for(i=0;i<6;i++)
	err[i]=(sigmagw[i]-Sigma[i])*(sigmagw[i]-Sigma[i]);
      
      ni+=area*((1/Eyoung)*(err[0]*err[0] - 2*err[0]*err[1]*Poisni - 2*err[0]*err[2]*Poisni + err[1]*err[1] - 2*err[1]*err[2]*Poisni + err[2]*err[2] + 2*err[5]*err[5] + 2*err[5]*err[5]*Poisni + 2*err[4]*err[4] + 2*err[4]*err[4]*Poisni + 2*err[3]*err[3] + 2*err[3]*err[3]*Poisni));
      
      //kwadrat sigmy
      for(i=0;i<6;i++)
	srsigm+=Sigma[i]*Sigma[i];
      src+=area*srsigm;
      
    }
  //src=sqrt(src);
  
  //printf("Srednia sigma z elementow=%lf\n",src);
  
  //printf("Error procentowy: %lf\n",ni/src*100);
  
  //printf("\nError indicator dla elementu %d = %lf, area=%lf\n",El,ni,area);
  
  return(ni);
}


/*---------------------------------------------------------
pdr_adapt - to enforce adaptation strategy for a given problem
---------------------------------------------------------*/
int pdr_adapt( /* returns: >0 - success, <=0 - failure */
  int field_id       /* in: problem data structure to be used */
  )
{

/* local variables */
  int mesh_id;
  int	 type;		/* strategy number for adaptation */
  int	 interval;	/* number of time steps between adaptations */
  double eps;		/* eps is assumed to be a global tolerance level */
  double el_eps;	/* coefficient for choosing elements to adapt */
  double fam_error;	/* family error */
  double ratio;		/* ratio of errors for derefinements */
  double *error_indi;	/* array with elements errors */
  int nr_ref, *list_ref;/* number and list of elements to refine */
  int nr_deref, *list_deref;/* number and list of elements to derefine */
  int nmel,nrel;/* number of elements: total, active */
  int nmfa,nrfa;/* number of faces: total, active */
  int nmed,nred;/* number of edges: total, active */
  int nmno,nrno;/* number of nodes: total, active */
  int father, elsons[MMC_MAXELSONS+1]; /* family information */

/* auxiliary variables */
  int i, iel, iaux, name, ison, iprob, gen_el;
  int iprint=0;
  int nno,mat,*errno;;
  double err,perc,deriv;
  double norm,sumnor,fam_norm=0;
  double fam_hsize;
  int fam_mate=0;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* get formulation parameters */
  name=2;
  mesh_id=apr_get_mesh_id(field_id);
  pdr_zzhu_error(field_id);
  //err=(double*)calloc(matnum,sizeof(double));
  //errno=(int*)calloc(matnum,sizeof(int));
  //sumnor=(double*)calloc(matnum,sizeof(double));
 iel=0;
 err=0;
  while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0)
 {
  	err+=sqrt(pdr_err_indi(field_id,2,iel));
  	//printf("Dla elementu %d err=%.15e\n",iel,sqrt(pdr_err_indi(field_id,2,iel)));
  }
  //err=sqrt(err);

  //printf("\n\nZienkiewicz-Zhu error estimator = %lf\n\n",err);

/* get necessary mesh parameters */
  i=1; nrno=mmr_mesh_i_params(mesh_id,i);
  i=2; nmno=mmr_mesh_i_params(mesh_id,i);
  i=5; nred=mmr_mesh_i_params(mesh_id,i);
  i=6; nmed=mmr_mesh_i_params(mesh_id,i);
  i=9; nrfa=mmr_mesh_i_params(mesh_id,i);
  i=10; nmfa=mmr_mesh_i_params(mesh_id,i);
  i=13; nrel=mmr_mesh_i_params(mesh_id,i);
  i=14; nmel=mmr_mesh_i_params(mesh_id,i);

  //for freeing the list
  nno=nrno;

/* get adaptation parameters */

  type=1;
  interval=1;
  eps=err/nrel;
  //perc=0.80;

  ratio=pdv_problem.adpt.ratio;
  deriv=pdv_problem.adpt.eps;

  iprint=2;

  if(iprint>1){
      printf("\nBefore adaptation.\n");
      printf("Parameters (number of active, maximal index):\n");
      printf("Elements: nrel %d, nmel %d\n", nrel, nmel);
      printf("Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa);
      printf("Edges:    nred %d, nmed %d\n", nred, nmed);
      printf("Nodes:    nrno %d, nmno %d\n", nrno, nmno);
  }

  /* uniform refinement */
  if (type == -1) {

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);

    /* perform uniform refinement with mesh regularity check*/
    i=apr_refine(mesh_id,MMC_DO_UNI_REF);

    /* project DOFS between generations */
    iaux=-1;
    /* project DOFS between generations */
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);

    apr_check_field(field_id);

  }/* end uniform refinement */
  else {/* adaptive refinement with error indicators */

/* set element limit for errors (for equidistribution principle) */
/* here: eps - global tolerance */
/* (el_eps is sometimes chosen as percentage of max_iel error[iel])
    eps *= eps;
    el_eps = eps/nrel;
*/

	//el_eps = eps;

/* allocate space for array with error indicators */
    error_indi=utr_dvector(nmel+1,"error_indi in adapt");
    //norm=utr_dvector(nmel+1,"error norm in adapt");
    nr_ref=0;
    list_ref=utr_ivector(nmel+1,"list_ref in adapt");
    nr_deref=0;
    list_deref=utr_ivector(nmel+1,"list_deref in adapt");

/*kbw
printf("In adapt: type %d, interval %d, eps %lf, ratio %lf\n",
type, interval, eps, ratio);
/*kew*/

/* compute error indicators for all active elements */
    iel=0;
    //smieci
    double er1,er2;
    int licz1,licz2;
    er1=er2=licz1=licz2=0;

    while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0){

	error_indi[iel]=sqrt(pdr_err_indi(field_id, 2, iel));
	if(mmr_el_groupID(mesh_id,iel)==1)
	{
		er1+=error_indi[iel];
		//printf("er1=%lf\n",error_indi[iel]);
		licz1++;
	}
	else if(mmr_el_groupID(mesh_id,iel)==2)
	{
		er2+=error_indi[iel];
		licz2++;
	}

/*kbw
printf("in active element %d - error_indi %20.15lf <> %20.15lf\n",
iel,error_indi[iel],el_eps);
/*kew*/

    }
    //printf("sredni error w mat1=%lf a w mat2=%lf eps=%lf\n",er1/licz1,er2/licz2,eps);

/* create lists of elements for refinements and derefinements */
/* loop over all active elements */
    iel=0;
    while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0){

    mat=mmr_el_groupID(mesh_id,iel);

/* if error small enough and the family not yet considered */
	if((mmr_el_fam(mesh_id,iel,NULL,NULL)>0 && error_indi[iel]<eps*deriv && error_indi[iel]>-1))
	{

/*kbw
printf("2in active element %d - error_indi %20.15lf <> %20.15lf\n",
iel,error_indi[iel],err[mat-1]);
/*kew*/

/* find father and sons */
	  father=mmr_el_fam(mesh_id,iel,NULL,NULL);
	  mmr_el_fam(mesh_id,father,elsons,NULL);

/* check whether all sons are active and compute the error for the family*/
	  iaux=0; fam_error=0.0; fam_norm=0.0;
	  for(ison=1;ison<=elsons[0];ison++){
	    if(mmr_el_status(mesh_id,elsons[ison])<=0) {
	      iaux=1;
	      break;
	    }
	    else {
	      fam_error+=error_indi[elsons[ison]];
/* indicate son should not be considered again */
	      if(error_indi[elsons[ison]]<eps*deriv)
	      {
	    	  error_indi[elsons[ison]] = -2;
	      }

	      //fam_hsize+=mmr_el_hsize(mesh_id,elsons[ison],NULL,NULL,NULL);
	      //printf("fam_mate=%d",fam_mate);



/*kbw
printf("in active element %d, father %d, son %d (%d)\n",
iel,father,elsons[ison],ison);
printf("in active element %d - error_indi %20.15lf <> %20.15lf\n",
elsons[ison],error_indi[elsons[ison]],err[mat-1]);
/*kew*/

	    }
	  }

	  fam_mate=0;

//	  if(mmr_el_fam(mesh_id,iel,NULL,NULL)>0 && mat==2)
//	  {
//		  fam_mate=0;
//		  father=mmr_el_fam(mesh_id,iel,NULL,NULL);
//	  	  mmr_el_fam(mesh_id,father,elsons,NULL);
//
//		  for(ison=1;ison<=elsons[0];ison++)
//		  {
//		  	    if(mmr_el_status(mesh_id,elsons[ison])<=0)
//		  	          break;
//		  	    else
//		  	          fam_mate+=mmr_el_groupID(mesh_id,elsons[ison]);
//
//		  }
//		  fam_mate=fam_mate/elsons[0];
//		  printf("fam_mate=%d\n",fam_mate);
//
//	  }

	  //fam_hsize=fam_hsize/elsons[0];
	  //printf("fam_hsize=%lf",fam_hsize);

/* if all sons active and family error is smaller than the limit */
	  if((iaux==0 && (fam_error<eps*deriv)))//||fam_mate==2)
	  {
/* if derefinement is not excluded because of irregularity constrained */
	    if(apr_limit_deref(field_id, iel)==APC_DEREF_ALLOWED){

/*kbw
  printf("element %d (%d) to derefine: fam_error %15.12lf < %15.12lf or fam_hsize %lf <2\n",
  elsons[1],nr_deref,fam_error,eps*deriv,fam_hsize);
/*kew*/


	      /* put first son (elsons[1]) on list to derefine */
	      list_deref[nr_deref]=elsons[1];
	      nr_deref++;
	    }

	  } /* end if all sons active and family error smaller than the limit */

	} /* end if error small in element */
	else if((error_indi[iel]>=eps*ratio)&&(mmr_el_hsize(mesh_id,iel,NULL,NULL,NULL)>=pdv_problem.gr.hsize))//&&(mat!=2))
	{

/*kbw
printf("element %d (%d) to refine: error_indi %15.12lf > %15.12lf size=%lf mate=%d\n",
iel,nr_ref,error_indi[iel],eps*ratio,mmr_el_hsize(mesh_id,iel,NULL,NULL,NULL),(mmr_el_groupID(mesh_id,iel)!=2));
/*kew*/

	  if(apr_limit_ref(field_id, iel)==APC_REF_ALLOWED){
	    /* put iel on list to refine */
	    list_ref[nr_ref]=iel;
	    nr_ref++;

	  }

	}

    } /* end loop over active elements */

/*kbw
    printf("List to derefine (%d elements): \n",nr_deref);
    for(iel=0;iel<nr_deref;iel++) printf("%d ",list_deref[iel]);
    printf("\n");
    //getchar();
/*kew*/

    /* DEREFINEMENTS */


/*kbw
    printf("List to derefine after exchange (%d elements): \n",nr_deref);
    for(iel=0;iel<nr_deref;iel++) printf("%d ",list_deref[iel]);
    printf("\n");
    //getchar();
/*kew*/

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);

    /* perform derefinements */
    for(iel=0;iel<nr_deref;iel++) {
      if(mmr_el_status(mesh_id, list_deref[iel])==MMC_ACTIVE){
	if(apr_limit_deref(field_id, list_deref[iel])==APC_DEREF_ALLOWED){
	  iaux=apr_derefine(mesh_id,list_deref[iel]);
	  if(iaux<0) return(iaux);
	}
      }
    }


    /* project DOFS between generations */
    iaux=-1;
    /* project DOFS between generations */
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);


    /* REFINEMENTS */

/*kbw
    printf("List to refine before update (%d elements): \n",nr_ref);
    for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
    printf("\n");
    getchar();
/*kew*/


/*kbw
      printf("List to refine after exchange (%d elements): \n",nr_ref);
      for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
      printf("\n");
      //getchar();
/*kew*/

    /* update list of refined elements due to irregularity constraint
    mmr_update_ref_list(mesh_id, &nr_ref, list_ref);
    */

/*kbw
    printf("List to refine after update (%d elements): \n",nr_ref);
    for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
    printf("\n");
    //getchar();
/*kew*/

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);

    /* next perform refinements */
    for(iel=0;iel<nr_ref;iel++) {
      if(mmr_el_status(mesh_id, list_ref[iel])==MMC_ACTIVE){
	if(apr_limit_ref(field_id, list_ref[iel])==APC_REF_ALLOWED){
	  iaux=apr_refine(mesh_id,list_ref[iel]);
	  if(iaux<0) return(iaux);
	}
      }
    }


    /* project DOFS between generations */
    iaux=-1;
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);

    apr_check_field(field_id);


/* free the space */
    //free(norm);
    free(error_indi);
    free(list_ref);
    free(list_deref);


  } /* end if not uniform refinements */

/* get necessary mesh parameters */
  i=1; nrno=mmr_mesh_i_params(mesh_id,i);
  i=2; nmno=mmr_mesh_i_params(mesh_id,i);
  i=5; nred=mmr_mesh_i_params(mesh_id,i);
  i=6; nmed=mmr_mesh_i_params(mesh_id,i);
  i=9; nrfa=mmr_mesh_i_params(mesh_id,i);
  i=10; nmfa=mmr_mesh_i_params(mesh_id,i);
  i=13; nrel=mmr_mesh_i_params(mesh_id,i);
  i=14; nmel=mmr_mesh_i_params(mesh_id,i);

  if(iprint>1){
      printf("\nAfter adaptation.\n");
      printf("Parameters (number of active, maximal index):\n");
      printf("Elements: nrel %d, nmel %d\n", nrel, nmel);
      printf("Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa);
      printf("Edges:    nred %d, nmed %d\n", nred, nmed);
      printf("Nodes:    nrno %d, nmno %d\n", nrno, nmno);
  }

  //global for freeing the list of patches
  //nrnode=nno;

/*	for(i=1;i<nno+1;i++)
		free(lista[i].patch);

	for(i=1;i<nno+1;i++)
		free(lista[i].sigma);

	free(lista);*/
	//free(err);
	//free(errno);
	//free(sumnor);

  return(1);

}

/*---------------------------------------------------------
pdr_adapt - to enforce adaptation strategy for a given problem
---------------------------------------------------------*/
int pdr_adapt2( /* returns: >0 - success, <=0 - failure */
  int field_id       /* in: problem data structure to be used */
  )
{

/* local variables */
  int mesh_id;
  int	 type;		/* strategy number for adaptation */
  int	 interval;	/* number of time steps between adaptations */
  double eps;		/* eps is assumed to be a global tolerance level */
  double el_eps;	/* coefficient for choosing elements to adapt */
  double fam_error;	/* family error */
  double ratio;		/* ratio of errors for derefinements */
  int *ref_indi;	/* array with elements indicator */
  int nr_ref, *list_ref;/* number and list of elements to refine */
  int nr_deref, *list_deref;/* number and list of elements to derefine */
  int nmel,nrel;/* number of elements: total, active */
  int nmfa,nrfa;/* number of faces: total, active */
  int nmed,nred;/* number of edges: total, active */
  int nmno,nrno;/* number of nodes: total, active */
  int father, elsons[MMC_MAXELSONS+1]; /* family information */

/* auxiliary variables */
  int i,j, iel, iaux, name, ison, iprob, gen_el;
  int iprint=0;
  int nno,mat,*errno;;
  double err,perc,deriv;
  double norm,sumnor,fam_norm=0;
  double fam_hsize;
  int fam_mate=0;
  int flag=0;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* get formulation parameters */
  type=1;
  name=2;
  mesh_id=apr_get_mesh_id(field_id);

/* get necessary mesh parameters */
  i=1; nrno=mmr_mesh_i_params(mesh_id,i);
  i=2; nmno=mmr_mesh_i_params(mesh_id,i);
  i=5; nred=mmr_mesh_i_params(mesh_id,i);
  i=6; nmed=mmr_mesh_i_params(mesh_id,i);
  i=9; nrfa=mmr_mesh_i_params(mesh_id,i);
  i=10; nmfa=mmr_mesh_i_params(mesh_id,i);
  i=13; nrel=mmr_mesh_i_params(mesh_id,i);
  i=14; nmel=mmr_mesh_i_params(mesh_id,i);

  //for freeing the list
  nno=nrno;

  pdr_create_patch(field_id);

  iprint=2;

  if(iprint>1){
      printf("\nBefore adaptation.\n");
      printf("Parameters (number of active, maximal index):\n");
      printf("Elements: nrel %d, nmel %d\n", nrel, nmel);
      printf("Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa);
      printf("Edges:    nred %d, nmed %d\n", nred, nmed);
      printf("Nodes:    nrno %d, nmno %d\n", nrno, nmno);
  }

  /* uniform refinement */
  if (type == -1) {

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);

    /* perform uniform refinement */
    i=apr_refine(mesh_id,MMC_DO_UNI_REF);

    /* project DOFS between generations */
    iaux=-1;
    /* project DOFS between generations */
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);

    apr_check_field(field_id);

  }/* end uniform refinement */
  else {/* adaptive refinement with error indicators */

	ref_indi=utr_ivector(nmel+2,"ref_indi in adapt");
    //norm=utr_dvector(nmel+1,"error norm in adapt");
    nr_ref=0;
    list_ref=utr_ivector(nmel+1,"list_ref in adapt");
    nr_deref=0;
    list_deref=utr_ivector(nmel+1,"list_deref in adapt");

    for(i=0;i<=nmel;i++)
    	ref_indi[i]=0;

/* create lists of elements for refinements and derefinements */
/* loop over all active elements */
    j=0;
    while((j=mmr_get_next_node_all(mesh_id, j))!=0)
    {
    	if(lista[j].size==0)
    		continue;
    	i=0;
    	flag=0;
    	while(lista[j].patch[i]!=0)
    	{
    		if(mmr_el_groupID(mesh_id,lista[j].patch[i])==2)
    			flag=1;
    		i++;
    	}
    	if(flag==1)
    	{
    		for(i=0;i<lista[j].size;i++)
    		{
    			if((mmr_el_groupID(mesh_id,lista[j].patch[i])==1)&&(mmr_el_hsize(mesh_id,lista[j].patch[i],NULL,NULL,NULL)>=pdv_problem.gr.hsize))
    			{
    				 if((apr_limit_ref(field_id, lista[j].patch[i])==APC_REF_ALLOWED)&&(ref_indi[lista[j].patch[i]]==0))
    				 {
    					    list_ref[nr_ref]=lista[j].patch[i];
    					    ref_indi[lista[j].patch[i]]=1;
    					    nr_ref++;
    				 }
    			}
    		}
    	}
    }

    iel=0;
    while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0)
    {
      mat=mmr_el_groupID(mesh_id,iel);
	  if(mmr_el_fam(mesh_id,iel,NULL,NULL)>0 && mat==2)
	  {
		  fam_mate=0;
		  father=mmr_el_fam(mesh_id,iel,NULL,NULL);
	  	  mmr_el_fam(mesh_id,father,elsons,NULL);

		  for(ison=1;ison<=elsons[0];ison++)
		  {
		  	    if(mmr_el_status(mesh_id,elsons[ison])<=0)
		  	          break;
		  	    else
		  	          fam_mate+=mmr_el_groupID(mesh_id,elsons[ison]);

		  }
		  fam_mate=fam_mate/elsons[0];
		  //printf("fam_mate=%d\n",fam_mate);

	  }
	  if(fam_mate==2)
	  {
	    if(apr_limit_deref(field_id, iel)==APC_DEREF_ALLOWED)
	    {
	      /* put first son (elsons[1]) on list to derefine */
	      //printf("fam_mate=%d\n",fam_mate);
	      list_deref[nr_deref]=elsons[1];
	      nr_deref++;
	    }
	  }
    } /* end loop over active elements */

/*kbw
    printf("List to derefine (%d elements): \n",nr_deref);
    for(iel=0;iel<nr_deref;iel++) printf("%d ",list_deref[iel]);
    printf("\n");
    //getchar();
/*kew*/

    /* DEREFINEMENTS */

/*kbw
    printf("List to derefine after exchange (%d elements): \n",nr_deref);
    for(iel=0;iel<nr_deref;iel++) printf("%d ",list_deref[iel]);
    printf("\n");
    //getchar();
/*kew*/

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);

    /* perform derefinements */
    for(iel=0;iel<nr_deref;iel++) {
      if(mmr_el_status(mesh_id, list_deref[iel])==MMC_ACTIVE){
	if(apr_limit_deref(field_id, list_deref[iel])==APC_DEREF_ALLOWED){
	  iaux=apr_derefine(mesh_id,list_deref[iel]);
	  if(iaux<0) return(iaux);
	}
      }
    }


    /* project DOFS between generations */
    iaux=-1;
    /* project DOFS between generations */
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);


    /* REFINEMENTS */

/*kbw
    printf("List to refine before update (%d elements): \n",nr_ref);
    for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
    printf("\n");
    getchar();
/*kew*/


/*kbw
      printf("List to refine after exchange (%d elements): \n",nr_ref);
      for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
      printf("\n");
      //getchar();
/*kew*/

    /* update list of refined elements due to irregularity constraint
    mmr_update_ref_list(mesh_id, &nr_ref, list_ref);
    */

/*kbw
    printf("List to refine after update (%d elements): \n",nr_ref);
    for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
    printf("\n");
    //getchar();
/*kew*/

    /* initialize refinement data structures */
    mmr_init_ref(mesh_id);

    /* next perform refinements */
    for(iel=0;iel<nr_ref;iel++) {
      if(mmr_el_status(mesh_id, list_ref[iel])==MMC_ACTIVE){
	if(apr_limit_ref(field_id, list_ref[iel])==APC_REF_ALLOWED){
	  iaux=apr_refine(mesh_id,list_ref[iel]);
	  if(iaux<0) return(iaux);
	}
      }
    }


    /* project DOFS between generations */
    iaux=-1;
    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

    /* restore consistency of mesh data structure and free space */
    mmr_final_ref(mesh_id);

    apr_check_field(field_id);


/* free the space */
    //free(norm);
    free(ref_indi);
    free(list_ref);
    free(list_deref);


  } /* end if not uniform refinements */

/* get necessary mesh parameters */
  i=1; nrno=mmr_mesh_i_params(mesh_id,i);
  i=2; nmno=mmr_mesh_i_params(mesh_id,i);
  i=5; nred=mmr_mesh_i_params(mesh_id,i);
  i=6; nmed=mmr_mesh_i_params(mesh_id,i);
  i=9; nrfa=mmr_mesh_i_params(mesh_id,i);
  i=10; nmfa=mmr_mesh_i_params(mesh_id,i);
  i=13; nrel=mmr_mesh_i_params(mesh_id,i);
  i=14; nmel=mmr_mesh_i_params(mesh_id,i);

  if(iprint>1){
      printf("\nAfter adaptation.\n");
      printf("Parameters (number of active, maximal index):\n");
      printf("Elements: nrel %d, nmel %d\n", nrel, nmel);
      printf("Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa);
      printf("Edges:    nred %d, nmed %d\n", nred, nmed);
      printf("Nodes:    nrno %d, nmno %d\n", nrno, nmno);
  }

  return(1);

}

/*---------------------------------------------------------
pdr_adapt - to enforce adaptation strategy for a given problem
---------------------------------------------------------*/
int pdr_constr( /* returns: >0 - success, <=0 - failure */
  int field_id       /* in: problem data structure to be used */
  )
{

	/* local variables */
	  int mesh_id;
	  int	 type;		/* strategy number for adaptation */
	  int	 interval;	/* number of time steps between adaptations */
	  double eps;		/* eps is assumed to be a global tolerance level */
	  double el_eps;	/* coefficient for choosing elements to adapt */
	  double fam_error;	/* family error */
	  double ratio;		/* ratio of errors for derefinements */
	  double *error_indi;	/* array with elements errors */
	  int nr_ref, *list_ref;/* number and list of elements to refine */
	  int nr_deref, *list_deref;/* number and list of elements to derefine */
	  int nmel,nrel;/* number of elements: total, active */
	  int nmfa,nrfa;/* number of faces: total, active */
	  int nmed,nred;/* number of edges: total, active */
	  int nmno,nrno;/* number of nodes: total, active */
	  int father, elsons[MMC_MAXELSONS+1]; /* family information */

	/* auxiliary variables */
	  int i, iel, iaux, name, ison, iprob, gen_el;
	  int iprint=0;
	  int nno,mat,*errno;;
	  double err,perc,deriv;
	  double norm,sumnor,fam_norm=0;
	  double fam_hsize;
	  int fam_mate=0;
	  int neig[6];

	  int licz;

	/*++++++++++++++++ executable statements ++++++++++++++++*/

	/* get formulation parameters */
	  name=2;
	  mesh_id=apr_get_mesh_id(field_id);

	  //err=(double*)calloc(matnum,sizeof(double));
	  //errno=(int*)calloc(matnum,sizeof(int));
	  //sumnor=(double*)calloc(matnum,sizeof(double));

	/* get necessary mesh parameters */
	  i=1; nrno=mmr_mesh_i_params(mesh_id,i);
	  i=2; nmno=mmr_mesh_i_params(mesh_id,i);
	  i=5; nred=mmr_mesh_i_params(mesh_id,i);
	  i=6; nmed=mmr_mesh_i_params(mesh_id,i);
	  i=9; nrfa=mmr_mesh_i_params(mesh_id,i);
	  i=10; nmfa=mmr_mesh_i_params(mesh_id,i);
	  i=13; nrel=mmr_mesh_i_params(mesh_id,i);
	  i=14; nmel=mmr_mesh_i_params(mesh_id,i);

	  //for freeing the list
	  nno=nrno;

	/* get adaptation parameters */

	  type=1;
	  interval=1;
	  //perc=0.80;

	  iprint=2;

	  if(iprint>1){
	      printf("\nBefore constrains check.\n");
	      printf("Parameters (number of active, maximal index):\n");
	      printf("Elements: nrel %d, nmel %d\n", nrel, nmel);
	      printf("Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa);
	      printf("Edges:    nred %d, nmed %d\n", nred, nmed);
	      printf("Nodes:    nrno %d, nmno %d\n", nrno, nmno);
	  }

	/* set element limit for errors (for equidistribution principle) */
	/* here: eps - global tolerance */
	/* (el_eps is sometimes chosen as percentage of max_iel error[iel])
	    eps *= eps;
	    el_eps = eps/nrel;
	*/
		//el_eps = eps;

	/* allocate space for array with error indicators */
	    //norm=utr_dvector(nmel+1,"error norm in adapt");
	    nr_ref=0;
	    list_ref=utr_ivector(nmel+1,"list_ref in constr");

	/* compute error indicators for all active elements */
	  iel=0;
	  nr_ref=0;
	  while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0)
	  {
		licz=0;
		mmr_el_eq_neig(mesh_id,iel,neig,NULL);
		//printf("Sasiedzi elementu %d:\n",iel);
		for(i=1;i<=neig[0];i++)
		{
			if(mmr_el_status(mesh_id,neig[i])==-1)
			{
				//printf("%d - %d\n",neig[i],mmr_el_status(mesh_id,neig[i]));
				licz++;
			}
		}
		//printf("licz=%d\n",licz);
		if(licz==2||licz==3)
		{
			//printf("Element %d to refine\n",iel);
			list_ref[nr_ref]=iel;
			nr_ref++;
		}

	  }
	  if(nr_ref==0)
		  return 5;

	    /* REFINEMENTS */

	/*kbw
	    printf("List to refine before update (%d elements): \n",nr_ref);
	    for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
	    printf("\n");
	    getchar();
	/*kew*/


	/*kbw
	      printf("List to refine after exchange (%d elements): \n",nr_ref);
	      for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
	      printf("\n");
	      //getchar();
	/*kew*/

	    /* update list of refined elements due to irregularity constraint
	    mmr_update_ref_list(mesh_id, &nr_ref, list_ref);
	    */

	/*kbw
	    printf("List to refine after update (%d elements): \n",nr_ref);
	    for(iel=0;iel<nr_ref;iel++) printf("%d ",list_ref[iel]);
	    printf("\n");
	    //getchar();
	/*kew*/

	    /* initialize refinement data structures */
	    mmr_init_ref(mesh_id);

	    /* next perform refinements */
	    for(iel=0;iel<nr_ref;iel++) {
	      if(mmr_el_status(mesh_id, list_ref[iel])==MMC_ACTIVE){
		if(apr_limit_ref(field_id, list_ref[iel])==APC_REF_ALLOWED){
		  iaux=apr_refine(mesh_id,list_ref[iel]);
		  if(iaux<0) return(iaux);
		}
	      }
	    }

	    /* project DOFS between generations */
	    iaux=-1;
	    if(nmno<mmr_mesh_i_params(mesh_id,2)) nmno=mmr_mesh_i_params(mesh_id,2);
	    if(nmed<mmr_mesh_i_params(mesh_id,6)) nmed=mmr_mesh_i_params(mesh_id,6);
	    if(nmfa<mmr_mesh_i_params(mesh_id,10)) nmfa=mmr_mesh_i_params(mesh_id,10);
	    if(nmel<mmr_mesh_i_params(mesh_id,14)) nmel=mmr_mesh_i_params(mesh_id,14);
	    apr_proj_dof_ref(field_id, iaux, nmel, nmfa, nmed, nmno);

	    /* restore consistency of mesh data structure and free space */
	    mmr_final_ref(mesh_id);

	    apr_check_field(field_id);

	/* get necessary mesh parameters */
	  i=1; nrno=mmr_mesh_i_params(mesh_id,i);
	  i=2; nmno=mmr_mesh_i_params(mesh_id,i);
	  i=5; nred=mmr_mesh_i_params(mesh_id,i);
	  i=6; nmed=mmr_mesh_i_params(mesh_id,i);
	  i=9; nrfa=mmr_mesh_i_params(mesh_id,i);
	  i=10; nmfa=mmr_mesh_i_params(mesh_id,i);
	  i=13; nrel=mmr_mesh_i_params(mesh_id,i);
	  i=14; nmel=mmr_mesh_i_params(mesh_id,i);

	  if(iprint>1){
	      printf("\nAfter constrains check.\n");
	      printf("Parameters (number of active, maximal index):\n");
	      printf("Elements: nrel %d, nmel %d\n", nrel, nmel);
	      printf("Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa);
	      printf("Edges:    nred %d, nmed %d\n", nred, nmed);
	      printf("Nodes:    nrno %d, nmno %d\n", nrno, nmno);
	  }

	  return(1);

}

int pdr_compute_eyoung(
		 int field_id
		 )
{
  
  //if fa_bc=neumann i sila >0 pobierz wertexy a potem pobierz przemieszczenia i oblicz eyounga
  
  int fa_type, fa_bc, bc_type;
  int mesh_id;
  int nfa,i,j,nno,nreq,all;
  double srx,sry,srz;
  double srcx,srcy,srcz;
  int nodes[MMC_MAXELVNO+1],constr[6];
  double u[MMC_MAXELVNO*PDC_NREQ+1];
  double dl=0, N=0, E=0, l=0, A=0;
  
  nreq=PDC_NREQ;
  
  
  //apr_read_ent_dofs(Field_id,APC_VERTEX,el_nodes[ino+1], nreq, Vect_id, &El_dofs_std[ino*nreq]);
  
  //	apr_get_el_dofs(field_id, El, sol_vec_id, dofs_loc);
  
  /*++++++++++++++++ executable statements ++++++++++++++++*/
  
  /* select the proper mesh */
  mesh_id = apr_get_mesh_id(field_id);
  
  
  srcx=0;
  //srcy=0;
  //srcz=0;
  all=0;
  nfa=0;
  while((nfa=mmr_get_next_act_face(mesh_id, nfa))!=0)
    {
      fa_bc=mmr_fa_bc(mesh_id,nfa);
      bc_type = pdr_get_bc_type(fa_bc);
      
      //printf("Sciana %d\n",nfa);
      
      if(bc_type==PDC_BC_NEUM)
	{
	  //printf("Sciana %d ma neumana\n",nfa);
	  for(i=0;i<bcnum;i++)
	    {
	      if(fa_bc==pdv_bc[i].type)
		{
		  if(pdv_bc[i].vec[0]!=0||pdv_bc[i].vec[1]!=0||pdv_bc[i].vec[2]!=0)
		    {
		      //printf("Sciana %d ma przylozona sile!\n",nfa);
		      
		      if(pdv_bc[i].vec[0]!=0)
			N=pdv_bc[i].vec[0];
		      else if(pdv_bc[i].vec[1]!=0)
			N=pdv_bc[i].vec[1];
		      else if(pdv_bc[i].vec[2]!=0)
			N=pdv_bc[i].vec[2];
		      
		      //printf("N=%lf",N);
		      
		      nno=mmr_fa_node_coor(mesh_id,nfa,nodes,NULL);
		      //		    				  for(j=0;j<nno;j++)
		      //		    					  printf("%d\t",nodes[j]);
		      //		    				  printf("\n");
		      for(j=0;j<nno;j++)
			{
			  if(lista[nodes[j]].size==0)
			    {
			      //printf("node=%d\n",nodes[j]);
			      apr_get_constr_data(field_id,nodes[j],constr);
			      //printf("nodes[%d]=%d,constr=%d\n",j,nodes[j],constr[0]);
			      continue;
			      
			      //rozwiazac sprawe constr
			      
			    }
			  apr_read_ent_dofs(field_id,PDC_VERTEX,nodes[j], nreq, 0, &u[j*nreq]);
			}
		      //		    				  printf("Dla face %d rozwiązania:\n",nfa);
		      //		    				  for(j=0;j<nno*nreq;j++)
		      //		    				  {
		      //		    					  printf("%lf\t",u[j]);
		      //		    					  if(j!=0&&((j+1)%3==0))
		      //		    						  printf("\n");
		      //		    				  }
		      
		      srx=0;
		      //sry=0;
		      //srz=0;
		      for(j=0;j<nno*nreq;j+=nreq)
			{
			  srx+=u[j];
			  //sry+=u[j+1];
			  //srz+=u[j+2];
			}
		      srx=srx/nno;
		      //sry=sry/nno;
		      //srz=srz/nno;
		      srcx+=srx;
		      //srcy+=sry;
		      //srcz+=srz;
		      all++;
		      //printf("srednie x=%lf,y=%lf,z=%lf\n",srx,sry,srz);
		      
		    }
		}
	    }
	  
	}
      
    }//nfa
  
  srcx=srcx/all;
  //srcy=srcy/all;
  //srcz=srcz/all;
  
  dl=srcx;
  E=fabs(N/dl);
  
  printf("E=%lf\n",E);
  
  //printf("N=%lf,srcx=%lf,srcy=%lf,srcz=%lf,all=%d\n",N,srcx,srcy,srcz,all);
  
  
  return 1;
}

int pdr_get_sigma(
		  int field_id
		  )
{
  int base_q;
  int iel,i,j,k,ki;
  int el_nodes[MMC_MAXELVNO+1];        /* list of nodes of El */
  double node_coor[3*MMC_MAXELVNO];  /* coord of nodes of El */
  double dofs_loc[APC_MAXELSD]; /* element solution dofs */
  double xcoor[3];      /* global coord of gauss point */
  double u_val[PDC_NREQ]; /* computed solution */
  double u_x[PDC_NREQ];   /* gradient of computed solution */
  double u_y[PDC_NREQ];   /* gradient of computed solution */
  double u_z[PDC_NREQ];   /* gradient of computed solution */
  double base_phi[APC_MAXELVD];    /* basis functions */
  double base_dphix[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];  /* y-derivatives of basis function */
  double determ;
  double Wsp;
  int El[2];
  double Sigma[7],Epsilon[7];
  int ngauss;           /* number of gauss points */
  double xg[3000];   	/* coordinates of gauss points in 3D */
  double wg[1000];      /* gauss weights */
  int pdeg, sol_vec_id,num_shap,mesh_id,iaux,mat,nreq;
  double Eyoung, Poisni;
  nreq=PDC_NREQ;
  
  mesh_id = apr_get_mesh_id(field_id);
  
  
  iel=0;
  while((iel=mmr_get_next_act_elem(mesh_id, iel))!=0)
    {
      
      El[0]=1;
      El[1]=iel;
      mmr_el_node_coor(mesh_id,iel,el_nodes,node_coor);
      
      apr_get_el_pdeg(field_id, lista[j].patch[i], &pdeg);
      
      base_q=apr_get_base_type(field_id,lista[j].patch[i]);
      
      num_shap = apr_get_el_pdeg_numshap(field_id,lista[j].patch[i],&pdeg);
      //ndofs = num_shap*1;*
      
      mat=mmr_el_groupID(mesh_id,iel);
      
      //printf("Element %d material %d\n",lista[j].patch[i],mat);
      for(k=0;k<matnum;k++)
	{
	  if(pdv_material[k].id==mat)
	    {
	      Eyoung=pdv_material[k].Eyoung;
	      Poisni=pdv_material[k].Poisni;
	      //printf("Eyoung=%.2lf, Poisni=%.2lf\n",Eyoung,Poisni);
	    }
	}
      
      /* prepare data for gaussian integration */
      apr_set_quadr_3D(base_q, &pdeg, &ngauss, xg, wg);
      
      /* get the most recent solution degrees of freedom */
      sol_vec_id = 1;
      apr_get_el_dofs(field_id, iel, sol_vec_id, dofs_loc);
      
      for (ki=0;ki<num_shap;ki++)
	{
	  // at the gauss point, compute basis functions, determinant etc
	  iaux = 2;  //calculations with jacobian but not on the boundary
	  
	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base_q, &node_coor[3*ki],node_coor,dofs_loc, base_phi,base_dphix,base_dphiy,base_dphiz, xcoor,u_val,u_x,u_y,u_z,NULL);
	  //apr_sol_xglob(field_id,&node_coor[3*ki],1,El,xcoor,u_val,u_x,u_y,u_z);
	  //area=determ*wg[ki];
	  
	  //printf("Punkt %.2lf %.2lf %.2lf\n",node_coor[3*ki],node_coor[3*ki+1],node_coor[3*ki+2]);
	  //printf("Node %d\n",el_nodes[ki+1]);
	  //printf ("%lf\t%lf\t%lf\n",u_val[0],u_val[1],u_val[2]);
	  
	  Epsilon[0]=u_x[0]; //Exx
	  Epsilon[1]=u_y[1]; //Eyy
	  Epsilon[2]=u_z[2]; //Ezz
	  Epsilon[3]=1/2.*(u_y[0]+u_x[1]); //Exy
	  Epsilon[4]=1/2.*(u_z[0]+u_x[2]); //Exz
	  Epsilon[5]=1/2.*(u_z[1]+u_y[2]); //Eyz
	  
	  //printf("Epsilony: Exx=%lf,Eyy=%lf,Ezz=%lf,Exy=%lf,Exz=%lf,Eyz=%lf\n",Epsilon[0],Epsilon[1],Epsilon[2],Epsilon[3],Epsilon[4],Epsilon[5]);
	  
	  if(pdv_problem.sigma==1)
	  {
	    Wsp=Eyoung/((1+Poisni)*(1-2*Poisni));
	    
	    Sigma[0]=(1-Poisni)*Epsilon[0]+Poisni*Epsilon[1]+Poisni*Epsilon[2];
	    Sigma[0]=Wsp*Sigma[0]; //Sxx
	    
	    Sigma[1]=Poisni*Epsilon[0]+(1-Poisni)*Epsilon[1]+Poisni*Epsilon[2];
	    Sigma[1]=Wsp*Sigma[1]; //Syy
	    
	    Sigma[2]=Poisni*Epsilon[0]+Poisni*Epsilon[1]+(1-Poisni)*Epsilon[2];
	    Sigma[2]=Wsp*Sigma[2]; //Szz
	    
	    Sigma[3]=2*(1/2.-Poisni)*Epsilon[3];
	    Sigma[3]=Wsp*Sigma[3]; //Sxy
	    
	    Sigma[4]=2*(1/2.-Poisni)*Epsilon[4];
	    Sigma[4]=Wsp*Sigma[4]; //Sxz
	    
	    Sigma[5]=2*(1/2.-Poisni)*Epsilon[5];
	    Sigma[5]=Wsp*Sigma[5]; //Syz
	  }
	  else
	  {
	    for(k=0;k<6;k++)
	      Sigma[k]=Epsilon[k];
	  }
	}//ki
      
      
    }//iel
  return 1;
}



/*---------------------------------------------------------
ut_dvector - to allocate a double vector: name[0..ncom-1]:
                  name=ut_dvector(ncom,error_text)
---------------------------------------------------------*/
double *ut_dvector( /* return: pointer to allocated vector */
	int ncom,  	/* in: number of components */
	char error_text[]/* in: error text to be printed */
	)
{

  double *v;

  v = (double *) malloc (ncom*sizeof(double));
  if(!v){
    printf("Not enough space for allocating vector: %s ! Exiting\n", error_text);
    exit(1);
  }
  return v;
}


/*---------------------------------------------------------
ut_ivector - to allocate an integer vector: name[0..ncom-1]:
                  name=ut_ivector(ncom,error_text)
---------------------------------------------------------*/
int *ut_ivector(    /* return: pointer to allocated vector */
	int ncom, 	/* in: number of components */
	char error_text[]/* in: error text to be printed */
	)
{

  int *v;

  v = (int *) malloc (ncom*sizeof(int));
  if(!v){
    printf("Not enough space for allocating vector: %s ! Exiting\n", error_text);
    exit(1);
  }
  return v;
}
