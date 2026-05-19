/************************************************************************
File pds_heat_ls_std_intf_util.c - utilities for the interface between 
  the problem dependent module and linear solver modules (direct and iterative)

Contains definitions of routines:

  pdr_heat_select_el_coeff_vect - to select coefficients returned 
            to approximation routines for element integrals in weak formulation
OBSOLETE  pdr_heat_select_el_coeff - to select coefficients returned to 
            approximation routines for element integrals in weak formulation
           (the procedure indicates which terms are non-zero in weak form)

  pdr_heat_el_coeff - to return coefficients for internal integrals

  pdr_heat_comp_el_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for an element       

  pdr_heat_comp_fa_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for an element containing a given face

  pdr_heat_get_temperature_at_point - to provide the temperature and its
    gradient at a particular point with local coordinates within an element

MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_heat_weakform.c )

  pdr_heat_compute_CFL - to compute and print local CFL numbers for element
  pdr_heat_get_velocity_at_point -  to provide the velocity and its
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
#include "../include/pdh_heat_problem.h"	/* USES */
/* weakform stabilization functions */
#include "../include/pdh_heat_weakform.h"	/* USES */
/* problem module's types and functions */
#include "../include/pdh_heat.h"	/* USES & IMPLEMENTS */

int pdr_heat_get_velocity_at_point(
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
					 );


/*------------------------------------------------------------
  pdr_heat_select_el_coeff_vect - to select coefficients  
                     returned to approximation routines for element integrals
------------------------------------------------------------*/
int pdr_heat_select_el_coeff_vect( // returns success indicator
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
  Coeff_vect_ind[17] = 0; // no reaction terms in heat problem
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
  pdr_heat_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals in weak formulation
------------------------------------------------------------*/
double *pdr_heat_select_el_coeff( 
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

  // heat problem is scalar - matrices and vectors are just scalars
  if(*Mval!=NULL) free(*Mval);
  *Mval = (double *) malloc(sizeof(double));
  if(*Axx!=NULL) free(*Axx);
  *Axx = (double *) malloc(sizeof(double));
  if(*Axy!=NULL) free(*Axy);
  *Axy = (double *) malloc(sizeof(double));
  if(*Axz!=NULL) free(*Axz);
  *Axz = (double *) malloc(sizeof(double));
  if(*Ayx!=NULL) free(*Ayx);
  *Ayx = (double *) malloc(sizeof(double));
  if(*Ayy!=NULL) free(*Ayy);
  *Ayy = (double *) malloc(sizeof(double));
  if(*Ayz!=NULL) free(*Ayz);
  *Ayz = (double *) malloc(sizeof(double));
  if(*Azx!=NULL) free(*Azx);
  *Azx = (double *) malloc(sizeof(double));
  if(*Azy!=NULL) free(*Azy);
  *Azy = (double *) malloc(sizeof(double));
  if(*Azz!=NULL) free(*Azz);
  *Azz = (double *) malloc(sizeof(double));
  if(*Bx!=NULL) free(*Bx);
  *Bx = (double *) malloc(sizeof(double));
  if(*By!=NULL) free(*By);
  *By = (double *) malloc(sizeof(double));
  if(*Bz!=NULL) free(*Bz);
  *Bz = (double *) malloc(sizeof(double));
  if(*Tx!=NULL) free(*Tx);
  *Tx = (double *) malloc(sizeof(double));
  if(*Ty!=NULL) free(*Ty);
  *Ty = (double *) malloc(sizeof(double));
  if(*Tz!=NULL) free(*Tz);
  *Tz = (double *) malloc(sizeof(double));
  if(*Cval!=NULL) free(*Cval); // no reaction terms in heat problem
  *Cval = NULL; // no reaction terms in heat problem
  if(*Lval!=NULL) free(*Lval);
  *Lval = (double *) malloc(sizeof(double));
  if(*Qx!=NULL) free(*Qx);
  *Qx = (double *) malloc(sizeof(double));
  if(*Qy!=NULL) free(*Qy);
  *Qy = (double *) malloc(sizeof(double));
  if(*Qz!=NULL) free(*Qz);
  *Qz = (double *) malloc(sizeof(double));
  if(*Sval!=NULL) free(*Sval);
  *Sval = (double *) malloc(sizeof(double));

  return (*Axx);
}

/*------------------------------------------------------------
pdr_heat_el_coeff - to return coefficients at Gauss point for internal 
                       element integrals for heat weak formulation
------------------------------------------------------------*/
int pdr_heat_el_coeff(
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
  double* Uk_x, 	/* in: gradient of computed solution Uk_val */
  double* Uk_y,   	/* in: gradient of computed solution Uk_val */
  double* Uk_z,   	/* in: gradient of computed solution Uk_val */
  double* Un_val, 	/* in: computed solution from previous time step */
  double* Un_x, 	/* in: gradient of computed solution Un_val */
  double* Un_y,   	/* in: gradient of computed solution Un_val */
  double* Un_z,   	/* in: gradient of computed solution Un_val */
  double* Mval,	/* out: mass matrix coefficient */
  double *Axx, double *Axy, double *Axz,  /* out:diffusion coefficients */
  double *Ayx, double *Ayy, double *Ayz,  /* e.g. Axy denotes scalar or matrix */
  double *Azx, double *Azy, double *Azz,  /* related to terms with dv/dx*du/dy */
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
  /* arguments SPECIFIC to heat problem */
  double *Velocity,		// in: velocity vector at point
  //double Thermal_diffusivity,	// in: thermal diffusivity - old
  double Thermal_conductivity,	// in: thermal conductivity - new 
  double Density_times_specific_heat,	// in: density*specific_heat - new
  double Delta_t,		// in: current time step
  double Implicitness_coeff	// in: implicitnes parameter alpha
)
{



/*+++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper field */
  int field_id = pdr_ctrl_i_params(Problem_id, 3);
  int mesh_id = apr_get_mesh_id(field_id);
  int nreq = PDC_HEAT_NREQ;
#ifdef DEBUG
  if(nreq != apr_get_nreq(field_id)){
    printf("wrong parameter HEAT_NREQ in pdr_heat_el_coeff 1\n");
    printf("%d != %d. Exiting !!!",nreq, apr_get_nreq(field_id));
    exit(-1);
  }
  if(nreq != pdr_ctrl_i_params(Problem_id,5)){
    printf("wrong parameter HEAT_NREQ in pdr_heat_el_coeff 2\n");
    printf("%d != %d. Exiting !!!",nreq, pdr_ctrl_i_params(Problem_id,5));
    exit(-1);
  }
#endif

  int idofs;
  int num_shap = apr_get_el_pdeg_numshap(field_id, El_id, &Pdeg);

/*kbw
  printf("In pdr_heat_el_coeff\n");
  printf("%d shape functions and derivatives: \n", num_shap);
  int i;
  for(i=0;i<num_shap;i++){
    printf("fun - %lf, der: x - %lf, y - %lf, z - %lf\n",
	   Base_phi[i],Base_dphix[i],Base_dphiy[i],Base_dphiz[i]);
  }
  printf("solution and derivatives at previous iteration (u_k): \n");
  printf("uk_x - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Uk_val[0],Uk_x[0],Uk_y[0],Uk_z[0]);
  printf("solution and derivatives at previous time step (u_n): \n");
  printf("Un_x - %lf, der: x - %lf, y - %lf, z - %lf\n",
	 Un_val[0],Un_x[0],Un_y[0],Un_z[0]);
  /*kew */
	 
/*kbw
  printf("Velocity %lf, %lf, %lf\n", Velocity[0], Velocity[1], Velocity[2]);
/*kew*/

  Mval[0]=0.0;
  Axx[0]=0.0; 
  Axy[0]=0.0; 
  Axz[0]=0.0; 
  Ayx[0]=0.0; 
  Ayy[0]=0.0; 
  Ayz[0]=0.0; 
  Azx[0]=0.0; 
  Azy[0]=0.0; 
  Azz[0]=0.0; 
  Bx[0]=0.0; 
  By[0]=0.0; 
  Bz[0]=0.0; 
  Tx[0]=0.0; 
  Ty[0]=0.0; 
  Tz[0]=0.0; 
  Lval[0]=0.0; 
  Qx[0]=0.0; 
  Qy[0]=0.0; 
  Qz[0]=0.0; 
  Sval[0]=0.0; 


  /*! ----------------------------------------------------------------------! */
  /*! ------------ CALCULATE ELEM. "SIZE" (h_k) AT GAUSS POINT----------- --! */
  /*! ----------------------------------------------------------------------! */
  /*
     Velocity[0] = 0.0;
  Velocity[1] = 2.0;
  Velocity[2] = 0.0;
*/
  
  
  double m_k = 1.0/3.0; // should be changed for higher order elements
  double norm_u = sqrt( Velocity[0] * Velocity[0] + 
			Velocity[1] * Velocity[1] +
			Velocity[2] * Velocity[2] );


  // h_k computations: FRANCA, TEZDUYAR
  double h_k = 0.0; 

  if(norm_u < 1.0e-6){ // when there is no velocity field inside the element
    h_k = mmr_el_hsize(mesh_id,El_id,NULL,NULL,NULL);//take standard element size
  }
  else{ // take element size in the direction of velocity
    // this definition may lead to oscillations - change then to standard size
    for (idofs = 0; idofs < num_shap; idofs++) {
            h_k += fabs(Velocity[0] * Base_dphix[idofs] + 
      		  Velocity[1] * Base_dphiy[idofs] + 
      		  Velocity[2] * Base_dphiz[idofs]);
    }
    h_k = 2.0 * norm_u / h_k;
  }
  

  
/*kbw
  printf("m_k %lf, norm_u %lf, h_k %lf, thermal_diff %lf\n",
	 m_k, norm_u, h_k, Thermal_diffusivity);
/*kew*/

  /*! ----------------------------------------------------------------------! */
  /*! ------------------ CALCULATE STABILIZATION COEFFS ----------------- --! */
  /*! ----------------------------------------------------------------------! */
  double peclet_local;
  if(Thermal_conductivity>1.e-6) {
    peclet_local = (m_k * norm_u * h_k * Density_times_specific_heat) / (2.0 * Thermal_conductivity);
  } else {
    peclet_local = 1.0e6;
  }
//  double ksi_peclet;
//  if ( peclet_local < 1.0 ) ksi_peclet = peclet_local;
//  else if (peclet_local >= 1.0) ksi_peclet = 1.0;
//  else { 
//    printf("ERROR: Peclet %lf < 0 (pdr_heat_el_coeff)\n", peclet_local); 
//    exit(-1);
// }
  
  double tau_therm; 
  if(peclet_local < 1.0) tau_therm = (h_k * h_k * m_k * Density_times_specific_heat) / (4.0*Thermal_conductivity);
  else if (peclet_local >= 1.0) tau_therm = h_k / (2.0 * norm_u);

/*kbw
  printf("h_k %lf (h_size %lf), normp_u %lf, peclet_local %lf, tau_therm %lf, dtsh %lf\n",
	 h_k, mmr_el_hsize(mesh_id,El_id,NULL,NULL,NULL),
	 norm_u, peclet_local, tau_therm, Density_times_specific_heat );
/*kew*/
  
 
   
   
   


  

  //      ((shp / Delta_t) * tst   
  // Mval[0] += 1/Delta_t; // old	
  Mval[0] += Density_times_specific_heat/Delta_t; // new
  
  // (vel_x * shp_dx +
  //Bx[0] += Implicitn ess_coeff * Velocity[0];  // old	
  Bx[0] += Implicitness_coeff * (Velocity[0]) * Density_times_specific_heat; // new
  //  vel_y * shp_dy + 
  //By[0] += Implicitness_coeff * Velocity[1];  // old	
  By[0] += Implicitness_coeff * (Velocity[1])* Density_times_specific_heat; // new
  //  vel_z * shp_dz) * tst +
  //Bz[0] += Implicitness_coeff * Velocity[2];  // old	
  Bz[0] += Implicitness_coeff * (Velocity[2]) * Density_times_specific_heat; // new
  
  if(Implicitness_coeff<1.0){
    double temp;

    temp = Velocity[0]*Un_x[0] + Velocity[1]*Un_y[0] + Velocity[2]*Un_z[0];
    //Sval[0] += (Implicitness_coeff-1.0) * temp; // old	
    Sval[0] += (Implicitness_coeff-1.0) * temp * Density_times_specific_heat; // new	

  }

  // stabilization terms are on implicit side only - no splitting
  // + (vel_x * shp_dx +  tau_therm * (vel_x * tst_dx
  Axx[0] += tau_therm * Velocity[0] * Velocity[0] * Density_times_specific_heat;
  // + (vel_x * shp_dx +  tau_therm * vel_y * tst_dy
  Ayx[0] += tau_therm * Velocity[1] * Velocity[0] * Density_times_specific_heat;
  // + (vel_x * shp_dx +  tau_therm * vel_z * tst_dz
  Azx[0] += tau_therm * Velocity[2] * Velocity[0] * Density_times_specific_heat;
  //    vel_y * shp_dy +  tau_therm * (vel_x * tst_dx
  Axy[0] += tau_therm * Velocity[0] * Velocity[1] * Density_times_specific_heat;
  //    vel_y * shp_dy +  tau_therm * vel_y * tst_dy
  Ayy[0] += tau_therm * Velocity[1] * Velocity[1] * Density_times_specific_heat;
  //    vel_y * shp_dy +  tau_therm * vel_z * tst_dz
  Azy[0] += tau_therm * Velocity[2] * Velocity[1] * Density_times_specific_heat;
  //    vel_z * shp_dz) *  tau_therm * (vel_x * tst_dx
  Axz[0] += tau_therm * Velocity[0] * Velocity[2] * Density_times_specific_heat;
  //    vel_z * shp_dz) *  tau_therm * vel_y * tst_dy
  Ayz[0] += tau_therm * Velocity[1] * Velocity[2] * Density_times_specific_heat;
  //    vel_z * shp_dz) *  tau_therm * vel_z * tst_dz
  Azz[0] += tau_therm * Velocity[2] * Velocity[2] * Density_times_specific_heat;

  // FOR STABILIZATION ON BOTH SIDES
  /* // + (vel_x * shp_dx +  tau_therm * (vel_x * tst_dx */
  /* Axx[0] += Implicitness_coeff * tau_therm * Velocity[0] * Velocity[0]; */
  /* // + (vel_x * shp_dx +  tau_therm * vel_y * tst_dy */
  /* Ayx[0] += Implicitness_coeff * tau_therm * Velocity[1] * Velocity[0]; */
  /* // + (vel_x * shp_dx +  tau_therm * vel_z * tst_dz */
  /* Azx[0] += Implicitness_coeff * tau_therm * Velocity[2] * Velocity[0]; */
  /* //    vel_y * shp_dy +  tau_therm * (vel_x * tst_dx */
  /* Axy[0] += Implicitness_coeff * tau_therm * Velocity[0] * Velocity[1]; */
  /* //    vel_y * shp_dy +  tau_therm * vel_y * tst_dy */
  /* Ayy[0] += Implicitness_coeff * tau_therm * Velocity[1] * Velocity[1]; */
  /* //    vel_y * shp_dy +  tau_therm * vel_z * tst_dz */
  /* Azy[0] += Implicitness_coeff * tau_therm * Velocity[2] * Velocity[1]; */
  /* //    vel_z * shp_dz) *  tau_therm * (vel_x * tst_dx */
  /* Axz[0] += Implicitness_coeff * tau_therm * Velocity[0] * Velocity[2]; */
  /* //    vel_z * shp_dz) *  tau_therm * vel_y * tst_dy */
  /* Ayz[0] += Implicitness_coeff * tau_therm * Velocity[1] * Velocity[2]; */
  /* //    vel_z * shp_dz) *  tau_therm * vel_z * tst_dz */
  /* Azz[0] += Implicitness_coeff * tau_therm * Velocity[2] * Velocity[2]; */

  /* if(Implicitness_coeff<1.0){ */

  /*   Qx[0] += (Implicitness_coeff-1.0) * tau_therm * Velocity[0] *  */
  /*     (Velocity[0]*Un_x[0] + Velocity[1]*Un_y[0] + Velocity[2]*Un_z[0]); */

  /*   Qy[0] += (Implicitness_coeff-1.0) * tau_therm * Velocity[1] *  */
  /*     (Velocity[0]*Un_x[1] + Velocity[1]*Un_y[1] + Velocity[2]*Un_z[1]); */

  /*   Qz[0] += (Implicitness_coeff-1.0) * tau_therm * Velocity[2] *  */
  /*     (Velocity[0]*Un_x[2] + Velocity[1]*Un_y[2] + Velocity[2]*Un_z[2]); */

  /* } */

  //  double Thermal_diffusivity = tconductivity / ( density * specific_heat );
  //Axx[0] += Implicitness_coeff * Thermal_diffusivity; // old
  Axx[0] += Implicitness_coeff * Thermal_conductivity;; // new
  // shp_dy * tst_dy + 
  //Ayy[0] += Implicitness_coeff * Thermal_diffusivity; // old
  Ayy[0] += Implicitness_coeff * Thermal_conductivity;; // new
  // shp_dz * tst_dz)
  //Azz[0] += Implicitness_coeff * Thermal_diffusivity; // old
  Azz[0] += Implicitness_coeff * Thermal_conductivity;; // new

  if(Implicitness_coeff<1.0){

    //Qx[0] += (Implicitness_coeff-1.0) * Thermal_diffusivity * Un_x[0]; // old
    Qx[0] += (Implicitness_coeff-1.0) * Thermal_conductivity * Un_x[0];

    //Qy[0] += (Implicitness_coeff-1.0) * Thermal_diffusivity * Un_y[0]; // old
    Qy[0] += (Implicitness_coeff-1.0) * Thermal_conductivity * Un_y[0];

    //Qz[0] += (Implicitness_coeff-1.0) * Thermal_diffusivity * Un_z[0]; // old
    Qz[0] += (Implicitness_coeff-1.0) * Thermal_conductivity * Un_z[0];

  }


  //     + (shp / Delta_t) *  tau_therm * (vel_x * tst_dx
  Tx[0] +=  (tau_therm * Velocity[0] * Density_times_specific_heat / Delta_t);;
  //     + (shp / Delta_t) *  tau_therm * (vel_y * tst_dy
  Ty[0] +=  (tau_therm * Velocity[1] * Density_times_specific_heat / Delta_t);;
  //     + (shp / Delta_t) *  tau_therm * (vel_y * tst_dz
  Tz[0] +=  (tau_therm * Velocity[2] * Density_times_specific_heat / Delta_t);;


  // ((tk / Delta_t) * tst + 
  Lval[0] += Un_val[0] * Density_times_specific_heat / Delta_t;

  // (tk / Delta_t) * /*res */ tau_therm * vel_x * tst_dx) 
  Qx[0] += Un_val[0] * Density_times_specific_heat / Delta_t * tau_therm * Velocity[0]; 
  // (tk / Delta_t) * /*res */ tau_therm * vel_y * tst_dy) 
  Qy[0] += Un_val[0] * Density_times_specific_heat / Delta_t * tau_therm * Velocity[1]; 
  // (tk / Delta_t) * /*res */ tau_therm * vel_z * tst_dz) 
  Qz[0] += Un_val[0] * Density_times_specific_heat / Delta_t * tau_therm * Velocity[2]; 

  //AS: current vector id = 1, sol_dofs_dtdt[0]
  /* num_shap = apr_get_el_pdeg_numshap(field_dtdt_id, El_id, &Pdeg); */
  //num_eq = pdv_heat_dtdt_problem.ctrl.nreq;
  //apr_read_ent_dofs(field_dtdt_id, APC_ELEMENT, El_id, num_eq, 1, sol_dofs_dtdt);
  //apr_read_ent_dofs(field_dtdt_id, APC_ELEMENT, El_id, num_eq, 1, sol_dofs_dtdt);
  //if ( sol_dofs_dtdt[0] != 0.0 )
  //  printf("\t\nAS (weakform): El_id = %d\tsol_dofs_dtdt[0] = %lf\tsol_dofs_dtdt[1] = %lf", El_id, sol_dofs_dtdt[0], sol_dofs_dtdt[1]);
  //Sval[0] += sol_dofs_dtdt[0];
/*
  int name = pdr_ctrl_i_params(PDC_USE_CURRENT_PROBLEM_ID, 1); 
  if(name == 23456){
    double   a1 = 1;
    double x = Xcoor[0];double y = Xcoor[1];double z = Xcoor[2]; 
    Sval[0] = -(2.0/a1)*((1-2*x*x/a1)+(1-2*y*y/a1)+(1-2*z*z/a1))
      *exp((-x*x-y*y-z*z)/a1);
  }
*/

  return (0);
}
  
/*------------------------------------------------------------
  pdr_heat_comp_el_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for an element
  the procedure uses apr_num_int_el and provides it with necessary parameters
  apr_num_int_el gets coefficients at each integration point from 
  pdr_el_coeff_heat procedure
------------------------------------------------------------*/
int pdr_heat_comp_el_stiff_mat(/*returns: >=0 -success code, <0 -error code */
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


  // to make OpenMP working
#define PDC_HEAT_NUM_DOFS_MAX 8

  int field_id, mesh_id, nreq;  
  int kk, idofs, jdofs;
  int i,j,k;
  int max_nrdofs;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  /* select the proper field */
  field_id = pdr_ctrl_i_params(Problem_id, 3);
  mesh_id = apr_get_mesh_id(field_id);
  //nreq =apr_get_nreq(field_id);

  nreq = PDC_HEAT_NREQ;

#ifdef DEBUG
  if(nreq != apr_get_nreq(field_id)){
    printf("PDC_HEAT_NREQ (%d) != nreq (%d)\n",
	   nreq, apr_get_nreq(field_id));
    exit(-1);
  }
#endif


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

    if(num_dofs>PDC_HEAT_NUM_DOFS_MAX){
      printf("num_dofs (%d) > PDC_HEAT_NUM_DOFS_MAX (%d) used for arrays dimensions\n",
	     num_dofs, PDC_HEAT_NUM_DOFS_MAX); 
      exit(-1);
    }


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
printf("NULL arrays Stiff_mat and Rhs_vect in pdr_heat_comp_stiff_el_mat. Exiting!");
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

      printf("Inactive element in pdr_heat_comp_el_stiff_mat! Exiting.\n");
      exit(-1);

      for (i = 0; i < num_dofs; i++) {
	sol_dofs_n[i] = 0;
	sol_dofs_k[i] = 0;
      }
    }

/*kbw
  for(i=0;i<6;i++){
    printf("Node: %d\n", i);
    printf("uk_x: %lf\tuk_y: %lf\tuk_z: %lf\t\n", 
         sol_dofs_k[i*3], sol_dofs_k[i*3+1], sol_dofs_k[i*3+2]);
    printf("un_x: %lf\tun_y: %lf\tun_z: %lf\t\n", 
         sol_dofs_n[i*3], sol_dofs_n[i*3+1], sol_dofs_n[i*3+2]);
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

    int diagonal[5]={0,0,0,0,0}; // diagonality of: M, A_ij, B_j, T_i, C 
          // diagonality has no meaning here since coefficients are scalar  
    /* perform numerical integration of terms from the weak formulation */
    apr_num_int_el(Problem_id,field_id,El_id,Comp_sm,&pdeg,sol_dofs_k,sol_dofs_n,
		   diagonal, Stiff_mat, Rhs_vect);



/*kbw
    if(El_id>0){
      printf("Element %d: Stiffness matrix:\n",El_id);
      for (idofs=0;idofs<num_dofs*nreq;idofs++) {
	for (jdofs=0;jdofs<num_dofs*nreq;jdofs++) {
	  printf("%20.12lf",Stiff_mat[idofs+jdofs*num_dofs*nreq]);    
	  if(  Stiff_mat[idofs+jdofs*num_dofs*nreq] < -3000 ){
	    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	    getchar(); getchar(); getchar();
	  }
	} 
	printf("\n");
      }
      printf("Element %d: Rhs_vect:\n",El_id);
      for (idofs=0;idofs<num_dofs*nreq;idofs++) {
	printf("%20.12lf",Rhs_vect[idofs]);
      }
      printf("\n");
      //getchar(); 
    } 
/*kew*/

    if(Rewr_dofs != NULL) *Rewr_dofs = 'F';

  } /* end if computing SM and/or RHSV */

  return(1);
}

/*------------------------------------------------------------
  pdr_heat_comp_fa_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for a face
------------------------------------------------------------*/
int pdr_heat_comp_fa_stiff_mat(/*returns: >=0 -success code, <0 -error code */
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
  double u_val_hat[PDC_MAXEQ];	/* specified solution for Dirichlet BC */
  double u_val[PDC_MAXEQ];	/* computed solution */
  double u_x[PDC_MAXEQ];		/* gradient of computed solution */
  double u_y[PDC_MAXEQ];		/* gradient of computed solution */
  double u_z[PDC_MAXEQ];		/* gradient of computed solution */
  double base_phi[APC_MAXELVD];	/* basis functions */
  
  double base_phi_temp[APC_MAXELVD];
  
  double base_dphix[APC_MAXELVD];	/* x-derivatives of basis function */
  double base_dphiy[APC_MAXELVD];	/* y-derivatives of basis function */
  double base_dphiz[APC_MAXELVD];	/* y-derivatives of basis function */
  double sol_dofs_n[APC_MAXELSD];	/* solution dofs */
  double sol_dofs_k[APC_MAXELSD];	/* solution dofs */
  double tk;
  int pp_node, vp_node;
  double x, y, z;
  int ignore_bc_vel_nodes_idx[4] = { 0, 0, 0, 0 };  //4 - max no. of face nodes

  double x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3; 
  
  
  pdt_heat_bctype bc_type_heat = BC_HEAT_NONE;
  utt_material_query_params query_params;
  utt_material_query_result query_result;

  double total_flux, goldak_flux, radconv_flux, rad_flux, conv_flux, rad;
  double density, specific_heat;

  /* some pointers to type less */
  pdt_heat_problem *problem =
    (pdt_heat_problem *) pdr_get_problem_structure(Problem_id);
  pdt_heat_ctrls *ctrls = &problem->ctrl;
  pdt_heat_times *time = &problem->time;
  pdt_heat_nonls *nonl = &problem->nonl;



  /* select the proper mesh */
  field_id = ctrls->field_id;
  mesh_id = apr_get_mesh_id(field_id);

  /* get type of face and bc */
  fa_type = mmr_fa_type(mesh_id, Fa_id);
  mmr_fa_area(mesh_id, Fa_id, &daux, vec_norm);
  hf_size = sqrt(daux);
  /* get boundary number */
  fa_bnum = mmr_fa_bc(mesh_id, Fa_id);
  /* get heat and flow b.condition types for boundary(face) */
  bc_type_heat = pdr_heat_get_bc_type(&problem->bc, fa_bnum);

  /* get approximation parameters */
  //nreq = ctrls->nreq;
  nreq = PDC_HEAT_NREQ;

  /* get neighbors list with corresponding neighbors' sides numbers */
  mmr_fa_neig(mesh_id, Fa_id, face_neig, neig_sides, &node_shift, NULL, acoeff, bcoeff);

  neig_id = abs(face_neig[0]);	//boundary face's element id

  apr_get_el_pdeg(field_id, neig_id, &pdeg);
  num_shap = apr_get_el_pdeg_numshap(field_id, neig_id, &pdeg);
  num_dofs = num_shap * nreq;
  base = apr_get_base_type(field_id, neig_id);

  mmr_el_node_coor(mesh_id, neig_id, el_nodes, node_coor);

  /*kbw
     printf("In pdr_heat_comp_fa_stiff_mat: field_id %d, mesh_id %d, Comp_sm %d\n",
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
/* get the most recent solutions degrees of freedom */
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
      /*! --------------- THERMAL BOUNDARY CONDITIONS -----------------------! */
      /*! -------------------------------------------------------------------! */

      /*! ------------------------------------------------------------------! */
      /*! -------------------------- ISOTHERMAL  ---------------------------! */
      if (bc_type_heat == BC_HEAT_ISOTHERMAL) {	//TODO: change to switch
	/*! DIAGNOSTICS - uncomment ! */
	//printf("------BC_HEAT_ISOTHERMAL BEGIN-----------------\n");
	//printf("Fa_id: %d\n",Fa_id);

	//get bc data
	pdt_heat_bc_isothermal *bc_isothermal_data = 
	  (pdt_heat_bc_isothermal *) pdr_heat_get_bc_data(&problem->bc, fa_bnum);

	for (idofs = 0; idofs < num_fa_nodes; ++idofs) {
	  kk = (num_dofs * fa_nodes[idofs] + fa_nodes[idofs]);

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("Idofs: %d\t\t",idofs);
	  //printf("x: %lf\ty: %lf\tz: %lf\n",node_coor[3*fa_nodes[idofs]],node_coor[3*fa_nodes[idofs]+1],node_coor[3*fa_nodes[idofs]+2]);
	  //printf("\tStiff & Rhs at .. will be written:\n");

	  Stiff_mat[kk] += penalty;
	  Rhs_vect[fa_nodes[idofs]] += (bc_isothermal_data->temp) * penalty;	//== given temperature on DIRICHLET boundary

	  /*! DIAGNOSTICS - uncomment ! */
	  //printf("\t\t%d\t%d\n", kk+j, fa_nodes[idofs]*nreq + ieq);
	}
	/*! DIAGNOSTICS - uncomment ! */
	//printf("******BC_HEAT_ISOTHERMAL END**************************\n");
      }

      /*! ------------------------------------------------------------------! */
      /*! -------------------------- HEAT FLUX  (NEUMANN)-------------------! */
      else if (bc_type_heat == BC_HEAT_NORMAL_HEAT_FLUX) {	//TODO: change to switch
	//get bc data
	pdt_heat_bc_normal_flux *bc_normal_heat_flux_data = 
	  (pdt_heat_bc_normal_flux *) pdr_heat_get_bc_data(&problem->bc,fa_bnum);

	apr_set_quadr_2D(fa_type, base, &pdeg, &ngauss, xg, wg);

	for (ki = 0; ki < ngauss; ++ki) {
// find coordinates within neighboring elements for a point on face 
	  // !!! for triangular faces coordinates are standard [0,1][0,1] 
	  // !!! for quadrilateral faces coordinates are [-1,1][-1,1] - 
	  // !!! which means that they do not conform to element coordinates 
	  // !!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!! 
	  mmr_fa_elem_coor(mesh_id, &xg[2*ki], face_neig, neig_sides, node_shift,
			   acoeff, bcoeff, loc_xg);
	  iaux = 3 + neig_sides[0];	// boundary data for face neig_sides[0]
	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, loc_xg, node_coor, 
				    sol_dofs_k, base_phi, base_dphix, base_dphiy,
				    base_dphiz, xcoor, u_val, u_x, u_y, u_z, 
				    vec_norm);
	  area = determ * wg[ki];
	  tk = u_val[0];	//most recent temperature at this gauss point

	  // CHECK WHETHER MATERIAL DATABASE USED AT ALL
	  /* //get thermal conductivity at this gauss point (could be function of temperature) */
	  /* query_params.material_idx = mmr_el_groupID(mesh_id, neig_id); */
	  /* //query_params.material_idx = 0;	//material by idx */
	  /* query_params.name = ""; */
	  /* query_params.temperature = tk; */
	  /* query_params.cell_id = neig_id; */
	  /* for( i=0; i<3; i++ ){ */
	  /*   query_params.xg[i] = xcoor[i]; */
	  /* } */
	  /* query_params.query_type = QUERY_HEAT_POINT; */
	  /* pdr_heat_material_query(&problem->materials, &query_params, &query_result); */
          /* specific_heat = query_result.specific_heat; */
          /* density = query_result.density; */


/*kbw
	//if(norm_u>0.00010){
	  if(nel==49031){
	    printf("element %d, num_shap %d, u_x %lf, u_y %lf, u_z %lf, h_k %lf\n", 
		   nel, num_shap, u_val[0], u_val[1], u_val[2], h_k);
	    for (idofs = 0; idofs < num_shap; idofs++) {
	      printf("element %d size: standard: %lf, norm u: %lf, dphix %lf, dphiy %lf, dphiz %lf\n",
		     nel, area, norm_u, 
		     base_dphix[idofs], base_dphiy[idofs], base_dphiz[idofs]);
      }
	  }
/*kew*/


	  for (i = 0; i < num_shap; ++i) {
	    Rhs_vect[i] += area * base_phi[i] * bc_normal_heat_flux_data->flux;
	  }
	}			//loop over gauss points
      }				//bc normal heat flux

      /*! ------------------------------------------------------------------! */
      /*! -------------------------- OUTFLOW -------------------! */
      else if (bc_type_heat == BC_HEAT_OUTFLOW) {	//TODO: change to switch

	  
	  
	  printf("OUTFLOW - ZERO DIFFUSIVE HEAT FLUX (ONLY CONVECTIVE)");
/* 	apr_set_quadr_2D(fa_type, base, &pdeg, &ngauss, xg, wg); */

/* 	for (ki = 0; ki < ngauss; ++ki) { */
/* // find coordinates within neighboring elements for a point on face  */
/* 	  // !!! for triangular faces coordinates are standard [0,1][0,1]  */
/* 	  // !!! for quadrilateral faces coordinates are [-1,1][-1,1] -  */
/* 	  // !!! which means that they do not conform to element coordinates  */
/* 	  // !!! proper care is taken in mmr_fa_elem_coor !!!!!!!!!!!!!!!  */
/* 	  mmr_fa_elem_coor(mesh_id, &xg[2*ki], face_neig, neig_sides, node_shift, */
/* 			   acoeff, bcoeff, loc_xg); */
/* 	  iaux = 3 + neig_sides[0];	// boundary data for face neig_sides[0] */
/* 	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, loc_xg, node_coor,  */
/* 				    sol_dofs_k, base_phi, base_dphix, base_dphiy, */
/* 				    base_dphiz, xcoor, u_val, u_x, u_y, u_z,  */
/* 				    vec_norm); */
/* 	  area = determ * wg[ki]; */
/* 	  tk = u_val[0];	//most recent temperature at this gauss point */

/* 	  //get thermal conductivity at this gauss point (could be function of temperature) */
/* 	  query_params.material_idx = mmr_el_groupID(mesh_id, neig_id); */
/* 	  //query_params.material_idx = 0;	//material by idx */
/* 	  query_params.name = ""; */
/* 	  query_params.temperature = tk; */
/* 	  query_params.cell_id = neig_id; */
/* 	  for( i=0; i<3; i++ ){ */
/* 	    query_params.xg[i] = xcoor[i]; */
/* 	  } */
/* 	  query_params.query_type = QUERY_HEAT_POINT; */
/* 	  pdr_heat_material_query(&problem->materials, &query_params, &query_result); */
/*           //specific_heat = query_result.specific_heat; */
/*           //density = query_result.density; */
/* 	  double thermal_conductivity = query_result.thermal_conductivity; */

/* /\*kbw */
/* 	//if(norm_u>0.00010){ */
/* 	  if(nel==49031){ */
/* 	    printf("element %d, num_shap %d, u_x %lf, u_y %lf, u_z %lf, h_k %lf\n",  */
/* 		   nel, num_shap, u_val[0], u_val[1], u_val[2], h_k); */
/* 	    for (idofs = 0; idofs < num_shap; idofs++) { */
/* 	      printf("element %d size: standard: %lf, norm u: %lf, dphix %lf, dphiy %lf, dphiz %lf\n", */
/* 		     nel, area, norm_u,  */
/* 		     base_dphix[idofs], base_dphiy[idofs], base_dphiz[idofs]); */
/*       } */
/* 	  } */
/* /\*kew*\/ */

	  
/* 	     for (jdofs = 0; jdofs < num_shap; jdofs++) { */
/* 	       for (idofs = 0; idofs < num_shap; idofs++) { */

/* 	         Stiff_mat[jdofs * num_dofs + idofs ] +=  */
/* 		   - area * base_phi[idofs] * thermal_conductivity * */
/* 	            (vec_norm[0]*base_dphix[jdofs] +  */
/* 	             vec_norm[1]*base_dphiy[jdofs] +  */
/* 	             vec_norm[2]*base_dphiz[jdofs]); */
/* 	       } // idofs */
/* 	     } // jdofs     */
	  

/* 	}			//loop over gauss points */

      }				//bcoutflow

      /*! ------------------------------------------------------------------! */
      /*! -------------------- RADCONV (NEUMANN)----------------------------! */
      else if ((bc_type_heat == BC_HEAT_RADCONV)) {	//TODO: change to switch
	//get bc data
	pdt_heat_bc_radconv *bc_radconv;

	bc_radconv = (pdt_heat_bc_radconv *) 
	               pdr_heat_get_bc_data(&problem->bc, fa_bnum);

	apr_set_quadr_2D(fa_type, base, &pdeg, &ngauss, xg, wg);

			
	for (ki = 0; ki < ngauss; ++ki) {
	  mmr_fa_elem_coor(mesh_id, &xg[2*ki], face_neig, neig_sides, node_shift,
			   acoeff, bcoeff, loc_xg);
	  iaux = 3 + neig_sides[0];	// boundary data for face neig_sides[0]
	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, loc_xg, node_coor, 
				    sol_dofs_k, base_phi, base_dphix, base_dphiy,
				    base_dphiz, xcoor, u_val, u_x, u_y, u_z, 
				    vec_norm);
	  area = determ * wg[ki];
	  tk = u_val[0];	//most recent temperature at this gauss point
	  
	  //bc_radconv->t_inf == bc_radconv->h
	  /* //get thermal conductivity at this gauss point (could be function of temperature) */
	  // CHECK WHETHER MATERIAL DATABASE USED AT ALL
	  /* query_params.material_idx = mmr_el_groupID(mesh_id, neig_id); */
	  /* //query_params.material_idx = 0;	//material by idx */
	  /* query_params.name = ""; */
	  /* query_params.temperature = tk; */
	  /* query_params.cell_id = neig_id; */
	  /* for( i=0; i<3; i++ ){ */
	  /*   query_params.xg[i] = xcoor[i]; */
	  /* } */
	  /* query_params.query_type = QUERY_HEAT_POINT; */
	  /* pdr_heat_material_query(&problem->materials, &query_params, &query_result); */
          /* specific_heat = query_result.specific_heat; */
          /* density = query_result.density; */

	  //calculate fluxes/////////////////////////////////////////////////
	  //	  conv_flux = -(bc_radconv->h * (tk - ctrls->ambient_temperature));
	  //conv_flux = bc_radconv->h * ctrls->ambient_temperature;
	  //rad_flux  = -(PDC_HEAT_SBC * bc_radconv->eps * (tk * tk * tk * tk - ctrls->ambient_temperature * ctrls->ambient_temperature * ctrls->ambient_temperature * ctrls->ambient_temperature));
          //radconv_flux = conv_flux + rad_flux;
	  //total_flux =  radconv_flux;
	  //double temp_0 = bc_radconv->temp_0;
	  
	  
	    double temp_out = bc_radconv->t_inf;
			if(temp_out==0){temp_out = ctrls->ambient_temperature;}
			
	 
		double conv_flux_coeff;
		
/*	   
	   
	  double daneT[26]={602.91,602.99,603.08,603.16,605.57,623.91,649.38,669.42,682.43,697.6,718.72,752.99,807.39,831.42,845.86,862.03,879.92,899.45,920.45,942.61,965.51,988.78,1009.84,1025.97,1037.36,1044.61};
double daneA[26]={750.0,800.0,1000.0,1001.8,1082.2,1569.2,1977.9,2129.6,2559.2,3075.6,3559.2,4029.7,4471.0,4523.6,4500.0,4431.0,4307.4,4120.1,3860.0,3517.9,3084.8,2551.5,1908.8,1296.2,1000.0,750.0};
double wartx,alfa1;
int liniowo;

liniowo=1;
wartx=temp_out;

if(tk>temp_out){wartx= tk;}


if(wartx<=daneT[0]){alfa1=daneA[0];liniowo=0;}
if(wartx>=daneT[25]){alfa1=daneA[25];liniowo=0;}


if(liniowo){
		int i=1;
        for(;i<26;++i){
                if(wartx<daneT[i]){
                alfa1=((daneT[i]-wartx)/(daneT[i]-daneT[i-1]))*daneA[i-1]+((wartx-daneT[i-1])/(daneT[i]-daneT[i-1]))*daneA[i]; break;
                }
        }
}

conv_flux_coeff =alfa1; 
*/
   
	
    if(bc_radconv->setting_length>1){conv_flux_coeff = pdr_heat_radconv_get_alfa_in_the_temp(bc_radconv,tk);} //Function takes a higher temperature
	else{conv_flux_coeff = bc_radconv->alfa;}
	
   
	
		
	   
	   
	   
	  double rad_flux_coeff = PDC_HEAT_SBC * bc_radconv->eps *
	                           (tk*tk+temp_out * temp_out)*
	                           (tk+temp_out);
	  double flux_coeff = conv_flux_coeff + rad_flux_coeff;

	  for (jdofs = 0; jdofs < num_shap; jdofs++) {
	    for (idofs = 0; idofs < num_shap; idofs++) {
	      Stiff_mat[jdofs * num_dofs + idofs ] += 
		area * base_phi[idofs] * flux_coeff * base_phi[jdofs];
	    } // idofs
	  } // jdofs    

	  for (i = 0; i < num_shap; ++i) {
	    Rhs_vect[i] += area*base_phi[i]*flux_coeff*(temp_out);
      }			//bc normal heat flux            
    
	}//loop over gauss points
	
	
	}//bc randcov
	
      /*! ------------------------------------------------------------------! */
      /*! ----------------- GOLDAK HEAT SOURCE (NEUMANN)--------------------! */
      /*! note: it's position is updated elsewhere - in time integration    ! */

      else if ((bc_type_heat == BC_HEAT_GOLDAK_HEAT_SOURCE)) {	
	                                          //TODO: change to switch
	//get bc data
	pdt_heat_bc_goldak_source *bc_goldak;
	
	bc_goldak = (pdt_heat_bc_goldak_source *) 
	  pdr_heat_get_bc_data(&problem->bc, fa_bnum);
	
	apr_set_quadr_2D(fa_type, base, &pdeg, &ngauss, xg, wg);
	
	for (ki = 0; ki < ngauss; ++ki) {
	  mmr_fa_elem_coor(mesh_id, &xg[2*ki], face_neig, neig_sides, node_shift,
			   acoeff, bcoeff, loc_xg);
	  iaux = 3 + neig_sides[0];	// boundary data for face neig_sides[0]
	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, loc_xg, node_coor, 
				    sol_dofs_k, base_phi, base_dphix, base_dphiy,
				    base_dphiz, xcoor, u_val, u_x, u_y, u_z, 
				    vec_norm);
	  area = determ * wg[ki];
	  tk = u_val[0];	//most recent temperature at this gauss point
	  
	  // CHECK WHETHER MATERIAL DATABASE USED AT ALL
	  /* //get thermal conductivity at this gauss point (could be function of temperature) */
	  /* query_params.material_idx = mmr_el_groupID(mesh_id, neig_id); */
	  /* //query_params.material_idx = 0;	//material by idx */
	  /* query_params.name = ""; */
	  /* query_params.temperature = tk; */
	  /* query_params.cell_id = neig_id; */
	  /* for( i=0; i<3; i++ ){ */
	  /*   query_params.xg[i] = xcoor[i]; */
	  /*   //printf("\n xcoor[%d] = %f", i, xcoor[i]); */
	  /* } */
	  /* //printf("\n"); */
	  /* query_params.query_type = QUERY_HEAT_POINT; */
	  /* pdr_heat_material_query(&problem->materials,  */
	  /* 			  &query_params, &query_result); */
          /* specific_heat = query_result.specific_heat; */
          /* density = query_result.density; */
	  //calculate fluxes////////////////////////////////////////
	  double nG_dot_nS, rad;
	  nG_dot_nS = bc_goldak->versor[0]*vec_norm[0]+bc_goldak->versor[1]*vec_norm[1]+bc_goldak->versor[2]*vec_norm[2];
	  rad =sqrt( (xcoor[0]-bc_goldak->current_pos[0]) * (xcoor[0]-bc_goldak->current_pos[0]) + 
		     (xcoor[1]-bc_goldak->current_pos[1]) * (xcoor[1]-bc_goldak->current_pos[1]) + 
		     (xcoor[2]-bc_goldak->current_pos[2]) * (xcoor[2]-bc_goldak->current_pos[2]) );
	  //if ( nG_dot_nS >= -0.50 )
	  //if ( rad >= (5.0 * bc_goldak->a ) )
	  //  printf("\nAS: Fa_id = %d\tnG_dot_nS = %lf", Fa_id, nG_dot_nS);
	  if ( nG_dot_nS < 0.0 )
	  {
	    goldak_flux = -nG_dot_nS * pdr_heat_bc_get_goldak_hf_at_point(
	       /* x coor (global) of current Gauss point */ xcoor[0],
	       /* y coor (global) of current Gauss point */ xcoor[1],
	       /* z coor (global) of current Gauss point */ xcoor[2],
	       /* bc goldak data for current boundary    */ bc_goldak,
	       /* norm vector (outward)           */ vec_norm) * bc_goldak->b; 
	    //Goldak source depth [W/m^2];
            // ??? direction on the outside: conv_flux < 0.0, rad_flux < 0.0;
	  } else {
	    goldak_flux = 0.0;
	  }
	  //conv_flux = -(bc_goldak->h * (tk - ctrls->ambient_temperature));
	  //rad_flux = -(PDC_HEAT_SBC * bc_goldak->eps * (tk * tk * tk * tk - ctrls->ambient_temperature * ctrls->ambient_temperature * ctrls->ambient_temperature * ctrls->ambient_temperature));
          //radconv_flux = conv_flux + rad_flux;
	  //total_flux = goldak_flux + radconv_flux;
	  //////////////////////////////////////////////////////////////////
	  //double conv_flux_coeff = bc_radconv->h;
	  
	  double conv_flux_coeff = bc_goldak->h;
	  
	  double rad_flux_coeff = PDC_HEAT_SBC * bc_goldak->eps *
	                           (tk*tk+ctrls->ambient_temperature * ctrls->ambient_temperature)*
	                           (tk+ctrls->ambient_temperature);
	  double flux_coeff = conv_flux_coeff + rad_flux_coeff;

	  //total_flux =  conv_flux;
	  /* printf("\n\t\tAS: Goldak-> gauss point no %d, [%lf, %lf, %lf]\n\t\ttk = %lf, ta = %lf\n\t\tgoldak = %lf, conv = %lf, rad = %lf\n\t\ttotal = %lf\t\ttotal_normalized = %lf", */
	  /* 	 ki, xcoor[0], xcoor[1], xcoor[2], tk, ctrls->ambient_temperature, goldak_flux, conv_flux, rad_flux, total_flux, total_flux/(density*specific_heat)); */

	  for (jdofs = 0; jdofs < num_shap; jdofs++) {
	    for (idofs = 0; idofs < num_shap; idofs++) {
	      Stiff_mat[jdofs * num_dofs + idofs ] += 
		area * base_phi[idofs] * flux_coeff * base_phi[jdofs];
	    } // idofs
	  } // jdofs    

	  for (i = 0; i < num_shap; ++i) {
	    Rhs_vect[i] += area*base_phi[i]*(flux_coeff*ctrls->ambient_temperature + goldak_flux);
            ///////////TEST////////////////////////////////////////////////////////////////////
            //if ( rad <= (0.05 * bc_goldak->a ))
              //printf("\n\t\t\tAS: Heat -> Rhs_vect[%d * %d + 4] += %lf", i, nreq, area * base_phi[i] * total_flux / (density * specific_heat));
            ///////////////////////////////////////////////////////////////////////////////////
	  }

	  ////////////////////////////////////////////////////////////////////////////////////
          //////////////TEST//////////////////////////////////////////////////////////////////
          //if ( rad <= (0.02 * bc_goldak->a )) {
	  //printf("\n\tAS: h = %lf, eps = %lf\n", bc_goldak->h, bc_goldak->eps);
	  //printf("\n\t\tAS: Goldak-> source point [%lf, %lf, %lf], rad = %lf\n", bc_goldak->current_pos[0], bc_goldak->current_pos[1], bc_goldak->current_pos[2], rad);
	  //printf("\n\t\tAS: Goldak-> gauss point no %d, [%lf, %lf, %lf]\n\t\ttk = %lf, ta = %lf\n\t\tgoldak = %l


	  /* for (i = 0; i < num_shap; ++i) { */
	  /*   Rhs_vect[i] += area*base_phi[i]*total_flux/(density*specific_heat); */
          /*   ///////////TEST//////////////////////////////////////////////////////////////////// */
          /*   //if ( rad <= (0.05 * bc_goldak->a )) */
          /*     //printf("\n\t\t\tAS: Heat -> Rhs_vect[%d * %d + 4] += %lf", i, nreq, area * base_phi[i] * total_flux / (density * specific_heat)); */
          /*   /////////////////////////////////////////////////////////////////////////////////// */
	  /* } */
	  
          //////////////TEST///////////////////////////////////////////////
          //if ( rad <= (0.02 * bc_goldak->a )) {
	  //printf("\n\tAS: h = %lf, eps = %lf\n", bc_goldak->h, bc_goldak->eps);
	  //printf("\n\t\tAS: Goldak-> source point [%lf, %lf, %lf], rad = %lf\n", bc_goldak->current_pos[0], bc_goldak->current_pos[1], bc_goldak->current_pos[2], rad);
	  //printf("\n\t\tAS: Goldak-> gauss point no %d, [%lf, %lf, %lf]\n\t\ttk = %lf, ta = %lf\n\t\tgoldak = %lf, conv = %lf, rad = %lf\n\t\ttotal = %lf",
	  //   ki, xcoor[0], xcoor[1], xcoor[2], tk, g_pdv_cfg.ambient_temperature, goldak_flux, conv_flux, rad_flux, total_flux);
          //}
          /////////////////////////////////////////////////////////////////////////////////////
	  
	  /*
	  kk = 0;
	  naj = 0;
	     for (jdofs = 0; jdofs < num_shap; jdofs++) {
	       nai = 0;
	       for (idofs = 0; idofs < num_shap; idofs++) {
	         //only temperature related entries - that's why 4
	         Stiff_mat[jdofs * kk + nai + 4 + num_dofs * 4] += area * base_phi[idofs] * query_result.thermal_conductivity *
	           (vec_norm[0] * base_dphix[jdofs] + 
	            vec_norm[1] * base_dphiy[jdofs] + 
	            vec_norm[2] * base_dphiz[jdofs]) / (density * specific_heat);
	         nai += nreq;
                 ///////////TEST////////////////////////////////////////////////////////////////////
                 //if ( rad <= (0.02 * bc_goldak->a ))
                   //printf("\n\t\t\tAS: Heat -> Stiff_mat[%d * %d + %d + 4 + %d * 4] += %lf", jdofs, kk, nai, num_dofs, 
                   //                                              area * base_phi[idofs] * query_result.thermal_conductivity *
                   //                                                        (vec_norm[0] * base_dphix[jdofs] + 
                   //                                                         vec_norm[1] * base_dphiy[jdofs] + 
                   //                                                         vec_norm[2] * base_dphiz[jdofs]) / (density * specific_heat));
                 ///////////////////////////////////////////////////////////////////////////////////
	       } // idofs
	       naj += nreq;
	       kk = nreq * num_dofs;
	     } // jdofs    
	  */

	}			//loop over gauss points
      }				//bc normal heat flux   


	  /*! ------------------------------------------------------------------! */
	  /*! ----------------- CONTACT BC	--------------------! */
	  /*! note: it's position is updated elsewhere - in time integration    ! */
		else if ((bc_type_heat == BC_HEAT_CONTACT)) {
	  
	  
		//get bc data
	pdt_heat_bc_contact *bc_contact;

	bc_contact = (pdt_heat_bc_contact *) 
	               pdr_heat_get_bc_data(&problem->bc, fa_bnum);

	apr_set_quadr_2D(fa_type, base, &pdeg, &ngauss, xg, wg);

	
	
    int el_id_bc_connect[6]={0};
    int fa_id_bc_connect=mmr_get_fa_el_bc_connect(Fa_id,el_id_bc_connect);
	double tabww[3],wyniki[6];
	int tabNeig[2],tabNeig1[2];
	int flaga1;
			
	for (ki = 0; ki < ngauss; ++ki) {
	  mmr_fa_elem_coor(mesh_id, &xg[2*ki], face_neig, neig_sides, node_shift,
			   acoeff, bcoeff, loc_xg);
	  iaux = 3 + neig_sides[0];	// boundary data for face neig_sides[0]
	  determ = apr_elem_calc_3D(iaux, nreq, &pdeg, base, loc_xg, node_coor, 
				    sol_dofs_k, base_phi, base_dphix, base_dphiy,
				    base_dphiz, xcoor, u_val, u_x, u_y, u_z, 
				    vec_norm);
	  area = determ * wg[ki];
	  tk = u_val[0];	//most recent temperature at this gauss point

	    double temp_out;
			
	 

		

			flaga1=1;
			tabww[0]=tk;
			tabww[2]= 3 + neig_sides[0];

			
			pdr_heat_get_temperature_at_point(Problem_id,el_id_bc_connect[3],loc_xg,base_phi,NULL,NULL,NULL,u_val,NULL,NULL,tabww);	
			wyniki[0]=tabww[0];wyniki[1]=tabww[1];wyniki[2]=tabww[2];
			

		while(flaga1){
		
		mmr_fa_neig(mesh_id, fa_id_bc_connect, tabNeig, tabNeig1, &node_shift, NULL, acoeff, bcoeff);

		mmr_fa_elem_coor(mesh_id, &xg[2*el_id_bc_connect[ki]], tabNeig, tabNeig1, node_shift,acoeff, bcoeff, loc_xg);	

		
        tabww[2]= 3 + tabNeig1[0];
        
			
			for (i = 0; i < num_shap; ++i) {base_phi_temp[i]=base_phi[i];}
			
			pdr_heat_get_temperature_at_point(Problem_id,el_id_bc_connect[4],loc_xg,base_phi_temp,NULL,NULL,NULL,u_val,NULL,NULL,tabww);
			wyniki[3]=tabww[0];wyniki[4]=tabww[1];wyniki[5]=tabww[2];
			
			
			if( !((fabs(wyniki[0]-wyniki[3])<0.000001) && (fabs(wyniki[1]-wyniki[4])<0.000001) && (fabs(wyniki[2]-wyniki[5])<0.000001) )){
			printf("   wsp1                %lf %lf %lf \n",wyniki[0],wyniki[1],wyniki[2]);
			printf("   wsp2                %lf %lf %lf \n",wyniki[3],wyniki[4],wyniki[5]);
			
			printf("   koniec temp                %d %d %d       %d   %d \n",neig_sides[0],tabNeig1[0],el_id_bc_connect[5],fa_id_bc_connect,ki);
			
			
			el_id_bc_connect[0]=-1;//jako warunek wejściowy do zmiany
			mmr_get_fa_el_bc_connect(Fa_id,el_id_bc_connect);
			}
			else{flaga1=0;}
			
			
			
			temp_out=u_val[0];
		
		}	

		
double flux_coeff,higherTemp;

/*	
double daneT[26]={602.91,602.99,603.08,603.16,605.57,623.91,649.38,669.42,682.43,697.6,718.72,752.99,807.39,831.42,845.86,862.03,879.92,899.45,920.45,942.61,965.51,988.78,1009.84,1025.97,1037.36,1044.61};
double daneA[26]={750.0,800.0,1000.0,1001.8,1082.2,1569.2,1977.9,2129.6,2559.2,3075.6,3559.2,4029.7,4471.0,4523.6,4500.0,4431.0,4307.4,4120.1,3860.0,3517.9,3084.8,2551.5,1908.8,1296.2,1000.0,750.0};
double wartx,alfa1;
int liniowo;

liniowo=1;
wartx=temp_out;

if(tk>temp_out){wartx= tk;}


if(wartx<=daneT[0]){alfa1=daneA[0];liniowo=0;}
if(wartx>=daneT[25]){alfa1=daneA[25];liniowo=0;}


if(liniowo){
		int i=1;
        for(;i<26;++i){
                if(wartx<daneT[i]){
                alfa1=((daneT[i]-wartx)/(daneT[i]-daneT[i-1]))*daneA[i-1]+((wartx-daneT[i-1])/(daneT[i]-daneT[i-1]))*daneA[i]; break;
                }
        }
}

flux_coeff =alfa1;
*/	
	
	higherTemp=temp_out;
	if(tk>temp_out){higherTemp= tk;}    
	
    if(bc_contact->setting_length>1){flux_coeff = pdr_heat_contact_get_alfa_in_the_temp(bc_contact,higherTemp);} //Function takes a higher temperature
	else{flux_coeff = bc_contact->alfa;}
	
	
	
	
	
	
	//printf("   wsp1                %lf %lf \n",wartx,flux_coeff);
		
	  for (jdofs = 0; jdofs < num_shap; jdofs++) {
	    for (idofs = 0; idofs < num_shap; idofs++) {
	      Stiff_mat[jdofs * num_dofs + idofs ] += 
		area * base_phi[idofs] * flux_coeff * base_phi[jdofs];
	    } // idofs
	  } // jdofs    

	  
	  
	  
	  for (i = 0; i < num_shap; ++i) {
	    Rhs_vect[i] += area*base_phi[i]*flux_coeff*(temp_out);
      }			//bc normal heat flux            
    
	}//loop over gauss points
	
		
		
		}
    
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

    }				/* end if computing entries to the stiffness matrix */
  }


  /* end if computing SM and/or RHSV */
  if (Rewr_dofs != NULL)
    *Rewr_dofs = 'F';


  return (1);
}

/*------------------------------------------------------------
pdr_heat_get_temperature_at_point - to provide the temperature and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_heat_weakform.c)
------------------------------------------------------------*/
int pdr_heat_get_temperature_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Temp, // temperature
  double *DTemp_dx, // x-derivative of temperature
  double *DTemp_dy, // y-derivative of temperature
  double *DTemp_dz // z-derivative of temperature
					 )
{
  pdt_heat_problem *problem = 
    (pdt_heat_problem *)pdr_get_problem_structure(Problem_id);

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
  
  if(DTemp_dx==NULL && Base_dphix==NULL){
/*
    // get temperature 
    int iaux = 1; 
	
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,NULL,dofs_loc,
		     Base_phi,NULL,NULL,NULL,
		     NULL,Temp,NULL,NULL,NULL,NULL);
  */  
  
    int iaux = 1;
    if(DTemp_dz!=NULL){
		iaux = (int)DTemp_dz[2];}
	double xcoor[3];

	    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,node_coor,dofs_loc,
		     Base_phi,NULL,NULL,NULL,
		     xcoor,Temp,NULL,NULL,NULL,NULL);
			 
			 DTemp_dz[0]=xcoor[0];
			 DTemp_dz[1]=xcoor[1];
			 DTemp_dz[2]=xcoor[2];
			 
	//printf("wyniki1 %f   %f   %f  \n",xcoor[0],xcoor[1],xcoor[2]);
	
	
  }
  else{

    // get temperature and its derivatives
    int iaux = 2; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,node_coor,dofs_loc,
		     Base_phi,Base_dphix,Base_dphiy,Base_dphiz,
		     NULL, Temp, DTemp_dx, DTemp_dy, DTemp_dz, NULL);
    
  }



  return(0);
}

/*------------------------------------------------------------
pdr_heat_get_temperature_at_point - to provide the eqivalent specific heat and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_heat_weakform.c)
------------------------------------------------------------*/
int pdr_heat_get_dtdt_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Temp, // temperature
  double *Ddtdt_dx, // x-derivative of temperature
  double *Ddtdt_dy, // y-derivative of temperature
  double *Ddtdt_dz // z-derivative of temperature
					 )
{
  pdt_heat_problem *problem = 
    (pdt_heat_problem *)pdr_get_problem_structure(Problem_id);

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
  
  if(Ddtdt_dx==NULL && Base_dphix==NULL){

    // get temperature 
    int iaux = 1; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,NULL,dofs_loc,
		     Base_phi,NULL,NULL,NULL,
		     NULL,Temp,NULL,NULL,NULL,NULL);
    
  }
  else{

    // get temperature and its derivatives
    int iaux = 2; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,node_coor,dofs_loc,
		     Base_phi,Base_dphix,Base_dphiy,Base_dphiz,
		     NULL, Temp, Ddtdt_dx, Ddtdt_dy, Ddtdt_dz, NULL);
    
  }

  return(0);
}


/*------------------------------------------------------------
pdr_heat_compute_CFL - to compute global CFL numbers (for a subdomain)
------------------------------------------------------------*/
int pdr_heat_compute_CFL(
  int Problem_id,
  double *CFL_min_p,
  double *CFL_max_p,
  double *CFL_ave_p
)
{

  double cfl_min=1000000, cfl_max=0, cfl_ave=0.0;
  int nrelem=0;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  pdt_heat_problem *problem = 
    (pdt_heat_problem *)pdr_get_problem_structure(Problem_id);

  int field_id = problem->ctrl.field_id;
  int mesh_id = problem->ctrl.mesh_id;

  double delta_t = problem->time.cur_dtime;

  // loop over elements
  int nel = 0;
  while ((nel = mmr_get_next_act_elem(mesh_id, nel)) != 0) {

  
    // at one point (that exists for all types of elements)
    double x[3];     
    x[0]=0.0; x[1]=0.0; x[2]=0.0; 

    double velocity[3];
    pdr_heat_get_velocity_at_point(Problem_id, nel, x, NULL, NULL, NULL, NULL,
				   velocity, NULL, NULL, NULL);

    double norm_u = 
      sqrt(velocity[0]*velocity[0]+velocity[1]*velocity[1]+velocity[2]*velocity[2]);

    // hsize - standard element size 
    double hsize = mmr_el_hsize(mesh_id,nel,NULL,NULL,NULL);

    double cfl = norm_u * delta_t / hsize;

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


/*------------------------------------------------------------
pdr_heat_get_velocity_at_point - to provide the velocity and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_heat_weakform.c)
------------------------------------------------------------*/
int pdr_heat_get_velocity_at_point(
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

  if(Velocity!=NULL && DVel_dx==NULL){

    // specify velocity vector
    Velocity[0] = 0.0;
    Velocity[1] = 0.0;
    Velocity[2] = 0.0;
    //Velocity[0] = X_loc[0];
    //Velocity[1] = X_loc[1];
    //Velocity[2] = X_loc[2];

  }
  else{

    printf("Derivatives of velocity requested in pdr_heat_get_velocity_at_point. Exiting!\n");
    exit(-1);

  }

  return(0);

}
