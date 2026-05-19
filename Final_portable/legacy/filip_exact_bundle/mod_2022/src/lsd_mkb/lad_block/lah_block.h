/* lah_block.h - local header for matrix storage and operations package */
/*    based on block storage and providing basic operations for */
/*    standard iterative methods (Jacobi, Gauss-Seidel, additive Schwarz, */
/*    multiplicative Schwarz)     */
/* Assumption: Each matrix is stored with the corresponding preconditioner */


#ifndef _lah_block_
#define _lah_block_

/*** External Parameters ***/

#define ITC_MAX_MATRICES 10



/*** Data types ***/

/* definition of type itt_block */
typedef struct {
/* number of degrees of freedom in a single diagonal elementary block */
  int Ndof;
/* list of neighbor blocks, Lngb[0] - number of neighbors */
  int *Lngb;
/* position of the first dof in the global rhs vector */
  int Posg;
/* array for pivoting information*/
  int *Ips;
/* stiffness matrix for diagonal elementary block */
  double *Dia;
/* RHS vector for diagonal elementary block*/
  double *Rhs;
/* stiffness matrices for off diagonal elementary blocks */
  double **Aux;
  } itt_blocks;

/* definition of type itt_blocks_dia */
typedef struct {
/* list of elementary blocks' numbers, Lsmall[0] - number of blocks */
  int *Lsmall;
/* list of elements in the subdoamin, Lelem[0] - number of elements */
  int *Lelem;
/* list of neighbors of the subdoamin, Lneig[0] - number of neighbors */
  int *Lneig;
/* list of positions of first dofs from elementary blocks in diagonal 
subarray; Lpos[0] - total number of dofs for dia */
  int *Lpos;
/* list of lower subdiagonal neighbors - for ILU preconditioning, */
/*  Llowerneig[0] - number of neighbors */
  int *Llowerneig;
/* list of upper subdiagonal neighbors - for ILU preconditioning, */
/*  Lupperneig[0] - number of neighbors */
  int *Lupperneig;
/* array for pivoting information*/
  int *Ips;
/* factorized diagonal blocks (subarray) */
  double *Dia;
/* factorized off diagonal blocks */
  double **Aux;
  } itt_blocks_dia;


typedef struct {

/* control variables */
  int Block_type; /* block types: number of nodes in a block */
		  /*   or some other indicator application dependent */
  int Precon;     /* type of preconditioner determining the storage format */
		  /*   of the preconditioner matrix Block_dia */
/* parameters */
  int Max_SM_size;      /* the maximal number of dofs in a stiffness matrix */
  int Nrblocks;		/* total number of small blocks */
  int Nrblocks_dia;	/* total number of diagonal blocks */
  int Nrdofgl;		/* total number of degrees of freedom */

/* blocks for storing system array and preconditioner arrays */
  itt_blocks **Block;	      /* array of pointers to small blocks */
  itt_blocks_dia **Block_dia; /* array of pointers to diagonal blocks */

} itt_matrices;


/* GLOBAL VARIABLES */
extern int   itv_nr_matrices;        /* the number of solvers in the problem */
extern int   itv_cur_matrix_id;                /* ID of the current problem */
extern itt_matrices itv_matrices[ITC_MAX_MATRICES];        /* array of solvers */



// internal utilities
/*
lar_util_dvector - to allocate a double vector: name[0..ncom-1]:
lar_util_ivector - to allocate an integer vector: name[0..ncom-1]:
lar_util_imatrix - to allocate an integer matrix name[0..nrow-1][0..ncol-1]: 
                  name=imatrix(nrow,ncol,error_text) 
lar_util_dmatrix - to allocate a double matrix name[0..nrow-1][0..ncol-1]: 
                  name=imatrix(nrow,ncol,error_text) 
lar_util_chk_list - to check whether a number is on the list
lar_util_put_list - to put Num on the list List with length Ll 
lar_util_d_zero - to zero a double vector
lar_util_i_zero - to zero an integer vector
lar_util_sort - to heap-sort an array
lar_util_dgetrf - quasi-LU decomposition of a matrix
lar_util_dgetrs - to perform forward reduction and back substitution
    of the RHS vector for solving a system of linear equations
*/


extern double *lar_util_dvector( 
	/* return: pointer to allocated vector */
	int Ncom,  	/* in: number of components */
	char Error_text[]/* in: error text to be printed */
	);

extern int *lar_util_ivector(    
	/* return: pointer to allocated vector */
	int Ncom, 	/* in: number of components */
	char Error_text[]/* in: error text to be printed */
	);

extern int **lar_util_imatrix( /* returns: pointer to array of pointers to integers */
	int Nrow, 	/* in: number of rows */
	int Ncol, 	/* in: number of columns */
	char Error_text[]/* in: text to print in case of error */
	);

extern double **lar_util_dmatrix( /* returns: pointer to array of pointers to doubles */
	int Nrow, 	/* in: number of rows */
	int Ncol, 	/* in: number of columns */
	char Error_text[]/* in: text to print in case of error */
	);

extern int lar_util_chk_list(	/* returns: */
			/* >0 - position on the list */
            		/* 0 - not found on the list */
	int Num, 	/* number to be checked */
	int* List, 	/* list of numbers */
	int Ll		/* length of the list */
	);

extern int lar_util_put_list( /* returns*/
		/*  >0 - position already occupied on the list */
             	/*   0 - put on the list */
            	/*  -1 - list full, not found on the list */
	int Num, 	/* in: number to put on the list */
	int* List, 	/* in: list */
	int Ll		/* in: total list's lengths */
	);

extern void lar_util_d_zero(double *Vec, int Num);

extern void lar_util_i_zero(int *Vec, int Num);

extern void lar_util_sort(
   int    *Ind_array,    /* in/out: index array for sorting */
   double *Val_array     /* in: array of values used for sorting */
   );

extern void lar_util_dgetrf(double* a, int m, int* ips);

extern void lar_util_dgetrs(double* a, int m, double* b, double* x, int* ips);



#endif
