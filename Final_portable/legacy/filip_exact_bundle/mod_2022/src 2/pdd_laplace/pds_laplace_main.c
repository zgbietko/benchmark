/************************************************************************
File pds_laplace_main - main control routine for interactive testing of
		 different modules from the code and the solution of simple
		 Laplace equation problem

Contains definitions of routines:

  main() - just for C


------------------------------
History:
	08.2008 - Krzysztof Banas, pobanas@cyf-kr.edu.pl, initial version
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

/* interface for thread management modules */
#include "tmh_intf.h"

/* interface for all solver modules */
#include "sih_intf.h"

/* problem dependent module interface */
#include "pdh_intf.h"

/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"

/* local declarations for problem dependent module */
#include "./pdh_laplace.h"

/* graphics module */
#include "mod_fem_viewer.h"

/*** GLOBAL VARIABLES for the whole module ***/
pdt_ctrls  pdv_ctrl;	/* structure with control parameters */
pdt_adpts  pdv_adpt;	/* structure with adaptation parameters */
pdt_material *pdv_material; /* pointers to material data */

//utt_patches *pdv_patches=NULL;

/* global variable */
char work_dir[200]; /* the name of the working directory */

/* local functions declarations */

/* to test setting initial approximation field */
double Initial_condition(int Field_id, double* Coor, int Sol_comp_id);

/*---------------------------------------------------------
  pdr_init_problem - to initialize problem dependent data
----------------------------------------------------------*/
int pdr_init_problem(
  char* Work_dir,
  FILE *Interactive_input,
  FILE *Interactive_output
  );

/*---------------------------------------------------------
  pdr_write_problem - to write problem dependent data to a disk file
----------------------------------------------------------*/
int pdr_write_problem(
  int Problem_id, 
  char* Filename
  );

/*---------------------------------------------------------
  pdr_error_test - to compute error norm for test examples
----------------------------------------------------------*/
double pdr_error_test( /* returns H1 norm of error for */
				/* for several known exact solutions */
  int Field_id,    /* in: approximation field ID  */
  FILE *Interactive_output
  );

/*---------------------------------------------------------
  pdr_post_process - to privide simple interactive post-processing
----------------------------------------------------------*/
double pdr_post_process(
  int Field_id,    /* in: approximation field ID  */
  FILE *Interactive_input,
  FILE *Interactive_output
  );

/*** DEFINITIONS ***/
int main(int argc, char **argv)
{

/* local variables */
  char interactive_input_name[300];
  char interactive_output_name[300], tmp[100];
  char field_module_name[100];
  FILE *interactive_input, *interactive_output;
  char c, d, arg[300]; /* string variable to read menu */
  int i, iaux, jaux, kaux;
  int nreq, nr_sol, iel, nr_mat, base;
  int problem_id, field_id, mesh_id;
  double daux, t_wall;
  int nrno, nmno, nred, nmed, nrfa, nmfa, nrel, nmel;

#ifdef PARALLEL
  int nr_proc;
  int my_proc_id;
  int info;
#endif

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
  pdr_init_problem(work_dir, interactive_input, interactive_output);
  problem_id = 1; // there is only one problem by default
  i=2; mesh_id=pdr_ctrl_i_params(problem_id,i);
  i=3; field_id = pdr_ctrl_i_params(problem_id,i);  
  i=4; nr_sol = pdr_ctrl_i_params(problem_id,i);
  i=5; nreq = pdr_ctrl_i_params(problem_id,i);
  // currently supported fields:
  // STANDARD_LINEAR - for continuous, vector, linear approximations
  // DG_SCALAR_PRISM - for discontinuous, scalar, high order approximations
  apr_module_introduce(field_module_name);
  
  /*++++++++++++ main menu loop ++++++++++++++++++++++++++++*/
  do {
    
#ifdef PARALLEL
    my_proc_id = pcr_my_proc_id();
    if(pcr_my_proc_id()==pcr_print_master()){
#endif

    if(interactive_input == stdin){
      
      
      do {
	/* define a menu */
	printf("\nChoose a command from the menu:\n");
	printf("\ts - solve test problem - Laplace's quation \n");
	printf("\tu - solve test problem with direct solver \n");
	printf("\tp - postprocessing\n");
	printf("\tk - launch graphic module\n");
	printf("\te - compute error for test problem\n");
	printf("\tz - compute ZZ error estimator for test problem\n");
	printf("\ta - automatic mesh adaptation based on ZZ error estimate\n");
	printf("\tm - perform manual mesh adaptation \n");
	printf("\td - dump out data \n");
	printf("\tv - dump out ParaView graphics data \n");
	printf("\tg - dump out ModFemViewer graphics data \n");
	printf("\tr - perform random test of h-adaptivity\n");
	printf("\tq - exit the program\n");
	
	scanf(" %c",&c);
      } while ( c != 'm' && c != 'd' && c != 'r' && c != 'v'
		&& c != 's' && c != 'e' && c != 'p' && c != 'a'
		&& c != 'z' && c != 'g' && c != 'u' && c != 'q' && c != 'k');
      
    } else{
      
      // read control data from interactive_input file
      fscanf(interactive_input,"%c\n",&c);
      
    }
    
#ifdef PARALLEL
    }

    pcr_bcast_char(pcr_print_master(),1,&c);
    //printf("After BCAST %c\n",c);

#endif

    // dumping data
    if(c=='d'){
      
/* different mesh files can be taken into account using MESHFIL_TYP parameter */ 
      sprintf(arg,"%s/mesh_prism.dmp", work_dir);
      iaux = mmr_export_mesh(mesh_id, MMC_MOD_FEM_MESH_DATA, arg); 
      if(iaux<0) {
	/* if error in writing data */
	fprintf(interactive_output,"Error in writing mesh data!\n");
      }
      
      nreq = pdr_ctrl_i_params(problem_id, 5);
      nr_sol =  pdr_ctrl_i_params(problem_id, 4);
      
      if(nr_sol==1) iaux=1; // parameter to select vectors written to file
      if(nr_sol==2) iaux=3; // both vectors written to file
      // scheme: 1 - 1, 2 - 2, 3 - 4, 1+2 - 3, 1+3 - 5, 2+3 - 6, 1+2+3 - 7
      
      sprintf(arg,"%s/field.dmp", work_dir);
      
      // write field to file arg with accuracy parameter = 0 - full accuracy 
      iaux = utr_io_export_field(field_id, nreq, iaux, 0, arg);
      if(iaux<0) {
	/* if error in writing data */
	fprintf(interactive_output,"Error in writing field data!\n");
      }
      
    }

    // storing ParaView graphics data
    else if(c=='v'){
      
      if( strncmp(field_module_name, "STANDARD_LINEAR", 15) == 0){  
	sprintf(arg, "%s/dump_paraview.vtu", work_dir);
	utr_write_paraview_std_lin(mesh_id, field_id, arg);
      }
      else{
	fprintf(interactive_output,"ParaView dump only for std_lin approx!\n");
      }


    }

    else if(c=='k'){
    	c='\0';
    	init_mod_fem_viewer(argc,argv,interactive_output);
    }

    // solving the problem with direct solver
    else if(c=='u'){
      
#ifndef PARALLEL

      /* very simple time measurement */
      t_wall=time_clock();

      /* problem dependent interface with linear solvers */
      sprintf(arg,"%s/pardiso.dat", work_dir);

      sir_direct_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg);

      /* very simple time measurement */
      t_wall=time_clock()-t_wall;
      
      fprintf(interactive_output,
	      "\nTime for solving a system of linear equations %lf\n\n",t_wall);
#endif

    }
    else if(c=='s'){
      
      
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
	appr_init_exchange_tables(pcr_nr_proc(), pcr_my_proc_id(), 1, &ione);
      }
#endif

      /* problem dependent interface with linear solvers */
      sprintf(arg,"%s/solver.dat", work_dir);

#ifndef PARALLEL
      sir_solve_lin_sys(problem_id, SIC_SEQUENTIAL, arg, // below: defaults
			-1, //pdr_lins_i_params(problem_id, 2), // max_iter
			-1, //pdr_lins_i_params(problem_id, 3), // error_type
			-1.0, //pdr_lins_d_params(problem_id, 4), // error_tol
			-1 //pdr_lins_i_params(problem_id, 5) // monitor level
			);
#endif
      
#ifdef PARALLEL
      sir_solve_lin_sys(problem_id, SIC_PARALLEL, arg, // below: defaults
			-1, //pdr_lins_i_params(problem_id, 2), // max_iter
			-1, //pdr_lins_i_params(problem_id, 3), // error_type
			-1.0, //pdr_lins_d_params(problem_id, 4), // error_tol
			-1 //pdr_lins_i_params(problem_id, 5) // monitor level
			);
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
    }

    // computing error
    else if(c=='e'){
      
      double error_h1;
      /* simple L2 and H1 error for the test problem (with known exact solution) */
      error_h1 = pdr_error_test(field_id,interactive_output);
      
    }
    else if(c=='z'){
      
      double error_ZZ;
      /* Zienkiewicz-Zhu error estimator */
      error_ZZ = pdr_zzhu_error(field_id,interactive_output);
      
    }

    // performing simple post-processing
    else if(c=='p'){
      
      pdr_post_process(field_id,interactive_input,interactive_output);
      
    }
    else if(c=='a'){
      
      /* adaptation based on Zienkiewicz-Zhu error estimator */
      pdr_adapt(problem_id, work_dir, 
		interactive_input, interactive_output );
      
    }

    // manual mesh adaptation
    else if(c=='m'){
      
      utr_manual_refinement( problem_id, work_dir, 
			     interactive_input, interactive_output );
      
      
    }

    // testing refinement/derefinement procedures
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
double Initial_condition(int Field_id, double* Coor, int Sol_comp_id)
{

  return(Coor[0]+Coor[1]+Coor[2]);
}

/*---------------------------------------------------------
  pdr_error_test - to compute error norm for test examples
----------------------------------------------------------*/
double pdr_error_test( /* returns H1 norm of error for */
				/* several known exact solutions */
  int Field_id,    /* in: approximation field ID  */
  FILE *interactive_output
		)
{

/* local variables */
  double l2_err = 0.0, h1_err = 0.0; /* error norms */

  int pdeg;		/* degree of polynomial */
  int pdeg_old=0; /* indicator for recomputing quadrature data */
  int base;		/* type of basis functions */
  int ngauss;            /* number of gauss points */
  double xg[3000];   	 /* coordinates of gauss points in 3D */
  double wg[1000];       /* gauss weights */

  int num_shap;         /* number of element shape functions */
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
  double dofs_loc[APC_MAXELSD]={0}; /* element solution dofs */

/* for scalar test cases - exact solution and its derivatives */
  double f, f_x, f_y, f_z;
/* auxiliary variables */
  int i, iel, ki, iaux, name, mat_num, mesh_id, nel, sol_vec_id, nreq, counter;
  double daux=0.0, eaux=0.0, volume=0.0, average=0.0, time=0.0;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the corresponding mesh */
  mesh_id = apr_get_mesh_id(Field_id);
  
  l2_err=0.0; h1_err=0.0;
  nel=0; counter=0;
  while((nel=mmr_get_next_act_elem(mesh_id,nel))!=0){
    
#ifdef PARALLEL
   if(mmpr_el_owner(mesh_id,nel)==pcr_my_proc_id()){
#endif

    counter++;
    
    /* find degree of polynomial and number of element scalar dofs */
    apr_get_el_pdeg(Field_id, nel, &pdeg);
    num_shap = apr_get_el_pdeg_numshap(Field_id, nel, &pdeg);
    nreq=apr_get_nreq(Field_id);

    /* get the coordinates of the nodes of El in the right order */
    mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
    
    /* get the most recent solution degrees of freedom */
    sol_vec_id = 1;
    apr_get_el_dofs(Field_id, nel, sol_vec_id, dofs_loc);
    
    
    /* prepare data for gaussian integration */
    if(pdeg!=pdeg_old){
      base=apr_get_base_type(Field_id,nel);
      apr_set_quadr_3D(base, &pdeg, &ngauss, xg, wg);
      pdeg_old = pdeg;
    }
    
    volume=0.0; average=0.0;
    for (ki=0;ki<ngauss;ki++) {
      
      /* at the gauss point, compute basis functions, determinant etc*/
      iaux = 2; /* calculations with jacobian but not on the boundary */
      determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base,
				&xg[3*ki],node_coor,dofs_loc,
				base_phi,base_dphix,base_dphiy,base_dphiz,
				xcoor,u_val,u_x,u_y,u_z,NULL);
      
      vol = determ * wg[ki];
      
      iaux=0;daux=0.0;
      pdr_exact_sol(iaux,xcoor[0],xcoor[1],xcoor[2],daux,
		    &f,&f_x,&f_y,&f_z,&eaux);
      
      
      volume += vol;
      average+= u_val[0]*vol;
      l2_err +=  (f-u_val[0])*(f-u_val[0])*vol;
      h1_err += ((f_x-u_x[0])*(f_x-u_x[0])+(f_y-u_y[0])*(f_y-u_y[0])
		 +(f_z-u_z[0])*(f_z-u_z[0]))*vol;
      
/*kbw
if(counter<10){
printf("at integration point %d, vol %lf, f %lf, sol %lf, l2_err %lf\n",
  ki, vol, f, u_val[0], sqrt(l2_err));
}
/*kew*/
      
    } /* ki */
    
/*kbw
printf("Average in element %d = %lf (at center = %lf, element volume %lf)\n",
nel,average/volume,dofs_loc[0]+dofs_loc[1]/3.0+dofs_loc[2]/3.0,volume);
/*kew*/
    
#ifdef PARALLEL
   } /* end if owned element */
#endif
    
  } /* end of loop over active elements */
  

#ifndef PARALLEL

  counter = apr_get_nrdofs_glob(Field_id);

  fprintf(interactive_output,
"\nLocal number of degrees of freedom: %d\n",	 counter);
  fprintf(interactive_output,
"Local L2 norm of error      = %20.15lf\n",	 sqrt(l2_err));
  fprintf(interactive_output,
"Local H1 seminorm of error  = %20.15lf\n\n",	 sqrt(h1_err));
#endif

#ifdef PARALLEL
  //counter = appr_get_nrdofs_owned(field_id);
  pcr_allreduce_sum_int(1, &counter, &iaux);
  pcr_allreduce_sum_double(1, &l2_err, &daux);
  l2_err = daux;
  pcr_allreduce_sum_double(1, &h1_err, &daux);
  h1_err = daux;
 
  if(pcr_my_proc_id()==pcr_print_master()){
    fprintf(interactive_output,
	    "\nGlobal number of degrees of freedom: %d\n",	 iaux);
    fprintf(interactive_output,
	    "Global L2 norm of error      = %20.15lf\n",	 sqrt(l2_err));
    fprintf(interactive_output,
	    "Global H1 seminorm of error  = %20.15lf\n\n",	 sqrt(h1_err));
  }
#endif


  return(l2_err+h1_err);
}

/*---------------------------------------------------------
  pdr_post_process - to privide simple interactive post-processing
----------------------------------------------------------*/
double pdr_post_process(
  int Field_id,    /* in: approximation field ID  */
  FILE *interactive_input,
  FILE *interactive_output
		)
{

/* local variables */

  int pdeg;		/* degree of polynomial */
  int base;		/* type of basis functions */

  int num_shap;         /* number of element shape functions */
  int ndofs;            /* local dimension of the problem */
  double determ;        /* determinant of jacobi matrix */
  double vol;           /* volume for integration rule */
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
  double xcoor[3];      /* global coord of gauss point */

/* for scalar test cases - exact solution and its derivatives */
  double f, f_x, f_y, f_z;
/* auxiliary variables */
  int i, j, iel, ki, iaux, name, mat_num, mesh_id, nel, sol_vec_id, nreq;
  double daux, eaux, volume, average, time;

  double x[3],xg[600],u_val_list[200],u_x_list[200],u_y_list[200],u_z_list[200];
  int list_el[200];

/*++++++++++++++++ executable statements ++++++++++++++++*/

  //double p1[3]={0.0,0.0,0.5},p2[3]={1.0,1.0,0.5};
  //apr_get_profile(stdout,Field_id,1,1,p1,p2,10);

  if(interactive_input!=stdin) return(-1);
  if(interactive_output!=stdout) return(-1);

  /* select the corresponding mesh */
  mesh_id = apr_get_mesh_id(Field_id);
  nreq=apr_get_nreq(Field_id);;

  printf("Give global coordinates of a point (x,y,z):\n");
  scanf("%lf",&x[0]);
  scanf("%lf",&x[1]);
  scanf("%lf",&x[2]);

  apr_sol_xglob(Field_id,x,1,list_el,xg,u_val_list,u_x_list,u_y_list,u_z_list,0);

  for(iel=1; iel<=list_el[0]; iel++){

	printf("\nSolution at local point %.2lf %.2lf %.2lf in element %d:\n\n",
	   xg[(iel-1)*3+0],xg[(iel-1)*3+1],xg[(iel-1)*3+2],list_el[iel]);

	for(j=0;j<nreq;j++)  {
	  printf("u_val[%d]=%lf, u_x[%d]=%lf, u_y[%d]=%lf, u_z[%d]=%lf\n",
		 j, u_val_list[(iel-1)*nreq+j], j, u_x_list[(iel-1)*nreq+j],
		 j, u_y_list[(iel-1)*nreq+j], j, u_z_list[(iel-1)*nreq+j]);
	}
  }

  printf("\nExact solution:\n");

  iaux=0;daux=0.0;
  pdr_exact_sol(iaux,x[0],x[1],x[2],daux,&f,&f_x,&f_y,&f_z,&eaux);

  for(j=0;j<nreq;j++) {
	printf("f[%d]=%lf, f_x[%d]=%lf, f_y[%d]=%lf, f_z[%d]=%lf\n"
	   ,j,f,j,f_x,j,f_y,j,f_z);
  }
  printf("\n");

  //  return(0);

  printf("Give element number:\n");
  scanf("%d",&nel);

  printf("Give local coordinates of a point (x,y,z):\n");
  scanf("%lf",&x[0]);
  scanf("%lf",&x[1]);
  scanf("%lf",&x[2]);
  //printf("x=%lf,y=%lf,z=%lf\n",x[0],x[1],x[2]);

  if(mmr_el_status(mesh_id, nel)==MMC_ACTIVE){

    base=apr_get_base_type(Field_id, nel);
    apr_get_el_pdeg(Field_id, nel, &pdeg);
    num_shap = apr_get_el_pdeg_numshap(Field_id, nel, &pdeg);
    ndofs = nreq*num_shap;
    
    /* get the coordinates of the nodes of El in the right order */
    mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
    
    /* get the most recent element solution degrees of freedom */
    sol_vec_id = 1;
    apr_get_el_dofs(Field_id, nel, sol_vec_id, dofs_loc2);
    
    iaux = 2; /* calculations with jacobian but not on the boundary */
    
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     x,node_coor,dofs_loc2,
		     base_phi,base_dphix,base_dphiy,base_dphiz,
		     xcoor,u_val,u_x,u_y,u_z,NULL);
    
    printf("\nSolution at point %.2lf %.2lf %.2lf in element %d:\n\n",
	   x[0],x[1],x[2],nel);
    
    for(j=0;j<nreq;j++) {
      printf("u_val[%d]=%lf, u_x[%d]=%lf, u_y[%d]=%lf, u_z[%d]=%lf\n",
	     j,u_val[j],j,u_x[j],j,u_y[j],j,u_z[j]);
    }
    printf("\nGlobal coordinates of the point:  %.2lf %.2lf %.2lf:\n\n",
	   xcoor[0],xcoor[1],xcoor[2]);
    
    printf("\nExact solution:\n");
    
    pdr_exact_sol(iaux,xcoor[0],xcoor[1],xcoor[2],daux,
		  &f,&f_x,&f_y,&f_z,&eaux);
    
    for(j=0;j<nreq;j++)  {
      printf("f[%d]=%lf, f_x[%d]=%lf, f_y[%d]=%lf, f_z[%d]=%lf\n"
	     ,j,f,j,f_x,j,f_y,j,f_z);
    }
    
    printf("\n");
    
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
  char c, d, field_type, keyword[300], arg[100];
  char problem_input_name[300];
  int meshfil_typ, num_bc, ibc,  pdeg;
  int nrno, nmno, nred, nmed, nrfa, nmfa, nrel, nmel;
  int imat;
  int i, j, iaux;
  double daux, t_wall;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  pdv_ctrl.name = 1;

  /*+++++++++++   opening file with problem data   ++++++++++++*/
  sprintf(problem_input_name,"%s/problem_laplace.dat", work_dir);
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
    fprintf(interactive_output,
	  "ERROR in reading input file for MESH_TYPE () \n");
    exit(-1);
  }
  fscanf(problem_input,"%c",&c);
  utr_skip_rest_of_line(problem_input);
  
  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"MESH_FILE",8) != 0) {
    fprintf(interactive_output,
	  "ERROR in reading input file for MESH_FILE\n");
    exit(-1);
  }
  fscanf(problem_input,"%s", arg);
  utr_skip_rest_of_line(problem_input);
  
  pdv_ctrl.mesh_id = utr_initialize_mesh(interactive_output, work_dir, c, arg);

  /*+++++++++++   initialization of approximation field data   ++++++++++++*/
  
  
  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"FIELD_SOURCE",8) != 0) {
    fprintf(interactive_output,
	  "ERROR in reading input file for FIELD_SOURCE\n");
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
    fprintf(interactive_output,
	  "ERROR in reading input file for NREQ\n");
    exit(-1);
  }
  fscanf(problem_input,"%d", &pdv_ctrl.nreq );
  utr_skip_rest_of_line(problem_input);
  if(pdv_ctrl.nreq > PDC_NREQ){
    fprintf(interactive_output,
	  "Parameter NREQ read from input file > PDC_NREQ\n");
    fprintf(interactive_output,
	  "Type any key (twice) to continue or CTRL C to quit\n"); 
    getchar();getchar();
  }
  
  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"NRSOL",4) != 0) {
    fprintf(interactive_output,
	  "ERROR in reading input file for NRSOL\n");
    exit(-1);
  }
  fscanf(problem_input,"%d", &pdv_ctrl.nr_sol );
  utr_skip_rest_of_line(problem_input);
  
  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"BASIS_FUNCTIONS",4) != 0) {
    fprintf(interactive_output,"ERROR in reading input file for BASIS_FUNCTIONS\n");
    exit(1);
  }
  fscanf(problem_input,"%c",&field_type);
  utr_skip_rest_of_line(problem_input);

  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"PDEG",4) != 0) {
    fprintf(interactive_output,"ERROR in reading input file for PDEG\n");
    exit(-1);
  }
  fscanf(problem_input,"%d", &pdeg );
  utr_skip_rest_of_line(problem_input);
  
  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"FIELD_FILE",8) != 0) {
    fprintf(interactive_output,
	  "ERROR in reading input file for FIELD_FILE\n");
    exit(-1);
  }
  fscanf(problem_input,"%s", arg);
  utr_skip_rest_of_line(problem_input);
  
  /* reading field values saved by previous runs */
  if(c=='r'){
    
    pdv_ctrl.field_id = utr_initialize_field(interactive_output,
      field_type,c,pdv_ctrl.mesh_id, 
      pdv_ctrl.nreq, pdv_ctrl.nr_sol, pdeg, arg, NULL);
    
  }
  /* initializing field values according to the specified function */
  else if(c=='i'){
    
    pdv_ctrl.field_id = utr_initialize_field(interactive_output,
      field_type,c,pdv_ctrl.mesh_id, 
      pdv_ctrl.nreq,pdv_ctrl.nr_sol,pdeg,NULL,&Initial_condition);
    
  }
  /* initializing field values to zero */
  else if(c=='z'){
    
      pdv_ctrl.field_id = utr_initialize_field(interactive_output,
        field_type,c,pdv_ctrl.mesh_id, 
        pdv_ctrl.nreq, pdv_ctrl.nr_sol, pdeg, NULL, NULL);
    
  }
  
  // ADAPTATION parameters - we always assume ZZ error estimation
  pdv_adpt.type = 2;

  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"ADAPT_MAXGEN_MAXGENDIFF",15) != 0) {
    fprintf(interactive_output,
	  "ERROR in reading input file for ADAPT_MAXGEN_MAXGENDIFF\n");
    exit(1);
  }
  fscanf(problem_input,"%d %d", &pdv_adpt.maxgen, &pdv_adpt.maxgendiff);
  utr_skip_rest_of_line(problem_input);
      
  // maximal generation for elements
  mmr_set_max_gen(pdv_ctrl.mesh_id, pdv_adpt.maxgen); 
  
  // irregularity constraints switched on/off 
  // (maxgendiff==0 - no constraints) 
  // (for constrained meshes only 1-irregular meshes allowed)
  if(pdv_adpt.maxgendiff > 0) 
    mmr_set_max_gen_diff(pdv_ctrl.mesh_id, pdv_adpt.maxgendiff);
  else mmr_set_max_gen_diff(pdv_ctrl.mesh_id, 0); 
  
  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"ADAPT_TOLERANCE_REF_UNREF",25) != 0) {
  	fprintf(interactive_output,
	  "ERROR in reading input file for ADAPT_TOLERANCE_REF_UNREF\n");
  	exit(1);
  }
  fscanf(problem_input,"%lg %lg",
  	     &pdv_adpt.eps, &pdv_adpt.ratio);
  utr_skip_rest_of_line(problem_input);
  
#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif
  fprintf(interactive_output,
"\nmaximal generation level set to %d, maximal generation difference set to %d\n", 
	  pdv_adpt.maxgen, pdv_adpt.maxgendiff);
#ifdef PARALLEL
    }
#endif

  /*++++++++ problem dependent data - in this case material constants ++++++++*/
  fscanf(problem_input,"%s\n",keyword);
  if(strncmp(keyword,"DIFFIUSON_COEFFICIENT",15) != 0) {
    fprintf(interactive_output,
	  "ERROR in reading input file for DIFFIUSON_COEFFICIENTS\n");
    exit(1);
  }
  fscanf(problem_input,"%lf\n",&daux);
#ifdef PARALLEL
    if(pcr_my_proc_id()==pcr_print_master()){
#endif
  fprintf(interactive_output, "Read diffusion coefficient: %lf\n\n", daux);
#ifdef PARALLEL
    }
#endif

  pdv_ctrl.nr_mat=1;

  pdv_material = (pdt_material *)malloc(sizeof(pdt_material));
  pdv_material->diff_coeff = daux;
  

  fclose(problem_input);

  return(1);
}
