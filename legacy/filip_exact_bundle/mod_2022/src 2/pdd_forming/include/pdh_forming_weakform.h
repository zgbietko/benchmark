/************************************************************************
File pdh_forming_weakform.h - weakform functions for formig 

Contains declarations of routines:

MODULE PROVIDES IMPLEMENTATION (in pds_forming_weakform.c) 
PROCEDURES CAN BE CALLED BY ALL OTHER PROBLEM MODULES 

  pdr_forming_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals

  pdr_forming_el_coeff - to return coefficients for element integrals

  pdr_forming_comp_el_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for an element

  pdr_forming_comp_fa_stiff_mat - to construct UNCONSTRAINED stiffness matrix and
                          a load vector for a face

  pdr_forming_compute_CFL - to compute and print local CFL numbers for element

  pdr_forming_get_velocity_at_point - to provide the velocity and its
    gradient at a particular point given its local coordinates within an element

  pdr_forming_get_stress_at_point - to provide the stress and its
    gradient at a particular point given its local coordinates within an element

  pdr_forming_get_strain_rate_at_point - to provide the strain rate and its
    gradient at a particular point given its local coordinates within an element

MODULE ASKS FOR IMPLEMENTATION - it has to be provided by procedures defined
 in weakform directory of the problem module that uses forming as submodule

  pdr_forming_give_me_temperature_at_point - to provide the temperature and its
    gradient at a particular point given its local coordinates within an element

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl) (ns_supg)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
    2015    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl) (forming)
*************************************************************************/

#ifndef PDH_FORMING_WEAKFORM
#define PDH_FORMING_WEAKFORM

#include<stdio.h>

#ifdef __cplusplus
extern "C" 
{
#endif

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**-----------------------------------------------------------
  pdr_forming_select_el_coeff_vect - to select coefficients 
                     returned to approximation routines for element integrals
------------------------------------------------------------*/
int pdr_forming_select_el_coeff_vect( // returns success indicator
  int Problem_id,
  int *Coeff_vect_ind	/* out: coefficient indicator */
				      );

/*!!!!!! OLD OBSOLETE VERSION !!!!!!*/
/**-----------------------------------------------------------
  pdr_forming_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals
------------------------------------------------------------*/
extern double* pdr_forming_select_el_coeff( 
			  /* returns: pointer !=NULL to indicate selection */
  int Problem_id,
  double **Mval,	/* out: mass matrix coefficient */
  double **Axx,double **Axy,double **Axz, /* out:diffusion coefficients, e.g.*/
  double **Ayx,double **Ayy,double **Ayz, /* Axy denotes scalar or matrix */
  double **Azx,double **Azy,double **Azz, /* related to terms with dv/dx*du/dy */
  /* second order derivatives in weak formulation (scalar for scalar problems */
  /* matrix for vector problems) */
  /* WARNING: if axy==NULL only diagonal (axx, ayy, azz) terms are considered */
  /* in apr_num_int_el */
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
  );

/**--------------------------------------------------------
  pdr_forming_el_coeff - to return coefficients for element integrals
----------------------------------------------------------*/
extern int pdr_forming_el_coeff(
  /* GENERIC arguments as in pdr_el_coeff */
  int Problem_id,
  int Elem,	/* in: element number */
  int Mat_num,	/* in: material number */
  double Hsize,	/* in: size of an element */
  int Pdeg,	/* in: local degree of polynomial */
  double *X_loc,      /* in: local coordinates of point within element */
  double *Base_phi,   /* in: basis functions */
  double *Base_dphix, /* in: x-derivatives of basis functions */
  double *Base_dphiy, /* in: y-derivatives of basis functions */
  double *Base_dphiz, /* in: z-derivatives of basis functions */
  double *Xcoor,	/* in: global coordinates of a point */
  double *Uk_val, 	/* in: computed solution from previous iteration */
  double *Uk_x, 	/* in: gradient of computed solution Uk_val */
  double *Uk_y,   	/* in: gradient of computed solution Uk_val */
  double *Uk_z,   	/* in: gradient of computed solution Uk_val */
  double *Un_val, 	/* in: computed solution from previous time step */
  double *Un_x, 	/* in: gradient of computed solution Un_val */
  double *Un_y,   	/* in: gradient of computed solution Un_val */
  double *Un_z,   	/* in: gradient of computed solution Un_val */
  double *Mval,	/* out: mass matrix coefficient */
  double *Axx, double *Axy, double *Axz,  /* out:diffusion coefficients */
  double *Ayx, double *Ayy, double *Ayz,  /* e.g. Axy denotes scalar or matrix */
  double *Azx, double *Azy, double *Azz,  /* related to terms with dv/dx*du/dy */
  /* second order derivatives in weak formulation (scalar for scalar problems */
  /* matrix for vector problems) */
  /* WARNING: if axy==NULL only diagonal (axx, ayy, azz) terms are considered */
  /* in apr_num_int_el */
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
  /* arguments SPECIFIC to forming */
  double Tk, // temperature at the current point */
  double Dynamic_viscosity,
  double Density,
  double Reference_density,
  double Reference_velocity,
  double Delta_t,
  double Implicitness_coeff,
  double Body_force_x,
  double Body_force_y,
  double Body_force_z
);

/**-----------------------------------------------------------
  pdr_forming_comp_el_stiff_mat - to construct a stiffness matrix and
                          a load vector for an element
------------------------------------------------------------*/
int pdr_forming_comp_el_stiff_mat(/*returns: >=0 -success code, <0 -error code */
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
  );

/**-----------------------------------------------------------
  pdr_forming_comp_fa_stiff_mat - to construct a stiffness matrix and
                          a load vector for a face
------------------------------------------------------------*/
int pdr_forming_comp_fa_stiff_mat(/*returns: >=0 -success code, <0 -error code */
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
  );

/**-----------------------------------------------------------
pdr_forming_compute_CFL - to compute global CFL numbers (for a subdomain)
------------------------------------------------------------*/
int pdr_forming_compute_CFL(
  int Problem_id,
  double *CFL_min_p,
  double *CFL_max_p,
  double *CFL_ave_p
);

/**-----------------------------------------------------------
  pdr_forming_get_velocity_at_point - to provide the velocity and its
    gradient at a particular point given its local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_forming_weakform.c)
------------------------------------------------------------*/
extern int pdr_forming_get_velocity_at_point(
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

/**-----------------------------------------------------------
  pdr_forming_get_stress_at_point - to provide the stress tensor and its
    gradient at a particular point given its local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_forming_weakform.c)
------------------------------------------------------------*/
extern int pdr_forming_get_stress_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Stress, // stress vector (tensor)
  double *DStress_dx, // x-derivative of stress vector
  double *DStress_dy, // y-derivative of stress vector
  double *DStress_dz  // z-derivative of stress vector
      );

/**-----------------------------------------------------------
  pdr_forming_get_strain_rate_at_point - to provide the strain rate tensor and its
    gradient at a particular point given its local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_forming_weakform.c)
------------------------------------------------------------*/
extern int pdr_forming_get_strain_rate_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *StrainR, // strain rate vector (tensor)
  double *DStrainR_dx, // x-derivative of strain rate vector
  double *DStrainR_dy, // y-derivative of strain rate vector
  double *DStrainR_dz  // z-derivative of strain rate vector
      );

/**-----------------------------------------------------------
  pdr_forming_give_me_temperature_at_point - to provide the temperature and its
    gradient at a particular point given its local coordinates within an element
FORMING MODULE ASKS FOR IMPLEMENTATION - it has to be provided by procedures
defined in ls_intf directory of the problem module that uses forming as submodule
HERE: forming gives itself trivial solution
------------------------------------------------------------*/
extern int pdr_forming_give_me_temperature_at_point(
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
 );
  

#ifdef __cplusplus
}
#endif

#endif
