/*************************************************************
File contains procedures:

it_gmres	- to solve a system of linear equations Ax = b
	using the left preconditioned GMRES method as defined in 
	"Templates for the Solution of Linear Systems: Building 
	Blocks for Iterative Methods", Barrett, Berry, Chan, 
	Demmel, Donato, Dongarra, Eijkhout, Pozo, Romine, 
	and van der Vorst, SIAM Publications, 1994 

The version below is a general purpose procedure, independent 
of the problem solved and the structure of the system matrix.
Neither the system matrix A, nor the preconditioner M, nor the
RHS vector b do not appear in it_gmres.


Required routines:

it_compreres - to compute the residual of the left preconditioned 
	system of equations, V = M^-1 * ( B - A*X ) (Control=1),
	used also to perform preconditioned matrix-vector 
	multiplication, V = M^-1 * A * X  (Control=0)

it_par_vec_norm - to perform parallel vector norm
it_par_sc_prod - to perform parallel scalar product


BLAS: drot, drotg, ddot, dnrm2, dscal, daxpy, dgemv, dtrsv

History:
	05.2001 - Krzysztof Banas, initial version		

*************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>

#include "../include/it_mkb.h"

/* LAPACK and BLAS procedures */
#include "lin_alg_intf.h"

/*---------------------------------------------------------
it_gmres - basic restarted GMRES solver procedure
---------------------------------------------------------*/
int it_gmres(	/* returns: convergence indicator: */
			/* 1 - convergence */
			/* 0 - no convergence */
	int Solver_id,  /* in: solver data structure indicator to be passed
	                       to data structure dependent routines */
	int ndof, 	/* in: 	the number of degrees of freedom */
	int ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* x, 	/* in: 	the initial guess */
			/* out:	the iterated solution */
        double* b,	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	int krylbas, 	/* in: 	number of Krylov space vectors */
	int*  iter, 	/* in:	the maximum GMRES iterations to be performed */
			/* out:	actual number of iterations performed */
	double* resid, 	/* in:	tolerance level for residual */
			/* out:	the final value of residual */
	double* res_rel, /* in:	tolerance level for relative residual */
			/* out:	the final value of relative residual */
	double* res_rhs, /* in:	tolerance level for ratio rhs/residual */
			/* out:	the final value of the ratio rhs/residual */
	int monitor,	/* in:	flag to determine monitoring level */
			/*	0 - silent run, 1 - warning messages */
			/*	2 - 1+restart data 3 - iteration data */
	double* pconvr	/* out: pointer to convergence rate */
	)
{


/* local variables (for meaning of symbols see "Templates..." */
  double* v; 	/* workspace for Krylov space basis vectors v */
  double* h; 	/* upper Hessenberg matrix h */ 
		/* (first column for vectors s and y) */
  double* giv;	/* stored Givens rotations */
  double convrate;/* convergence rate */
  double rnrm2;	/* L2 norm of the residual vector */
  double bnrm2;	/* L2 norm of the right hand side vector (or just 1.0) */
  double beta0;	/* L2 norm of the initial residual vector */
  double beta1; 	/* to compute convergence in last restart */
  double tol_res,tol_rel,tol_rhs;	/* tolerances for error measures */
  int restrt; 	/* number of restarts */
  int restit = 0;	/* counter for restarts */
  int gmiter = 0;	/* counter for GMRES iterations */
  int ikryl = 0;	/* counter for Krylov space base vectors */

/* auxiliary variables */
  int i,k,iaux,jaux;
  double daux,eaux,faux;

/* constants */
  int IONE = 1;
  int IZERO = 0;
  double DONE = 1.0;
  const double MAX_CONVRATE = 0.99;
  const double SMALL = 1.0e-15;


/*++++++++++++++++ executable statements ++++++++++++++++*/

/* rewrite input parameters */
  tol_res = *resid;
  tol_rel = *res_rel; if(tol_rel<SMALL) tol_rel=SMALL;
  tol_rhs = *res_rhs; 
  if(tol_rhs > 0) {
    printf("RHS based convergence indicator not yet supported in parallel versionof solver !\n");
    exit(-1);
  }

/* write parameters */
  if(monitor>=2){
    printf("\nGMRES: entered with tolerances for: absolute norm of residual %14.12f\n",
	   tol_res);
    printf("       (relative to RHS %14.12f, relative to initial %14.12f)\n",
	   tol_rhs,tol_rel);
  }

/* allocate and initialize arrays */
  v = it_dvector(ndof*(krylbas+1),"v in gmres");
  h = it_dvector((krylbas+1)*(krylbas+1),"h in gmres");
  giv = it_dvector(2*(krylbas+1),"giv in gmres");

/* compute number of restarts */
  restrt = (*iter) / krylbas + 1 ;

/* restarts' loop */
  do{

/* compute initial preconditioned(!) residual (iaux=1) */
    iaux=1; /* take into account right hand side vector */
    if(gmiter==0) jaux=ini_zero; /* for first restart take input vector x */
    else jaux=0; /* for subsequent restarts assume initial guess is not zero */
    it_compreres(Solver_id,iaux,jaux,ndof,x,b,v);
    /*||begin||*/
    rnrm2 = it_par_vec_norm(ndof, v);
    /*||end||*/
/*kbw
	printf("rnrm2 = %lf\n",rnrm2);
/*kew*/
    //    rnrm2 = dnrm2_(&ndof, v, &ione);

    bnrm2=0.0;
    if(ini_zero&&gmiter==0) {
      bnrm2 = rnrm2;
    }
    if (bnrm2 < SMALL*SMALL) {
      bnrm2 = 1.;
    }

/* compute and print convergence rate for the restart */
    if(restit==0) {
      convrate = 0.0;
      faux = 0.0;
      beta0 = rnrm2;
      beta1 = beta0;
    }
    else  if(rnrm2 < SMALL*SMALL || gmiter==0 ||
             beta0 < SMALL*SMALL || beta1 < SMALL*SMALL  ) {
      convrate = 0.0;
      faux = 0.0;
      beta1 = 0.0;
    }
    else{
      convrate = exp(log(rnrm2/beta0)/(gmiter));
      faux = exp(log(rnrm2/beta1)/(krylbas));
      beta1 = rnrm2;
    }
    if(monitor>=2){
      if(restit==0){
	printf("\nGMRES: norm of initial residual %10.8f (relative to RHS %10.8f)\n",
	       rnrm2,rnrm2/bnrm2);
	printf("       norm of RHS vector %14.12f\n",
	       bnrm2);
      }
      else{
	printf("\nGMRES: after restart %d (%d vectors), norm of residual %14.12f\n",
	       restit,krylbas,rnrm2);
	printf("       (relative to RHS %14.12f, relative to initial %14.12f)\n",
	       rnrm2/bnrm2,rnrm2/beta0);
	printf("       total convergence rate %14.12f, in last restart %14.12f\n",
	       convrate,faux);
      }
    }

/* check for convergence*/
    *resid = rnrm2; 
    if(beta0!=0.0) *res_rel = rnrm2/beta0; else *res_rel = 0;
    if(bnrm2!=0.0) *res_rhs = rnrm2/bnrm2; else *res_rhs = 0;
    if ( rnrm2 < tol_res  ||  *res_rel < tol_rel
	 ||  *res_rhs < tol_rhs || rnrm2 < SMALL*SMALL )  goto convergence;
    if( convrate > MAX_CONVRATE || faux > MAX_CONVRATE){
      printf("\nConvergence rate above %f - abandoning GMRES\n",MAX_CONVRATE);
      goto noconvergence;
    }

/* construct the first Krylov space basis vector (scaled initial residual) */
    daux = 1. / rnrm2;
    dscal_(&ndof, &daux, &v[0], &IONE);

/* initialize the elementary vector e1 scaled by norm of residual */
/* and store it in the first column (index 0) of h */
    h[0] = rnrm2;
    for(i=1;i<=krylbas+1;i++) h[i] = 0;

/* Krylov space basis vectors' loop - GMRES iterations - Arnoldi process */
    for(ikryl=1;ikryl<=krylbas;ikryl++){

      gmiter++;

/* compute preconditioned matrix-vector product M^-1 * A * v_i */
      iaux=0; /* do not take into account right hand side vector */
      jaux=0; /* assume initial guess is not zero */
      it_compreres(Solver_id,iaux,jaux,ndof,&v[(ikryl-1)*ndof],b,
		   &v[ikryl*ndof]);

/* create a column of matrix h */
      k=(krylbas+1)*ikryl;
      for(i=1;i<=ikryl;i++){
	/*||begin||*/
	h[k] = it_par_sc_prod(ndof, &v[(i-1)*ndof], &v[ikryl*ndof]);
/*kbw
	printf("h[%d] = %lf\n",k,h[k]);
/*kew*/

	/*||end||*/
	// h[k] = ddot_(&ndof, &v[(i-1)*ndof], &IONE, &v[ikryl*ndof], &IONE);
	k++;
      }

/* subtract the last column of v from previous columns */
      k=(krylbas+1)*ikryl;
      for(i=1;i<=ikryl;i++){
	daux = -h[k];
	daxpy_(&ndof, &daux, &v[(i-1)*ndof], &IONE, &v[ikryl*ndof], &IONE);
	k++;
      }

      k=(krylbas+1)*ikryl+ikryl;
      /*||begin||*/
      h[k] = it_par_vec_norm(ndof, &v[ikryl*ndof]);
/*kbw
	printf("h[%d] = %lf\n",k,h[k]);
/*kew*/
      /*||end||*/
      // h[k] = dnrm2_(&ndof, &v[ikryl*ndof], &IONE);

/* normalize kolumn of v to get next Krylov space basis vector */
      daux = 1. / h[k];
      dscal_(&ndof, &daux, &v[ikryl*ndof], &IONE);

/* apply stored Givens rotations to the i-th column of h */
      for(i=1;i<ikryl;i++){
	drot_(&IONE, &h[i-1+ikryl*(krylbas+1)], &IONE, 
	      &h[i+ikryl*(krylbas+1)], &IONE, 
	      &giv[2*(i-1)], &giv[2*(i-1)+1]);
      }

/* construct the i-th rotation matrix and apply it to h */
/* so that h(i+1,i) = 0. */
      daux=h[ikryl-1+ikryl*(krylbas+1)];
      eaux=h[ikryl+ikryl*(krylbas+1)];
      drotg_(&daux, &eaux, 
	     &giv[2*(ikryl-1)], &giv[2*(ikryl-1)+1]);
      
      drot_(&IONE,&h[ikryl-1+ikryl*(krylbas+1)], &IONE,
	    &h[ikryl+ikryl*(krylbas+1)], &IONE,
	    &giv[2*(ikryl-1)], &giv[2*(ikryl-1)+1]);

/* apply the i-th rotation matrix to [ s(i), s(i+1) ]' */
/* this gives an approximation of the residual norm */
      drot_(&IONE, &h[ikryl-1], &IONE, &h[ikryl], 
	    &IONE, &giv[2*(ikryl-1)], &giv[2*(ikryl-1)+1]);

/* compute convergence indicator and convergence rate */
      daux = *resid;
      *resid = fabs(h[ikryl]); 
      if(beta0!=0.0) *res_rel = *resid/beta0; else *res_rel = 0;
      if(bnrm2!=0.0) *res_rhs = *resid/bnrm2; else *res_rhs = 0;
      if(*res_rel<SMALL*SMALL){
	convrate=0.0;
      }
      else{
	convrate=exp(log(*res_rel)/(gmiter));
      }

/* monitor results */
      if(monitor>=1&&(*resid)>daux){
	printf("\nGMRES warning: diverging residuals\n");
      }
      if(monitor>=3){
	printf("\nGMRES: iteration %d, norm of residual %14.12f\n",
	       gmiter,*resid);
	printf("       (relative to RHS %14.12f, relative to initial %14.12f)\n",
	       *res_rhs,*res_rel);
	printf("       convergence rate: total %14.12f, in last iteration %14.12f\n",
	       convrate, *resid/daux);
      }

/* if conditions are met to stop Arnoldi process */
      if ( ikryl==krylbas || gmiter== *iter || *resid < tol_res  ||  
	   *res_rel < tol_rel ||  *res_rhs < tol_rhs ) {

/* compute the vector of coefficients y (stored in first column of h) */
	iaux=krylbas+1;
	dtrsv_("U", "N", "N", &ikryl, &h[krylbas+1], &iaux, &h[0], &IONE);

/* monitor results */
	if(monitor>=4){
	  printf("GMRES: computed vector y:\n");
	  for(i=0;i<krylbas+1;i++) printf("%e  ",h[i]);
	  printf("\n");
	}

/* compute current solution vector x */
	dgemv_("N", &ndof, &ikryl, &DONE, &v[0], &ndof, &h[0], &IONE, 
	       &DONE, &x[0], &IONE);   

/* monitor results */
	if ( gmiter== *iter || *resid < tol_res  ||  
	     *res_rel < tol_rel ||  *res_rhs < tol_rhs ) {
	  if(monitor>=2){
	    faux = exp(log(*resid/beta1)/(gmiter-restit*krylbas));
	    printf("\nGMRES: after restart %d (%d vectors), norm of residual %14.12f\n",
		   restit+1,gmiter-restit*krylbas,*resid);
	    printf("       (relative to RHS %14.12f, relative to initial %14.12f)\n",
		   *res_rhs,*res_rel);
	    printf("       convergence rate: total %14.12f, in last restart %14.12f\n",
		   convrate,faux);
	  }
	}

	if ( *resid < tol_res  || *res_rel < tol_rel 
	     || *res_rhs < tol_rhs ) goto convergence;
	else if(gmiter== *iter) goto noconvergence;
	else break;

      }
/* ^the end of breaking procedures */

    }
/* ^the end of Arnoldi process */

    restit++;

  }while(restit<=restrt);
/* ^the end of restarts' loop */

  printf("Hi, you should not be here at all...\n");

convergence:{
    if(monitor>=2){
      printf("\nGMRES convergence achieved in %d iterations\n\n",gmiter);
    }

    *iter=gmiter;
    *pconvr=convrate;

    free(v);
    free(h);
    free(giv);

    return(IONE);
  }

noconvergence:{
    if(monitor>=2){
      printf("\nNo GMRES convergence after %d iterations\n\n",gmiter);
    }

    *iter=gmiter;
    *pconvr=convrate;

    free(v);
    free(h);
    free(giv);

    return(IZERO);
  }

}
