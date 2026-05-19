/************************************************************************
File uts_ls_intf - utility routines for interactions with linear solver
                   (common to all problem modules, 
		   possibly used also by other modules)

Contains definitions of routines:

  utr_get_list_ent - to return the list of:
			  1. integration entities - entities
			 for which stiffness matrices and load vectors are
			 provided by the FEM code to the solver module,
		  2. DOF entities - entities with which there are dofs 
			 associated by the given approximation

  utr_get_list_ent_coarse - to return the list of integration entities - entities
                         for which stiffness matrices and load vectors are
                         provided by the FEM code to the solver module,
                         and DOF entities - entities with which there are dofs 
                         associated by the given approximation for COARSE level
                         given the corresponding lists from the fine level 
              (it calls separate implementations for dg and std?)

  utr_create_assemble_stiff_mat - to create element stiffness matrices
                                 and assemble them to the global SM
              (it calls implementations for particular platfroms)

  utr_create_assemble_stiff_mat_elem_accel - to create element stiffness matrices
                                 and assemble them to the global SM using ACCELERATOR

------------------------------
History:
	08.2008 - Krzysztof Banas, pobanas@cyf-kr.edu.pl, initial version
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<assert.h>
#include<signal.h>

#include<omp.h>

/* interface for all mesh manipulation modules */
#include "mmh_intf.h"

/* interface for all approximation modules */
#include "aph_intf.h"

#ifdef PARALLEL
/* interface for parallel mesh manipulation modules */
#include "mmph_intf.h"	

/* interface with parallel communication library */
#include "pch_intf.h"
#endif

/* interface for general purpose utilities - for all problem dependent modules*/
#include "uth_intf.h"

/* interface for linear algebra packages */
#include "lin_alg_intf.h"

/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"

#include "pdh_intf.h"

// maximal number of solution components
#define UTC_MAXEQ PDC_MAXEQ


/*------------------------------------------------------------
  utr_get_list_ent - to return the list of integration entities - entities
			 for which stiffness matrices and load vectors are
			 provided by the FEM code to the solver module,
			 and DOF entities - entities with which there are dofs 
			 associated by the given approximation 
------------------------------------------------------------*/
int utr_get_list_ent( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,     /* in:  problem (and solver) identification */
  int* Nr_int_ent,    /* out: number of integration entitites */
	/* GHOST ENTITIES HAVE NEGATIVE TYPE !!! */
  int** List_int_ent_type,/* out: list of types of integration entitites */
  int** List_int_ent_id,  /* out: list of IDs of integration entitites */
  int* Nr_dof_ent,    /* out: number of dof entities (entities with which there
					  are dofs associated by the given approximation) */
  int** List_dof_ent_type,/* out: list of types of integration entitites */
  int** List_dof_ent_id,  /* out: list of IDs of integration entitites */
  int** List_dof_ent_nrdofs,/* out: list of no of dofs for 'dof' entity */
  int* Nrdofs_glob,    /* out: global number of degrees of freedom (unknowns) */
  int* Max_dofs_per_dof_ent/* out: maximal number of dofs per dof entity */
  )
{

  char field_module_name[100];
  int solver_id, field_id, mesh_id, nreq;
  int nel, nfa, nno, int_ent, dof_ent, nr_elem, nr_face, nr_node, i, iaux, iedg; 
  int nrdofsgl, nrintent, nrdofent, face_neig[2];
  int nrdofsloc, max_dofs_ent_dof;
  int *temp_list_dofs; int *temp_list_edges;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* associate the suitable field with the problem and the solver */
  i=2; mesh_id=pdr_ctrl_i_params(Problem_id,i);
  i=3; field_id = pdr_ctrl_i_params(Problem_id,i);
  i=6; solver_id = pdr_ctrl_i_params(Problem_id,i);
  //mesh_id = apr_get_mesh_id(field_id);
  nreq=apr_get_nreq(field_id);

  // check the name of the field module
  apr_module_introduce(field_module_name);

  // for standard continuous linear FEM approximation
  if( strncmp(field_module_name, "STANDARD_LINEAR", 15) == 0){ 

#ifdef PARALLEL
	/* temporary list of nodes */
	iaux = mmr_get_max_node_id(mesh_id);
	temp_list_dofs = (int *) malloc( (iaux+1)*sizeof(int) );
	for(i=0;i<=iaux;i++) temp_list_dofs[i]=0;
#endif

	/* prepare arrays */
	nr_elem = mmr_get_nr_elem(mesh_id);
	nr_face = mmr_get_nr_face(mesh_id);
	nr_node= mmr_get_nr_node(mesh_id);
	
	*List_int_ent_type = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );
	*List_int_ent_id = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );
	
	*List_dof_ent_type = (int *) malloc( (nr_node+1)*sizeof(int) );
	*List_dof_ent_id = (int *) malloc( (nr_node+1)*sizeof(int) );
	*List_dof_ent_nrdofs = (int *) malloc( (nr_node+1)*sizeof(int) );
	
	if((*List_dof_ent_nrdofs)==NULL){
	  printf("Not enough space for allocating lists in utr_get_list_ent! Exiting\n");
	  exit(-1);
	} 
	
	nrdofsgl = 0;
	
	int_ent=0; dof_ent=0;
	
	/* loop over elements - integration entities*/
	nel=0;
	while((nel=mmr_get_next_elem_all(mesh_id, nel))!=0){
	  
	  if(mmr_el_status(mesh_id,nel)==MMC_ACTIVE ) {
	
	int_ent++;
	(*List_int_ent_type)[int_ent-1]=PDC_ELEMENT;
	(*List_int_ent_id)[int_ent-1]=nel;
	
	  } // end if element included for integration
	  
	} // end for all elements
	
	/*loop over faces*/
	nfa=0;
	while((nfa=mmr_get_next_face_all(mesh_id, nfa))!=0) {
	  
	  if(mmr_fa_status(mesh_id,nfa)==MMC_ACTIVE) {
	if(mmr_fa_bc(mesh_id, nfa)>0) {
	  
	  int_ent++;
	  (*List_int_ent_type)[int_ent-1]=PDC_FACE;
	  (*List_int_ent_id)[int_ent-1]=nfa;
	  
	}
	  }
	}
	
	/*loop over vertexes - dof entities*/
	
	nno=0;
	while((nno=mmr_get_next_node_all(mesh_id,nno))!=0) {
	  
/*kbw
  printf("In pdr_get_list_ent: nno %d, pdeg %d, dof_ent %d, nrdofsgl %d\n",
  nno,apr_get_ent_pdeg(field_id,APC_VERTEX,nno),dof_ent, nrdofsgl);
  /*kew*/

	  if(apr_get_ent_pdeg(field_id,APC_VERTEX,nno)>0 ){
	
#ifdef PARALLEL
	/* in first round only internal vertices=nodes are considered */ 
	if(mmpr_ve_owner(mesh_id,nno)==pcr_my_proc_id()){
#endif
	  
	  /* real node pdeg>0 */
	  (*List_dof_ent_type)[dof_ent]=PDC_VERTEX;
	  (*List_dof_ent_id)[dof_ent]=nno;
	  (*List_dof_ent_nrdofs)[dof_ent]=nreq;
	  dof_ent++;
	  nrdofsgl+=nreq;
	  
#ifdef PARALLEL
	  temp_list_dofs[nno]=1;
	} /* end if internal vertex */
#endif
	  }
	  else if(apr_get_ent_pdeg(field_id,APC_VERTEX,nno)==0){
	/* constrained node pdeg==0 */
	  }
	  
	}
	
/*kbw
  printf("After first round pdr_get_list_ent - internal nodes\n");  
  printf("\nNr_dof_ent %d, Nrdofs_glob %d, Max_dofs_per_dof_ent %d\n",
  dof_ent, nrdofsgl, nreq);
  for(i=0;i<dof_ent;i++)  printf("type %d, id %d, nrdofs %d\n",
  (*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdofs)[i]);
//kew*/
	
#ifdef PARALLEL
	// second round
	{
	  int nr_dof_ent_loc = 27; // maximal number of DOF ents per INT ent
	  // i.e. maximal number of DOF blocks (nodes) per element
	  int l_dof_ent_types[27], l_dof_ent_ids[27], l_dof_ent_nrdofs[27];
	  int ino, is_in_overlap;
	  
	  nel=0;
	  while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0){
	
	is_in_overlap=0;
	pdr_comp_stiff_mat(Problem_id, PDC_ELEMENT, nel, PDC_NO_COMP, NULL,
			   &nr_dof_ent_loc, l_dof_ent_types, 
			   l_dof_ent_ids, l_dof_ent_nrdofs,
			   NULL, NULL, NULL, NULL);
	
	// check whether there are owned DOFs (vertices=nodes)
	
	for(ino=0;ino<nr_dof_ent_loc;ino++){
	  
	  nno = l_dof_ent_ids[ino];
	  
	  if(mmpr_ve_owner(mesh_id,nno)!=pcr_my_proc_id()){
		is_in_overlap = 1;
		break;
	  }
	  
	}
	if(is_in_overlap){
	  
	  for(ino=0;ino<nr_dof_ent_loc;ino++){
		
		nno = l_dof_ent_ids[ino];
		
		if(temp_list_dofs[nno]==0){
		  
		  temp_list_dofs[nno]=1;
		  
		  (*List_dof_ent_type)[dof_ent]=PDC_VERTEX;
		  (*List_dof_ent_id)[dof_ent]=nno;
		  (*List_dof_ent_nrdofs)[dof_ent]=nreq;
		  dof_ent++;
		  nrdofsgl+=nreq;
		  
		  
		  
		}
	  }
	} // end if element in overlap
	  } // end loop over active elements
	} // end of second round
	
#endif
	
	*Nr_int_ent = int_ent;
	*Nrdofs_glob = nrdofsgl;
	*Max_dofs_per_dof_ent = nreq;
	*Nr_dof_ent = dof_ent;
	
/*kbw
  printf("In pdr_get_list_ent for standard linear approximation\n");
  printf("nrelem %d, nrface %d, nr_int_ent %d\n",
  nr_elem, nr_face, *Nr_int_ent);
  for(i=0;i<int_ent;i++)  printf("type %d, id %d\n",
  (*List_int_ent_type)[i],(*List_int_ent_id)[i]);
  printf("\nNr_dof_ent %d, Nrdofs_glob %d, Max_dofs_per_dof_ent %d\n",
  *Nr_dof_ent, *Nrdofs_glob, *Max_dofs_per_dof_ent);
  for(i=0;i<dof_ent;i++)  printf("type %d, id %d, nrdofs %d\n",
  (*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdofs)[i]);
  /*kew*/
	
  } // end if standard continuous linear FEM approximation
  else if(!strncmp(field_module_name,"STANDARD_QUADRATIC",18)) {

    /* auxiliary variable */
    int nr_edge, nne;

#ifdef PARALLEL
	/* temporary list of nodes */
    iaux = mmr_get_max_node_id(mesh_id);
    temp_list_dofs = (int *) malloc( (iaux+1)*sizeof(int) );
    for(i=0;i<=iaux;i++) temp_list_dofs[i]=0;

    iedg = mmr_get_max_edge_id(mesh_id);
    temp_list_edges = (int *) malloc( (iedg+1)*sizeof(int) );
    for(i=0;i<=iedg;i++) temp_list_edges[i]=0;
#endif
    
    /* prepare arrays */
    nr_elem = mmr_get_nr_elem(mesh_id);
    nr_face = mmr_get_nr_face(mesh_id);
    nr_node = mmr_get_nr_node(mesh_id);
    nr_edge = mmr_get_nr_edge(mesh_id);

    *List_int_ent_type = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );
    *List_int_ent_id = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );

    *List_dof_ent_type = (int *) malloc( (nr_node+nr_edge+1)*sizeof(int) );
    *List_dof_ent_id = (int *) malloc( (nr_node+nr_edge+1)*sizeof(int) );
    *List_dof_ent_nrdofs = (int *) malloc( (nr_node+nr_edge+1)*sizeof(int) );


    if((*List_dof_ent_nrdofs)==NULL){
      printf("Not enough space for allocating lists in utr_get_list_ent! Exiting\n");
      exit(-1);
    }

    nrdofsgl = 0;

    int_ent=0; dof_ent=0;

    /* loop over elements - integration entities*/
    nel=0;
    while((nel=mmr_get_next_elem_all(mesh_id, nel))!=0){
      if(mmr_el_status(mesh_id,nel)==MMC_ACTIVE ) {
	int_ent++;
	(*List_int_ent_type)[int_ent-1]=PDC_ELEMENT;
	(*List_int_ent_id)[int_ent-1]=nel;
      } // end if element included for integration
    } // end for all elements

    /*loop over faces*/
    nfa=0;
    while((nfa=mmr_get_next_face_all(mesh_id, nfa))!=0) {
      if(mmr_fa_status(mesh_id,nfa)==MMC_ACTIVE) {
	if(mmr_fa_bc(mesh_id, nfa)>0) {
	  int_ent++;
	  (*List_int_ent_type)[int_ent-1]=PDC_FACE;
	  (*List_int_ent_id)[int_ent-1]=nfa;
	}
      }
    }

    /*loop over vertexes - dof vertex entities*/
    nno=0;
    while((nno=mmr_get_next_node_all(mesh_id,nno))!=0) {
      if(apr_get_ent_pdeg(field_id,APC_VERTEX,nno)>0 ){

#ifdef PARALLEL
	/* in first round only internal vertices=nodes are considered */ 
	if(mmpr_ve_owner(mesh_id,nno)==pcr_my_proc_id()){
#endif

	  /* real node pdeg>0 */
	  (*List_dof_ent_type)[dof_ent]=PDC_VERTEX;
	  (*List_dof_ent_id)[dof_ent]=nno;
	  (*List_dof_ent_nrdofs)[dof_ent]=nreq;
	  dof_ent++;
	  nrdofsgl+=nreq;

#ifdef PARALLEL
	  temp_list_dofs[nno]=1;
	} /* end if internal vertex */
#endif

      }
      else if(apr_get_ent_pdeg(field_id,APC_VERTEX,nno)==0){
	/* constrained node pdeg==0 */
      }
    }

    /*loop over edges - dof edge entities*/
    nne=0;
    while((nne=mmr_get_next_edge_all(mesh_id,nne))!=0) {
      if(apr_get_ent_pdeg(field_id,APC_EDGE,nne)>0){

#ifdef PARALLEL
	/* in first round only internal vertices=nodes are considered */ 
	if(mmpr_ed_owner(mesh_id,nne)==pcr_my_proc_id()){
#endif

	/* real node pdeg>0 */
	(*List_dof_ent_type)[dof_ent]=PDC_EDGE;
	(*List_dof_ent_id)[dof_ent]=nne;
	(*List_dof_ent_nrdofs)[dof_ent]=nreq;
	dof_ent++;
	nrdofsgl+=nreq;

#ifdef PARALLEL
	  temp_list_edges[nne]=1;
	} /* end if internal vertex */
#endif
      }
      else if(apr_get_ent_pdeg(field_id,APC_EDGE,nne)==0){
	/* constrained node pdeg==0 */
      } 
      else if(apr_get_ent_pdeg(field_id,APC_EDGE,nne)==-11){

#ifdef PARALLEL
	/* in first round only internal vertices=nodes are considered */ 
	if(mmpr_ed_owner(mesh_id,nne)==pcr_my_proc_id()){
#endif

	/* inactive node in use pdeg==-11 */
	(*List_dof_ent_type)[dof_ent]=PDC_EDGE;
	(*List_dof_ent_id)[dof_ent]=nne;
	(*List_dof_ent_nrdofs)[dof_ent]=nreq;
	dof_ent++;
	nrdofsgl+=nreq;

#ifdef PARALLEL
	  temp_list_edges[nne]=1;
	} /* end if internal vertex */
#endif
      }
    }


/*kbw
  printf("After first round pdr_get_list_ent - internal nodes\n");  
  printf("\nNr_dof_ent %d, Nrdofs_glob %d, Max_dofs_per_dof_ent %d\n",
  dof_ent, nrdofsgl, nreq);
  for(i=0;i<dof_ent;i++)  printf("type %d, id %d, nrdofs %d\n",
  (*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdofs)[i]);
//kew*/
	
#ifdef PARALLEL

    // second round
    {
      int nr_dof_ent_loc = 27; // maximal number of DOF ents per INT ent
      // i.e. maximal number of DOF blocks (nodes) per element
      int l_dof_ent_types[27], l_dof_ent_ids[27], l_dof_ent_nrdofs[27];
      int ino, is_in_overlap;
	  
      nel=0;
      while((nel=mmr_get_next_act_elem(mesh_id, nel))!=0) {
	
	is_in_overlap=0;
	pdr_comp_stiff_mat(Problem_id, PDC_ELEMENT, nel, PDC_NO_COMP, NULL,
			   &nr_dof_ent_loc, l_dof_ent_types, 
			   l_dof_ent_ids, l_dof_ent_nrdofs,
			   NULL, NULL, NULL, NULL);
	
	// check whether there are owned DOFs (vertices=nodes)
	
	for(ino=0;ino<nr_dof_ent_loc;ino++){
	  
	  nno = l_dof_ent_ids[ino];
	  
	  if(mmpr_ve_owner(mesh_id,nno)!=pcr_my_proc_id()){
		is_in_overlap = 1;
		break;
	  }
	  
	}
	if(is_in_overlap){
	  
	  for(ino=0;ino<nr_dof_ent_loc;ino++){
		
	    nno = l_dof_ent_ids[ino];
		
	    if(temp_list_dofs[nno]==0){
		  
	      temp_list_dofs[nno]=1;
		  
	      (*List_dof_ent_type)[dof_ent]=PDC_VERTEX;
	      (*List_dof_ent_id)[dof_ent]=nno;
	      (*List_dof_ent_nrdofs)[dof_ent]=nreq;
	      dof_ent++;
	      nrdofsgl+=nreq;
		  		  	      
	    }
	  }
	} // end if element in overlap
      } // end loop over active elements
    } // end of second round
	
#endif

    *Nr_int_ent = int_ent;
    *Nrdofs_glob = nrdofsgl;
    *Max_dofs_per_dof_ent = nreq;
    *Nr_dof_ent = dof_ent;
    
    /*kbw
      printf("In pdr_get_list_ent for standard quadratic approximation\n");
      printf("nrelem %d, nrface %d, nr_int_ent %d\n",
      nr_elem, nr_face, *Nr_int_ent);
      for(i=0;i<int_ent;i++)  printf("type %d, id %d\n",(*List_int_ent_type)[i],(*List_int_ent_id)[i]);
      printf("\nNr_dof_ent %d, Nrdofs_glob %d, Max_dofs_per_dof_ent %d\n",*Nr_dof_ent, *Nrdofs_glob, *Max_dofs_per_dof_ent);
      for(i=0;i<dof_ent;i++)  printf("type %d, id %d, nrdofs %d\n",(*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdofs)[i]);
    /*kbw*/
  
  } // end if standard quadratic approximation
  else{
	
	// for discontinuous Galerkin approximation
	
	/* prepare arrays */
	nr_elem = mmr_get_nr_elem(mesh_id);
	nr_face = mmr_get_nr_face(mesh_id);
	
	*List_int_ent_type = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );
	*List_int_ent_id = (int *) malloc( (nr_elem+nr_face+1)*sizeof(int) );
	*List_dof_ent_type = (int *) malloc( (nr_elem+1)*sizeof(int) );
	*List_dof_ent_id = (int *) malloc( (nr_elem+1)*sizeof(int) );
	*List_dof_ent_nrdofs = (int *) malloc( (nr_elem+1)*sizeof(int) );
	
#ifdef PARALLEL
	/* temporary list of elements */
	iaux = mmr_get_max_elem_id(mesh_id);
	temp_list_dofs = (int *) malloc( (iaux+1)*sizeof(int) );
	for(i=0;i<=iaux;i++) temp_list_dofs[i]=0;
#endif
	
	if((*List_dof_ent_nrdofs)==NULL){
	  printf("Not enough space for allocating lists in pdr_get_list_ent! Exiting\n");
	  exit(-1);
	} 
	
	nrdofsgl = 0; max_dofs_ent_dof = 0;
	
	int_ent=0; dof_ent=0;
	
	/* lop over elements - integration and dof entities in DG */
	nel=0;
	while((nel=mmr_get_next_elem_all(mesh_id, nel))!=0){
	  
#ifdef PARALLEL
	  /* integration concerns only internal elements */ 
	  if(mmpr_el_owner(mesh_id,nel)==pcr_my_proc_id()){
#endif
	
	if(mmr_el_status(mesh_id,nel)==MMC_ACTIVE ) {
	  
	  int_ent++;
	  (*List_int_ent_type)[int_ent-1]=PDC_ELEMENT;
	  (*List_int_ent_id)[int_ent-1]=nel;
	  
#ifdef PARALLEL
	  temp_list_dofs[nel]=1;
#endif
	  
	  /* !!! only for DG - dofs are associated only with elements !!! */
	  dof_ent++;
	  nrdofsloc = apr_get_ent_nrdofs(field_id, APC_ELEMENT, nel);
	  (*List_dof_ent_type)[dof_ent-1]=PDC_ELEMENT;
	  (*List_dof_ent_id)[dof_ent-1]=nel;
	  (*List_dof_ent_nrdofs)[dof_ent-1]=nrdofsloc;
	  
	  if(nrdofsloc>max_dofs_ent_dof) max_dofs_ent_dof = nrdofsloc;
	  nrdofsgl += nrdofsloc;
	  
	  /*kbw
		printf(" element %d int_ent %d, dof_ent %d, nrdof %d\n", 
		nel, int_ent, dof_ent, nrdofsgl);
		/*kew*/ 
	  
	  
	} /* end if element included for integration */
	
#ifdef PARALLEL
	  } /* end if internal element */
#endif
	  
	} /* end for all elements */
	
	
	/* loop over faces - integration entities in DG */
	nfa=0;
	while((nfa=mmr_get_next_face_all(mesh_id, nfa))!=0){
	  
#ifdef PARALLEL
	  /* integration concerns only faces not on the inter-subdomain boundary */ 
	  if(!mmr_fa_sub_bnd(mesh_id, nfa)){
#endif
	
	if(mmr_fa_status(mesh_id,nfa)==MMC_ACTIVE ) {
	  
#ifdef PARALLEL
	  /* integration concerns only faces adjacent to internal elements */ 
	  mmr_fa_neig(mesh_id, nfa, face_neig, NULL, NULL, NULL, NULL, NULL);
	  face_neig[0]=abs(face_neig[0]);
	  face_neig[1]=abs(face_neig[1]);
	  if(mmpr_el_owner(mesh_id,face_neig[0])==pcr_my_proc_id() ||
		 (face_neig[1]!=0 && 
		  mmpr_el_owner(mesh_id,face_neig[1])==pcr_my_proc_id())) {
#endif
		
		/* add face to the list of integration entities */
		int_ent++;
		(*List_int_ent_type)[int_ent-1]=PDC_FACE;
		(*List_int_ent_id)[int_ent-1]=nfa;
		
		/*kbw
		  printf("face %d int_ent %d\n", nfa, int_ent);
		  /*kew*/ 
		
#ifdef PARALLEL
		/* additionally to DOFs of internal elements also DOFs of neighbours 
		   of considered faces must be taken into account */
		/* THESE GHOST ENTITIES HAVE NEGATIVE TYPE !!! */
		/* add first neighbor to the list of DOF entities */
		nel=face_neig[0];
		if(temp_list_dofs[nel]==0){
		  temp_list_dofs[nel]=1;
		  dof_ent++;
		  nrdofsloc = apr_get_ent_nrdofs(field_id, APC_ELEMENT, nel);
		  (*List_dof_ent_type)[dof_ent-1]=-PDC_ELEMENT;
		  (*List_dof_ent_id)[dof_ent-1]=nel;
		  (*List_dof_ent_nrdofs)[dof_ent-1]=nrdofsloc;
		  
		  if(nrdofsloc>max_dofs_ent_dof) max_dofs_ent_dof = nrdofsloc;
		  nrdofsgl += nrdofsloc;
		  
		  /*kbw
		printf(" element %d int_ent %d, dof_ent %d, nrdof %d\n", 
		nel, int_ent, dof_ent, nrdofsgl);
		/*kew*/ 
		}	  
		/* add second neighbor to the list of DOF entities */
		nel=face_neig[1];
		if(nel!=0 && temp_list_dofs[nel]==0){
		  temp_list_dofs[nel]=1;
		  dof_ent++;
		  nrdofsloc = apr_get_ent_nrdofs(field_id, APC_ELEMENT, nel);
		  (*List_dof_ent_type)[dof_ent-1]=-PDC_ELEMENT;
		  (*List_dof_ent_id)[dof_ent-1]=nel;
		  (*List_dof_ent_nrdofs)[dof_ent-1]=nrdofsloc;
		  
		  if(nrdofsloc>max_dofs_ent_dof) max_dofs_ent_dof = nrdofsloc;
		  nrdofsgl += nrdofsloc;
		  
		  /*kbw
		printf(" element %d int_ent %d, dof_ent %d, nrdof %d\n", 
		nel, int_ent, dof_ent, nrdofsgl);
		/*kew*/ 
		}	  
		
	  } /* end if face adjacent to internal element */
#endif
	  
	}
	
#ifdef PARALLEL
	  } /* end if face not on the inter-subdomain boundary */
#endif
	  
	} /* end loop over all faces: nfa */ 
	
	
	*Nr_int_ent = int_ent;
	*Nr_dof_ent = dof_ent;
	*Nrdofs_glob = nrdofsgl;
	*Max_dofs_per_dof_ent = max_dofs_ent_dof;
	
#ifdef PARALLEL
	/*kbw
	  sleep(pcr_my_proc_id());
	  printf("Subdomain %d\n", pcr_my_proc_id());
	  /*kew*/
#endif
	
	/*kbw
	  printf("In pdr_get_list_ent for DG approximation\n");
	  printf("nrelem %d, nrface %d, nr_int_ent %d\n",
	  nr_elem, nr_face, *Nr_int_ent);
	  for(i=0;i<int_ent;i++)  printf("type %d, id %d\n",
	  (*List_int_ent_type)[i],(*List_int_ent_id)[i]);
	  printf("\nNr_dof_ent %d, Nrdofs_glob %d, Max_dofs_per_dof_ent %d\n",
	  *Nr_dof_ent, *Nrdofs_glob, *Max_dofs_per_dof_ent); 
	  for(i=0;i<dof_ent;i++)  printf("type %d, id %d, nrdof %d\n",
	  (*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdofs)[i]);
	  /*kew*/
	
#ifdef PARALLEL
	free(temp_list_dofs);
#endif
	
  }
  
  return(1);
}


/*------------------------------------------------------------
  utr_get_list_ent_coarse - the same as above but for COARSE level and
                            given the corresponding lists from the fine level 
------------------------------------------------------------*/
int utr_get_list_ent_coarse( /* returns: >=0 - success code, <0 - error code */
  int Problem_id,	/* in:  problem (and solver) identification */
  int Nr_int_ent_fine,	/* in: number of integration entitites */
  int *List_int_ent_type_fine,	/* in: list of types of integration entitites */
  int *List_int_ent_id_fine,	/* in: list of IDs of integration entitites */
  int Nr_dof_ent_fine,	/* in: number of dof entities (entities with which there
			   are dofs associated by the given approximation) */
  int *List_dof_ent_type_fine,	/* in: list of types of integration entitites */
  int *List_dof_ent_id_fine,	/* in: list of IDs of integration entitites */
  int *List_dof_ent_nrdof_fine,	/* in: list of no of dofs for 'dof' entity */
  int Nrdof_glob_fine,	/* in: global number of degrees of freedom (unknowns) */
  int Max_dof_per_ent_fine,	/* in: maximal number of dofs per dof entity */
  int *Pdeg_coarse_p,	/* in: degree of approximation for coarse space */
  int *Nr_int_ent_p,	/* out: number of integration entitites */
  int **List_int_ent_type,	/* out: list of types of integration entitites */
  int **List_int_ent_id,	/* out: list of IDs of integration entitites */
  int *Nr_dof_ent_p,	/* out: number of dof entities (entities with which there
			   are dofs associated by the given approximation) */
  int **List_dof_ent_type,	/* out: list of types of integration entitites */
  int **List_dof_ent_id,	/* out: list of IDs of integration entitites */
  int **List_dof_ent_nrdofs,	/* out: list of no of dofs for 'dof' entity */
  int *Nrdof_glob,	/* out: global number of degrees of freedom (unknowns) */
  int *Max_dof_per_ent	/* out: maximal number of dofs per dof entity */
  )
{


  char field_module_name[100];
  int solver_id, field_id, mesh_id;
  int i, iaux, ient, nel, nfa, nreq;
  int nrdofgl, nr_int_ent, nr_dof_ent;
  int nrdofloc, max_dof_ent_dof;
  int ient_fine, int_ent_id, int_ent_type, dof_ent_id, dof_ent_type;
  int father, father_old, pdeg_coarse;
  int* temp_list_elem, * temp_list_face;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* associate the suitable field with the solver */
  /* here there is only one problem one field and one solver */
  solver_id = Problem_id;
  field_id = solver_id;
  mesh_id = apr_get_mesh_id(field_id);
  
  pdeg_coarse = *Pdeg_coarse_p;
  nreq = apr_get_nreq(field_id);

  // check the name of the field module
  apr_module_introduce(field_module_name);

  // for standard continuous linear FEM approximation
  if( strncmp(field_module_name, "STANDARD_LINEAR", 15) == 0 || strncmp(field_module_name,"STANDARD_QUADRATIC",18) == 0){ 

    printf("pdr_get_list_ent_coarse NOT IMPLEMENTED for STD approximation!");
    exit (-1);

  }
  else { // for DG approximation

/*ok_kbw*/
  printf("Constructing coarse mesh from the fine mesh: Problem_id %d\n",
	 Problem_id);
  printf("Nr_int_ent_fine %d, Nr_dof_ent_fine %d, Nrdof_glob_fine %d\n",
	 Nr_int_ent_fine, Nr_dof_ent_fine, Nrdof_glob_fine);
  printf("Max_dof_per_ent_fine %d, pdeg_coarse %d\n",
	 Max_dof_per_ent_fine, pdeg_coarse);
/*kew*/

  father_old = -1; nr_int_ent = 0; 
  temp_list_face = (int *) malloc( (Nr_int_ent_fine+1)*sizeof(int) );
  /* in a loop over fine level integration and dof entities */
  /*    - create lists of integration and dof entities for coarse level */
  /*    - find data related to coarse entities */
  for(ient_fine=0; ient_fine<Nr_int_ent_fine; ient_fine++){

    int_ent_id = List_int_ent_id_fine[ient_fine];
    int_ent_type = List_int_ent_type_fine[ient_fine];

/*kbw
    printf("In intent fine: ient %d, id %d, type %d\n",
	   ient_fine, int_ent_id, int_ent_type);
/*kew*/
 
    if(int_ent_type == PDC_ELEMENT){

      father=mmr_el_fam(mesh_id,int_ent_id,NULL,NULL);

/*kbw
      printf("father: %d (previous %d)\n", father, father_old);
/*kew*/

      if(father==0){

	temp_list_face[nr_int_ent]=int_ent_id;
	nr_int_ent++;

      }
      /* we assume families are listed together !!!!*/
      else if(father != father_old){
      
	temp_list_face[nr_int_ent]=father;
	nr_int_ent++;
	father_old = father;
      
      } 
    }
    else if(int_ent_type == PDC_FACE){

      father = mmr_fa_fam(mesh_id, int_ent_id, NULL, NULL);

/*kbw
      printf("father: %d (previous %d)\n", father, father_old);
/*kew*/

      if(father==0){
	int face_neig[2];
	/* check whether neighbors are initial mesh elements */
	mmr_fa_eq_neig(mesh_id, int_ent_id, face_neig, NULL, NULL);
	if(mmr_el_fam(mesh_id, face_neig[0], NULL, NULL)==0){

/*kbw
	  printf("face %d, neig %d, father_neig %d\n", int_ent_id, 
		 face_neig[0], mmr_el_fam(mesh_id, face_neig[0], NULL, NULL));
/*kew*/

	  temp_list_face[nr_int_ent]=int_ent_id;
	  nr_int_ent++;
	}  
      }
      else{

	if(father != father_old){
      
	  temp_list_face[nr_int_ent]=father;
	  nr_int_ent++;
	  father_old = father;
      
	} 


      }
    }
    else{

      printf("Unknown int_ent type %d in get_list_ent_coarse. Exiting!\n",
	     int_ent_type);
	exit(-1);

    }
  }

  father_old = -1; nr_dof_ent = 0;
  temp_list_elem = (int *) malloc( (Nr_dof_ent_fine+1)*sizeof(int));
  /* in a loop over fine level integration and dof entities */
  /*    - create lists of integration and dof entities for coarse level */
  /*    - find data related to coarse entities */
  for(ient_fine=0; ient_fine<Nr_dof_ent_fine; ient_fine++){

    dof_ent_id = List_dof_ent_id_fine[ient_fine];
    dof_ent_type = List_dof_ent_type_fine[ient_fine];

/*kbw
    printf("In dofent fine: ient %d, id %d, type %d\n",
	   ient_fine, dof_ent_id, dof_ent_type);
/*kew*/
 
    if(dof_ent_type == PDC_ELEMENT){

      father=mmr_el_fam(mesh_id,dof_ent_id,NULL,NULL);

/*kbw
      printf("father: %d (previous %d)\n", father, father_old);
/*kew*/

      if(father==0){

	temp_list_elem[nr_dof_ent]=dof_ent_id;
	nr_dof_ent++;

      }
      /* we assume families are listed together !!!!*/
      else if(father != father_old){
      
	temp_list_elem[nr_dof_ent]=father;
	nr_dof_ent++;
	father_old = father;
      
      } 
    }
    else{

      printf("Unknown dof_ent type %d in get_list_ent_coarse. Exiting!\n",
	     dof_ent_type);
	exit(-1);

    }

  }

/*kbw
  printf("%d coarse elements:\n", nr_dof_ent);
  for(i=0;i<nr_dof_ent; i++){
    printf("%d  ", temp_list_elem[i]);
    if(temp_list_elem[i] != temp_list_face[i]){
      printf("Not DG - really?\n");
    }
  }
  printf("\n");
  printf("%d coarse faces:\n", nr_int_ent-nr_dof_ent);
  for(i=nr_dof_ent;i<nr_int_ent; i++){
    printf("%d  ", temp_list_face[i]);
  }
  printf("\n");
/*kew*/

  if(pdeg_coarse <=0){
    iaux = temp_list_elem[0];
    if(mmr_el_status(mesh_id,iaux)==MMC_ACTIVE){ 
      pdeg_coarse = apr_get_ent_pdeg(field_id, APC_ELEMENT, iaux);
    }
    else{ 
      int istop=0;
      int sons[10];
      while(istop==0){
	mmr_el_fam(mesh_id,iaux,sons,NULL);
	for(i=1;i<=sons[0];i++){
	  if(mmr_el_status(mesh_id,sons[i])==MMC_ACTIVE){

	    pdeg_coarse = apr_get_ent_pdeg(field_id, APC_ELEMENT, sons[i]);;

/*kbw
	    printf("original element %d, antecedent %d, pdeg %d, nrdofloc %d\n",
		   nel, sons[i], pdeg_coarse, nrdofloc);
/*kew*/

	    istop=1;
	    break;
	  }
	}
	iaux = sons[1];
      }

    }
  }



/*kbw
  printf("nr_int_ent %d, nr_dof_ent %d\n", nr_int_ent, nr_dof_ent);
/*kew*/

  /* prepare arrays */

  *List_int_ent_type = (int *) malloc( (nr_int_ent+1)*sizeof(int) );
  *List_int_ent_id = (int *) malloc( (nr_int_ent+1)*sizeof(int) );
  *List_dof_ent_type = (int *) malloc( (nr_dof_ent+1)*sizeof(int) );
  *List_dof_ent_id = (int *) malloc( (nr_dof_ent+1)*sizeof(int) );
  *List_dof_ent_nrdofs = (int *) malloc( (nr_dof_ent+1)*sizeof(int) );

  if((*List_dof_ent_nrdofs)==NULL){
    printf("Not enough space for allocating lists in pdr_get_list_ent! Exiting\n");
    exit(-1);
  } 



  nrdofgl = 0; max_dof_ent_dof = 0;
  for(ient=0;ient<nr_dof_ent; ient++){

    nel = temp_list_elem[ient];
    (*List_int_ent_type)[ient]=PDC_ELEMENT;
    (*List_int_ent_id)[ient]=nel;
      
    nrdofloc = nreq*apr_get_el_pdeg_numshap(field_id, nel, &pdeg_coarse);
    (*List_dof_ent_type)[ient]=PDC_ELEMENT;
    (*List_dof_ent_id)[ient]=nel;
    (*List_dof_ent_nrdofs)[ient]=nrdofloc;
      
    if(nrdofloc>max_dof_ent_dof) max_dof_ent_dof = nrdofloc;
    nrdofgl += nrdofloc;
      
/*kbw
	  printf(" element %d ient %d, nrdofloc %d, nrdofgl %d\n", 
		 nel, ient, nrdofloc, nrdofgl);
/*kew*/ 


    
    
  } /* end for all elements */

  
  /* loop over faces - integration entities in DG */
  for(ient=nr_dof_ent;ient<nr_int_ent; ient++){
    nfa = temp_list_face[ient];

    /* add face to the list of integration entities */
    (*List_int_ent_type)[ient]=PDC_FACE;
    (*List_int_ent_id)[ient]=nfa;

/*kbw
	  printf("face %d ient %d\n", nfa, ient);
/*kew*/ 


  } /* end loop over all faces: nfa */ 



    *Nr_int_ent_p = nr_int_ent;
    *Nr_dof_ent_p = nr_dof_ent;
    *Nrdof_glob = nrdofgl;
    *Max_dof_per_ent = max_dof_ent_dof;
    *Pdeg_coarse_p = pdeg_coarse;

/*kbw
    printf("In pdr_get_list_ent\n");
    printf("nr_int_ent %d\n",*Nr_int_ent_p);
    for(i=0;i<nr_int_ent;i++)  printf("type %d, id %d\n",
	 (*List_int_ent_type)[i],(*List_int_ent_id)[i]);
    printf("\nNr_dof_ent_p %d, Nrdof_glob %d, Max_dof_per_ent %d\n",
	   *Nr_dof_ent_p, *Nrdof_glob, *Max_dof_per_ent); 
    for(i=0;i<nr_dof_ent;i++)  printf("type %d, id %d, nrdof %d\n",
	(*List_dof_ent_type)[i],(*List_dof_ent_id)[i],(*List_dof_ent_nrdof)[i]);

    getchar(); getchar();
/*kew*/

  } // end if DG

  return(1);


  }


/*------------------------------------------------------------
 utr_create_assemble_stiff_mat_elem_accel - to create element stiffness matrices
                                 and assemble them to the global SM using ACCELERATOR
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat_elem_accel(
  int Problem_id, 
  int Level_id, 
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_int_ent,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
					 );

/*------------------------------------------------------------
 utr_create_assemble_stiff_mat_elem_mic - to create element stiffness matrices
                                 and assemble them to the global SM using Intel MIC
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat_elem_mic(
  int Problem_id,
  int Level_id,
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_elems_mic,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
					 );


/*------------------------------------------------------------
 utr_create_assemble_stiff_mat_cuda - to create element stiffness matrices
                                 and assemble them to the global SM
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat_elem_cuda(
  int Problem_id,
  int Level_id,
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_elems_cuda,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
					);



/*------------------------------------------------------------
 utr_create_assemble_stiff_mat - to create element stiffness matrices
                                 and assemble them to the global SM
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat(
  int Problem_id, 
  int Level_id, 
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_int_ent,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
)
{


/*++++++++++++++++ executable statements ++++++++++++++++*/

/*kbw*/
  printf("In utr_create_assemble_stiff_mat: Problem_id %d, Level_id %d\n",
	 Problem_id, Level_id);
  printf("Comp_type %d, Nr_int_ent %d, Max_dofs_int_ent %d\n",
	 Comp_type, Nr_int_ent, Max_dofs_int_ent);
/*kew*/
  
  // divide list into elements and faces
  int nr_elems=0;  
  int int_entity; 
  int nr_elems_accel=0;
  int nr_elems_mic=0;
  int nr_elems_cuda=0;
  int previous_type = PDC_ELEMENT;
  int ok = 1;
  for(int_entity=0;int_entity<Nr_int_ent;int_entity++){
    if(L_int_ent_type[int_entity]==PDC_ELEMENT){
      nr_elems++;
      if(previous_type != PDC_ELEMENT) ok = 0;
    }
    else previous_type = PDC_FACE;
  }
  int nr_faces =  Nr_int_ent - nr_elems;

  // divide elements into processed by accelerator and standard routines
  // TODO: nr_elems_accel = pdr_nr_elems_for_accelerator(nr_elems, nr_faces);
#if (defined OPENCL_CPU || defined OPENCL_GPU || defined OPENCL_PHI || defined OPENCL_HSA)
  nr_elems_accel = nr_elems;
#endif

#ifdef OFFLOAD_MIC
  nr_elems_mic = nr_elems;
#endif

#ifdef CUDA
  nr_elems_cuda = nr_elems;
#endif

/*kbw*/
  printf("nr_elems %d (nr_elems for accelerator %d or nr_elems for mic %d or nr_elems_cuda %d), nr_faces %d, ok %d\n",
	 nr_elems, nr_elems_accel, nr_elems_mic, nr_elems_cuda, nr_faces, ok);
/*kew*/


  if(ok!=1){
    printf("Elements are not first on the list to integrate. \n");

#if (defined OPENCL_CPU || defined OPENCL_GPU || defined OPENCL_PHI || defined OPENCL_HSA || OFFLOAD_MIC || CUDA)
    printf("Accelerators do not work in this case.\n");
    nr_elems_accel = 0;
    nr_elems_mic = 0;
    nr_elems_cuda = 0;
#else
    printf("OpenMP is not optimized for that case.\n");
#endif
    exit(-1);
  }
  
#ifndef OFFLOAD_MIC
#pragma omp parallel default(none) firstprivate(nr_elems_accel, nr_elems_mic, nr_elems_cuda, nr_elems, nr_faces ) \
  firstprivate(Problem_id, Level_id, Comp_type, Nr_int_ent, L_int_ent_type, \
	       L_int_ent_id, Max_dofs_int_ent)// L_dof_elem_to_struct,	\
	       L_dof_face_to_struct, L_dof_edge_to_struct,  L_dof_vert_to_struct )
#endif
  {
    
    int my_thread_id = omp_get_thread_num();
    int num_threads = omp_get_num_threads();
    
    int nrdofs_glob, max_nrdofs, nr_dof_ent, nr_levels, posglob, nrdofs_int_ent;
    int l_dof_ent_id[PDC_MAX_DOF_PER_INT], l_dof_ent_nrdof[PDC_MAX_DOF_PER_INT];
    int l_dof_ent_posglob[PDC_MAX_DOF_PER_INT];
    int l_dof_ent_type[PDC_MAX_DOF_PER_INT];
    int i,j,k, iaux, kaux, intent, ibl, ient, nrdofbl, ini_zero;
    int level_id=0;
    char rewrite;
    
    /* allocate memory for the stiffness matrices and RHS */
    max_nrdofs = Max_dofs_int_ent;
    double *stiff_mat = (double *)malloc(max_nrdofs*max_nrdofs*sizeof(double));
    double *rhs_vect = (double *)malloc(max_nrdofs*sizeof(double));
    //double stiff_mat[100];
    //double rhs_vect[10];
    
    memset(stiff_mat, 0, max_nrdofs*max_nrdofs*sizeof(double));
    memset(rhs_vect, 0, max_nrdofs*sizeof(double));
    
/*kbw*/
//#pragma omp critical(printing)
{
  printf("In utr_create_assemble_stiff_mat before loop over entities\n");
  printf("thread_id %d, num_threads %d\n",
	 omp_get_thread_num(),	 omp_get_num_threads() );
}
/*kew*/

    if( nr_elems_accel==0 && nr_elems_mic==0 && nr_elems_cuda==0 || num_threads==1 || (num_threads>1 && my_thread_id!=0) ){
      
      int flag = 0;
      int faces_per_thread;
      int my_first_face;
      int my_last_face;
      
      if( nr_elems_accel==0 && nr_elems_mic==0 && nr_elems_cuda==0){ // no accelerator - all threads compute all matrices
	faces_per_thread = nr_faces / (num_threads);
	my_first_face = nr_elems + (my_thread_id)*faces_per_thread;
	my_last_face = nr_elems + (my_thread_id+1)*faces_per_thread;
	if(my_thread_id==num_threads-1) my_last_face = Nr_int_ent;
      }
      else if( num_threads==1) { // one thread only - it computes all matrices
	faces_per_thread = nr_faces;
	my_first_face = nr_elems;
	my_last_face = Nr_int_ent;
      }     
      else { 
	faces_per_thread = nr_faces / (num_threads-1);
	my_first_face = nr_elems + (my_thread_id-1)*faces_per_thread;
	my_last_face = nr_elems + (my_thread_id)*faces_per_thread;
	if(my_thread_id==num_threads-1) my_last_face = Nr_int_ent;
      }
      
/*kbw
#pragma omp critical(printing)
      {
	printf("In utr_create_assemble_stiff_mat before calling pdr_comp_stiff_mat for faces\n");
	printf("my_first_face %d, last %d, nr_faces %d, thread_id %d (%d), num_threads %d (%d)\n",
	       my_first_face, my_last_face-1, my_last_face-my_first_face,
	       omp_get_thread_num(), 	my_thread_id,  omp_get_num_threads(), num_threads );
      }
/*kew*/

      // loop over faces assigned to a single thread
      for(intent=my_first_face;intent<my_last_face;intent++){
	
	int nrdfobl;
	int idofent;
	int nr_dof_ent = PDC_MAX_DOF_PER_INT;
	int nrdofs_int_ent = max_nrdofs;
	int l_bl_id[PDC_MAX_DOF_PER_INT], l_bl_nrdofs[PDC_MAX_DOF_PER_INT];
       

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ****************************************************************
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//#pragma omp critical(pdr_comp_stiff_mat)
	{
	  pdr_comp_stiff_mat(Problem_id, L_int_ent_type[intent], 
			     L_int_ent_id[intent], Comp_type, NULL,
			     &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdof, 
			     &nrdofs_int_ent, stiff_mat, rhs_vect, &rewrite);
	}
	
	
#ifdef DEBUG_SIM
	if(nrdofs_int_ent>max_nrdofs){
	  printf("Too small arrays stiff_mat and rhs_vect passed to comp_el_stiff_mat\n");
	  printf("from sir_create in sis_mkb. %d < %d. Exiting !!!",
		 max_nrdofs, nrdofs_int_ent);
	  exit(-1);
	}
#endif
       
       
/*kbw
#pragma omp critical(printing)
       {
	 if(l_bl_id[1]==1){
	   //if(L_int_ent_id[intent]!=-1) {
	   int i=6;
	   int solver_id = pdr_ctrl_i_params(Problem_id,i);
	   printf("utr_create_assemble: before assemble: Solver_id %d, level_id %d, sol_typ %d\n", 
		  solver_id, level_id, Comp_type);
	   int ibl,jbl,pli,plj,nri,nrj,nrdof,jaux;
	   printf("ient %d, int_ent_id %d, nr_dof_ent %d\n", 
		  intent, L_int_ent_id[intent], nrdofbl);
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
		   
		   if(stiff_mat[jaux+j]< -1.e20){
		     getchar(); getchar(); getchar();
		   }
		   
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
       }
/*kew*/
    
#pragma omp critical(assembling)
	{
	  pdr_assemble_local_stiff_mat(Problem_id, level_id, Comp_type,
				       nr_dof_ent, l_dof_ent_type,
				       l_dof_ent_id,l_dof_ent_nrdof, 
				       stiff_mat, rhs_vect, &rewrite);
	}
	
	
      } /* end loop over integration entities: intent */
      
    } // end if no accelerator or sequential run or not-master thread

// EXCHANGED FOR BARRIER
//#ifdef OPENCL_CPU
//  } // the end of parallel region - break to free cpu resources
//#endif

  //  BARIER - FOR SEPARATING FACE AND ELEMENT SM ASSEMBLY
#ifdef OPENCL_CPU
  #pragma omp barrier
#endif

    //#ifndef OPENCL_CPU
    if(my_thread_id==0 && nr_elems_accel>0)
    //#endif
    { // master thread sends nr_elems_accel elements
      // for creating and assembling by accelerator

/*kbw
      printf("In utr_create_assemble_stiff_mat in thread %d\n", my_thread_id);
/*kew*/

// WE DEVELOP A GENERIC INTEGRATION ROUTINE FOR DIFFERENT OPENCL DEVICES
// DIFFERENT VERSIONS ARE CREATED USING SWITCHES (SEE BELOW)
// PARTICULAR VERSIONS _GPU, _CPU, _PHI - are considered obsolete, but may
// be used for testing !!!!

      utr_create_assemble_stiff_mat_elem_accel(Problem_id, Level_id, Comp_type,  
					       nr_elems_accel,
					       L_int_ent_type, L_int_ent_id,
					       Max_dofs_int_ent);
      
      
      
    } // end if master thread (controlling OpenCL create-assemble)

    //barrier for mic tests on cpu
#ifdef OFFLOAD_MIC
  #pragma omp barrier
#endif


    if(my_thread_id==0 && nr_elems_mic>0)
	//#endif
	{ // master thread sends nr_elems_accel elements
	  // for creating and assembling by accelerator

/*kbw
	  printf("In utr_create_assemble_stiff_mat in thread %d\n", my_thread_id);
/*kew*/

// WE DEVELOP A GENERIC INTEGRATION ROUTINE FOR DIFFERENT OPENCL DEVICES
// DIFFERENT VERSIONS ARE CREATED USING SWITCHES (SEE BELOW)
// PARTICULAR VERSIONS _GPU, _CPU, _PHI - are considered obsolete, but may
// be used for testing !!!!

	  utr_create_assemble_stiff_mat_elem_mic(Problem_id, Level_id, Comp_type,
						   nr_elems_mic,
						   L_int_ent_type, L_int_ent_id,
						   Max_dofs_int_ent);



	} // end if master thread (controlling OpenCL create-assemble)

//#ifdef CUDA
//  #pragma omp barrier
//#endif

    //#ifndef OPENCL_CPU
     if(my_thread_id==0 && nr_elems_cuda>0)
     //#endif
     { // master thread sends nr_elems_accel elements
       // for creating and assembling by accelerator

 /*kbw
       printf("In utr_create_assemble_stiff_mat in thread %d\n", my_thread_id);
 /*kew*/

 // WE DEVELOP A GENERIC INTEGRATION ROUTINE FOR DIFFERENT OPENCL DEVICES
 // DIFFERENT VERSIONS ARE CREATED USING SWITCHES (SEE BELOW)
 // PARTICULAR VERSIONS _GPU, _CPU, _PHI - are considered obsolete, but may
 // be used for testing !!!!

       utr_create_assemble_stiff_mat_elem_cuda(Problem_id, Level_id, Comp_type,
 					       nr_elems_cuda,
 					       L_int_ent_type, L_int_ent_id,
 					       Max_dofs_int_ent);



     } // end if master thread (controlling OpenCL create-assemble)




    // EXCHANGED FOR BARRIER
/* #ifdef OPENCL_CPU */
/*     //start after break */
/* #pragma omp parallel  default(none) firstprivate(nr_elems_accel, nr_elems, nr_faces ) \ */
/*   firstprivate(Problem_id, Level_id, Comp_type, Nr_int_ent, L_int_ent_type, \ */
/* 	       L_int_ent_id, Max_dofs_int_ent, L_dof_elem_to_struct,	\ */
/* 	       L_dof_face_to_struct, L_dof_edge_to_struct,  L_dof_vert_to_struct ) */
/*   { */


/* 	  int my_thread_id = omp_get_thread_num(); */
/* 	  int num_threads = omp_get_num_threads(); */
/* 	  int max_nrdofs; */
/* 	  int l_dof_ent_id[PDC_MAX_DOF_PER_INT], l_dof_ent_nrdof[PDC_MAX_DOF_PER_INT]; */
/* 	  int l_dof_ent_posglob[PDC_MAX_DOF_PER_INT]; */
/* 	  int l_dof_ent_type[PDC_MAX_DOF_PER_INT]; */
/* 	  int intent, nrdofbl; */
/* 	  int level_id=0; */
/* 	  char rewrite; */

/* 	  /\* allocate memory for the stiffness matrices and RHS *\/ */
/* 	 max_nrdofs = Max_dofs_int_ent; */
/* 	 double *stiff_mat = (double *)malloc(max_nrdofs*max_nrdofs*sizeof(double)); */
/* 	 double *rhs_vect = (double *)malloc(max_nrdofs*sizeof(double)); */
/* 	 //double stiff_mat[100]; */
/* 	 //double rhs_vect[10]; */

/* 	 memset(stiff_mat, 0, max_nrdofs*max_nrdofs*sizeof(double)); */
/* 	 memset(rhs_vect, 0, max_nrdofs*sizeof(double)); */
/* #endif */
    // for elements not processed by accelerators

#ifdef OFFLOAD_MIC
    if( nr_elems_accel==0 && nr_elems_mic==0 && nr_elems_cuda==0 || (num_threads>1 && my_thread_id!=0) ){
#else
    if( nr_elems_accel==0 && nr_elems_mic==0 && nr_elems_cuda==0 || num_threads==1 || (num_threads>1 && my_thread_id!=0) ){
#endif
      int elems_per_thread;
      int my_first_elem;
      int my_last_elem;
      
      if( nr_elems_accel==0 && nr_elems_mic==0 && nr_elems_cuda==0 ){
	elems_per_thread = (nr_elems) / (num_threads);
	my_first_elem = (my_thread_id)*elems_per_thread;
	my_last_elem = (my_thread_id+1)*elems_per_thread;
	if(my_thread_id==num_threads-1) my_last_elem = nr_elems;
      }
      else if(num_threads==1) {
	elems_per_thread = nr_elems - nr_elems_accel;
	my_first_elem = nr_elems_accel;
	my_last_elem = nr_elems;
      }
      else {
	elems_per_thread = (nr_elems - nr_elems_accel - nr_elems_mic - nr_elems_cuda) / (num_threads-1);
	my_first_elem = nr_elems_accel + nr_elems_mic + nr_elems_cuda + (my_thread_id-1)*elems_per_thread;
	my_last_elem = nr_elems_accel + nr_elems_mic + nr_elems_cuda + (my_thread_id)*elems_per_thread;
	if(my_thread_id==num_threads-1) my_last_elem = nr_elems;
      }

/*kbw
#pragma omp critical(printing)
      {
	printf("In utr_create_assemble_stiff_mat before calling pdr_comp_stiff_mat for elems\n");
	printf("my_first_elem %d, last %d, nr_elems %d, thread_id %d (%d), num_threads %d (%d)\n",
	       my_first_elem, my_last_elem-1, my_last_elem-my_first_elem,
	       omp_get_thread_num(), 	my_thread_id,  omp_get_num_threads(), num_threads );
      }
/*kew*/

      // loop over elems assigned to a single thread
      for(intent=my_first_elem;intent<my_last_elem;intent++){

	int nrdfobl;
	int idofent;
	int nr_dof_ent = PDC_MAX_DOF_PER_INT;
	int nrdofs_int_ent = max_nrdofs;
	int l_bl_id[PDC_MAX_DOF_PER_INT], l_bl_nrdofs[PDC_MAX_DOF_PER_INT];
	
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ****************************************************************
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//#pragma omp critical (pdr_comp_stiff_mat)
	{
	  pdr_comp_stiff_mat(Problem_id, L_int_ent_type[intent], 
			     L_int_ent_id[intent], Comp_type, NULL,
			     &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdof, 
			     &nrdofs_int_ent, stiff_mat, rhs_vect, &rewrite);
	}
	
	
#ifdef DEBUG_SIM
	if(nrdofs_int_ent>max_nrdofs){
	  printf("Too small arrays stiff_mat and rhs_vect passed to comp_el_stiff_mat\n");
	  printf("from sir_create in sis_mkb. %d < %d. Exiting !!!",
		 max_nrdofs, nrdofs_int_ent);
	  exit(-1);
	}
#endif
	
	
	
/*kbw
#pragma omp critical(printing)
	{
	  if(L_int_ent_id[intent]!=-1) {
	    printf("utr_create_assemble: before assemble: Solver_id %d, level_id %d, sol_typ %d\n", 
		   Problem_id, level_id, Comp_type);
	    int ibl,jbl,pli,plj,nri,nrj,nrdof,jaux;
	    printf("ient %d, int_ent_id %d, nr_dof_ent %d\n", 
		   intent, L_int_ent_id[intent], nrdofbl);
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
	  }
	}
/*kew*/
    
#pragma omp critical(assembling)
	{
	  pdr_assemble_local_stiff_mat(Problem_id, level_id, Comp_type,
				       nr_dof_ent, l_dof_ent_type,
				       l_dof_ent_id,l_dof_ent_nrdof, 
				       stiff_mat, rhs_vect, &rewrite);
	}
	
	
      } // end loop over elements processed in standard way
      
    } // end if no accelerator or sequential run or not-master thread
    
 
   
    free(stiff_mat);
    free(rhs_vect);
    
  } // the end of parallel region
  
  return(1);
}


/*------------------------------------------------------------
 utr_create_assemble_stiff_mat_elem_accel - to create element stiffness matrices
                                 and assemble them to the global SM using ACCELERATOR
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat_elem_accel(
  int Problem_id, 
  int Level_id, 
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_elems_accel,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
					 )
{

#if (defined OPENCL_CPU || defined OPENCL_GPU || defined OPENCL_PHI || defined OPENCL_HSA)

  pdr_create_assemble_stiff_mat_opencl_elem(Problem_id, Level_id, Comp_type,  
					    Nr_elems_accel,
					    L_int_ent_type, L_int_ent_id,
					    Max_dofs_int_ent);

#endif

  return(0);

}


/*------------------------------------------------------------
 utr_create_assemble_stiff_mat_elem_mic - to create element stiffness matrices
                                 and assemble them to the global SM using Intel MIC architecture
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat_elem_mic(
  int Problem_id,
  int Level_id,
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_elems_mic,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
					 )
{

#if (defined OFFLOAD_MIC)

  pdr_create_assemble_stiff_mat_mic_elem(Problem_id, Level_id, Comp_type,
					    Nr_elems_mic,
					    L_int_ent_type, L_int_ent_id,
					    Max_dofs_int_ent);

#endif

  return(0);

}


/*------------------------------------------------------------
 utr_create_assemble_stiff_mat_cuda - to create element stiffness matrices
                                 and assemble them to the global SM
------------------------------------------------------------*/
int utr_create_assemble_stiff_mat_elem_cuda(
  int Problem_id,
  int Level_id,
  int Comp_type,         /* in: indicator for the scope of computations: */
  //extern const int PDC_NO_COMP  ; /* do not compute stiff matrix and rhs vector */
  //extern const int PDC_COMP_SM  ; /* compute entries to stiff matrix only */
  //extern const int PDC_COMP_RHS ; /* compute entries to rhs vector only */
  //extern const int PDC_COMP_BOTH; /* compute entries for sm and rhsv */
  int Nr_elems_cuda,
  int* L_int_ent_type,
  int* L_int_ent_id,
  int Max_dofs_int_ent
					 )
{
#if (defined CUDA)

	pdr_create_assemble_stiff_mat_cuda_elem(Problem_id, Level_id, Comp_type,
						    Nr_elems_cuda,
						    L_int_ent_type, L_int_ent_id,
						    Max_dofs_int_ent);

#endif

	return(0);

}
