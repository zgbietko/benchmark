/*************************************************************
lss_mkb_intf.c - file contains definitions of procedures:

  lsr_mkb_init - to create a new solver instance, read its control parameters
             and initialize its data structure
  lsr_mkb_create_matrix - to allocate space for a global system matrix
  lsr_mkb_create_precon - to create preconditioner blocks corresponding
                       to small subdomains of neighboring elements
  lsr_mkb_clear_matrix - to initialize block structure of system matrix
  lsr_mkb_assemble_local_sm - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
  lsr_mkb_fill_precon - to prepare preconditioner by factorizing the stiffness 
                    matrix, either only diagonal blocks or block ILU(0)
  lsr_mkb_solve - to solve a system of equations, given previously constructed
             system matrix, preconditioner
  lsr_mkb_free_matrix - to free space for solver data structure (system
                       matrix, preconditioner and possibly other)
  lsr_mkb_destroy - to destroy a particular instance of the solver
  lsr_get_pdeg_coarse - to get enforced pdeg for the coarse mesh
	  
History:
        08.2013 - Krzysztof Banas, initial version

*************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

/* provided interface of the solver - headers of routines defined in this file */
#include "./lsh_mkb_intf.h" 

/* internal information for the solver module */
#include "./include/it_mkb.h"
#include "./include/lah_block.h"
#include "./include/lah_petsc.h"

/* GLOBAL VARIABLES */
int   itv_nr_solvers=0;        /* the number of solvers in the problem */
int   itv_cur_solver_id;                /* ID of the current problem */
itt_solvers itv_solver[ITC_MAX_NUM_SOLV];        /* array of solvers */

/*** Procedures ***/

/*------------------------------------------------------------
  lsr_mkb_init - to create a new solver instance, read its control parameters
             and initialize its data structure
------------------------------------------------------------*/
int lsr_mkb_init( /* returns: >0 - solver ID, <0 - error code */
  int Parallel,   /* in: parameter specifying sequential (ITC_SEQUENTIAL)*/
                  /*     or parallel (ITC_PARALLEL) execution */
  int* Nr_levels_p,  /* in: requested number of levels for multigrid: */
                  /*     0 - read the number > 1 from supplied file Filename */
                  /*         if there is no file default GMRES is used */
                  /*     1 - enforce single level solver */
                  /*     >1 - enforce two level solver */
                  /* out: actual number of levels !!! */
  char* Filename,  /* in: name of the file with control parameters */	    
  int Max_iter, /* maximal number of iterations, -1 for values from Filename */
  int Error_type, /* type of error norm (stopping criterion), -1 for Filename*/
  double Error_tolerance, /* value for stopping criterion, -1.0 for Filename */ 
  int Monitoring_level /* Level of output, -1 for Filename */
  )
{

/* local variables */
  itt_levels *it_level, temp_level; /* mesh levels */
  int solver, gmres_type, krylbas;
  int i, ilev, nr_level; 

/* auxiliary variables */
  FILE *fp;
  char keyword[20];
 
/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* increase the counter for solvers */
  itv_nr_solvers++;
  
  /* set the current solver ID */
  itv_cur_solver_id = itv_nr_solvers;
  
  // initiate local variable
  nr_level = *Nr_levels_p;
  
#ifdef DEBUG_LSM
  if(LSC_MKB_MAX_NUM_LEV!=ITC_MAX_NUM_LEV || 
     LSC_MKB_MAX_NUM_SOLV!=ITC_MAX_NUM_SOLV){
    printf("Error 384207 in lsr_mkb_init!!! Exiting");
    exit(-1);
  }
#endif
  
/*kbw
    printf("\nIn lsr_init: Solver %d, Parallel %d, Nr_levels %d, Filename %s\n",
    itv_cur_solver_id, Parallel, nr_level, Filename);
    printf("Monitor %d, Nr_iter %d, Conv_type %d, Conv_meas %15.12lf\n",
    Monitoring_level, Max_iter, Error_type, Error_tolerance);
/*kew*/
  
  if (0 == strlen(Filename)){
        
    if(nr_level <= 1){
      
      itv_solver[itv_cur_solver_id].solver_id = itv_cur_solver_id;
      itv_solver[itv_cur_solver_id].parallel = Parallel;
      itv_solver[itv_cur_solver_id].nr_level = 1;
      it_level = &(itv_solver[itv_cur_solver_id].level[0]);
      
      it_level->Solver = GMRES;
      if(Max_iter<0) it_level->Max_iter = 100;
      else it_level->Max_iter = Max_iter;
      if(Error_type<0) it_level->Conv_type = REL_RES_INI;
      else  it_level->Conv_type = Error_type;
      if(Error_tolerance<0.0) it_level->Conv_meas = 1e-6; 
      else it_level->Conv_meas = Error_tolerance;
      if(Monitoring_level < 0) it_level->Monitor=ITC_SILENT; 
      else it_level->Monitor = Monitoring_level;
      
      /* default solver is single level GMRES with 50 Krylov vectors */
      it_level->GMRES_type = STANDARD_GMRES;
      it_level->Krylbas = 50;
      it_level->Pdeg = -1;
      it_level->Precon = BLOCK_GS; /* preconditioning by a single iteration */
      it_level->Nr_prec = 1;       /* of block Gauss-Seidel */
      it_level->Block_type = 1; /* single DOF structure subdomains */
      
      printf("MKB iterative solver not received correct control file\n");
      printf("Using default single level GMRES solver with simple block-GS preconditioning\n"); 
      if(Max_iter<0){
	printf("Using default value %d for max_iter\n", it_level->Max_iter);
      }
      if(Error_type<0){
	printf("Using default REL_RES_INI (0) - relative wrt resiual convergence measure\n");
      }
      if(Error_tolerance<0.0){
	printf("Using default convergence measure %lf\n", it_level->Conv_meas);
      }
      if(Monitoring_level < 0){
	printf("Using default monitoring level (0-silent)\n");
      }
      
  
    } // end if single level requested 
    else {
      
      printf("No default values for multigrid solver %d:\n",itv_cur_solver_id);
      printf("Specify linear solver parameters in proper input file (format: mkb.dat) \n");
      exit(0);
      
    }

  } /* end if no file with solver data */
  else {
    
    fp=NULL;
    /* read parameters from input file */
    fp=fopen(Filename,"r");
    if(fp==NULL) {
      printf("ERROR in opening solver input file %s !\n", Filename);
      exit(1);
    }

    fscanf(fp,"%s",keyword);  it_skip_rest_of_line(fp);

    if(strcmp(keyword,"FINE_LEVEL") != 0) {
      printf("ERROR in reading solver input file for FINE_LEVEL !\n");
      exit(1);
    }

    fscanf(fp,"%d %d %d", &solver, &nr_level, &krylbas );

    if(nr_level != *Nr_levels_p){
      printf("Solver called with nr_levels = %d.\n", *Nr_levels_p);
      printf("Read from input file nr_levels = %d.\n", nr_level);
      printf("Press any key (twice) to continue with enforced nr_levels = %d\n",
	     *Nr_levels_p);
      printf("or press CTRL C to stop\n");
      getchar();getchar();
      nr_level = *Nr_levels_p;
    }

    it_skip_rest_of_line(fp);

    gmres_type = STANDARD_GMRES;
      
    if(nr_level==1) {
      if(solver==MULTI_GMRES) solver = GMRES;
      if(solver==MULTI_GRID) solver = STANDARD_IT;
    }

    if(solver==GMRES||solver==STANDARD_IT){
      /* single level GMRES or standard iterative method */

      itv_solver[itv_cur_solver_id].solver_id = itv_cur_solver_id;
      itv_solver[itv_cur_solver_id].parallel = Parallel;
      itv_solver[itv_cur_solver_id].nr_level = 1;
      it_level = &(itv_solver[itv_cur_solver_id].level[0]);

      it_level->Solver = solver;
      it_level->GMRES_type = gmres_type;
      it_level->Krylbas = krylbas;
      it_level->Pdeg = -1;

      fscanf(fp,"%d %d %d",&(it_level->Precon),
	     &(it_level->Nr_prec),&(it_level->Block_type));
      it_skip_rest_of_line(fp);
      fscanf(fp,"%d %d %lg", &(it_level->Max_iter),
	     &(it_level->Conv_type),&(it_level->Conv_meas) );
      it_skip_rest_of_line(fp);
      fscanf(fp,"%d",  
	     &(it_level->Monitor));
      it_skip_rest_of_line(fp);

      if(Max_iter>0) it_level->Max_iter = Max_iter;
      if(Error_type>0) it_level->Conv_type = Error_type;
      if(Error_tolerance>0) it_level->Conv_meas = Error_tolerance;
      if(Monitoring_level > 0) it_level->Monitor = Monitoring_level;

      fclose(fp);

    } /* end if single level solver */
    else if(solver==MULTI_GMRES||solver==MULTI_GRID){

      itv_solver[itv_cur_solver_id].solver_id = itv_cur_solver_id;
      itv_solver[itv_cur_solver_id].parallel = Parallel;
      itv_solver[itv_cur_solver_id].nr_level = nr_level;

      /* fine level data */ 
      it_level = &(itv_solver[itv_cur_solver_id].level[nr_level-1]);

      it_level->Solver = solver;
      it_level->GMRES_type = gmres_type;
      it_level->Krylbas = krylbas;
      it_level->Pdeg = -1;
      
      ilev = nr_level-1;

      fscanf(fp,"%d %d %d",&(it_level->Precon),
	     &(it_level->Nr_prec),&(it_level->Block_type));
      it_skip_rest_of_line(fp);
      fscanf(fp,"%d %d %lg", &(it_level->Max_iter),
	     &(it_level->Conv_type),&(it_level->Conv_meas) );
      it_skip_rest_of_line(fp);
      fscanf(fp,"%d",  
	     &(it_level->Monitor));
      it_skip_rest_of_line(fp);
      
      if(Max_iter>0) it_level->Max_iter = Max_iter;
      if(Error_type>0) it_level->Conv_type = Error_type;
      if(Error_tolerance>0) it_level->Conv_meas = Error_tolerance;
      if(Monitoring_level > 0) it_level->Monitor = Monitoring_level;

      fscanf(fp,"%s",keyword); it_skip_rest_of_line(fp);
#ifdef DEBUG_LSM     
      if(strcmp(keyword,"COARSE_LEVEL") != 0) {
	printf("ERROR in reading solver input file for COARSE_LEVEL !\n");
	exit(1);
      }
#endif

      if(nr_level>1){

	/*coarse level data */
	it_level = &(itv_solver[itv_cur_solver_id].level[0]);

	/* read data from a file for the coarse solver */
	fscanf(fp,"%d %d %d", &(it_level->Solver),
	       &(it_level->Krylbas), &(it_level->Pdeg) );
	it_skip_rest_of_line(fp);
	fscanf(fp,"%d %d %d", &(it_level->Precon),
	       &(it_level->Nr_prec),&(it_level->Block_type) );
	it_skip_rest_of_line(fp);
	fscanf(fp,"%d %d %lg", &(it_level->Max_iter),
	       &(it_level->Conv_type),&(it_level->Conv_meas) );
	it_skip_rest_of_line(fp);
	fscanf(fp,"%d",  
	       &(it_level->Monitor));
	it_skip_rest_of_line(fp);
      
	fscanf(fp,"%s",keyword); it_skip_rest_of_line(fp);
#ifdef DEBUG_LSM     
	if(strcmp(keyword,"INTER_LEVELS") != 0) {
	  printf("ERROR in reading solver input file for INTER_LEVELS !\n");
	  exit(1);
	}
#endif
      } /* end if nr_level>1 */

      if(nr_level>2){

	/* intermediate levels data */
	it_level = &temp_level;
	/* read data from a file for the intermediate solvers */
	fscanf(fp,"%d %d %d",   
	       &(it_level->GMRES_type),&(it_level->Krylbas),&(it_level->Pdeg));
	it_skip_rest_of_line(fp);
	fscanf(fp,"%d %d %d",&(it_level->Precon),
	       &(it_level->Nr_prec),&(it_level->Block_type) );
	it_skip_rest_of_line(fp);
	fscanf(fp,"%d",  
	       &(it_level->Monitor) );
	it_skip_rest_of_line(fp);

      }
      
      fclose(fp);
      
      for(ilev=1;ilev<itv_solver[itv_cur_solver_id].nr_level-1;ilev++){
	
/* for the time being solver works in a multigrid fashion, creating */
/* intermediate solver structures at each mesh level */
/* (contrary to a domain decomposition fashion were solver structures */
/* are usually only for extreme coarse and fine mesh levels) */
	itv_solver[itv_cur_solver_id].cur_level=ilev;
	it_level = &(itv_solver[itv_cur_solver_id].level[ilev]);
	
	it_level->Solver = 10 ;
	it_level->GMRES_type = temp_level.GMRES_type ;
	it_level->Krylbas = temp_level.Krylbas ;
	it_level->Pdeg = temp_level.Pdeg ;
	it_level->Precon = temp_level.Precon ;
	it_level->Nr_prec = temp_level.Nr_prec ;
	it_level->Block_type = temp_level.Block_type ;
	it_level->Max_iter = 10000 ;
	it_level->Conv_type = 0 ;
	it_level->Conv_meas = 1e-15 ;
	it_level->Monitor = temp_level.Monitor ;
	
	
      } /* end loop over levels: ilev */
      
      if(itv_solver[itv_cur_solver_id].level[nr_level-1].Monitor>ITC_ERRORS)
	printf("Linear solver parameters read from file for solver %d:\n",
	       itv_cur_solver_id);
      
    } /* end if multi-level solver */
   
    else{
    
      if(itv_solver[itv_cur_solver_id].level[nr_level-1].Monitor>=ITC_ERRORS)
	printf("Wrong solver identification in lsr_mkb_init\n");
    
    }

  } /* end if found file with solver data */


  for(ilev=0;ilev<nr_level;ilev++){

    /* check data */
    itv_solver[itv_cur_solver_id].cur_level=ilev;
    it_level = &itv_solver[itv_cur_solver_id].level[ilev];

    if(nr_level>1 && it_level->Block_type==0) 
      it_level->Block_type=1;
    if(it_level->Solver==GMRES&&it_level->GMRES_type==MATRIX_FREE 
       && it_level->Block_type==0 ) it_level->Block_type=1;
    if(it_level->Precon == ADD_SCHWARZ && it_level->Block_type==0 ) 
      it_level->Block_type=1;
    if(it_level->Precon == MULTI_ILU) it_level->Block_type=1;
    if(it_level->Precon == BLOCK_ILU) it_level->Block_type=1;
    if(it_level->Precon == NO_PRECON) it_level->Block_type=0;
  
  }

/*ok_kbw*/
  if(itv_solver[itv_cur_solver_id].level[nr_level-1].Monitor>ITC_ERRORS){
    
    printf("\nInitiated data for solver %d: %d mesh levels with parameters:\n",
	   itv_cur_solver_id,itv_solver[itv_cur_solver_id].nr_level);
    
    for(ilev=0;ilev<itv_solver[itv_cur_solver_id].nr_level;ilev++){

      itv_solver[itv_cur_solver_id].cur_level=ilev;
      it_level = &(itv_solver[itv_cur_solver_id].level[ilev]);
      
      printf("\nLevel: %d\n", ilev);
      printf("Solver: %d, type/Presmooth  %d, Krylbas/Postsmooth %d, Pdeg %d\n",
	     it_level->Solver, it_level->GMRES_type, it_level->Krylbas,
	     it_level->Pdeg);
      printf("Preconditioner %d, Number of sweeps %d, Block type/size %d\n",
	     it_level->Precon, it_level->Nr_prec, it_level->Block_type);
      printf("Max_iter %d, Conv_type %d, Conv_meas %20.15lf\n",
	     it_level->Max_iter, it_level->Conv_type, it_level->Conv_meas); 
      printf("Monitor %d \n",
	     it_level->Monitor ); 
    }
  }
/*kew*/

  *Nr_levels_p = itv_solver[itv_cur_solver_id].nr_level;
  return(itv_cur_solver_id);

}


/*---------------------------------------------------------
  lsr_mkb_create_matrix - to allocate space for a global system matrix
---------------------------------------------------------*/
int lsr_mkb_create_matrix( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,   /* in: solver ID (used to identify the subproblem) */
  int Level_id,    /* in: level ID */
  int Nrblocks,    /* in: number of DOF blocks */
  int Nrdof_glob,  /* in: total number of DOFs */
  int Max_sm_size, /* in: maximal size of the stiffness matrix */
  int* Nrdofbl,	   /* in: list of numbers of dofs in a block */
  int* Posglob,	   /* in: list of global numbers of first dof */
  int* Nroffbl,	   /* in: list of numbers of off diagonal blocks */
  int** L_offbl	   /* in: list of lists of off diagonal blocks */
  )
{

  itt_levels *it_level;
  int i, j, info;

  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

  it_level = &itv_solver[Solver_id].level[Level_id];

/*kbw
  printf("before calling it_create_blocks: solver %d, level %d\n",
	 Solver_id, Level_id);
  printf("nrblocks %d, max_sm_size %d, nrdof_glob %d\n",
	 Nrblocks, Max_sm_size, Nrdof_glob);
  printf("\n");
  printf("block_id,\tnrdofbl,\tposglob,\tnroffbl\n");
  for(i=1; i<=Nrblocks; i++){
    printf("%d\t\t%d\t\t%d\t\t%d\n",i,Nrdofbl[i], Posglob[i], Nroffbl[i]);
    for(j=0;j<Nroffbl[i]; j++){
      printf("%10d",L_offbl[i][j]);
    }
    printf("\n");
  }
  printf("before calling it_create_blocks: solver %d, level %d\n",
	 Solver_id, Level_id);
/*kew*/

  it_level->Nrdofgl = Nrdof_glob;
  it_level->SM_and_LV_id = lar_allocate_SM_and_LV(Max_sm_size, Nrblocks, Nrdof_glob,
						  Nrdofbl, Posglob, Nroffbl, L_offbl, 
						  it_level->Block_type, it_level->Precon);

  //info = it_create_blocks(Solver_id, Level_id, Nrblocks, Nrdof_glob,
  //                            Nrdofbl, Posglob, Nroffbl, L_offbl);

  return(it_level->SM_and_LV_id);
}

/*---------------------------------------------------------
lsr_mkb_create_precon - to create preconditioner blocks corresponding
                       to small subdomains of neighboring elements
---------------------------------------------------------*/
int lsr_mkb_create_precon( /* returns:   >0 number of diagonal blocks */
                          /*	       <=0 - error */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id           /* in: level ID */
  )
{

  itt_levels *it_level;
  int info;

  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

  it_level = &itv_solver[Solver_id].level[Level_id];

  if(it_level->Precon != NO_PRECON){

/*kbw
  printf("before calling it_create_blocks_dia: solver %d, level %d\n",
	 Solver_id, Level_id);
/*kew*/

    int SM_and_LV_id = itv_solver[Solver_id].level[Level_id].SM_and_LV_id;
    info = lar_allocate_preconditioner(SM_and_LV_id);

    //info = it_create_blocks_dia(Solver_id, Level_id);

  }

  return(info);
}

/*---------------------------------------------------------
  lsr_mkb_clear_matrix - to initialize block structure of system matrix
---------------------------------------------------------*/
int lsr_mkb_clear_matrix( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,   /* in: solver ID (used to identify the subproblem) */
  int Level_id,    /* in: level ID */
  int Comp_type          /* in: indicator for the scope of computations: */
                         /*   LSC_MKB_SOLVE - solve the system */
                         /*   LSC_MKB_RESOLVE - resolve for the new rhs vector */
  )
{

/*kbw
  printf("before calling it_init_blocks: solver %d, level %d, comp_type %d\n",
	 Solver_id, Level_id, Comp_type);
/*kew*/

  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

  int SM_and_LV_id = itv_solver[Solver_id].level[Level_id].SM_and_LV_id;
  int info = lar_initialize_SM_and_LV(SM_and_LV_id, Comp_type);

  //int info = it_init_blocks(Solver_id, Level_id, Comp_type);

  return(info);
}

/*------------------------------------------------------------
  lsr_mkb_assemble_local_sm - to assemble entries to the global stiffness matrix
                           and the global load vector using the provided local 
                           stiffness matrix and load vector
------------------------------------------------------------*/
int lsr_mkb_assemble_local_sm( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id,          /* in: level ID */
  int Comp_type,         /* in: indicator for the scope of computations: */
                         /*   LSC_MKB_SOLVE - solve the system */
                         /*   LSC_MKB_RESOLVE - resolve for the new rhs vector */
  int Nr_dof_bl,         /* in: number of global dof blocks */
                         /*     associated with the local stiffness matrix */
  int* L_bl_id,          /* in: list of dof blocks' IDs */
  int* L_bl_nrdof,       /* in: list of blocks' numbers of dof */
  double* Stiff_mat,     /* in: stiffness matrix stored columnwise */
  double* Rhs_vect,      /* in: rhs vector */
  char* Rewr_dofs         /* in: flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
  )
{


/*kbw
  {
    int nrdofall=0; int i;
    printf("before calling it_assemble_blocks: solver %d, level %d, comp_type %d, rewr_dofs %c\n",
	   Solver_id, Level_id, Comp_type, *Rewr_dofs);
    printf("nr_dof_bl %d\n", Nr_dof_bl);
    printf("block_index,\tblock_id,\tnrdofbl\n");
    for(i=0; i<Nr_dof_bl; i++){
      printf("%d\t\t%d\t\t%d\t\t%d\n",i,L_bl_id[i], L_bl_nrdof[i]);
      nrdofall += L_bl_nrdof[i];
    }
    printf("SM\n");
    for(i=0; i<nrdofall*nrdofall; i++) printf("%10.6lf",Stiff_mat[i]);
    printf("\n");
    printf("LV\n");
    for(i=0; i<nrdofall; i++) printf("%10.6lf",Rhs_vect[i]);
    printf("\n");
  }
/*kew*/

  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

  int SM_and_LV_id = itv_solver[Solver_id].level[Level_id].SM_and_LV_id;
  int info = lar_assemble_SM_and_LV( SM_and_LV_id, Comp_type,
			       Nr_dof_bl, L_bl_id, L_bl_nrdof,
			       Stiff_mat, Rhs_vect, Rewr_dofs);

  //int info = it_assemble_blocks(Solver_id, Level_id, Comp_type,
  //			       Nr_dof_bl, L_bl_id, L_bl_nrdof,
  //			       Stiff_mat, Rhs_vect, Rewr_dofs);

  return(info);
}


/*---------------------------------------------------------
  lsr_mkb_fill_precon - to prepare preconditioner by factorizing the stiffness 
                    matrix, either only diagonal blocks or block ILU(0)
---------------------------------------------------------*/
int lsr_mkb_fill_precon(  
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id           /* in: level ID */
  )
{

/*kbw
  printf("before calling it_factor_blocks_dia: solver %d, level %d\n",
	 Solver_id, Level_id);
/*kew*/

  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

  //print_block_matrix()

  int SM_and_LV_id = itv_solver[Solver_id].level[Level_id].SM_and_LV_id;
  int info = lar_fill_preconditioner( SM_and_LV_id );

  //  int info = it_factor_blocks_dia(Solver_id, Level_id);

  return(info);
}

int amg_solve(int Solver_id, int nrdof_glob, int Ini_guess, double* X, double* B)
{
	printf("MY AMG SOLWER :)\n");
	petsc_perform_BJ_iterations(X, B, nrdof_glob, 1);
}

/*---------------------------------------------------------
  lsr_mkb_solve - to solve a system of equations, given previously constructed
             system matrix, preconditioner
---------------------------------------------------------*/
int lsr_mkb_solve( /* returns: convergence indicator: */
			/* 1 - convergence */
			/* 0 - noconvergence */
                        /* <0 - error code */
	int Solver_id,      /* in: solver ID */
	int Ndof, 	/* in: 	the number of degrees of freedom */
	int Ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* X, 	/* in: 	the initial guess */
			/* out:	the iterated solution */
        double* B,	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	int* Nr_iter, 	/* in:	the maximum iterations to be performed */
			/* out:	actual number of iterations performed */
	double* Toler, 	/* in:	tolerance level for chosen measure */
			/* out:	the final value of convergence measure */
	int Monitor,	/* in:	flag to determine monitoring level */
			/*	0 - silent run, 1 - warning messages */
			/*	2 - 1+restart data, 3 - 2+iteration data */
	double* Conv_rate /* out: convergence rate */
	)
{

  printf("BEFORE SOLVE %s\n\n\n", B == NULL ? "true" : "false");


  int nr_levels, ilev, nrdofgl, nrdofbl, max_nrdof;
  itt_levels *it_level; /* mesh levels */
  itt_blocks* block;         /* pointers to single blocks */
  itt_blocks_dia *block_dia; /* to simplify typing */
  //double *v_sol;
  double storage, total_storage; /* storage in MBytes */
  int i,j,k, iaux, ient, iblock;

/*++++++++++++++++ executable statements ++++++++++++++++*/


  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

/*------------------------------------------------------------*/
/* when monitoring the run compute the storage */
/*------------------------------------------------------------*/
  if(Monitor>ITC_ERRORS){

    int nroffbl, jaux, kaux;
    double daux;

    int nr_levels = itv_solver[Solver_id].nr_level;
    
    total_storage=0.0;
    
    for(ilev=0;ilev<nr_levels;ilev++){
      
      itv_solver[Solver_id].cur_level=ilev;
      it_level = &itv_solver[Solver_id].level[ilev];
      
      storage = 0.0; daux = 0.0;
      
      printf("\nLEVEL %d\n",ilev);
      
      // get storage from LA package
      daux = lar_get_storage( it_level->SM_and_LV_id );
      storage += daux;
      printf("Storage for SM, LV and preconditioner matrices: %f MBytes\n",daux);
      
      if(it_level->Solver==GMRES||it_level->Solver==MULTI_GMRES)    
	storage+=(it_level->Krylbas+1.0)*it_level->Nrdofgl
	  *sizeof(double)/1024.0/1024.0;
      printf("Storage for Krylov vectors %f MBytes\n",storage-daux);
      daux=storage;
      
      storage+= 2.0*it_level->Nrdofgl
	*sizeof(double)/1024.0/1024.0;
      printf("Storage for initial guess and solution vectors %f MBytes\n",
	     storage-daux);
      
      printf("Total storage for level: over %f MBytes\n",storage);
      total_storage += storage;
      printf("Total storage for solver: over %f MBytes\n",total_storage);
      
    } /* end loop over levels */
  } /* end if computing storage */

/*------------------------------------------------------------*/
/* SOLVE THE PROBLEM FOR A GIVEN LEVEL (WHEN NEEDED)          */
/*------------------------------------------------------------*/

  nr_levels = itv_solver[Solver_id].nr_level;
  nrdofgl = itv_solver[Solver_id].level[nr_levels-1].Nrdofgl;
  //v_sol = (double *)malloc(nrdofgl*sizeof(double));

  for(ilev=0;ilev<nr_levels;ilev++){

    itv_solver[Solver_id].cur_level=ilev;
    it_level = &(itv_solver[Solver_id].level[ilev]);

/* if it is the last (or the only) mesh or if we solve to get initial guess */
    //if( ilev==nr_levels-1 || FULL_MULTIGRID!=0 ){
    if( ilev==nr_levels-1  ){

      int iaux,jaux,kaux;
      double caux,daux,faux,eaux;

      /* take into account input parameters */
      if(*Nr_iter>0) it_level->Max_iter = *Nr_iter;
      if(*Toler>0) it_level->Conv_meas = *Toler;
      if(Monitor>0) it_level->Monitor = Monitor;

/* specify maximal number of iterations*/
      kaux=it_level->Max_iter; 

/* rewrite initial guess to enable computing of error in update */
      //for (i=0;i<it_level->Nrdofgl;i++) v_sol[i] = X[i];

/*------- AUXILIARY STANDARD ITERATIONS SOLVER  --------*/
      if(ilev==nr_levels-1 && 
	 ( it_level->Solver==STANDARD_IT || it_level->Solver==MULTI_GRID ) ){

/* specify kind and value of tolerance */
	daux = it_level->Conv_meas;

        iaux = it_standard(Solver_id, it_level->Nrdofgl, Ini_zero, X,
			   NULL, /* RHS vector taken from block data */
			   &kaux, &daux, it_level->Monitor, 
			   &caux);

/* prepare parameters for return */
	if(Toler!=NULL) *Toler = daux;
	if(Conv_rate!=NULL) *Conv_rate = caux;
/* number of performed iterations */
	if(Nr_iter!=NULL) *Nr_iter = kaux;

	if(it_level->Monitor>=ITC_INFO){
	  if(iaux)printf("Convergence in GS after %d iterations!!!\n",kaux);
	  else printf("No convergence in GS after %d iterations!!!\n",kaux);
	  printf("Total convergence rate (average decrease in update per iteration) %f\n",
		 caux);
	  printf("Norm of update %15.12f\n",daux);
	}

      }
/*------- DEFAULT GMRES SOLVER  --------*/
      else{

/* specify kind and value of tolerance */
	daux = 0.0;     /* relative to initial residual tolerance */
	eaux = 0.0;	/* residual tolerance */
	faux = 0.0;	/* relative to rhs residual tolerance */
	if(it_level->Conv_type==0) daux = it_level->Conv_meas;
	else if(it_level->Conv_type==1) eaux = it_level->Conv_meas;
	else if(it_level->Conv_type==2) faux = it_level->Conv_meas;

/*------- M A I N  S O L U T I O N  P R O C E D U R E --------*/

	//printf("calling gmres with parameters: \n:");
	//printf("Solver_id %d, it_level->Nrdofgl %d, Ini_zero %d\n",
	//       Solver_id, it_level->Nrdofgl, Ini_zero);
	//printf("it_level->Krylbas %d, kaux %d, eaux %15.12lf, daux %15.12lf, faux %15.12lf\n",
	//       it_level->Krylbas, kaux, eaux, daux, faux);

	iaux = it_gmres(Solver_id, it_level->Nrdofgl, Ini_zero, X,
			NULL, /* RHS vector taken from block data */
                        it_level->Krylbas, &kaux, &eaux, &daux, &faux,
		        it_level->Monitor, &caux);


    

    if(it_level->Monitor>=ITC_INFO){
	  if(iaux) printf("Convergence in GMRES after %d iterations!!!\n",kaux);
	  else  printf("No convergence in GMRES after %d iterations!!!\n",kaux);
	  printf("Total convergence rate (average decrease in residual per iteration) %f\n",
		 caux);
	  printf("Norm of residual %15.12f, relative decrease %15.12f\n",eaux,daux);
	  printf("Current ratio (norm of residual)/(norm of rhs) = %15.12f\n",faux);
	}

/* prepare parameters for return */
	if(Toler!=NULL){
	  if(it_level->Conv_type==0) *Toler = daux;
	  else if(it_level->Conv_type==1) *Toler = eaux;
	  else if(it_level->Conv_type==2) *Toler = faux;
	}
	if(Conv_rate!=NULL) *Conv_rate = caux;
/* number of performed iterations */
	if(Nr_iter!=NULL) *Nr_iter = kaux;
	
      }
 /*kbw
#ifdef DEBUG_LSM
    printf("level %d, nrdof_glob %d, Solution:\n", ilev, it_level->Nrdofgl );
    for(i=0;i<it_level->Nrdofgl;i++)printf("%20.15lf",X[i]);
    printf("\n");
    getchar();
#endif
/*kew*/

    } /* end if solving the problem for a given level */

  } /* the end of the main loop over levels */


  return(0);

}

/*---------------------------------------------------------
  lsr_mkb_free_matrix - to free space for solver data structure (system
                       matrix, preconditioner and possibly other)
---------------------------------------------------------*/
int lsr_mkb_free_matrix(
  int Solver_id   /* in: solver ID (used to identify the subproblem) */
  )
{

  int nr_levels, ilev;
  itt_levels *it_level;       /* mesh levels */

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

  nr_levels = itv_solver[Solver_id].nr_level;

  /* in a big loop over mesh levels */
  for(ilev=0;ilev<nr_levels;ilev++){

    it_level = &itv_solver[Solver_id].level[ilev];

    int SM_and_LV_id = itv_solver[Solver_id].level[ilev].SM_and_LV_id;
    
    if(it_level->Precon != NO_PRECON) lar_free_preconditioner(SM_and_LV_id);
    
    lar_free_SM_and_LV(SM_and_LV_id);
    
    //it_free_blocks(Solver_id, ilev);
    
    //if(it_level->Precon != NO_PRECON) it_free_blocks_dia(Solver_id, ilev);

  }
  return(0);
}


/*---------------------------------------------------------
  lsr_mkb_destroy - to destroy a particular instance of the solver
  (should be done in reverse order wrt creation)
---------------------------------------------------------*/
int lsr_mkb_destroy(
  int Solver_id   /* in: solver ID (used to identify the subproblem) */
  )
{

  /* set the current solver ID */
  itv_cur_solver_id = Solver_id;

  /* decrease the counter for solvers */
  itv_nr_solvers--;

  /* set the current solver ID */
  if(itv_cur_solver_id > itv_nr_solvers) itv_cur_solver_id = itv_nr_solvers;
  else{
    printf("Solver destroyed is not the last solver in lsr_mkb_destroy!!!\n");
    exit(0);
  }


  return(1);
}

/*---------------------------------------------------------
  lsr_get_pdeg_coarse - to get enforced pdeg for the coarse mesh
---------------------------------------------------------*/
int lsr_get_pdeg_coarse( // returns: enforced pdeg for the coarse mesh
  int Solver_id,   /* in: solver ID (used to identify the subproblem) */
  int Level_id /* in: level number */
  )
{
  itt_levels *it_level;

  it_level = &itv_solver[Solver_id].level[Level_id];

  return(it_level->Pdeg);
}


