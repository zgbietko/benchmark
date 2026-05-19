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

/* provided interface of the multigrid_krylow_bliter module */
#include "../lsd_mkb/lsh_mkb_intf.h"

#define TIME_TEST

/* internal information for the krylow bliter solver interface module */
#include "sih_amg.h"

/*** CONSTANTS ***/
/* Solver (preconditioner) types */
const int SIC_SINGLE_LEVEL = 1;
const int SIC_TWO_LEVEL    = 2;
const int SIC_MULTI_LEVEL  = 0;


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
int sir_sort_short(
		   int* A, // array
		   int p,  //first index
		   int k   // last index
		   );

/*------------------------------------------------------------
  sir_module_introduce - to return the solver name
------------------------------------------------------------*/
int sir_module_introduce( /* returns: >=0 - success code, <0 - error code */
  char* Solver_name  /* out: the name of the solver */
  )
{
  char* string = "KRYLOW_BLITER";

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
  char* Filename,  /* in: name of the file with control parameters */
  int Max_iter, /* maximal number of iterations, -1 for values from Filename */
  int Error_type, /* type of error norm (stopping criterion), -1 for Filename*/
  double Error_tolerance, /* value for stopping criterion, -1.0 for Filename */ 
  int Monitoring_level /* Level of output, -1 for Filename */
  )
{


  int comp_type, solver_id, monitor, ini_guess;
  char solver_name[100];
  double conv_rate;

/*++++++++++++++++ executable statements ++++++++++++++++*/


  /*kbw
  printf("In sir_solve_lin_sys before init: problem_id %d\n",
	 problem_id);
  /*kew*/

  /* initialize the solver */
  solver_id = sir_init(Parallel, Filename, Max_iter, Error_type, Error_tolerance,
		       Monitoring_level);

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
  //monitor = SIC_PRINT_INFO;
  //monitor = SIC_PRINT_NOT;
  ini_guess = 0; // do not get initial guess
  sir_solve(solver_id, comp_type, ini_guess, Monitoring_level,
	    &Max_iter, &Error_tolerance, &conv_rate);

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
  char* Filename,  /* in: name of the file with control parameters */
  int Max_iter, /* maximal number of iterations, -1 for values from Filename */
  int Error_type, /* type of error norm (stopping criterion), -1 for Filename*/
  double Error_tolerance, /* value for stopping criterion, -1.0 for Filename */ 
  int Monitoring_level /* Level of output, -1 for Filename */
  )
{

/*++++++++++++++++ executable statements ++++++++++++++++*/

  int info;

  /* check constants compatibility */
  if(SIC_SEQUENTIAL != LSC_SEQUENTIAL || SIC_PARALLEL != LSC_PARALLEL){
    printf("Wrong constants in KRB\n"); exit(0);
  }    
  if(SIC_SOLVE != LSC_MKB_SOLVE || SIC_RESOLVE != LSC_MKB_RESOLVE){
    printf("Wrong constatnts in KRB\n"); exit(0);
  }    

  /* increase the counter for solvers */
  siv_nr_solvers++;

  /* set the current solver ID */
  siv_cur_solver_id = siv_nr_solvers;
  siv_solver[siv_cur_solver_id].parallel = Parallel;
  siv_solver[siv_cur_solver_id].nr_levels = SIC_SINGLE_LEVEL;

  // SIC_SINGLE_LEVEL == 1
  info = lsr_mkb_init(Parallel, &siv_solver[siv_cur_solver_id].nr_levels, 
		      Filename, Max_iter, Error_type, Error_tolerance,
		      Monitoring_level);

#ifdef DEBUG_SIM
  if(info!=siv_cur_solver_id){
    printf("Error 213758 in sir_init!!! Exiting.\n");
    exit(-1);
  }
#endif

  return(info);
}

/*------------------------------------------------------------
  sir_reverse_cuthill_mckee - bandwidth reduction algorithm (renumbering)
------------------------------------------------------------*/
void sir_reverse_cuthill_mckee(sit_dof_struct * l_dof_struct, int nr_dof_ent){


  list_row *head=NULL, *tail=NULL, *lhead=NULL, *ltail=NULL, *elem=NULL, *lelem=NULL;
  int sum_nreig, icrs=0, jcrs=0;

  /* array of visited DOF - 1 visited, 0 not visited */
  int *vt=NULL, *permutation=NULL, *rpermutation=NULL;

  permutation =(int *)malloc( nr_dof_ent*sizeof(int) );
  rpermutation =(int *)malloc( nr_dof_ent*sizeof(int) );
  if(permutation==NULL){printf("no memory in sir_reverse_cuthill_mckee permutation"); exit(0);}
  if(rpermutation==NULL){printf("no memory in sir_reverse_cuthill_mckee rpermutation"); exit(0);}

  vt =(int *)malloc( nr_dof_ent*sizeof(int));
  if(vt==NULL){printf("no memory in sir_reverse_cuthill_mckee vt"); exit(0);}


  sit_dof_struct *dof_struct_p;
  int ibl,ineig,i,start_dof=0,iper;
  int iaux;
  
  sum_nreig=l_dof_struct[start_dof].nrneig;
  iper=nr_dof_ent-1;
  for(ibl = 0; ibl< nr_dof_ent; ibl++){vt[ibl]=0;}
  
  printf("nr_dof_ent=%d",nr_dof_ent);

  for(ibl = 1; ibl< nr_dof_ent; ibl++){ 
    sum_nreig+=l_dof_struct[ibl].nrneig;
    if(l_dof_struct[ibl].nrneig <= l_dof_struct[start_dof].nrneig) {start_dof=ibl;}
  }
  
  for(ibl = 0; ibl< nr_dof_ent; ibl++)rpermutation[ibl]=-1;
  
  
  // printf("start_dof %d, min %d\n", start_dof, l_dof_struct[start_dof].nrneig);


  /* start compute permutation vector */
  vt[start_dof]=1;
  permutation[iper]=start_dof;
  rpermutation[permutation[iper]]=iper;
  iper=iper-1;
  while(iper>=0){
    for(ineig=0;ineig<l_dof_struct[start_dof].nrneig;ineig++){
      iaux=l_dof_struct[start_dof].l_neig[ineig];


      if(vt[iaux]==0){
	vt[iaux]=1;
	elem =(list_row*)malloc(sizeof(list_row));
	elem->row=iaux;
	elem->next=NULL;
	if(lhead==NULL){
	  lhead = elem;
	  ltail = lhead;
	}
	else{
	  if(l_dof_struct[iaux].nrneig < l_dof_struct[lhead->row].nrneig){
	    elem->next = lhead;
	    lhead = elem;
	  }else{
	    lelem = lhead;
	    while((lelem->next != NULL) &&
		  (l_dof_struct[iaux].nrneig >= l_dof_struct[lelem->row].nrneig)){
	      lelem = lelem->next;
	    }
	    elem->next = lelem->next;
	    lelem->next = elem;
	    if(lelem == ltail)ltail=elem;
	  }
	}
      }

      //printf("%6d",l_dof_struct[iaux].dof_ent_id);
    }

    if(head == NULL) { head=lhead; tail=ltail; lhead=NULL; ltail=NULL; }
    else if(lhead!=NULL){ tail->next = lhead; tail=ltail; lhead=NULL; ltail=NULL;}

    start_dof=head->row;
    elem=head;
    head=head->next;
    free(elem);
    permutation[iper]=start_dof;
    rpermutation[permutation[iper]]=iper;
    iper=iper-1;


    //printf("ddddd %d\n",start_dof);
    //getchar();
  }



  /*
    printf("permutacja:%d \n",nr_dof_ent);
    for(ibl = 0; ibl< nr_dof_ent; ibl++)printf("%d ",permutation[ibl]);
    printf("\nrpermutacja:\n");
    for(ibl = 0; ibl< nr_dof_ent; ibl++)printf("%d ",rpermutation[ibl]);
    /**/

  // setting block_id, pos_glob, l_neig_bl

  start_dof=0;
  for(ibl = 0; ibl< nr_dof_ent; ibl++){
    l_dof_struct[rpermutation[ibl]].block_id = ibl;
    l_dof_struct[rpermutation[ibl]].posglob=start_dof;
    start_dof+=l_dof_struct[rpermutation[ibl]].nrdofs;

  }


  free(vt);
  free(permutation);
  free(rpermutation);

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
  int nr_int_ent, nr_dof_ent, max_dofs_per_dof_ent, max_dofs_int_ent, max_dof_ent_id;
  /* the global number of degrees of freedom */
  int nrdofs_glob, int_ent_id, nrdofs_ent_loc, dof_ent_id, pos_glob;
  int nr_dof_struct, dof_struct_id, nr_levels, nr_dof_ent_loc;
  int* temp_list_dof_type;
  int* temp_list_dof_id;
  int* temp_list_dof_nrdofs;
  int l_dof_ent_types[SIC_MAX_DOF_PER_INT], l_dof_ent_ids[SIC_MAX_DOF_PER_INT];
  int l_dof_ent_nrdofs[SIC_MAX_DOF_PER_INT];
  int *l_bl_nrdofs, *l_bl_posglob, *l_bl_nrneig, **l_bl_l_neig;
  int *l_bl_dof_ent_type, *l_bl_dof_ent_id;

  /* auxiliary variables */
  int i,j,k, idof, index, iaux, ient, ineig, level_id, ibl;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  siv_cur_solver_id=Solver_id;
  siv_solver[Solver_id].problem_id = Problem_id;
  siv_solver[Solver_id].cur_level = 0;
  level_p = &(siv_solver[Solver_id].level[0]);
  level_id = 0;

  /*kbw
  printf("In sir_create: solver_id %d, Problem_id %d\n",
	 Solver_id, Problem_id);
  /*kew*/

  /* get lists of integration and dof entities (types and IDs and nrdofs) */
  /* the order on the lists determine the order of DOF structures !!! */
  pdr_get_list_ent(Problem_id, &nr_int_ent, 
		       &level_p->l_int_ent_type, &level_p->l_int_ent_id, 
                       &nr_dof_ent, &temp_list_dof_type, 
		       &temp_list_dof_id, &temp_list_dof_nrdofs, 
		       &nrdofs_glob, &max_dofs_per_dof_ent);


/*kbw
    printf("In sir_create after pdr_get_list_ent\n");
    printf("nr_int_ent %d\n", nr_int_ent);
    for(i=0;i<nr_int_ent;i++)  printf("type %d, id %d\n",
	    level_p->l_int_ent_type[i],level_p->l_int_ent_id[i]);
    printf("\nNr_dof_ent %d, Nrdofs_glob %d, max_dofs_per_dof_ent %d\n",
	   nr_dof_ent, nrdofs_glob, max_dofs_per_dof_ent); 
    for(i=0;i<nr_dof_ent;i++)  printf("type %d, id %d, nrdof %d\n",
	    temp_list_dof_type[i], temp_list_dof_id[i],
				      temp_list_dof_nrdofs[i]);
/*kew*/

  level_p->nr_int_ent = nr_int_ent;
  level_p->nr_dof_ent = nr_dof_ent;
  level_p->nrdofs_glob = nrdofs_glob;

  /* array of structures storing DOF data */
  level_p->l_dof_struct = 
    (sit_dof_struct *)malloc( nr_dof_ent*sizeof(sit_dof_struct) );

  /* dof_ent_index to dof_struct_index (based on which dof_ent_id and */
  /* dof_ent_type can be find */
  level_p->max_dof_vert_id = -1;
  level_p->max_dof_edge_id = -1;
  level_p->max_dof_face_id = -1;
  level_p->max_dof_elem_id = -1;
  for(i=0; i< nr_dof_ent; i++){

    if(abs(temp_list_dof_type[i]) == PDC_ELEMENT ){
      if ( temp_list_dof_id[i] > level_p->max_dof_elem_id )
	                        level_p->max_dof_elem_id = temp_list_dof_id[i];
    } else if(abs(temp_list_dof_type[i]) == PDC_FACE ){
      if ( temp_list_dof_id[i] > level_p->max_dof_face_id )
	                        level_p->max_dof_face_id = temp_list_dof_id[i];
    } else if(abs(temp_list_dof_type[i]) == PDC_EDGE ){
      if ( temp_list_dof_id[i] > level_p->max_dof_edge_id )
	                        level_p->max_dof_edge_id = temp_list_dof_id[i];
    } else if(abs(temp_list_dof_type[i]) == PDC_VERTEX ){
      if ( temp_list_dof_id[i] > level_p->max_dof_vert_id )
	                        level_p->max_dof_vert_id = temp_list_dof_id[i];
    } else {
      printf("Error 87373732 in sis_krylow_bliter_intf/sir_create!!! Exiting\n");
      exit(-1);
    }


  }

  if(level_p->max_dof_elem_id>=0){
    level_p->l_dof_elem_to_struct=
      (int*)malloc((level_p->max_dof_elem_id+1)*sizeof(int)); 
    for(i=0;i<=level_p->max_dof_elem_id;i++) 
      level_p->l_dof_elem_to_struct[i] = -1;
  }
  if(level_p->max_dof_face_id>=0){
    level_p->l_dof_face_to_struct=
      (int*)malloc((level_p->max_dof_face_id+1)*sizeof(int)); 
    for(i=0;i<=level_p->max_dof_face_id;i++) 
      level_p->l_dof_face_to_struct[i] = -1;
  }
  if(level_p->max_dof_edge_id>=0){
    level_p->l_dof_edge_to_struct=
      (int*)malloc((level_p->max_dof_edge_id+1)*sizeof(int)); 
    for(i=0;i<=level_p->max_dof_edge_id;i++) 
      level_p->l_dof_edge_to_struct[i] = -1;
  }
  if(level_p->max_dof_vert_id>=0){
    level_p->l_dof_vert_to_struct=
      (int*)malloc((level_p->max_dof_vert_id+1)*sizeof(int)); 
    for(i=0;i<=level_p->max_dof_vert_id;i++) 
      level_p->l_dof_vert_to_struct[i] = -1;
  }

    // PLACE FOR  CALL TO RENUMBERING PROCEDURE

//!!! If renumbering not used, the order in pdr_ and sir_ routines the same !!! 

//int tabpos[8] = {7, 3, 6, 4, 2, 0, 5, 1};

  pos_glob = 0; 
  for(index = 0; index < nr_dof_ent; index++){

    /* in the case of no renumbering index on temp_list_dof_... arrays */
    /* is the same as index=ID of DOF structure in the l_dof_struct array */
    //idof = tabpos[index];
	idof = index;
    /* otherwise idof should be given by some output array */
    /* of the renumbering procedure idof = renumber_array[index]; */

    dof_struct_p = &level_p->l_dof_struct[idof];


    nrdofs_ent_loc = temp_list_dof_nrdofs[index];
    if(nrdofs_ent_loc > level_p->max_dofs_dof_ent) 
      level_p->max_dofs_dof_ent = nrdofs_ent_loc;
    if(temp_list_dof_type[index]<0){
      dof_struct_p->nrneig = -1;
      temp_list_dof_type[index] = abs(temp_list_dof_type[index]);
    } else {
      dof_struct_p->nrneig = 0;
    }
    dof_struct_p->dof_ent_type = temp_list_dof_type[index];
    dof_struct_p->dof_ent_id = temp_list_dof_id[index];
    dof_struct_p->nrdofs = nrdofs_ent_loc;
    dof_struct_p->posglob = pos_glob;
    pos_glob += nrdofs_ent_loc;

    /* initialize lists of integration entities and neighbouring dof_ent */
    dof_struct_p->nr_int_ent = 0;
    for(i=0;i<SIC_MAX_INT_PER_DOF;i++) 
      dof_struct_p->l_int_ent_index[i]=SIC_LIST_END_MARK;
    for(i=0;i<SIC_MAX_DOF_STR_NGB;i++) 
      dof_struct_p->l_neig[i]=SIC_LIST_END_MARK;

    /* arrays dof_ent to dof_struct */
    if(dof_struct_p->dof_ent_type == PDC_ELEMENT ) {
      level_p->l_dof_elem_to_struct[dof_struct_p->dof_ent_id] = idof;

#ifdef DEBUG_SIM
      if(dof_struct_p->dof_ent_id > level_p->max_dof_elem_id){
      printf("Error 84543732 in sis_krylow_bliter_intf/sir_create!!! Exiting\n");
      printf("%d > %d\n", dof_struct_p->dof_ent_id, level_p->max_dof_elem_id);
      exit(-1);
    }
#endif

    } else if(dof_struct_p->dof_ent_type == PDC_FACE ) {
      level_p->l_dof_face_to_struct[dof_struct_p->dof_ent_id] = idof;

#ifdef DEBUG_SIM
    if(dof_struct_p->dof_ent_id > level_p->max_dof_face_id){
      printf("Error 84543732 in sis_krylow_bliter_intf/sir_create!!! Exiting\n");
      printf("%d > %d\n", dof_struct_p->dof_ent_id, level_p->max_dof_face_id);
      exit(-1);
    }
#endif

    } else if(dof_struct_p->dof_ent_type == PDC_EDGE ) {
      level_p->l_dof_edge_to_struct[dof_struct_p->dof_ent_id] = idof;

#ifdef DEBUG_SIM
    if(dof_struct_p->dof_ent_id > level_p->max_dof_edge_id){
      printf("Error 84543732 in sis_krylov_bliter_intf/sir_create!!! Exiting\n");
      printf("%d > %d\n", dof_struct_p->dof_ent_id, level_p->max_dof_edge_id);
      exit(-1);
    }
#endif

    } else if(dof_struct_p->dof_ent_type == PDC_VERTEX ) {
      level_p->l_dof_vert_to_struct[dof_struct_p->dof_ent_id] = idof;
#ifdef DEBUG_SIM
    if(dof_struct_p->dof_ent_id > level_p->max_dof_vert_id){
      printf("Error 84543732 in sis_krylow_bliter_intf/sir_create!!! Exiting\n");
      printf("%d > %d\n", dof_struct_p->dof_ent_id, level_p->max_dof_vert_id);
      exit(-1);
    }
#endif
    } else {
      printf("Error 8963732 in sis_krylow_bliter_intf/sir_create!!! Exiting\n");
      exit(-1);
    }


/*kbw
    printf("In sir_create after filling dof_struct no %d\n", idof);
    printf("dof_ent_type %d, dof_ent_id %d, nrdof %d, posglob %d\n",
	   dof_struct_p->dof_ent_type , dof_struct_p->dof_ent_id, 
	   dof_struct_p->nrdofs, dof_struct_p->posglob);
    printf("Initialized lists of int_ent %d, neig %d\n",
	   dof_struct_p->l_int_ent_index[0], dof_struct_p->l_neig[0]); 
/*kew*/

  }

#ifdef DEBUG_SIM
  if ( level_p->nrdofs_glob != pos_glob ){
    printf("Error 843732 in sis_krylow_bliter_intf/sir_create!!! Exiting\n");
    exit(-1);
  }
#endif

  free(temp_list_dof_type);
  free(temp_list_dof_id);
  free(temp_list_dof_nrdofs);

  /* getting information on the structure of the global stiffness matrix */
  nr_dof_struct = 0;
  max_dofs_int_ent = 0;
  for(ient=0; ient<level_p->nr_int_ent;ient++){

    int idofent;

    int nrdofs_int_ent = 0;
    int_ent_id = level_p->l_int_ent_id[ient];
    nr_dof_ent_loc = SIC_MAX_DOF_PER_INT;

    pdr_comp_stiff_mat(Problem_id, level_p->l_int_ent_type[ient], 
		       level_p->l_int_ent_id[ient], PDC_NO_COMP, NULL,
		       &nr_dof_ent_loc, l_dof_ent_types, 
		       l_dof_ent_ids, l_dof_ent_nrdofs,
		       NULL, NULL, NULL, NULL);


/*kbw
   printf("in sir_create after pdr_comp_stiff_mat for int_ent no %d (id %d):\n", 
	   ient, int_ent_id);
    printf("nr_dof_ent_loc %d, types, ids, nrdofs:\n", nr_dof_ent_loc);
    for(idofent=0; idofent<nr_dof_ent_loc; idofent++){
      printf("%d %d %d\n", l_dof_ent_types[idofent], 
	     l_dof_ent_ids[idofent], l_dof_ent_nrdofs[idofent]);
    }
/*kew*/

    for(idofent=0; idofent<nr_dof_ent_loc; idofent++){

      int dof_ent_id = l_dof_ent_ids[idofent];
      int dof_ent_type = l_dof_ent_types[idofent];

      if(dof_ent_type == PDC_ELEMENT ) {
	
	dof_struct_id = level_p->l_dof_elem_to_struct[dof_ent_id];
	
#ifdef DEBUG_SIM
	if( level_p->l_dof_elem_to_struct[dof_ent_id] == -1){
	  printf("Error 3472941 in sir_create!!! Exiting\n");
	  exit(-1);
	}
#endif
	
      } else if(dof_ent_type == PDC_FACE ) {
	
	dof_struct_id = level_p->l_dof_face_to_struct[dof_ent_id];
	
#ifdef DEBUG_SIM
	if( level_p->l_dof_face_to_struct[dof_ent_id] == -1){
	  printf("Error 3472942 in sir_create!!! Exiting\n");
	  exit(-1);
	}
#endif
	
      } else if(dof_ent_type == PDC_EDGE ) {
	
	dof_struct_id = level_p->l_dof_edge_to_struct[dof_ent_id];
	
#ifdef DEBUG_SIM
	if( level_p->l_dof_edge_to_struct[dof_ent_id] == -1){
	  printf("Error 3472943 in sir_create!!! Exiting\n");
	  exit(-1);
	}
#endif
	
      } else if(dof_ent_type == PDC_VERTEX ) {
	
	dof_struct_id = level_p->l_dof_vert_to_struct[dof_ent_id];
	
#ifdef DEBUG_SIM
	if( level_p->l_dof_vert_to_struct[dof_ent_id] == -1){
	  printf("Error 3472944 for %d in sir_create!!! Exiting\n",
		 dof_ent_id);
	  exit(-1);
	}
#endif
	
      } else {
	printf("Error 34fsf7294 in sir_create!!! Exiting\n");
	exit(-1);
      }

      dof_struct_p = &level_p->l_dof_struct[dof_struct_id];

/*kbw
	printf("for int_type %d, int_id %d, dof_type %d, dof_id %d, struct %d\n",
	       level_p->l_int_ent_type[ient], level_p->l_int_ent_id[ient],
	       l_dof_ent_types[idofent], l_dof_ent_ids[idofent],dof_struct_id );
/*kew*/

#ifdef DEBUG_SIM
      if((dof_struct_p->dof_ent_id != dof_ent_id) || 
	 (dof_struct_p->dof_ent_type != dof_ent_type) || 
	 (dof_struct_p->nrdofs != l_dof_ent_nrdofs[idofent]) ){
	printf("Error 3827 in sir_create!!! Exiting");
	exit(-1);
      }
#endif      

      nrdofs_int_ent += l_dof_ent_nrdofs[idofent];

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

/*kbw
      if(dof_struct_p->nr_int_ent>77){
	printf("For dof_ent %d, list of int_ent:\n", dof_ent_id);
	for(i=0;i<dof_struct_p->nr_int_ent;i++){
	  printf("%d  ", dof_struct_p->l_int_ent_index[i]);
	}
	printf("\n");
      }
/*kew*/

#ifdef DEBUG_SIM
	  if(iaux == 0){ // list full - increase SIC_MAX_INT_PER_DOF
	    printf("Error 383627 (nr_int_ent %d) in sir_create!!! Exiting", 
		   dof_struct_p->nr_int_ent);
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

	//	if(ineig != idofent){
	/* change for constrained approximation */
	/* ineig may be != idofent but this is the same dof_ent */
	if(dof_ent_type != l_dof_ent_types[ineig] ||
	   dof_ent_id != l_dof_ent_ids[ineig]){	

	  int neig_id = l_dof_ent_ids[ineig];
	  int neig_type = l_dof_ent_types[ineig];

/*kbw
      printf("dof_ent %d: putting ineig no %d (id %d) on the list of neig, nrneig %d\n", 
	     idofent, ineig, neig_id, dof_struct_p->nrneig);
      printf("before:");
      for(i=0;i<SIC_MAX_DOF_STR_NGB;i++) {
	printf("%d",dof_struct_p->l_neig[i]) ;
      }
      printf("\n");
/*kew*/

	  int neig_index = 0;
	  if(neig_type==PDC_ELEMENT){
	    neig_index = level_p->l_dof_elem_to_struct[neig_id];
	  } else if(neig_type==PDC_FACE){
	    neig_index = level_p->l_dof_face_to_struct[neig_id];
	  } else if(neig_type==PDC_EDGE){
	    neig_index = level_p->l_dof_edge_to_struct[neig_id];
	  } else if(neig_type==PDC_VERTEX){
	    neig_index = level_p->l_dof_vert_to_struct[neig_id];
	  } 

/*kbw
      printf("dof_ent %d: putting ineig no %d (id %d, index %d) on the list of neig, nrneig %d\n", 
	     idofent, ineig, neig_id, neig_index, dof_struct_p->nrneig);
      printf("before:");
      for(i=0;i<SIC_MAX_DOF_STR_NGB;i++) {
	printf("%d",dof_struct_p->l_neig[i]) ;
      }
      printf("\n");
/*kew*/

	/* only active elements=DOF entities store neighbors */
	  if(dof_struct_p->nrneig>=0){

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

      }
    
    } /* end loop over dof_ents of a given int_ent (idofent) */

    if(nrdofs_int_ent > max_dofs_int_ent) max_dofs_int_ent = nrdofs_int_ent; 

  } /* end loop over int_ent */

  
  //call renumbering
  sir_reverse_cuthill_mckee(level_p->l_dof_struct, level_p->nr_dof_ent);
  
  int istruct;
  // use renumbering data to prepare data for assembling
  for(istruct = 0; istruct< nr_dof_ent; istruct++){

    //* set the list of neighboring blocks */
    for(ineig=0;ineig<level_p->l_dof_struct[istruct].nrneig;ineig++){
      iaux=level_p->l_dof_struct[istruct].l_neig[ineig];
      level_p->l_dof_struct[istruct].l_neig_bl[ineig]=level_p->l_dof_struct[iaux].block_id;
    }

/*kbw
    printf("dof_struct %d, dof_ent_type %d, dof_ent_id %d, block_id %d, posglob %d, nrneig %d\n",
	   istruct, l_dof_struct[istruct].dof_ent_type , l_dof_struct[istruct].dof_ent_id ,
	   l_dof_struct[istruct].block_id , l_dof_struct[istruct].posglob ,
	   l_dof_struct[istruct].nrneig);
    printf("neighbors before sorting: \n");
    for(ineig=0;ineig<l_dof_struct[istruct].nrneig;ineig++){
      printf("neig_id %d, struct_id %d, block_id %d\n",
	     ineig, l_dof_struct[istruct].l_neig[ineig],
	     l_dof_struct[istruct].l_neig_bl[ineig]);
    }
/*kew*/

    sir_sort_short(level_p->l_dof_struct[istruct].l_neig_bl, 0,
		   level_p->l_dof_struct[istruct].nrneig-1);

/*kbw
    printf("neighbors after sorting: \n");
    for(ineig=0;ineig<l_dof_struct[istruct].nrneig;ineig++){
      printf("neig_id %d, struct_id %d, block_id %d\n",
	     ineig, l_dof_struct[istruct].l_neig[ineig],
	     l_dof_struct[istruct].l_neig_bl[ineig]);
    }
/*kew*/

  }

   
  level_p->max_dofs_int_ent = max_dofs_int_ent;


  //level_p = &(siv_solver[siv_cur_solver_id].level[0]);
  nr_levels = siv_solver[siv_cur_solver_id].nr_levels;

  /* allocate memory for temporary lists */
  if(siv_solver[Solver_id].parallel){
    l_bl_dof_ent_id =  (int *)malloc((level_p->nr_dof_ent+1)*sizeof(int));
    l_bl_dof_ent_type =  (int *)malloc((level_p->nr_dof_ent+1)*sizeof(int));
  }
  l_bl_nrdofs =  (int *)malloc((level_p->nr_dof_ent+1)*sizeof(int));
  l_bl_posglob = (int *)malloc((level_p->nr_dof_ent+1)*sizeof(int));
  l_bl_nrneig = (int *)malloc((level_p->nr_dof_ent+1)*sizeof(int));
  l_bl_l_neig = (int **)malloc((level_p->nr_dof_ent+1)*sizeof(int *));

/*kbw
  printf("filling temp lists for mkb_create_matrix: solver %d, level %d (%lu)\n",
	 Solver_id, level_id, level_p);
  printf("nrblocks %d, max_sm_size %d, nrdofs_glob %d\n",
	 level_p->nr_dof_ent, max_dofs_int_ent, level_p->nrdofs_glob);
  printf("\tnrdofbl,\tposglob,\tnroffbl\n");
/*kew*/

  /* !!! offset 1 numbering of blocks in it_bliter !!! */
  for(istruct = 1; istruct<= level_p->nr_dof_ent; istruct++){

      dof_struct_id = istruct-1;

      dof_struct_p = &level_p->l_dof_struct[dof_struct_id];

    /* renumbering !*/
    /* !!! offset 1 numbering of blocks in it_bliter !!! */
      ibl = dof_struct_p->block_id +1;

/*kbw
      printf("dof_struct_p %lu, id %d, dof_ent %d\n", dof_struct_p,
	     dof_struct_id, dof_struct_p->dof_ent_id);
      printf("nrdofs %d, posglob %d, nrneig %d\nneig: ",
	     dof_struct_p->nrdofs, dof_struct_p->posglob, dof_struct_p->nrneig);
      for(ineig=0;ineig<dof_struct_p->nrneig;ineig++){
	int iaux=dof_struct_p->l_neig[ineig];
	printf("%6d",level_p->l_dof_struct[iaux].dof_ent_id);
      }
      printf("\nint_ent: ");
	for(i=0;i<dof_struct_p->nr_int_ent;i++){
	  printf("%d  ", dof_struct_p->l_int_ent_index[i]);
	}
	printf("\n");
	getchar();
/*kew*/

      l_bl_nrdofs[ibl] = dof_struct_p->nrdofs;
      l_bl_posglob[ibl] = dof_struct_p->posglob;
      l_bl_nrneig[ibl] = dof_struct_p->nrneig;
      l_bl_l_neig[ibl]= (int *)malloc(l_bl_nrneig[ibl]*sizeof(int));
      for(ineig=0;ineig<l_bl_nrneig[ibl];ineig++){
	// int iaux=dof_struct_p->l_neig[ineig]; - old without renumbering
	int iaux=dof_struct_p->l_neig_bl[ineig];
	/* !!! offset 1 numbering of blocks in it_bliter !!! */
	l_bl_l_neig[ibl][ineig]=iaux+1;
      }
  }
  

  
/*kbw
  printf("before calling mkb_create_matrix: solver %d, level %d\n",
	 Solver_id, level_id);
  printf("nrblocks %d, max_sm_size %d, nrdofs_glob %d\n",
	 level_p->nr_dof_ent, max_dofs_int_ent, level_p->nrdofs_glob);
  printf("\tnrdofbl,\tposglob,\tnroffbl\n");
  for(ibl = 1; ibl<= level_p->nr_dof_ent; ibl++){
    printf("\t%d\t\t%d\t\t%d\n",l_bl_nrdofs[ibl], l_bl_posglob[ibl], 
	   l_bl_nrneig[ibl]);
    printf("\n");
    for(j=0;j<l_bl_nrneig[ibl]; j++){
      printf("%10d",l_bl_l_neig[ibl][j]);
    }
    printf("\n");
  }
/*kew*/

  lsr_mkb_create_matrix(Solver_id, level_id, level_p->nr_dof_ent, 
			level_p->nrdofs_glob,
			max_dofs_int_ent, l_bl_nrdofs, l_bl_posglob, 
			l_bl_nrneig, l_bl_l_neig);

 
  /* create tables for exchanging data between processors */ 
  if(siv_solver[Solver_id].parallel){

    // TABLES FOR BLOCKS ARE USED BUT THE CONTENT IS FOR DOF STRUCTURES

  /* !!! offset 0 numbering of DOF structs  !!! */
    for(dof_struct_id = 0; dof_struct_id< level_p->nr_dof_ent; dof_struct_id++){

      dof_struct_p = &level_p->l_dof_struct[dof_struct_id];

/*kbw
      printf("dof_struct_p %lu, id %d, dof_ent %d\n", dof_struct_p,
	     dof_struct_id, dof_struct_p->dof_ent_id);
      printf("nrdofs %d, posglob %d, nrneig %d\nneig: ",
	     dof_struct_p->nrdofs, dof_struct_p->posglob, dof_struct_p->nrneig);
      for(ineig=0;ineig<dof_struct_p->nrneig;ineig++){
	int iaux=dof_struct_p->l_neig[ineig];
	printf("%6d",level_p->l_dof_struct[iaux].dof_ent_id);
      }
      printf("\nint_ent: ");
	for(i=0;i<dof_struct_p->nr_int_ent;i++){
	  printf("%d  ", dof_struct_p->l_int_ent_index[i]);
	}
	printf("\n");
	getchar();
/*kew*/

      l_bl_nrdofs[dof_struct_id] = dof_struct_p->nrdofs;
      l_bl_posglob[dof_struct_id] = dof_struct_p->posglob;
      l_bl_dof_ent_id[dof_struct_id] = dof_struct_p->dof_ent_id;
      l_bl_dof_ent_type[dof_struct_id] = dof_struct_p->dof_ent_type;

    }

/*kbw
    printf("solver: sending to exchange_tables - nrblocks %d\n",
	   level_p->nr_dof_ent);
    //for(ibl=1;ibl<=it_level->Nrblocks;ibl++){
    for(ibl=1;ibl<=10;ibl++){
      printf("ibl %d, nrdof %d, posglob %d\n",
	     ibl, l_bl_nrdofs[ibl], l_bl_posglob[ibl]);
    }
/*kew*/

    pdr_create_exchange_tables(Problem_id, Solver_id, level_id, 
			       level_p->nr_dof_ent, 
			       l_bl_dof_ent_type, l_bl_dof_ent_id,
			       l_bl_nrdofs, l_bl_posglob, 
			       level_p->l_dof_elem_to_struct,
			       level_p->l_dof_face_to_struct,
			       level_p->l_dof_edge_to_struct,
			       level_p->l_dof_vert_to_struct
			       );
  }


  for(ibl=1;ibl<=level_p->nr_dof_ent;ibl++){
    free(l_bl_l_neig[ibl]);
  }
  if(siv_solver[Solver_id].parallel){
    free(l_bl_dof_ent_id);
    free(l_bl_dof_ent_type);
  }
  free(l_bl_nrdofs);
  free(l_bl_posglob);
  free(l_bl_nrneig);
  free(l_bl_l_neig);

  /* create preconditioner data structure */
  lsr_mkb_create_precon(Solver_id, level_id);

/*kbw

    sit_blocks *block;	
    int iblock,iaux;

    level_p = &siv_solver[siv_cur_solver_id].level[level_id];
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
  int Ini_guess,     /* in: indicator on whether to set initial guess (>0), */
                     /*     or to initialize it to zero (0) */
         /* if >0 then it indicates from which solution vector to take data */
  int Monitor,       /* in: monitoring flag with options (-1 for defaults): */
                     /*   SIC_PRINT_NOT - do not print anything */ 
                     /*   SIC_PRINT_ERRORS - print error messages only */
                     /*   SIC_PRINT_INFO - print most important information */
                     /*   SIC_PRINT_ALLINFO - print all available information */
  int* Nr_iter,      /* in:	the maximum iterations to be performed */
                     /* out:	actual number of iterations performed */
  double* Conv_meas, /* in:	tolerance level for chosen measure */
		     /* out:	the final value of convergence measure */
  double *Conv_rate  /* out (optional): the total convergence rate */ 
  )
{

  /* pointer to solver structure */
  sit_solvers *solver_p;
  sit_levels *level_p; /* mesh levels */

  /* pointer to dofs structure */
  sit_dof_struct *dof_struct_p;
  
  /* auxiliary variables */
  int nrdofs_glob, max_nrdofs, nr_dof_ent, nr_levels, posglob, nrdofs_int_ent;
  int l_dof_ent_id[SIC_MAX_DOF_PER_INT], l_dof_ent_nrdof[SIC_MAX_DOF_PER_INT];
  int l_dof_ent_posglob[SIC_MAX_DOF_PER_INT];
  int l_dof_ent_type[SIC_MAX_DOF_PER_INT];
  int pdr_comp_type;
  double *stiff_mat, *rhs_vect, *x_ini, normb;
  int i,j,k, iaux, kaux, intent, ibl, ient, nrdofbl, ini_zero;
  int level_id=0;
  char rewrite;


/* variables to store timing results */
  double t_int_el=0.0;
  //double t_int_fa=0.0;
  double t_fac_dia=0.0;
  double t_iter=0.0;
  double t_temp=0.0;
  double t_total=0.0;
  double time_clock();

/*++++++++++++++++ executable statements ++++++++++++++++*/

  siv_cur_solver_id=Solver_id;
  solver_p = &siv_solver[siv_cur_solver_id];
  level_p = &(siv_solver[Solver_id].level[0]);

  nr_levels = siv_solver[Solver_id].nr_levels;

  /*kbw
  printf("in sir_solve: solver %d, nr_levels %d, level_p %lu, nrdofs_glob %d\n",
	 Solver_id, nr_levels, level_p, level_p->nrdofs_glob);
  printf("Monitor %d, Nr_iter %d, Conv_meas %15.12lf\n",
	 Monitor, *Nr_iter, *Conv_meas);
  /*kew*/


#ifdef TIME_TEST
  t_total=time_clock();
#endif

  /* allocate memory for the initial guess and solution vectors */
  nrdofs_glob = siv_solver[Solver_id].level[nr_levels-1].nrdofs_glob;
  x_ini = (double *)malloc(nrdofs_glob*sizeof(double));
  ini_zero=1;
  for(i=0;i<nrdofs_glob;i++) x_ini[i]=0.0;
  
/*------------------------------------------------------------*/
/* GET INITIAL GUESS WHEN POSSIBLE AND NECESSARY              */
/*------------------------------------------------------------*/
  if(Ini_guess>0){
    
    ini_zero=0;
    
    /* get initial guess */
    for(ibl=0;ibl<level_p->nr_dof_ent;ibl++){
      int dof_struct_id = ibl;
      sit_dof_struct dof_struct = level_p->l_dof_struct[dof_struct_id];

  /*kbw
      printf("in sir_solve before pdr_read_sol_dofs: ibl %d, struct_id %d \n",
	     ibl, dof_struct_id);
      printf("dof_struct_p %lu\n", dof_struct_p);
  printf("problem_id %d, Dof_ent_type %d, Dof_ent_id %d, nrdof %d\n",
	solver_p->problem_id, dof_struct.dof_ent_type, dof_struct.dof_ent_id,  
	 dof_struct.nrdofs);
  /*kew*/

      
      pdr_read_sol_dofs(solver_p->problem_id, 
			dof_struct.dof_ent_type,
			dof_struct.dof_ent_id,
			dof_struct.nrdofs,
			&x_ini[dof_struct.posglob]);
      
/*kbw
      if(ibl<5){
printf("Initial guess in dof_ent %d (local %d)\n", dof_struct.dof_ent_id, ibl);
for (i=0;i<dof_struct.nrdofs;i++) printf("%20.10lf",x_ini[dof_struct.posglob+i]);
printf("\n");
    getchar();
      }
/*kew*/

    }
    
/*kbw
    //if(siv_solver[Solver_id].parallel){
      printf("x_ini before exchange dofs\n");
      for(i=0;i<nrdofs_glob;i++) printf("%5d%15.10lf",i/4+1,x_ini[i]);
      printf("\n");
      getchar();
      //}
/*kew*/

/*||begin||
    if(siv_solver[Solver_id].parallel){
      pdr_exchange_dofs(solver_p->problem_id, Solver_id, nr_levels-1, x_ini);
    }
/*||end||*/



/*kbw
    //if(siv_solver[Solver_id].parallel){
      printf("x_ini after exchange dofs\n");
      for(i=0;i<nrdofs_glob;i++) printf("%5d%15.10lf",i/4+1,x_ini[i]);
      printf("\n");
      getchar();
      //}
/*kew*/
  
  }


#ifdef MULTITHREADED
  // Solver controls assembling - one solver thread per one problem thread - no OpenMP
  int Assemble_control = SIC_PROBLEM_ASSEMBLE;
#else
  // Problem module controls assembling - one call for many elements - OpenMP possible
  int Assemble_control = SIC_SOLVER_ASSEMBLE;
  //int Assemble_control = SIC_PROBLEM_ASSEMBLE;
#endif

#ifdef TIME_TEST
    printf("\nbeginning integration of %d elements and faces\n",
	   level_p->nr_int_ent);
    t_temp=time_clock();
#endif    

  if(Assemble_control == SIC_SOLVER_ASSEMBLE){
    
    /* allocate memory for the stiffness matrices and RHS */
    max_nrdofs = level_p->max_dofs_int_ent;
    stiff_mat = (double *)malloc(max_nrdofs*max_nrdofs*sizeof(double));
    rhs_vect = (double *)malloc(max_nrdofs*sizeof(double));
    
    lsr_mkb_clear_matrix(Solver_id, level_id, Comp_type);   
	
    {
      
      /* compute local stiffness matrices */
      for(intent=0;intent<level_p->nr_int_ent;intent++){
	
	int nrdfobl;
	int idofent;
	int nr_dof_ent = SIC_MAX_DOF_PER_INT;
	int nrdofs_int_ent = max_nrdofs;
	int l_bl_id[SIC_MAX_DOF_PER_INT], l_bl_nrdofs[SIC_MAX_DOF_PER_INT];
	
	if(Comp_type==SIC_SOLVE) pdr_comp_type = PDC_COMP_BOTH;
	else pdr_comp_type = PDC_COMP_RHS;
	
	pdr_comp_stiff_mat(solver_p->problem_id, level_p->l_int_ent_type[intent], 
			   level_p->l_int_ent_id[intent], pdr_comp_type, NULL,
			   &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdof, 
			   &nrdofs_int_ent, stiff_mat, rhs_vect, &rewrite);
	
	
#ifdef DEBUG_SIM
	if(nrdofs_int_ent>max_nrdofs){
	  printf("Too small arrays stiff_mat and rhs_vect passed to comp_el_stiff_mat\n");
	  printf("from sir_create in sis_mkb. %d < %d. Exiting !!!",
		 max_nrdofs, nrdofs_int_ent);
	  exit(-1);
	}
#endif
	
    
	
    /*kbw
    //if(level_p->l_int_ent_id[intent]==1 || level_p->l_int_ent_id[intent]==4) {
      printf("In sir_solve before assemble: Solver_id %d, level_id %d, sol_typ %d\n", Solver_id, level_id, SIC_SOLVE);
      int ibl,jbl,pli,plj,nri,nrj,nrdof,jaux;
      printf("ient %d, int_ent_id %d, nr_dof_ent %d\n", 
	     intent, level_p->l_int_ent_id[intent], nrdofbl);
      pli = 0; nrdof=0;
      for(ibl=0;ibl<nrdofbl; ibl++) nrdof+=l_bl_nrdofs[ibl];
      for(ibl=0;ibl<nrdofbl; ibl++){
	printf("bl_id %d, bl_nrdof %d\n",
	  l_bl_id[ibl],l_bl_nrdofs[ibl]);
	nri = l_bl_nrdofs[ibl];
	plj=0;
	for(jbl=0;jbl<nrdofbl;jbl++){
	  printf("Stiff_mat (blocks %d:%d)\n",jbl,ibl);
	  nrj = l_bl_nrdofs[jbl];
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
      //}
/*kew*/
    
	sir_assemble_local_stiff_mat(Solver_id, level_id, SIC_SOLVE,
				   nr_dof_ent, l_dof_ent_type,
				   l_dof_ent_id,l_dof_ent_nrdof,
				   stiff_mat, rhs_vect, &rewrite);
		
      } /* end loop over integration entities: ient */
      
    }

  } // end if solver controls assembly

  // if problem module controls assembly
  else{


    lsr_mkb_clear_matrix(Solver_id, level_id, Comp_type);
    

    if(Comp_type==SIC_SOLVE) pdr_comp_type = PDC_COMP_BOTH;
    else pdr_comp_type = PDC_COMP_RHS;

/*kbw
  printf("before  pdr_create_assemble_stiff_mat: Problem_id %d, Level_id %d\n",
	 solver_p->problem_id, level_id);
  printf("Comp_type %d, Nr_int_ent %d, Max_dofs_int_ent %d\n",
	 pdr_comp_type, level_p->nr_int_ent, level_p->max_dofs_int_ent);
/*kew*/

    pdr_create_assemble_stiff_mat(solver_p->problem_id, level_id, pdr_comp_type,
				  level_p->nr_int_ent,
				  level_p->l_int_ent_type, 
				  level_p->l_int_ent_id,
				  level_p->max_dofs_int_ent);
    


  }
    
#ifdef TIME_TEST
  t_int_el+=time_clock()-t_temp;
  printf("beginning factorization of diagonal blocks\n");
  t_temp=time_clock();
#endif

 //lsr_mkb_show_blocks_martix(Solver_id, level_id);

  lsr_mkb_fill_precon(Solver_id, level_id);

#ifdef TIME_TEST
  t_fac_dia+=time_clock()-t_temp;
#endif



  /*kbw
    printf("Initial guess:\n");
    for(i=0;i<nrdofs_glob;i++)printf("%20.15lf",x_ini[i]);
    printf("\n");
    getchar();getchar();
/*kew*/


#ifdef TIME_TEST
  printf("beginning iterations\n");
  t_temp=time_clock();
#endif

/*------------------------------------------------------------*/
/* SOLVE THE PROBLEM                                          */
/*------------------------------------------------------------*/



  lsr_mkb_solve(Solver_id, nrdofs_glob, Ini_guess, x_ini, rhs_vect,
		Nr_iter, Conv_meas, Monitor, Conv_rate);

  amg_solve(Solver_id, nrdofs_glob, Ini_guess, x_ini, rhs_vect);

#ifdef TIME_TEST
  t_iter+=time_clock()-t_temp;
#endif

  /* rewrite the solution */
  int istruct;
  for(istruct=0;istruct<level_p->nr_dof_ent;istruct++){
    int dof_struct_id = istruct;
    sit_dof_struct dof_struct = level_p->l_dof_struct[dof_struct_id];
    
 //print out solution
    if(dof_struct.dof_ent_id<10){
      printf("Solution in block %d, dof_ent %d posglob=%d\n", ibl, dof_struct.dof_ent_id,dof_struct.posglob);
      for (i=0;i<dof_struct.nrdofs;i++) {
	printf("%20.10lf",x_ini[dof_struct.posglob+i]);
	printf("%20.10lf",x_ini[dof_struct_id+i]);
	}
      printf("\n");
      //getchar();
    }


    pdr_write_sol_dofs(solver_p->problem_id, 
		       dof_struct.dof_ent_type,
		       dof_struct.dof_ent_id,
		       dof_struct.nrdofs,
		       &x_ini[dof_struct.posglob]);



  }

  if(Assemble_control == SIC_SOLVER_ASSEMBLE){
    free(stiff_mat);
    free(rhs_vect);
  }
  free(x_ini);

/*ok_kbw*/
#ifdef TIME_TEST
  t_total=time_clock()-t_total;
  printf("Total solver times:\n");
  printf("\tintegration of elements and faces \t%lf\n", t_int_el);
  printf("\tfactorization of diagonal blocks  \t%lf\n", t_fac_dia);
  printf("\titerations                        \t%lf\n", t_iter);
  t_temp=t_int_el+t_fac_dia+t_iter;
  printf("\tsuma                        \t%lf\n", t_temp);
  printf("\ttotal                       \t%lf\n", t_total);
  //printf("%lf  %lf  %lf  %lf  %lf  %lf\n",
  //	 t_int_el,t_fac_dia,t_iter,t_temp,t_total);
#endif
/*kew*/


  return(1);
}


/*------------------------------------------------------------
  sir_assemble_local_stiff_mat - to assemble an element stiffness matrix
                                   to the global SM
------------------------------------------------------------*/
int sir_assemble_local_stiff_mat( 
                         /* returns: >=0 - success code, <0 - error code */
  int Solver_id,         /* in: solver ID (used to identify the subproblem) */
  int Level_id,          /* in: level ID */
  int Comp_type,         /* in: indicator for the scope of computations: */
                         /*   MKB_SOLVE - solve the system */
                         /*   MKB_RESOLVE - resolve for the new rhs vector */
  int Nr_dof_ent,        /* in: number of global dof blocks */
                         /*     associated with the local stiffness matrix */
  int* L_dof_ent_type,   /* in: list of dof blocks' IDs */
  int* L_dof_ent_id,     /* in: list of dof blocks' IDs */
  int* L_dof_ent_nrdofs, /* in: list of blocks' numbers of dof */
  double* Stiff_mat,     /* in: stiffness matrix stored columnwise */
  double* Rhs_vect,      /* in: rhs vector */
  char* Rewr_dofs         /* in: flag to rewrite or sum up entries */
                         /*   'T' - true, rewrite entries when assembling */
                         /*   'F' - false, sum up entries when assembling */
)
{
  int l_bl_id[SIC_MAX_DOF_PER_INT], l_bl_nrdofs[SIC_MAX_DOF_PER_INT];
  int solver_comp_type;
  sit_levels *level_p;       /* mesh levels */
  level_p = &siv_solver[Solver_id].level[Level_id];

  if(Comp_type==PDC_COMP_BOTH) solver_comp_type = SIC_SOLVE;
  else solver_comp_type = SIC_RESOLVE;

  int nrdofbl = Nr_dof_ent;
  int idofent;
  for(idofent=0; idofent<Nr_dof_ent; idofent++){
    int dof_ent_id = L_dof_ent_id[idofent];
    int dof_ent_type = L_dof_ent_type[idofent];
    int dof_struct_id;

    if(dof_ent_type == PDC_ELEMENT ) {

      dof_struct_id = level_p->l_dof_elem_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
      if( level_p->l_dof_elem_to_struct[dof_ent_id] == -1){
	printf("Error 347294 in sir_create!!! Exiting\n");
	exit(-1);
      }
#endif

    } else if(dof_ent_type == PDC_FACE ) {

      dof_struct_id = level_p->l_dof_face_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
      if( level_p->l_dof_face_to_struct[dof_ent_id] == -1){
	printf("Error 347294 in sir_create!!! Exiting\n");
	exit(-1);
      }
#endif

    } else if(dof_ent_type == PDC_EDGE ) {

      dof_struct_id = level_p->l_dof_edge_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
      if( level_p->l_dof_edge_to_struct[dof_ent_id] == -1){
	printf("Error 347294 in sir_create!!! Exiting\n");
	exit(-1);
      }
#endif

    } else if(dof_ent_type == PDC_VERTEX ) {

      dof_struct_id = level_p->l_dof_vert_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
      if( level_p->l_dof_vert_to_struct[dof_ent_id] == -1){
	printf("Error 347294 in sir_create!!! Exiting\n");
	exit(-1);
      }
#endif

    }

    /* blocks within solver are offset 1 */
    l_bl_id[idofent] = level_p->l_dof_struct[dof_struct_id].block_id + 1;
    l_bl_nrdofs[idofent] = level_p->l_dof_struct[dof_struct_id].nrdofs;

  }


  /*kbw
  //if(level_p->l_int_ent_id[intent]==1 || level_p->l_int_ent_id[intent]==4) {
      printf("In sir_solve before assemble: Solver_id %d, level_id %d, sol_typ %d\n", Solver_id, level_id, SIC_SOLVE);
      int ibl,jbl,pli,plj,nri,nrj,nrdof,jaux;
      printf("ient %d, int_ent_id %d, nr_dof_ent %d\n",
	     intent, level_p->l_int_ent_id[intent], nrdofbl);
      pli = 0; nrdof=0;
      for(ibl=0;ibl<nrdofbl; ibl++) nrdof+=l_bl_nrdofs[ibl];
      for(ibl=0;ibl<nrdofbl; ibl++){
	printf("bl_id %d, bl_nrdof %d\n",
	  l_bl_id[ibl],l_bl_nrdofs[ibl]);
	nri = l_bl_nrdofs[ibl];
	plj=0;
	for(jbl=0;jbl<nrdofbl;jbl++){
	  printf("Stiff_mat (blocks %d:%d)\n",jbl,ibl);
	  nrj = l_bl_nrdofs[jbl];
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
      //}
/*kew*/


  lsr_mkb_assemble_local_sm(Solver_id, Level_id, solver_comp_type,
			    nrdofbl, l_bl_id, l_bl_nrdofs,
			    Stiff_mat, Rhs_vect, Rewr_dofs);

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

  int nr_levels, level_id;
  sit_levels *level_p;       /* mesh levels */

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* free solver data structures */
  lsr_mkb_free_matrix(Solver_id);

  siv_cur_solver_id=Solver_id;
  nr_levels = siv_solver[Solver_id].nr_levels;
  level_id =0;

  /* in a big loop over mesh levels free renumbering data structures */
  //for(ilev=0;ilev<nr_levels;ilev++){

  level_p = &siv_solver[Solver_id].level[level_id];
  
  free(level_p->l_int_ent_type);
  free(level_p->l_int_ent_id);
  if(level_p->max_dof_elem_id>=0) free(level_p->l_dof_elem_to_struct);
  if(level_p->max_dof_face_id>=0) free(level_p->l_dof_face_to_struct);
  if(level_p->max_dof_edge_id>=0) free(level_p->l_dof_edge_to_struct);
  if(level_p->max_dof_vert_id>=0) free(level_p->l_dof_vert_to_struct);
  free(level_p->l_dof_struct);

    //}

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

/*---------------------------------------------------------
  sir_sort_short - to sort an array by insertion (for small integer arrays)
---------------------------------------------------------*/
int sir_sort_short(
		   int* A, // array
		   int p,  //first index
		   int k   // last index
		   )
{
  int i,j;
  int t;

  for(i=p+1;i<=k;i++) {
    t=A[i];
    j=i-1;
    while( j>=p && A[j]>t ) {
      A[j+1]=A[j];
      j--;
    }
    A[j+1]=t;
  }
  return 0;
}


#ifdef TIME_TEST

///* PROCEDURES AND VARIABLES THAT ARE SYSTEM DEPENDENT */
//#include<time.h>
//#include<sys/time.h>
//#include<sys/resource.h>
//
///*---------------------------------------------------------
//time_clock - to return number of wall clock seconds from 
//	time measurement initialization
//---------------------------------------------------------*/
//double time_clock()
//{ 
//
//  double daytime;
//  struct timezone tzp;
//  struct timeval tk;
//
//  gettimeofday(&tk, &tzp);
//
//  daytime=(tk.tv_usec)/1e6+tk.tv_sec;
//
//return(daytime);
//}
//
///*---------------------------------------------------------
//su_getcputime - to return number of cpu seconds from 
//	time measurement initialization
//---------------------------------------------------------*/
//double su_getcputime()
//{ 
//
//  double cputime;
//  struct rusage rk;
//
//  getrusage(RUSAGE_SELF, &rk);
//
//  cputime = (rk.ru_utime.tv_usec)/1e6;
//  cputime += rk.ru_utime.tv_sec;
//
//return(cputime);
//}


#endif
