/**********************************************************************
File sis_pardiso_intf.c - implementation of the linear solver interface
for the PARDISO solver (based on sid_lapack)

Contains:

Interface routines:
sir_direct_module_introduce - to return the solver name
sir_direct_solve_lin_sys - to perform the five steps below in one call
sir_direct_init - to create a new solver instance and read its control parameters
sir_direct_create - to create and initialize solver data structure
sir_direct_solve - to solve the system for a given data
sir_direct_free - to free memory for a given solver instance
sir_direct_destroy - to destroy a solver instance

Local procedures:
sir_direct_assemble_stiff_mat - to assemble entries to the global stiffness matrix
and the global load vector using the provided local
stiffness matrix and load vector
------------------------------
History:
02.2002 - Krzysztof Banas, initial version (lapack)
09.2011 - Przemek Plaszewski
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#include<vector>
#include<map>

/* problem dependent interface between approximation and solver modules */
#include "pdh_intf.h"

/* interface for general purpose linear solver modules */
#include "sih_intf.h"

/* internal information for the PARDISO linear solver module */
#include "sih_pardiso.h"

/* LAPACK procedures */
//#include "lin_alg_intf.h"

//#include "pomiar_czasu.h"

#include "mkl_solver.h"
#include "mkl_spblas.h"

#include "uth_log.h"
#include "uth_intf.h"

using namespace std;


/*** GLOBAL VARIABLES (for the solver module) ***/
int   siv_direct_nr_solvers = 0;    /* the number of solvers in the problem */
int   siv_direct_cur_solver_id;                /* ID of the current problem */
sit_direct_solver siv_direct_solvers[SIC_MAX_NUM_SOLV];   /* array of solvers */
sit_pardiso_config siv_pardiso_config; //PARDISO control data
bool isPardisoConfigInitialized = false;


/* utility procedure */
#define SIC_LIST_END_MARK -1 /* number marking the end of list - cannot be put */
int sir_direct_put_list( /* returns*/
    /*  >0 - position already occupied on the list */
    /*  <0 - position at which put on the list */
    /*   0 - list full, not found on the list */
    int Num, 	/* in: number to put on the list */
    int* List, 	/* in: list */
    int Ll		/* in: total list's lengths */
);

int sir_direct_solve_internal( /* returns: >=0 - success code, <0 - error code */
    int Solver_id,     /* in: solver identification */
    int Comp_type,     /* in: indicator for the scope of computations: */
    /*   SIC_SOLVE - solve the system */
    /*   SIC_RESOLVE - resolve for the new right hand side */
    int Monitor       /* in: monitoring flag with options: */
    /*   SIC_PRINT_NOT - do not print anything */
    /*   SIC_PRINT_ERRORS - print error messages only */
    /*   SIC_PRINT_INFO - print most important information */
    /*   SIC_PRINT_ALLINFO - print all available information */
) ;

/*------------------------------------------------------------
sir_direct_module_introduce - to return the solver name
------------------------------------------------------------*/
int sir_direct_module_introduce( /* returns: >=0 - success code, <0 - error code */
    char* Solver_name  /* out: the name of the solver */
)
{
    char* string = "PARDISO";

    strcpy(Solver_name,string);

    return(1);
}

/*------------------------------------------------------------
sir_direct_solve_lin_sys - to solve the system of linear equations for the current
setting of parameters (with initialization and finalization phases)
------------------------------------------------------------*/
int sir_direct_solve_lin_sys( /* returns: >=0 - success code, <0 - error code */
    int Problem_id,    /* ID of the problem associated with the solver */
    int Parallel,      /* parameter specifying sequential (SIC_SEQUENTIAL) */
    /* or parallel (SIC_PARALLEL) execution */
    char* Filename  /* in: name of the file with control parameters */
)
{


    int comp_type, solver_id, monitor, ini_guess;
//	char solver_name[100];

    /*++++++++++++++++ executable statements ++++++++++++++++*/

    /*kbw
    printf("In sir_direct_solve_lin_sys before init: problem_id %d\n",
    problem_id);
    /*kew*/

    /* initialize the solver - empty data, nothing is read by LAPACK module */
    solver_id = sir_direct_init(Parallel, Filename);

    /*kbw
    printf("In sir_direct_solve_lin_sys before create: solver_id %d, problem_id %d\n",
    solver_id, Problem_id);
    /*kew*/

    /* create the solver data structure and asociate it with a given problem */
    sir_direct_create(solver_id, Problem_id);

    /*kbw
    printf("In sir_direct_solve_lin_sys before solve: solver_id %d, problem_id %d\n",
    solver_id, Problem_id);
    /*kew*/

    /* launch the solver */
    comp_type = SIC_SOLVE;
    monitor = SIC_PRINT_NOT;
    sir_direct_solve_internal(solver_id,comp_type,monitor);

    /*kbw
    printf("In sir_direct_solve_lin_sys before free: solver_id %d, problem_id %d\n",
    solver_id, Problem_id);
    /*kew*/

    /* free the solver data structure - together with renumbering data */
    sir_direct_free(solver_id);

    /*kbw
    printf("In sir_direct_solve_lin_sys before destroy: solver_id %d, problem_id %d\n",
    solver_id, Problem_id);
    /*kew*/

    /* destroy the solver */
    sir_direct_destroy(solver_id);

    return(0);
}


/*------------------------------------------------------------
sir_direct_init - to create a new solver and read its control parameters
------------------------------------------------------------*/
int sir_direct_init( /* returns: >0 - solver ID, <0 - error code */
    int Parallel,      /* parameter specifying sequential (SIC_SEQUENTIAL) */
    /* or parallel (SIC_PARALLEL) execution */
    char* Filename  /* in: name of the file with control parameters */
)
{
    //char pardisoFileName[300];
#ifdef PARALLEL
    // KM 11.2013:
    // as far as I know, sid_pardiso is not supporting execution in distributed memory model.
    mf_log_warn("Using direct solver in distributed memory environment!");
    printf("PARDISO not adapted to parallel (distributed memory) execution. Exiting!\n");
    exit(-1);
#endif
    /* increase the counter for solvers */
    siv_direct_nr_solvers++;

    /* set the current solver ID */
    siv_direct_cur_solver_id = siv_direct_nr_solvers;

    if(!isPardisoConfigInitialized) siv_pardiso_config.setDefaults();
    isPardisoConfigInitialized = true;

    //set configuration data
    // if(!isPardisoConfigInitialized)
    //   {
    // 	siv_pardiso_config.setDefaults();
    // 	//sprintf(pardisoFileName,"%s/pardiso.dat",Filename);
    // 	//sprintf(pardisoFileName,"pardiso.dat");
    // 	printf("Opening PARDISO config file %s\n",Filename);
    // 	if(!readPardisoConfig(siv_pardiso_config,Filename))
    // 	  {
    // 	    siv_pardiso_config.setDefaults();
    // 	    printf("PARDISO config file (filename:%s) missing or corrupted - using defaults !!!\n",Filename);
    // 	  }
    // 	isPardisoConfigInitialized = true;
    //   }
    
    return(siv_direct_cur_solver_id);
}


/*------------------------------------------------------------
sir_direct_create - to create solver's data structure
------------------------------------------------------------*/
int sir_direct_create( /* returns: >0 - solver ID, <0 - error code */
    int Solver_id,    /* in: solver identification */
    int Problem_id    /* ID of the problem associated with the solver */
)
{

    /* pointer to solver structure */
    sit_direct_solver *solver_p;
    /* pointer to dofs structure */
    sit_direct_dof_struct *dof_struct_p=NULL;

    /* the number of (different) mesh entities for which entries to the global */
    /* stiffness matrix and load vector will be provided */
    int nr_int_ent, nr_dof_ent, max_dofs_per_dof_ent, max_dofs_int_ent;
    /* the global number of degrees of freedom */
    int nrdofs_glob, int_ent_id, nr_dof_ent_loc, idofent, dof_ent_id=0, pos_glob;
    int nr_dof_struct, dof_struct_id, dof_ent_type;
    int* temp_list_dof_type;
    int* temp_list_dof_id;
    int* temp_list_dof_nrdofs;
    int l_dof_ent_types[SIC_MAX_DOF_PER_INT], l_dof_ent_ids[SIC_MAX_DOF_PER_INT];
    int l_dof_ent_nrdofs[SIC_MAX_DOF_PER_INT];

    /* auxiliary variables */
    int i, idof, iaux, ient, ineig;

    /*++++++++++++++++ executable statements ++++++++++++++++*/


    /* get pointer to solver structure */
    solver_p = sir_direct_select_solver(Solver_id);
    solver_p->problem_id = Problem_id;

    /*kbw
    printf("In sir_direct_create: solver_id %d, problem_id %d\n",
    Solver_id, solver_p->problem_id);
    /*kew*/

    /* prepare renumbering arrays */
    /*------------------------------------------------------------
      pdr_get_list_int_ent - to return the list of integration entities - entities
                             for which stiffness matrices and load vectors are
                             provided by the FEM code to the solver module
    ------------------------------------------------------------*/
    pdr_get_list_ent(
        solver_p->problem_id,
        &nr_int_ent, /* out: number of integration entitites */
        /* number of integration entities - entities that */
        /* provide solver with stiffness matrices and load vectors*/
        &solver_p->l_int_ent_type,/* out: list of types of integration entitites */
        /*list of types of entities providing local SMs and LVs */
        &solver_p->l_int_ent_id, /* out: list of IDs of integration entitites */
        /* list of ID's of entities providing local SMs and LVs */
        &nr_dof_ent,/* out: number of dof entities (entities with which there
		              are dofs associated by the given approximation) */
        /* number of dof entities - mesh entities with which */
        /* degrees of freedom are associated */
        &temp_list_dof_type, /* out: list of types of integration entitites */
        &temp_list_dof_id,  /* out: list of IDs of integration entitites */
        &temp_list_dof_nrdofs, /* out: list of no of dofs for 'dof' entity */
        &nrdofs_glob, /* out: global number of degrees of freedom (unknowns) */
        &max_dofs_per_dof_ent); /* out: maximal number of dofs per dof entity */


    /*kbw
    printf("In sir_direct_create after pdr_get_list_ent\n");
    printf("nr_int_ent %d\n", nr_int_ent);
    for(i=0;i<nr_int_ent;i++)  printf("type %d, id %d\n",
    solver_p->l_int_ent_type[i],solver_p->l_int_ent_id[i]);
    printf("\nNr_dof_ent %d, Nrdofs_glob %d, Max_dofs_per_ent %d\n",
    nr_dof_ent, nrdofs_glob, max_dofs_per_dof_ent);
    for(i=0;i<nr_dof_ent;i++)  printf("type %d, id %d, nrdofs %d\n",
    temp_list_dof_type[i], temp_list_dof_id[i],
    temp_list_dof_nrdofs[i]);
    /*kew*/

    solver_p->nr_int_ent = nr_int_ent;
    solver_p->nr_dof_ent = nr_dof_ent;
    solver_p->nrdofs_glob = nrdofs_glob;

    /* array of structures storing DOF data */
    solver_p->l_dof_struct =
        (sit_direct_dof_struct *)malloc( nr_dof_ent*sizeof(sit_direct_dof_struct) );

    /* dof_ent_index to dof_struct_index (based on which dof_ent_id and */
    /* dof_ent_type can be find */
    solver_p->max_dof_vert_id = -1;
    solver_p->max_dof_edge_id = -1;
    solver_p->max_dof_face_id = -1;
    solver_p->max_dof_elem_id = -1;

    //Find number of element, face, edge, vertex entities
    for (i=0; i< nr_dof_ent; i++) {
        if (temp_list_dof_type[i] == PDC_ELEMENT ) {
            if ( temp_list_dof_id[i] > solver_p->max_dof_elem_id )
                solver_p->max_dof_elem_id = temp_list_dof_id[i];
        } else if (temp_list_dof_type[i] == PDC_FACE ) {
            if ( temp_list_dof_id[i] > solver_p->max_dof_face_id )
                solver_p->max_dof_face_id = temp_list_dof_id[i];
        } else if (temp_list_dof_type[i] == PDC_EDGE ) {
            if ( temp_list_dof_id[i] > solver_p->max_dof_edge_id )
                solver_p->max_dof_edge_id = temp_list_dof_id[i];
        } else if (temp_list_dof_type[i] == PDC_VERTEX ) {
            if ( temp_list_dof_id[i] > solver_p->max_dof_vert_id )
                solver_p->max_dof_vert_id = temp_list_dof_id[i];
        } else {
            printf("Error 87373732 in sis_pardiso_intf/sir_direct_create!!! Exiting\n");
            exit(-1);
        }


    }

    //allocate solver_p->l_dof_..._to_struct tables
    if (solver_p->max_dof_elem_id>=0) {
        solver_p->l_dof_elem_to_struct=
            (int*)malloc((solver_p->max_dof_elem_id+1)*sizeof(int));
        for (i=0;i<=solver_p->max_dof_elem_id;i++)
            solver_p->l_dof_elem_to_struct[i] = -1;
    }
    if (solver_p->max_dof_face_id>=0) {
        solver_p->l_dof_face_to_struct=
            (int*)malloc((solver_p->max_dof_face_id+1)*sizeof(int));
        for (i=0;i<=solver_p->max_dof_face_id;i++)
            solver_p->l_dof_face_to_struct[i] = -1;
    }
    if (solver_p->max_dof_edge_id>=0) {
        solver_p->l_dof_edge_to_struct=
            (int*)malloc((solver_p->max_dof_edge_id+1)*sizeof(int));
        for (i=0;i<=solver_p->max_dof_edge_id;i++)
            solver_p->l_dof_edge_to_struct[i] = -1;
    }
    if (solver_p->max_dof_vert_id>=0) {
        solver_p->l_dof_vert_to_struct=
            (int*)malloc((solver_p->max_dof_vert_id+1)*sizeof(int));
        for (i=0;i<=solver_p->max_dof_vert_id;i++)
            solver_p->l_dof_vert_to_struct[i] = -1;
    }


    /* in case of no renumbering global position of dof block is specified */
    pos_glob = 0;
    for (idof = 0; idof < nr_dof_ent; idof++) {

        dof_struct_p = &solver_p->l_dof_struct[idof];

        nr_dof_ent_loc = temp_list_dof_nrdofs[idof];
        dof_struct_p->dof_ent_type = temp_list_dof_type[idof];
        dof_struct_p->dof_ent_id = temp_list_dof_id[idof];
        dof_struct_p->nrdofs = nr_dof_ent_loc;
        dof_struct_p->posglob = pos_glob;
        pos_glob += nr_dof_ent_loc;

        /* initialize lists of integration entities and neighbouring dof_ent */
        dof_struct_p->nr_int_ent = 0;
        for (i=0;i<SIC_MAX_INT_PER_DOF;i++)
            dof_struct_p->l_int_ent_index[i]=SIC_LIST_END_MARK;
        dof_struct_p->nrneig = 0;
        for (i=0;i<SIC_MAX_DOF_STR_NGB;i++)
            dof_struct_p->l_neig[i]=SIC_LIST_END_MARK;

        /* arrays dof_ent to dof_struct */
        if (dof_struct_p->dof_ent_type == PDC_ELEMENT ) {
            solver_p->l_dof_elem_to_struct[dof_struct_p->dof_ent_id] = idof;

#ifdef DEBUG_SIM
            if (dof_struct_p->dof_ent_id > solver_p->max_dof_elem_id) {
                printf("Error 84543732 in sis_pardiso_intf/sir_direct_create!!! Exiting\n");
                printf("%d > %d\n", dof_struct_p->dof_ent_id, solver_p->max_dof_elem_id);
                exit(-1);
            }
#endif

        } else if (dof_struct_p->dof_ent_type == PDC_FACE ) {
            solver_p->l_dof_face_to_struct[dof_struct_p->dof_ent_id] = idof;

#ifdef DEBUG_SIM
            if (dof_struct_p->dof_ent_id > solver_p->max_dof_face_id) {
                printf("Error 84543732 in sis_pardiso_intf/sir_direct_create!!! Exiting\n");
                printf("%d > %d\n", dof_struct_p->dof_ent_id, solver_p->max_dof_face_id);
                exit(-1);
            }
#endif

        } else if (dof_struct_p->dof_ent_type == PDC_EDGE ) {
            solver_p->l_dof_edge_to_struct[dof_struct_p->dof_ent_id] = idof;

#ifdef DEBUG_SIM
            if (dof_struct_p->dof_ent_id > solver_p->max_dof_edge_id) {
                printf("Error 84543732 in sis_pardiso_intf/sir_direct_create!!! Exiting\n");
                printf("%d > %d\n", dof_struct_p->dof_ent_id, solver_p->max_dof_edge_id);
                exit(-1);
            }
#endif

        } else if (dof_struct_p->dof_ent_type == PDC_VERTEX ) {
            solver_p->l_dof_vert_to_struct[dof_struct_p->dof_ent_id] = idof;
#ifdef DEBUG_SIM
            if (dof_struct_p->dof_ent_id > solver_p->max_dof_vert_id) {
                printf("Error 84543732 in sis_pardiso_intf/sir_direct_create!!! Exiting\n");
                printf("%d > %d\n", dof_struct_p->dof_ent_id, solver_p->max_dof_vert_id);
                exit(-1);
            }
#endif
        } else {
            printf("Error 8963732 in sis_lapack_intf/sir_direct_create!!! Exiting\n");
            exit(-1);
        }




        /*kbw
        printf("In sir_direct_create after filling dof_struct no %d\n", idof);
        printf("dof_ent_type %d, dof_ent_id %d, nrdofs %d, posglob %d\n",
        dof_struct_p->dof_ent_type , dof_struct_p->dof_ent_id,
        dof_struct_p->nrdofs, dof_struct_p->posglob);
        printf("Initialized lists of int_ent %d, neig %d\n",
        dof_struct_p->l_int_ent_index[0], dof_struct_p->l_neig[0]);
        /*kew*/

    }

#ifdef DEBUG_SIM
    if ( solver_p->nrdofs_glob != pos_glob ) {
        printf("Error 843732 in sis_pardiso_intf/sir_direct_create!!! Exiting\n");
        exit(-1);
    }
#endif

    /* getting information on the structure of the global stiffness matrix */
    nr_dof_struct = 0;
    max_dofs_int_ent = 0;
    for (ient=0; ient<nr_int_ent;ient++) {

        int nrdofs_int_ent = 0;
        int_ent_id = solver_p->l_int_ent_id[ient];
        nr_dof_ent_loc = SIC_MAX_DOF_PER_INT;

        pdr_comp_stiff_mat(
            solver_p->problem_id,
            solver_p->l_int_ent_type[ient],
            solver_p->l_int_ent_id[ient],
            PDC_NO_COMP,
            0,
            &nr_dof_ent_loc,
            l_dof_ent_types,
            l_dof_ent_ids,
            l_dof_ent_nrdofs,
            NULL,
            NULL,
            NULL,
            NULL);


        /*kbw
        printf("in sir_direct_create after pdr_comp_stiff_mat for int_ent no %d (id %d):\n",
        ient, int_ent_id);
        printf("nr_dof_ent_loc %d, types, ids, nrdofs:\n", nr_dof_ent_loc);
        for(idofent=0; idofent<nr_dof_ent_loc; idofent++){
        printf("%d %d %d\n", l_dof_ent_types[idofent],
        l_dof_ent_ids[idofent], l_dof_ent_nrdofs[idofent]);
        }
        /*kew*/

        for (idofent=0; idofent<nr_dof_ent_loc; idofent++) {

            dof_ent_id = l_dof_ent_ids[idofent];
            dof_ent_type = l_dof_ent_types[idofent];

            if (dof_ent_type == PDC_ELEMENT ) {

                dof_struct_id = solver_p->l_dof_elem_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
                if ( solver_p->l_dof_elem_to_struct[dof_ent_id] == -1) {
                    printf("Error 3472941 in sir_direct_create!!! Exiting\n");
                    exit(-1);
                }
#endif

            } else if (dof_ent_type == PDC_FACE ) {

                dof_struct_id = solver_p->l_dof_face_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
                if ( solver_p->l_dof_face_to_struct[dof_ent_id] == -1) {
                    printf("Error 3472942 in sir_direct_create!!! Exiting\n");
                    exit(-1);
                }
#endif

            } else if (dof_ent_type == PDC_EDGE ) {

                dof_struct_id = solver_p->l_dof_edge_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
                if ( solver_p->l_dof_edge_to_struct[dof_ent_id] == -1) {
                    printf("Error 3472943 in sir_direct_create!!! Exiting\n");
                    exit(-1);
                }
#endif

            } else if (dof_ent_type == PDC_VERTEX ) {

                dof_struct_id = solver_p->l_dof_vert_to_struct[dof_ent_id];

#ifdef DEBUG_SIM
                if ( solver_p->l_dof_vert_to_struct[dof_ent_id] == -1) {
                    printf("Error 3472944 in sir_direct_create!!! Exiting\n");
                    exit(-1);
                }
#endif

            } else {
                printf("Error 34fsf7294 in sir_direct_create!!! Exiting\n");
                exit(-1);
            }

            dof_struct_p = &solver_p->l_dof_struct[dof_struct_id];

            /*kbw
            printf("for int_type %d, int_id %d, dof_type %d, dof_id %d, struct %d\n",
            solver_p->l_int_ent_type[ient], solver_p->l_int_ent_id[ient],
            l_dof_ent_types[idofent], l_dof_ent_ids[idofent],dof_struct_id );
            /*kew*/

#ifdef DEBUG_SIM
            if ((dof_struct_p->dof_ent_id != dof_ent_id) ||
                    (dof_struct_p->dof_ent_type != dof_ent_type) ||
                    (dof_struct_p->nrdofs != l_dof_ent_nrdofs[idofent]) ) {
                printf("Error 3827 in sir_direct_create!!! Exiting");
                exit(-1);
            }
#endif

            nrdofs_int_ent += l_dof_ent_nrdofs[idofent];

            /*kbw
            printf("putting int_ent no %d on the list of int_ent, nr_int_ent %d\n",
            ient, nr_int_ent);
            printf("before:");
            for(i=0;i<SIC_MAX_INT_PER_DOF;i++) {
            printf("%d",dof_struct_p->l_int_ent_index[i]) ;
            }
            printf("\n");
            /*kew*/

            /*       iaux=sir_direct_put_list(ient,  */
            /* 			dof_struct_p->l_int_ent_index, SIC_MAX_INT_PER_DOF); */
            /*       if(iaux<0) dof_struct_p->nr_int_ent++; */

            /* #ifdef DEBUG_SIM */
            /* 	  if(iaux == 0){ // list full - increase SIC_MAX_INT_PER_DOF */
            /* 	    printf("Error 383627 in sir_direct_create!!! Exiting"); */
            /* 	    exit(-1); */
            /* 	  } */
            /* #endif       */

            /*kbw
            printf("putting int_ent no %d on the list of int_ent, nr_int_ent %d\n",
            ient, nr_int_ent);
            printf("after:");
            for(i=0;i<SIC_MAX_INT_PER_DOF;i++) {
            printf("%d",dof_struct_p->l_int_ent_index[i]) ;
            }
            printf("\n");
            /*kew*/

            for (ineig = 0; ineig<nr_dof_ent_loc; ineig++) {

                if (ineig != idofent) {

                    int neig_id = l_dof_ent_ids[ineig];
                    int neig_type = l_dof_ent_types[ineig];
                    int neig_index = 0;
                    if (neig_type==PDC_ELEMENT) {
                        neig_index = solver_p->l_dof_elem_to_struct[neig_id];
                    } else if (neig_type==PDC_FACE) {
                        neig_index = solver_p->l_dof_face_to_struct[neig_id];
                    } else if (neig_type==PDC_EDGE) {
                        neig_index = solver_p->l_dof_edge_to_struct[neig_id];
                    } else if (neig_type==PDC_VERTEX) {
                        neig_index = solver_p->l_dof_vert_to_struct[neig_id];
                    }

                    /*kbw
                    printf("dof_ent %d: putting ineig no %d (id %d) on the list of neig, nrneig %d\n",
                    idofent, ineig, neig_id, dof_struct_p->nrneig);
                    printf("before:");
                    for(i=0;i<SIC_MAX_DOF_STR_NGB;i++) {
                    printf("%d",dof_struct_p->l_neig[i]) ;
                    }
                    printf("\n");
                    /*kew*/

                    iaux=sir_direct_put_list(neig_index,
                                      dof_struct_p->l_neig, SIC_MAX_DOF_STR_NGB);
                    if (iaux<0) {

                        dof_struct_p->nrneig++;

                    }

#ifdef DEBUG_SIM
                    if (iaux == 0) { // list full - increase SIC_MAX_DOF_STR_NGB
                        printf("Error 385627 in sir_direct_create!!! Exiting");
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

        if (nrdofs_int_ent > max_dofs_int_ent) max_dofs_int_ent = nrdofs_int_ent;

    } /* end loop over int_ent */

    solver_p->max_dofs_int_ent = max_dofs_int_ent;

    // PLACE FOR CALL TO RENUMBERING PROCEDURE

#ifdef DEBUG_SIM
    printf("\nEntering PARDISO solver for linear equations\n");
    printf("Problem size %d degrees of freedom\n"
           ,nrdofs_glob);
#endif

    utr_io_result_add_value_int(RESULT_N_DOFS,nrdofs_glob);


#ifdef DEBUG_SIM
    /*kbw
    printf("Before allocating global SM and RHS: nrdofs_glob %d\n",
    nrdofs_glob);
    printf("solver_p->stiff_mat %lu, solver_p->rhs_vect %lu, solver_p->sol_vect %lu, aux_vect %lu\n",
    solver_p->stiff_mat,solver_p->rhs_vect,solver_p->sol_vect, solver_p->aux_vect);
    /*kew*/
#endif

    //solver_p->stiff_mat = (double *)malloc(nrdofs_glob*nrdofs_glob*sizeof(double));

    solver_p->rhs_vect = (double *)malloc(nrdofs_glob*sizeof(double));

    solver_p->sol_vect = (double *)malloc(nrdofs_glob*sizeof(double));

    solver_p->aux_vect = (int *)malloc(nrdofs_glob*sizeof(int));

#ifdef DEBUG_SIM
    /*kbw
    printf("After allocating global SM and RHS: nrdofs_glob %d\n",
    nrdofs_glob);
    printf("solver_p->stiff_mat %lu, solver_p->rhs_vect %lu, solver_p->sol_vect %lu, aux_vect %lu\n",
    solver_p->stiff_mat,solver_p->rhs_vect,solver_p->sol_vect, solver_p->aux_vect);
    /*kew*/
#endif



    //for(i=0;i<nrdofs_glob*nrdofs_glob;i++) solver_p->stiff_mat[i] = 0.0;
    for (i=0;i<nrdofs_glob;i++) solver_p->rhs_vect[i] = 0.0;
    for (i=0;i<nrdofs_glob;i++) solver_p->sol_vect[i] = 0.0;
    for (i=0;i<nrdofs_glob;i++) solver_p->aux_vect[i] = 0;

    if (solver_p->aux_vect==NULL) {
#ifdef DEBUG_SIM
        printf("Not enough memory for PARDISO solver\n");
#endif
        exit(-1);
    }

    /*kbw
    printf("In sir_direct_init befor leaving: nr_int_ent %d, max_nrdofs %d\n",
    solver_p->nr_int_ent, solver_p->max_dofs_int_ent);
    for(i=0;i<solver_p->nr_dof_ent;i++){
    printf("block %d, nrdofs %d, posglob %d\n",
    i, solver_p->l_dof_struct[i].nrdofs,
    solver_p->l_dof_struct[i].posglob);
    }
    /*kew*/


    /* no additional parameters read from input file Filename */

    free(temp_list_dof_type);
    free(temp_list_dof_id);
    free(temp_list_dof_nrdofs);

    return(0);
}


/*------------------------------------------------------------
sir_direct_solve - to solve the system for a given data
------------------------------------------------------------*/
int sir_direct_solve( /* returns: >=0 - success code, <0 - error code */
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
    double *Conv_rate  /* out (optional): convergence rate */
)
{

  printf("sir_direct_solve not implemented for PARDISO!\nExiting!\n");
  exit(-1);

  //KB old sir_direct_solve not working as it should put as sir_direct_solve_internal
  //char arg[300]; /* string variable to read menu */
  //sprintf(arg,"./pardiso.dat");
  //sir_direct_solve_lin_sys(Solver_id, SIC_SEQUENTIAL, "");
  
  //sir_direct_solve_internal(Solver_id, Comp_type, Monitor);
  
  return(0);
}

/*------------------------------------------------------------
sir_direct_solve_internal - to solve the system for a given data
------------------------------------------------------------*/
int sir_direct_solve_internal( /* returns: >=0 - success code, <0 - error code */
    int Solver_id,     /* in: solver identification */
    int Comp_type,     /* in: indicator for the scope of computations: */
    /*   SIC_SOLVE - solve the system */
    /*   SIC_RESOLVE - resolve for the new right hand side */
    int Monitor       /* in: monitoring flag with options: */
    /*   SIC_PRINT_NOT - do not print anything */
    /*   SIC_PRINT_ERRORS - print error messages only */
    /*   SIC_PRINT_INFO - print most important information */
    /*   SIC_PRINT_ALLINFO - print all available information */
)
{

    /* pointer to solver structure */
    sit_direct_solver *solver_p;

    /* auxiliary variables */
    int nrdofs_glob, max_nrdofs, nr_int_ent, nr_dof_ent;
    int l_dof_ent_id[SIC_MAX_DOF_PER_INT], l_dof_ent_nrdofs[SIC_MAX_DOF_PER_INT];
    int l_dof_ent_posglob[SIC_MAX_DOF_PER_INT];
    int l_dof_ent_type[SIC_MAX_DOF_PER_INT];
    int pdr_comp_type;
    double *stiff_mat;
    double *rhs_vect;
    int i,j, iaux, kaux, intent, idofent, ibl;
    const int level_id=0;
    int nrdofs_int_ent;

    /* Setup PARDISO control parameters.*/
    int nrhs = 1; /* Number of right hand sides. */
    double ddum; /* Double dummy */
    int idum; /* Integer dummy. */
    int phase;

    void *pt[64]; /* Internal solver memory pointer pt, */
    for (i = 0; i < 64; ++i) {
        pt[i] = 0;
    }

    int* iparm = (int*) malloc(64 * sizeof(int)); /* Pardiso control parameters.*/
    for (i = 0; i < 64; ++i) {
        iparm[i] = 0;
    }

    int mtype = siv_pardiso_config.mtype;
    int maxfct = siv_pardiso_config.maxfct;
    int mnum = siv_pardiso_config.mnum;
    int msglvl = siv_pardiso_config.msglvl;
    int error = 0;

    // iparm[0] = siv_pardiso_config.iparm1;
    // iparm[1] = siv_pardiso_config.iparm2; //fill-in reducing ordering.
    // iparm[2] = siv_pardiso_config.iparm3; //currently is not used.
    // iparm[3] = siv_pardiso_config.iparm4; //preconditioned CGS.
    // iparm[4] = siv_pardiso_config.iparm5; //user permutation.
    // iparm[5] = siv_pardiso_config.iparm6; //write solution on x.

    // iparm[7] = siv_pardiso_config.iparm8; //iterative refinement step.
    // iparm[8] = siv_pardiso_config.iparm9; //must be set to 0.
    // iparm[9] = siv_pardiso_config.iparm10; //pivoting perturbation.
    // iparm[10] = siv_pardiso_config.iparm11; //scaling vectors.
    // iparm[11] = siv_pardiso_config.iparm12; //must be set to 0.
    // iparm[12] = siv_pardiso_config.iparm13; //improved accuracy using (non-)symmetric weighted matchings.

    // iparm[17] = siv_pardiso_config.iparm18; //-1
    // iparm[18] = siv_pardiso_config.iparm19; //MFlops of factorization.

    // iparm[20] = siv_pardiso_config.iparm21; //pivoting for symmetric indefinite matrices.

    // iparm[26] = siv_pardiso_config.iparm27; //matrix checker. default is 0
    // iparm[27] = siv_pardiso_config.iparm28; //single or double precision of PARDISO.
    // iparm[59] = siv_pardiso_config.iparm60; //version of PARDISO.
    /*END PARDISO setup*/

    if(msglvl == 1)
    {
    	printf("PARDISO parameters:\n");
    	printf("\tmtype: %i \n", mtype);
    	printf("\tmsglvl: %i \n", msglvl);
    	printf("\tiparm1: %i \n", iparm[0]);
    	printf("\tiparm2: %i \n", iparm[1]);
    	printf("\tiparm3: %i \n", iparm[2]);
    	printf("\tiparm4: %i \n", iparm[3]);
    	printf("\tiparm5: %i \n", iparm[4]);
    	printf("\tiparm6: %i \n", iparm[5]);
    	printf("\tiparm8: %i \n", iparm[7]);
    	printf("\tiparm9: %i \n", iparm[8]);
    	printf("\tiparm10: %i \n", iparm[9]);
    	printf("\tiparm11: %i \n", iparm[10]);
    	printf("\tiparm12: %i \n", iparm[11]);
    	printf("\tiparm13: %i \n", iparm[12]);
    	printf("\tiparm18: %i \n", iparm[17]);
    	printf("\tiparm19: %i \n", iparm[18]);
    	printf("\tiparm21: %i \n", iparm[20]);
    	printf("\tiparm27: %i \n", iparm[26]);
    	printf("\tiparm28: %i \n", iparm[27]);
    	printf("\tiparm60: %i \n", iparm[59]);
    }


    /*++++++++++++++++ executable statements ++++++++++++++++*/


    /* get pointer to solver structure */
    solver_p = sir_direct_select_solver(Solver_id);

    /* global number of degrees of freedom */
    nrdofs_glob = solver_p->nrdofs_glob;
    max_nrdofs = solver_p->max_dofs_int_ent;
    nr_int_ent = solver_p->nr_int_ent;
    int n = solver_p->nrdofs_glob;


    /* storage for local stiffness matrices  */
    stiff_mat = (double *)malloc(max_nrdofs*max_nrdofs*sizeof(double));
    rhs_vect = (double *)malloc(max_nrdofs*sizeof(double));


    if (Comp_type==SIC_SOLVE) pdr_comp_type = PDC_COMP_BOTH;
    else pdr_comp_type = PDC_COMP_RHS;

    std::vector<std::map<int, double> > dov_matrix(n);

    /* compute local stiffness matrices */
    for (intent=0;intent<nr_int_ent;intent++) {
        nr_dof_ent = SIC_MAX_DOF_PER_INT; /* in: size of arrays, */
        /* out: no of filled entries */
        nrdofs_int_ent = max_nrdofs;   /* in: size of arrays, */
        /* out: no of filled entries */
        pdr_comp_stiff_mat(solver_p->problem_id, solver_p->l_int_ent_type[intent],
                           solver_p->l_int_ent_id[intent], pdr_comp_type, 0,
                           &nr_dof_ent,l_dof_ent_type,l_dof_ent_id,l_dof_ent_nrdofs,
                           &nrdofs_int_ent, stiff_mat, rhs_vect, NULL);

		/*pcw/
		printf("\n");
		printf("Before Asembling (only diagonal) \n");
		printf("numer el %d\n",intent);
		printf("\n");
		
		int kk = 0;int num_dofss=16;	
		for (int jdofs=0;jdofs<num_dofss;jdofs++) {
			printf("\n");
			if(jdofs%4==0){printf("\n");}
			for (int idofs=0;idofs<num_dofss;idofs++) {
				if(jdofs==idofs)printf("%10.5lf",stiff_mat[kk+idofs]);
			}
			kk+=num_dofss;
			}		
		getchar();
		/*ew*/						   
							   
						   
						   
        for (idofent=0;idofent<nr_dof_ent;idofent++) {
            int dof_ent_id = l_dof_ent_id[idofent];
            int dof_ent_type = l_dof_ent_type[idofent];
            int dof_struct_id = -1;

            if (dof_ent_type == PDC_ELEMENT ) {
                dof_struct_id = solver_p->l_dof_elem_to_struct[dof_ent_id];
            } else if (dof_ent_type == PDC_FACE ) {
                dof_struct_id = solver_p->l_dof_face_to_struct[dof_ent_id];
            } else if (dof_ent_type == PDC_EDGE ) {
                dof_struct_id = solver_p->l_dof_edge_to_struct[dof_ent_id];
            } else if (dof_ent_type == PDC_VERTEX ) {
                dof_struct_id = solver_p->l_dof_vert_to_struct[dof_ent_id];
            }

            l_dof_ent_posglob[idofent]=solver_p->l_dof_struct[dof_struct_id].posglob;
        }

        sir_direct_assemble_stiff_mat(Solver_id, Comp_type, nr_dof_ent,
                               l_dof_ent_nrdofs, l_dof_ent_posglob,
                               stiff_mat, rhs_vect, NULL, dov_matrix);
    }

    //create CRS - START
    int num_nnz = 0;
    for (i = 0;i < n; ++i) {
        (dov_matrix[i])[i] += 0.0; //to make sure diagonal is stored
        num_nnz += dov_matrix[i].size();
    }

    solver_p->stiff_mat_crs = (double *)malloc(num_nnz*sizeof(double));
    solver_p->columns_crs = (int *)malloc(num_nnz*sizeof(int));
    solver_p->rows_crs = (int *)malloc((n + 1)*sizeof(int));

    std::map<int,double>::iterator it;
    int idx = 0;
    int rowidx = 1;
    int oldrow = 1;
    for (i = 0;i < n; ++i) {
      solver_p->rows_crs[i] = idx+1;
      for (it = dov_matrix[i].begin();it!=dov_matrix[i].end();++it) {
	int col = (*it).first + 1;
	solver_p->stiff_mat_crs[idx] = (*it).second;
	solver_p->columns_crs[idx] = col;
	
	/*pcw/
	  printf("\n");
	  printf("After Asembling (only diagonal) \n");
	  if(solver_p->columns_crs[idx]==i+1){
	  printf("\n");
	  if(i%4==0){printf("\n");}
	  printf("%d, %10.5lf",i,solver_p->stiff_mat_crs[idx]);
	  }
	  /*ew*/		
	
	++idx;
      }
    }
    solver_p->rows_crs[n]=num_nnz+1;
    //create CRS - END
    
    if(msglvl == 1)
    {
    	printf("Number of equations: %i \n", n);
    	printf("Number of non-zeros in A: %i \n", num_nnz);
    }

    //double t_wall;
    //t_wall = czas_zegara();

    //phase = 11;
    ////printf("Running pardiso! - PHASE - %i \n", phase);
    //PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &n, solver_p->stiff_mat_crs,
    //		solver_p->rows_crs, solver_p->columns_crs, &idum, &nrhs, iparm,
    //		&msglvl, &ddum, &ddum, &error);
    ////printf("Running pardiso finished with result: %i \n", error);

    //phase = 22;
    ////printf("Running pardiso! - PHASE - %i \n", phase);
    //PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &n, solver_p->stiff_mat_crs,
    //		solver_p->rows_crs, solver_p->columns_crs, &idum, &nrhs, iparm,
    //		&msglvl, &ddum, &ddum, &error);
    ////printf("Running pardiso finished with result: %i \n", error);

    phase = 13;//33;

    if(msglvl == 1)
    {
    	printf("Starting PARDISO phase %i \n", phase);
    }

    assert(sizeof(_INTEGER_t) == sizeof(mnum));
    PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &n, solver_p->stiff_mat_crs,
            solver_p->rows_crs, solver_p->columns_crs, &idum, &nrhs, iparm,
            &msglvl, solver_p->rhs_vect, solver_p->sol_vect, &error);

    if(msglvl == 1)
    {
    	printf("Running PARDISO finished with result: %i (0 - means no error)\n", error);
    	printf("Number of threads used by PARDISO: %i \n", iparm[2]);
    }

    //t_wall=czas_zegara()- t_wall;
    //printf("\nTime of solving sparse system: %f \n", t_wall);

    /* rewrite the solution */
    for (ibl=0;ibl<solver_p->nr_dof_ent;ibl++) {
      sit_direct_dof_struct dof_struct = solver_p->l_dof_struct[ibl];
      pdr_write_sol_dofs(solver_p->problem_id,
			 dof_struct.dof_ent_type,
			 dof_struct.dof_ent_id,
			 dof_struct.nrdofs,
			 &solver_p->sol_vect[dof_struct.posglob]);
/* print out solution 
      if(ibl<10){
	printf("Solution in block %d, dof_ent %d\n", ibl, dof_struct.dof_ent_id);
	for (i=0;i<dof_struct.nrdofs;i++) 
	  printf("%20.10lf",solver_p->sol_vect[dof_struct.posglob+i]);
	printf("\n");
	getchar();
      }
/**/
    }

    phase = -1;
    PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &n, solver_p->stiff_mat_crs,
            solver_p->rows_crs, solver_p->columns_crs, &idum, &nrhs, iparm,
            &msglvl, solver_p->rhs_vect, solver_p->sol_vect, &error);


    /* free the storage space */
    free(stiff_mat);
    free(rhs_vect);

    free(iparm);

    free(solver_p->stiff_mat_crs);
    free(solver_p->columns_crs);
    free(solver_p->rows_crs);

    return(1);
}


/*------------------------------------------------------------
sir_direct_assemble_stiff_mat - to assemble entries to the global stiffness matrix
and the global load vector using the provided local
stiffness matrix and load vector
------------------------------------------------------------*/
int sir_direct_assemble_stiff_mat(
    /* returns: >=0 - success code, <0 - error code */
    int Solver_id,         /* in: solver ID (used to identify the subproblem) */
    int Comp_type,         /* in: indicator for the scope of computations: */
    /*   SIC_SOLVE - solve the system */
    /*   SIC_RESOLVE - resolve for the new rhs vector */
    int Nr_dof_bl,         /* in: number of global dof entities (blocks) */
    /*     associated with the local stiffness matrix */
    int* L_bl_nrdofs,       /* in: list of dof blocks' nr dofs */
    int* L_bl_posglob,     /* in: list of blocks' global positions */
    double* Stiff_mat,     /* in: stiffness matrix stored columnwise */
    double* Rhs_vect,      /* in: rhs vector */
    char* Rewr_dofs,         /* in: flag to rewrite or sum up entries */
    /*   'T' - true, rewrite entries when assembling */
    /*   'F' - false, sum up entries when assembling */
    std::vector<std::map<int, double> > &dov_matrix
)
{
    /* auxiliary variables */
    int iblock, jblock, nrdofs_i, nrdofs_j, posglob_i, posglob_j;
    int posloc_i, posloc_j;
    int i,j, iaux, jaux;

    /*kbw
    static FILE *fp=NULL;
    /*kew*/

    /*++++++++++++++++ executable statements ++++++++++++++++*/

    /*kbw
    if(fp==NULL) fp=fopen("stiff_mat","a");
    /*kew*/

    /* pointers to solver structure and stiffness matrix blocks' info */
    /* get pointer to solver structure */
    sit_direct_solver *solver_p = sir_direct_select_solver(Solver_id);

    /* global number of degrees of freedom */
    int nrdofs_glob = solver_p->nrdofs_glob;

    /* compute local stiffness matrix nrdofs */
    int nrdofs=0;
    for (iblock=0;iblock<Nr_dof_bl;iblock++) {
        nrdofs += L_bl_nrdofs[iblock];
    }

    /* loop over stiffness matrix blocks */
    posloc_i=0;
    for (iblock=0;iblock<Nr_dof_bl;iblock++) {

        /* global position of the first entry and number of dofs for a block */
        posglob_i = L_bl_posglob[iblock];
        nrdofs_i = L_bl_nrdofs[iblock];


        /*kbw
        fprintf(fp,"block %d, posglob_i %d, nrdofs_i %d\n",
        iblock, L_bl_posglob[iblock], L_bl_nrdofs[iblock]);
        /*kew*/


        posloc_j=0;
        for (jblock=0;jblock<Nr_dof_bl;jblock++) {

            /* global position of the first entry and number of dofs for a block */
            posglob_j = L_bl_posglob[jblock];
            nrdofs_j = L_bl_nrdofs[jblock];

            /*kbw
            fprintf(fp,"block %d, posglob_i %d, nrdofs_i %d\n",
            jblock, L_bl_posglob[jblock], L_bl_nrdofs[jblock]);
            /*kew*/

            for (i=0;i<nrdofs_i;i++) {

                /* global and local positions of the first entry in the column */
                iaux = posglob_j+(posglob_i+i)*nrdofs_glob;
                jaux = posloc_j+(posloc_i+i)*nrdofs;

                /*kbw
                fprintf(fp,"assembling sm entries %d - %d\n",
                iaux,iaux+nrdofs_j-1,jaux,jaux+nrdofs_j-1 );
                /*kew*/

                for (j=0;j<nrdofs_j;j++) {

                    /* assemble stiffness matrix block's entries */
                    //solver_p->stiff_mat[iaux+j] += Stiff_mat[jaux+j]; //HERE!
                    //dov_matrix[iaux+j] += Stiff_mat[jaux+j];
		    
		    
		   // (dov_matrix[posglob_i+i])[posglob_j+j] += Stiff_mat[jaux+j]; 
		     
		     (dov_matrix[posglob_j+j])[posglob_i+i] += Stiff_mat[jaux+j]; //CHANGED !!!!!!!!!!!!!!!1

                    /*kbw
                    fprintf(fp,"%20.15lf   %20.15lf\n",
                    Stiff_mat[jaux+j], solver_p->stiff_mat[iaux+j]);
                    /*kew*/

                }
            }

            posloc_j += nrdofs_j;
        }

        /*kbw
        fprintf(fp,"assembling rhsv entries %d - %d\n",
        posglob_i,posglob_i+nrdofs_i-1,posloc_i,posloc_i+nrdofs_i-1);
        /*kew*/

        for (i=0;i<nrdofs_i;i++) {

            /* assemble right hand side block's entries */
            solver_p->rhs_vect[posglob_i+i] += Rhs_vect[posloc_i+i];

            /*kbw
            fprintf(fp,"%20.15lf   %20.15lf\n",
            Rhs_vect[posloc_i+i], solver_p->rhs_vect[posglob_i+i]);
            /*kew*/

        }

        posloc_i += nrdofs_i;
    }

    /*kbw
    getchar();
    /*kew*/


    return(1);
}


/*------------------------------------------------------------
sir_direct_free - to  free memory
------------------------------------------------------------*/
int sir_direct_free( /* returns: >=0 - success code, <0 - error code */
    int Solver_id   /* in: solver identification */
)
{

    /* pointers to the solver and its levels' structures - to simplify */
    sit_direct_solver* solver_p;


    /*++++++++++++++++ executable statements ++++++++++++++++*/

    solver_p = sir_direct_select_solver(Solver_id);

    free(solver_p->l_int_ent_type);
    free(solver_p->l_int_ent_id);
    if (solver_p->max_dof_elem_id>=0) free(solver_p->l_dof_elem_to_struct);
    if (solver_p->max_dof_face_id>=0) free(solver_p->l_dof_face_to_struct);
    if (solver_p->max_dof_edge_id>=0) free(solver_p->l_dof_edge_to_struct);
    if (solver_p->max_dof_vert_id>=0) free(solver_p->l_dof_vert_to_struct);
    free(solver_p->l_dof_struct);

    free(solver_p->rhs_vect);
    free(solver_p->sol_vect);
    free(solver_p->aux_vect);

    return(0);
}

/*------------------------------------------------------------
sir_direct_destroy - to destroy the solver
------------------------------------------------------------*/
int sir_direct_destroy( /* returns: >=0 - success code, <0 - error code */
    int Solver_id   /* in: solver identification */
)
{

  /* set the current solver ID */
  if(siv_direct_cur_solver_id==siv_direct_nr_solvers) siv_direct_cur_solver_id--;

  /* decrease the counter for solvers */
  siv_direct_nr_solvers--;
  
  return(1);
}


/*** AUXILIARY LOCAL PROCEDURES ***/

/*------------------------------------------------------------
sir_direct_select_solver - to return the pointer to a given solver
------------------------------------------------------------*/
sit_direct_solver* sir_direct_select_solver( /* returns: pointer to solver */
    int Solver_id    /* in: solver identification */
)
{
  if (Solver_id>0&&Solver_id<=siv_direct_nr_solvers) {
    siv_direct_cur_solver_id=Solver_id;
  }
  return(&siv_direct_solvers[siv_direct_cur_solver_id-1]);
}


/*---------------------------------------------------------
sir_direct_put_list - to put Num on the list List with length Ll
(filled with numbers and SIC_LIST_END_MARK at the end)
---------------------------------------------------------*/
int sir_direct_put_list( /* returns*/
    /*  >0 - position already occupied on the list */
    /*  <0 - position at which put on the list */
    /*   0 - list full, not found on the list */
    int Num, 	/* in: number to put on the list */
    int* List, 	/* in: list */
    int Ll		/* in: total list's lengths */
)
{

    int i, il, pos;

    for (i=0;i<Ll;i++) {
        pos = i;
        if ((il=List[pos])==SIC_LIST_END_MARK) break;
        /* found on the list on (i+1) position; 1 offset - position 1 = List[0] */
        if (Num==il) return(pos+1);
    }
    /* if list is full return error message */
    if (pos==Ll-1) return(0);
    /* update the list and return*/
    List[pos]=Num;
    return(-(++pos));
}


bool textStartsWith(const std::string& text,const std::string& token)
{
	if(text.length() < token.length())
		return false;
	return (text.compare(0, token.length(), token) == 0);
}


/*------------------------------------------------------------
readPardisoConfig - reads PARDISO config file
------------------------------------------------------------*/
bool readPardisoConfig(sit_pardiso_config &config, const char* filePath)
{
/* Input PARDISO parameters which are read from config file:
	mtype  // Matrix type
    msglvl // Print statistical information in file
    iparm1 //use solver defaults?
    iparm2 //fill-in reducing ordering.
    iparm4 //preconditioned CGS.
    iparm8 //iterative refinement step.
    iparm10 //pivoting perturbation.
    iparm11 //scaling vectors.
    iparm13 //improved accuracy using (non-)symmetric weighted matchings.
    iparm18 //-1
    iparm19 //MFlops of factorization.
    iparm21 //pivoting for symmetric indefinite matrices.
    iparm27 //matrix checker. default is 0
    iparm60 //version of PARDISO.
*/


	fstream configFileStream(filePath);
	string configLine;
	bool isValid = true;

	if(configFileStream.is_open())
	{
		while(!configFileStream.eof() && isValid)
		{
			getline(configFileStream, configLine);
			istringstream iss(configLine);

			std::vector<string> tokens;
			copy(istream_iterator<string>(iss),
				 istream_iterator<string>(),
				 back_inserter<vector<string> >(tokens));

			if(tokens.size() > 1 && !textStartsWith(tokens[0],"#"))
			{
				string key = tokens[0];
				transform(key.begin(), key.end(), key.begin(), ::toupper);

				if(key.compare("MTYPE") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.mtype);

				}
				else if(key.compare("MSGLVL") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.msglvl);

				}
				else if(key.compare("IPARM1") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm1);

				}
				else if(key.compare("IPARM2") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm2);

				}
				else if(key.compare("IPARM4") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm4);

				}
				else if(key.compare("IPARM8") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm8);

				}
				else if(key.compare("IPARM10") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm10);

				}
				else if(key.compare("IPARM11") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm11);

				}
				else if(key.compare("IPARM13") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm13);

				}
				else if(key.compare("IPARM18") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm18);

				}
				else if(key.compare("IPARM19") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm19);

				}
				else if(key.compare("IPARM21") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm21);

				}
				else if(key.compare("IPARM27") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm27);

				}
				else if(key.compare("IPARM60") == 0 && tokens.size() >= 2)
				{
					istringstream valueStream(tokens[1]);
					isValid = (valueStream>>config.iparm60);

				}
				else
				{
					isValid = false;
				}

			}
		}
		configFileStream.close();
	}
	else
	{
		isValid = false;
	}

	return isValid;
} //END OF readPardisoConfig function
