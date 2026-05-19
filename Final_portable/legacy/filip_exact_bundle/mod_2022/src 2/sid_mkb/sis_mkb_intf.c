/****************************************************************
File sis_krylow_bliter_intf.c - implementation of the interface module
   between the iterative block based Krylow solver and the finite element
   code (the module forms part of the fem code): definition of parameters,
   data types, global variables and external functions)


Contains definitions of interface routines:
  sir_module_introduce - to return the solver name
  sir_solve_lin_sys - to perform the five steps below in one call
  sir_init - to create a new solver instance and read its control parameters
  sir_create - to create and initialize solver data structure
  sir_solve - to solve the system for a given data
  sir_free - to free memory for a given solver instance
  sir_destroy - to destroy a solver instance



------------------------------  			
History:        
	02.2002 - Krzysztof Banas, initial version		
****************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

/* problem dependent interface between approximation and solver modules */
#include "pdh_intf.h"

/* interface for general purpose linear solver modules */
#include "sih_intf.h" 

/* internal information for the krylow bliter solver interface module */
#include "./sih_mkb.h"

/* provided interface of the multigrid_krylow_bliter module */
#include "../lsd_mkb/lsh_mkb_intf.h"

//#define TIME_TEST


/*** CONSTANTS ***/
/* Solver (preconditioner) types */
const int SIC_SINGLE_LEVEL = 1;
const int SIC_TWO_LEVEL    = 2;
const int SIC_MULTI_LEVEL  = 0;

/* Options for parallel execution */
const int SIC_SEQUENTIAL = LSC_SEQUENTIAL;    
const int SIC_PARALLEL   = LSC_PARALLEL;    

/* Options for solution procedure */
const int SIC_SOLVE   = LSC_MKB_SOLVE;   /* solve the system */ 
const int SIC_RESOLVE = LSC_MKB_RESOLVE; /* resolve for new right hand side */ 

/* Monitoring options */
const int SIC_PRINT_NOT     = 0;  /* do not print anything */ 
const int SIC_PRINT_ERRORS  = 1;  /* print error messages only */
const int SIC_PRINT_INFO    = 2;  /* print most important information */
const int SIC_PRINT_ALLINFO = 3;  /* print all available information */


/*** GLOBAL VARIABLES (for the solver module) ***/
int   siv_nr_solvers = 0;    /* the number of solvers in the problem */
int   siv_cur_solver_id = 0;                /* ID of the current problem */
sit_solvers siv_solver[SIC_MAX_NUM_SOLV];        /* array of solvers */


const int FULL_MULTIGRID = 0;
const int GALERKIN_PROJ = 0; 

/* utility procedure */
#define SIC_LIST_END_MARK -1 /* number marking the end of list - cannot be put */
int sir_put_list( /* returns*/
		/*  >0 - position already occupied on the list */
             	/*  <0 - position at which put on the list */
            	/*   0 - list full, not found on the list */
	int Num, 	/* in: number to put on the list */
	int* List, 	/* in: list */
	int Ll		/* in: total list's lengths */
	);


/*------------------------------------------------------------
  sir_module_introduce - to return the solver name
------------------------------------------------------------*/
int sir_module_introduce( /* returns: >=0 - success code, <0 - error code */
  char* Solver_name  /* out: the name of the solver */
  )
{
  char* string = "MKB";

  strcpy(Solver_name,string);

  return(1);
}

/*------------------------------------------------------------
  sir_solve_lin_sys - to solve the system of linear equations for the current
          setting of parameters (with initialization and finalization phases)
------------------------------------------------------------*/
int sir_solve_lin_sys( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,    /* ID of the problem associated with the solver */
  int Parallel,      /* parameter specifying sequential (SIC_SEQUENTIAL) */
                     /* or parallel (SIC_PARALLEL) execution */
  char* Filename  /* in: name of the file with control parameters */
  )
{


  int comp_type, solver_id, monitor, ini_guess;
  char solver_name[100];

/*++++++++++++++++ executable statements ++++++++++++++++*/


  /*kbw
  printf("In sir_solve_lin_sys before init: problem_id %d\n",
	 problem_id);
  /*kew*/

  /* initialize the solver */
  solver_id = sir_init(Parallel, Filename);

  /*kbw
  printf("In sir_solve_lin_sys before create: solver_id %d, problem_id %d\n",
	 solver_id, problem_id);
  /*kew*/

  /* create the solver data structure and asociate it with a given problem */
  sir_create(solver_id, Problem_id);

  /*kbw
  printf("In sir_solve_lin_sys before solve: solver_id %d, problem_id %d\n",
	 solver_id, problem_id);
  /*kew*/

  /* launch the solver */
  comp_type = SIC_SOLVE;
  monitor = SIC_PRINT_INFO;
  ini_guess = 1;
  sir_solve(solver_id, comp_type, ini_guess, monitor, NULL, NULL, NULL);

  /*kbw
  printf("In sir_solve_lin_sys before free: solver_id %d, problem_id %d\n",
	 solver_id, problem_id);
  /*kew*/

  /* free the solver data structure - together with renumbering data */
  sir_free(solver_id);
  
  /*kbw
  printf("In sir_solve_lin_sys before destroy: solver_id %d, problem_id %d\n",
	 solver_id, problem_id);
  /*kew*/

  /* destroy the solver */
  sir_destroy(solver_id);

  return(0);
}

/*------------------------------------------------------------
  sir_init - to create a new solver, read its control parameters
             and initialize its data structure
------------------------------------------------------------*/
int sir_init( /* returns: >0 - solver ID, <0 - error code */
  int Parallel,      /* parameter specifying sequential (SIC_SEQUENTIAL) */
                     /* or parallel (SIC_PARALLEL) execution */
  char* Filename  /* in: name of the file with control parameters */
  )
{

  int info;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* increase the counter for solvers */
  siv_nr_solvers++;

  /* set the current solver ID */
  siv_cur_solver_id = siv_nr_solvers;
  siv_solver[siv_cur_solver_id].parallel = Parallel;
  siv_solver[siv_cur_solver_id].nr_levels = SIC_MULTI_LEVEL;

  // SIC_MULTI_LEVEL == 0
  info = lsr_mkb_init(Parallel, &siv_solver[siv_cur_solver_id].nr_levels, 
		      Filename);


#ifdef DEBUG_SIM
  if(info!=siv_cur_solver_id){
    printf("Error 213758 in sir_init!!! Exiting.\n");
    exit(-1);
  }
#endif

  /*ok_kbw*/
    printf("In sir_init: solver_id %d, Parallel %d, Nr_levels %d\n",
	   siv_cur_solver_id, siv_solver[siv_cur_solver_id].parallel, 
	   siv_solver[siv_cur_solver_id].nr_levels);
  /*kew*/

  return(siv_cur_solver_id);
}

/*------------------------------------------------------------
  sir_create - to create solver's data structure
------------------------------------------------------------*/
int sir_create( /* returns: >0 - solver ID, <0 - error code */
  int Solver_id,    /* in: solver identification */
  int Problem_id    /* ID of the problem associated with the solver */
  )
{


/* local variables */
  sit_levels *level_p; /* mesh levels */

  /* pointer to dofs structure */
  sit_dof_struct *dof_struct_p;
  
  /* the number of (different) mesh entities for which entries to the global */
  /* stiffness matrix and load vector will be provided */
  int nr_int_ent, nr_dof_ent, max_dof_per_ent, max_dof_int_ent, max_dof_ent_id;
  int nr_int_ent_fine, nr_dof_ent_fine, max_dof_per_ent_fine;
  int max_dof_int_ent_fine, max_dof_ent_id_fine, nrdof_glob_fine ;
  /* the global number of degrees of freedom */
  int nrdof_glob, int_ent_id, nrdof_ent_loc, idofent, dof_ent_id, pos_glob;
  int nr_dof_struct, dof_struct_id, nr_levels, nr_dof_ent_loc, pdeg_coarse;
  int *temp_list_dof_type, *temp_list_dof_type_fine;
  int *temp_list_dof_id, *temp_list_dof_id_fine;
  int *temp_list_dof_nrdof, *temp_list_dof_nrdof_fine;
  int l_dof_ent_types[SIC_MAX_DOF_PER_INT], l_dof_ent_ids[SIC_MAX_DOF_PER_INT];
  int l_dof_ent_nrdofs[SIC_MAX_DOF_PER_INT];
  int *l_bl_nrdof, *l_bl_posglob, *l_bl_nrneig, **l_bl_l_neig;

  /* auxiliary variables */
  int i,j,k, idof, iaux, ient, ineig, ibl, ilev;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  siv_cur_solver_id=Solver_id;
  siv_solver[Solver_id].problem_id = Problem_id;

  nr_levels = siv_solver[Solver_id].nr_levels;

  /* in a loop over levels */
  for(ilev=nr_levels-1;ilev>=0;ilev--){
    
    siv_solver[Solver_id].cur_level = ilev;
    level_p = &(siv_solver[Solver_id].level[ilev]);

  /*ok_kbw*/
    printf("In sir_create: solver_id %d, Problem_id %d, level %d\n",
	   Solver_id, Problem_id, ilev);
  /*kew*/

    if(ilev==nr_levels-1){

  /* get lists of integration and dof entities (types and IDs and nrdofs) */
  /* the order on the lists determine the order of DOF structures !!! */
      pdr_get_list_ent(Problem_id, &nr_int_ent, 
		       &level_p->l_int_ent_type, &level_p->l_int_ent_id, 
                       &nr_dof_ent, &temp_list_dof_type, 
		       &temp_list_dof_id, &temp_list_dof_nrdof, 
		       &nrdof_glob, &max_dof_per_ent);
      pdeg_coarse = SIC_PDEG_FINEST;
      level_p->pdeg_coarse = pdeg_coarse;

    }
    else{

      nr_int_ent_fine = siv_solver[Solver_id].level[ilev+1].nr_int_ent;
      nr_dof_ent_fine = siv_solver[Solver_id].level[ilev+1].nr_dof_ent;
      nrdof_glob_fine = siv_solver[Solver_id].level[ilev+1].nrdof_glob;
      max_dof_per_ent_fine = max_dof_per_ent;

      temp_list_dof_type_fine = 
	(int *) malloc( (nr_dof_ent_fine+1)*sizeof(int) );
      temp_list_dof_id_fine = 
	(int *) malloc( (nr_dof_ent_fine+1)*sizeof(int) );
      temp_list_dof_nrdof_fine = 
	(int *) malloc( (nr_dof_ent_fine+1)*sizeof(int) );

      for(i=0;i<nr_dof_ent_fine;i++){
	temp_list_dof_type_fine[i] = temp_list_dof_type[i];
	temp_list_dof_id_fine[i] = temp_list_dof_id[i];
	temp_list_dof_nrdof_fine[i] = temp_list_dof_nrdof[i];
	
      }

      free(temp_list_dof_type);
      free(temp_list_dof_id);
      free(temp_list_dof_nrdof);

      //works only for uniform approximation !!!!
      pdeg_coarse = lsr_get_pdeg_coarse(Solver_id, ilev); 

  /* get lists of integration and dof entities (types and IDs and nrdofs) */
  /* the order on the lists determine the order of DOF structures !!! */
      pdr_get_list_ent_coarse(Problem_id, nr_int_ent_fine, 
		       siv_solver[Solver_id].level[ilev+1].l_int_ent_type,
		       siv_solver[Solver_id].level[ilev+1].l_int_ent_id,
                       nr_dof_ent_fine, temp_list_dof_type_fine, 
		       temp_list_dof_id_fine, temp_list_dof_nrdof_fine, 
		       nrdof_glob_fine, max_dof_per_ent_fine,
		       &pdeg_coarse, &nr_int_ent, 
		       &level_p->l_int_ent_type, &level_p->l_int_ent_id, 
		       &nr_dof_ent, &temp_list_dof_type, 
		       &temp_list_dof_id, &temp_list_dof_nrdof, 
			      &nrdof_glob, &max_dof_per_ent);

#ifdef DEBUG_SIM
      if(pdeg_coarse<=0){
	printf("Error in pdeg_coarse obtained from pdr_get_list_ent_coarse, %d\n"
	       , pdeg_coarse);
	exit(-1);
      }
#endif

      level_p->pdeg_coarse = pdeg_coarse;

      free(temp_list_dof_type_fine);
      free(temp_list_dof_id_fine);
      free(temp_list_dof_nrdof_fine);

    }

#ifdef DEBUG_SIM
    if ( max_dof_per_ent > SIC_MAX_DOF_PER_INT ){
      printf("Error 87232 in sis_mkb_intf/sir_create!!! Exiting\n");
      exit(-1);
    }
#endif


/*kbw
    printf("In sir_create after pdr_get_list_ent\n");
    printf("nr_int_ent %d\n", nr_int_ent);
    for(i=0;i<nr_int_ent;i++)  printf("type %d, id %d\n",
	    level_p->l_int_ent_type[i],level_p->l_int_ent_id[i]);
    printf("\nNr_dof_ent %d, Nrdof_glob %d, Max_dof_per_ent %d\n",
	   nr_dof_ent, nrdof_glob, max_dof_per_ent); 
    for(i=0;i<nr_dof_ent;i++)  printf("type %d, id %d, nrdof %d\n",
	    temp_list_dof_type[i], temp_list_dof_id[i],
				      temp_list_dof_nrdof[i]);
/*kew*/

    level_p->nr_int_ent = nr_int_ent;
    level_p->nr_dof_ent = nr_dof_ent;
    level_p->nr_dof_bl = nr_dof_ent;
    level_p->nrdof_glob = nrdof_glob;

  /* array of structures storing DOF data */
    level_p->l_dof_struct = 
      (sit_dof_struct *)malloc( nr_dof_ent*sizeof(sit_dof_struct) );

  /* renumbering array - block_id to dof_struct_index */
    level_p->l_bl_to_struct = (int *)malloc( nr_dof_ent*sizeof(int) );
  /* renumbering array - dof_struct_index to block_id */
  /* for the beginning no renumbering: dof_ent_id = dof_struct_index = block_id*/
    for(i=0; i< nr_dof_ent; i++) level_p->l_bl_to_struct[i]=i;

  /* the last array (for DG - for other approximations there may be 
     more arrays necessary, one for each dof_ent type) */
  /* dof_ent_index to dof_struct_index (based on which dof_ent_id and */
  /* dof_ent_type can be find */
    max_dof_ent_id = 0;
    for(i=0; i< nr_dof_ent; i++){

#ifdef DEBUG_SIM
      if ( temp_list_dof_type[i] != PDC_ELEMENT ){
	printf("Error 8732 in sis_mkb_intf/sir_create!!! Exiting\n");
	exit(-1);
      }
#endif

      if ( temp_list_dof_id[i] > max_dof_ent_id ) 
                                     max_dof_ent_id = temp_list_dof_id[i];

    }

    level_p->max_dof_ent_id = max_dof_ent_id;
    level_p->l_dof_ent_to_struct = (int*)malloc((max_dof_ent_id+1)*sizeof(int)); 

    for(i=0;i<max_dof_ent_id;i++) level_p->l_dof_ent_to_struct[i] = -1;

  /* in case of no renumbering global position of dof block is specified */
    pos_glob = 0;
    for(idof = 0; idof < nr_dof_ent; idof++){

      dof_struct_p = &level_p->l_dof_struct[idof];

      nrdof_ent_loc = temp_list_dof_nrdof[idof];
      if(nrdof_ent_loc > level_p->max_dof_dof_ent) 
	level_p->max_dof_dof_ent = nrdof_ent_loc;
      dof_struct_p->dof_ent_type = temp_list_dof_type[idof];
      dof_struct_p->dof_ent_id = temp_list_dof_id[idof];
      dof_struct_p->nrdof = nrdof_ent_loc;
  /* for the beginning no renumbering: dof_struct_index = block_id*/
      dof_struct_p->block_id = idof;
      dof_struct_p->posglob = pos_glob;
      pos_glob += nrdof_ent_loc;

    /* initialize lists of integration entities and neighbouring dof_ent */
      dof_struct_p->nr_int_ent = 0;
      for(i=0;i<SIC_MAX_INT_PER_DOF;i++) 
	dof_struct_p->l_int_ent_index[i]=SIC_LIST_END_MARK;
      dof_struct_p->nrneig = 0;
      for(i=0;i<SIC_MAX_DOF_STR_NGB;i++) 
	dof_struct_p->l_neig[i]=SIC_LIST_END_MARK;

    /* for DG - dofs are associated with elements only */
      level_p->l_dof_ent_to_struct[dof_struct_p->dof_ent_id] = idof;

#ifdef DEBUG_SIM
      if(dof_struct_p->dof_ent_id > max_dof_ent_id){
	printf("Error 84543732 in sis_mkb_intf/sir_create!!! Exiting\n");
	printf("%d > %d\n", dof_struct_p->dof_ent_id, max_dof_ent_id-1);
	exit(-1);
      }
#endif

/*kbw
    printf("In sir_create after filling dof_struct no %d\n", idof);
    printf("dof_ent_type %d, dof_ent_id %d, nrdof %d, posglob %d\n",
	   dof_struct_p->dof_ent_type , dof_struct_p->dof_ent_id, 
	   dof_struct_p->nrdof, dof_struct_p->posglob);
    printf("Initialized lists of int_ent %d, neig %d\n",
	   dof_struct_p->l_int_ent_index[0], dof_struct_p->l_neig[0]); 
/*kew*/

    }

#ifdef DEBUG_SIM
    if ( level_p->nrdof_glob != pos_glob ){
      printf("Error 843732 in sis_krylow_bliter_intf/sir_create!!! Exiting\n");
      exit(-1);
    }
#endif

  /* getting information on the structure of the global stiffness matrix */
    nr_dof_struct = 0;
    max_dof_int_ent = 0;
    for(ient=0; ient<level_p->nr_int_ent;ient++){
      int nrdof_int_ent = 0;
      int_ent_id = level_p->l_int_ent_id[ient];
      nr_dof_ent_loc = SIC_MAX_DOF_PER_INT;
 
      pdr_comp_stiff_mat(Problem_id, level_p->l_int_ent_type[ient], 
			 level_p->l_int_ent_id[ient], PDC_NO_COMP, pdeg_coarse,
			 &nr_dof_ent_loc, l_dof_ent_types, 
			 l_dof_ent_ids, l_dof_ent_nrdofs,
			 NULL, NULL, NULL, NULL);
      
#ifdef DEBUG_SIM
      if ( nr_dof_ent_loc > SIC_MAX_DOF_PER_INT ){
	printf("Error 87232 in sis_mkb_intf/sir_create!!! Exiting\n");
	exit(-1);
      }
#endif
      
/*kbw
   printf("in sir_create after pdr_comp_stiff_mat for int_ent no %d (id %d):\n", 
	   ient, int_ent_id);
    printf("level %d, nr_dof_ent_loc %d, types, ids, nrdofs:\n", 
	   ilev, nr_dof_ent_loc);
    for(idofent=0; idofent<nr_dof_ent_loc; idofent++){
      printf("%d %d %d\n", l_dof_ent_types[idofent], 
	     l_dof_ent_ids[idofent], l_dof_ent_nrdofs[idofent]);
    }
/*kew*/

      for(idofent=0; idofent<nr_dof_ent_loc; idofent++){
	
	dof_ent_id = l_dof_ent_ids[idofent];
	
#ifdef DEBUG_SIM
	if( level_p->l_dof_ent_to_struct[dof_ent_id] == -1){
	  printf("Error 347294 in sir_create!!! Exiting\n");
	  exit(-1);
	}
#endif

	dof_struct_id = level_p->l_dof_ent_to_struct[dof_ent_id];
	
	dof_struct_p = &level_p->l_dof_struct[dof_struct_id];

/*kbw
	printf("for int_type %d, int_id %d, dof_type %d, dof_id %d, struct %d\n",
	       level_p->l_int_ent_type[ient], level_p->l_int_ent_id[ient],
	       l_dof_ent_types[idofent], l_dof_ent_ids[idofent],dof_struct_id );
/*kew*/

#ifdef DEBUG_SIM
	if((dof_struct_p->dof_ent_id != dof_ent_id) || 
	   (dof_struct_p->nrdof != l_dof_ent_nrdofs[idofent]) ){
	  printf("Error 3827 in sir_create!!! Exiting");
	  exit(-1);
	}
#endif      

	nrdof_int_ent += l_dof_ent_nrdofs[idofent];

/*kbw
      printf("putting int_ent no %d on the list of int_ent, nr_int_ent %d\n", 
	     ient, dof_struct_p->nr_int_ent);
      printf("before:");
      for(i=0;i<SIC_MAX_INT_PER_DOF;i++) {
	printf("%d",dof_struct_p->l_int_ent_index[i]) ;
      }
      printf("\n");
/*kew*/

	iaux=sir_put_list(ient, 
			  dof_struct_p->l_int_ent_index, SIC_MAX_INT_PER_DOF);
	if(iaux<0) dof_struct_p->nr_int_ent++;

#ifdef DEBUG_SIM
	if(iaux == 0){ // list full - increase SIC_MAX_INT_PER_DOF
	  printf("Error 383627 in sir_create!!! Exiting");
	  exit(-1);
	}
#endif      
/*kbw
      printf("putting int_ent no %d on the list of int_ent, nr_int_ent %d\n", 
	     ient,dof_struct_p->nr_int_ent);
      printf("after:");
      for(i=0;i<SIC_MAX_INT_PER_DOF;i++) {
	printf("%d",dof_struct_p->l_int_ent_index[i]) ;
      }
      printf("\n");
/*kew*/

	for(ineig = 0; ineig<nr_dof_ent_loc; ineig++){ 
	  
	  /* change for constrained approximation */
	  /* ineig may be != idofent but this is the same dof_ent */
	  if(dof_ent_type != l_dof_ent_types[ineig] ||
	     dof_ent_id != l_dof_ent_ids[ineig]){	
	    
	    int neig_id = l_dof_ent_ids[ineig];
	    int neig_index = level_p->l_dof_ent_to_struct[neig_id];
	    
/*kbw
      printf("dof_ent %d: putting ineig no %d (id %d, index %d) on the list of neig, nrneig %d\n", 
	     idofent, ineig, neig_id, dof_struct_p->nrneig);
      printf("before:");
      for(i=0;i<SIC_MAX_DOF_STR_NGB;i++) {
	printf("%d",dof_struct_p->l_neig[i]) ;
      }
      printf("\n");
/*kew*/

	    iaux=sir_put_list(neig_index, 
			      dof_struct_p->l_neig, SIC_MAX_DOF_STR_NGB);
	    if(iaux<0) {
	      
	      dof_struct_p->nrneig++;
	      
	    }

#ifdef DEBUG_SIM
	    if(iaux == 0){ // list full - increase SIC_MAX_DOF_STR_NGB
	      printf("Error 385627 in sir_create!!! Exiting");
	      exit(-1);
	    }
#endif      

/*kbw
      printf("putting ineig no %d (id %d) on the list of neig, nrneig %d\n", 
	     ineig, neig_id, dof_struct_p->nrneig);
      printf("after:");
      for(i=0;i<SIC_MAX_DOF_STR_NGB;i++) {
	printf("%d",dof_struct_p->l_neig[i]) ;
      }
      printf("\n");
/*kew*/

	  }

	}

      } /* end loop over dof_ents of a given int_ent */

      if(nrdof_int_ent > max_dof_int_ent) max_dof_int_ent = nrdof_int_ent; 

    } /* end loop over int_ent */

    level_p->max_dof_int_ent = max_dof_int_ent;

  } // end loop over levels
 
  free(temp_list_dof_type);
  free(temp_list_dof_id);
  free(temp_list_dof_nrdof);


    // CALL TO RENUMBERING PROCEDURE ?


  /* in a loop over levels */
  for(ilev=nr_levels-1;ilev>=0;ilev--){
    
    siv_solver[Solver_id].cur_level = ilev;
    level_p = &(siv_solver[Solver_id].level[ilev]);
    

  /* allocate memory for temporary lists of neighbors */
    l_bl_nrdof =  (int *)malloc((level_p->nr_dof_bl+1)*sizeof(int));
    l_bl_posglob = (int *)malloc((level_p->nr_dof_bl+1)*sizeof(int));
    l_bl_nrneig = (int *)malloc((level_p->nr_dof_bl+1)*sizeof(int));
    l_bl_l_neig = (int **)malloc((level_p->nr_dof_bl+1)*sizeof(int *));

/*kbw
  printf("filling temp lists for mkb_create_matrix: solver %d, level %d (%lu)\n",
	 Solver_id, ilev, level_p);
  printf("nrblocks %d, max_sm_size %d, nrdof_glob %d\n",
	 level_p->nr_dof_bl, max_dof_int_ent, level_p->nrdof_glob);
  printf("\tnrdofbl,\tposglob,\tnroffbl\n");
/*kew*/

  /* !!! offset 1 numbering of blocks in it_bliter !!! */
    for(ibl = 1; ibl<= level_p->nr_dof_bl; ibl++){

    /* renumbering !*/
    /* !!! offset 1 numbering of blocks in it_bliter !!! */
      dof_struct_id = level_p->l_bl_to_struct[ibl-1];

      dof_struct_p = &level_p->l_dof_struct[dof_struct_id];

/*kbw
      printf("dof_struct_p %lu\n", dof_struct_p);
      printf("\t%d\t\t%d\t\t%d\n",
	     dof_struct_p->nrdof, dof_struct_p->posglob, dof_struct_p->nrneig);
      for(ineig=0;ineig<dof_struct_p->nrneig;ineig++){
	int iaux=dof_struct_p->l_neig[ineig];
	printf("%10d",level_p->l_dof_struct[iaux].block_id+1);
      }
      printf("\n");
/*kew*/

      l_bl_nrdof[ibl] = dof_struct_p->nrdof;
      l_bl_posglob[ibl] = dof_struct_p->posglob;
      l_bl_nrneig[ibl] = dof_struct_p->nrneig;
      l_bl_l_neig[ibl]= (int *)malloc(l_bl_nrneig[ibl]*sizeof(int));
      for(ineig=0;ineig<l_bl_nrneig[ibl];ineig++){
	int iaux=dof_struct_p->l_neig[ineig];
	/* !!! offset 1 numbering of blocks in it_bliter !!! */
	l_bl_l_neig[ibl][ineig]=level_p->l_dof_struct[iaux].block_id+1;
      }
    }
  
/*kbw
  printf("before calling mkb_create_matrix: solver %d, level %d\n",
	 Solver_id, ilev);
  printf("nrblocks %d, max_sm_size %d, nrdof_glob %d\n",
	 level_p->nr_dof_bl, max_dof_int_ent, level_p->nrdof_glob);
  printf("\tnrdofbl,\tposglob,\tnroffbl\n");
  for(ibl = 1; ibl<= level_p->nr_dof_bl; ibl++){
    printf("\t%d\t\t%d\t\t%d\n",l_bl_nrdof[ibl], l_bl_posglob[ibl], 
	   l_bl_nrneig[ibl]);
    printf("\n");
    for(j=0;j<l_bl_nrneig[ibl]; j++){
      printf("%10d",l_bl_l_neig[ibl][j]);
    }
    printf("\n");
  }
/*kew*/

    lsr_mkb_create_matrix(Solver_id, ilev, level_p->nr_dof_bl, 
			  level_p->nrdof_glob,
			  max_dof_int_ent, l_bl_nrdof, l_bl_posglob, 
			  l_bl_nrneig, l_bl_l_neig);
    
    
    for(ibl=1;ibl<=level_p->nr_dof_bl;ibl++){
      free(l_bl_l_neig[ibl]);
    }
    free(l_bl_nrdof);
    free(l_bl_posglob);
    free(l_bl_nrneig);
    free(l_bl_l_neig);
    
  /* create preconditioner data structure */
    lsr_mkb_create_precon(Solver_id, ilev);

/*kbw

    sit_blocks *block;	
    int iblock,iaux;

    level_p = &siv_solver[siv_cur_solver_id].level[ilev];
    for(iblock=1;iblock<=level_p->Nrblocks;iblock++){
      block = level_p->Block[iblock];
      iaux=level_p->Block[iblock]->Ndof;
      printf("Block %d, ndof %d, Posg %d\n   Neighbors:",
	     iblock,level_p->Block[iblock]->Ndof, 
	     level_p->Block[iblock]->Posg);
      for(i=1;i<=level_p->Block[iblock]->Lngb[0];i++)
	printf("  %d",level_p->Block[iblock]->Lngb[i]);
      printf("\n");
      getchar();
    }
/*kew*/

  } // end loop over levels

  return(siv_cur_solver_id);
}


/*------------------------------------------------------------
sir_solve - to solve the system for a given data
------------------------------------------------------------*/
int sir_solve(/* returns: >=0 - success code, <0 - error code */
  int Solver_id,     /* in: solver identification */
  int Comp_type,     /* in: indicator for the scope of computations: */
                     /*   SIC_SOLVE - solve the system */
                     /*   SIC_RESOLVE - resolve for the new right hand side */
  int Ini_guess,     /* in: indicator on whether to set initial guess (1), */
                     /*     or to initialize it to zero (0) */
  int Monitor,       /* in: monitoring flag with options: */
                     /*   SIC_PRINT_NOT - do not print anything */ 
                     /*   SIC_PRINT_ERRORS - print error messages only */
                     /*   SIC_PRINT_INFO - print most important information */
                     /*   SIC_PRINT_ALLINFO - print all available information */
  int *Nr_iter,	     /* out (optional): actual number of performed iterations */
  double *Conv_meas, /* out (optional): actual convergence measure */
  double *Conv_rate  /* out (optional): the total convergence rate */ 
  )
{

  /* pointer to solver structure */
  sit_solvers *solver_p;
  sit_levels *level_p; /* mesh levels */

  /* pointer to dofs structure */
  sit_dof_struct *dof_struct_p;
  
  /* auxiliary variables */
  int nrdof_glob, max_nrdof, nr_dof_ent, nr_levels, posglob, nrdofs_int_ent;
  int l_dof_ent_id[SIC_MAX_DOF_PER_INT], l_dof_ent_nrdof[SIC_MAX_DOF_PER_INT];
  int l_dof_ent_posglob[SIC_MAX_DOF_PER_INT];
  int l_dof_ent_type[SIC_MAX_DOF_PER_INT];
  int pdr_comp_type, pdeg_coarse;
  double *stiff_mat, *rhs_vect, *x_ini, normb;
  int i,j,k, iaux, kaux, intent, idofent, ibl, ient, nrdofbl, ini_zero, level_id;
  char rewrite;
  int l_bl_id[SIC_MAX_DOF_PER_INT], l_bl_nrdof[SIC_MAX_DOF_PER_INT];


/* variables to store timing results */
  double t_int_el=0.0;
  double t_int_fa=0.0;
  double t_fac_dia=0.0;
  double t_iter=0.0;
  double t_temp=0.0;
  double t_total=0.0;
  double su_getdaytime();

/*++++++++++++++++ executable statements ++++++++++++++++*/

  siv_cur_solver_id=Solver_id;
  solver_p = &siv_solver[siv_cur_solver_id];
  nr_levels = siv_solver[Solver_id].nr_levels;

  level_id = nr_levels-1;
  level_p = &(siv_solver[Solver_id].level[level_id]);

  /*ok_kbw*/
  printf("in sir_solve: solver %d, nr_levels %d, level_p %lu, nrdof_glob %d\n",
	 Solver_id, nr_levels, level_p, level_p->nrdof_glob);
  /*kew*/


#ifdef TIME_TEST
  t_total=su_getdaytime();
#endif

  /* allocate memory for the initial guess and solution vectors */
  nrdof_glob = siv_solver[Solver_id].level[nr_levels-1].nrdof_glob;
  x_ini = (double *)malloc(nrdof_glob*sizeof(double));
  ini_zero=1;
  for(i=0;i<nrdof_glob;i++) x_ini[i]=0.0;
  
/*------------------------------------------------------------*/
/* GET INITIAL GUESS WHEN POSSIBLE AND NECESSARY              */
/*------------------------------------------------------------*/
  if(Ini_guess==1){
    
    ini_zero=0;
    
    /* get initial guess */
    iaux = 1; /* read from the solution vector no 1 */
    for(ibl=0;ibl<level_p->nr_dof_bl;ibl++){
      int dof_struct_id = level_p->l_bl_to_struct[ibl];
      sit_dof_struct dof_struct = level_p->l_dof_struct[dof_struct_id];

  /*kbw
      printf("in sir_solve before pdr_read_sol_dofs: ibl %d, struct_id %d \n",
	     ibl, dof_struct_id);
      printf("dof_struct_p %lu\n", dof_struct_p);
  printf("problem_id %d, Dof_ent_type %d, Dof_ent_id %d, nrdof %d\n",
	solver_p->problem_id, dof_struct.dof_ent_type, dof_struct.dof_ent_id,  
	 dof_struct.nrdof);
  /*kew*/

      
      pdr_read_sol_dofs(solver_p->problem_id, 
			dof_struct.dof_ent_type,
			dof_struct.dof_ent_id,
			dof_struct.nrdof,
			&x_ini[dof_struct.posglob]);
      
    }
    
/*kbw
  if(level_id>=0){
    printf("x_ini before exchange dofs\n");
    for(i=0;i<nrdof_glob;i++) printf("%3d%7.3lf",i/4+1,x_ini[i]);
    printf("\n");
    getchar();
  }
/*kew*/

/*||begin||*/
    //ddr_exchange_dofs(Solver_id, level_id, x_ini);
/*||end||*/



/*kbw
  if(level_id>=0){
    printf("x_ini after exchange dofs\n");
    for(i=0;i<ndof;i++) printf("%5d%15.10lf",i/4+1,x_ini[i]);
    printf("\n");
    getchar();
  }
/*kew*/

  }

/* allocate memory for the stiffness matrices and RHS */
  max_nrdof = level_p->max_dof_int_ent;
  stiff_mat = (double *)malloc(max_nrdof*max_nrdof*sizeof(double));
  rhs_vect = (double *)malloc(max_nrdof*sizeof(double));

  /* for each level */
  for(level_id=nr_levels-1;level_id>=0;level_id--){

    level_p = &(siv_solver[Solver_id].level[level_id]);

    lsr_mkb_clear_matrix(Solver_id, level_id, Comp_type);

    pdeg_coarse = SIC_PDEG_FINEST;
    if(level_id<nr_levels-1) pdeg_coarse = level_p->pdeg_coarse;

    /* compute local stiffness matrices */
    for(intent=0;intent<level_p->nr_int_ent;intent++){
    
#ifdef TIME_TEST
      if(intent==0){
	printf("\nbeginning integration of %d elements\n",level_p->nr_dof_ent);
	t_temp=su_getdaytime();
      }
      else if(intent==level_p->nr_dof_bl){
	t_int_el+=su_getdaytime()-t_temp;
	printf("beginning integration of %d faces\n",
	       level_p->nr_int_ent-level_p->nr_dof_ent);
	t_temp=su_getdaytime();
      }
#endif    

      if(Comp_type==SIC_SOLVE) pdr_comp_type = PDC_COMP_BOTH;
      else pdr_comp_type = PDC_COMP_RHS;
    
      nr_dof_ent = SIC_MAX_DOF_PER_INT;
      nrdofs_int_ent = max_nrdof;
      pdr_comp_stiff_mat(solver_p->problem_id, level_p->l_int_ent_type[intent], 
		       level_p->l_int_ent_id[intent], pdr_comp_type, pdeg_coarse,
		       &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdof, 
		       &nrdofs_int_ent, stiff_mat, rhs_vect, &rewrite);
    
      nrdofbl = nr_dof_ent;

#ifdef DEBUG_SIM
      if(nrdofs_int_ent>max_nrdof){
	printf("Too small arrays stiff_mat and rhs_vect passed to comp_el_stiff_mat\n");
	printf("from sir_create in sis_mkb. %d < %d. Exiting !!!",
	       max_nrdof, nrdofs_int_ent);
	exit(-1);
      }
#endif


      for(idofent=0;idofent<nr_dof_ent;idofent++){
	/* only for DG - dof type independent; elements only */
	int dof_struct_id = level_p->l_dof_ent_to_struct[l_dof_ent_id[idofent]];
	/* blocks within solver are offset 1 */
	l_bl_id[idofent] = level_p->l_dof_struct[dof_struct_id].block_id + 1;
	l_bl_nrdof[idofent] = level_p->l_dof_struct[dof_struct_id].nrdof;
      }

/*kbw
#ifdef DEBUG_SIM
    {
      printf("In sir_solve before assemble: Solver_id %d, level_id %d, sol_typ %d\n", Solver_id, level_id, SIC_SOLVE);
      int ibl,jbl,pli,plj,nri,nrj,nrdof,jaux;
      printf("ient %d, int_ent_id %d, nr_dof_bl %d\n", 
	     intent, level_p->l_int_ent_id[intent], nrdofbl);
      pli = 0; nrdof=0;
      for(ibl=0;ibl<nrdofbl; ibl++) nrdof+=l_bl_nrdof[ibl];
      for(ibl=0;ibl<nrdofbl; ibl++){
	printf("bl_id %d, bl_nrdof %d\n",
	  l_bl_id[ibl],l_bl_nrdof[ibl]);
	nri = l_bl_nrdof[ibl];
	plj=0;
	for(jbl=0;jbl<nrdofbl;jbl++){
	  printf("Stiff_mat (blocks %d:%d)\n",jbl,ibl);
	  nrj = l_bl_nrdof[jbl];
	  for(i=0;i<nri;i++){
   	    jaux = plj+(pli+i)*nrdof;
	    for(j=0;j<nrj;j++){
	      printf("%20.15lf",stiff_mat[jaux+j]);
	    }
	    printf("\n");
	  }
	  plj += nrj;
	}
	printf("Rhs_vect:\n");
	for(i=0;i<nri;i++){
	  printf("%20.15lf",rhs_vect[pli+i]);
	}
	printf("\n");
	pli += nri;    
      }
      getchar();
    }
#endif
/*kew*/
    

    lsr_mkb_assemble_local_sm(Solver_id, level_id, SIC_SOLVE,
			      nrdofbl, l_bl_id, l_bl_nrdof, 
			      stiff_mat, rhs_vect, &rewrite);
    
    
   
    } /* end loop over integration entities: ient */

#ifdef TIME_TEST
    t_int_fa+=su_getdaytime()-t_temp;
    printf("beginning factorization of diagonal blocks\n");
    t_temp=su_getdaytime();
#endif

    lsr_mkb_fill_precon(Solver_id, level_id);

#ifdef TIME_TEST
    t_fac_dia+=su_getdaytime()-t_temp;
#endif

  } /* end loop over levels: ilev */

/*kbw
    printf("Initial guess:\n");
    for(i=0;i<it_level->Nrdof_Glob;i++)printf("%20.15lf",x_ini[i]);
    printf("\n");
    getchar();
/*kew*/



#ifdef TIME_TEST
  printf("beginning iterations\n");
   t_temp=su_getdaytime();
#endif

/*------------------------------------------------------------*/
/* SOLVE THE PROBLEM                                          */
/*------------------------------------------------------------*/

  lsr_mkb_solve(Solver_id, nrdof_glob, Ini_guess, x_ini, rhs_vect,
		Nr_iter, Conv_meas, Monitor, Conv_rate);

#ifdef TIME_TEST
  t_iter+=su_getdaytime()-t_temp;
#endif

  /* rewrite the solution */
  level_id = nr_levels-1;
  level_p = &(siv_solver[Solver_id].level[level_id]);
  iaux = 1; /* write to the solution vector no 1 */
  for(ibl=0;ibl<level_p->nr_dof_bl;ibl++){
    int dof_struct_id = level_p->l_bl_to_struct[ibl];
    sit_dof_struct dof_struct = level_p->l_dof_struct[dof_struct_id];
    
    pdr_write_sol_dofs(solver_p->problem_id, 
		       dof_struct.dof_ent_type,
		       dof_struct.dof_ent_id,
		       dof_struct.nrdof,
		       &x_ini[dof_struct.posglob]);


/* print out solution
    printf("Solution in block %d, elem %d\n", ibl, dof_struct.dof_ent_id);
    for (i=0;i<dof_struct.nrdof;i++) 
      printf("%20.10lf",x_ini[dof_struct.posglob+i]);
    printf("\n");
/**/

  }

  free(stiff_mat);
  free(rhs_vect);
  free(x_ini);

/*ok_kbw*/
#ifdef TIME_TEST
  t_total=su_getdaytime()-t_total;
  printf("Total solver times:\n");
  printf("\tintegration of elements     \t%lf\n", t_int_el);
  printf("\tintegration of faces        \t%lf\n", t_int_fa);
  printf("\tfactorization of dia blocks \t%lf\n", t_fac_dia);
  printf("\titerations                  \t%lf\n", t_iter);
  t_temp=t_int_el+t_int_fa+t_fac_dia+t_iter;
  printf("\tsuma                        \t%lf\n", t_temp);
  printf("\ttotal                       \t%lf\n", t_total);
  //printf("%lf  %lf  %lf  %lf  %lf  %lf\n",
  //	 t_int_el,t_int_fa,t_fac_dia,t_iter,t_temp,t_total);
#endif
/*kew*/


  return(1);
}


/*------------------------------------------------------------
  sir_free - to free memory for stiffness and preconditioner matrices
             and make room for next solvers
------------------------------------------------------------*/
int sir_free(/* returns: >=0 - success code, <0 - error code */
  int Solver_id   /* in: solver identification */
  )
{

  int nr_levels, ilev;
  sit_levels *level_p;       /* mesh levels */

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* free solver data structures */
  lsr_mkb_free_matrix(Solver_id);

  siv_cur_solver_id=Solver_id;
  nr_levels = siv_solver[Solver_id].nr_levels;

  /* in a big loop over mesh levels free renumbering data structurev*/
  for(ilev=0;ilev<nr_levels;ilev++){

    level_p = &siv_solver[Solver_id].level[ilev];

    free(level_p->l_int_ent_type);
    free(level_p->l_int_ent_id);
    free(level_p->l_dof_ent_to_struct);
    free(level_p->l_dof_struct);
    free(level_p->l_bl_to_struct);

  }

  return(0);

}

/*------------------------------------------------------------
  sir_destroy - to make room for next solvers - in LIFO manner !!!!!
------------------------------------------------------------*/
int sir_destroy(/* returns: >=0 - success code, <0 - error code */
  int Solver_id   /* in: solver identification */
  )
{

  /* destroy the solver instance */
  lsr_mkb_destroy(Solver_id);

  /* decrease the counter for solvers */
  siv_nr_solvers--;

  /* set the current solver ID */
  if(siv_cur_solver_id > siv_nr_solvers) siv_cur_solver_id = siv_nr_solvers;

  return(1);
}


/*---------------------------------------------------------
sir_put_list - to put Num on the list List with length Ll 
	(filled with numbers and SIC_LIST_END_MARK at the end)
---------------------------------------------------------*/
int sir_put_list( /* returns*/
		/*  >0 - position already occupied on the list */
             	/*  <0 - position at which put on the list */
            	/*   0 - list full, not found on the list */
	int Num, 	/* in: number to put on the list */
	int* List, 	/* in: list */
	int Ll		/* in: total list's lengths */
	)
{

  int i, il;
  
  for(i=0;i<Ll;i++){
    if((il=List[i])==SIC_LIST_END_MARK) break;
    /* found on the list on (i+1) position */
    if(Num==il) return(i+1);
  }
  /* if list is full return error message */
  if(i==Ll) return(0);
  /* update the list and return*/
  List[i]=Num;
  return(-(i+1));
}

#ifdef TIME_TEST

/* PROCEDURES AND VARIABLES THAT ARE SYSTEM DEPENDENT */
#include<time.h>
#include<sys/time.h>
#include<sys/resource.h>

/*---------------------------------------------------------
su_getdaytime - to return number of wall clock seconds from 
	time measurement initialization
---------------------------------------------------------*/
double su_getdaytime()
{ 

  double daytime;
  struct timeval tk;
  struct timezone tzp;

  gettimeofday(&tk, &tzp);

  daytime=(tk.tv_usec)/1e6+tk.tv_sec;

return(daytime);
}

/*---------------------------------------------------------
su_getcputime - to return number of cpu seconds from 
	time measurement initialization
---------------------------------------------------------*/
double su_getcputime()
{ 

  double cputime;
  struct rusage rk;

  getrusage(RUSAGE_SELF, &rk);

  cputime = (rk.ru_utime.tv_usec)/1e6;
  cputime += rk.ru_utime.tv_sec;

return(cputime);
}


#endif
