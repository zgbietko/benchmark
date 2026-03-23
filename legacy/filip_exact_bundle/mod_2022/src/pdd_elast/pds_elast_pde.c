/************************************************************************
File pds_elast_pde.c - interface providing data on elasticity problem


Contains implementation of routines:
  pdr_read_pde_data - to read coefficients of PDE from file pde.dat
  pdr_get_bc_type - to get BC type given BC flag from mesh data structure
  pdr_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals
  pdr_el_coeff - to return coefficients for element integrals
  pdr_select_fa_coeff - to select coefficients returned to approximation
                        routines for face integrals
  pdr_fa_coeff - to return coefficients for face integrals
  pdr_pde_coeff - to return coefficients of the original pdes
  pdr_bc_diri_coeff - to return DIRICHLET boundary coeficients
  pdr_bc_neum_coeff - to return NEUMANN boundary coeficients
  pdr_bc_mixed_coeff - to return ROBIN (mixed) boundary coeficients
  pdr_get_mat_data - to get material data from material structures
  pdr_renum_coeff - to return a coefficient being a basis for renumbering
  pdr_exact_sol - to return values and derivatives at a point
	          for functions used as exact solutions for test problems

------------------------------
History:
	02.2002 - Krzysztof Banas, initial version
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>

/* problem dependent interface specyfing PDE data */
#include "pdh_intf.h"

/* internal header file of the problem dependent module for Laplace's equation */
#include "./pdh_elast.h"


/* NREQ defined as constant to speed up computations */
#define NREQ PDC_NREQ

///*** utilities ***/


/*---------------------------------------------------------
pdr_ctrl_i_params - to return one of control parameters
---------------------------------------------------------*/
int pdr_ctrl_i_params( /* returns: integer problem parameter */
	int Problem_id,	/* in: problem ID  */
	int Num         /* in: parameter number in control structure */
	)
{

  if(Num==1) return(pdv_problem.name);
  else if(Num==2) return(pdv_problem.mesh_id);
  else if(Num==3) return(pdv_problem.field_id);
  else if(Num==4) return(pdv_problem.nr_sol);
  else if(Num==5) return(pdv_problem.nreq);
  else if(Num==6) return(Problem_id); // solver_id == problem_id
  else {
    printf("Wrong parameter number in ctrl_i_params!");
    exit(1);
  }

/* error condition - that point should not be reached */
  return(-1);
}

/*---------------------------------------------------------
pdr_ctrl_d_params - to return one of control parameters
---------------------------------------------------------*/
double pdr_ctrl_d_params( /* returns: real problem parameter */
	int Problem_id,	/* in: problem ID  */
	int Num         /* in: parameter number in control structure */
	)
{
  return(1);
}

/*---------------------------------------------------------
pdr_adapt_i_params - to return parameters of adaptation
---------------------------------------------------------*/
int pdr_adapt_i_params( /* returns: integer adaptation parameter */
	int Problem_id,	/* in: data structure to be used  */
	int Num         /* in: parameter number in control structure */
	)
{

  if(Num==1) return(pdv_problem.adpt.type);
  else if(Num==2) return(pdv_problem.adpt.interval);
  else if(Num==3) return(pdv_problem.adpt.maxgen);
  else if(Num==4) return(pdv_problem.adpt.maxgendiff);
  else if(Num==7) return(pdv_problem.adpt.monitor);
  else {
    printf("Wrong parameter number in adapt_i_params!");
    exit(1);
  }

/* error condition - that point should not be reached */
  return(-1);
}

/*---------------------------------------------------------
pdr_adapt_d_params - to return parameters of adaptation
---------------------------------------------------------*/
double pdr_adapt_d_params( /* returns: real adaptation parameter */
	int Problem_id,	/* in: data structure to be used  */
	int Num         /* in: parameter number in control structure */
	)
{

  if(Num==5) return(pdv_problem.adpt.eps);
  else if(Num==6) return(pdv_problem.adpt.ratio);
  else {
    printf("Wrong parameter number in adapt_d_params!");
    exit(1);
  }

/* error condition - that point should not be reached */
  return(-1);
}


/*------------------------------------------------------------
  pdr_get_bc_type - to get BC type given BC flag from mesh data structure
                    !!! according to some adopted convention !!!
------------------------------------------------------------*/
int pdr_get_bc_type( /* returns: bc type for a face: */
		/*  	1 (PDC_INTERIOR) - interior face */
		/*  	2 (PDC_BC_DIRI) - Dirichlet boundary face */
		/*  	3 (PDC_BC_NEUM) - Neumann boundary face */
		/*  	4 (PDC_BC_MIXED) - Robin boundary face */
   int Fa_bc	/* in: BC flag (returned by mesh data structure) */
   )
{

/*++++++++++++++++ executable statements ++++++++++++++++*/

  if(Fa_bc<=0) return(PDC_INTERIOR);
  else if(Fa_bc<PDC_MAXBCVAL) return(PDC_BC_DIRI);
  else if(Fa_bc<2*PDC_MAXBCVAL) return(PDC_BC_NEUM);
  else if(Fa_bc<3*PDC_MAXBCVAL) return(PDC_BC_MIXED);
  else {
    printf("Wrong Fa_bc flag %d inpdr_get_bc_type \n", Fa_bc);
    exit(1);
  }

}


/*------------------------------------------------------------
  pdr_select_el_coeff_vect - to select coefficients returned to approximation
                        routines for element integrals in weak formulation
           (the procedure indicates which terms are non-zero in weak form)
------------------------------------------------------------*/
int pdr_select_el_coeff_vect( // returns success indicator
  int Problem_id,
  int *Coeff_vect_ind	/* out: coefficient indicator */
			      )
{
 
  // first indicate the vector has been filled
  Coeff_vect_ind[0] = 1;

  // Mval - mass matrix in time integration
  Coeff_vect_ind[1] = 0;
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
  Coeff_vect_ind[11] = 0;
  // By
  Coeff_vect_ind[12] = 0;
  // Bz
  Coeff_vect_ind[13] = 0;
  // Tx
  Coeff_vect_ind[14] = 0;
  // Ty
  Coeff_vect_ind[15] = 0;
  // Tz
  Coeff_vect_ind[16] = 0;
  // Cval
  Coeff_vect_ind[17] = 0; 
  // Lval - for time integration
  Coeff_vect_ind[18] = 0;
  // Qx
  Coeff_vect_ind[19] = 0;
  // Qy
  Coeff_vect_ind[20] = 0;
  // Qz
  Coeff_vect_ind[21] = 0;
  // Sval
  Coeff_vect_ind[22] = 1;
  
  return(1);
}


/*!!!!!! OLD OBSOLETE VERSION !!!!!!*/
/*------------------------------------------------------------
  pdr_select_el_coeff - to select coefficients returned to approximation
                        routines for element integrals
------------------------------------------------------------*/
double* pdr_select_el_coeff( /* returns: pointer !=NULL to indicate selection */
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
  *Mval = NULL;
  *Axx	= (double *)malloc(sizeof(double));
  *Axy	= (double *)malloc(sizeof(double));
  *Axz  = (double *)malloc(sizeof(double));
  *Ayx  = (double *)malloc(sizeof(double));
  *Ayy  = (double *)malloc(sizeof(double));
  *Ayz  = (double *)malloc(sizeof(double));
  *Azx  = (double *)malloc(sizeof(double));
  *Azy  = (double *)malloc(sizeof(double));
  *Azz  = (double *)malloc(sizeof(double));
  *Bx   = NULL;
  *By   = NULL;
  *Bz   = NULL;
  *Cval = NULL;
  *Lval = NULL;
  *Qx   = NULL;
  *Qy   = NULL;
  *Qz   = NULL;
  *Sval = (double *)malloc(sizeof(double));

  return(*Axx);
}

/*------------------------------------------------------------
  pdr_select_fa_coeff - to select coefficients returned to approximation
                        routines for face integrals
------------------------------------------------------------*/
double* pdr_select_fa_coeff( /* returns: pointer !=NULL to indicate selection */
  double** Anx,  /* out: diffusion coefficients */
  double** Any,
  double** Anz,
  double** Bn,	 /* out: convection coefficients */
  double** Qn,	 /* out: rhs coefficients with derivatives */
  double** Fval,	/* out: rhs  Dirichlet data f */
  double** Gval,	/* out: rhs  Neumann data g */
  double** Vel_norm /* out: velocity normal to the boundary */
  )
{
  *Anx	= (double *)malloc(sizeof(double));
  *Any	= (double *)malloc(sizeof(double));
  *Anz  = (double *)malloc(sizeof(double));
  *Bn   = NULL;
  *Qn   = NULL;
  *Fval = (double *)malloc(sizeof(double));
  *Gval = (double *)malloc(sizeof(double));
  *Vel_norm = NULL;

  return(*Anx);
}

/*---------------------------------------------------------
pdr_el_coeff - to return coefficients for internal integrals
	in DGFEM for convection-diffusion-reaction-etc equations
----------------------------------------------------------*/
int pdr_el_coeff(
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
  double *Sval	/* out: rhs coefficients without derivatives (source terms) */
	)
{

  double time=0.0;

/*++++++++++++++++ executable statements ++++++++++++++++*/


/* when not using any additional modifications (e.g. artificial viscosity) */
/* get directly coefficients of the corresponding pde */
  //FK wyłączam
  /*
  pdr_pde_coeff(Mat_num,Xcoor,time,U_val,U_x,U_y,U_z,
			Axx,Axy,Axz,Ayx,Ayy,Ayz,Azx,Azy,Azz,
			Bx,By,Bz,Cval,Mval,Lval,Sval,Qx,Qy,Qz);
	*/

  return(0);
}

/*---------------------------------------------------------
pdr_fa_coeff - to return coefficients for face integrals
	in DGFEM for convection-diffusion-reaction-etc equations
----------------------------------------------------------*/
void pdr_fa_coeff(
	int Face,	/* in: face number */
        int BC_flag,	/* in: boundary condition flag */
	int Elem,	/* in: element number */
	int Mat_num,	/* in: material number */
	double Hsize,	/* in: size of an element */
	double Time,    /* in: time instant */
	int Pdeg,	/* in: local degree of polynomial */
	double* Xcoor,	/* in: global coordinates of a point */
	double* Vec_norm, /* in: unit normal to the boundary */
	double* U_val, 	/* in: computed solution */
	double* U_x, 	/* in: gradient of computed solution */
	double* U_y,   	/* in: gradient of computed solution */
	double* U_z,   	/* in: gradient of computed solution */
	double* Anx,	/* out: diffusion coefficients */
	double* Any,
	double* Anz,
	double* Bn,	/* out: convection coefficients */
	double* Fval,	/* out: rhs  Dirichlet data f */
	double* Gval,	/* out: rhs  Neumann data g */
	double* Qn,	/* out: rhs normal fluxes */
	double* Vel_norm /* out: velocity normal to the boundary */
	)
{

  int i,iaux, ieq1, ieq2;
  double x, y, z, f, f_x, f_y, f_z, daux;
  double axx[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double axy[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double axz[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double ayx[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double ayy[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double ayz[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double azx[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double azy[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double azz[PDC_NREQ*PDC_NREQ];  /* coeff of PDE */
  double bx[PDC_NREQ*PDC_NREQ];   /* coeff of PDE */
  double by[PDC_NREQ*PDC_NREQ];   /* coeff of PDE */
  double bz[PDC_NREQ*PDC_NREQ];   /* coeff of PDE */
  double cval[PDC_NREQ*PDC_NREQ]; /* coeff of PDE */
  double mval[PDC_NREQ];            /* coeff of PDE */
  double lval[PDC_NREQ];           /* coeff of PDE */
  double sval[PDC_NREQ];            /* coeff of PDE */
  double qx[PDC_NREQ];              /* coeff of PDE */
  double qy[PDC_NREQ];              /* coeff of PDE */
  double qz[PDC_NREQ];              /* coeff of PDE */

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* get coefficients of the corresponding pde */
  pdr_pde_coeff(Mat_num,Xcoor,Time,U_val,U_x,U_y,U_z,
			axx,axy,axz,ayx,ayy,ayz,azx,azy,azz,
			bx,by,bz,cval,mval,lval,sval,qx,qy,qz);

/* initialize coefficients when necessary */
  utr_d_zero(Qn,NREQ);

/* internal faces */
  if(BC_flag==PDC_INTERIOR){

/* compute normal fluxes */
    for(ieq1=0;ieq1<NREQ;ieq1++){
      for(ieq2=0;ieq2<NREQ;ieq2++){
	Anx[ieq1*NREQ+ieq2] =   axx[ieq1*NREQ+ieq2] * Vec_norm[0] +
	                        ayx[ieq1*NREQ+ieq2] * Vec_norm[1] +
	                        azx[ieq1*NREQ+ieq2] * Vec_norm[2] ;
	Any[ieq1*NREQ+ieq2] =   axy[ieq1*NREQ+ieq2] * Vec_norm[0] +
	                        ayy[ieq1*NREQ+ieq2] * Vec_norm[1] +
	                        azy[ieq1*NREQ+ieq2] * Vec_norm[2] ;
	Anz[ieq1*NREQ+ieq2] =   axz[ieq1*NREQ+ieq2] * Vec_norm[0] +
	                        ayz[ieq1*NREQ+ieq2] * Vec_norm[1] +
	                        azz[ieq1*NREQ+ieq2] * Vec_norm[2] ;
      }
    }
  }
  else if(BC_flag==PDC_BC_DIRI){
/* Dirichlet faces */

/* compute normal fluxes */
    for(ieq1=0;ieq1<NREQ;ieq1++){
      for(ieq2=0;ieq2<NREQ;ieq2++){
	Anx[ieq1*NREQ+ieq2] =   axx[ieq1*NREQ+ieq2] * Vec_norm[0] +
	                        ayx[ieq1*NREQ+ieq2] * Vec_norm[1] +
	                        azx[ieq1*NREQ+ieq2] * Vec_norm[2] ;
	Any[ieq1*NREQ+ieq2] =   axy[ieq1*NREQ+ieq2] * Vec_norm[0] +
	                        ayy[ieq1*NREQ+ieq2] * Vec_norm[1] +
	                        azy[ieq1*NREQ+ieq2] * Vec_norm[2] ;
	Anz[ieq1*NREQ+ieq2] =   axz[ieq1*NREQ+ieq2] * Vec_norm[0] +
	                        ayz[ieq1*NREQ+ieq2] * Vec_norm[1] +
	                        azz[ieq1*NREQ+ieq2] * Vec_norm[2] ;
      }
    }

/* get boundary coefficient of the corresponding pde */
    pdr_bc_diri_coeff(Face,Mat_num,Xcoor,Vec_norm,Time,
		       U_val,U_x,U_y,U_z,
		       Fval);

  }
  else if(BC_flag==PDC_BC_NEUM){
/* Neumann faces */

/* get boundary coefficient of the corresponding pde */
    pdr_bc_neum_coeff(Face,Mat_num,Xcoor,Vec_norm,Time,
		       U_val,U_x,U_y,U_z,
		       Gval);

  }
  else if(BC_flag==PDC_BC_MIXED){
/* Robin faces */

/* get boundary coefficient of the corresponding pde */
    pdr_bc_mixed_coeff(Face,Mat_num,Xcoor,Vec_norm,Time,
		       U_val,U_x,U_y,U_z,
		       Fval, Vel_norm);

  }

  return;
}

/*---------------------------------------------------------
pdr_pde_coeff - to return coefficients for internal integrals
	in DGFEM for convection-diffusion-reaction-etc equations
----------------------------------------------------------*/
void pdr_pde_coeff(
	int Mat_num,	/* in: material number */
	double* Xcoor,	/* in: global coordinates of a point */
	double  Time,	/* in: time instant */
	double* U_val, 	/* in: computed solution */
	double* U_x, 	/* in: gradient of computed solution */
	double* U_y,   	/* in: gradient of computed solution */
	double* U_z,   	/* in: gradient of computed solution */
	double* Axx, 	/* out: diffusion coefficients */
	double* Axy,
	double* Axz,
	double* Ayx,
	double* Ayy,
	double* Ayz,
	double* Azx,
	double* Azy,
	double* Azz,
	double* Bx,	/* out: convection coefficients */
	double* By,
	double* Bz,
	double* Cval,	/* out: reaction coefficients */
	double* Mval,	/* out: mass matrix coefficients */
	double* Lval,	/* out: rhs time coefficients */
	double* Sval,	/* out: rhs coefficients without derivatives */
	double* Qx,	/* out: rhs coefficients with derivatives */
	double* Qy,	/* out: rhs coefficients with derivatives */
	double* Qz	/* out: rhs coefficients with derivatives */
	)
{

  int i, iaux;
  double x,y,z,Eyoung,Poisni,g,rl,gprl;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  pdr_get_mat_data(Mat_num, &Eyoung, &Poisni);

  /*stale Lame'go */
  g=Eyoung/(2*(1+Poisni));
  rl=Poisni*Eyoung/((1-2*Poisni)*(1+Poisni));
  gprl=g+rl;

/* rewrite and check parameters*/
  x = Xcoor[0];
  y = Xcoor[1];
  z = Xcoor[2];

/* initialize coefficients */
  utr_d_zero(Axx,NREQ*NREQ);
  utr_d_zero(Axy,NREQ*NREQ);
  utr_d_zero(Axz,NREQ*NREQ);
  utr_d_zero(Ayx,NREQ*NREQ);
  utr_d_zero(Ayy,NREQ*NREQ);
  utr_d_zero(Ayz,NREQ*NREQ);
  utr_d_zero(Azx,NREQ*NREQ);
  utr_d_zero(Azy,NREQ*NREQ);
  utr_d_zero(Azz,NREQ*NREQ);
  utr_d_zero(Sval,NREQ);


/* coefficients for Hooke's material */
  Axx[0]=gprl;Axy[1]=rl;Axz[2]=rl;Axy[3]=g;Axz[6]=g;
  Ayx[1]=g;Ayx[3]=rl;Ayy[4]=gprl;Ayz[5]=rl;Ayz[7]=g;
  Azx[2]=g;Azy[5]=g;Azx[6]=rl;Azy[7]=rl;Azz[8]=gprl;


  /*right hand side for body forces */
  Sval[0]= 0;

return;
}

/*---------------------------------------------------------
pdr_bc_diri_coeff - to return DIRICHLET boundary coeficients
                     (routine substitutes only non-zero values)
----------------------------------------------------------*/
void pdr_bc_diri_coeff(
	int Face,       /* in: index in an array of BC coefficients */
	int Mat_num,	/* in: material number */
	double* Xcoor,	/* in: global coordinates of a point */
	double* Vec_norm,/* in: unit normal to the boundary */
	double  Time,	/* in: time instant */
	double* U_val, 	/* in: computed solution */
	double* U_x, 	/* in: gradient of computed solution */
	double* U_y,   	/* in: gradient of computed solution */
	double* U_z,   	/* in: gradient of computed solution */
	double* Fval	/* out: BC DIRICHLET coefficients */
	)
{

  int i, bc_num;
  double x, y, z, f, f_x, f_y, f_z, daux;

/*++++++++++++++++ executable statements ++++++++++++++++*/


/* rewrite and check parameters*/
  x = Xcoor[0];
  y = Xcoor[1];
  z = Xcoor[2];

/* initialize coefficients */
  utr_d_zero(Fval,NREQ);


  Fval[1] = 1.0;

  return;
}

/*---------------------------------------------------------
pdr_bc_neum_coeff - to return NEUMANN boundary coeficients
                     (routine substitutes only non-zero values)
----------------------------------------------------------*/
void pdr_bc_neum_coeff(
	int Face,	/* in: face number */
	int Mat_num,	/* in: material number */
	double* Xcoor,	/* in: global coordinates of a point */
	double* Vec_norm,/* in: unit normal to the boundary */
	double  Time,	/* in: time instant */
	double* U_val, 	/* in: computed solution */
	double* U_x, 	/* in: gradient of computed solution */
	double* U_y,   	/* in: gradient of computed solution */
	double* U_z,   	/* in: gradient of computed solution */
	double* Gval	/* out: BC NEUMANN coefficients */
	)
{

  int i, iaux;
  double x, y, z, f, f_x, f_y, f_z, daux;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* rewrite and check parameters*/
  x = Xcoor[0];
  y = Xcoor[1];
  z = Xcoor[2];

/* initialize coefficients */
  utr_d_zero(Gval,NREQ);

  return;
}


/*---------------------------------------------------------
pdr_bc_mixed_coeff - to return ROBIN (mixed) boundary coeficients
----------------------------------------------------------*/
void pdr_bc_mixed_coeff(
	int Face,	/* in: face number */
	int Mat_num,	/* in: material number */
	double* Xcoor,	/* in: global coordinates of a point */
	double* Vec_norm,/* in: unit normal to the boundary */
	double  Time,	/* in: time instant */
	double* U_val, 	/* in: computed solution */
	double* U_x, 	/* in: gradient of computed solution */
	double* U_y,   	/* in: gradient of computed solution */
	double* U_z,   	/* in: gradient of computed solution */
	double* Fval,	/* out: BC mixed coefficients */
	double* Kr	/* out: proportionality coefficient */
	)
{

  int i;
  double x,y,z,daux;
/*++++++++++++++++ executable statements ++++++++++++++++*/

/* rewrite and check parameters*/
  x = Xcoor[0];
  y = Xcoor[1];
  z = Xcoor[2];

  return;
}

/*---------------------------------------------------------
pdr_get_mat_data - to get material data from material structures
----------------------------------------------------------*/
int pdr_get_mat_data(
        int Mat_num,   /* in: material number */
	double* Eyoung,  /* out: Young modulus */
	double* Poisni /*Poisson constant */
	)
{

/* auxiliary variables */
  int i, imat, nr_mat;
  pdt_material *material; /* pointer to material data */

/*++++++++++++++++ executable statements ++++++++++++++++*/

  nr_mat = 1;

/* get problem identification */
  if(Mat_num>0 && Mat_num<=nr_mat) {
    *Eyoung=material[Mat_num-1].Eyoung;
    *Poisni=material[Mat_num-1].Poisni;
  }
  else {
    printf("Unknown material number %d\n",Mat_num);
    return(-1);
  }

  return(1);
}

/*---------------------------------------------------------
pdr_exact_sol - to return values and derivatives at a point
	for functions used as exact solutions for test problems
----------------------------------------------------------*/
int pdr_exact_sol(	/* returns: 1-found exact solution, 0-not found */
	int Mat_num,    /* in: material number (coefficients of pde) */
	double X,	/* in: coordinates of a point */
	double Y,
	double Z,
	double Time,	/* in: time instant */
	double *Exact,	/* out: values of solution and derivatives */
	double *Exact_x,
	double *Exact_y,
	double *Exact_z
	)
{

/* auxiliary variables */
  int i;
  double a1,daux,cd1d_coeff,x,y,z;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  x=X; y=Y; z=Z;


/* u = exp(-(x^2+y^2)/a1)
a1=1.0;
*Exact   =  exp((-x*x-y*y)/a1);
*Exact_x = -2.0*x*exp((-x*x-y*y)/a1)/a1;
*Exact_y = -2.0*y*exp((-x*x-y*y)/a1)/a1;
*Exact_z = 0.0;
*Lapl    = -(2.0/a1)*((1-2*x*x/a1)+(1-2*y*y/a1))
			*exp((-x*x-y*y)/a1);
/**/

/* u = exp(-(x^2+y^2+z^2)/a1) */
/*     a1=1.0; */
/*     *Exact   =  exp((-x*x-y*y-z*z)/a1); */
/*     *Exact_x = -2.0*x*exp((-x*x-y*y-z*z)/a1)/a1; */
/*     *Exact_y = -2.0*y*exp((-x*x-y*y-z*z)/a1)/a1; */
/*     *Exact_z = -2.0*z*exp((-x*x-y*y-z*z)/a1)/a1; */
/*     *Lapl    = -(2.0/a1)*((1-2*x*x/a1)+(1-2*y*y/a1)+(1-2*z*z/a1)) */
/* 			*exp((-x*x-y*y-z*z)/a1); */


/* u = -(x^2+y^2+z^2)
*Exact   =  -x*x-y*y-z*z;
*Exact_x = -2.0*x;
*Exact_y = -2.0*y;
*Exact_z = -2.0*z;
*Lapl    = -6.0;
/**/


  return(0);
}
