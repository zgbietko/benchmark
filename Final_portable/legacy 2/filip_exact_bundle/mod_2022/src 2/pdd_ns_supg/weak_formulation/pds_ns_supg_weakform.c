/************************************************************************
File pds_ns_supg_ls_std_intf_util.c - utilities for the interface between 
  the problem dependent module and linear solver modules (direct and iterative)

Contains definitions of routines:
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER PROBLEM MODULES 

  pdr_ns_supg_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals in weak formulation
           (the procedure indicates which terms are non-zero in weak form)

  pdr_ns_supg_el_coeff - to return coefficients for internal integrals

  pdr_ns_supg_comp_el_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for an element       

  pdr_ns_supg_comp_fa_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for an element containing a given face


  pdr_ns_supg_compute_CFL - to compute and print local CFL numbers for element
  pdr_ns_supg_get_velocity_at_point - to provide the velocity and its
    gradient at a particular point with local coordinates within an element


------------------------------
History:
	02.2002 - Krzysztof Banas, initial version
	2011    - Przemyslaw Plaszewski
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)

*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<assert.h>

/* problem dependent module interface */
#include "pdh_intf.h"		/* IMPLEMENTS */
#include "pdh_control_intf.h"		/* IMPLEMENTS */
/* interface for all mesh manipulation modules */
#include "mmh_intf.h"		/* USES */
/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* utilities - including simple time measurement library */
#include "uth_intf.h"		/* USES */

#ifdef PARALLEL
/* interface of parallel mesh manipulation modules */
#include "mmph_intf.h"		/* USES */
/* interface for all parallel approximation modules */
#include "apph_intf.h"		/* USES */
/* interface for parallel communication modules */
#include "pch_intf.h"		/* USES */
#endif

/* problem module's types and functions */
#include "../include/pdh_ns_supg.h" // USES
/* types and functions related to problem structure */
#include "../include/pdh_ns_supg_problem.h"	/* USES */
/* weakform stabilization functions */
#include "../include/pdh_ns_supg_weakform.h"	/* USES */

#include "uth_log.h"

#ifndef NULL
#define NULL NULL
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(ptr) if(ptr!=NULL) free(ptr); ptr=NULL;
#endif

/*------------------------------------------------------------
  pdr_ns_supg_select_el_coeff_vect - to select coefficients 
                     returned to approximation routines for element integrals
------------------------------------------------------------*/
int pdr_ns_supg_select_el_coeff_vect( // returns success indicator
  int Problem_id,
  int *Coeff_vect_ind	/* out: coefficient indicator */
				   )
{

  // first indicate the vector has been filled
  Coeff_vect_ind[0] = 1;

  // Mval - mass matrix in time integration
  Coeff_vect_ind[1] = 1;
  // Axx
  Coeff_vect_ind[2] = 1;
  // Axy
  Coeff_vect_ind[3] = 1;
  // Axz
  Coeff_vect_ind[4] = 1;
  // Ayx
  Coeff_vect_ind[5] = 1;
  // Ayy
  Coeff_vect_ind[6] = 1;
  // Ayz
  Coeff_vect_ind[7] = 1;
  // Azx
  Coeff_vect_ind[8] = 1;
  // Azy
  Coeff_vect_ind[9] = 1;
  // Azz
  Coeff_vect_ind[10] = 1;
  // Bx
  Coeff_vect_ind[11] = 1;
  // By
  Coeff_vect_ind[12] = 1;
  // Bz
  Coeff_vect_ind[13] = 1;
  // Tx
  Coeff_vect_ind[14] = 1;
  // Ty
  Coeff_vect_ind[15] = 1;
  // Tz
  Coeff_vect_ind[16] = 1;
  // Cval
  Coeff_vect_ind[17] = 0; // no reaction terms in ns_supg problem
  // Lval - for time integration
  Coeff_vect_ind[18] = 1;
  // Qx
  Coeff_vect_ind[19] = 1;
  // Qy
  Coeff_vect_ind[20] = 1;
  // Qz
  Coeff_vect_ind[21] = 1;
  // Sval
  Coeff_vect_ind[22] = 1;

  return(1);
}

/*!!!!!! OLD OBSOLETE VERSION !!!!!!*/
/*------------------------------------------------------------
  pdr_ns_supg_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals in weak formulation
------------------------------------------------------------*/
double *pdr_ns_supg_select_el_coeff( 
     /* returns: pointer !=NULL to indicate that selection was done */
  int Problem_id,
  double **Mval,	/* out: mass matrix coefficient */
  double **Axx,double **Axy,double **Axz, /* out:diffusion coefficients, e.g.*/
  double **Ayx,double **Ayy,double **Ayz, /* Axy denotes scalar or matrix */
  double **Azx,double **Azy,double **Azz, /* related to terms with dv/dx*du/dy */
  /* second order derivatives in weak formulation (scalar for scalar problems */
  /* matrix for vector problems) */
  /* WARNING: if axy==NULL only diagonal (axx, ayy, azz) terms are considered */
  /* in apr_num_int_el */
  /* OPTIONS: */
  /* azz!=NULL, axy!=NULL - all a.. matrices must be specified */
  /* azz!=NULL, axy==NULL - axx, ayy, azz matrices must be specified */
  /* azz==NULL - axx, axy, ayx, ayy matrices must be specified */
  double **Bx,double **By,double **Bz,	/* out: convection coefficients */
  /* Bx denotes scalar or matrix related to terms with du/dx*v in weak form */
  double **Tx,double **Ty,double **Tz,	/* out: convection coefficients */
  /* Tx denotes scalar or matrix related to terms with u*dv/dx in weak form */
  double **Cval,/* out: reaction coefficients - for terms without derivatives */
  /*  in weak form (as usual: scalar for scalar problems, matrix for vectors) */
  double **Lval,/* out: rhs coefficient for time term, Lval denotes scalar */
  /* or matrix corresponding to time derivative - similar as mass matrix but  */
  /* with known solution at the previous time step (usually denoted by u_n) */
  double **Qx,/* out: rhs coefficients for terms with derivatives */
  double **Qy,/* Qy denotes scalar or matrix corresponding to terms with dv/dy */
  double **Qz,/* derivatives in weak formulation */
  double **Sval	/* out: rhs coefficients without derivatives (source terms) */
  )
{


/*++++++++++++++++ executable statements ++++++++++++++++*/

  //int nreq = pdr_ctrl_i_params(Problem_id, 5);
  int nreq = PDC_NS_SUPG_NREQ;
  if(nreq != pdr_ctrl_i_params(Problem_id,5)){
    printf("wrong parameter NS_SUPG_NREQ in pdr_ns_supg_select_el_coeff\n");
    printf("%d != %d. Exiting !!!",nreq, pdr_ctrl_i_params(Problem_id,5));
    exit(-1);
  }

  if(*Mval!=NULL) free(*Mval);
  *Mval = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Axx!=NULL) free(*Axx);
  *Axx = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Axy!=NULL) free(*Axy);
  *Axy = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Axz!=NULL) free(*Axz);
  *Axz = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Ayx!=NULL) free(*Ayx);
  *Ayx = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Ayy!=NULL) free(*Ayy);
  *Ayy = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Ayz!=NULL) free(*Ayz);
  *Ayz = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Azx!=NULL) free(*Azx);
  *Azx = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Azy!=NULL) free(*Azy);
  *Azy = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Azz!=NULL) free(*Azz);
  *Azz = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Bx!=NULL) free(*Bx);
  *Bx = (double *) malloc(nreq*nreq*sizeof(double));
  if(*By!=NULL) free(*By);
  *By = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Bz!=NULL) free(*Bz);
  *Bz = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Tx!=NULL) free(*Tx);
  *Tx = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Ty!=NULL) free(*Ty);
  *Ty = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Tz!=NULL) free(*Tz);
  *Tz = (double *) malloc(nreq*nreq*sizeof(double));
  if(*Cval!=NULL) free(*Cval); // no reaction terms in NS
  *Cval = NULL; // no reaction terms in NS
  if(*Lval!=NULL) free(*Lval);
  *Lval = (double *) malloc(nreq*sizeof(double));
  if(*Qx!=NULL) free(*Qx);
  *Qx = (double *) malloc(nreq*sizeof(double));
  if(*Qy!=NULL) free(*Qy);
  *Qy = (double *) malloc(nreq*sizeof(double));
  if(*Qz!=NULL) free(*Qz);
  *Qz = (double *) malloc(nreq*sizeof(double));
  if(*Sval!=NULL) free(*Sval);
  *Sval = (double *) malloc(nreq*sizeof(double));

  return (*Axx);
}

/*------------------------------------------------------------
pdr_ns_supg_el_coeff - to return coefficients at Gauss point for internal 
                       element integrals for ns_supg weak formulation
------------------------------------------------------------*/
int pdr_ns_supg_el_coeff(
  /* GENERIC arguments as in pdr_el_coeff */
  int Problem_id,
  int El_id,	/* in: element number */
  int Mat_num,	/* in: material number */
  double Hsize,	/* in: size of an element */
  int Pdeg,	/* in: local degree of polynomial */
  double *X_loc,      /* in: local coordinates of point within element */
  double *Base_phi,   /* in: basis functions */
  double *Base_dphix, /* in: x-derivatives of basis functions */
  double *Base_dphiy, /* in: y-derivatives of basis functions */
  double *Base_dphiz, /* in: z-derivatives of basis functions */
  double *Xcoor,	/* in: global coordinates of a point */
  double* Uk_val, 	/* in: computed solution from previous iteration */
  double* Uk_x, 	/* in: x-derivatives of components of Uk_val */
  double* Uk_y,   	/* in: y-derivatives of components of Uk_val */
  double* Uk_z,   	/* in: z-derivatives of components of Uk_val */
  double* Un_val, 	/* in: computed solution from previous time step */
  double* Un_x, 	/* in: x-derivatives of components of Un_val */
  double* Un_y,   	/* in: y-derivatives of components of Un_val */
  double* Un_z,   	/* in: z-derivatives of components of Un_val */
  double* Mval,	/* out: mass matrix coefficient */
  double *Axx, double *Axy, double *Axz, /* out:diffusion coefficients */
  double *Ayx, double *Ayy, double *Ayz, /* e.g. Axy denotes scalar or matrix */
  double *Azx, double *Azy, double *Azz, /* related to terms with dv/dx*du/dy */
  /* second order derivatives in weak formulation (scalar for scalar problems */
  /* matrix for vector problems) */
  /* WARNING: if axy==NULL only diagonal (axx, ayy, azz) terms are considered */
  /* in apr_num_int_el */
  /* OPTIONS: */
  /* azz!=NULL, axy!=NULL - all a.. matrices must be specified */
  /* azz!=NULL, axy==NULL - axx, ayy, azz matrices must be specified */
  /* azz==NULL - axx, axy, ayx, ayy matrices must be specified */
  double *Bx, double *By, double *Bz,	/* out: convection coefficients */
  /* Bx denotes scalar or matrix related to terms with du/dx*v in weak form */
  double *Tx, double *Ty, double *Tz,	/* out: convection coefficients */
  /* Tx denotes scalar or matrix related to terms with u*dv/dx in weak form */
  double *Cval,	/* out: reaction coefficients - for terms without derivatives */
  /*  in weak form (as usual: scalar for scalar problems, matrix for vectors) */
  double *Lval,	/* out: rhs coefficient for time term, Lval denotes scalar */
  /* or matrix corresponding to time derivative - similar as mass matrix but  */
  /* with known solution at the previous time step (usually denoted by u_n) */
  double *Qx, /* out: rhs coefficients for terms with derivatives */
  double *Qy, /* Qy denotes scalar or matrix corresponding to terms with dv/dy */
  double *Qz, /* derivatives in weak formulation */
  double *Sval,	/* out: rhs coefficients without derivatives (source terms) */
  /* arguments SPECIFIC to ns_supg */
  double Tk,			// in: temperature at the current point
  double Dynamic_viscosity,	// in: dynamic viscosity
  double Density,		// in: density
  double Reference_density,	// in: reference density
  double Reference_velocity,	// in: reference velocity
  double Delta_t,		// in
  double Implicitness_coeff,
  double Body_force_x,
  double Body_force_y,
  double Body_force_z
  )
{
	
	
	Reference_density = Density;
/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper field */
  int field_id = pdr_ctrl_i_params(Problem_id, 3);
  int mesh_id = apr_get_mesh_id(field_id);
  
  // nreq substituted as constant to allow compilers for constants propagation
  int nreq = PDC_NS_SUPG_NREQ;
#ifdef DEBUG
  if(nreq != apr_get_nreq(field_id)){
    printf("wrong parameter NS_SUPG_NREQ in pdr_ns_supg_el_coeff 1\n");
    printf("%d != %d. Exiting !!!",nreq, apr_get_nreq(field_id));
    exit(-1);
  }
  if(nreq != pdr_ctrl_i_params(Problem_id,5)){
    printf("wrong parameter NS_SUPG_NREQ in pdr_ns_supg_el_coeff 2\n");
    printf("%d != %d. Exiting !!!",nreq, pdr_ctrl_i_params(Problem_id,5));
    exit(-1);
  }
#endif

  int idofs;
  int num_shap = apr_get_el_pdeg_numshap(field_id, El_id, &Pdeg);

/*kbw
  if(El_id == 55){
  printf("In pdr_ns_supg_el_coeff\n");
  //printf("%d shape functions and derivatives: \n", num_shap);
  //for(idofs=0;idofs<num_shap;idofs++){
  //  printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
  //	  Base_phi[idofs],Base_dphix[idofs],Base_dphiy[idofs],Base_dphiz[idofs]);
  //}
  printf("solution and derivatives at previous iteration (u_k): \n");
  printf("uk_x - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Uk_val[0],Uk_x[0],Uk_y[0],Uk_z[0]);
  printf("uk_y - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Uk_val[1],Uk_x[1],Uk_y[1],Uk_z[1]);
  printf("uk_z - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Uk_val[2],Uk_x[2],Uk_y[2],Uk_z[2]);
  printf("solution and derivatives at previous time step (u_n): \n");
  printf("Un_x - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Un_val[0],Un_x[0],Un_y[0],Un_z[0]);
  printf("Un_y - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Un_val[1],Un_x[1],Un_y[1],Un_z[1]);
  printf("Un_z - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Un_val[2],Un_x[2],Un_y[2],Un_z[2]);
  }
/*kew */
  
  memset(Mval, 0, nreq*nreq*sizeof(double));
  memset(Axx, 0, nreq*nreq*sizeof(double));
  memset(Axy, 0, nreq*nreq*sizeof(double));
  memset(Axz, 0, nreq*nreq*sizeof(double));
  memset(Ayx, 0, nreq*nreq*sizeof(double));
  memset(Ayy, 0, nreq*nreq*sizeof(double));
  memset(Ayz, 0, nreq*nreq*sizeof(double));
  memset(Azx, 0, nreq*nreq*sizeof(double));
  memset(Azy, 0, nreq*nreq*sizeof(double));
  memset(Azz, 0, nreq*nreq*sizeof(double));
  memset(Bx, 0, nreq*nreq*sizeof(double));
  memset(By, 0, nreq*nreq*sizeof(double));
  memset(Bz, 0, nreq*nreq*sizeof(double));
  memset(Tx, 0, nreq*nreq*sizeof(double));
  memset(Ty, 0, nreq*nreq*sizeof(double));
  memset(Tz, 0, nreq*nreq*sizeof(double));
  memset(Lval, 0, nreq*sizeof(double));
  memset(Qx, 0, nreq*sizeof(double));
  memset(Qy, 0, nreq*sizeof(double));
  memset(Qz, 0, nreq*sizeof(double));
  memset(Sval, 0, nreq*sizeof(double));

  /*! ----------------------------------------------------------------------! */
  /*! ------------ CALCULATE ELEM. "SIZE" (h_k) AT GAUSS POINT----------- --! */
  /*! ----------------------------------------------------------------------! */
  double m_k = 1.0/3.0; // should be changed for higher order elements
  double norm_u = 
    sqrt(Uk_val[0]*Uk_val[0]+Uk_val[1]*Uk_val[1]+Uk_val[2]*Uk_val[2]);

  // h_k computations: FRANCA, TEZDUYAR
  double h_k = 0.0; 

  if(norm_u < 1.0e-6){ // when there is no velocity field inside the element
    h_k = mmr_el_hsize(mesh_id,El_id,NULL,NULL,NULL); //take standard element size
  }
  else{ // take element size in the direction of velocity
    // this definition may lead to oscillations - change then to standard size
    for (idofs = 0; idofs < num_shap; idofs++) {
      h_k += fabs(Uk_val[0] * Base_dphix[idofs] + 
		  Uk_val[1] * Base_dphiy[idofs] + 
		  Uk_val[2] * Base_dphiz[idofs]);
    }
    // h_k = norm_u / h_k; ???
    h_k = 2.0 * norm_u / h_k;
  }

  // OTHER POSSIBILITIES EXIST FOR COMPUTING h

  /*! ----------------------------------------------------------------------! */
  /*! --------- CALCULATE STABILIZATION COEFFS FRANCA/FREY -- ----------- --! */
  /*! ----------------------------------------------------------------------! */

  double reynolds_local = (m_k * norm_u * h_k) / (4.0 * Dynamic_viscosity/Reference_density);  
  double ksi_reynolds;
  if(reynolds_local<1.0 && reynolds_local>=0.0 ) ksi_reynolds = reynolds_local;
  else if(reynolds_local>=1.0) ksi_reynolds = 1.0;
  else {printf("ERROR: Reynolds < 0 (pdr_ns_supg_el_coeff)\n"); exit(-1);}

  // sigma has dimension [m^2/s], has to be taken into account when calculating stabilization terms
  double sigma_franca = norm_u * h_k * ksi_reynolds;
  
  // tau has dimension [s], has to be taken into account when calculating stabilization terms
  double tau_franca;
  if(reynolds_local<1.0) {
    tau_franca = (h_k * h_k * m_k) / (8.0 * Dynamic_viscosity/Reference_density);
  } else {
    tau_franca = h_k / (2.0 * norm_u);
  }

 /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
  /*! ----------------------------------------------------------------------! */
  /*! ---------- CALCULATE STABILIZATION COEFFS TEZDUYAR ---------------- --! */
  /*! ----------------------------------------------------------------------! */
//!!!!!!!!!!!!! Tezduyar, Hughes version: tau_pres based on h_size and u_ref
  //double h_size = mmr_el_hsize(mesh_id, El_id, NULL, NULL, NULL);
  //double u_ref = Reference_velocity;
  //double reynolds_ref = (m_k * u_ref * h_size) / (4.0 * Dynamic_viscosity/Reference_density);
  
  //double tau_tezd_pres=0.0;
  //if(reynolds_ref<1.0) {
  //  tau_tezd_pres = (h_size * h_size * m_k) / (8.0 * Dynamic_viscosity/Reference_density);
  //} else {
  //  tau_tezd_pres = h_k / (2.0 * u_ref);
  //}


  /*!!!!!!!!!!!!! HANSBO version !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
  /*! ----------------------------------------------------------------------! */
  /*! ---------- CALCULATE STABILIZATION COEFFS ------------------------- --! */
  /*! ----------------------------------------------------------------------! */
  //double tau_hansbo = h_k / (1.0 + norm_u);
  //double sigma_hansbo = h_k;
  //double tau_hansbo = h_size / (1.0 + norm_u);
  //double sigma_hansbo = h_size;

  /*!!!!!!!!!!!!! yet another Tazduyar version !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
  /*! ----------------------------------------------------------------------! */
  /*! ---------- CALCULATE STABILIZATION COEFFS ------------------------- --! */
  /*! ----------------------------------------------------------------------! */
  //double h_ugn = h_k;

  //double tau_sugn1 = 2.0*norm_u / (h_ugn + 1.0e-6 );

  //double tau_sugn2 = 2.0 / Delta_t ;

  //double tau_sugn3 = (8.0 * Dynamic_viscosity/Reference_density) / (1.0/3.0 * h_ugn * h_ugn);

  //double tau_tezd = 1.0 / sqrt( tau_sugn1*tau_sugn1 +
  //			       tau_sugn2*tau_sugn2 + 
  //			       tau_sugn3*tau_sugn3 );


/*kbw
  if(El_id==5||El_id==55||El_id==555){
  
    printf("\nFRANCA h_k %lf, norm_u %lf, tau %lf, sigma %lf\n",
	   h_k, norm_u, tau_franca, sigma_franca);
    printf("TEZD   h_k %lf, norm_u %lf, tau %lf, sigma %lf\n",
	   h_k, norm_u, tau_tezd, sigma_ugn);
    printf("HANSBO h_k %lf, norm_u %lf, tau %lf, sigma %lf \n",
	   h_size, norm_u, tau_hansbo, sigma_hansbo);
    printf("PRES   h_k %lf, norm_u %lf, tau_pres %lf, sigma %lf\n",
	   h_size, u_ref, tau_tezd_pres, sigma_franca);

  }
/*kew */



/************ THE FINAL CHOICE OF COEFFICIENTS *****************************/
  //double tau = tau_tezd; // square of sum of 3 norms
  double tau = tau_franca; // selection of the same norms 
  //double tau = tau_hansbo; // just h/norm_u

  //double tau_pres = tau_tezd_pres; //special based on U_ref and standard h_size
  double tau_pres = tau_franca; 
  //double tau_pres = tau_tezd;
  //double tau_pres = tau_hansbo;

  double sigma = sigma_franca + 1.0e-6; // to make system matrix non-singular
  //double sigma = sigma_franca; // special norm_u * h_k * reynolds_local
  //double sigma = sigma_hansbo; // just h

  /*! ----------------------------------------------------------------------! */
  /*! SPLITTING OF TERMS INTO LHS AND RHS FOR ALPHA TIME INTEGRATION SCHEME ! */
  /*! ----------------------------------------------------------------------! */
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
  // 0. PRESSURE TERMS ARE NOT SPLIT
  // several strategies:
  // 1. only real terms are split, stabilization terms are treated implicitly

  //printf("implicitness alpha = %lf\n",Implicitness_coeff);

  /*! ----------------------------------------------------------------------! */
  /*! ------ CALCULATE MATRICES OF WEAK FORM COEFFICIENTS FOR LHS ----------! */
  /*! ----------------------------------------------------------------------! */

  /*!!! ---- EACH matrix has to have proper dimension so that the corrersponding --- !!!*/
  /*!!! ---- term in the weak formulation has the final dimension [kg/(m*s^3)] ----- !!!*/
     
//
//  Matrix (1): Reference_density*(u_dt, v)  - Mval [kg/(m^3*s)]
//
//  [kg/(m^3*s)]*[m/s]*[m/s] = [kg/(m*s^3)] - OK
//
//  Reference_density/Delta_t * (u_x*v_x + u_y*v_y + u_z*v_z)
//
//  Reference_density/Delta_t [kg/m^3 * 1/s] = [kg/(m^3*s)] - OK
//



  double dxdeta[9]={0},Xcoor_new[3]={0},Node_coor[12]={0},Eta[3]={0};
  
  double old_GaussP[3]={0};
  
  
  double dxdeta1[9]={0},dxdeta1_2[9]={0},det1,det,det1_2=1,temp1_2=1,Node_coor1[12]={0};//old mesh - Node_coor1,dxdeta1 ,dxdeta1_2,det1_2 - J ->old to new
  int idGP=0,X=0,Y=1,Z=2;
  
  
  //int ii=0;
  //for(;ii<30;ii++){ std::cout<<X_loc[ii]<<" ";}
//printf("Pdeg %d %f ",num_shap,X_loc[ii]);
/*

  if(X_loc[3] == 0.0){idGP=3;Eta[0]=X_loc[0];Eta[1]=X_loc[1];Eta[2]=X_loc[2];}
  else if(X_loc[6]==0.0){idGP=2;Eta[0]=X_loc[3];Eta[1]=X_loc[4];Eta[2]=X_loc[5];}
  else if(X_loc[9]==0.0){idGP=1;Eta[0]=X_loc[6];Eta[1]=X_loc[7];Eta[2]=X_loc[8];}
  else if(X_loc[12]>=0.0){idGP=0;Eta[0]=X_loc[9];Eta[1]=X_loc[10];Eta[2]=X_loc[11];}
  else{printf("X_loc %f !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", X_loc[13]);} 
  
  mmr_get_coor_from_montion_element(El_id,idGP,Node_coor,1);
  
  dxdeta[0]=Node_coor[3+X]-Node_coor[0+X];
  dxdeta[3]=Node_coor[3+Y]-Node_coor[0+Y];
  dxdeta[6]=Node_coor[3+Z]-Node_coor[0+Z];
  
  dxdeta[1]=Node_coor[6+X]-Node_coor[0+X];
  dxdeta[4]=Node_coor[6+Y]-Node_coor[0+Y];
  dxdeta[7]=Node_coor[6+Z]-Node_coor[0+Z];
  
  dxdeta[2]=Node_coor[9+X]-Node_coor[0+X];
  dxdeta[5]=Node_coor[9+Y]-Node_coor[0+Y];
  dxdeta[8]=Node_coor[9+Z]-Node_coor[0+Z];
  
  Xcoor_new[X]=Node_coor[0]+Eta[X]*dxdeta[0]+Eta[Y]*dxdeta[1]+Eta[Z]*dxdeta[2];
  Xcoor_new[Y]=Node_coor[1]+Eta[X]*dxdeta[3]+Eta[Y]*dxdeta[4]+Eta[Z]*dxdeta[5];
  Xcoor_new[Z]=Node_coor[2]+Eta[X]*dxdeta[6]+Eta[Y]*dxdeta[7]+Eta[Z]*dxdeta[8];
  
  
  
  
  mmr_get_coor_from_montion_element(El_id,idGP,Node_coor1,0);
  
  dxdeta1[0]=Node_coor1[3+X]-Node_coor1[0+X];	
  dxdeta1[3]=Node_coor1[3+Y]-Node_coor1[0+Y];	
  dxdeta1[6]=Node_coor1[3+Z]-Node_coor1[0+Z];	
  
  dxdeta1[1]=Node_coor1[6+X]-Node_coor1[0+X];	
  dxdeta1[4]=Node_coor1[6+Y]-Node_coor1[0+Y];	
  dxdeta1[7]=Node_coor1[6+Z]-Node_coor1[0+Z];	
  
  dxdeta1[2]=Node_coor1[9+X]-Node_coor1[0+X];	
  dxdeta1[5]=Node_coor1[9+Y]-Node_coor1[0+Y];	
  dxdeta1[8]=Node_coor1[9+Z]-Node_coor1[0+Z];	
  
  
  dxdeta1_2[0]= dxdeta[0]/dxdeta1[0];
  dxdeta1_2[1]= dxdeta[1]/dxdeta1[1];
  dxdeta1_2[2]= dxdeta[2]/dxdeta1[2];
  
  dxdeta1_2[3]= dxdeta[3]/dxdeta1[3];
  dxdeta1_2[4]= dxdeta[4]/dxdeta1[4];
  dxdeta1_2[5]= dxdeta[5]/dxdeta1[5];
  
  dxdeta1_2[6]= dxdeta[6]/dxdeta1[6];
  dxdeta1_2[7]= dxdeta[7]/dxdeta1[7];
  dxdeta1_2[8]= dxdeta[8]/dxdeta1[8];
  
  

  
  //dxdeta1_2,det1_2 - J ->new to old
  
  dxdeta1_2[0]= dxdeta1[0]/dxdeta[0];
  dxdeta1_2[1]= dxdeta1[1]/dxdeta[1];
  dxdeta1_2[2]= dxdeta1[2]/dxdeta[2]; 
  
  dxdeta1_2[3]= dxdeta1[3]/dxdeta[3];
  dxdeta1_2[4]= dxdeta1[4]/dxdeta[4];
  dxdeta1_2[5]= dxdeta1[5]/dxdeta[5];
  
  dxdeta1_2[6]= dxdeta1[6]/dxdeta[6];
  dxdeta1_2[7]= dxdeta1[7]/dxdeta[7];
  dxdeta1_2[8]= dxdeta1[8]/dxdeta[8];  
  
  
  
  det = dxdeta[0]*dxdeta[4]*dxdeta[8] + dxdeta[3]*dxdeta[7]*dxdeta[2] + dxdeta[6]*dxdeta[1]*dxdeta[5] -
		 dxdeta[2]*dxdeta[4]*dxdeta[6] - dxdeta[5]*dxdeta[7]*dxdeta[0] - dxdeta[8]*dxdeta[1]*dxdeta[3];
		 
  det1 = dxdeta1[0]*dxdeta1[4]*dxdeta1[8] + dxdeta1[3]*dxdeta1[7]*dxdeta1[2] + dxdeta1[6]*dxdeta1[1]*dxdeta1[5] -
		 dxdeta1[2]*dxdeta1[4]*dxdeta1[6] - dxdeta1[5]*dxdeta1[7]*dxdeta1[0] - dxdeta1[8]*dxdeta1[1]*dxdeta1[3];
		   
	det1_2 = det1/det;


temp1_2 = det1_2;
   int znakA=-1;


*/   
 //if(det1_2!=1){printf("%lf \n",det1_2);}
 int znakA=0;
  det1_2=1;	
   

  old_GaussP[0]= (Xcoor[0]-Xcoor_new[0])/Delta_t;
  old_GaussP[1]= (Xcoor[1]-Xcoor_new[1])/Delta_t;
  old_GaussP[2]= (Xcoor[2]-Xcoor_new[2])/Delta_t;
  
  //}
  
  
  old_GaussP[0]*=znakA;
  old_GaussP[1]*=znakA;
  old_GaussP[2]*=znakA;

double old_GaussP1[3];

 old_GaussP1[0] =old_GaussP[0];
 old_GaussP1[1] =old_GaussP[1];
 old_GaussP1[2] =old_GaussP[2];




  Mval[0+nreq*0] += Reference_density/Delta_t; // u_x*v_x
  Mval[1+nreq*1] += Reference_density/Delta_t; // u_y*v_y
  Mval[2+nreq*2] += Reference_density/Delta_t; // u_z*v_z
  Mval[3+nreq*3] += 0; // - no time derivative of pressure
		   
		   
//
//  Matrix (2): Reference_density*((grad*u)u, v) - Bx, By, Bz [kg/(m^2*s)]
//
//  [kg/(m^2*s)]*[1/s]*[m/s] = [kg/(m*s^3)] - OK
//
//     + Reference_density * (
//         v_x*(uk_x*u_xdx + uk_y*u_xdy + uk_z*u_xdz) + 
//         v_y*(uk_x*u_ydx + uk_y*u_ydy + uk_z*u_ydz) +
//         v_z*(uk_x*u_zdx + uk_y*u_zdy + uk_z*u_zdz)
//       )
//
//  Reference_density * U = [kg/m^3 * m/s] = [kg/(m^2*s)] - OK
//
  /* Bx[0+nreq*0] += Reference_density * Uk_val[0]; // v_x*u_xdx */
  /* Bx[1+nreq*1] += Reference_density * Uk_val[0]; // v_y*u_ydx */
  /* Bx[2+nreq*2] += Reference_density * Uk_val[0]; // v_z*u_zdx */
  
  /* By[0+nreq*0] += Reference_density * Uk_val[1]; // v_x*u_xdy */
  /* By[1+nreq*1] += Reference_density * Uk_val[1]; // v_y*u_ydy */
  /* By[2+nreq*2] += Reference_density * Uk_val[1]; // v_z*u_zdy */
  
  /* Bz[0+nreq*0] += Reference_density * Uk_val[2]; // v_x*u_xdy */
  /* Bz[1+nreq*1] += Reference_density * Uk_val[2]; // v_y*u_ydy */
  /* Bz[2+nreq*2] += Reference_density * Uk_val[2]; // v_z*u_zdy */

  // take into account alpha time integration scheme
  
  det1_2 = temp1_2;
  //det1_2 = 1;
  
  Bx[0+nreq*0] += Implicitness_coeff * Reference_density * (Uk_val[0]-old_GaussP[0])*det1_2; // v_x*u_xdx
  Bx[1+nreq*1] += Implicitness_coeff * Reference_density * (Uk_val[0]-old_GaussP[0])*det1_2; // v_y*u_ydx
  Bx[2+nreq*2] += Implicitness_coeff * Reference_density * (Uk_val[0]-old_GaussP[0])*det1_2; // v_z*u_zdx
  
  By[0+nreq*0] += Implicitness_coeff * Reference_density * (Uk_val[1]-old_GaussP[1])*det1_2; // v_x*u_xdy
  By[1+nreq*1] += Implicitness_coeff * Reference_density * (Uk_val[1]-old_GaussP[1])*det1_2; // v_y*u_ydy
  By[2+nreq*2] += Implicitness_coeff * Reference_density * (Uk_val[1]-old_GaussP[1])*det1_2; // v_z*u_zdy
  
  Bz[0+nreq*0] += Implicitness_coeff * Reference_density * (Uk_val[2]-old_GaussP[2])*det1_2; // v_x*u_xdz
  Bz[1+nreq*1] += Implicitness_coeff * Reference_density * (Uk_val[2]-old_GaussP[2])*det1_2; // v_y*u_ydz
  Bz[2+nreq*2] += Implicitness_coeff * Reference_density * (Uk_val[2]-old_GaussP[2])*det1_2; // v_z*u_zdz


  // Sval [kg/(m^2*s^2)] ([kg/(m^2*s^2)]*[m/s] = [kg/(m*s^3)] - OK)
  // B_i*U_,i = [kg/(m^2*s) * 1/s] = [kg/(m^2*s^2)] - OK
  if(Implicitness_coeff<1.0){
    double temp;

    temp = Un_val[0]*Un_x[0] + Un_val[1]*Un_y[0] + Un_val[2]*Un_z[0];
    Sval[0] += (Implicitness_coeff-1.0) * Reference_density * temp;

    temp = Un_val[0]*Un_x[1] + Un_val[1]*Un_y[1] + Un_val[2]*Un_z[1];
    Sval[1] += (Implicitness_coeff-1.0) * Reference_density * temp;

    temp = Un_val[0]*Un_x[2] + Un_val[1]*Un_y[2] + Un_val[2]*Un_z[2];
    Sval[2] += (Implicitness_coeff-1.0) * Reference_density * temp;
  }


//
//  Matrix (3): (2*Dynamic_viscosity*e(u), e(v)) - A.. matrices [kg/(m*s)]
//
//  [kg/(m*s)]*[1/s]*[1/s] = [kg/(m*s^3)] - OK
//
//       // the same results assuming Dynamic_viscosity*(u_i,j + u_j,i)*w_i,j
//       // !!! Dynamic_viscosity is DYNAMIC visocosity (not kinematic = Dynamic_viscosity/rho)
//
//       // Dynamic_viscosity[kg/m/s], [g/cm/s] - OK
//       +                          
//       2.0*Dynamic_viscosity*(
//         u_xdx*v_xdx + u_ydy*v_ydy + u_zdz*v_zdz +
//         0.5*(u_ydx+u_xdy)*0.5*(v_ydx+v_xdy) +
//         0.5*(u_zdx+u_xdz)*0.5*(v_zdx+v_xdz) +
//         0.5*(u_zdy+u_ydz)*0.5*(v_zdy+v_ydz)
//       )
// convention: axy[jeq*nreq+ieq] (axy[ieq,jeq]) * dv[ieq]/dx * du[jeq]/dy
//
  /* Axx[0+nreq*0] += 2.0*Dynamic_viscosity; // u_xdx*v_xdx  */
  /* Axx[1+nreq*1] += Dynamic_viscosity; // u_ydx*v_ydx  */
  /* Axx[2+nreq*2] += Dynamic_viscosity; // u_zdx*v_zdx  */
  
  /* Ayy[0+nreq*0] += Dynamic_viscosity; // u_xdy*v_xdy  */
  /* Ayy[1+nreq*1] += 2.0*Dynamic_viscosity; // u_ydy*v_ydy  */
  /* Ayy[2+nreq*2] += Dynamic_viscosity; // u_zdy*v_zdy  */
  
  /* Azz[0+nreq*0] += Dynamic_viscosity; // u_xdz*v_xdz  */
  /* Azz[1+nreq*1] += Dynamic_viscosity; // u_ydz*v_ydz  */
  /* Azz[2+nreq*2] += 2.0*Dynamic_viscosity; // u_zdz*v_zdz  */
  
  /* Ayx[0+nreq*1] += Dynamic_viscosity; // u_ydx*v_xdy  */
  /* Axy[1+nreq*0] += Dynamic_viscosity; // u_xdy*v_ydx  */
  /* Azx[0+nreq*2] += Dynamic_viscosity; // u_zdx*v_xdz  */
  /* Axz[2+nreq*0] += Dynamic_viscosity; // u_xdz*v_zdx  */
  /* Azy[1+nreq*2] += Dynamic_viscosity; // u_zdy*v_ydz  */
  /* Ayz[2+nreq*1] += Dynamic_viscosity; // u_ydz*v_zdy  */

  // take into account alpha time integration scheme
  
  det1_2 = temp1_2;
  //det1_2 = 1;
  Axx[0+nreq*0] += Implicitness_coeff * 2.0*Dynamic_viscosity*det1_2; // u_xdx*v_xdx
  Axx[1+nreq*1] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_ydx*v_ydx
  Axx[2+nreq*2] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_zdx*v_zdx
  
  Ayy[0+nreq*0] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_xdy*v_xdy
  Ayy[1+nreq*1] += Implicitness_coeff * 2.0*Dynamic_viscosity*det1_2; // u_ydy*v_ydy
  Ayy[2+nreq*2] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_zdy*v_zdy
  
  Azz[0+nreq*0] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_xdz*v_xdz
  Azz[1+nreq*1] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_ydz*v_ydz
  Azz[2+nreq*2] += Implicitness_coeff * 2.0*Dynamic_viscosity*det1_2; // u_zdz*v_zdz
  
  Ayx[0+nreq*1] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_ydx*v_xdy
  Axy[1+nreq*0] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_xdy*v_ydx
  Azx[0+nreq*2] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_zdx*v_xdz
  Axz[2+nreq*0] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_xdz*v_zdx
  Azy[1+nreq*2] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_zdy*v_ydz
  Ayz[2+nreq*1] += Implicitness_coeff * Dynamic_viscosity*det1_2; // u_ydz*v_zdy

  
  
  if(Implicitness_coeff<1.0){

    // Q [kg/(m*s^2)] ([kg/(m*s^2)]*[1/s] = [kg/(m*s^3)] - OK)
    // A.. * U_,i = [kg/(m*s)]*[1/s] = [kg/(m*s^2)] - OK)

    Qx[0] += (Implicitness_coeff-1.0) * 2.0*Dynamic_viscosity * Un_x[0];
    Qx[1] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_x[1];
    Qx[2] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_x[2];

    Qy[0] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_y[0];
    Qy[1] += (Implicitness_coeff-1.0) * 2.0*Dynamic_viscosity * Un_y[1];
    Qy[2] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_y[2];

    Qz[0] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_z[0];
    Qz[1] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_z[1];
    Qz[2] += (Implicitness_coeff-1.0) * 2.0*Dynamic_viscosity * Un_z[2];

    Qy[0] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_x[1];
    Qx[1] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_y[0];
    Qz[0] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_x[2];
    Qx[2] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_z[0];
    Qz[1] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_y[2];
    Qy[2] += (Implicitness_coeff-1.0) * Dynamic_viscosity * Un_z[1];

  }


// assuming that velocity field is divergence free we can obtain simpler form
// (but we do not enforce additionally divergence free condition)
//          Axx[0+nreq*0] += Dynamic_viscosity // u_xdx*v_xdx 
//          Axx[1+nreq*1] += Dynamic_viscosity // u_ydx*v_ydx 
//          Axx[2+nreq*2] += Dynamic_viscosity // u_zdx*v_zdx 
//
//          Ayy[0+nreq*0] += Dynamic_viscosity // u_xdy*v_xdy 
//          Ayy[1+nreq*1] += Dynamic_viscosity // u_ydy*v_ydy 
//          Ayy[2+nreq*2] += Dynamic_viscosity // u_zdy*v_zdy 
//
//          Azz[0+nreq*0] += Dynamic_viscosity // u_xdz*v_xdz 
//          Azz[1+nreq*1] += Dynamic_viscosity // u_ydz*v_ydz 
//          Azz[2+nreq*2] += Dynamic_viscosity // u_zdz*v_zdz 
//


//
//  Matrix (4): -(grad*v, p)  - Tx, Ty, Tz matrices - nondimensional for pressure terms
//
//  []*[kg/(m*s^2) * 1/s] = [kg/(m*s^3)] - OK
//
//          
//    -1.0 * p * (v_xdx + v_ydy + v_zdz)
//
  // final choice - not split in time integration!!!
  Tx[0+nreq*3]  += -1.0; // p*v_xdx
  Ty[1+nreq*3]  += -1.0; // p*v_ydy
  Tz[2+nreq*3]  += -1.0; // p*v_zdz

  // version for splitting in time integration!!!
  //Tx[0+nreq*3]  += -1.0*Implicitness_coeff; // p*v_xdx
  //Ty[1+nreq*3]  += -1.0*Implicitness_coeff; // p*v_ydy
  //Tz[2+nreq*3]  += -1.0*Implicitness_coeff; // p*v_zdz
  //if(Implicitness_coeff<1.0){
  //  Qx[0] += -1.0*(Implicitness_coeff-1.0) * Un_val[3];
  //  Qy[1] += -1.0*(Implicitness_coeff-1.0) * Un_val[3];
  //  Qz[2] += -1.0*(Implicitness_coeff-1.0) * Un_val[3];
  //}

// Matrix (5):  -Reference_density*(grad*u, q) - Bx, By, Bz matrices [kg/m^3]
//
// !!!!!!!!!!!!!! new version for q [m^2/s^2] (same as pressure/density) 
//
// !!!!!! dimension q = [m^2/s^2] -> [kg/m^3]*[1/s]*[m^2/s^2] =  [kg/(m*s^3)] - OK
//

  // final choice - not split in time integration!!!
  
  
    det1_2 = temp1_2;
  //det1_2 = 1;
  Bx[3+nreq*0] += -Reference_density*det1_2; // q*u_xdx
  By[3+nreq*1] += -Reference_density*det1_2; // q*u_ydy
  Bz[3+nreq*2] += -Reference_density*det1_2; // q*u_zdz


// Matrix (5):  -Reference_density*(grad*u, q) - Bx, By, Bz matrices - non-dimensional
//
// !!!!!!!!!!!!!! old version for q [kg/(m*s^2)] (same as pressure) 
//
// !!!!!! dimension q = [kg/(m*s^2)] -> [1/s]*[kg/(m*s^2)] =  [kg/(m*s^3)] - OK
//
//   VERSION with -(grad*u, q) not integrated (observe "-" sign !!!)
//     -1.0 * q * (u_xdx + u_ydy + u_zdz)
//
// !!!!!!!!!!!!! pdd_ns_supg_new - Franca and Frey version: -1.0
//  Bx[3+nreq*0] += -1.0; // q*u_xdx
//  By[3+nreq*1] += -1.0; // q*u_ydy
//  Bz[3+nreq*2] += -1.0; // q*u_zdz
//
//   VERSION with (grad*u, q) not integrated (observe "+" sign !!!)
//     1.0 * q * (u_xdx + u_ydy + u_zdz)
//
//!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0
//  Bx[3+nreq*0] += 1.0; // q*u_xdx
//  By[3+nreq*1] += 1.0; // q*u_ydy
//  Bz[3+nreq*2] += 1.0; // q*u_zdz

  // final choice - not split in time integration!!!
  //old//  Bx[3+nreq*0] += -1.0; // q*u_xdx
  //old//  By[3+nreq*1] += -1.0; // q*u_ydy
  //old//  Bz[3+nreq*2] += -1.0; // q*u_zdz


  // version for splitting in time integration!!!
  //Bx[3+nreq*0] += -1.0*Implicitness_coeff; // q*u_xdx
  //By[3+nreq*1] += -1.0*Implicitness_coeff; // q*u_ydy
  //Bz[3+nreq*2] += -1.0*Implicitness_coeff; // q*u_zdz
  //if(Implicitness_coeff<1.0){
  //  double temp;
  //  temp = Un_x[0] + Un_y[1] + Un_z[2];
  //  Sval[3] += -1.0*(Implicitness_coeff-1.0) * temp;
  //}

// VERSION with (grad*u, q) term (observe "+" sign !!!) INTEGRATED BY PARTS
//          - observe nice symmetry with the term (grad*v, p)
//
//     -1.0 * (dq/dx*u_x + dq/dy*u_y + dq/dz*u_z) 
//                                  - Tx, Ty, Tz matrices of coefficients
//
//  Tx[3+nreq*0]  += -1.0; // dq/dx*u_x
//  Ty[3+nreq*1]  += -1.0; // dq/dy*u_y
//  Tz[3+nreq*2]  += -1.0; // dq/dz*u_z
//
// ALE:
// Qx[3] += -1.0 * U_GaussP_x // dq/dx
// Qy[3] += -1.0 * U_GaussP_y // dq/dy
// Qz[3] += -1.0 * U_GaussP_z // dq/dz
  

//
//   Matrix (6): stabilization part: Reference_density * (u_dt, res) - Tx, Ty, Tz [kg/(m^2*s)]
//
//             [kg/(m^2*s)][m/s]*[1/s] =  [kg/(m*s^3)] - OK
//
//     +
//   Reference_density * (tau/Delta_t)*( 
//     u_x* /*res*/(uk_x*v_xdx + uk_y*v_xdy + uk_z*v_xdz) +
//     u_y* /*res*/(uk_x*v_ydx + uk_y*v_ydy + uk_z*v_ydz) + 
//     u_z* /*res*/(uk_x*v_zdx + uk_y*v_zdy + uk_z*v_zdz)
//   )
//
//  [kg/m^3]*[s]*[1/s]*[m/s] = [kg/(m^2*s)]
//

  det1_2 = temp1_2;
  det1_2 = 1;
  /*
 old_GaussP[0] =old_GaussP1[0];
 old_GaussP[1] =old_GaussP1[1];
 old_GaussP[2] =old_GaussP1[2];
 */
 old_GaussP[0] =0;
 old_GaussP[1] =0;
 old_GaussP[2] =0;
 
 
  Tx[0+nreq*0] += Reference_density * (tau/Delta_t) * (Uk_val[0]- old_GaussP[0])*det1_2; // u_x*v_xdx
  Tx[1+nreq*1] += Reference_density * (tau/Delta_t) * (Uk_val[0]- old_GaussP[0])*det1_2; // u_y*v_ydx
  Tx[2+nreq*2] += Reference_density * (tau/Delta_t) * (Uk_val[0]- old_GaussP[0])*det1_2; // u_z*v_zdx
  
  Ty[0+nreq*0] += Reference_density * (tau/Delta_t) * (Uk_val[1]- old_GaussP[1])*det1_2; // u_x*v_xdy
  Ty[1+nreq*1] += Reference_density * (tau/Delta_t) * (Uk_val[1]- old_GaussP[1])*det1_2; // u_y*v_ydy
  Ty[2+nreq*2] += Reference_density * (tau/Delta_t) * (Uk_val[1]- old_GaussP[1])*det1_2; // u_z*v_zdy
  
  Tz[0+nreq*0] += Reference_density * (tau/Delta_t) * (Uk_val[2]- old_GaussP[2])*det1_2; // u_x*v_xdz
  Tz[1+nreq*1] += Reference_density * (tau/Delta_t) * (Uk_val[2]- old_GaussP[2])*det1_2; // u_y*v_ydz
  Tz[2+nreq*2] += Reference_density * (tau/Delta_t) * (Uk_val[2]- old_GaussP[2])*det1_2; // u_z*v_zdz
  

//
//   Matrix (6): stabilization part: Reference_density * (u_dt, res) - Tx, Ty, Tz [kg/m^3]
//
// new version - q [m^2/s^2] -> [kg/m^3]*[m/s]*[m/s^2] = [kg/(m*s^3)] - OK
//
//     +
//   Reference_density * (tau/Delta_t)*( 
//     u_x* /*res*/(q_dx) +
//     u_y* /*res*/(q_dy) + 
//     u_z* /*res*/(q_dz)
//   )
//
//  [kg/m^3]*[s]*[1/s] = [kg/m^3] - OK
  det1_2 = temp1_2;
  det1_2 = 1; 
  Tx[3+nreq*0] += -Reference_density * (tau_pres/Delta_t)*det1_2;
  Ty[3+nreq*1] += -Reference_density * (tau_pres/Delta_t)*det1_2;
  Tz[3+nreq*2] += -Reference_density * (tau_pres/Delta_t)*det1_2;
	

//
//   Matrix (6): stabilization part: Reference_density * (u_dt, res) - Tx, Ty, Tz 
//
// old version - q [kg/(m*s^2)] -> [m/s]*[kg/(m^2*s^2)] = [kg/m*s^3] - OK
//     +
//   Reference_density * (tau/Delta_t)*( 
//     u_x* /*res*/(q_dx/Reference_density) +
//     u_y* /*res*/(q_dy/Reference_density) + 
//     u_z* /*res*/(q_dz/Reference_density)
//   )

  // Tx, Ty, Tz - non-dimensional for pressure terms
  // !!!!!!!!!!!!! pdd_ns_supg_new version: -1.0*Reference_density*(tau/Delta_t)
  //Tx[3+nreq*0] += -1.0 * Reference_density * (tau/Delta_t);
  //Ty[3+nreq*1] += -1.0 * Reference_density * (tau/Delta_t);
  //Tz[3+nreq*2] += -1.0 * Reference_density * (tau/Delta_t);
  // !!!!!!!!!!!!! Franca and Frey version: -1.0*(tau/Delta_t)
  //Tx[3+nreq*0] += -1.0 * (tau/Delta_t);
  //Ty[3+nreq*1] += -1.0 * (tau/Delta_t);
  //Tz[3+nreq*2] += -1.0 * (tau/Delta_t);
  //!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0*(tau_pres/Delta_t)
  //Tx[3+nreq*0] += (tau_pres/Delta_t); // u_x*q_dx
  //Ty[3+nreq*1] += (tau_pres/Delta_t); // u_y*q_dy
  //Tz[3+nreq*2] += (tau_pres/Delta_t); // u_z*q_dz

  // final choice - nondimensional for pressure terms
  //old//Tx[3+nreq*0] += -1.0 * (tau_pres/Delta_t);
  //old//Ty[3+nreq*1] += -1.0 * (tau_pres/Delta_t);
  //old//Tz[3+nreq*2] += -1.0 * (tau_pres/Delta_t);


//
//  Matrix (7): stabilization part: Reference_density * ((grad*u)u, res) - A.. matrices [kg/(m*s)]
//  [kg/(m*s)]*[1/s]*[1/s] = [kg/(m*s^3)] - OK
//      +
//   Reference_density * tau*(
//     (uk_x*u_xdx + uk_y*u_xdy + uk_z*u_xdz)* 
//        /*res*/(uk_x*v_xdx + uk_y*v_xdy + uk_z*v_xdz) + 
//     (uk_x*u_ydx + uk_y*u_ydy + uk_z*u_ydz)* 
//        /*res*/(uk_x*v_ydx + uk_y*v_ydy + uk_z*v_ydz) +
//     (uk_x*u_zdx + uk_y*u_zdy + uk_z*u_zdz)* 
//        /*res*/(uk_x*v_zdx + uk_y*v_zdy + uk_z*v_zdz)
//    )
//
//  [kg/m^3 * s * m/s * m/s] = [kg/(m*s)] - OK
//

  det1_2 = temp1_2;
  det1_2 = 1;
  /*
 old_GaussP[0] =old_GaussP1[0];
 old_GaussP[1] =old_GaussP1[1];
 old_GaussP[2] =old_GaussP1[2];
 */
 old_GaussP[0] =0;
 old_GaussP[1] =0;
 old_GaussP[2] =0;
 
  Axx[0+nreq*0] += Reference_density * tau * Uk_val[0] * Uk_val[0]*det1_2; // u_xdx*v_xdx
  Axx[1+nreq*1] += Reference_density * tau * Uk_val[0] * Uk_val[0]*det1_2; // u_ydx*v_ydx
  Axx[2+nreq*2] += Reference_density * tau * Uk_val[0] * Uk_val[0]*det1_2; // u_zdx*v_zdx

  Ayx[0+nreq*0] += Reference_density * tau * Uk_val[0] * Uk_val[1]*det1_2; // u_xdx*v_xdy
  Ayx[1+nreq*1] += Reference_density * tau * Uk_val[0] * Uk_val[1]*det1_2; // u_ydx*v_ydy
  Ayx[2+nreq*2] += Reference_density * tau * Uk_val[0] * Uk_val[1]*det1_2; // u_zdx*v_zdy

  Azx[0+nreq*0] += Reference_density * tau * Uk_val[0] * Uk_val[2]*det1_2; // u_xdx*v_xdz
  Azx[1+nreq*1] += Reference_density * tau * Uk_val[0] * Uk_val[2]*det1_2; // u_ydx*v_ydz
  Azx[2+nreq*2] += Reference_density * tau * Uk_val[0] * Uk_val[2]*det1_2; // u_zdx*v_zdz
  
  Axy[0+nreq*0] += Reference_density * tau * Uk_val[1] * Uk_val[0]*det1_2; // u_xdy*v_xdx
  Axy[1+nreq*1] += Reference_density * tau * Uk_val[1] * Uk_val[0]*det1_2; // u_ydy*v_ydx
  Axy[2+nreq*2] += Reference_density * tau * Uk_val[1] * Uk_val[0]*det1_2; // u_zdy*v_zdx

  Ayy[0+nreq*0] += Reference_density * tau * Uk_val[1] * Uk_val[1]*det1_2; // u_xdy*v_xdy
  Ayy[1+nreq*1] += Reference_density * tau * Uk_val[1] * Uk_val[1]*det1_2; // u_ydy*v_ydy
  Ayy[2+nreq*2] += Reference_density * tau * Uk_val[1] * Uk_val[1]*det1_2; // u_zdy*v_zdy

  Azy[0+nreq*0] += Reference_density * tau * Uk_val[1] * Uk_val[2]*det1_2; // u_xdy*v_xdz
  Azy[1+nreq*1] += Reference_density * tau * Uk_val[1] * Uk_val[2]*det1_2; // u_ydy*v_ydz
  Azy[2+nreq*2] += Reference_density * tau * Uk_val[1] * Uk_val[2]*det1_2; // u_zdy*v_zdz
  
  Axz[0+nreq*0] += Reference_density * tau * Uk_val[2] * Uk_val[0]*det1_2; // u_xdz*v_xdx
  Axz[1+nreq*1] += Reference_density * tau * Uk_val[2] * Uk_val[0]*det1_2; // u_ydz*v_ydx
  Axz[2+nreq*2] += Reference_density * tau * Uk_val[2] * Uk_val[0]*det1_2; // u_zdz*v_zdx

  Ayz[0+nreq*0] += Reference_density * tau * Uk_val[2] * Uk_val[1]*det1_2; // u_xdz*v_xdy
  Ayz[1+nreq*1] += Reference_density * tau * Uk_val[2] * Uk_val[1]*det1_2; // u_ydz*v_ydy
  Ayz[2+nreq*2] += Reference_density * tau * Uk_val[2] * Uk_val[1]*det1_2; // u_zdz*v_zdy

  Azz[0+nreq*0] += Reference_density * tau * Uk_val[2] * Uk_val[2]*det1_2; // u_xdz*v_xdz
  Azz[1+nreq*1] += Reference_density * tau * Uk_val[2] * Uk_val[2]*det1_2; // u_ydz*v_ydz
  Azz[2+nreq*2] += Reference_density * tau * Uk_val[2] * Uk_val[2]*det1_2; // u_zdz*v_zdz


//
//  Matrix (7): stabilization part: Reference_density * ((grad*u)u, res) - A.. matrices [kg/m^2]
//
//  [kg/m^2]*[1/s]*[m/s^2] =  [kg/(m*s^3)] - OK
//
// !!!!!!!!!!!!!! new version for q [m^2/s^2] (same as pressure/density) 
//      +
//   Reference_density * tau*(
//     (uk_x*u_xdx + uk_y*u_xdy + uk_z*u_xdz)* 
//        /*res*/(q_dx) + 
//     (uk_x*u_ydx + uk_y*u_ydy + uk_z*u_ydz)* 
//        /*res*/(q_dy) +
//     (uk_x*u_zdx + uk_y*u_zdy + uk_z*u_zdz)* 
//        /*res*/(q_dz)
//    )
//
//  [kg/m^3]*[s]*[m/s] = [kg/m^2]  - OK
//

  det1_2 = temp1_2;
  det1_2 = 1;
  
  Axx[3+nreq*0] += -Reference_density * tau_pres * Uk_val[0]*det1_2; // u_xdx*q_dx
  Ayx[3+nreq*1] += -Reference_density * tau_pres * Uk_val[0]*det1_2; // u_ydx*q_dy
  Azx[3+nreq*2] += -Reference_density * tau_pres * Uk_val[0]*det1_2; // u_zdx*q_dz

  Axy[3+nreq*0] += -Reference_density * tau_pres * Uk_val[1]*det1_2; // u_xdy*q_dx
  Ayy[3+nreq*1] += -Reference_density * tau_pres * Uk_val[1]*det1_2; // u_ydy*q_dy
  Azy[3+nreq*2] += -Reference_density * tau_pres * Uk_val[1]*det1_2; // u_zdy*q_dz

  Axz[3+nreq*0] += -Reference_density * tau_pres * Uk_val[2]*det1_2; // u_xdz*q_dx
  Ayz[3+nreq*1] += -Reference_density * tau_pres * Uk_val[2]*det1_2; // u_ydz*q_dy
  Azz[3+nreq*2] += -Reference_density * tau_pres * Uk_val[2]*det1_2; // u_zdz*q_dz

//
//  Matrix (7): stabilization part: Reference_density * ((grad*u)u, res) - A.. matrices [m]
//
// !!!!!!!!!!!!!! old version for q [kg/(m*s^2)] (same as pressure) 
//
//  [m]*[1/s]*[kg/(m^2*s^2)] =  [kg/(m*s^3)] - OK
//
//      +
//   Reference_density * tau*(
//     (uk_x*u_xdx + uk_y*u_xdy + uk_z*u_xdz)* 
//        /*res*/(q_dx/Reference_density) + 
//     (uk_x*u_ydx + uk_y*u_ydy + uk_z*u_ydz)* 
//        /*res*/(q_dy/Reference_density) +
//     (uk_x*u_zdx + uk_y*u_zdy + uk_z*u_zdz)* 
//        /*res*/(q_dz/Reference_density)
//    )
//
//  [s]*[m/s] = [m] - OK
//

  // !!!!!!!!!!!!! pdd_ns_supg_new version: -1.0*Reference_density * tau * Uk_val[0] 
  //Axx[3+nreq*0] += -1.0*Reference_density * tau * Uk_val[0]; // u_xdx*q_dx
  //Ayx[3+nreq*1] += -1.0*Reference_density * tau * Uk_val[0]; // u_ydx*q_dy
  //Azx[3+nreq*2] += -1.0*Reference_density * tau * Uk_val[0]; // u_zdx*q_dz
  // !!!!!!!!!!!!! Franca and Frey version: -1.0 * tau * Uk_val[0] 
  //Axx[3+nreq*0] += -1.0 * tau * Uk_val[0]; // u_xdx*q_dx
  //Ayx[3+nreq*1] += -1.0 * tau * Uk_val[0]; // u_ydx*q_dy
  //Azx[3+nreq*2] += -1.0 * tau * Uk_val[0]; // u_zdx*q_dz
  //!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0*tau_pres * Uk_val[0]
  //Axx[3+nreq*0] += tau_pres * Uk_val[0]; // u_xdx*q_dx
  //Ayx[3+nreq*1] += tau_pres * Uk_val[0]; // u_ydx*q_dy
  //Azx[3+nreq*2] += tau_pres * Uk_val[0]; // u_zdx*q_dz

  // final choice
  //old//Axx[3+nreq*0] += -1.0*tau_pres * Uk_val[0]; // u_xdx*q_dx
  //old//Ayx[3+nreq*1] += -1.0*tau_pres * Uk_val[0]; // u_ydx*q_dy
  //old//Azx[3+nreq*2] += -1.0*tau_pres * Uk_val[0]; // u_zdx*q_dz


  // !!!!!!!!!!!!! pdd_ns_supg_new version: -1.0*Reference_density * tau * Uk_val[1] 
  //Axy[3+nreq*0] += -1.0*Reference_density * tau * Uk_val[1]; // u_xdy*q_dx
  //Ayy[3+nreq*1] += -1.0*Reference_density * tau * Uk_val[1]; // u_ydy*q_dy
  //Azy[3+nreq*2] += -1.0*Reference_density * tau * Uk_val[1]; // u_zdy*q_dz
  // !!!!!!!!!!!!! Franca and Frey version: -1.0 * tau * Uk_val[1] 
  //Axy[3+nreq*0] += -1.0 * tau * Uk_val[1]; // u_xdy*q_dx
  //Ayy[3+nreq*1] += -1.0 * tau * Uk_val[1]; // u_ydy*q_dy
  //Azy[3+nreq*2] += -1.0 * tau * Uk_val[1]; // u_zdy*q_dz
  //!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0*tau_pres * Uk_val[1]
  //Axy[3+nreq*0] += tau_pres * Uk_val[1]; // u_xdy*q_dx
  //Ayy[3+nreq*1] += tau_pres * Uk_val[1]; // u_ydy*q_dy
  //Azy[3+nreq*2] += tau_pres * Uk_val[1]; // u_zdy*q_dz

  // final choice
  //old//Axy[3+nreq*0] += -1.0*tau_pres * Uk_val[1]; // u_xdy*q_dx
  //old//Ayy[3+nreq*1] += -1.0*tau_pres * Uk_val[1]; // u_ydy*q_dy
  //old//Azy[3+nreq*2] += -1.0*tau_pres * Uk_val[1]; // u_zdy*q_dz


  // !!!!!!!!!!!!! pdd_ns_supg_new version: -1.0*Reference_density * tau * Uk_val[2] 
  //Axz[3+nreq*0] += -1.0*Reference_density * tau * Uk_val[2]; // u_xdz*q_dx
  //Ayz[3+nreq*1] += -1.0*Reference_density * tau * Uk_val[2]; // u_ydz*q_dy
  //Azz[3+nreq*2] += -1.0*Reference_density * tau * Uk_val[2]; // u_zdz*q_dz
  // !!!!!!!!!!!!! Franca and Frey version: -1.0 * tau * Uk_val[2] 
  //Axz[3+nreq*0] += -1.0 * tau * Uk_val[2]; // u_xdz*q_dx
  //Ayz[3+nreq*1] += -1.0 * tau * Uk_val[2]; // u_ydz*q_dy
  //Azz[3+nreq*2] += -1.0 * tau * Uk_val[2]; // u_zdz*q_dz
  //!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0*tau_pres * Uk_val[2]
  //Axz[3+nreq*0] += tau_pres * Uk_val[2]; // u_xdz*q_dx
  //Ayz[3+nreq*1] += tau_pres * Uk_val[2]; // u_ydz*q_dy
  //Azz[3+nreq*2] += tau_pres * Uk_val[2]; // u_zdz*q_dz

  // final choice
  //old//Axz[3+nreq*0] += -1.0*tau_pres * Uk_val[2]; // u_xdz*q_dx
  //old//Ayz[3+nreq*1] += -1.0*tau_pres * Uk_val[2]; // u_ydz*q_dy
  //old//Azz[3+nreq*2] += -1.0*tau_pres * Uk_val[2]; // u_zdz*q_dz


//
// Matrix (9): stabilization part: (grad*p * res) - A.. matrices [m]
//
//  [m]*[kg/(m^2*s^2)]*[1/s] = [kg/(m*s^3)] - OK
//
//          +
//    tau*(
//      p_dx* /*res*/(uk_x*v_xdx + uk_y*v_xdy + uk_z*v_xdz) +
//      p_dy* /*res*/(uk_x*v_ydx + uk_y*v_ydy + uk_z*v_ydz) + 
//      p_dz* /*res*/(uk_x*v_zdx + uk_y*v_zdy + uk_z*v_zdz)
//    )
//
//  [s]*[m/s] = [m] - OK
//

  det1_2 = temp1_2;
  det1_2 = 1;
 /* 
 old_GaussP[0] =old_GaussP1[0];
 old_GaussP[1] =old_GaussP1[1];
 old_GaussP[2] =old_GaussP1[2];
*/
 old_GaussP[0] =0;
 old_GaussP[1] =0;
 old_GaussP[2] =0;
 
 
  Axx[0+nreq*3] += tau * (Uk_val[0]- old_GaussP[0])*det1_2; // p_dx*v_xdx
  Axy[1+nreq*3] += tau * (Uk_val[0]- old_GaussP[0])*det1_2; // p_dy*v_ydx
  Axz[2+nreq*3] += tau * (Uk_val[0]- old_GaussP[0])*det1_2; // p_dz*v_zdx
  
  Ayx[0+nreq*3] += tau * (Uk_val[1]- old_GaussP[1])*det1_2; // p_dx*v_xdy
  Ayy[1+nreq*3] += tau * (Uk_val[1]- old_GaussP[1])*det1_2; // p_dy*v_ydy
  Ayz[2+nreq*3] += tau * (Uk_val[1]- old_GaussP[1])*det1_2; // p_dz*v_zdy
  
  Azx[0+nreq*3] += tau * (Uk_val[2]- old_GaussP[2])*det1_2; // p_dx*v_xdz
  Azy[1+nreq*3] += tau * (Uk_val[2]- old_GaussP[2])*det1_2; // p_dy*v_ydz
  Azz[2+nreq*3] += tau * (Uk_val[2]- old_GaussP[2])*det1_2; // p_dz*v_zdz
  
//
// Matrix (9): stabilization part: (grad*p * res) - A.. matrices [s]
//
// !!!!!!!!!!!!!! new version for q [m^2/s^2] (same as pressure/density) 
//
//  [s]*[kg/(m^2*s^2)]*[m/s^2] = [kg/(m*s^3)] - OK
//
//          +
//    tau*(
//      p_dx* /*res*/(q_dx/Reference_density) +
//      p_dy* /*res*/(q_dy/Reference_density) + 
//      p_dz* /*res*/(q_dz/Reference_density)
//    )

  det1_2 = temp1_2;
  det1_2 = 1;
  Axx[3+nreq*3] += -1.0 * tau_pres *temp1_2; // p_dx*q_dx
  Ayy[3+nreq*3] += -1.0 * tau_pres *temp1_2; // p_dy*q_dy
  Azz[3+nreq*3] += -1.0 * tau_pres *temp1_2; // p_dz*q_dz

//
// Matrix (9): stabilization part: (grad*p * res) - A.. matrices [m^3*s/kg]
//
// !!!!!!!!!!!!!! old version for q [kg/(m*s^2)] (same as pressure) 
//
//  [m^3*s/kg]*[kg/(m^2*s^2)]*[kg/(m^2*s^2)] = [kg/(m*s^3)] - OK
//
//          +
//    tau*(
//      p_dx* /*res*/(q_dx/Reference_density) +
//      p_dy* /*res*/(q_dy/Reference_density) + 
//      p_dz* /*res*/(q_dz/Reference_density)
//    )
//
//  [s]*[m^3/kg] = [m^3*s/kg] - OK
//

  // !!!!!!!!!!!!! pdd_ns_supg_new version: -1.0 * tau 
  //Axx[3+nreq*3] += -1.0 * tau; // p_dx*q_dx
  //Ayy[3+nreq*3] += -1.0 * tau; // p_dy*q_dy
  //Azz[3+nreq*3] += -1.0 * tau; // p_dz*q_dz
  //!!!!!!!!!!!!! Franca and Frey version: -1.0 * tau / Reference_density
  //Axx[3+nreq*3] += -tau/Reference_density; // p_dx*q_dx
  //Ayy[3+nreq*3] += -tau/Reference_density; // p_dy*q_dy
  //Azz[3+nreq*3] += -tau/Reference_density; // p_dz*q_dz
  //!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0*tau_pres/Reference_density
  //Axx[3+nreq*3] += tau_pres/Reference_density; // p_dx*q_dx
  //Ayy[3+nreq*3] += tau_pres/Reference_density; // p_dy*q_dy
  //Azz[3+nreq*3] += tau_pres/Reference_density; // p_dz*q_dz
 
  // final choice
  //old//Axx[3+nreq*3] += -1.0 * tau_pres/Reference_density; // p_dx*q_dx
  //old//Ayy[3+nreq*3] += -1.0 * tau_pres/Reference_density; // p_dy*q_dy
  //old//Azz[3+nreq*3] += -1.0 * tau_pres/Reference_density; // p_dz*q_dz


//
// Matrix (8): Reference_density * (grad*u, sigma*grad*v) - A.. matrices
//
// [kg/(m*s)]*[1/s]*[1/s] =  [kg/(m*s^3)] - OK
//
//          + Reference_density * 
//          sigma*(u_xdx + u_ydy + u_zdz)*(v_xdx + v_ydy + v_zdz)
//
// [kg/m^3]*[m^2/s] = [kg/(m*s)] - OK
//
  // !!!!!!!!!!!!! pdd_ns_supg_new version: not multiplied by Reference_density

  /* !!!!!!!!!!!!!!! Franca and Frey -  Reference_density * sigma */
  /* !!!!!!!!!!!!!!! Hughes, Tezduyar, Mittal -  Reference_density * sigma */
det1_2 = temp1_2;
  det1_2 = 1;
  Axx[0+nreq*0] += Reference_density * sigma *det1_2; // u_xdx*v_xdx
  Axy[0+nreq*1] += Reference_density * sigma *det1_2; // u_ydy*v_xdx
  Axz[0+nreq*2] += Reference_density * sigma *det1_2; // u_zdz*v_xdx
  
  Ayx[1+nreq*0] += Reference_density * sigma *det1_2; // u_xdx*v_ydy
  Ayy[1+nreq*1] += Reference_density * sigma *det1_2; // u_ydy*v_ydy
  Ayz[1+nreq*2] += Reference_density * sigma *det1_2; // u_zdz*v_ydy
  
  Azx[2+nreq*0] += Reference_density * sigma *det1_2; // u_xdx*v_zdz
  Azy[2+nreq*1] += Reference_density * sigma *det1_2; // u_ydy*v_zdz
  Azz[2+nreq*2] += Reference_density * sigma *det1_2; // u_zdz*v_zdz


  /*! ----------------------------------------------------------------------! */
  /*! ------ CALCULATE TIME DERIVATIVE AND STABILIZATION TERMS FOR RHS -----! */
  /*! ----------------------------------------------------------------------! */

//       
//       Reference_density * (un_x * v_x + un_y * v_y + un_z * v_z) / Delta_t
//
// [kg/(m^2*s^2)] * [m/s] =  [kg/(m*s^3)] - OK
//
// [kg/m^3 * m/s * 1/s] = [kg/(m^2*s^2)] - OK
//

  det1_2 = temp1_2;
  det1_2 = 1;
  
  Lval[0] += (Reference_density * Un_val[0]  / Delta_t)*det1_2; // v_x
  Lval[1] += (Reference_density * Un_val[1]  / Delta_t)*det1_2; // v_y
  Lval[2] += (Reference_density * Un_val[2]  / Delta_t)*det1_2; // v_z
  
//        +
//  Reference_density * (tau / Delta_t) * (
//    un_x * 
//      /*res*/(uk_x * v_xdx + uk_y * v_xdy + uk_z * v_xdz) +
//    un_y * 
//      /*res*/(uk_x * v_ydx + uk_y * v_ydy + uk_z * v_ydz) +
//    un_z * 
//      /*res*/(uk_x * v_zdx + uk_y * v_zdy + uk_z * v_zdz)
//  )
//
// [kg/(m*s^2)] * [1/s] = [kg/(m*s^3)] - OK
//
// [kg/m^3 * s/s * m/s * m/s] = [kg/(m*s^2)] - OK
//

det1_2 = temp1_2;
  det1_2 = 1;

/*
 old_GaussP[0] =old_GaussP1[0];
 old_GaussP[1] =old_GaussP1[1];
 old_GaussP[2] =old_GaussP1[2];
 */
 old_GaussP[0] =0;
 old_GaussP[1] =0;
 old_GaussP[2] =0;
 
  Qx[0] += Reference_density * (tau / Delta_t) * Un_val[0] * Uk_val[0]; // v_xdx
  Qx[1] += Reference_density * (tau / Delta_t) * Un_val[1] * Uk_val[0]; // v_ydx
  Qx[2] += Reference_density * (tau / Delta_t) * Un_val[2] * Uk_val[0]; // v_zdx
  
  Qy[0] += Reference_density * (tau / Delta_t) * Un_val[0] * Uk_val[1]; // v_xdy
  Qy[1] += Reference_density * (tau / Delta_t) * Un_val[1] * Uk_val[1]; // v_ydy
  Qy[2] += Reference_density * (tau / Delta_t) * Un_val[2] * Uk_val[1]; // v_zdy
  
  Qz[0] += Reference_density * (tau / Delta_t) * Un_val[0] * Uk_val[2]; // v_xdz
  Qz[1] += Reference_density * (tau / Delta_t) * Un_val[1] * Uk_val[2]; // v_ydz
  Qz[2] += Reference_density * (tau / Delta_t) * Un_val[2] * Uk_val[2]; // v_zdz
  
//        +
//  Reference_density * (tau / Delta_t) * (
//    un_x * /*res*/ q_dx) +
//    un_y * /*res*/ q_dy) +
//    un_z * /*res*/ q_dz)
//  )
//
// !!!!!!!!!!!!!! new version for q [m^2/s^2] (same as pressure/density) 
//
// [kg/(m^2*s)] * [m/s^2] = [kg/(m*s^3)] - OK
//
// [kg/m^3 * s/s * m/s] = [kg/(m^2*s)] - OK
//

  det1_2 = temp1_2;
  det1_2 = 1;
 /*
 old_GaussP[0] =old_GaussP1[0];
 old_GaussP[1] =old_GaussP1[1];
 old_GaussP[2] =old_GaussP1[2];
 */
 old_GaussP[0] =0;
 old_GaussP[1] =0;
 old_GaussP[2] =0; 
  Qx[3] += -Reference_density * (tau_pres / Delta_t) * Un_val[0]*det1_2; // q_dx
  Qy[3] += -Reference_density * (tau_pres / Delta_t) * Un_val[1]*det1_2; // q_dy
  Qz[3] += -Reference_density * (tau_pres / Delta_t) * Un_val[2]*det1_2; // q_dz

//        +
//  Reference_density * (tau / Delta_t) * (
//    un_x * /*res*/ q_dx/Reference_density) +
//    un_y * /*res*/ q_dy/Reference_density) +
//    un_z * /*res*/ q_dz/Reference_density)
//  )
//
// !!!!!!!!!!!!!! old version for q [kg/(m*s^2)] (same as pressure) 
//
// [m/s] * [kg/(m^2*s^2)] = [kg/(m*s^3)] - OK
//
  // !!!!!!!!!!!!! pdd_ns_supg_new version: -1.0*Reference_density * tau/Delta_t
  //Qx[3] += -1.0*Reference_density * (tau / Delta_t) * Un_val[0]; // q_dx
  //Qy[3] += -1.0*Reference_density * (tau / Delta_t) * Un_val[1]; // q_dy
  //Qz[3] += -1.0*Reference_density * (tau / Delta_t) * Un_val[2]; // q_dz
  // !!!!!!!!!!!!! Franca and Frey version: -1.0 * tau/Delta_t
  //Qx[3] += -1.0 * (tau / Delta_t) * Un_val[0]; // q_dx
  //Qy[3] += -1.0 * (tau / Delta_t) * Un_val[1]; // q_dy
  //Qz[3] += -1.0 * (tau / Delta_t) * Un_val[2]; // q_dz
  //!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0*tau_pres/Delta_t
  //Qx[3] += (tau_pres / Delta_t) * Un_val[0]; // q_dx
  //Qy[3] += (tau_pres / Delta_t) * Un_val[1]; // q_dy
  //Qz[3] += (tau_pres / Delta_t) * Un_val[2]; // q_dz

  // final choice 
  //old//Qx[3] += -1.0*(tau_pres / Delta_t) * Un_val[0]; // q_dx
  //old//Qy[3] += -1.0*(tau_pres / Delta_t) * Un_val[1]; // q_dy
  //old//Qz[3] += -1.0*(tau_pres / Delta_t) * Un_val[2]; // q_dz


  /*! ----------------------------------------------------------------------! */
  /*! ---------------- CALCULATE BODY FORCES TERMS FOR RHS -----------------! */
  /*! ----------------------------------------------------------------------! */

  /* !!!!!!! body force should be in [kg/m^2/sec^2] or equivalent */

  // Currently only body force provided by ls_intf procedure

//        Body_force_x * v_x + Body_force_y * v_y + Body_force_z * v_z  - 
//
//  [kg/(m^2*s^2)] * [m/s] = [kg/(m*s^3)] - OK
//
  Sval[0] += Body_force_x; // v_x      
  Sval[1] += Body_force_y; // v_y      
  Sval[2] += Body_force_z; // v_z      


//     + 
//     tau * (
//       Body_force_x * (uk_x * v_xdx + uk_y * v_xdy + uk_z * v_xdz) +
//       Body_force_y * (uk_x * v_ydx + uk_y * v_ydy + uk_z * v_ydz) +
//       Body_force_z * (uk_x * v_zdx + uk_y * v_zdy + uk_z * v_zdz)
//     )
//
// [kg/(m*s^2)]*[1/s] = [kg/(m*s^3)] - OK
//
// [s] * [kg/(m^2*s^2)] * [m/s] = [kg/(m*s^2)] - OK
//
  Qx[0] += tau * Body_force_x * Uk_val[0]; // v_xdx
  Qx[1] += tau * Body_force_y * Uk_val[0]; // v_ydx
  Qx[2] += tau * Body_force_z * Uk_val[0]; // v_zdx
  
  Qy[0] += tau * Body_force_x * Uk_val[1]; // v_xdy
  Qy[1] += tau * Body_force_y * Uk_val[1]; // v_ydy
  Qy[2] += tau * Body_force_z * Uk_val[1]; // v_zdy
  
  Qz[0] += tau * Body_force_x * Uk_val[2]; // v_xdz
  Qz[1] += tau * Body_force_y * Uk_val[2]; // v_ydz
  Qz[2] += tau * Body_force_z * Uk_val[2]; // v_zdz

// !!!!!!!!!!!!!! new version for q [m^2/s^2] (same as pressure/density) 
//     + 
//     tau * (
//       Body_force_x * (q_dx) +
//       Body_force_y * (q_dy) +
//       Body_force_z * (q_dz)
//     )
//
// [kg/(m^2*s)]*[m/s^2] = [kg/(m*s^3)] - OK
//
// [s] * [kg/(m^2*s^2)] = [kg/(m^2*s)] - OK
//
  Qx[3] += -1.0*tau_pres * Body_force_x; // q_dx
  Qy[3] += -1.0*tau_pres * Body_force_y; // q_dy
  Qz[3] += -1.0*tau_pres * Body_force_z; // q_dz


// !!!!!!!!!!!!!! old version for q [kg/(m*s^2)] (same as pressure) 
//     + 
//     tau * (
//       Body_force_x * (uk_x * v_xdx + uk_y * v_xdy + uk_z * v_xdz + q_dx/Reference_density) +
//       Body_force_y * (uk_x * v_ydx + uk_y * v_ydy + uk_z * v_ydz + q_dy/Reference_density) +
//       Body_force_z * (uk_x * v_zdx + uk_y * v_zdy + uk_z * v_zdz + q_dz/Reference_density)
//     )
//
// [m/s] * [kg/(m^2*s^2)] = [kg/(m*s^3)] - OK
//
// [s] * [kg/(m^2*s^2)] * [m^3/kg] = [m/s] - OK
//
  // !!!!!!!!!!!!! pdd_ns_supg_new version: no q terms
  //
  //!!!!!!!!!!!!! Franca and Frey version: -1.0 * tau * Body_force_x / Reference_density
  //Qx[3] += -tau * Body_force_x / Reference_density; // q_dx
  //Qy[3] += -tau * Body_force_y / Reference_density; // q_dy
  //Qz[3] += -tau * Body_force_z / Reference_density; // q_dz
  //!!!!!!!!!!!!! Tezduyar, Hughes version: +1.0*tau_pres * Body_force_x/Reference_density
  //Qx[3] += tau_pres * Body_force_x / Reference_density; // q_dx
  //Qy[3] += tau_pres * Body_force_y / Reference_density; // q_dy
  //Qz[3] += tau_pres * Body_force_z / Reference_density; // q_dz

  // final choice
  //old//Qx[3] += -1.0*tau_pres * Body_force_x / Reference_density; // q_dx
  //old//Qy[3] += -1.0*tau_pres * Body_force_y / Reference_density; // q_dy
  //old//Qz[3] += -1.0*tau_pres * Body_force_z / Reference_density; // q_dz


  /*! ----------------------------------------------------------------------! */
  /*! --- SPLIT TERMS INTO LHS AND RHS FOR ALPHA TIME INTEGRATION SCHEME ---! */
  /*! ----------------------------------------------------------------------! */
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
  // FOR SIMPLE LINEAR PROBLEMS ONLY - DOES NOT WORK FOR NAVIER-STOKES
  //double Implicitness_coeff = pdr_time_d_params(Problem_id, 2);
  /* double Implicitness_coeff = problem->time.alpha; */
      
  /* for theta/alpha time integration method (theta=0.5 - Crank-Nicholson) */
  /* if(Implicitness_coeff<1.0){ */
    
  /*   int weq, ueq, i; */
    
  /*   for(weq=0;weq<nreq;weq++){ */
      
  /*     for(ueq=0;ueq<nreq;ueq++){ */
  /* 	Qx[weq] += (Implicitness_coeff-1.0) * ( */
  /* 				       Axx[weq+nreq*ueq]  * Un_x[ueq] */
  /* 				     + Axy[weq+nreq*ueq]  * Un_y[ueq] */
  /* 				     + Axz[weq+nreq*ueq]  * Un_z[ueq] */
  /* 				     + Tx[weq+nreq*ueq]   * Un_val[ueq] */
  /* 				     ); */
  /* 	Qy[weq] += (Implicitness_coeff-1.0) * ( */
  /* 				       Ayx[weq+nreq*ueq]  * Un_x[ueq] */
  /* 				     + Ayy[weq+nreq*ueq]  * Un_y[ueq] */
  /* 				     + Ayz[weq+nreq*ueq]  * Un_z[ueq] */
  /* 				     + Ty[weq+nreq*ueq]   * Un_val[ueq] */
  /* 				     ); */
  /* 	Qz[weq] += (Implicitness_coeff-1.0) * ( */
  /* 				       Azx[weq+nreq*ueq]  * Un_x[ueq] */
  /* 				     + Azy[weq+nreq*ueq]  * Un_y[ueq] */
  /* 				     + Azz[weq+nreq*ueq]  * Un_z[ueq] */
  /* 				     + Tz[weq+nreq*ueq]   * Un_val[ueq] */
  /* 				     ); */
  /* 	Sval[weq] += (Implicitness_coeff-1.0) * ( */
  /* 	                        	 Cval[weq+nreq*ueq] * Un_val[ueq] */
  /* 				       + Bx[weq+nreq*ueq] * Un_x[ueq] */
  /* 				       + By[weq+nreq*ueq] * Un_y[ueq] */
  /* 				       + Bz[weq+nreq*ueq] * Un_z[ueq] */
  /* 				       ); */
  /*     } */
	
  /*   } */

  /*   for(weq=0;weq<nreq;weq++){ */
      
  /*     for(ueq=0;ueq<nreq;ueq++){ */

  /* 	i = weq+nreq*ueq; */
    
  /* 	Axx[i] *= Implicitness_coeff; */
  /* 	Axy[i] *= Implicitness_coeff; */
  /* 	Axz[i] *= Implicitness_coeff; */
  /* 	Ayx[i] *= Implicitness_coeff; */
  /* 	Ayy[i] *= Implicitness_coeff; */
  /* 	Ayz[i] *= Implicitness_coeff; */
  /* 	Azx[i] *= Implicitness_coeff; */
  /* 	Azy[i] *= Implicitness_coeff; */
  /* 	Azz[i] *= Implicitness_coeff; */
  /* 	Bx[i] *= Implicitness_coeff; */
  /* 	By[i] *= Implicitness_coeff; */
  /* 	Bz[i] *= Implicitness_coeff; */
  /* 	Tx[i] *= Implicitness_coeff; */
  /* 	Ty[i] *= Implicitness_coeff; */
  /* 	Tz[i] *= Implicitness_coeff; */
  /* 	Cval[i] *= Implicitness_coeff;  */
  /*     } */
  /*   } */

  /* } */

  return (0);
}
  
/*------------------------------------------------------------
  pdr_ns_supg_comp_el_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for an element
  the procedure uses apr_num_int_el and provides it with necessary parameters
  apr_num_int_el gets coefficients at each integration point from 
  pdr_ns_supg_el_coeff procedure
------------------------------------------------------------*/
int pdr_ns_supg_comp_el_stiff_mat(/*returns: >=0 -success code, <0 -error code */
  int Problem_id,	/* in: approximation field ID  */
  int El_id,	/* in: unique identifier of the element */
  int Comp_sm,	/* in: indicator for the scope of computations: */
  /*   PDC_NO_COMP  - do not compute anything */
  /*   PDC_COMP_SM - compute entries to stiff matrix only */
  /*   PDC_COMP_RHS - compute entries to rhs vector only */
  /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int Pdeg_in,	/* in: enforced degree of polynomial (if > 0 ) */
  int *Nr_dof_ent,	/* in: size of arrays, */
  /* out: no of filled entries, i.e. number of mesh entities */
  /* with which dofs and stiffness matrix blocks are associated */
  int *List_dof_ent_type,	/* out: list of no of dofs for 'dof' entity */
  int *List_dof_ent_id,	/* out: list of no of dofs for 'dof' entity */
  int *List_dof_ent_nrdofs,	/* out: list of no of dofs for 'dof' entity */
  int *Nrdofs_loc,	/* in(optional): size of Stiff_mat and Rhs_vect */
  /* out(optional): actual number of dofs per integration entity */
  double *Stiff_mat,	/* out(optional): stiffness matrix stored columnwise */
  double *Rhs_vect,	/* out(optional): rhs vector */
  char *Rewr_dofs	/* out(optional): flag to rewrite or sum up entries */
  /*   'T' - true, rewrite entries when assembling */
  /*   'F' - false, sum up entries when assembling */
  )
{

  int pdeg;		/* degree of polynomial */
  int el_mate;		/* element material */
  int sol_vec_id;       /* indicator for the solution dofs */
  int num_shap;         /* number of element shape functions */
  int num_dofs;         /* number of element degrees of freedom */

/*   static int ndofs_max=0;            /\* local dimension of the problem *\/ */
/* #pragma omp threadprivate(ndofs_max) */

  // to make OpenMP working
  int ndofs_max=0;

  int field_id, mesh_id, nreq;  
  int kk, idofs, jdofs;
  int i,j,k;
  int max_nrdofs;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper field */
  field_id = pdr_ctrl_i_params(Problem_id, 3);
  mesh_id = apr_get_mesh_id(field_id);
  //nreq =apr_get_nreq(field_id);
  nreq = PDC_NS_SUPG_NREQ;

  if(Comp_sm!=PDC_NO_COMP){

    /* find degree of polynomial and number of element scalar dofs */
    /* Pdeg_in is now a number - we are in linear approximation!!! */
    if (Pdeg_in > 0){
      /* if Pdeg_in is specified as argument it takes precedence over */
      /* pdeg stored in data structures - for multigrid */
      pdeg = Pdeg_in;
    }
    else {
      apr_get_el_pdeg(field_id, El_id, &pdeg);
    }
    num_shap = apr_get_el_pdeg_numshap(field_id, El_id, &pdeg);
    num_dofs = num_shap*nreq;

    /* get material number */
    el_mate =  mmr_el_groupID(mesh_id, El_id);

#ifdef DEBUG
    if(*Nr_dof_ent < 27){
      printf("Too small arrays List_dof_... passed to comp_el_stiff_mat\n");
      printf("%d < 3. Exiting !!!", *Nr_dof_ent);
      exit(-1);
    }
#endif
    
    
#ifdef DEBUG
    if(Nrdofs_loc == NULL || Stiff_mat == NULL || Rhs_vect == NULL){
printf("NULL arrays Stiff_mat and Rhs_vect in pdr_comp_stiff_el_mat. Exiting!");
      exit(-1);
    }
    if(*Nrdofs_loc<num_dofs){
printf("Too small arrays Stiff_mat and Rhs_vect passed to comp_el_stiff_mat\n");
      printf("%d < %d. Exiting !!!", *Nrdofs_loc, num_dofs);
      exit(-1);
    }

#endif


    /* get the most recent solution degrees of freedom */
    double sol_dofs_n[APC_MAXELSD];	/* solution dofs */
    double sol_dofs_k[APC_MAXELSD];	/* solution dofs */

    if (mmr_el_status(mesh_id, El_id) == MMC_ACTIVE) {
      sol_vec_id = 1; // the most recent solution (at that moment - i.e. when
                      // forming linear system - equal to 
                      // sol_vec_id = 2 as well; after solving the system
                      // soldofs_1 are different than soldofs_2 and before 
                      // starting new solution they are rewritten to soldofs_2
      apr_get_el_dofs(field_id, El_id, sol_vec_id, sol_dofs_k);
      sol_vec_id = 3; // solution from the previous time step
      apr_get_el_dofs(field_id, El_id, sol_vec_id, sol_dofs_n);
    } else {
      /*!!! coarse element dofs should be supplied by calling routine !!! */

      printf("Inactive element in pdr_comp_el_stiff_mat! Exiting.\n");
      exit(-1);

      for (i = 0; i < num_dofs; i++) {
	sol_dofs_n[i] = 0;
	sol_dofs_k[i] = 0;
      }
    }

/*kbw
  if(El_id==5){
    printf("DOFS k:\n");
    for(i=0;i<num_dofs;i++) printf("%20.15lf",sol_dofs_k[i]);
    printf("\n");
    printf("DOFS n:\n");
    for(i=0;i<num_dofs;i++) printf("%20.15lf",sol_dofs_n[i]);
    printf("\n");
    //getchar();
  } 
/*kew*/


    if(Comp_sm==PDC_COMP_SM||Comp_sm==PDC_COMP_BOTH){

      /* initialize the matrices to zero */
      for(i=0;i<num_dofs*num_dofs;i++) Stiff_mat[i]=0.0;

    }

    if(Comp_sm==PDC_COMP_RHS||Comp_sm==PDC_COMP_BOTH){
    
      /* initialize the vector to zero */
      for(i=0;i<num_dofs;i++) Rhs_vect[i]=0.0; 

    }

    int diagonal[5]={1,0,0,0,0}; // diagonality of: M, A_ij, B_j, T_i and C  
    // coefficient matrices returned to apr_num_int_el by pdr_el_coeff
    /* perform numerical integration of terms from the weak formualation */
    apr_num_int_el(Problem_id,field_id,El_id,Comp_sm,&pdeg,sol_dofs_k,sol_dofs_n,
		   diagonal, Stiff_mat, Rhs_vect);


  
/*kbw
if(El_id>0){
  kk = 0;
  printf("Element %d: Stiffness matrix:\n",El_id);
    for (jdofs=0;jdofs<num_dofs;jdofs++) {
      for (idofs=0;idofs<num_dofs;idofs++) {
	printf("%10.6lf",Stiff_mat[kk+idofs]);
      }
      kk+=num_dofs;
      printf("\n");
    }
    
    printf("\nRhs_vect for element:\n");
    for (idofs=0;idofs<num_dofs;idofs++) {
      printf("%10.6lf",Rhs_vect[idofs]);
    }
    printf("\n\n");
    getchar(); 
    } 
/*kew*/

    if(Rewr_dofs != NULL) *Rewr_dofs = 'F';

  } /* end if computing SM and/or RHSV */


  return(1);
}

/*------------------------------------------------------------
  pdr_ns_supg_comp_fa_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for a face
------------------------------------------------------------*/
int pdr_ns_supg_comp_fa_stiff_mat(/*returns: >=0 -success code, <0 -error code */
  int Problem_id,	/* in: approximation field ID  */
  int Fa_id,	/* in: unique identifier of the face */
  int Comp_sm,	/* in: indicator for the scope of computations: */
  /*   PDC_NO_COMP  - do not compute anything */
  /*   PDC_COMP_SM - compute entries to stiff matrix only */
  /*   PDC_COMP_RHS - compute entries to rhs vector only */
  /*   PDC_COMP_BOTH - compute entries for sm and rhsv */
  int Pdeg_in,	/* in: enforced degree of polynomial (if > 0 ) */
  int *Nr_dof_ent,	/* in: size of arrays List_dof_ent_... */
  /* out: number of mesh entities with which dofs and */
  /*      stiffness matrix blocks are associated */
  int *List_dof_ent_type,	/* out: list of no of dofs for 'dof' entity */
  int *List_dof_ent_id,	/* out: list of no of dofs for 'dof' entity */
  int *List_dof_ent_nrdofs,	/* out: list of no of dofs for 'dof' entity */
  int *Nrdofs_loc,	/* in(optional): size of Stiff_mat and Rhs_vect */
  /* out(optional): actual number of dofs per integration entity */
  double *Stiff_mat,	/* out(optional): stiffness matrix stored columnwise */
  double *Rhs_vect,	/* out(optional): rhs vector */
  char *Rewr_dofs	/* out(optional): flag to rewrite or sum up entries */
  /*   'T' - true, rewrite entries when assembling */
  /*   'F' - false, sum up entries when assembling */
  )
{
  /* local variables */
  int fa_type;			/* type of face: quad or triangle */
  int fa_bnum;			/* boundary number for a face */
  int base;			/* type of basis functions */
  int nreq;			/* number of equations */
  int pdeg = 0, num_shap = 0;	/* local dimension for neigbhor 1 */
  int face_neig[2];		/* list of neighbors */
  int num_fa_nodes;
  int fa_nodes[4];
  int neig_sides[2] = { 0, 0 };	/* sides of face wrt neigs */
  double loc_xg[6];		/* local coord of gauss points for neighbors */
  int node_shift;
  double acoeff[4], bcoeff[2];	/* to transform coordinates between faces */
  double hf_size;		/* size of face */
  int i, ieq, kk, ki, k, idofs, jdofs, neig_id, num_dofs, j, iaux, naj, nai;
  double daux, vec_norm[3], penalty;
  double area;
  int field_id, mesh_id;
  int el_nodes[MMC_MAXELVNO + 1];	// list of nodes of El
  double node_coor[3 * MMC_MAXELVNO];	// coord of nodes of El
  int ngauss;			/* number of gauss points */
  double xg[3000];		/* coordinates of gauss points in 3D */
  double wg[1000];		/* gauss weights */
  double determ;		/* determinant of jacobi matrix */
  double xcoor[3];		/* global coord of gauss point */
  double u_val_hat[PDC_MAXEQ - 1];	/* specified solution for Dirichlet BC */
  double u_val[PDC_MAXEQ];	/* computed solution */
  double u_x[PDC_MAXEQ];		/* gradient of computed solution */
  double u_y[PDC_MAXEQ];		/* gradient of computed solution */
  double u_z[PDC_MAXEQ];		/* gradient of computed solution */
  double base_phi[APC_MAXELVD];	/* basis functions */
  double base_dphix[APC_MAXELVD];	/* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];	/* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];	/* y-derivatives of basis function */
  double sol_dofs_n[APC_MAXELSD];	/* solution dofs */
  double sol_dofs_k[APC_MAXELSD];	/* solution dofs */
  double tk, t_grad[3]; // value and gradient of temperature at point
  int pp_node, vp_node;
  double x, y, z;
  int presspinsfound, velopinsfound;
  int ignore_bc_vel_nodes_idx[4] = { 0, 0, 0, 0 };  //4 - max no. of face nodes
  double inflow_d, inflow_diameter, inflow_d_scaled, inflow_vel;
  double x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
  pdt_ns_supg_bctype bc_type = BC_NS_SUPG_NONE;
  double rad;
  double t_x, t_y, t_z;	// Marangoni shear stress components
  double density, specific_heat;
  int udofs, wdofs;

  /* some pointers to type less */
  pdt_ns_supg_problem *problem =
    (pdt_ns_supg_problem *) pdr_get_problem_structure(Problem_id);
  pdt_ns_supg_ctrls *ctrls = &problem->ctrl;
  pdt_ns_supg_times *time = &problem->time;
  pdt_ns_supg_nonls *nonl = &problem->nonl;

  /* select the proper mesh */
  field_id = ctrls->field_id;
  mesh_id = apr_get_mesh_id(field_id);

  /* get type of face and bc */
  fa_type = mmr_fa_type(mesh_id, Fa_id);
  mmr_fa_area(mesh_id, Fa_id, &daux, vec_norm);
  hf_size = sqrt(daux);
  /* get boundary number */
  fa_bnum = mmr_fa_bc(mesh_id, Fa_id);
  /* get b.condition types for boundary(face) */
  bc_type = pdr_ns_supg_get_bc_type(&problem->bc, fa_bnum);

  /* get approximation parameters */
  //nreq = ctrls->nreq;
  nreq = PDC_NS_SUPG_NREQ;

  /* get neighbors list with corresponding neighbors' sides numbers */
  mmr_fa_neig(mesh_id, Fa_id, face_neig, neig_sides, &node_shift, 
	      NULL, acoeff, bcoeff);

  neig_id = abs(face_neig[0]);	//boundary face's element id

  if(Pdeg_in>0) pdeg = Pdeg_in;
  else{
    /* find degree of polynomial and number of element scalar dofs */
    /*!!! need some trick for coarse solve with different Pdeg !!!*/
    apr_get_el_pdeg(field_id, neig_id, &pdeg);
  }

  num_shap = apr_get_el_pdeg_numshap(field_id, neig_id, &pdeg);
  num_dofs = num_shap * nreq;
  base = apr_get_base_type(field_id, neig_id);

  mmr_el_node_coor(mesh_id, neig_id, el_nodes, node_coor);

  /*kbw
     printf("In pdr_comp_fa_stiff_mat: field_id %d, mesh_id %d, Comp_sm %d\n",
     field_id, mesh_id, Comp_sm);
     printf("Fa_id %d, Fa_type %d, size %lf, Fa_bc %d, bc_type %d\n",
     Fa_id, fa_type, hf_size, fa_bc, bc_type);
     printf("elem %d, el_side %d, el_mate %d, num_shap %d, num_dofs %d\n",
     neig_id, neig_sides[0], el_mate, num_shap, num_dofs);
     printf("For each block: \ttype, \tid, \tnrdof\n");
     for(i=0;i<*Nr_dof_ent;i++){
     printf("\t\t\t%d\t%d\t%d\n",
     List_dof_ent_type[i],List_dof_ent_id[i],List_dof_ent_nrdofs[i]);
     }
     /*kew */


  if (Comp_sm != PDC_NO_COMP) {
    if (Comp_sm == PDC_COMP_SM || Comp_sm == PDC_COMP_BOTH) {

      penalty = ctrls->penalty;

      if (mmr_el_status(mesh_id, neig_id) == MMC_ACTIVE) {	
	/* get the most recent solution degrees of freedom */
	apr_get_el_dofs(field_id, neig_id, 2, sol_dofs_k);	//u_k, tk
	apr_get_el_dofs(field_id, neig_id, 3, sol_dofs_n);	//u_n, tn
      } else {
	for (i = 0; i < num_dofs; i++) {	
	  /* coarse element dofs should be supplied by calling routine */
	  sol_dofs_n[i] = 0;
	  sol_dofs_k[i] = 0;
	}
      }

      for (i = 0; i < num_dofs * num_dofs; i++)
	Stiff_mat[i] = 0.0;
      for (i = 0; i < num_dofs; i++)
	Rhs_vect[i] = 0.0;

      if (fa_type == MMC_TRIA) {
	num_fa_nodes = 3;
      } else if (fa_type == MMC_QUAD) {
	num_fa_nodes = 4;
      }
      mmr_el_fa_nodes(mesh_id, neig_id, neig_sides[0], fa_nodes);

      /*! DIAGNOSTICS - uncomment ! */
      /*
         printf("ELEMENT ID: %d\tFACE ID: %d\n", neig_id, Fa_id);
         printf("FACE NODE COOR:\n");
         printf("%lf\t%lf\t%lf\t\n", node_coor[3*fa_nodes[0]], node_coor[3*fa_nodes[0]+1], node_coor[3*fa_nodes[0]+2]);
         printf("%lf\t%lf\t%lf\t\n", node_coor[3*fa_nodes[1]], node_coor[3*fa_nodes[1]+1], node_coor[3*fa_nodes[1]+2]);
         printf("%lf\t%lf\t%lf\t\n", node_coor[3*fa_nodes[2]], node_coor[3*fa_nodes[2]+1], node_coor[3*fa_nodes[2]+2]);
         if(num_fa_nodes == 4)
         printf("%lf\t%lf\t%lf\t\n", node_coor[3*fa_nodes[3]], node_coor[3*fa_nodes[3]+1], node_coor[3*fa_nodes[3]+2]);
       */


      /*! -------------------------------------------------------------------! */
      /*! ------------------ FLOW BOUNDARY CONDITIONS -----------------------! */
      /*! -------------------------------------------------------------------! */



      /*! -------------------------------------------------------------------! */
      /*! -------------------------- PRESSURE PINS --------------------------! */
      presspinsfound = 0;
      if (pdr_ns_supg_get_pressure_pins_count(&problem->bc) > 0) {
	for (i = 0; i < pdr_ns_supg_get_pressure_pins_count(&problem->bc); ++i) {
	  pdt_ns_supg_pin_pressure *pin_pressure = 
	    pdr_ns_supg_get_pressure_pin(&problem->bc, i);
	  for (pp_node = 0; pp_node < num_fa_nodes; ++pp_node) {
	    x = node_coor[3 * fa_nodes[pp_node]];
	    y = node_coor[3 * fa_nodes[pp_node] + 1];
	    z = node_coor[3 * fa_nodes[pp_node] + 2];

	    if ((fabs(x - pin_pressure->pin_node_coor[0]) < 1.e-6) && 
		(fabs(y - pin_pressure->pin_node_coor[1]) < 1.e-6) && 
		(fabs(z - pin_pressure->pin_node_coor[2]) < 1.e-6)) {
	      kk = (num_dofs * fa_nodes[pp_node] + fa_nodes[pp_node]) * nreq;
	      j = 3 * num_dofs + 3;	//pressure is 4th, thats why...
	      Stiff_mat[kk + j] += penalty;
	      Rhs_vect[fa_nodes[pp_node] * nreq + 3] += pin_pressure->p*penalty;

	      /*! DIAGNOSTICS - uncomment ! */
	      //printf("------PRESSPNODE FOUND-----------------\n");
	      //printf("Fa_id: %d\tpp_node: %d\tfa_nodes[pp_node]: %d\n",Fa_id, pp_node, fa_nodes[pp_node]);
	      //printf("x: %lf\ty: %lf\tz: %lf\n",x,y,z);
	      //printf("Writing Stiff:\n");
	      //printf("\tStiff_mat[%d]\n",kk+j);
	      //printf("******PRESSPNODE END*******************\n");

	      break;
	    }
	  }
	  if (++presspinsfound == pdr_ns_supg_get_pressure_pins_count(&problem->bc))
	    break;
	}
      }

      /*! -------------------------------------------------------------------! */
      /*! -------------------------- VELOCITY PINS --------------------------! */
      velopinsfound = 0;
      if (pdr_ns_supg_get_velocity_pins_count(&problem->bc) > 0) {
	for (i = 0; i < pdr_ns_supg_get_velocity_pins_count(&problem->bc); ++i) {
	  pdt_ns_supg_pin_velocity *pin_velocity = 
	    pdr_ns_supg_get_velocity_pin(&problem->bc, i);
	  for (vp_node = 0; vp_node < num_fa_nodes; ++vp_node) {
	    x = node_coor[3 * fa_nodes[vp_node]];
	    y = node_coor[3 * fa_nodes[vp_node] + 1];
	    z = node_coor[3 * fa_nodes[vp_node] + 2];

	    if ((fabs(x - pin_velocity->pin_node_coor[0]) < 1.e-6) && 
		(fabs(y - pin_velocity->pin_node_coor[1]) < 1.e-6) && 
		(fabs(z - pin_velocity->pin_node_coor[2]) < 1.e-6)) {
	      kk = (num_dofs * fa_nodes[vp_node] + fa_nodes[vp_node]) * nreq;

	      j = 0 * num_dofs + 0;	//vel x
	      Stiff_mat[kk + j] += penalty;
	      Rhs_vect[fa_nodes[vp_node]*nreq + 0] += pin_velocity->v[0]*penalty;

	      /*! DIAGNOSTICS - uncomment ! */
	      //printf("------VPNODE FOUND--------------------\n");
	      //printf("Fa_id: %d\tvp_node: %d\tfa_nodes[vp_node]: %d\n",Fa_id, vp_node, fa_nodes[vp_node]);
	      //printf("x: %lf\ty: %lf\tz: %lf\n",x,y,z);
	      //printf("Writing Stiff & Rhs:\n");
	      //printf("\tStiff_mat[%d]\tRhs[%d]\t\tvel[0]=%lf\n",kk+j,fa_nodes[vp_node]*nreq + 0,g_pdv_bc[i].vel[0]);

	      j = 1 * num_dofs + 1;	//vel y
	      Stiff_mat[kk + j] += penalty;
	      Rhs_vect[fa_nodes[vp_node]*nreq + 1] += pin_velocity->v[1]*penalty;

	      /*! DIAGNOSTICS - uncomment ! */
	      //printf("\tStiff_mat[%d]\tRhs[%d]\t\tvel[1]=%lf\n",kk+j,fa_nodes[vp_node]*nreq + 1,g_pdv_bc[i].vel[1]);

	      j = 2 * num_dofs + 2;	//vel z
	      Stiff_mat[kk + j] += penalty;
	      Rhs_vect[fa_nodes[vp_node]*nreq + 2] += pin_velocity->v[2]*penalty;

	      /*! DIAGNOSTICS - uncomment ! */
	      //printf("\tStiff_mat[%d]\tRhs[%d]\t\tvel[2]=%lf\n",kk+j,fa_nodes[vp_node]*nreq + 2,g_pdv_bc[i].vel[2]);
	      //printf("******VPNODE END-*********************\n");

	      ignore_bc_vel_nodes_idx[vp_node] = 1;	//to not set vel on this node in bc_vel
	      break;
	    }
	  }
	  if (++velopinsfound == pdr_ns_supg_get_velocity_pins_count(&problem->bc))
	    break;
	}
      }
    
      // terms from the weak statement that have to be computed on the boundary
      // 
      // 1. normal derivative of velocity components - part of traction
      //
      // Stiff_mat(ux,wx) = - miu * (ux_dx*nx + ux_dy*ny + ux_dz*nz)
      // Stiff_mat(uy,wy) = - miu * (uy_dx*nx + uy_dy*ny + uy_dz*nz)
      // Stiff_mat(uz,wz) = - miu * (uz_dx*nx + uz_dy*ny + uz_dz*nz)
      // 
      // 2. derivatives of normal velocity - part of traction
      //
      // Stiff_mat(ux,wx) = - miu * (ux_dx*nx)
      // Stiff_mat(uy,wx) = - miu * (uy_dx*ny)
      // Stiff_mat(uz,wx) = - miu * (uz_dx*nz)
      //
      // Stiff_mat(ux,wy) = - miu * (ux_dy*nx)
      // Stiff_mat(uy,wy) = - miu * (uy_dy*ny)
      // Stiff_mat(uz,wy) = - miu * (uz_dy*nz)
      //
      // Stiff_mat(ux,wz) = - miu * (ux_dz*nx)
      // Stiff_mat(uy,wz) = - miu * (uy_dz*ny)
      // Stiff_mat(uz,wz) = - miu * (uz_dz*nz)
      //
      // 3. pressure - part of traction
      //
      // Stiff_mat(p,wx) = nx
      // Stiff_mat(p,wy) = ny
      // Stiff_mat(p,wz) = nz
      //
      // 4. integrated by parts term (grad*u, q)
      //
      // Stiff_mat(ux,q) = nx
      // Stiff_mat(uy,q) = ny
      // Stiff_mat(uz,q) = nz
      //
      // ALE:
      // Rhs_vect(q) = U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz 
    
      /*! -------------------------------------------------------------------! */
      /*! --------------------------- BC_NS_SUPG_SYMM  ----------------------! */
      if (bc_type == BC_NS_SUPG_SYMMETRY) {	//TODO: change to switch
	/*! DIAGNOSTICS - uncomment ! */
	//printf("------BC_NS_SUPG_SYMM BEGIN-----------------\n");
	//printf("Fa_id: %d, vec_norm %lf, %lf, %lf\n",
	//      Fa_id, vec_norm[0], vec_norm[1], vec_norm[2]);
	//printf("Writing Stiff:\n");


	// NORMAL VELOCITY = 0
	for (idofs = 0; idofs < num_fa_nodes; ++idofs) {
	  kk = (num_dofs * fa_nodes[idofs] + fa_nodes[idofs]) * nreq;
	  for(ieq=0;ieq<3;ieq++){
	    j = ieq * num_dofs;
	    Stiff_mat[kk + j ] += vec_norm[ieq]*vec_norm[0]*penalty;
	    Stiff_mat[kk + j + 1 ] += vec_norm[ieq]*vec_norm[1]*penalty;
	    Stiff_mat[kk + j + 2 ] += vec_norm[ieq]*vec_norm[2]*penalty;
	    //printf("ieq = %d, pos %d, %lf, %lf, %lf\n",
	    //	   ieq, kk + j, Stiff_mat[kk + j ], Stiff_mat[kk + j + 1 ], 
	    //	   Stiff_mat[kk + j + 2 ]);
	  }
	}

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("\tStiff_mat[%d]\t\t",kk+j);
	  //printf("x: %lf\ty: %lf\tz: %lf\n",node_coor[3*fa_nodes[idofs]],node_coor[3*fa_nodes[idofs]+1],node_coor[3*fa_nodes[idofs]+2]);
      
	// NORMAL DERIVATIVES OF TANGENT VELOCITIES = 0 - not implemented


      // TERMS FROM THE WEAK STATEMENT THAT HAVE TO BE COMPUTED ON THE BOUNDARY
	  // BY NUMERICAL INTEGRATION !!!

	double vec_norm2[3]={vec_norm[0],vec_norm[1],vec_norm[2]};

	apr_set_quadr_2D(fa_type,base,&pdeg,&ngauss,xg,wg);
   
	// loop over integration points 
	for (ki=0;ki<ngauss;ki++) {

// find coordinates within neighboring elements for a point on face 
	  // !!! for triangular faces coordinates are standard [0,1][0,1] 
	  // !!! for quadrilateral faces coordinates are [-1,1][-1,1] - 
	  // !!! which means that they do not conform to element coordinates 
	  // !!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!! 
	  mmr_fa_elem_coor(mesh_id,&xg[2*ki],face_neig,neig_sides,node_shift,
			     acoeff,bcoeff,loc_xg);

// at the gauss point for neig , compute basis functions 
	  iaux = 3+neig_sides[0]; // boundary data for face neig_sides[0] 

	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base,
				      loc_xg,node_coor,sol_dofs_k,
				      base_phi,base_dphix,base_dphiy,base_dphiz,
				      xcoor,u_val,u_x,u_y,u_z,vec_norm2);

	  // temporary for DEBUG
	  assert(vec_norm[0] == vec_norm2[0]);
	  assert(vec_norm[1] == vec_norm2[1]);
	  assert(vec_norm[2] == vec_norm2[2]);

	  // coefficient for 2D numerical integration 
	  area = determ*wg[ki];
	  
	  double ref_temperature = ctrls->ref_temperature;
	  double dynamic_viscosity;

	  if(ref_temperature<=0.0){

	    // viscosity and density constant and stored in ctrl structure
            dynamic_viscosity = ctrls->dynamic_viscosity;

	  }
	  else{

	    // viscosity specified as material parameter (possibly temperature dependent)
	    
	    // every problem dependent module that uses ns_supg must provide 
	    // implementation of this procedure in files from ls_intf directory
	    pdr_ns_supg_give_me_temperature_at_point(
                                       Problem_id, neig_id, loc_xg, 
				       base_phi,base_dphix,base_dphiy,base_dphiz,
				       &tk, NULL, NULL, NULL);

	    //printf("in fa_stiff: Tk = %lf\n", tk);
	    
	    //1.set query parameters (which material and temperature)
        utt_material_query_params query_params;
        utt_material_query_result query_result;

        query_params.group_idx = mmr_el_groupID(mesh_id, neig_id);
	    //query_params.material_idx = 0;	// query by material index ...
	    query_params.name = "";	// ... not by material name 
	    query_params.temperature = tk;	// current temperature
	    query_params.cell_id = neig_id;
	    for( i=0; i<3; i++ ){
	      query_params.xg[i] = xcoor[i];
	    }
        query_params.query_type = QUERY_POINT;
	    //2.get query results
        pdr_ns_supg_material_query(&query_params,&query_result);
	    //3.set values to those obtained with query
	    dynamic_viscosity = query_result.dynamic_viscosity;
	    
	  }

	  if(dynamic_viscosity<1e-12){
	    printf("dynamic viscosity %lf < 1.e-12 in comp_fa_stiff!\n",
		   dynamic_viscosity);
	    printf("Exiting!\n"); exit(-1);
	  }

	
      // 
      // 1. normal derivative of velocity components - usually assumed 0
      //
      // Stiff_mat(ux,wx) = - miu * (ux_dx*nx + ux_dy*ny + ux_dz*nz) * wx
      // Stiff_mat(uy,wy) = - miu * (uy_dx*nx + uy_dy*ny + uy_dz*nz) * wy
      // Stiff_mat(uz,wz) = - miu * (uz_dx*nx + uz_dy*ny + uz_dz*nz) * wz
      // 
      /* kk=0;  */
      /* for (udofs=0;udofs<num_shap;udofs++) { */
      /*   for (wdofs=0;wdofs<num_shap;wdofs++) { */
      
      /* 	Stiff_mat[kk+wdofs*nreq+0*num_dofs+0] += (  */
      /* 		      // - miu * wx * dux/dx * nx */
      /*     - Dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs] */
      /* 		      // - miu * wx * dux/dy * ny */
      /*     - Dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs] */
      /* 		      // - miu * wx * dux/dz * nz */
      /*     - Dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs] */
      /* 						   ) * area; */
      
      /* 	Stiff_mat[kk+wdofs*nreq+1*num_dofs+1] += (  */
      /* 		      // - miu * wy * duy/dx * nx */
      /*     - Dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs] */
      /* 		      // - miu * wy * duy/dy * ny */
      /*     - Dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs] */
      /* 		      // - miu * wy * duy/dz * nz */
      /*     - Dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs] */
      /* 						   ) * area; */
      
      /* 	Stiff_mat[kk+wdofs*nreq+2*num_dofs+2] += (  */
      /* 		      // - miu * wz * duz/dx * nx */
      /*     - Dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs] */
      /* 		      // - miu * wz * duz/dy * ny */
      /*     - Dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs] */
      /* 		      // - miu * wz * duz/dz * nz */
      /*     - Dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs] */
      /* 						   ) * area; */
      
      /*   } // wdofs  */
      /*   kk += nreq*num_dofs; */
      
      /* } // udofs  */
      
      // 2. derivatives of normal velocity - since we keep terms miu*u_j,i*w_i,j
      //                                     in weak formulation
      //
      // Stiff_mat(ux,wx) = - miu * (ux_dx*nx) * wx
      // Stiff_mat(uy,wx) = - miu * (uy_dx*ny) * wx
      // Stiff_mat(uz,wx) = - miu * (uz_dx*nz) * wx

	  kk=0;
	  for (udofs=0;udofs<num_shap;udofs++) {
	    for (wdofs=0;wdofs<num_shap;wdofs++) {
	      
      		Stiff_mat[kk+wdofs*nreq+0*num_dofs+0] += (
      			      // - miu * wx * dux/dx * nx
      	        - dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs]
      							   ) * area;
      		Stiff_mat[kk+wdofs*nreq+1*num_dofs+0] += (
      			      // - miu * wx * duy/dx * ny
      	        - dynamic_viscosity * vec_norm[1] * base_dphix[udofs] * base_phi[wdofs]
      							   ) * area;
      		Stiff_mat[kk+wdofs*nreq+2*num_dofs+0] += (
      			      // - miu * wx * duz/dx * nz
      	        - dynamic_viscosity * vec_norm[2] * base_dphix[udofs] * base_phi[wdofs]
      							   ) * area;
      //
      // Stiff_mat(ux,wy) = - miu * (ux_dy*nx) * wy
      // Stiff_mat(uy,wy) = - miu * (uy_dy*ny) * wy
      // Stiff_mat(uz,wy) = - miu * (uz_dy*nz) * wy

      		Stiff_mat[kk+wdofs*nreq+0*num_dofs+1] += (
      			      // - miu * wy * dux/dy * nx
      	        - dynamic_viscosity * vec_norm[0] * base_dphiy[udofs] * base_phi[wdofs]
      							   ) * area;
      		Stiff_mat[kk+wdofs*nreq+1*num_dofs+1] += (
      			      // - miu * wy * duy/dy * ny
      	        - dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs]
      							   ) * area;
      		Stiff_mat[kk+wdofs*nreq+2*num_dofs+1] += (
      			      // - miu * wy * duz/dy * nz
      	        - dynamic_viscosity * vec_norm[2] * base_dphiy[udofs] * base_phi[wdofs]
      							   ) * area;
      //
      // Stiff_mat(ux,wz) = - miu * (ux_dz*nx) * wz
      // Stiff_mat(uy,wz) = - miu * (uy_dz*ny) * wz
      // Stiff_mat(uz,wz) = - miu * (uz_dz*nz) * wz

      		Stiff_mat[kk+wdofs*nreq+0*num_dofs+2] += (
      			      // - miu * wz * dux/dz * nx
      	        - dynamic_viscosity * vec_norm[0] * base_dphiz[udofs] * base_phi[wdofs]
      							   ) * area;
      		Stiff_mat[kk+wdofs*nreq+1*num_dofs+2] += (
      			      // - miu * wz * duy/dz * ny
      	        - dynamic_viscosity * vec_norm[1] * base_dphiz[udofs] * base_phi[wdofs]
      							   ) * area;
      		Stiff_mat[kk+wdofs*nreq+2*num_dofs+2] += (
      			      // - miu * wz * duz/dz * nz
      	        - dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs]
      							   ) * area;


	    } // wdofs
	    kk += nreq*num_dofs;
	    
	  } // udofs
	    
      //
      // 3. pressure - specified (and enforced by penalty) or assumed to be zero
      //
      // Stiff_mat(p,wx) = p * nx * wx
      // Stiff_mat(p,wy) = p * ny * wy
      // Stiff_mat(p,wz) = p * nz * wz

      /* kk=0; */
      /* for (udofs=0;udofs<num_shap;udofs++) { */
      /*   for (wdofs=0;wdofs<num_shap;wdofs++) { */
      
      /* 	Stiff_mat[kk+wdofs*nreq+3*num_dofs+0] += (  */
      /* 		      // p * nx * wx */
      /*     vec_norm[0] * base_phi[udofs] * base_phi[wdofs] */
      /* 						   ) * area; */
      /* 	Stiff_mat[kk+wdofs*nreq+3*num_dofs+1] += (  */
      /* 		      // p * ny * wy */
      /*     vec_norm[1] * base_phi[udofs] * base_phi[wdofs] */
      /* 						   ) * area; */
      /* 	Stiff_mat[kk+wdofs*nreq+3*num_dofs+2] += (  */
      /* 		      // p * nz * wz */
      /*     vec_norm[2] * base_phi[udofs] * base_phi[wdofs] */
      /* 						   ) * area; */
      /*   } // wdofs  */
      /*   kk += nreq*num_dofs; */
      
      /* } // udofs  */
       
      //
      // 4. integrated by parts term (grad*u, q)
      //
      // Stiff_mat(ux,q) = ux * nx * q
      // Stiff_mat(uy,q) = uy * ny * q
      // Stiff_mat(uz,q) = uz * nz * q
      //
      //
      // ALE:
      // Rhs_vect(q) = U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz 

	   // kk=0;
	   //for (udofs=0;udofs<num_shap;udofs++) {
	   //  for (wdofs=0;wdofs<num_shap;wdofs++) {
	   
	   //Stiff_mat[kk+wdofs*nreq+0*num_dofs+3] += ( 
	   // q * nx * ux
	   //vec_norm[0] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+1*num_dofs+3] += ( 
	   //		      // q * ny * uy
	   //vec_norm[1] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+2*num_dofs+3] += ( 
	   // q * nz * uz
	   //vec_norm[2] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //  } // wdofs 
	   //  kk += nreq*num_dofs;
	   
	   //} // udofs 

	  // for (wdofs=0;wdofs<num_shap;wdofs++) {
	  //   Rhs_vect[wdofs*nreq + 3] += (
	  //  q * (U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz)
	  //     vec_norm[0]*U_GaussP_x + vec_norm[1]*U_GaussP_y + vec_norm[2]*U_GaussP_z
	  //                      ) * base_phi[wdofs] * area
	  // } // wdofs


	} // end loop over integration points
      

	/*! DIAGNOSTICS - uncomment ! */
	//printf("******BC_NS_SUPG_SYMM END*********************\n");
      }
      

      /*! ------------------------------------------------------------------! */
      /*! -------------------------- BC_NS_SUPG_VEL  -----------------------! */
      /*! ------------------------ BC_NS_SUPG_NOSLIP  ----------------------! */
      else if (bc_type == BC_NS_SUPG_VELOCITY || bc_type == BC_NS_SUPG_NOSLIP) {	//TODO: change to switch
	/*! DIAGNOSTICS - uncomment ! */
	//printf("------BC_NS_SUPG_VEL BEGIN-----------------\n");
	//printf("Fa_id: %d\n",Fa_id);

	if (bc_type == BC_NS_SUPG_VELOCITY) {
	  //get bc data
	  pdt_ns_supg_bc_velocity *bc_velocity_data = (pdt_ns_supg_bc_velocity *) pdr_ns_supg_get_bc_data(&problem->bc, fa_bnum);

	  u_val_hat[0] = bc_velocity_data->v[0];
	  u_val_hat[1] = bc_velocity_data->v[1];
	  u_val_hat[2] = bc_velocity_data->v[2];
	} else {
	  u_val_hat[0] = 0.0;
	  u_val_hat[1] = 0.0;
	  u_val_hat[2] = 0.0;
	}

	for (idofs = 0; idofs < num_fa_nodes; ++idofs) {
	  kk = (num_dofs * fa_nodes[idofs] + fa_nodes[idofs]) * nreq;
	  if (ignore_bc_vel_nodes_idx[idofs] != 1)	
	    //ignore nodes on which velocity pin is set
	  {
	    /*! DIAGNOSTICS - uncomment ! */
	    //printf("Idofs: %d\t\t",idofs);
	    //printf("x: %lf\ty: %lf\tz: %lf\n",node_coor[3*fa_nodes[idofs]],node_coor[3*fa_nodes[idofs]+1],node_coor[3*fa_nodes[idofs]+2]);
	    //printf("\tStiff & Rhs at .. will be written:\n");

	    for (ieq = 0; ieq < 3; ieq++)	
	      // 3 : because we're not touching pressure & temperature here
	    {
	      j = ieq * num_dofs + ieq;
	      Stiff_mat[kk + j] += penalty;
	      Rhs_vect[fa_nodes[idofs] * nreq + ieq] += u_val_hat[ieq] * penalty;
	      //== given velocity on DIRICHLET boundary

	      /*! DIAGNOSTICS - uncomment ! */
	      //printf("\t\t%d\t%d\n", kk+j, fa_nodes[idofs]*nreq + ieq);
	    }
	  } else		//nothing there - just for diagnostics
	  {
	    /*! DIAGNOSTICS - uncomment ! */
	    //printf("Idofs: %d IGNORED!\t\t",idofs);
	    //printf("x: %lf\ty: %lf\tz: %lf\n",node_coor[3*fa_nodes[idofs]],node_coor[3*fa_nodes[idofs]+1],node_coor[3*fa_nodes[idofs]+2]);
	    //printf("\tStiff & Rhs at .. will NOT be written:\n");
	    //printf("\t\t%d\t%d\n",kk+(0*num_dofs+0),fa_nodes[idofs]*nreq + 0);
	    //printf("\t\t%d\t%d\n",kk+(1*num_dofs+1),fa_nodes[idofs]*nreq+ 1);
	    //printf("\t\t%d\t%d\n",kk+(2*num_dofs+2),fa_nodes[idofs]*nreq+ 2);
	  }
	}
          
      //
      // 4. integrated by parts term (grad*u, q)
      //
      // Stiff_mat(ux,q) = ux * nx * q
      // Stiff_mat(uy,q) = uy * ny * q
      // Stiff_mat(uz,q) = uz * nz * q
      //
      //
      // ALE:
      // Rhs_vect(q) = U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz 

	   // kk=0;
	   //for (udofs=0;udofs<num_shap;udofs++) {
	   //  for (wdofs=0;wdofs<num_shap;wdofs++) {
	   
	   //Stiff_mat[kk+wdofs*nreq+0*num_dofs+3] += ( 
	   // q * nx * ux
	   //vec_norm[0] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+1*num_dofs+3] += ( 
	   //		      // q * ny * uy
	   //vec_norm[1] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+2*num_dofs+3] += ( 
	   // q * nz * uz
	   //vec_norm[2] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //  } // wdofs 
	   //  kk += nreq*num_dofs;
	   
	   //} // udofs 

	  // for (wdofs=0;wdofs<num_shap;wdofs++) {
	  //   Rhs_vect[wdofs*nreq + 3] += (
	  //  q * (U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz)
	  //     vec_norm[0]*U_GaussP_x + vec_norm[1]*U_GaussP_y + vec_norm[2]*U_GaussP_z
	  //                      ) * base_phi[wdofs] * area
	  // } // wdofs

	/*! DIAGNOSTICS - uncomment ! */
	//printf("******BC_NS_SUPG_VEL END**************************\n");
      }


      /*! ------------------------------------------------------------------! */
      /*! -------------------- BC_NS_SUPG_INFLOW_CIRCLE3D  -----------------! */
      else if (bc_type == BC_NS_SUPG_INFLOW_CIRCLE3D) {	//TODO: change to switch

	/*! DIAGNOSTICS - uncomment ! */
	//printf("------BC_NS_SUPG_INFLOW_CIRCLE3D BEGIN-----------------\n");

	//get bc data
	pdt_ns_supg_bc_inflow_circle_3d *bc_inflow_data = (pdt_ns_supg_bc_inflow_circle_3d *) pdr_ns_supg_get_bc_data(&problem->bc, fa_bnum);

	x1 = bc_inflow_data->center[0];
	y1 = bc_inflow_data->center[1];
	z1 = bc_inflow_data->center[2];
	inflow_diameter = bc_inflow_data->radius;

	for (idofs = 0; idofs < num_fa_nodes; idofs++) {
	  x0 = node_coor[3 * fa_nodes[idofs]];
	  y0 = node_coor[3 * fa_nodes[idofs] + 1];
	  z0 = node_coor[3 * fa_nodes[idofs] + 2];

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("NODE COOR: x0: %lf\ty0: %lf\tz0: %lf ---------\n",x0,y0,z0);

	  inflow_d = sqrt((x0-x1)*(x0-x1)+(y0-y1)*(y0-y1)+(z0-z1)*(z0-z1));

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("inflow_d: %lf\n",inflow_d);
    inflow_d_scaled = inflow_d / inflow_diameter;
	if(inflow_d_scaled<1.0){
	  inflow_vel = (-1.0 * inflow_d_scaled * inflow_d_scaled + 1) 
	                                * bc_inflow_data->v;
		}
		else{inflow_vel = 0.0;}
	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("inflow_d_scaled: %lf\n",inflow_d_scaled);
	  //printf("inflow_vel: %lf\n",inflow_vel);

	  kk = (num_dofs * fa_nodes[idofs] + fa_nodes[idofs]) * nreq;

	  if (ignore_bc_vel_nodes_idx[idofs] != 1)	
	    //ignore nodes on which velocity pin is set
	  {
	    for (ieq = 0; ieq < 3; ieq++)	
	      //we're not touching pressure & temperature here..
	    {
	      j = ieq * num_dofs + ieq;
	      Stiff_mat[kk + j] += penalty;
	      Rhs_vect[fa_nodes[idofs] * nreq + ieq] += -1.0 * vec_norm[ieq] 
		                                          * inflow_vel * penalty;

	      /*! DIAGNOSTICS - uncomment ! */
	      //printf("RHS = : %lf\n",-1.0*vec_norm[ieq]*inflow_vel);
	    }
	  } else		//nothing there - just for diagnostics
	  {
	    /*! DIAGNOSTICS - uncomment ! */
	    //printf("IGNORED!!!!!\n");
	  }
	}

      //
      // 4. integrated by parts term (grad*u, q)
      //
      // Stiff_mat(ux,q) = ux * nx * q
      // Stiff_mat(uy,q) = uy * ny * q
      // Stiff_mat(uz,q) = uz * nz * q
      //
      //
      // ALE:
      // Rhs_vect(q) = U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz 

	   // kk=0;
	   //for (udofs=0;udofs<num_shap;udofs++) {
	   //  for (wdofs=0;wdofs<num_shap;wdofs++) {
	   
	   //Stiff_mat[kk+wdofs*nreq+0*num_dofs+3] += ( 
	   // q * nx * ux
	   //vec_norm[0] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+1*num_dofs+3] += ( 
	   //		      // q * ny * uy
	   //vec_norm[1] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+2*num_dofs+3] += ( 
	   // q * nz * uz
	   //vec_norm[2] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //  } // wdofs 
	   //  kk += nreq*num_dofs;
	   
	   //} // udofs 

	  // for (wdofs=0;wdofs<num_shap;wdofs++) {
	  //   Rhs_vect[wdofs*nreq + 3] += (
	  //  q * (U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz)
	  //     vec_norm[0]*U_GaussP_x + vec_norm[1]*U_GaussP_y + vec_norm[2]*U_GaussP_z
	  //                      ) * base_phi[wdofs] * area
	  // } // wdofs

	//! DIAGNOSTICS - uncomment ! */
	//printf("------BC_NS_SUPG_INFLOW_CIRCLE3D END-----------------\n");
      }


      /*! ------------------------------------------------------------------! */
      /*! -------------------- BC_NS_SUPG_INFLOW_RECT2D  -------------------! */
      else if (bc_type == BC_NS_SUPG_INFLOW_RECT2D) {	//TODO: change to switch

	/*! DIAGNOSTICS - uncomment ! */
	//printf("------BC_NS_SUPG_INFLOW_RECT2D BEGIN-----------------\n");

	//get bc data
	pdt_ns_supg_bc_inflow_rect_2d *bc_inflow_data = (pdt_ns_supg_bc_inflow_rect_2d *) pdr_ns_supg_get_bc_data(&problem->bc, fa_bnum);

	x1 = bc_inflow_data->n1[0];
	y1 = bc_inflow_data->n1[1];
	z1 = bc_inflow_data->n1[2];

	x2 = bc_inflow_data->n2[0];
	y2 = bc_inflow_data->n2[1];
	z2 = bc_inflow_data->n2[2];

	x3 = bc_inflow_data->n3[0];
	y3 = bc_inflow_data->n3[1];
	z3 = bc_inflow_data->n3[2];

	/*kbw
	printf("fa_bnum %d, vec_nor: %lf, %lf, %lf\n",
	       fa_bnum, vec_norm[0],vec_norm[1],vec_norm[2]);

	printf("n1: %lf, %lf, %lf\n",
	       bc_inflow_data->n1[0],bc_inflow_data->n1[1],bc_inflow_data->n1[2]);

	printf("n2: %lf, %lf, %lf\n",
	       bc_inflow_data->n2[0],bc_inflow_data->n2[1],bc_inflow_data->n2[2]);

	printf("n3: %lf, %lf, %lf\n",
	       bc_inflow_data->n3[0],bc_inflow_data->n3[1],bc_inflow_data->n3[2]);
	/*kew*/

	inflow_diameter = sqrt(((((x1 - x3) * (x1 - x3) + (y1 - y3) * (y1 - y3) + (z1 - z3) * (z1 - z3)) * ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1)))
				- (((x2 - x1) * (x1 - x3) + (y2 - y1) * (y1 - y3) + (z2 - z1) * (z1 - z3)) * ((x2 - x1) * (x1 - x3) + (y2 - y1) * (y1 - y3) + (z2 - z1) * (z1 - z3))))
			       / ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1)));

	/*! DIAGNOSTICS - uncomment ! */
	//printf("inflow_diameter: %lf\n",inflow_diameter);

	for (idofs = 0; idofs < num_fa_nodes; idofs++) {
	  x0 = node_coor[3 * fa_nodes[idofs]];
	  y0 = node_coor[3 * fa_nodes[idofs] + 1];
	  z0 = node_coor[3 * fa_nodes[idofs] + 2];

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("NODE COOR: x0: %lf\ty0: %lf\tz0: %lf --------\n",x0,y0,z0);

	  inflow_d = sqrt(((((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0) + (z1 - z0) * (z1 - z0)) * ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1)))
			   - (((x2 - x1) * (x1 - x0) + (y2 - y1) * (y1 - y0) + (z2 - z1) * (z1 - z0)) * ((x2 - x1) * (x1 - x0) + (y2 - y1) * (y1 - y0) + (z2 - z1) * (z1 - z0))))
			  / ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1)));

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("inflow_d: %lf\n",inflow_d);
	  //printf(" bc_inflow_velocity: %lf\n", bc_inflow_data->v);

	  inflow_d_scaled = inflow_d / inflow_diameter;
	  inflow_vel = ((-1.0 * ((inflow_d_scaled - 0.5) * (inflow_d_scaled - 0.5)) + 0.25) * 4.0) * bc_inflow_data->v;

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("inflow_d_scaled: %lf\n",inflow_d_scaled);
	  //printf("inflow_vel: %lf\n",inflow_vel);

	  kk = (num_dofs * fa_nodes[idofs] + fa_nodes[idofs]) * nreq;

	  if (ignore_bc_vel_nodes_idx[idofs] != 1)	
	    //ignore nodes on which velocity pin is set
	  {
	    for (ieq = 0; ieq < 3; ieq++)	
	      //we're not touching pressure & temperaure here..
	    {
	      j = ieq * num_dofs + ieq;
	      Stiff_mat[kk + j] += penalty;
	      Rhs_vect[fa_nodes[idofs] * nreq + ieq] += -1.0 * vec_norm[ieq] 
		                                         * inflow_vel * penalty;

	      /*! DIAGNOSTICS - uncomment ! */
	      //printf("RHS = : %lf\n",-1.0*vec_norm[ieq]*inflow_vel);
	    }
	  } else		//nothing there - just for diagnostics
	  {
	    /*! DIAGNOSTICS - uncomment ! */
	    //printf("IGNORED!!!!!\n");
	  }
	}
      
      //
      // 4. integrated by parts term (grad*u, q)
      //
      // Stiff_mat(ux,q) = ux * nx * q
      // Stiff_mat(uy,q) = uy * ny * q
      // Stiff_mat(uz,q) = uz * nz * q
      //
      //
      // ALE:
      // Rhs_vect(q) = U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz 

	   // kk=0;
	   //for (udofs=0;udofs<num_shap;udofs++) {
	   //  for (wdofs=0;wdofs<num_shap;wdofs++) {
	   
	   //Stiff_mat[kk+wdofs*nreq+0*num_dofs+3] += ( 
	   // q * nx * ux
	   //vec_norm[0] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+1*num_dofs+3] += ( 
	   //		      // q * ny * uy
	   //vec_norm[1] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+2*num_dofs+3] += ( 
	   // q * nz * uz
	   //vec_norm[2] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //  } // wdofs 
	   //  kk += nreq*num_dofs;
	   
	   //} // udofs 

	  // for (wdofs=0;wdofs<num_shap;wdofs++) {
	  //   Rhs_vect[wdofs*nreq + 3] += (
	  //  q * (U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz)
	  //     vec_norm[0]*U_GaussP_x + vec_norm[1]*U_GaussP_y + vec_norm[2]*U_GaussP_z
	  //                      ) * base_phi[wdofs] * area
	  // } // wdofs

	/*! DIAGNOSTICS - uncomment ! */
	//printf("------BC_NS_SUPG_INFLOW_RECT2D END-----------------\n");
      }


      /*! -------------------------------------------------------------------! */
      /*! ----------------------- BC_NS_SUPG_OUTFLOW  -----------------------! */
      else if (bc_type == BC_NS_SUPG_OUTFLOW) {	
	//TODO: set ignoring press pins here too
	//TODO: change to switch
	// !!! PRESSURE IS ASSUMED 0 AT OUTFLOW !!!
	  for (idofs = 0; idofs < num_fa_nodes; idofs++) {
	    kk = (num_dofs * fa_nodes[idofs] + fa_nodes[idofs]) * nreq;
	    ieq = 3;		//pressure is 4th, thats why...
	    j = ieq * num_dofs + ieq;
	    Stiff_mat[kk + j] += penalty;
	  }
      // TERMS FROM THE WEAK STATEMENT THAT HAVE TO BE COMPUTED ON THE BOUNDARY
	  // BY NUMERICAL INTEGRATION !!!

	  apr_set_quadr_2D(fa_type,base,&pdeg,&ngauss,xg,wg);
   
	  // loop over integration points 
	  for (ki=0;ki<ngauss;ki++) {

// find coordinates within neighboring elements for a point on face 
	    // !!! for triangular faces coordinates are standard [0,1][0,1] 
	    // !!! for quadrilateral faces coordinates are [-1,1][-1,1] - 
	    // !!! which means that they do not conform to element coordinates 
	    // !!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!! 
	    mmr_fa_elem_coor(mesh_id,&xg[2*ki],face_neig,neig_sides,node_shift,
			     acoeff,bcoeff,loc_xg);

// at the gauss point for neig , compute basis functions 
	    iaux = 3+neig_sides[0]; // boundary data for face neig_sides[0] 

	    determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base,
				      loc_xg,node_coor,sol_dofs_k,
				      base_phi,base_dphix,base_dphiy,base_dphiz,
				      xcoor,u_val,u_x,u_y,u_z,vec_norm);
// coefficient for 2D numerical integration 
	    area = determ*wg[ki];

	    double ref_temperature = ctrls->ref_temperature;
	    double dynamic_viscosity;

	    if(ref_temperature<=0.0){
	      
	      // viscosity and density constant and stored in ctrl structure
	      dynamic_viscosity = ctrls->dynamic_viscosity;
	      
	    }
	    else{

	      // every problem dependent module that uses ns_supg must provide 
	      // implementation of this procedure in files from ls_intf directory
	      pdr_ns_supg_give_me_temperature_at_point(
                                       Problem_id, neig_id, loc_xg, 
				       base_phi,base_dphix,base_dphiy,base_dphiz,
				       &tk, NULL, NULL, NULL);

	      //printf("in fa_stiff: Tk = %lf\n", tk);
	      
	      //1.set query parameters (which material and temperature)
          utt_material_query_params query_params;
          utt_material_query_result query_result;

          query_params.group_idx = mmr_el_groupID(mesh_id, neig_id);
	      //query_params.material_idx = 0;	// query by material index ...
	      query_params.name = "";	// ... not by material name 
	      query_params.temperature = tk;	// current temperature
	      query_params.cell_id = neig_id;
	      for( i=0; i<3; i++ ){
	        query_params.xg[i] = xcoor[i];
	      }
          query_params.query_type = QUERY_POINT;
	      //2.get query results
          pdr_ns_supg_material_query(&query_params,&query_result);
	      //3.set values to those obtained with query
	      dynamic_viscosity = query_result.dynamic_viscosity;

	    }
	      
	    if(dynamic_viscosity<1e-12){
	      printf("dynamic viscosity %lf < 1.e-12 in comp_fa_stiff!\n",
		     dynamic_viscosity);
	      printf("Exiting!\n"); exit(-1);
	    }
	  
      // 
      // 1. normal derivative of velocity components - 0 - traction free
      //
      // Stiff_mat(ux,wx) = - miu * (ux_dx*nx + ux_dy*ny + ux_dz*nz) * wx
      // Stiff_mat(uy,wy) = - miu * (uy_dx*nx + uy_dy*ny + uy_dz*nz) * wy
      // Stiff_mat(uz,wz) = - miu * (uz_dx*nx + uz_dy*ny + uz_dz*nz) * wz
      // 
      /* kk=0; */
      /* for (udofs=0;udofs<num_shap;udofs++) { */
      /*   for (wdofs=0;wdofs<num_shap;wdofs++) { */
      
      /*     Stiff_mat[kk+wdofs*nreq+0*num_dofs+0] += (  */
      /*       - Dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs] */
      /*       - Dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs] */
      /*       - Dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs] */
      /* 						 ) * area; */
      
      /*     Stiff_mat[kk+wdofs*nreq+1*num_dofs+1] += (  */
      /*       - Dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs] */
      /*       - Dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs] */
      /*       - Dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs] */
      /* 						 ) * area; */
      
      /*     Stiff_mat[kk+wdofs*nreq+2*num_dofs+2] += (  */
      /*       - Dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs] */
      /*       - Dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs] */
      /*       - Dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs] */
      /* 						 ) * area; */
      
      /*   } // wdofs  */
      /*   kk += nreq*num_dofs; */
      
      /* } // udofs  */

      // 2. derivatives of normal velocity 
      //
      // Stiff_mat(ux,wx) = - miu * (ux_dx*nx) * wx
      // Stiff_mat(uy,wx) = - miu * (uy_dx*ny) * wx
      // Stiff_mat(uz,wx) = - miu * (uz_dx*nz) * wx

	  kk=0;
	  for (udofs=0;udofs<num_shap;udofs++) {
	    for (wdofs=0;wdofs<num_shap;wdofs++) {
	      
	      Stiff_mat[kk+wdofs*nreq+0*num_dofs+0] += ( 
	        - dynamic_viscosity * vec_norm[0] * base_dphix[udofs] * base_phi[wdofs]
							 ) * area;
	      Stiff_mat[kk+wdofs*nreq+1*num_dofs+0] += ( 
	        - dynamic_viscosity * vec_norm[1] * base_dphix[udofs] * base_phi[wdofs]
							 ) * area;
	      Stiff_mat[kk+wdofs*nreq+2*num_dofs+0] += ( 
	        - dynamic_viscosity * vec_norm[2] * base_dphix[udofs] * base_phi[wdofs]
							 ) * area;
      //
      // Stiff_mat(ux,wy) = - miu * (ux_dy*nx) * wy
      // Stiff_mat(uy,wy) = - miu * (uy_dy*ny) * wy
      // Stiff_mat(uz,wy) = - miu * (uz_dy*nz) * wy

	      Stiff_mat[kk+wdofs*nreq+0*num_dofs+1] += ( 
	        - dynamic_viscosity * vec_norm[0] * base_dphiy[udofs] * base_phi[wdofs]
							 ) * area;
	      Stiff_mat[kk+wdofs*nreq+1*num_dofs+1] += ( 
	        - dynamic_viscosity * vec_norm[1] * base_dphiy[udofs] * base_phi[wdofs]
							 ) * area;
	      Stiff_mat[kk+wdofs*nreq+2*num_dofs+1] += ( 
	        - dynamic_viscosity * vec_norm[2] * base_dphiy[udofs] * base_phi[wdofs]
							 ) * area;
      //
      // Stiff_mat(ux,wz) = - miu * (ux_dz*nx) * wz
      // Stiff_mat(uy,wz) = - miu * (uy_dz*ny) * wz
      // Stiff_mat(uz,wz) = - miu * (uz_dz*nz) * wz

	      Stiff_mat[kk+wdofs*nreq+0*num_dofs+2] += ( 
	        - dynamic_viscosity * vec_norm[0] * base_dphiz[udofs] * base_phi[wdofs]
							 ) * area;
	      Stiff_mat[kk+wdofs*nreq+1*num_dofs+2] += ( 
	        - dynamic_viscosity * vec_norm[1] * base_dphiz[udofs] * base_phi[wdofs]
							 ) * area;
	      Stiff_mat[kk+wdofs*nreq+2*num_dofs+2] += ( 
	        - dynamic_viscosity * vec_norm[2] * base_dphiz[udofs] * base_phi[wdofs]
							 ) * area;


	    } // wdofs 
	    kk += nreq*num_dofs;
	    
	  } // udofs 
	  
      //
      // 3. pressure - 0 - traction free
	 
      //
      // Stiff_mat(p,wx) = p * nx * wx
      // Stiff_mat(p,wy) = p * ny * wy
      // Stiff_mat(p,wz) = p * nz * wz

      //
      // 4. integrated by parts term (grad*u, q)
      //
      // Stiff_mat(ux,q) = ux * nx * q
      // Stiff_mat(uy,q) = uy * ny * q
      // Stiff_mat(uz,q) = uz * nz * q
      //
      //
      // ALE:
      // Rhs_vect(q) = U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz 

	   // kk=0;
	   //for (udofs=0;udofs<num_shap;udofs++) {
	   //  for (wdofs=0;wdofs<num_shap;wdofs++) {
	   
	   //Stiff_mat[kk+wdofs*nreq+0*num_dofs+3] += ( 
	   // q * nx * ux
	   //vec_norm[0] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+1*num_dofs+3] += ( 
	   //		      // q * ny * uy
	   //vec_norm[1] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //Stiff_mat[kk+wdofs*nreq+2*num_dofs+3] += ( 
	   // q * nz * uz
	   //vec_norm[2] * base_phi[udofs] * base_phi[wdofs]
	   //						   ) * area;
	   //  } // wdofs 
	   //  kk += nreq*num_dofs;
	   
	   //} // udofs 

	  // for (wdofs=0;wdofs<num_shap;wdofs++) {
	  //   Rhs_vect[wdofs*nreq + 3] += (
	  //  q * (U_GaussP_x * nx + U_GaussP_y * ny + U_GaussP_z * nz)
	  //     vec_norm[0]*U_GaussP_x + vec_norm[1]*U_GaussP_y + vec_norm[2]*U_GaussP_z
	  //                      ) * base_phi[wdofs] * area
	  // } // wdofs

	  
	  } // end loop over integration points
	
      } // end outflow BC
    
      /*! -------------------------------------------------------------------! */
      /*! -------------------------- MARANGONI ------------------------------! */
/*       if (bc_type == BC_NS_SUPG_MARANGONI) { */
/*         //get bc data */
/*         pdt_ns_supg_bc_marangoni *bc_marangoni_data =  */
/* 	  (pdt_ns_supg_bc_marangoni*) pdr_ns_supg_get_bc_data(&problem->bc,  */
/* 							      fa_bnum); */
/* 	// NORMAL VELOCITY = 0 */
/* 	for (idofs = 0; idofs < num_fa_nodes; ++idofs) { */
/* 	  kk = (num_dofs * fa_nodes[idofs] + fa_nodes[idofs]) * nreq; */
/* 	  for(ieq=0;ieq<3;ieq++){ */
/* 	    j = ieq * num_dofs; */
/* 	    Stiff_mat[kk + j ] += vec_norm[ieq]*vec_norm[0]*penalty; */
/* 	    Stiff_mat[kk + j + 1 ] += vec_norm[ieq]*vec_norm[1]*penalty; */
/* 	    Stiff_mat[kk + j + 2 ] += vec_norm[ieq]*vec_norm[2]*penalty; */
/* 	    //printf("ieq = %d, pos %d, %lf, %lf, %lf\n", */
/* 	    //	   ieq, kk + j, Stiff_mat[kk + j ], Stiff_mat[kk + j + 1 ],  */
/* 	    //	   Stiff_mat[kk + j + 2 ]); */
/* 	  } */
/* 	} */

/* 	// NORMAL DERIVATIVES OF TANGENT VELOCITIES = 0 - not implemented */

/* 	  // forces */
/*    /\* prepare data for gaussian integration *\/ */
/*    /\* !!!!! for triangular faces coordinates are standard [0,1][0,1] *\/ */
/*    /\* !!!!! for quadrilateral faces coordinates are [-1,1][-1,1] - *\/ */
/*    /\* !!!!! which means that they do not conform to element coordinates *\/ */
/*    /\* !!!!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!!!!! *\/ */
/*         apr_set_quadr_2D(fa_type,base,&pdeg,&ngauss,xg,wg); */

/* 	for (ki=0; ki<ngauss; ++ki) { */

/* /\* find coordinates within neighboring elements for a point on face *\/ */
/*    /\* !!!!! for triangular faces coordinates are standard [0,1][0,1] *\/ */
/*    /\* !!!!! for quadrilateral faces coordinates are [-1,1][-1,1] - *\/ */
/*    /\* !!!!! which means that they do not conform to element coordinates *\/ */
/*    /\* !!!!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!!!!! *\/ */
/*           mmr_fa_elem_coor(mesh_id,&xg[2*ki],face_neig,neig_sides,node_shift, */
/* 			   acoeff,bcoeff,loc_xg); */
/*           iaux = 3+neig_sides[0]; // boundary data for face neig_sides[0] */
/*           determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base,loc_xg, node_coor,  */
/* 				    sol_dofs_k, base_phi,  */
/* 				    base_dphix, base_dphiy, base_dphiz,  */
/* 				    xcoor, u_val, u_x, u_y, u_z, vec_norm); */
/*           area = determ*wg[ki]; */
/* 	  // every problem dependent module that uses ns_supg must provide  */
/* 	  // implementation of this procedure in files from ls_intf directory */
/* 	  pdr_ns_supg_give_me_temperature_at_point(Problem_id, neig_id, loc_xg,  */
/* 				       base_phi,base_dphix,base_dphiy,base_dphiz, */
/* 				       &tk, &t_grad[0], &t_grad[1], &t_grad[2]); */
/* 	  //double norm_dT, cos_dT_dTs; */
/* 	  double dTs_x, dTs_y, dTs_z, dTn, dTs_dot_n; */
/* 	  /\* norm_dT = sqrt( pow( t_grad[0], 2 ) + pow( t_grad[1], 2 ) + pow( t_grad[2], 2 ) ); *\/ */
/* 	  /\* if (norm_dT > 0.0) *\/ */
/* 	  /\* { *\/ */
/* 	  /\*   cos_dT_dTs = sqrt( 1 - pow( ( t_grad[0] * vec_norm[0] + t_grad[1] * vec_norm[1] + t_grad[2] * vec_norm[2] ), 2 ) / *\/ */
/* 	  /\* 		       ( pow( t_grad[0], 2 ) + pow( t_grad[1], 2 ) + pow( t_grad[2], 2 ) ) ); *\/ */
/* 	  /\* } else { *\/ */
/* 	  /\*   cos_dT_dTs = 0.0; *\/ */
/* 	  /\* } *\/ */
/* 	  dTn = t_grad[0] * vec_norm[0] + t_grad[1] * vec_norm[1] + t_grad[2] * vec_norm[2]; */
/* 	  dTs_x = t_grad[0] - vec_norm[0] * dTn; */
/* 	  dTs_y = t_grad[1] - vec_norm[1] * dTn; */
/* 	  dTs_z = t_grad[2] - vec_norm[2] * dTn; */
/* 	  //if (Fa_id == 965858 || Fa_id == 960529 ) { */
/* 	    /\* printf("\n\tAS: Fa_id = %d\t  x = %lf\t\t  y = %lf\t\t  z = %lf", Fa_id, xcoor[0], xcoor[1], xcoor[2]); *\/ */
/* 	    /\* printf("\n\tAS: Fa_id = %d\t n_x = %lf\t\t n_y = %lf\t\t n_z = %lf", Fa_id, vec_norm[0], vec_norm[1], vec_norm[2]); *\/ */
/* 	    /\* printf("\n\tAS: Fa_id = %d\tdT_x = %lf\t\tdT_y = %lf\t\tdT_z = %lf", Fa_id, t_grad[0], t_grad[1], t_grad[2]); *\/ */
/* 	    /\* printf("\n\tAS: Fa_id = %d\tdTn = %lf", Fa_id, dTn); *\/ */
/* 	    /\* printf("\n\tAS: Fa_id = %d\tdTs_x = %lf\t\tdTs_y = %lf\t\tdTs_z = %lf", Fa_id, dTs_x, dTs_y, dTs_z); *\/ */
/* 	    /\* dTs_dot_n = dTs_x * vec_norm[0] + dTs_y * vec_norm[1] + dTs_z * vec_norm[2]; *\/ */
/* 	    //if ( dTs_dot_n > 1.0E-5 ) printf("\n\t------>AS: Fa_id = %d\tdTs_x * n_x + dTs_y * n_y + dTs_z * n_z = %lf\n", Fa_id, dTs_dot_n); */
/* 	  //} */
	    
/* 	//norm_dT = sqrt( pow( t_grad[0], 2 ) + pow( t_grad[1], 2 ) + pow( t_grad[2], 2 ) ); */
/* 	//cos_dT_dTs = sqrt( 1 - pow( ( t_grad[0] * vec_norm[0] + t_grad[1] * vec_norm[1] + t_grad[2] * vec_norm[2] ), 2 ) / */
/* 	//		       ( pow( t_grad[0], 2 ) + pow( t_grad[1], 2 ) + pow( t_grad[2], 2 ) ) ); */

/*           //get thermal surface tension coefficients and VOF (fL) at this point  */
/* 	  // (VOF, dg_dT could be function of temperature) */
/* 	  query_params.material_idx = mmr_el_groupID(mesh_id, neig_id); */
/*           //query_params.material_idx = 0; //material by idx */
/*           query_params.name = ""; */
/*           query_params.temperature = tk; */
/* 	  query_params.cell_id = neig_id; */
/* 	  for( i=0; i<3; i++ ){ */
/* 	    query_params.xg[i] = xcoor[i]; */
/* 	    //printf("\n xcoor[%d] = %f", i, xcoor[i]); */
/* 	  } */
/* 	  //printf("\n"); */
/* 	  query_params.query_type = QUERY_NS_SUPG_POINT; */
/* 	  pdr_ns_supg_material_query(&problem->materials,  */
/* 		     &query_params, &query_result); */
/*           //calculate components of surface tension */
/*           for (wdofs = 0; wdofs < num_shap; wdofs++) { */
/*             Rhs_vect[wdofs * nreq + 0] +=  */
/* 	      area * base_phi[wdofs] * query_result.VOF * query_result.dg_dT * dTs_x; */
/*             Rhs_vect[wdofs * nreq + 1] +=  */
/* 	      area * base_phi[wdofs] * query_result.VOF * query_result.dg_dT * dTs_y; */
/*             Rhs_vect[wdofs * nreq + 2] +=  */
/* 	      area * base_phi[wdofs] * query_result.VOF * query_result.dg_dT * dTs_z; */

/* 	  } */

/* 	  if(Dynamic_viscosity<1e-9){ */
/* 	    // for Navier-Stokes with viscosity temperature dependent - HMT */
	    
/* 	    //3.set values to those obtained with query */
/* 	    Dynamic_viscosity = query_result.dynamic_viscosity; */
	    
/* 	    if(Dynamic_viscosity<1e-9){ */
/* 	      printf("dynamic viscosity %lf < 1.e-9 in comp_fa_stiff!\n", */
/* 		     Dynamic_viscosity); */
/* 	      printf("Exiting!\n"); exit(-1); */
/* 	    } */
/* 	  } */

      /* 	} // end loop over integration points */
      
      /* } // end if MARANGONI */
    
      /*kbw
         #ifdef DEBUG
         {
         int ibl;
         printf("leaving pdr_fa_stiff_mat:\n");
         printf("face %d, nrdofbl %d\n", Fa_id, *Nr_dof_ent);
         for(ibl=0;ibl<*Nr_dof_ent; ibl++){
         printf("bl_id %d, bl_nrdof %d\n",
         List_dof_ent_id[ibl],List_dof_ent_nrdof[ibl]);
         }
         }
         #endif
         /*kew */

/*kbw
if(Fa_id>0){
  kk = 0;
  printf("Face %d: Stiffness matrix:\n",Fa_id);
    for (jdofs=0;jdofs<num_dofs;jdofs++) {
      for (idofs=0;idofs<num_dofs;idofs++) {
	printf("%10.6lf",Stiff_mat[kk+idofs]);
      }
      kk+=num_dofs;
      printf("\n");
    }
    
    printf("\nRhs_vect for element:\n");
    for (idofs=0;idofs<num_dofs;idofs++) {
      printf("%10.6lf",Rhs_vect[idofs]);
    }
    printf("\n\n");
    getchar(); 
    } 
/*kew*/
     
    }	 /* end if computing entries to the stiffness matrix */
  }   /* end if computing SM and/or RHSV */

  if (Rewr_dofs != NULL)  *Rewr_dofs = 'F';

  return (1);
}


/*------------------------------------------------------------
pdr_ns_supg_get_velocity_at_point - to provide the velocity and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_ns_supg_weakform.c)
------------------------------------------------------------*/
int pdr_ns_supg_get_velocity_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Velocity, // velocity vector
  double *DVel_dx, // x-derivative of velocity vector
  double *DVel_dy, // y-derivative of velocity vector
  double *DVel_dz // z-derivative of velocity vector
					 )
{
  
  pdt_ns_supg_problem *problem = 
    (pdt_ns_supg_problem *)pdr_get_problem_structure(Problem_id);

  int field_id = problem->ctrl.field_id;
  int mesh_id = problem->ctrl.mesh_id;

  int nel = El_id;

  /* find degree of polynomial and number of element scalar dofs */
  int pdeg = 0;
  int base = apr_get_base_type(field_id, nel);
  apr_get_el_pdeg(field_id, nel, &pdeg);
  int num_shap = apr_get_el_pdeg_numshap(field_id, nel, &pdeg);
  int nreq=apr_get_nreq(field_id);
  
  /* get the coordinates of the nodes of nel in the right order */
  int el_nodes[MMC_MAXELVNO+1];    
  double node_coor[3*MMC_MAXELVNO]; 
  mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
  
  /* get the most recent solution degrees of freedom */
  int sol_vec_id = 1;
  double dofs_loc[APC_MAXELSD]={0}; /* element solution dofs */
  apr_get_el_dofs(field_id, nel, sol_vec_id, dofs_loc);

  int i, ieq;
  
  if(Velocity!=NULL && Base_phi!=NULL && DVel_dx==NULL){
    
    // get velocity 
    for(ieq=0;ieq<3;ieq++) Velocity[ieq]=0.0;
    for(i=0;i<num_shap;i++){
      for(ieq=0;ieq<3;ieq++){
    	Velocity[ieq] += dofs_loc[i*nreq+ieq]*Base_phi[i];
      }
    }
    
#ifdef DEBUG
    double Sol[PDC_MAXEQ];
    int iaux = 1;
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
    		     X_loc,node_coor,dofs_loc,
    		     Base_phi,NULL,NULL,NULL,
    		     NULL,Sol,NULL,NULL,NULL,NULL);
    for(ieq=0;ieq<3;ieq++){
      if(fabs(Velocity[ieq]-Sol[ieq])>1e-9){
	printf("Error 25yh483 in pdr_ns_supg_get_velocity_at_point. Exiting!\n");
	exit(-1);
      }
    }
#endif
    
    
  }
  else if(Velocity!=NULL && Base_phi!=NULL && DVel_dx!=NULL && Base_dphix!=NULL){
    

    for(ieq=0;ieq<3;ieq++){
      Velocity[ieq]=0.0;
      DVel_dx[ieq] = 0.0;
      DVel_dy[ieq] = 0.0;
      DVel_dz[ieq] = 0.0;
    }
    for(i=0;i<num_shap;i++){
      for(ieq=0;ieq<3;ieq++){
    	Velocity[ieq] += dofs_loc[i*nreq+ieq]*Base_phi[i];
	DVel_dx[ieq] += dofs_loc[i*nreq+ieq]*Base_dphix[i];
	DVel_dy[ieq] += dofs_loc[i*nreq+ieq]*Base_dphiy[i];
	DVel_dz[ieq] += dofs_loc[i*nreq+ieq]*Base_dphiz[i];
      }
    }
    
#ifdef DEBUG
    double Sol[PDC_MAXEQ];
    double Dsolx[PDC_MAXEQ], Dsoly[PDC_MAXEQ], Dsolz[PDC_MAXEQ];
    // get velocity and its derivatives
    int iaux = 2; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,node_coor,dofs_loc,
		     Base_phi,Base_dphix,Base_dphiy,Base_dphiz,
		     NULL, Sol, Dsolx, Dsoly, Dsolz, NULL);
    
    for(ieq=0;ieq<3;ieq++){
      if(fabs(Velocity[ieq]-Sol[ieq])>1e-9){
	printf("Error 25yha83 in pdr_ns_supg_get_velocity_at_point. Exiting!\n");
	exit(-1);
      }
      if(fabs(DVel_dx[ieq]-Dsolx[ieq])>1e-9){
	printf("Error 25yhb83 in pdr_ns_supg_get_velocity_at_point. Exiting!\n");
	exit(-1);
      }
      if(fabs(DVel_dy[ieq]-Dsoly[ieq])>1e-9){
	printf("Error 25yhc83 in pdr_ns_supg_get_velocity_at_point. Exiting!\n");
	exit(-1);
      }
      if(fabs(DVel_dz[ieq]-Dsolz[ieq])>1e-9){
	printf("Error 25yhd83 in pdr_ns_supg_get_velocity_at_point. Exiting!\n");
	exit(-1);
      }
    }
#endif
  
  }
  else if(Velocity!=NULL && Base_phi==NULL && DVel_dx==NULL ){

    double base_phi_loc[APC_MAXELVD];    /* basis functions */
    double Sol[PDC_MAXEQ];
    int iaux = 1;
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
    		     X_loc,node_coor,dofs_loc,
    		     base_phi_loc,NULL,NULL,NULL,
    		     NULL,Sol,NULL,NULL,NULL,NULL);
    for(ieq=0;ieq<3;ieq++){
      Velocity[ieq]=Sol[ieq];
    }
  }
  else if(Velocity!=NULL && Base_phi==NULL && DVel_dx!=NULL){

    double base_phi_loc[APC_MAXELVD];    /* basis functions */
    double base_dphix_loc[APC_MAXELVD];  /* x-derivatives of basis function */
    double base_dphiy_loc[APC_MAXELVD];  /* y-derivatives of basis function */
    double base_dphiz_loc[APC_MAXELVD];  /* z-derivatives of basis function */
    double Sol[PDC_MAXEQ];
    double Dsolx[PDC_MAXEQ], Dsoly[PDC_MAXEQ], Dsolz[PDC_MAXEQ];
    // get velocity and its derivatives
    int iaux = 2; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,node_coor,dofs_loc,
		     base_phi_loc,base_dphix_loc,base_dphiy_loc,base_dphiz_loc,
		     NULL, Sol, Dsolx, Dsoly, Dsolz, NULL);
    
    for(ieq=0;ieq<3;ieq++){
      Velocity[ieq] = Sol[ieq];
      DVel_dx[ieq] = Dsolx[ieq];
      DVel_dy[ieq] = Dsoly[ieq];
      DVel_dz[ieq] = Dsolz[ieq];
    }

  }
  else{

    printf("No good data - no computing.\n");
    exit(-1);

  }

  return(0);
}

/*------------------------------------------------------------
pdr_ns_supg_compute_CFL - to compute global CFL numbers (for a subdomain)
------------------------------------------------------------*/
int pdr_ns_supg_compute_CFL(
  int Problem_id,
  double *CFL_min_p,
  double *CFL_max_p,
  double *CFL_ave_p
)
{

  double cfl_min=1000000, cfl_max=0, cfl_ave=0.0;
  int nrelem=0;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  pdt_ns_supg_problem *problem = 
    (pdt_ns_supg_problem *)pdr_get_problem_structure(Problem_id);

  int field_id = problem->ctrl.field_id;
  int mesh_id = problem->ctrl.mesh_id;

  double delta_t = problem->time.cur_dtime;

  // loop over elements
  int nel = 0;
  while ((nel = mmr_get_next_act_elem(mesh_id, nel)) != 0) {

    /* find degree of polynomial and number of element scalar dofs */
    int pdeg = 0;
    int base = apr_get_base_type(field_id, nel);
    apr_get_el_pdeg(field_id, nel, &pdeg);
    int num_shap = apr_get_el_pdeg_numshap(field_id, nel, &pdeg);
    int nreq=apr_get_nreq(field_id);

    //printf("element %d, num_shap %d\n", nel, num_shap);

    /* get the coordinates of the nodes of nel in the right order */
    int el_nodes[MMC_MAXELVNO+1];    
    double node_coor[3*MMC_MAXELVNO]; 
    mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
    
    /* get the most recent solution degrees of freedom */
    int sol_vec_id = 1;
    double dofs_loc[APC_MAXELSD]={0}; /* element solution dofs */
    apr_get_el_dofs(field_id, nel, sol_vec_id, dofs_loc);
    
    
    // at one point (that exist for all types of elements)
    double x[3];     
    x[0]=0.0; x[1]=0.0; x[2]=0.0; 

    // get velocity and derivatives of shape functions 
    int iaux = 2; 
    double base_phi[APC_MAXELVD];    /* basis functions */
    double base_dphix[APC_MAXELVD];  /* x-derivatives of basis function */
    double base_dphiy[APC_MAXELVD];  /* y-derivatives of basis function */
    double base_dphiz[APC_MAXELVD];  /* y-derivatives of basis function */
    double xcoor[3];      /* global coord of gauss point */
    double u_val[PDC_NS_SUPG_NREQ]; /* computed solution */
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
			      x,node_coor,dofs_loc,
			      base_phi,base_dphix,base_dphiy,base_dphiz,
			      xcoor,u_val,NULL,NULL,NULL,NULL);


    // hsize - standard element size 
    double hsize = mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL);
    // h_k - element size in the direction of flow
    double h_k = 0.0; 

    double norm_u = 
      sqrt(u_val[0]*u_val[0]+u_val[1]*u_val[1]+u_val[2]*u_val[2]);

    if(norm_u < 1.0e-6){ // when there is no velocity field inside the element
      h_k = hsize;//take standard element size
    }
    else{ // take element size in the direction of velocity
      int idofs;
      for (idofs = 0; idofs < num_shap; idofs++) {
	h_k += fabs(u_val[0] * base_dphix[idofs] + 
		    u_val[1] * base_dphiy[idofs] + 
		    u_val[2] * base_dphiz[idofs]);
/*kbw
	//if(norm_u>0.00010){
    if(nel==49031){
      printf("element %d, num_shap %d, u_x %lf, u_y %lf, u_z %lf, h_k %lf\n", 
	     nel, num_shap, u_val[0], u_val[1], u_val[2], h_k);
      printf("element %d size: standard: %lf, norm u: %lf, dphix %lf, dphiy %lf, dphiz %lf\n",
	     nel, hsize, norm_u, base_dphix[idofs], base_dphiy[idofs], base_dphiz[idofs]);
    }
/*kew*/
      }
      h_k = norm_u / h_k;
    }

    double cfl = norm_u * delta_t / h_k;

/*kbw
  if(hsize < 0.0006) {
    if(cfl>0.10){
      printf("element %d size: standard: %lf, directional: %.12lf, norm u: %lf, CFL: %lf\n",
	     nel, hsize, h_k, norm_u, cfl);
    }
	   }
/*kew*/

    if(cfl<cfl_min) cfl_min = cfl;
    if(cfl>cfl_max) cfl_max = cfl;
    cfl_ave += cfl;
    nrelem++;

  }


  *CFL_min_p = cfl_min;
  *CFL_max_p = cfl_max;
  *CFL_ave_p = cfl_ave/nrelem;

  return(1);
}
