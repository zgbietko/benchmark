/************************************************************************
File pds_laplace_dg_adapt - contains routines for adaptations
  pdr_adapt - to enforce adaptation strategy for a given problem
  pdr_err_indi - to return error indicator for an element (internal procedure)
  pdr_err_indi_exact - to return error indicator for an element based on
                       the knowledge of exact solution (internal procedure)
  pdr_err_indi_ZZ - to return ZZ error indicator for element (internal procedure)
  pdr_zzhu_error - to compute Zienkiewicz-Zhu error estimator

------------------------------
History:
        06.2011 - KGB, interoperability with utd_util routines
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>


/* header file of the problem dependent module for Laplace's equation */
#include "../pdh_laplace.h"	

/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"

/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"

/* interface of the mesh manipulation module */
#include "mmh_intf.h"	

/* interface for all approximation modules */
#include "aph_intf.h"	

/* general purpose utilities */
#include "uth_intf.h"


/* interface for linear algebra packages */
#include "lin_alg_intf.h"

/* global variable for the patch data structure */
utt_patches *pdv_patches;

/*---------------------------------------------------------
pdr_err_indi_exact - to return error indicator for an element,
	based on the knowledge of the exact solution 
----------------------------------------------------------*/
double pdr_err_indi_exact (	/* returns error indicator for an element */
        int Problem_id,	/* in: data structure to be used  */
	int El		/* in: element number */
);

/*---------------------------------------------------------
pdr_err_indi_ZZ - to return error indicator for an element,
	based on ZZ first derivative recovery 
----------------------------------------------------------*/
double pdr_err_indi_ZZ (	/* returns error indicator for an element */
        int Problem_id,	/* in: data structure to be used  */
	int El		/* in: element number */
);

/*---------------------------------------------------------
pdr_err_indi - to return error indicator for an element
----------------------------------------------------------*/
double pdr_err_indi (	/* returns error indicator for an element */
        int Problem_id,	/* in: data structure to be used  */
	int Mode,	/* in: mode of operation */
	int El		/* in: element number */
)
{

  if(Mode==PDC_ADAPT_EXACT){

    return pdr_err_indi_exact(Problem_id, El);

  } else {

    printf("Only error indicators based on the knowledge of exact solution\n");
    printf("implemented for DG! (use PDC_ADAPT_EXACT==1 \n");
    printf("in problem_conv_diff.dat as the first of ADAPTATION_PARAMETERS)\n");

  }

  return(0.0);
}



/*---------------------------------------------------------
pdr_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
int pdr_adapt( /* returns: >0 - success, <=0 - failure */
  int Problem_id,       /* in: problem data structure to be used */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  )
{


  // depending on type of adaptation (type=pdr_adapt_i_params(Problem_id,i);)
  // type is specified as the first ADAPTATION_PARAMETERS in problem input file
  // type == -2 - uniform derefinement
  // type == -1 - uniform refinement
  // type == 0 - no adaptations
  // type > 0  - adaptive refinement: element error indicators are computed
  //             by pdr_err_indi, elements with error greater then 
  //             eps*average_error are broken, families of elements (sons
  //             of a single father) with total error less than 
  //             ratio*eps*average_error are clustered back
  // particular values of type may be problem dependent
  // most popular types are encoded in include/pdh_intf.h as:
  //  PDC_ADAPT_EXACT = 1 - adaptations based on the knowledge of exact solution
  //  PDC_ADAPT_ZZ    = 2 - adaptations based on Zienkiewicz-Zhu error estimate
  //  PDC_ADAPT_EXPL  = 3 - adaptations based on explicit residual error estimate


  // enforce default adaptation strategy (utr_adapt):
  // eps is specified as the first of ADAPT_TOLERANCE_REF_UNREF
  // ratio is specified as the second of ADAPT_TOLERANCE_REF_UNREF
  // (there is one more hook - if eps is less than 0.1 it is considered
  // as global parameter and elements with error greater then eps are broken)
  utr_adapt(Problem_id, Work_dir, Interactive_input, Interactive_output);

  // option to consider: utr_adapt accepting the following parameters:
  //i=1; name=pdr_ctrl_i_params(Problem_id,i);
  //i=2; mesh_id=pdr_ctrl_i_params(Problem_id,i);
  //i=3; field_id=pdr_ctrl_i_params(Problem_id,i);
  //i=1; type=pdr_adapt_i_params(Problem_id,i);
  //i=5; eps=pdr_adapt_d_params(Problem_id,i);
  //i=6; ratio=pdr_adapt_d_params(Problem_id,i);
  //i=7; iprint=pdr_adapt_i_params(Problem_id,i);


  return(1);

}

/*---------------------------------------------------------
pdr_err_indi_exact - to return error indicator for an element,
	based on the knowledge of the exact solution 
----------------------------------------------------------*/
double pdr_err_indi_exact (	/* returns error indicator for an element */
        int Problem_id,	/* in: data structure to be used  */
	int El		/* in: element number */
)
{

  double l2_err = 0.0, h1_err = 0.0; /* error norms */
  int el_type;		/* element type */
  int nreq;		/* number of equations */
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


/* for scalar test cases - exact solution and its derivatives */
  double f, f_x, f_z, f_y;
/* auxiliary variables */
  int field_id, mesh_id;
  int i, ki, iaux, name, mat_num, sol_vec_id;
  double daux, eaux,volume, average, time;

  /* quadrature rules */
/*   int pdeg;		/\* degree of polynomial *\/ */
/*   static int pdeg_old=-1; /\* indicator for recomputing quadrature data *\/ */
/*   int base;		/\* type of basis functions  *\/ */
/*   static int base_old=-1;		/\* type of basis functions  *\/ */
/*   static int ngauss;            /\* number of gauss points *\/ */
/*   static double xg[3000];   	 /\* coordinates of gauss points in 3D *\/ */
/*   static double wg[1000];       /\* gauss weights *\/ */

/* #pragma omp threadprivate (pdeg_old) */
/* #pragma omp threadprivate (base_old) */
/* #pragma omp threadprivate (ngauss) */
/* #pragma omp threadprivate (xg) */
/* #pragma omp threadprivate (wg) */

  // to make OpenMP working
  int pdeg;		/* degree of polynomial */
  int pdeg_old = -1; /* indicator for recomputing quadrature data */
  int base;		/* type of basis functions for quadrilaterals */
  int base_old = -1;		/* type of basis functions for quadrilaterals */
  int ngauss;            /* number of gauss points */
  double xg[3000];   	 /* coordinates of gauss points in 3D */
  double wg[1000];       /* gauss weights */

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* get formulation parameters */
  i=1; name=pdr_ctrl_i_params(Problem_id,i);
  i=2; mesh_id=pdr_ctrl_i_params(Problem_id,i);
  i=3; field_id=pdr_ctrl_i_params(Problem_id,i);
  i=5; nreq=pdr_ctrl_i_params(Problem_id,i);
  base=apr_get_base_type(field_id, El);

/* only if exact solution is known */
  if(name==1||name==101||name==105){


/* find element type */
    el_type = mmr_el_type(Problem_id,El);
    mat_num = mmr_el_groupID(Problem_id,El);

/* find degree of polynomial and number of element scalar dofs */
    apr_get_el_pdeg(field_id, El, &pdeg);
    num_shap = apr_get_el_pdeg_numshap(field_id,El,&pdeg);
    ndofs = nreq*num_shap;

/* get the coordinates of element nodes in the right order */
    mmr_el_node_coor(mesh_id,El,el_nodes,node_coor);

    /* get the most recent solution degrees of freedom */
    sol_vec_id = 1;
    apr_get_el_dofs(field_id,El,sol_vec_id,dofs_loc);

/* prepare data for gaussian integration */
    if(pdeg!=pdeg_old||base!=base_old){
      apr_set_quadr_3D(base, &pdeg, &ngauss, xg, wg);
      pdeg_old = pdeg;
      base_old = base;
    }

/*kbw
      if(El==269){
printf("In err_indi for element %d, type %d\n",El,el_type);
printf("pdeg %d, ngauss %d\n",pdeg,ngauss);
printf("nreq %d, ndof %d, local_dim %d\n",nreq,num_shap,ndofs);
printf("%d nodes with coordinates:\n",el_nodes[0]);
for(i=0;i<el_nodes[0];i++){
  printf("node %d: x - %f, y - %f, z - %f\n",
el_nodes[i+1],node_coor[3*i],node_coor[3*i+1],node_coor[3*i+2]);
}
printf("solution dofs:\n");
for(i=0;i<ndofs;i++){
  printf("%20.12lf",dofs_loc[i]);
}
printf("\n");
getchar();
      }
/*kew*/

    volume=0.0; average=0.0;
    for (ki=0;ki<ngauss;ki++) {
    
      /* at the gauss point, compute basis functions, determinant etc*/
      iaux = 2; /* calculations with jacobian but not on the boundary */
      determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base,
				&xg[3*ki],node_coor,dofs_loc,
				base_phi,base_dphix,base_dphiy,base_dphiz,
				xcoor,u_val,u_x,u_y,u_z,NULL);
    
      vol = determ * wg[ki];

      pdr_exact_sol(mat_num,xcoor[0],xcoor[1],xcoor[2],0,
		    &f,&f_x,&f_y,&f_z,&eaux);


/*kbw
      if(El==269){
printf("at gauss point %d, local coor %lf, %lf, %lf\n", 
ki,xg[3*ki],xg[3*ki+1],xg[3*ki+2]);
printf("global coor %lf %lf %lf\n",xcoor[0],xcoor[1],xcoor[2]);
printf("weight %lf, determ %lf, coeff %lf\n",
	wg[ki],determ,vol);
printf("%d shape functions and derivatives: \n", num_shap);
for(i=0;i<num_shap;i++){
  printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
  base_phi[i],base_dphix[i],base_dphiy[i],base_dphiz[i]);
}
printf("u_val = %f, exactv = %f\n", u_val[0], f); 
printf("u_x = %f, exactv_x = %f\n", u_x[0], f_x); 
printf("u_y = %f, exactv_y = %f\n", u_y[0], f_y); 
printf("u_z = %f, exactv_z = %f\n", u_z[0], f_z); 
getchar();
      }
/*kew*/

	volume += vol;
	average+= u_val[0]*vol;
	l2_err +=  (f-u_val[0])*(f-u_val[0])*vol;
	h1_err += ((f_x-u_x[0])*(f_x-u_x[0])+(f_y-u_y[0])*(f_y-u_y[0])
			+(f_z-u_z[0])*(f_z-u_z[0]))*vol;

      } /* ki */

/*kbw
      if(El==269){
printf("Average in element %d = %lf (at center = %lf)\n",
El,average/volume,dofs_loc[0]+dofs_loc[2]/3.0+dofs_loc[3]/3.0);
      }
/*kew*/

  } /* if exact solution known */
  else{

    printf("Cannot compute error for unknown exact solution...\n");
    return(-1.0);

  }

  return((l2_err + h1_err));

}

/*---------------------------------------------------------
pdr_zzhu_error - to compute estimated norm of error based on recovered 
		 first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_zzhu_error( /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  int Field_id,    /* in: approximation field ID  */
  FILE *interactive_output
		)
{

  printf("ZZ error estimator not implemented for DG.\n");

  return(1);
}
