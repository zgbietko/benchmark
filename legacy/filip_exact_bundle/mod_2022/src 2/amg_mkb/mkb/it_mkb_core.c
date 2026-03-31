/*************************************************************
File contains procedures:
it_comp_norm_rhs - to compute the norm of the preconditioned rhs 
                   v = || M^-1 *  b ||

it_standard - to solve the problem by standard iterations
              x_out ~= A^-1 * b, || x^k - x^k-1 ||_max < Toler

it_vcycle - to perform one V-cycle of multigrid (as multi-level
            smoother): x_out = x_in + M^-1 * ( b - A * x_in )

it_smooth - to perform one smoothing step using different algorithms
            x_out = x_in + M^-1 * ( b - A * x_in )

it_precon - to perform preconditioning using different algorithms
            x_out = M^-1 * b 

it_compreres - to compute the residual of the left preconditioned 
	system of equations, v = M^-1 * ( b - Ax )
        ( used also to compute the product v = M^-1 * Ax)

it_solve_coarse - to launch solver for the coarse grid problem

Required routines:
	it_gmres - gmres core procedure
	BLAS: dnrm2, daxpy, dcopy, dgemv, dscal,  
              dgetrf, dgetrs, dgetri, dgemm

History:
        05.2001 - Krzysztof Banas, initial version

*************************************************************/
#include<stdlib.h>
#include<stdio.h>
#include<math.h>

#include "../include/it_mkb.h"
#include "../include/lah_block.h"

#include "lin_alg_intf.h"

/* minimal number of dofs to make use of DGEMV reasonable */
#define MIN_DOF_DGEMV 10000
#define SMALL 1e-15 /* approximation of round-off error */
#define TOL 1e-9    /* default tolerance for different iterative processes */


/*----------------------------------------------------------------
it_comp_norm_rhs - to compute the norm of the preconditioned rhs 
                   v = || M^-1 *  b ||
----------------------------------------------------------------*/
double it_comp_norm_rhs ( /* return: the norm of the rhs vector */
	int Solver_id      /* in: solver data structure to be used */
	)
{

/* constants */
  int ione = 1;

/* local variables */
  itt_levels* it_level;  /* pointer to current level data structure */

/* auxiliary variables */
  int i, iaux, jaux, ndof, ilev;
  double daux, *vtemp, *vtemp1;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* take proper solver data structure */
  ilev = itv_solver[Solver_id].cur_level;
  it_level = &itv_solver[Solver_id].level[ilev];
  ndof = it_level->Nrdofgl;
  vtemp = it_dvector(ndof,"temporary vector in comp_norm_rhs");
  vtemp1 = it_dvector(ndof,"temporary vector in comp_norm_rhs");
  it_d_zero(vtemp,ndof);

/* just compute initial preconditioned(!) residual with zero initial guess */
  iaux=1; /* take into account right hand side vector */
  jaux=1; /* set initial guess to zero */
  it_compreres(Solver_id,iaux,jaux,ndof,vtemp,NULL,vtemp1);
  daux = dnrm2_(&ndof, vtemp1, &ione);


  free(vtemp);
  free(vtemp1);

  return(daux);
}

/*---------------------------------------------------------
it_standard - to solve the problem by standard iterations
              x_out ~= A^-1 * b, || x^k - x^k-1 ||_max < Toler
---------------------------------------------------------*/
int it_standard( /* returns: convergence indicator: */
			/* 1 - convergence */
			/* 0 - noconvergence */
	int Solver_id,      /* in: solver data structure to be used */
	int Ndof, 	/* in: 	the number of degrees of freedom */
	int Ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* X, 	/* in: 	the initial guess */
			/* out:	the iterated solution */
        double* B,	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	int*  Iter, 	/* in:	the maximum iterations to be performed */
			/* out:	actual number of iterations performed */
	double* Toler, 	/* in:	tolerance level for max norm of update */
			/* out:	the final value of max norm of update */
	int Monitor,	/* in:	flag to determine monitoring level */
			/*	0 - silent run, 1 - warning messages */
			/*	2 - 1+restart data, 3 - 2+iteration data */
	double* Pconvr	/* out: convergence rate */
	)
{

/* constants */
  int ione = 1;

/* local variables */
  itt_levels* it_level;  /* pointer to current level data structure */
  double *uit;	        /* auxiliary vector */
  int it;		/* iteration counter */ 
  double diff;	        /* max norm of the difference between two iterates */
  double normini;	/* norm of initial correction */
  int iconv = 0;	/* convergence indicator */

/* auxiliary variables */
  int i, iaux, ilev;
  double *vtemp, daux, *exact;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* take proper solver data structure */
  ilev = itv_solver[Solver_id].cur_level;
  it_level = &itv_solver[Solver_id].level[ilev];
  if(Ndof != it_level->Nrdofgl) {
    printf("Not proper number of unknowns (dofs) in it_standard: %d!=%d\n",
	   Ndof,it_level->Nrdofgl);
  }
  
/* write parameters */
  if(Monitor>=ITC_INFO){
    printf("\nStandard iterative solver - entered level %d with tolerance: %14.12f\n",
	   ilev, *Toler);
  }
  
/*kbw
  printf("Level %d - norm of X on entrance %lf\n",ilev,dnrm2_(&Ndof, X, &ione));
  if(B!=NULL) printf("norm B on entrance %lf\n",dnrm2_(&Ndof, B, &ione));
  if(ilev<itv_solver[Solver_id].nr_level-1){
    printf("X on entrance\n");
    for(i=0;i<Ndof;i++) printf("%10.6lf",X[i]);
    printf("\n");
    printf("B on entrance\n");
    for(i=0;i<Ndof;i++) printf("%10.6lf",B[i]);
    printf("\n");
  }
/*kew*/

  uit = it_dvector(Ndof,"usol in GS");
  
  it=0; diff = *Toler+1;
  while(diff>*Toler&&it<*Iter){
    
    it++;

/* rewrite x to uit - to be able to compute the norm of update*/
    dcopy_(&Ndof, X, &ione, uit, &ione);

    iaux=1; /* take into account rhs (possibly from data structure B==NULL */
    if(it>1) Ini_zero=0; /* X is non-zero */

    if(it_level->Solver==STANDARD_IT){

      it_smooth(Solver_id,ilev,iaux,Ini_zero,X,B);

    }
    else if(it_level->Solver==MULTI_GRID){
     
      /* v-cycle is used as multigrid solver */
      it_vcycle(Solver_id,ilev,iaux,Ini_zero,X,B);

    } /* for multi-level GS  */
    else{

      printf("Standard iterations called with wrong Solver parameter\n");
      exit(1);

    }

    diff=0;
    for(i=0;i<Ndof;i++) if(fabs(uit[i]-X[i])>diff) diff=fabs(uit[i]-X[i]);
    
    if(it==1) normini = diff;
    
    if(Monitor>=ITC_INFO){
      printf("GS iteration %d, max norm of difference %.12f\n",
	     it,diff);
    }
    
  } 
  
  free(uit);
  
  if(it<*Iter) iconv = 1;
  else iconv = 0;
  
  *Iter=it;
  *Toler=diff;
  *Pconvr = exp(log(diff/normini)/(it));
  
  return(iconv);
}

/*---------------------------------------------------------
it_vcycle - to perform one V-cycle of multigrid (as multi-level
            smoother): x_out = x_in + M^-1 * ( b - A * x_in )
---------------------------------------------------------*/
void it_vcycle ( 
	int Solver_id,      /* in: solver data structure to be used */
	int Ilev,	/* in: index of the mesh (level) */
	int Use_rhs,	/* in: flag for considering RHS */ 
	int Ini_zero,	/* in: flag for zero initial guess */ 
	double* X, 	/* in/out: initial guess vector and solution */
        double* B	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	)
{

/* constants */
  int ione = 1;
 
/* auxiliary variables */
  itt_levels* it_level; /* in: pointer to current level data structure */
  int i, iaux, jaux, ndof, nr_pre, nr_post;
  double daux, *vtemp, *vtemp1;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/*kbw
  double *vtest;
/*kew*/

/* take proper solver data structure */
  it_level = &itv_solver[Solver_id].level[Ilev];
  ndof = it_level->Nrdofgl;

/*kbw
  vtest = it_dvector(ndof,"temporary vector in vcycle");
  dcopy_(&ndof, X, &ione, vtest, &ione);

  if(Ilev>=0){
    printf("level %d X on entrance\n", Ilev);
    for(i=0;i<ndof;i++) printf("%10.6lf",X[i]);
    printf("\n");
    if(B!=NULL){
      printf("level %d B on entrance\n", Ilev);
      for(i=0;i<ndof;i++) printf("%10.6lf",B[i]);
      printf("\n");
    }
    getchar();getchar();
  }
/*kew*/

/* if not on the coarsest level */
  if(Ilev>0){

    vtemp = it_dvector(ndof,"temporary vector in vcycle");
    vtemp1 = it_dvector(ndof,"temporary vector in vcycle");

/* additional control */
    if(Ilev<itv_solver[Solver_id].nr_level-1){
      nr_pre=it_level->GMRES_type;
      nr_post=it_level->Krylbas;
    }
    else{
      if(it_level->Solver==STANDARD_IT||it_level->Solver==MULTI_GRID){
	nr_pre=it_level->GMRES_type;
	nr_post=it_level->Krylbas;
      }
      else{
	nr_pre=0;
	nr_post=1;
      }
    }

/* perform presmoothing using proper algorithms */
    for(i=0;i<nr_pre;i++) it_smooth(Solver_id, Ilev, Use_rhs, Ini_zero, X, B);
    
/* indicate initial guess is no longer zero */
    Ini_zero=0;

/*kbw
  if(Ilev>=0){
    printf("M^-1*(b-Ax) after %d smoothing steps: \n",nr_pre);
    for(i=0;i<ndof;i++) printf("%10.6lf",X[i]-vtest[i]);
    printf("\n");
    getchar();
  }
/*kew*/

    if(it_level->Monitor>ITC_INFO) 
      printf("\nProjecting error from level %d to level %d\n",Ilev,Ilev-1);

/* transpose of L2 projection from coarse to fine */
/* compute the residual of the not-preconditioned system vtemp = b - Ax */
    //it_compres(it_level, Use_rhs, Ini_zero, ndof, X, B, vtemp);
    lar_compute_residual(it_level->SM_and_LV_id, Use_rhs, Ini_zero, ndof, X, B, vtemp);

/*kbw
  if(Ilev>=0){
    printf("residual on fine grid: \n");
    for(i=0;i<ndof;i++) printf("%10.6lf",vtemp[i]);
    printf("\n");
    getchar();
  }
/*kew*/

/* project residual on coarse grid - stored in vtemp1 */
    itr_proj_sol_lev(Solver_id,Ilev,vtemp,Ilev-1,vtemp1);

/* solve problem on coarser grid */
/* set current solver data structure number */
    itv_solver[Solver_id].cur_level=Ilev-1;

/* vtemp1 is the rhs vector for the coarse grid correction problem */
/* vtemp is the initial guess and is set to zero */
    iaux=1; it_d_zero(vtemp,ndof);

/* v-cycle is used for coarse grid correction (if one wants w-cycle */
/* it_standard should be used here) - vtemp is corrected vtemp1*/
    it_vcycle(Solver_id,Ilev-1,iaux,iaux,vtemp,vtemp1);

/*kbw
  if(Ilev>=0){
    printf("after coarse solve: \n");
    for(i=0;i<ndof;i++) printf("%10.6lf",vtemp[i]);
    printf("\n");
    getchar();
  }
/*kew*/

/* reset current solver data structure number */
    itv_solver[Solver_id].cur_level=Ilev;

/* project back to finer grid */
    if(it_level->Monitor>ITC_INFO) 
      printf("\nProjecting solution from level %d to level %d\n",Ilev-1,Ilev);

    itr_proj_sol_lev(Solver_id,Ilev-1,vtemp,Ilev,vtemp1);


/*kbw
  printf("coarse grid correction projected on fine grid\n");
  for(i=0;i<ndof;i++) printf("%10.6lf",vtemp1[i]);
  printf("\n");
  getchar();
/*kew*/


/* sum up the smoothed initial guess and coarse grid correction */
/* daxpy: X:= daux*vtemp + X */
    daux=1;
    daxpy_(&ndof,&daux,vtemp1,&ione,X,&ione);
    Ini_zero=0; /* indicate initial guess is no longer zero */

    free(vtemp);
    free(vtemp1);

/* perform post-smoothing using proper algorithms */
    for(i=0;i<nr_post;i++) it_smooth(Solver_id, Ilev, Use_rhs, Ini_zero, X, B);

/*kbw
  if(Ilev>=0){
    printf("V after smoothing %d\n", nr_post);
    for(i=0;i<ndof;i++) printf("%10.6lf",X[i]-vtest[i]);
    printf("\n");
    getchar();
  }
  free(vtest);
/*kew*/

  } /* end if not on the coarsest level */
  else {

    it_solve_coarse(Solver_id, Ilev, Ini_zero, X, B);

/*kbw
  if(Ilev>=0){
    printf("X after solving coarse problem\n");
    for(i=0;i<ndof;i++) printf("%10.6lf",X[i]);
    printf("\n");
    getchar();
  }
/*kew*/

  }

  return;
}


/*---------------------------------------------------------
it_smooth - to perform one smoothing step using different algorithms
            x_out = x_in + M^-1 * ( b - A * x_in )
---------------------------------------------------------*/
void it_smooth( 
	int Solver_id,      /* in: solver data structure to be used */
	int Ilev,	/* in: index of the mesh (level) */
	int Use_rhs,	/* in: flag for considering RHS */ 
	int Ini_zero,	/* in: flag for zero initial guess */ 
	double* X, 	/* in/out: initial guess vector and solution */
        double* B	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	)
{
  printf("SMOTHER \n\n\n");
/* constants */
  int ione = 1;

/* auxiliary variables */
  itt_levels* it_level; /* in: pointer to current level data structure */
  int i, iaux, jaux, ndof;
  double daux, *vtemp, *vtemp1;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* take proper solver data structure */
  it_level = &itv_solver[Solver_id].level[Ilev];
  ndof = it_level->Nrdofgl;

/*kbw
  if(Ilev>=0){
    printf("level %d X on entrance\n", Ilev);
    for(i=0;i<ndof;i++) printf("%20.15lf",X[i]);
    //for(i=0;i<ndof;i++) printf("%5d%15.10lf",i/4+1,X[i]);
    printf("\n");
    printf("after level %d X on entrance\n", Ilev);
  }
/*kew*/
/*kbw
  if(Ilev>=0){
    if(B!=NULL){
      printf("level %d B on entrance\n", Ilev);
      for(i=0;i<ndof;i++) printf("%5d%15.10lf",i/4+1,B[i]);
      printf("\n");
    }
    printf("after level %d B on entrance\n", Ilev);
    getchar();
  }
/*kew*/

/* different preconditioner options used for smoothing */
  if(it_level->Solver==GMRES&&it_level->GMRES_type==MATRIX_FREE){
    
    if(it_level->Precon==BLOCK_GS){
      it_mfmiter(it_level,Use_rhs,Ini_zero,ndof,X,B); // not implemented for ModFEM
    }
    else if(it_level->Precon==BLOCK_JACOBI){
      it_mfaiter(it_level,Use_rhs,Ini_zero,ndof,X,B); // not implemented for ModFEM
    }
      
  }
  else{
    
    if(it_level->Precon==MULTI_ILU || it_level->Precon==BLOCK_ILU){

/* incomplete LU preconditioning : */
      vtemp = it_dvector(ndof,"temporary vector in smooth");
      vtemp1 = it_dvector(ndof,"temporary vector in smooth");
      
      iaux = 0; /* initial guess not zero */
      //it_compres(it_level, Use_rhs, iaux, ndof, X, B, vtemp);
      lar_compute_residual ( it_level->SM_and_LV_id,  Use_rhs, iaux, ndof, X, B, vtemp);
/* routine returned: vtemp = B - A * X */ 

      //it_rhsub(it_level,ndof,vtemp1,vtemp);
      lar_perform_rhsub( it_level->SM_and_LV_id, ndof, vtemp1, vtemp);
/* routine returned: vtemp1 = M^-1 * vtemp = M^-1 * ( B - A * X ) */ 
	
/* add X to get  X:= X + M^-1 ( B - A * X )*/
      daux = 1.;
      daxpy_(&ndof, &daux, vtemp1, &ione, X, &ione);
	
      free(vtemp1);
      free(vtemp);
 	
    }
    else if(it_level->Precon==BLOCK_JACOBI || it_level->Precon==BLOCK_GS){


      lar_perform_BJ_or_GS_iterations( it_level->SM_and_LV_id,
				       Use_rhs, Ini_zero, it_level->Nr_prec, ndof, X, B);

      /* if(abs(it_level->Block_type)==0) { */
      /* 	it_blsiter(it_level,Use_rhs,Ini_zero,ndof,X,B); */
      /* } */
      /* else { */
      /* 	it_blliter(it_level,Use_rhs,Ini_zero,ndof,X,B); */
      /* } */

    }
    else{

      printf("Unknown option for smoothing (not preconditioning!) %d\n",
	     it_level->Precon);
      exit(1);

    }

  }


/*kbw
  if(Ilev>=0){
    printf("X before exchange dofs\n");
    //for(i=0;i<ndof;i++) printf("%5d%15.10lf",i/4+1,X[i]);
    for(i=0;i<40;i++) printf("%20.15lf",X[i]);
    printf("\n");
    getchar();
  }
/*kew*/

/*||begin||*/
  it_par_exchange_dofs(X);
/*||end||*/

/*kbw
  if(Ilev>=0){
    printf("X after exchange dofs\n");
    //for(i=0;i<ndof;i++) printf("%5d%15.10lf",i/4+1,X[i]);
    for(i=0;i<40;i++) printf("%20.15lf",X[i]);
    printf("\n");
    getchar();
  }
/*kew*/

  return;

}


/*---------------------------------------------------------
it_precon - to perform preconditioning using different algorithms
            x_out = M^-1 * b 
---------------------------------------------------------*/
void it_precon( 
	int Solver_id,      /* in: solver data structure to be used */
	int Ilev,	/* in: index of the mesh (level) */
	double* X, 	/* in/out: initial guess vector and solution */
        double* B	/* in:  the rhs vector, if NULL take rhs */
			/*      from block data structure */
	)
{

	printf("PRECONDITIONER \n\n\n");
/* constants */
  int ione = 1;

/* auxiliary variables */
  itt_levels* it_level; /* in: pointer to current level data structure */
  int i, iaux, jaux, ndof;
  double daux, *vtemp, *vtemp1;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* take proper solver data structure */
  it_level = &itv_solver[Solver_id].level[Ilev];
  ndof = it_level->Nrdofgl;

/* different preconditioner options */
  if(it_level->Precon==ADD_SCHWARZ){

    iaux = 1; /* initial guess is zero for preconditioning */
    jaux = 1; /* use supplied B as rhs */
    it_d_zero(X,ndof);

/*kbw
    printf("it_precon - V on entrance (ndof == %d)\n", ndof);
  int i;
  for(i=0;i<20;i++) printf("%20.15lf",X[i]);
  //for(i=0;i<Ndof;i++) printf("%10.6lf",V[i]);
  printf("\n");
  getchar();
/*kew*/

    //it_blliter(it_level,jaux,iaux,ndof,X,B);
    lar_perform_BJ_or_GS_iterations( it_level->SM_and_LV_id,
				     jaux, iaux,  it_level->Nr_prec, ndof, X, B);
      
  }
  else if(it_level->Precon==BLOCK_ILU){

/* incomplete LU preconditioning : */
    //it_rhsub(it_level,ndof,X,B);
    lar_perform_rhsub( it_level->SM_and_LV_id, ndof, X, B);
/* routine returned: X = M^-1 * B */ 
	 
  }
  else{

    printf("Unknown option for preconditioning (not smoothing!) %d\n",
	   it_level->Precon);
    exit(1);

  }

/*||begin||*/
  it_par_exchange_dofs(X);
/*||end||*/

  return;

}

/*---------------------------------------------------------
it_compreres - to compute the residual of the left preconditioned 
	system of equations, v = M^-1 * ( b - Ax )
        ( used also to compute the product v = M^-1 * Ax)
---------------------------------------------------------*/
void it_compreres (
	int Solver_id,   /* in: solver data structure to be used */
	int Control, /* in: indicator whether to compute residual (1-use RHS) */
		     /*	or matrix-vector product (0-do not use RHS) */
	int Ini_zero,/* in: flag for zero initial guess */ 
	int Ndof,    /* in: number of unknowns (components of x) */
	double* X,   /* in: initial guess vector */
        double* B,   /* in:  the rhs vector, if NULL take rhs */
		     /*      from block data structure */
	double* V    /* out: initial residual, v = M^-1*(b-Ax) */
	)
{

/* constants */
  int ione = 1;

/* auxiliary variables */
  itt_levels* it_level; /* in: pointer to current level data structure */
  int i, iaux, jaux, ilev;
  double daux, *vtemp;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* take proper solver data structure */
  ilev = itv_solver[Solver_id].cur_level;
  it_level = &itv_solver[Solver_id].level[ilev];
  if(Ndof != it_level->Nrdofgl) {
    printf("Not proper number of unknowns (dofs) in it_compreres: %d!=%d\n",
	   Ndof,it_level->Nrdofgl);
  }

/*kbw
  if(ilev>=0){
    printf("X on entrance in compreres\n");
    //for(i=0;i<20;i++) printf("%10.6lf",X[i]);
    for(i=0;i<Ndof;i++) printf("%10.6lf",X[i]);
    printf("\nafter X on entrance in compreres\n");
    getchar();getchar();
  }
/*kew*/

/* if there is no preconditioning */
  if(it_level->Precon == NO_PRECON){

/* just compute the residual of the not-preconditioned system */
    //it_compres(it_level, Control, Ini_zero, Ndof, X, B, V);
    lar_compute_residual ( it_level->SM_and_LV_id, Control, Ini_zero, Ndof, X, B, V);

  }
/* single level preconditioning */
  else if(it_level->Precon==ADD_SCHWARZ||it_level->Precon==BLOCK_ILU){

/* incomplete LU preconditioning or */
/* textbook additive Schwarz preconditioning with */
/* 1. multiplying X by A (or computing not preconditioned residual) */
/* 2. standard iterations with residual as rhs and zero initial guess */

    vtemp = it_dvector(Ndof,"temporary vector in compreres");
      
    //it_compres(it_level, Control, Ini_zero, Ndof, X, B, vtemp);
    lar_compute_residual ( it_level->SM_and_LV_id, Control, Ini_zero, Ndof, X, B, vtemp);
/* the result is vtemp = B - A * X */

/* if on the coarsest or the only one level */
    if(ilev==0){

/* perform preconditioning */
      it_precon(Solver_id, ilev, V, vtemp);
/* the result is V = M^-1 * ( B - A * X ) */

    } /* end if on the coarsest level */
    else {

/* perform multi-level preconditioning */
      printf("Multi-level preconditioning only based on smoothing. Exiting\n");
      exit(0);

    }
	
    free(vtemp);
 	
  }
/* single or multi level preconditioning using smoothing */
  else{

/* copy X to V */
    dcopy_(&Ndof, X, &ione, V, &ione);

/* if on the coarsest or the only one level */
    if(ilev==0){

/*kbw
  if(ilev>=0){
    printf("X before smoothing\n");
    for(i=0;i<Ndof;i++) printf("%10.6lf",V[i]);
    printf("\n");
  }
/*kew*/

/* perform smoothing as preconditioning */
      it_smooth(Solver_id, ilev, Control, Ini_zero, V, B);
/* the result is V = X + M^-1 * ( B - A * X ) */

/*kbw
  if(ilev>=0){
    printf("X after smoothing\n");
    for(i=0;i<Ndof;i++) printf("%10.6lf",V[i]);
    //for(i=0;i<20;i++) printf("%10.6lf",V[i]);
    printf("\n");
  }
/*kew*/

    } /* end if on the coarsest level */
    else {

/* one v-cycle multigrid is used as approximate solver-preconditioner */
      it_vcycle(Solver_id, ilev, Control, Ini_zero, V, B);
/* the result is V = X + M^-1 * ( B - A * X ) */

    }

/* subtract X from V to get V = M^-1 * ( B - A * X ) */
    daux = -1.;
    daxpy_(&Ndof, &daux, X, &ione, V, &ione);

  } /* end if there is preconditioning */

  if(Control==0){
/* multiply by -1 to get v = M^-1 *  Ax  */
    daux= -1;
    dscal_(&Ndof, &daux, V, &ione);
  }
   
/*kbw
  if(ilev>=0){
    printf("V after compreres\n");
    for(i=0;i<Ndof;i++) printf("%10.6lf",V[i]);
    //for(i=0;i<20;i++) printf("%10.6lf",V[i]);
    printf("\n");
    getchar();getchar();
  }
/*kew*/

  return;
}


/*---------------------------------------------------------
it_solve_coarse - to launch solver for the coarse grid problem
---------------------------------------------------------*/
int it_solve_coarse(  /* returns: 1 - success; <=0 - error code*/
	int Solver_id,        /* in: solver data structure to be used */
	int Ilev,	/* in: index of the mesh (level) */
	int Ini_zero,   /* in:  indicator whether initial guess is zero (0/1) */
	double* X,	/* in/out: initial guess and solution vector */
	double* B	/* in: rhs vector */
	)
{

/* auxiliary variables */
  itt_levels *it_level;
  int i, iaux, jaux, kaux, iblock, ierr;
  double daux, eaux, faux, gaux, normb;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* take proper solver data structure */
  it_level = &itv_solver[Solver_id].level[Ilev];

/* specify maximal number of iterations */
  kaux=it_level->Max_iter; 

  if(it_level->Solver==STANDARD_IT||it_level->Solver==MULTI_GRID){

/* specify kind and value of tolerance */
    daux = it_level->Conv_meas;

/*------- STANDARD ITERATIONS SOLVER  --------*/
    iaux = it_standard(Solver_id, it_level->Nrdofgl, Ini_zero, X, B,
		       &kaux, &daux, it_level->Monitor, &gaux);
    
    if(it_level->Monitor>=ITC_INFO){
      if(iaux)printf("Convergence in GS after %d iterations!!!\n",kaux);
      else printf("No convergence in GS after %d iterations!!!\n",kaux);
      printf("Total convergence rate (average decrease in update per iteration) %f\n",
	     gaux);
      printf("Norm of update %15.12f\n",daux);
    }

  }
  else{
    
/* specify kind and value of tolerance */
    daux = 0.0;       /* relative to initial residual tolerance */
    eaux = 0.0;	/* residual tolerance */
    faux = 0.0;	/* relative to rhs residual tolerance */
    if(it_level->Conv_type==0) daux = it_level->Conv_meas;
    else if(it_level->Conv_type==1) eaux = it_level->Conv_meas;
    else if(it_level->Conv_type==2) faux = it_level->Conv_meas;

/* compute the norm of rhs (excluded since it is not parallel) */
    iaux=1;
    normb=1.0;  

/*kbw
    printf("Before entering GMRES: Solver_id %d, Nrdofgl %d, Ini_zero %d\n",
	   Solver_id, it_level->Nrdofgl, Ini_zero);
    printf("                       Krylbas %d, Monitor %d\n",
	   it_level->Krylbas, it_level->Monitor);
printf("rhs:\n");
for(i=0;i<it_level->Nrdofgl;i++) printf("%10.6lf",B[i]);
printf("\nini_guess:\n");
for(i=0;i<it_level->Nrdofgl;i++) printf("%10.6lf",X[i]);
printf("\n");
/*kew*/

/*--- GMRES SOLVER ---*/
    iaux = it_gmres(Solver_id, it_level->Nrdofgl, Ini_zero, X, B,
		    it_level->Krylbas, &kaux, &eaux, &daux, &faux,
		    it_level->Monitor, &gaux);
        
    if(it_level->Monitor>=ITC_INFO){
      if(iaux) printf("Convergence in GMRES after %d iterations!!!\n",kaux);
      else  printf("No convergence in GMRES after %d iterations!!!\n",kaux);
      printf("Total convergence rate (average decrease in residual per iteration) %f\n",
	     gaux);
      printf("Norm of residual %15.12f, relative decrease %15.12f\n",eaux,daux);
      printf("Current ratio (norm of residual)/(norm of rhs) = %15.12f\n",faux);
    }
  }
    
  if(it_level->Monitor>=ITC_INFO){
    printf("\nCoarse grid correction problem Solved for level %d\n", Ilev);
  }


/*kbw
printf("mesh level %d, nrdof %d, precon %d\n",
Ilev,it_level->Nrdofgl,it_level->Precon);
printf("rhs:\n");
for(i=0;i<it_level->Nrdofgl;i++) printf("%10.6lf",B[i]);
printf("\nsolution:\n");
for(i=0;i<it_level->Nrdofgl;i++) printf("%10.6lf",X[i]);
printf("\n");
getchar();
/*kew*/
/* check whether B = A * X */  
#ifdef DEBUG
  if(Ilev==0){
    
    // just compute the residual
    double *vtemp;
    vtemp = (double *) malloc(it_level->Nrdofgl*sizeof(double));
    
    int matrix_id = it_level->SM_and_LV_id;
    lar_compute_residual ( matrix_id, 1, 0, it_level->Nrdofgl, X, B, vtemp);
    
    for(i=0; i<it_level->Nrdofgl; i++){
      if(fabs(vtemp[i])>TOL){
	ierr=1;
	printf("Error in coarse solve: entry %d - residual %lf \n",
	       i,vtemp[i]);
      }
    }
    if(!ierr) printf("Coarse problem solved correctly !\n");
  }
#endif
  
  return(1);
}

