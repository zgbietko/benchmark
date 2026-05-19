/************************************************************************
File pds_ns_supg_ale_adapt.c - adaptation functions for ns_supg module

Contains declarations of routines:
  pdr_ns_supg_ale_adapt - to enforce adaptation strategy for a given problem 
  pdr_ns_supg_ale_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
  pdr_ns_supg_ale_err_indi_explicit
  pdr_ns_supg_ale_err_indi_ZZ - to return error indicator for an element,
        based on ZZ first derivative recovery 

------------------------------
History:
        02.2002 - Krzysztof Banas, initial version
        03.2011 - Filip Krużel, navier-stokes
        06.2011 - KGB, interoperability with utd_util routines
        01.2012 - Krzysztof Banas, pobanas@cyf-kr.edu.pl
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<assert.h>
/* problem dependent interface with the PDEs  */
#include "pdh_intf.h"		/* USES */
/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"	/* IMPLEMENTS */
/* interface of the mesh manipulation module */
#include "mmh_intf.h"		/* USES */
/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* general purpose utilities */
#include "uth_intf.h"		/* USES */
/* interface for linear algebra packages */
#include "lin_alg_intf.h"	/* USES */

/* problem module's types and functions */
#include "../include/pdh_ns_supg_ale.h"	/* USES & IMPLEMENTS*/


/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
// bc and material header files are included in problem header files

/* global variable for the patch data structure used in derivative recovery */
utt_patches *pdv_patches;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */



/*---------------------------------------------------------
pdr_ns_supg_ale_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
int pdr_ns_supg_ale_adapt(  /* returns: >0 - success, <=0 - failure */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output)
{

  double daux, clock_time;
  int i, iaux, ino, nno, mesh_id, field_id;
  int type, problem_id, nr_patches;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;
  problem_id = PDC_NS_SUPG_ID;
  field_id = pdr_ctrl_i_params(problem_id, 3);
  mesh_id = apr_get_mesh_id(field_id);

/* get adaptation parameters */
  i=1; type=pdr_adapt_i_params(problem_id,i);
  //i=5; eps=pdr_adapt_d_params(problem_id,i);
  //i=6; ratio=pdr_adapt_d_params(problem_id,i);
  //i=7; iprint=pdr_adapt_i_params(problem_id,i);

  // type == -2 - uniform derefinement - may be performed by utr_adapt
  // type == -1 - uniform refinement - may be performed by utr_adapt
  // type == 0 - no adaptations
  // type > 0  - adaptive refinement - default strategy for SINGLE problem at utr_adapt

  // particular values of type may be problem dependent
  // most popular types are encoded in include/pdh_intf.h as:
  //  PDC_ADAPT_EXACT = 1 - adaptations based on the knowledge of exact solution
  //  PDC_ADAPT_ZZ    = 2 - adaptations based on Zienkiewicz-Zhu error estimate
  //  PDC_ADAPT_EXPL  = 3 - adaptations based on explicit residual error estimate
  if (type == PDC_ADAPT_EXACT) {
    printf("No exact solution for ns_supg problem!.\n");
    printf("Change adaptation type in input file to: \n");
    printf("PDC_ADAPT_ZZ = 2 or PDC_ADAPT_EXPL  = 3 (not working now) \n");
  }
  if (type == PDC_ADAPT_ZZ) {

    // create patches for nno_old nodes
    clock_time = time_clock();
    //nr_patches = utr_create_patches(field_id, &pdv_patches);
    nr_patches = utr_create_patches_small(field_id, &pdv_patches);
    clock_time = time_clock() - clock_time;
    printf("New patch creation - time %lf\n", clock_time);


    // recover all first derivatives of all solution components
    clock_time = time_clock();

    // !!! it is assumed that the most recent dofs are in the last vector
    //int sol_vec_id = apr_get_nr_sol(field_id);
    // !!! it is assumed that the most recent dofs are in the first vector
    int sol_vec_id = 1;
    // recover all first derivatives of all solution components
    //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
    utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

    clock_time = time_clock() - clock_time;
    printf("Derivatives recovery - time %lf\n", clock_time);



	/*kbw
  nno=0;
  while((nno=mmr_get_next_node_all(mesh_id, nno))!=0){
    double f,f_x,f_y,f_z,xcoor[3],eaux;
	int ider,nr_deriv=12;
	printf("\nReal derivatives for node %d :\n",nno);
    mmr_node_coor(mesh_id, nno, xcoor);
    //pdr_exact_sol(iaux,xcoor[0],xcoor[1],xcoor[2],daux,
    //                &f,&f_x,&f_y,&f_z,&eaux);
    printf("%8.4lf%8.4lf%8.4lf", f_x, f_y, f_z);
    printf("\nRecovered derivatives for node %d :\n",nno);
    for(ider=0;ider<nr_deriv;ider++)
      {
        printf("%8.4lf", pdv_patches[nno].deriv[ider]);
      }
    printf("\n");
  }
/*kew*/

    // enforce default adaptation strategy (utr_adapt) for a SINGLE PROBLEM:
    // for type > 0  - adaptive refinement: element error indicators are computed
    //             by pdr_err_indi, elements with error greater then 
    //             eps*average_error are broken, families of elements (sons
    //             of a single father) with total error less than 
    //             ratio*eps*average_error are clustered back
    // eps is specified as the first of ADAPT_TOLERANCE_REF_UNREF
    // ratio is specified as the second of ADAPT_TOLERANCE_REF_UNREF
    // (there is one more hook - if eps is less than 0.1 it is considered
    // as global parameter and elements with error greater then eps are broken)
    utr_adapt(problem_id, Work_dir, 
	      Interactive_input, Interactive_output);

    // option to consider: utr_adapt accepting the following parameters:
    //i=1; name=pdr_ctrl_i_params(problem_id,i);
    //i=2; mesh_id=pdr_ctrl_i_params(problem_id,i);
    //i=3; field_id=pdr_ctrl_i_params(problem_id,i);
    //i=1; type=pdr_adapt_i_params(problem_id,i);
    //i=5; eps=pdr_adapt_d_params(problem_id,i);
    //i=6; ratio=pdr_adapt_d_params(problem_id,i);
    //i=7; iprint=pdr_adapt_i_params(problem_id,i);


    // free the space - only nno_old patches; no patches for new nodes !!! */
    for (ino = 1; ino <= nr_patches; ino++) {
      if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
    }
    free(pdv_patches);

  }				// end if ZZ adaptation
  if (type == PDC_ADAPT_EXPL) {

    printf("Wrong adapt_type in adapt!");
    // ?????

  }

  return (1);
}


/*---------------------------------------------------------
pdr_ns_supg_ale_ZZ_error - to compute estimated norm of error based on 
                 recovered first derivatives - the notorious ZZ error estimate
---------------------------------------------------------*/
double pdr_ns_supg_ale_ZZ_error(		
	   /* returns  - Zienkiewicz-Zhu error for the whole mesh */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
)
{

/* local variables */
  double err_ZZ = 0.0, el_err = 0.0;	/* error norms */
  double err_ZZ_LDC = 0.0;	/* error norms */
  int i, j, k, mesh_id, nreq, nr_deriv, ino, nno, nel;
  double clock_time;
  int problem_id, name, field_id, nr_patches;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;
  problem_id = PDC_NS_SUPG_ID;
  field_id = pdr_ctrl_i_params(problem_id, 3);
  mesh_id = apr_get_mesh_id(field_id);

  nreq = apr_get_nreq(field_id);
  nr_deriv = 3 * nreq;

  // create patches for nno_old nodes
  clock_time = time_clock();
  //nr_patches = utr_create_patches(field_id, &pdv_patches);
  nr_patches = utr_create_patches_small(field_id, &pdv_patches);
  clock_time = time_clock()-clock_time;
  fprintf(Interactive_output, "Patch creation - time %lf\n", clock_time);


  // recover all first derivatives of all solution components
  clock_time = time_clock();

  // !!! it is assumed that the most recent dofs are in the last vector
  //int sol_vec_id = apr_get_nr_sol(field_id);
  // !!! it is assumed that the most recent dofs are in the first vector
  int sol_vec_id = 1;
  // recover all first derivatives of all solution components
  //utr_recover_derivatives(field_id, sol_vec_id, nr_patches, pdv_patches);
  utr_recover_derivatives_small(field_id, sol_vec_id, nr_patches, pdv_patches);

  clock_time = time_clock()-clock_time;
  fprintf(Interactive_output, "Derivative recovery - time %lf\n", clock_time);

  /*kbw
  nno=0;
  while((nno=mmr_get_next_node_all(mesh_id, nno))!=0){
    double f,f_x,f_y,f_z,xcoor[3],eaux;
	int ider;
	printf("\nReal derivatives for node %d :\n",nno);
    mmr_node_coor(mesh_id, nno, xcoor);
    //pdr_exact_sol(iaux,xcoor[0],xcoor[1],xcoor[2],daux,
    //                &f,&f_x,&f_y,&f_z,&eaux);
    printf("%8.4lf%8.4lf%8.4lf", f_x, f_y, f_z);
    printf("\nRecovered derivatives for node %d :\n",nno);
    for(ider=0;ider<nr_deriv;ider++)
      {
        printf("%8.4lf", pdv_patches[nno].deriv[ider]);
      }
    printf("\n");
  }
/*kew*/

  nel = 0;
  err_ZZ = 0;
  err_ZZ_LDC = 0;
  while ((nel = mmr_get_next_act_elem(mesh_id, nel)) != 0) {
    el_err = pdr_ns_supg_ale_err_indi_ZZ(field_id, nel);
    
#ifdef DEBUG
    if (el_err < -10e200 || el_err > 10e200) {
      printf("in active element %d - error_indi %20.15lf. Exiting! \n",
	     nel,el_err);
      exit(-1);
    }
#endif
    
    err_ZZ += el_err;
    
    /*kbw
      printf("in active element %d - error_indi %20.15lf \n",
      nel,el_err);
      err_ZZ += pdr_ns_supg_ale_err_indi_ZZ(field_id,nel);
      /*kew */
    
// to compute special error for LDC problem (10 <= name < 20)
    name = pdr_ctrl_i_params(problem_id, 1);
    if(name>=10 && name <20) {
      int el_nodes_loc[10];
      double node_coor_loc[30];
      double max_x = 0.0, max_y = 0.0;
      mmr_el_node_coor(mesh_id, nel, el_nodes_loc, node_coor_loc);
      for (i = 0; i < el_nodes_loc[0]; i++) {
	if (node_coor_loc[i * 3] > max_x) {
	  max_x = node_coor_loc[i * 3];
	}
	if (node_coor_loc[i * 3 + 1] > max_y) {
	  max_y = node_coor_loc[i * 3 + 1];
	}
      }
      if (max_y < 0.9) {
	//if(pdr_ns_supg_ale_err_indi_ZZ(Field_id,nel)>0.004){
	//if(max_x>0.9 && max_y>0.9){
	//  printf("in active element %d (max_x %lf, max_y %lf - error_indi %20.15lf \n",
	//     nel,max_x, max_y, pdr_ns_supg_ale_err_indi_ZZ(Field_id,nel));
	err_ZZ_LDC += pdr_ns_supg_ale_err_indi_ZZ(field_id, nel);
      }
    }
    
  }
  err_ZZ = sqrt(err_ZZ);
  fprintf(Interactive_output,"\nZienkiewicz-Zhu error estimator = %lf\n",err_ZZ);

  if(name>=10 && name <20) {
    err_ZZ_LDC = sqrt(err_ZZ_LDC);
    fprintf(Interactive_output, 
	    "\nspecial Zienkiewicz-Zhu error estimator for LDC problem = %lf\n", 
	    err_ZZ_LDC);
  }
  
  // free the space */
  for (ino = 1; ino <= nr_patches; ino++) {
    if(pdv_patches[ino].deriv!=NULL) free(pdv_patches[ino].deriv);
  }
  free(pdv_patches);

  return (err_ZZ);

}

/*---------------------------------------------------------
pdr_ns_supg_ale_err_indi_ZZ - to return error indicator for an element
----------------------------------------------------------*/
double pdr_ns_supg_ale_err_indi_ZZ(/* returns error indicator for an element */
  int Problem_id_fake, 
  int El	/* in: element number */
    )
{
  double err = 0;
  int i, ki, j, k, ieq, nr_comp;
  int problem_id, field_id, mesh_id, pdeg, num_shap, sol_vec_id, iaux, nreq;
  int base;		/* type of basis functions  */
  int el_nodes[MMC_MAXELVNO + 1]={0};	/* list of nodes of El */
  double node_coor[3 * MMC_MAXELVNO]={0.0};	/* coord of nodes of El */
  double dofs_loc[APC_MAXELSD]={0.0};	/* element solution dofs */
  double xcoor[3]={0.0};		/* global coord of gauss point */
  double u_val[PDC_MAXEQ]={0.0};	/* computed solution */
  double u_x[PDC_MAXEQ]={0.0};		/* gradient of computed solution */
  double u_y[PDC_MAXEQ]={0.0};		/* gradient of computed solution */
  double u_z[PDC_MAXEQ]={0.0};		/* gradient of computed solution */
  double base_phi[APC_MAXELVD]={0.0};	/* basis functions */
  double base_dphix[APC_MAXELVD]={0.0};	/* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD]={0.0};	/* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD]={0.0};	/* y-derivatives of basis function */
  double determ;
  int ngauss;			/* number of gauss points */
  double xg[3000]={0.0};		/* coordinates of gauss points in 3D */
  double wg[1000]={0.0};		/* gauss weights */
  double vol;			/* volume for integration rule */
  double deriv_el_nodes[3][9]={0.0};	/* derivatives at element nodes */
  double deriv_at_point[3]={0.0};

/*++++++++++++++++ executable statements ++++++++++++++++*/

  pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;
  problem_id = PDC_NS_SUPG_ID;

/* get formulation parameters */
  //i=1; name=pdr_ctrl_i_params(problem_id,i);
  i=2; mesh_id=pdr_ctrl_i_params(problem_id,i);
  i=3; field_id=pdr_ctrl_i_params(problem_id,i);
  //nreq = apr_get_nreq(field_id);
  nreq = 4; // four components in NS
  nr_comp = 3; // we consider only velocities for ns_supg error indicator

/* get adaptation parameters */
  //i=1; type=pdr_adapt_i_params(problem_id,i);
  //i=5; eps=pdr_adapt_d_params(problem_id,i);
  //i=6; ratio=pdr_adapt_d_params(problem_id,i);
  //i=7; iprint=pdr_adapt_i_params(problem_id,i);


  /* find degree of polynomial and number of element scalar dofs */
  apr_get_el_pdeg(field_id, El, &pdeg);

  /* get the coordinates of the nodes of El in the right order */
  mmr_el_node_coor(mesh_id, El, el_nodes, node_coor);

  /* prepare data for gaussian integration */
  base = apr_get_base_type(field_id, El);
  apr_set_quadr_3D(base, &pdeg, &ngauss, xg, wg);

  /* get the most recent solution degrees of freedom */
  sol_vec_id = 1;
  apr_get_el_dofs(field_id, El, sol_vec_id, dofs_loc);

  // initialize element error
  err = 0.0;

  // for each solution component
  for (ieq = 0; ieq < nr_comp; ieq++) {

    // get 3 derivatives 
    for (j = 0; j < 3; j++) {
      k = 0;
      for (i = 1; i <= el_nodes[0]; i++) {
	deriv_el_nodes[j][k] = pdv_patches[el_nodes[i]].deriv[3 * ieq + j];

/*kbw
	if(El==2232){
	  printf("El %d, node %d, index %d, deriv %lf\n",
		 El, el_nodes[i], 3 * ieq + j, 
		 pdv_patches[el_nodes[i]].deriv[3 * ieq + j]);
	}
/*kew*/
	k++;
      }
    }
    for (j = 0; j < 3; j++) deriv_at_point[j] = 0.0;


    for (ki = 0; ki < ngauss; ki++) {
      /* at the gauss point, compute basis functions, determinant etc */
      iaux = 2;	/* calculations with jacobian but not on the boundary */
      determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, &xg[3 * ki], node_coor, 
				dofs_loc, base_phi, base_dphix, base_dphiy, 
				base_dphiz, xcoor, u_val, u_x, u_y, u_z, NULL);
      vol = determ * wg[ki];


      // get derivatives for ieq unknown at gauss points
      for (i = 0; i < 3; i++) {
	iaux = 1;
	apr_elem_calc_3D(iaux, 1, &pdeg, base, &xg[3 * ki], node_coor, 
			 &deriv_el_nodes[i][0], base_phi, base_dphix, 
			 base_dphiy, base_dphiz, xcoor, &deriv_at_point[i], 
			 NULL, NULL, NULL, NULL);
      }

      err += vol * 
	((deriv_at_point[0] - u_x[ieq]) * (deriv_at_point[0] - u_x[ieq]) + 
	 (deriv_at_point[1] - u_y[ieq]) * (deriv_at_point[1] - u_y[ieq]) + 
	 (deriv_at_point[2] - u_z[ieq]) * (deriv_at_point[2] - u_z[ieq])
	  );

    }				// end loop over gauss points

  }				// end loop over solution components

  assert(err > -10e200);
  assert(err < 10e200);
  return (err);

}


/*---------------------------------------------------------
pdr_ns_supg_ale_err_indi_explicit - to return error indicator for an element
  // 06.2011 - K.Michalik
----------------------------------------------------------*/
double pdr_ns_supg_ale_err_indi_explicit(	
                             /* returns error indicator for an element */
  int Problem_id_fake, 
  int El	/* in: element number */
    )
{

#define MAX_GAUSS 30 // should be "const int" but no compiling in msvc 9.0
  static const int X = 0, Y = 1, Z = 2, P = 3;
  double ni = 0, err = 0, err2, ni2 = 0;
  int i, ki, j, k;
  int problem_id, field_id, mesh_id, pdeg, num_shap, sol_vec_id, iaux, nreq;
  int base = 0;	/* type of basis functions for quadrilaterals */
  int el_nodes[MMC_MAXELVNO + 1];	/* list of nodes of El */
  double node_coor[3 * MMC_MAXELVNO];	/* coord of nodes of El */
  double dofs_loc[APC_MAXELSD];	/* element solution dofs */
  double xcoor[3];		/* global coord of gauss point */
  double u_val[PDC_MAXEQ];	/* computed solution */
  double u_x[PDC_MAXEQ];		/* gradient of computed solution */
  double u_y[PDC_MAXEQ];		/* gradient of computed solution */
  double u_z[PDC_MAXEQ];		/* gradient of computed solution */
  double base_phi[APC_MAXELVD];	/* basis functions */
  double base_dphix[APC_MAXELVD];	/* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];	/* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];	/* y-derivatives of basis function */
  double determ;
  double tmp;

  // ^ introduced by KM with explicit adapt
  int ngauss = 0;		/* number of gauss points */
  double xg[MAX_GAUSS * 3];	/* coordinates of gauss points in 3D */
  double wg[MAX_GAUSS];		/* gauss weights */
  double area;			/* volume for integration rule */
  double vol = 0.0, re, tmpE = 0.0, tmpVec[3] = { 0.0 }, vecNorm[3] = {
  0.0}, xneig[7] = {
  0.0};
  int ifa = 1;
  int fa[6] = {0}, faOrient[6] = {0}, faNeig[6] = {0}, faNeigSides[6] = {0};

/*++++++++++++++++ executable statements ++++++++++++++++*/


// explicit error estimate works currently only for ns_supg_problem
  pdv_ns_supg_ale_current_problem_id = PDC_NS_SUPG_ID;
  problem_id = PDC_NS_SUPG_ID;

/* get formulation parameters */
  //i=1; name=pdr_ctrl_i_params(problem_id,i);
  i=2; mesh_id=pdr_ctrl_i_params(problem_id,i);
  i=3; field_id=pdr_ctrl_i_params(problem_id,i);
  nreq = apr_get_nreq(field_id);

  //printf("\nErr indi Element %d:\n",El);

  /* find degree of polynomial and number of element scalar dofs */
  apr_get_el_pdeg(field_id, El, &pdeg);

  /* get the coordinates of the nodes of El in the right order */
  mmr_el_node_coor(mesh_id, El, el_nodes, node_coor);

  num_shap = apr_get_el_pdeg_numshap(field_id, El, &pdeg);
  //ndofs = num_shap*1;*

  /* prepare data for gaussian integration */
  base = apr_get_base_type(field_id, El);
  apr_set_quadr_3D(base, &pdeg, &ngauss, xg, wg);

  //assert(ngauss < MAX_GAUSS);

  /* get the most recent solution degrees of freedom */
  sol_vec_id = 1;
  apr_get_el_dofs(field_id, El, sol_vec_id, dofs_loc);

  // !!!
  re = 0.0;			//!!!!!! NO REYNOLDS IN HMT ?????????????
  //assert(re > 0.0);

  tmp = 0.0;
  err = 0.0;
  /// err = Error indicator for one element.
  // err = h_k^2 +
  err += mmr_el_hsize(mesh_id, El, NULL, NULL, NULL);	//h_k
  err *= err;			// ^2
  /// (integration over element volume begins)
  tmpVec[X] = 0.0;
  tmpVec[Y] = 0.0;
  tmpVec[Z] = 0.0;
  for (ki = 0; ki < ngauss; ++ki) {
    iaux = 2;
    determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, &xg[3 * ki], node_coor, 
			      dofs_loc, base_phi, base_dphix, base_dphiy, 
			      base_dphiz, xcoor, u_val, u_x, u_y, u_z, NULL);
    //vol = determ * wg[ki];
    // int{-nabla p_t - Re(u_t*nabla)u_t} +
    tmp += wg[ki] * (-u_x[P] - (re * (u_x[X] + u_y[Y] + u_z[Z])) * u_val[X]);
    tmp += wg[ki] * (-u_y[P] - (re * (u_x[X] + u_y[Y] + u_z[Z])) * u_val[Y]);
    tmp += wg[ki] * (-u_z[P] - (re * (u_x[X] + u_y[Y] + u_z[Z])) * u_val[Z]);
    // int{div u_t} +
    tmpE += wg[ki] * (u_x[X] + u_y[Y] + u_z[Z]);
  }
  err *= tmp;
  err += tmpE;
  /// (loop (and integration) over element faces)
  /// (TODO: preparation data...?)
  // 0.5* {sum for all faces of elem} h_f int{[n_f*(nabla u_t - p_t I)]}
  mmr_el_faces(mesh_id, El, fa, faOrient);
  tmp = 0.0;
  for (ifa = 1; ifa <= fa[0]; ++ifa) {
    mmr_fa_area(mesh_id, fa[ifa], &area, vecNorm); // linear characteristics size
    apr_set_quadr_2D(mmr_fa_type(mesh_id, fa[ifa]), base, &pdeg,&ngauss,xg,wg);
    mmr_fa_elem_coor(mesh_id, &xg[3 * ki], faNeig, faNeigSides, 0, NULL, NULL, 
		     xneig);
    tmpE = 0.0;
    for (ki = 0; ki < ngauss; ++ki) {
      // integration at gauss point ki
      determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, xneig, node_coor, 
				dofs_loc, base_phi, base_dphix, base_dphiy, 
				base_dphiz, xcoor, u_val, u_x, u_y, u_z, NULL);
      tmpVec[X] = u_x[X] - u_val[P];
      tmpVec[Y] = u_y[Y] - u_val[P];
      tmpVec[Z] = u_z[Z] - u_val[P];
      tmpE += wg[ki] * utr_vec3_dot(vecNorm, tmpVec);
    }
    tmp += tmpE * area;		// here is sum over faces
  }

  err += 0.5 * tmp;
  err = sqrt(err);

  return (err);
}

