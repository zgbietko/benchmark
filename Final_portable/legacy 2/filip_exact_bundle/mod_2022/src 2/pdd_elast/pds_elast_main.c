/************************************************************************
File pds_elast_modules_main - main control routine for elasticity

Contains definitions of routines:

  main() - just for C99


------------------------------
History:
	08.2008 - Krzysztof Banas, pobanas@cyf-kr.edu.pl, initial version
	11.2010 - Filip Kruzel, fkruzel@pk.edu.pl, rc version
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include<string.h>

/* utilities - including simple time measurement library */
#include "uth_intf.h"

/* interface for all mesh manipulation modules */
#include "mmh_intf.h"

/* interface for all approximation modules */
#include "aph_intf.h"

#ifdef PARALLEL
/* interface of parallel mesh manipulation modules */
#include "mmph_intf.h"
/* interface for all parallel approximation modules */
#include "apph_intf.h"
/* interface for parallel communication modules */
#include "pch_intf.h"
#endif

/* interface for all solver modules */
#include "sih_intf.h"

/* problem dependent module interface */
#include "pdh_intf.h"

#include "./pdh_elast.h"

/* local functions declarations */

/* to test setting initial approximation field */
double Initial_condition(double* Coor);

/*---------------------------------------------------------
  pdr_init_problem - to initialize problem dependent data
----------------------------------------------------------*/
int pdr_init_problem(
  char* Work_dir,
  FILE *interactive_input,
  FILE *interactive_output
  );

/*---------------------------------------------------------
  pdr_error_test - to compute error norm for test examples
----------------------------------------------------------*/
double pdr_error_test( /* returns H1 norm of error for */
	         	/* for several known exact solutions */
  int Field_id,    /* in: approximation field ID  */
  FILE *interactive_output
  );

/*---------------------------------------------------------
  pdr_post_process -
----------------------------------------------------------*/
double pdr_post_process(
  int Field_id,    /* in: approximation field ID  */
  FILE *interactive_output
  );

/*** DEFINITIONS ***/
int main(int argc, char **argv)
{

/* local variables */
  FILE *problem_input, *problem_output;
  FILE *interactive_input, *interactive_output;
  FILE *grains;
  char grains_name[200];
  char problem_input_name[300], problem_output_name[300];
  char interactive_input_name[300];
  char interactive_output_name[300], tmp[100];
  char field_name[300];
  int problem_id;
  char c, arg[100], arg2[100], arg3[100]; /* string variable to read menu */
  int i, j,k, iaux, jaux, iprob, icount;
  int lel[100], nreq, iel, base_q;
  int field_id, mesh_id;
  double daux, eaux, t_wall, t_wall2, t_wall3;
  int max_gen, max_gen_diff;
  int nrno, nmno, nred, nmed, nrfa, nmfa, nrel, nmel;
  int indeks=0;
  int licznik,nel,nmaxel;
  double *x,*y,*z,*xa,*ya,*za;
  int *nrgrain,*elemgr;
  int *tmpfer,liczfer;
  double tmpx,tmpy,tmpz;
  int el_nodes[MMC_MAXELVNO+1];        // list of nodes of El
  double node_coor[3*MMC_MAXELVNO];  // coord of nodes of El
  double p[3];
  int spr,spr2,flag;
  double hsize, min, mn;
  int neig[6],neig2[6],matl,tmpnel,matl2,max,*counter,anc;
  double xs,ys,zs;
  int zxs,zys,zzs,zxa,zya,zza;
#ifdef PARALLEL
  int info;
#endif

   //arrays for grains

   x=(double *)calloc(1000000,sizeof(double));
   y=(double *)calloc(1000000,sizeof(double));
   z=(double *)calloc(1000000,sizeof(double));
   xa=(double *)calloc(1000000,sizeof(double));
   ya=(double *)calloc(1000000,sizeof(double));
   za=(double *)calloc(1000000,sizeof(double));
   nrgrain=(int *)calloc(1000000,sizeof(int));
   tmpfer=(int *)calloc(1000000,sizeof(int));

  /*++++++++++++++++ executable statements ++++++++++++++++*/

    /* very simple - for the beginning instead of GUI */
    if(argv[1]==NULL){
  	strcpy(work_dir,".");
    } else {
  	sprintf(work_dir,"%s",argv[1]);
    }

    utr_set_interactive(work_dir, argc, argv,
			&interactive_input, &interactive_output);

  #ifdef DEBUG
    fprintf(interactive_output,"Starting program in debug mode.\n");
  #endif
  #ifdef DEBUG_MMM
    fprintf(interactive_output,"Starting mesh module in debug mode.\n");
  #endif
  #ifdef DEBUG_APM
    fprintf(interactive_output,"Starting approximation module in debug mode.\n");
  #endif
  #ifdef DEBUG_SIM
    fprintf(interactive_output,"Starting solver interface in debug mode.\n");
  #endif
  #ifdef DEBUG_LSM
    fprintf(interactive_output,"Starting linear solver (adapter) module in debug mode.\n");
  #endif

  /*++++ initialization of problem, mesh and field data ++++*/
    problem_id=pdr_init_problem(work_dir, interactive_input, interactive_output);

  /*++++++++++++ main menu loop ++++++++++++++++++++++++++++*/
  do {

#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif

    if(interactive_input == stdin){


      do {
	/* define a menu */
	printf("\nChoose a command from the menu:\n");
	printf("\ts - solve test problem - Laplace's quation \n");
	printf("\tp - postprocessing\n");
	printf("\te - compute error\n");
	printf("\tm - perform manual mesh adaptation \n");
	printf("\ta - perform automatic mesh adaptation \n");
	printf("\td - dump out data \n");
	printf("\tr - perform random test of h-adaptivity\n");
	if(pdv_problem.gr.en==1)
		printf("\tg - solve grains\n");
	printf("\tq - exit the program\n");

	scanf(" %c",&c);
      } while ( c != 'm' && c != 'd' && c != 'r' && c != 'g'
		&& c != 's' && c != 'e' && c != 'p'
		&& c != 'a' && c != 'q' && c != 'q' );

    } else{

      // read control data from interactive_input file
      fscanf(interactive_input,"%c\n",&c);

    }

#ifdef PARALLEL
    }

    pcr_bcast_char(pcr_print_master(),1,&c);
    //printf("After BCAST %c\n",c);

#endif


    if(c=='g'){

		j=0;

#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif
		printf("Compute grains\n");
#ifdef PARALLEL
    }
#endif
		k=0;
		for(licznik=0;licznik<680;licznik+=10)
		{
				//problem_id=pdr_init_problem(work_dir, interactive_input, interactive_output);
#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif
				printf("\nIteration=%d\n",licznik);
#ifdef PARALLEL
    }
#endif
				sprintf(grains_name,"%s/przemiana/iteration_%d.txti", work_dir,licznik);
//				grains = fopen(grains_name,"r");
//
//				if(grains==NULL)
//				{
//					printf("Unable to open grains file for %d iteration - make sure that you have 'przemiana' directory!!\n",licznik);
//					exit(-1);
//				}
//
//				utr_skip_rest_of_line(grains);//
//
//				for(i=0;i<90000;i++)
//					fscanf(grains,"%d %d %d %d\n",&tmpx,&tmpy,&nrgrain[i],&tmpfer);
//
//				fclose(grains);
//
//				max=nrgrain[0];
//				for(i=0;i<90000;i++)
//				{
//					if(nrgrain[i]>=max)
//						max=nrgrain[i];
//				}
//				//printf("max=%d\n",max);
//				counter=(int *)calloc(max+2,sizeof(int));
//				for(i=0;i<90000;i++)
//				{
//					counter[nrgrain[i]]++;
//				}
//				for(i=1;i<=max;i++)
//				{
//					printf("ziarno %d wystepuje %d razy\n",i,counter[i]);
//				}


				grains = fopen(grains_name,"r");
				if(grains==NULL)
				{
					printf("Unable to open grains file for %d iteration - make sure that you have 'przemiana' directory!!\n",licznik);
					exit(-1);
				}
				utr_skip_rest_of_line(grains);
				mn=pdv_problem.gr.multip;

				liczfer=0;
				for(i=0;i<90000;i++)
				{
						fscanf(grains,"%lf %lf %d %d\n",&tmpx,&tmpy,&nrgrain[i],&tmpfer[i]);
						if((tmpfer[i]==0))//&&(counter[nrgrain[i]]>4))
						{
							x[liczfer]=tmpx/mn;
							y[liczfer]=299/mn-tmpy/mn;
							liczfer++;
						}
						xa[i]=tmpx/mn;
						ya[i]=299/mn-tmpy/mn;
				}
				//printf("Ferrite grains for iteration %d - %d\n",licznik,liczfer);

				fclose(grains);

				elemgr=(int *)calloc(mmr_get_max_elem_id(mesh_id)+1,sizeof(int));

				nel=0;
				spr=0;
				while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0)
				{
						mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
						flag=0;
						for(i=0;i<liczfer;i++)
						{
							p[0]=x[i];
							p[1]=y[i];
							p[2]=0;
							spr=pointintriangle(p,&node_coor[0],&node_coor[3],&node_coor[6]);
							if(spr==1)
							{
								flag=1;
							}
						}
						if(flag==0)
						{
							spr=0;
							for(i=0;i<90000;i++)
							{
								p[0]=xa[i];
								p[1]=ya[i];
								p[2]=0;
								spr+=pointintriangle(p,&node_coor[0],&node_coor[3],&node_coor[6]);
							}
							if(spr==0)
							{
								flag=3;
							}
						}

						if(flag==1)
						{
							mmr_el_set_mate(mesh_id,nel,2);
						}
						else if(flag==0)
						{
							mmr_el_set_mate(mesh_id,nel,1);
						}
						else
						{
							xs=node_coor[0]+node_coor[3]+node_coor[6];
							xs=xs/3;
							xs=xs*mn;
							ys=node_coor[1]+node_coor[4]+node_coor[7];
							ys=ys/3;
							ys=ys*mn;
							zxs=round(xs);
							zys=round(ys);
							for(i=0;i<90000;i++)
							{
								zxa=round(xa[i]*mn);
								zya=round(ya[i]*mn);
								if((zxa==zxs)&&(zya==zys))
								{
									//printf("xa[%d]=%lf ya[%d]=%lf\n",zxa,xa[i],zya,ya[i]);
									if(tmpfer[i]==1)
										mmr_el_set_mate(mesh_id,nel,1);
									else
										mmr_el_set_mate(mesh_id,nel,2);
								}
							}

						}

						for(i=0;i<90000;i++)
						{
							p[0]=xa[i];
							p[1]=ya[i];
							p[2]=0;
							spr=pointintriangle(p,&node_coor[0],&node_coor[3],&node_coor[6]);
							if(spr==1)
							{
								elemgr[nel]=nrgrain[i];
							}
						}
						/*
						if(k==0)
							min=mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL);
						hsize=mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL);
						if(hsize<min)
							min=hsize;
						//printf("Element %d nalezy do ziarna %d\n",nel,elemgr[nel]);
						k++;
						/**/
				}
				/*
				printf("Min=%lf\n",min);
				getchar();
				/**/

				//solve
				sprintf(arg,"%s/solver.dat", work_dir);
				if(licznik==0)
				{
					 //t_wall2=time_clock();
					 sir_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg, -2, -2, -2, 2);
					 //t_wall2=time_clock()-t_wall2;
					 if(pdv_problem.gr.err==1)
					 {
						 pdr_get_zzhu_error(field_id);
					 	 pdr_free_list();
					 }
					 if(pdv_problem.gr.save==2||(pdv_problem.gr.save%10)==2||(pdv_problem.gr.save/10)==2)
					 {
						 sprintf(arg3, "%s/all_merged_grains_%d.vtu", work_dir,licznik);
						 utr_write_paraview_std_lin(mesh_id, field_id, arg3);
						 //mergeParaviewMeshAndField(mesh_id, field_id, arg3);
					 }
					 if(pdv_problem.gr.save==3||(pdv_problem.gr.save%10)==3||(pdv_problem.gr.save/10)==3)
					 {
						 sprintf(arg3, "%s/grains_%d.mesh", work_dir,licznik);
						 writeGrains(mesh_id,field_id,elemgr,licznik,arg3);
					 }

				}
				if(j!=liczfer&&licznik!=0)
				{
					 //t_wall2=time_clock();
					printf("wejście=%d\n",licznik);
					 sir_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg, -2, -2, -2, 2);
					 printf("wyjście!!\n");

					 //t_wall2=time_clock()-t_wall2;

					 if(pdv_problem.gr.err==1)
					 {
						 pdr_get_zzhu_error(field_id);
						 pdr_free_list();
					 }
					 if(pdv_problem.gr.save==2||(pdv_problem.gr.save%10)==2||(pdv_problem.gr.save/10)==2)
					 {
						 sprintf(arg3, "%s/all_merged_grains_%d.vtu", work_dir,licznik);
						 utr_write_paraview_std_lin(mesh_id, field_id, arg3);
						 //mergeParaviewMeshAndField(mesh_id, field_id, arg3);
					 }
					 if(pdv_problem.gr.save==3||(pdv_problem.gr.save%10)==3||(pdv_problem.gr.save/10)==3)
					 {
						 sprintf(arg3, "%s/grains_%d.mesh", work_dir,licznik);
						 writeGrains(mesh_id,field_id,elemgr,licznik,arg3);
					 }
					 if(pdv_problem.gr.adpt==1)
					 {
						 //t_wall=t_wall3=time_clock();
						 	 pdr_adapt(field_id);
						 //t_wall=time_clock()-t_wall;
						 pdr_free_list();
						 while(pdr_constr(field_id)!=5);
						 //t_wall3=time_clock()-t_wall3;
					 }
					 if(pdv_problem.gr.adpt==2)
					 {
						 pdr_adapt2(field_id);
						 pdr_free_list();

						 /*Adaptacja wielokrotna*/
						 /*
						 int nrbef=mmr_get_nr_node(mesh_id);
						 //printf("nrbef=%d\n",nrbef);
						 int nrafter=0;
						 while(nrafter!=nrbef)
						 {
							 nrbef=mmr_get_nr_node(mesh_id);
							 pdr_adapt2(field_id);
							 pdr_free_list();
							 sir_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg);
							 nrafter=mmr_get_nr_node(mesh_id);
							 //printf("nrafter=%d\n",nrafter);
							 //getchar();
						 }
						 /**/
						 //if(licznik==670)
						 while(pdr_constr(field_id)!=5);
					 }
					 if(pdv_problem.gr.adpt==3)
					 {
						 pdr_adapt2(field_id);
						 pdr_free_list();
						 sir_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg, -2, -2, -2, 2);
						 pdr_adapt(field_id);
						 pdr_free_list();
						 while(pdr_constr(field_id)!=5);
				 }
				}
				else if(j==liczfer)
				{
					 t_wall2=time_clock();
					 printf("wejście2=%d\n",licznik);
					 sir_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg, -2, -2, -2, 2);
					 t_wall2=time_clock()-t_wall2;

					 if(pdv_problem.gr.err==1)
					 {
						 pdr_get_zzhu_error(field_id);
					 	 pdr_free_list();
					 }
					 if(pdv_problem.gr.save==2||(pdv_problem.gr.save%10)==2||(pdv_problem.gr.save/10)==2)
					 {
						 sprintf(arg3, "%s/all_merged_grains_%d.vtu", work_dir,licznik);
						 utr_write_paraview_std_lin(mesh_id, field_id, arg3);
						 //mergeParaviewMeshAndField(mesh_id, field_id, arg3);
					 }
					 if(pdv_problem.gr.save==3||(pdv_problem.gr.save%10)==3||(pdv_problem.gr.save/10)==3)
					 {
						 sprintf(arg3, "%s/grains_%d.mesh", work_dir,licznik);
						 writeGrains(mesh_id,field_id,elemgr,licznik,arg3);
					 }
				}

				j=liczfer;
				//printf("\nCzas adaptacji: %lf\nCzas adaptacji z usuwaniem constr: %lf\nCzas obliczen: %lf\n",t_wall,t_wall3,t_wall2);
		}//licznik

    }
    else if(c=='d'){

      sprintf(arg,"%s/mesh_prism.dmp", work_dir);
      iaux = mmr_export_mesh(mesh_id, MMC_MOD_FEM_MESH_DATA, arg);
      if(iaux<0) {
	/* if error in writing data */
      fprintf(interactive_output,"Error in writing mesh data!\n");
      }
      sprintf(arg,"%s/field_std_lin.dmp", work_dir);
      iaux = utr_io_export_field(field_id, nreq,-1,0.000000001, arg);
      if(iaux<0) {
	/* if error in writing data */
      fprintf(interactive_output,"Error in writing field data!\n");
      }

      if(pdv_problem.inv==1)  //write stress invariants
    	  pdr_write_sigma(field_id);

      sprintf(arg3, "%s/paraview_%d.vtu", work_dir,indeks);
      utr_write_paraview_std_lin(mesh_id, field_id, arg3);

         //constraints remove
         //while(pdr_constr(field_id)!=5);

         sprintf(arg3, "%s/grains3D_%d.mesh", work_dir,indeks);
         writeGrains3Dconstr(mesh_id,field_id,elemgr,licznik,arg3);

    }
    else if(c=='s'){
      indeks++;
      printf("Start\n");
      if(pdv_problem.gr.en==1)
      {
    	  if(indeks==1)
    	  {
    		  printf("Wczytuję!\n");
			  //grains = fopen("data_3D.txt","r");
    		  grains = fopen("data_3D_small.txt","r");
			  if(grains==NULL)
			  {
				 printf("Unable to open grains file - make sure that you have 'data_3D' file!\n");
				 exit(-1);
			  }
			  //utr_skip_rest_of_line(grains);
			  mn=pdv_problem.gr.multip;

			  liczfer=0;
			  //for(i=0;i<1000000;i++)
			  for(i=0;i<125000;i++)
			  {
				  fscanf(grains,"%lf %lf %lf %d\n",&tmpx,&tmpy,&tmpz,&tmpfer[i]);
//				  if((tmpfer[i]==3))
//				  {
//					 x[liczfer]=tmpx;
//					 y[liczfer]=tmpy;
//					 z[liczfer]=tmpz;
//					 liczfer++;
//				  }
				  xa[i]=tmpx;
				  ya[i]=tmpy;
				  za[i]=tmpz;
				  //if(i==0)
					//  printf("Wczytane: xa[i]=%lf,ya[i]=%lf,za[i]=%lf",xa[i],ya[i],za[i]);
			  }
			  //printf("Ferrite grains - %d\n",liczfer);

			  fclose(grains);
    	  }

    	  elemgr=(int *)calloc(mmr_get_max_elem_id(mesh_id)+1,sizeof(int));

    	  printf("Setting material data\n");
    	  printf("\n");
    	  nel=0;
    	  nmaxel=mmr_get_max_elem_id(mesh_id);

//    	  while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0)
//    	  {
//    		  mmr_el_set_mate(mesh_id,nel,-1);
//    		  //printf("%d\r",nel/nmaxel*100);
//    		  spr=0;
//    		  spr2=0;
//    		  mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
////    		  for(i=0;i<18;i++)
////    		  {
////    			  printf("Node %d - %lf\n",i/3,node_coor[i]);
////    		  }
////    		  printf("\n");
//
//    		  flag=0;
//    	  	  for(i=0;i<liczfer;i++)
//    	  	  {
//    	  		 p[0]=x[i];
//    	  		 p[1]=y[i];
//    	  		 p[2]=0;
//    	  		 spr=pointintriangle(p,&node_coor[0],&node_coor[3],&node_coor[6]);
//    	  		 if(spr==1)
//    	  		   if(z[i]>node_coor[2] && z[i]<node_coor[11])
//   	  		       {
//   	  			      //spr2=1;
//   	  			      //printf("z[i]=%lf,node_coor[2,11]=%lf,%lf!\n",z[i],node_coor[2],node_coor[11]);
//   	  		          flag=1;
//    	  			  //printf("Punkt %lf,%lf,%lf należy do elementu %d o wspolrzednych trojkat: a=%lf,%lf,b=%lf,%lf,c=%lf,%lf ,czworokat - %lf,%lf\n",x[i],y[i],z[i],nel,node_coor[0],node_coor[1],node_coor[3],node_coor[4],node_coor[6],node_coor[7],node_coor[2],node_coor[11]);
//    	  		   }
//    	  	  }
//    	  	  if(flag==0)
//    	  	  {
//    	  		 spr=0;
//    	  		 spr2=0;
//    	  		 //for(i=0;i<1000000;i++)
//    	  		 for(i=0;i<125000;i++)
//    	  		 {
//    	  			p[0]=xa[i];
//    	  			p[1]=ya[i];
//    	  			p[2]=0;
//    	  			spr+=pointintriangle(p,&node_coor[0],&node_coor[3],&node_coor[6]);
//    	  		 }
//    	  		 if(spr==0)
//    	  		 {
//    	  			flag=3;
//    	  			//printf("flag=3 dla elementu %d o wspolrzednych - trojkat: (%lf %lf %lf),(%lf %lf %lf),(%lf %lf %lf),czworokat - %lf,%lf\n",nel,node_coor[0],node_coor[1],node_coor[2],node_coor[3],node_coor[4],node_coor[5],node_coor[6],node_coor[7],node_coor[8],node_coor[2],node_coor[11]);
//    	  		 }
//    	  	  }
//
//    	  	  if(flag==1)
//    	  	  {
//    	  		 mmr_el_set_mate(mesh_id,nel,2);
//    	  		 elemgr[nel]=2;
//    	  	  }
//    	  	  else if(flag==0)
//    	  	  {
//    	  		 mmr_el_set_mate(mesh_id,nel,1);
//    	  		 elemgr[nel]=1;
//    	  	  }
//    	  	  else
//    	  	  {
//    	  		mmr_el_set_mate(mesh_id,nel,3);
//    	  		//elemgr[nel]=1;
////    	  		 xs=node_coor[0]+node_coor[3]+node_coor[6];
////    	  		 xs=xs/3;
////    	  		 xs=xs*mn;
////    	  		 ys=node_coor[1]+node_coor[4]+node_coor[7];
////    	  		 ys=ys/3;
////    	  		 ys=ys*mn;
////    	  		 zs=node_coor[2]+node_coor[11];
////    	  		 zs=zs/2;
////    	  		 zs=zs*mn;
////    	  		 zxs=round(xs);
////    	  		 zys=round(ys);
////    	  		 zzs=round(zs);
////    	  		 //for(i=0;i<1000000;i++)
////    	  		 //if(nel==69324)
////    	  		 //{
////    	  		//	 printf("xs=%lf,ys=%lf,zs=%lf,zxs=%d,zys=%d,zzs=%d\n",xs,ys,zs,zxs,zys,zzs);
////    	  		 //}
////
////    	  		 for(i=0;i<125000;i++)
////    	  		 {
////    	  			zxa=round(xa[i]*mn);
////    	  			zya=round(ya[i]*mn);
////    	  			zza=round(za[i]*mn);
////    	  			//if(nel==69324&&i==0)
////    	  			//	printf("xa[i]=%lf,ya[i]=%lf,za[i]=%lf,zxa=%d zya=%d zza=%d\n",xa[i],ya[i],za[i],zxa,zya,zza);
////    	  			if((zxa==zxs)&&(zya==zys)&&(zza==zzs))
////    	  			{
////    	  			   //printf("xa[%d]=%lf ya[%d]=%lf za[%d]=%lf\n",zxa,xa[i],zya,ya[i],zza,za[i]);
////    	  			   if(tmpfer[i]==2)
////    	  			   {
////    	  				   mmr_el_set_mate(mesh_id,nel,1);
////    	  				   elemgr[nel]=1;
////    	  			   }
////    	  			   else
////    	  			   {
////    	  				   mmr_el_set_mate(mesh_id,nel,2);
////    	  				   elemgr[nel]=2;
////    	  			   }
////    	  			   //printf("OK!\n");
////    	  			}
////    	  			/*else
////    	  			{
////
////    	  				printf("błąd? %d\n",nel);
////    	  				printf("xa[%d]=%lf ya[%d]=%lf za[%d]=%lf\n",zxa,xa[i],zya,ya[i],zza,za[i]);
////    	  			  for(i=0;i<18;i++)
////    	      		  {
////    	      			  printf("Node %d - %lf\n",i/3,node_coor[i]);
////    	  	   		  }
////    	  	   		  printf("\n");
////    	  				exit(1);
////    	  				mmr_el_set_mate(mesh_id,nel,1);
////    	  			}*/
////    	  		 }
//    	  	  }
//
//    	  						/*
//    	  						if(k==0)
//    	  							min=mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL);
//    	  						hsize=mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL);
//    	  						if(hsize<min)
//    	  							min=hsize;
//    	  						//printf("Element %d nalezy do ziarna %d\n",nel,elemgr[nel]);
//    	  						k++;
//    	  						/**/
////    	  	  if(mmr_el_groupID(mesh_id,nel)!=2 && mmr_el_groupID(mesh_id,nel)!=1)
////    	  	  {
////    	  	      printf("Element %d - hsize: %lf ma material %d\n",nel,mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL),mmr_el_groupID(mesh_id,nel));
////    	  	      for(i=0;i<6;i++)
////    	  	    	  printf("Node %d - %lf,%lf,%lf\n",el_nodes[i+1],node_coor[3*i],node_coor[3*i+1],node_coor[3*i+2]);
////
////    	  	      //exit(-1);
////    	  	  }
//    	  } //loop over elements


    	  double odleglosc,srodek[3];
    	  nel=0;

    	  while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0)
    	  {
    		  mmr_el_set_mate(mesh_id,nel,-1);
    		  //if(mmr_el_groupID(mesh_id,nel)==3)
    		  //{
				  mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
				  srodek[0]=node_coor[0]+node_coor[3]+node_coor[6];
				  srodek[0]=srodek[0]/3;
				  srodek[1]=node_coor[1]+node_coor[4]+node_coor[7];
				  srodek[1]=srodek[1]/3;
				  srodek[2]=node_coor[2]+node_coor[11];
				  srodek[2]/=2;
//				  printf("Srodek pryzmy o wspolrzednych:\t");
//				  for(i=0;i<18;i++)
//				  {
//					  printf("%lf ",node_coor[i]);
//					  if(i!=0&&(i+1)%3==0)
//						  printf("\n");
//				  }
//				  printf("\n");
//				  printf("%lf,%lf,%lf\n",srodek[0],srodek[1],srodek[2]);
				  double min=0;
				  int wsp=0;
    			  for(i=0;i<125000;i++)
    			  {
    				  //sortowanie babaelkowe??Minimum??
    				  odleglosc=sqrt(pow(xa[i]-srodek[0],2)+pow(ya[i]-srodek[1],2)+pow(za[i]-srodek[2],2));
    				  if(i==0)
    				  {
    					  min=odleglosc;
    					  wsp=i;
    				  }
    				  else
    				  {
    					  if(odleglosc<min)
    					  {
    						  min=odleglosc;
    						  wsp=i;
    					  }
    				  }
    			  }
    			  //printf("Minimalna odleglosc = %lf dla punktu %d o wsp: %lf,%lf,%lf - ferryt=%d\n",min,wsp,xa[wsp],ya[wsp],za[wsp],tmpfer[wsp]);
    			  mmr_el_set_mate(mesh_id,nel,tmpfer[wsp]-1);
    			  elemgr[nel]=tmpfer[wsp]-1;
    		  //}
    		  if(mmr_el_groupID(mesh_id,nel)!=2 && mmr_el_groupID(mesh_id,nel)!=1)
			  {
				  printf("Element %d - hsize: %lf ma material %d\n",nel,mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL),mmr_el_groupID(mesh_id,nel));
				  for(i=0;i<6;i++)
					  printf("Node %d - %lf,%lf,%lf\n",el_nodes[i+1],node_coor[3*i],node_coor[3*i+1],node_coor[3*i+2]);

				  exit(-1);
			  }
    	  }




//    	  int gen=0;
//    	  int anc=0;
//    	  int mat=0;
//    	  int weight1=0;
//    	  int weight2=0;
//    	  int weight3=0;
//    	  int neig[6];
//    	  nel=0;
//    	  while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0)
//    	  {
//    		 if(mmr_el_groupID(mesh_id,nel)==3)
//    		 {
//    			 printf("Element %d!! - ",nel);
//    			 gen=mmr_el_gen(mesh_id,nel);
//    			 printf("Generation level: %d - ",gen);
//    			 anc=mmr_el_ancestor(mesh_id,nel,gen-1);
//    			 mat=mmr_el_groupID(mesh_id,anc);
//    			 printf("Przodek %d ma material %d \n",anc,mat);
//    			 mmr_el_set_mate(mesh_id,nel,mat);
    			 //mmr_el_set_mate(mesh_id,nel,1);
//    			 mmr_el_eq_neig(mesh_id,nel,neig,NULL);
//    			 for (i=1;i<6;i++)
//    			 {
//    				 printf("Sąsiad %d -%d",i,neig[i]);
//    				 if(neig[i]>0)
//    				 {
//    					 mat = mmr_el_groupID(mesh_id,neig[i]);
//    					 printf(" - material %d",mat);
//    					 if(mat==1)
//    						 weight1++;
//    					 else if(mat==2)
//    						 weight2++;
//    					 else
//    						 weight3++;
//
//    				 }
//    				 printf("\n");
//    			 }
//    			 if(weight1>weight2)
//    			 {
//    				 mmr_el_set_mate(mesh_id,nel,1);
//    				 elemgr[nel]=1;
//    			 }
//    			 else if(weight2>=weight1)
//    			 {
//    		    	 mmr_el_set_mate(mesh_id,nel,2);
//    		    	 elemgr[nel]=2;
//    		     }
    			 //else if(weight3>=weight1 && weight3>=weight2)
    			 //{
    			//	 gen=mmr_el_gen(mesh_id,nel);
    			//	 anc=mmr_el_ancestor(mesh_id,nel,gen-1);
    			//	 mat=mmr_el_groupID(mesh_id,anc);
    			//	 mmr_el_set_mate(mesh_id,nel,mat);
    			 //  	 elemgr[nel]=mat;
    			 //}

//    	  }


      }//end grains 3d
      //printf("\n");
#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif
      fprintf(interactive_output,
	"\nBeginning solution of the test problem - Laplace's equation\n\n");
#ifdef PARALLEL
    }
#endif
      /* very simple time measurement */
      t_wall=time_clock();

#ifdef PARALLEL
      {
	int ione = 1;
/* initiate exchange tables for DOFs - for one field = one solver, one level */
	appr_init_exchange_tables(pdv_nr_proc, pcr_my_proc_id(), 1, &ione);
      }
#endif

      /* problem dependent interface with linear solvers */
      sprintf(arg,"%s/solver.dat", work_dir);
#ifndef PARALLEL
      sir_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg,-2,-2,-2,2);
#endif

#ifdef PARALLEL
      sir_solve_lin_sys(problem_id, SIC_PARALLEL, arg);
/* free exchange tables for DOFs - for one field = one solver */
      appr_free_exchange_tables(1);
#endif

      /* very simple time measurement */
      t_wall=time_clock()-t_wall;

#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif
      fprintf(interactive_output,
	      "\nTime for solving a system of linear equations %lf\n\n",t_wall);

#ifdef PARALLEL
    }
#endif
    }//end of solve
    else if(c=='a'){
    	//pdr_adapt(field_id);
    	//pdr_create_patch(field_id);
    	//utr_adapt(problem_id,work_dir,interactive_input,interactive_output);
    	//pdr_free_list();
    	pdr_adapt(field_id);
    	//t_wall=time_clock()-t_wall;
    	pdr_free_list();

    }
    else if(c=='e'){

    	fprintf(interactive_output,"\n\nZienkiewicz-Zhu error estimator:\n\n");
    	pdr_get_zzhu_error(field_id);
    	//pdr_get_sigma(field_id);
    	//pdr_compute_eyoung(field_id);
    }
    else if(c=='p'){
//postprocessing
      pdr_post_process(field_id,interactive_output);

    }
    else if(c=='m'){

      utr_manual_refinement( problem_id, work_dir, 
			     interactive_input, interactive_output );

    }
    else if(c=='r'){

      utr_test_refinements(problem_id, work_dir, 
			   interactive_input, interactive_output );

    } /* end test of mesh refinements */

  } while(c != 'q');

  /* free allocated space */
  apr_free_field(field_id);
  mmr_free_mesh(mesh_id);

  fclose(interactive_input);
  fclose(interactive_output);

#ifdef PARALLEL
  pcr_exit_parallel();
#endif

  return(0);

}


/* to test setting initial approximation field */
double Initial_condition(double* Coor)
{

  return(Coor[0]+Coor[1]+Coor[2]);
}

/*---------------------------------------------------------
  pdr_post_process -
----------------------------------------------------------*/
double pdr_post_process(
  int Field_id,    /* in: approximation field ID  */
  FILE *interactive_output
        )
{

/* local variables */
  double x[3];

  int pdeg;		/* degree of polynomial */
  int pdeg_old=0; /* indicator for recomputing quadrature data */
  int base_q;		/* type of basis functions for quadrilaterals */
  int ngauss;            /* number of gauss points */
  double xg[3000];   	 /* coordinates of gauss points in 3D */
  double wg[1000];       /* gauss weights */

  int num_shap;         /* number of element shape functions */
  int ndofs;            /* local dimension of the problem */
  double determ;        /* determinant of jacobi matrix */
  double vol;           /* volume for integration rule */
  double xcoor[3];      /* global coord of gauss point */
  double u_val[PDC_NREQ]; /* computed solution */
  double u_x[PDC_NREQ];   /* gradient of computed solution */
  double u_y[PDC_NREQ];   /* gradient of computed solution */
  double u_z[PDC_NREQ];   /* gradient of computed solution */
  double base_phi[APC_MAXELVD];    /* basis functions */
  double base_dphix[APC_MAXELVD];  /* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];  /* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];  /* y-derivatives of basis function */
  int el_nodes[MMC_MAXELVNO+1];        /* list of nodes of El */
  double node_coor[3*MMC_MAXELVNO];  /* coord of nodes of El */
  double dofs_loc[APC_MAXELSD]; /* element solution dofs */
  double dofs_loc2[APC_MAXELSD]; /* element solution dofs */

/* for scalar test cases - exact solution and its derivatives */
  double f, f_x, f_y, f_z;
/* auxiliary variables */
  int i, j, iel, ki, iaux, name, mat_num, mesh_id, nel, sol_vec_id, nreq;
  double daux, eaux, volume, average, time;
  int list_el[20];

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the corresponding mesh */
  mesh_id = apr_get_mesh_id(Field_id);
  nreq=PDC_NREQ;

  printf("Give global coordinates of a point (x,y,z):\n");
  scanf("%lf",&x[0]);
  scanf("%lf",&x[1]);
  scanf("%lf",&x[2]);


  apr_sol_xglob(Field_id, x, 1, list_el, xg, u_val, NULL, NULL, NULL);
  printf("\nSolution at point %.2lf %.2lf %.2lf in element %d:\n\n",
	 xg[0],xg[1],xg[2],list_el[1]);

  for(j=0;j<nreq;j++)  printf("u_val[%d]=%lf\n",j,u_val[j]);

  return(0);

  printf("Give local coordinates of a point (x,y,z):\n");
  scanf("%lf",&x[0]);
  scanf("%lf",&x[1]);
  scanf("%lf",&x[2]);
  //printf("x=%lf,y=%lf,z=%lf\n",x[0],x[1],x[2]);


  nel=0;
  while((nel=mmr_get_next_act_elem(mesh_id,nel))!=0)
  {

    num_shap = 6;
    nreq=PDC_NREQ;
    ndofs = nreq*num_shap;

    /* get the coordinates of the nodes of El in the right order */
    mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);

    /* get the most recent solution degrees of freedom */
    sol_vec_id = 1;
    for(j=0;j<nreq;j++){
      for(i=0;i<el_nodes[0];i++){
	apr_read_ent_dofs(Field_id,APC_VERTEX,el_nodes[i+1],
			  nreq,sol_vec_id,dofs_loc);
	dofs_loc2[j*num_shap+i]=dofs_loc[j];
      }
    }

    iaux = 2; /* calculations with jacobian but not on the boundary */

    apr_elem_calc_3D(iaux, nreq, &pdeg, base_q,
		     x,node_coor,dofs_loc2,
		     base_phi,base_dphix,base_dphiy,base_dphiz,
		     xcoor,u_val,u_x,u_y,u_z,NULL);

    printf("\nSolution at point %.2lf %.2lf %.2lf in element %d:\n\n",
	   x[0],x[1],x[2],nel);

    for(j=0;j<nreq;j++)  printf("u_val[%d]=%lf\n",j,u_val[j]);

    printf("\nGlobal coordinates of the point:  %.2lf %.2lf %.2lf:\n\n",
	   xcoor[0],xcoor[1],xcoor[2]);


  } /* end of loop over active elements */


  return(1);
}


/*---------------------------------------------------------
  pdr_init_problem - to initialize problem (including mesh and field) data
----------------------------------------------------------*/
int pdr_init_problem(
  char* Work_dir,
  FILE *interactive_input,
  FILE *interactive_output
  )
{

/* auxiliary variables */
  FILE *problem_input;
  char c, d, keyword[300], arg[100];
  char problem_input_name[300];
  int num_prob, iprob, meshfil_typ, num_bc, ibc, mesh_id, field_id, pdeg;
  int nrno, nmno, nred, nmed, nrfa, nmfa, nrel, nmel;
  int nreq, nr_sol, imat;
  int nrmat,bc,iaux,jaux;
  double daux, eaux;
  double vec[3];
  int i, j;

/*++++++++++++++++ executable statements ++++++++++++++++*/


  /*+++++++++++   opening file with problem data   ++++++++++++*/
  sprintf(problem_input_name,"%s/problem_elast.dat", Work_dir);
  problem_input = fopen(problem_input_name,"r");

  if(problem_input == NULL){
    fprintf(interactive_output,
	    "\nCannot find problem input file %s. \n", problem_input_name);
    exit(-1);
  }

    /*+++++++++++   initialization of mesh data   ++++++++++++*/

    // mesh/meshfile types:
    //   j - gradmesh 2D mesh
    //   p - prismatic mesh in standard (text) format
    //   t - tetrahedral mesh in standard format
    //   h - hybrid mesh in standard format

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"MESH_TYPE",8) != 0) {
      printf("ERROR in reading input file for MESH_TYPE\n");
      exit(-1);
    }
    fscanf(problem_input,"%c",&c);
    utr_skip_rest_of_line(problem_input);

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"MESH_FILE",8) != 0) {
      printf("ERROR in reading input file for MESH_FILE\n");
      exit(-1);
    }
    fscanf(problem_input,"%s",&arg);
    utr_skip_rest_of_line(problem_input);

    mesh_id = utr_initialize_mesh(interactive_output, work_dir, c, arg);
    pdv_problem.mesh_id = mesh_id;

    /*+++++++++++   initialization of approximation field data   ++++++++++++*/

    // field type standard
    d ='s';

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"FIELD_SOURCE",8) != 0) {
      printf("ERROR in reading input file for FIELD_SOURCE\n");
      exit(-1);
    }

    // options for field initialization
    // r - read from file
    // i - initialize to specified function values
    // z - initialize to zero
    fscanf(problem_input,"%c",&c);
    utr_skip_rest_of_line(problem_input);

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"NREQ",4) != 0) {
      printf("ERROR in reading input file for NREQ\n");
      exit(-1);
    }
    fscanf(problem_input,"%d", &pdv_problem.nreq );
    utr_skip_rest_of_line(problem_input);
    if(pdv_problem.nreq > PDC_NREQ){
      printf("Parameter NREQ read from input file > PDC_NREQ\n");
      printf("Type any key (twice) to continue or CTRL C to quit\n");
      getchar();getchar();
    }

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"NRSOL",4) != 0) {
      printf("ERROR in reading input file for NRSOL\n");
      exit(-1);
    }
    fscanf(problem_input,"%d", &pdv_problem.nr_sol );
    utr_skip_rest_of_line(problem_input);

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"FIELD_FILE",8) != 0) {
      printf("ERROR in reading input file for FIELD_FILE\n");
      exit(-1);
    }
    fscanf(problem_input,"%s",&arg);
    utr_skip_rest_of_line(problem_input);

    pdeg = -77;

    /* reading field values saved by previous runs */
    if(c=='r'){

      field_id = utr_initialize_field(interactive_output, d, c, mesh_id,
    		  pdv_problem.nreq, pdv_problem.nr_sol, pdeg, arg, NULL);

    }
    /* initializing field values according to the specified function */
    else if(c=='i'){

      field_id = utr_initialize_field(interactive_output, d, c, mesh_id,
    		  pdv_problem.nreq,pdv_problem.nr_sol,pdeg,NULL,&Initial_condition);

    }
    /* initializing field values to zero */
    else if(c=='z'){

      field_id = utr_initialize_field(interactive_output, d, c, mesh_id,
    		  pdv_problem.nreq, pdv_problem.nr_sol, pdeg, NULL, NULL);

    }

    pdv_problem.field_id = field_id;

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"NUMBER_OF_MATERIALS",8) != 0) {
          printf("ERROR in reading input file for NUMBER_OF_MATERIALS\n");
          exit(-1);
    }

    fscanf(problem_input,"%d",&matnum);
    utr_skip_rest_of_line(problem_input);

    //printf("matnum=%d\n",matnum);

    pdv_material = (pdt_material *)malloc(matnum*sizeof(pdt_material));

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"MATERIAL_COEFF",8) != 0) {
          printf("ERROR in reading input file for MATERIAL_COEFF %s\n",keyword);
          exit(-1);
    }

    for(i=0;i<matnum;i++)
    {
   	  fscanf(problem_input,"%d %lf %lf",&nrmat,&daux,&eaux);
   	  utr_skip_rest_of_line(problem_input);
   	  pdv_material[i].id = nrmat;
   	  pdv_material[i].Eyoung= daux;
   	  pdv_material[i].Poisni= eaux;
   	  /*fk
   	  printf("Read Eyoung: %lf (%lf), Poisni: %lf %lf\n", pdv_material[i].Eyoung, daux,pdv_material[i].Poisni,eaux);
   	  /**/
     }

    fscanf(problem_input,"%s\n",keyword);
    if(strncmp(keyword,"SIGMA",5) != 0) {
               printf("ERROR in reading input file for SIGMA\n");
               exit(-1);
    }
    fscanf(problem_input,"%d",&pdv_problem.sigma);
    utr_skip_rest_of_line(problem_input);

    fscanf(problem_input,"%s\n",keyword);
	if(strncmp(keyword,"INVARIANTS",8) != 0) {
			   printf("ERROR in reading input file for INVARIANTS\n");
			   exit(-1);
	}
	fscanf(problem_input,"%d",&pdv_problem.inv);
	utr_skip_rest_of_line(problem_input);

	fscanf(problem_input,"%s\n",keyword);
	if(strncmp(keyword,"EPS",3) != 0) {
		   printf("ERROR in reading input file for EPS\n");
		   exit(-1);
	}
	fscanf(problem_input,"%lf",&pdv_problem.adpt.eps);
	utr_skip_rest_of_line(problem_input);

	fscanf(problem_input,"%s\n",keyword);
	if(strncmp(keyword,"RATIO",5) != 0) {
		   printf("ERROR in reading input file for RATIO\n");
		   exit(-1);
	}
	fscanf(problem_input,"%lf",&pdv_problem.adpt.ratio);
	utr_skip_rest_of_line(problem_input);

	 fscanf(problem_input,"%s\n",keyword);
     if(strncmp(keyword,"BOUNDARY_COND",8) != 0) {
           printf("ERROR in reading input file for BOUNDARY_COND\n");
           exit(-1);
     }
     fscanf(problem_input,"%d",&bcnum);
     utr_skip_rest_of_line(problem_input);
     pdv_bc = (pdt_boundary *)malloc(bcnum*sizeof(pdt_boundary));

     for(i=0;i<bcnum;i++)
     {
     	  fscanf(problem_input,"%d %lf %lf %lf",&bc,&vec[0],&vec[1],&vec[2]);
     	  utr_skip_rest_of_line(problem_input);
     	  pdv_bc[i].type=bc;
     	  pdv_bc[i].vec[0]=vec[0];
     	  pdv_bc[i].vec[1]=vec[1];
     	  pdv_bc[i].vec[2]=vec[2];
     	  /*fk
     	  printf("Boundary %d - bc=%d, vec=%lf %lf %lf\n",i,pdv_bc[i].type,pdv_bc[i].vec[0],pdv_bc[i].vec[1],pdv_bc[i].vec[2]);
     	  /**/
     }

     fscanf(problem_input,"%s\n",keyword);
     if(strncmp(keyword,"GRAINS",6) != 0) {
		   printf("ERROR in reading input file for GRAINS\n");
		   exit(-1);
     }
     fscanf(problem_input,"%d",&iaux);
     utr_skip_rest_of_line(problem_input);
     pdv_problem.gr.en=iaux;

     if(iaux==1)
     {
    	 fscanf(problem_input,"%s\n",keyword);
    	 if(strncmp(keyword,"HSIZE",5) != 0) {
			   printf("ERROR in reading input file for HSIZE\n");
			   exit(-1);
		 }
		 fscanf(problem_input,"%lf",&pdv_problem.gr.hsize);
		 utr_skip_rest_of_line(problem_input);

    	 fscanf(problem_input,"%s\n",keyword);
    	 if(strncmp(keyword,"ADAPT",5) != 0) {
			   printf("ERROR in reading input file for ADAPT\n");
			   exit(-1);
		 }
		 fscanf(problem_input,"%d",&pdv_problem.gr.adpt);
		 utr_skip_rest_of_line(problem_input);

    	 fscanf(problem_input,"%s\n",keyword);
    	 if(strncmp(keyword,"SAVE",4) != 0) {
			   printf("ERROR in reading input file for SAVE\n");
			   exit(-1);
		 }
		 fscanf(problem_input,"%d",&pdv_problem.gr.save);
		 utr_skip_rest_of_line(problem_input);

		 fscanf(problem_input,"%s\n",keyword);
		 if(strncmp(keyword,"ERROR",5) != 0) {
			   printf("ERROR in reading input file for ERROR\n");
			   exit(-1);
		 }
		 fscanf(problem_input,"%d",&pdv_problem.gr.err);
		 utr_skip_rest_of_line(problem_input);

		 fscanf(problem_input,"%s\n",keyword);
		 if(strncmp(keyword,"MULTIPLIER",8) != 0) {
			   printf("ERROR in reading input file for MULTIPLIER\n");
			   exit(-1);
		 }
		 fscanf(problem_input,"%lf",&pdv_problem.gr.multip);
		 //utr_skip_rest_of_line(problem_input);
     }



     iaux=15;jaux=-1;
     mmr_set_max_gen(mesh_id, iaux);
     mmr_set_max_gen_diff(mesh_id, jaux);
     pdv_problem.adpt.maxgen=iaux;
     pdv_problem.adpt.maxgendiff=jaux;

     fprintf(interactive_output,"\nmaximal generation level set to %d, maximal generation difference set to %d\n", iaux,jaux);

  apr_check_field(field_id);

  i=1; nrno=mmr_mesh_i_params(mesh_id,i);
  i=2; nmno=mmr_mesh_i_params(mesh_id,i);
  i=5; nred=mmr_mesh_i_params(mesh_id,i);
  i=6; nmed=mmr_mesh_i_params(mesh_id,i);
  i=9; nrfa=mmr_mesh_i_params(mesh_id,i);
  i=10; nmfa=mmr_mesh_i_params(mesh_id,i);
  i=13; nrel=mmr_mesh_i_params(mesh_id,i);
  i=14; nmel=mmr_mesh_i_params(mesh_id,i);

  fprintf(interactive_output,"\nAfter reading initial data.\n");
  fprintf(interactive_output,"Parameters (number of active, maximal index):\n");
  fprintf(interactive_output,"Elements: nrel %d, nmel %d\n", nrel, nmel);
  fprintf(interactive_output,"Faces:    nrfa %d, nmfa %d\n", nrfa, nmfa);
  fprintf(interactive_output,"Edges:    nred %d, nmed %d\n", nred, nmed);
  fprintf(interactive_output,"Nodes:    nrno %d, nmno %d\n", nrno, nmno);

  fclose(problem_input);

  return(1);
}

