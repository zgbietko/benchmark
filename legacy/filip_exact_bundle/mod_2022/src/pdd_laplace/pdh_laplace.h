/************************************************************************
File: pdh_intf.h - headers of internal routines for problem depending test
                   module (with Laplace's equation)

Contains declarations of routines:
  pdr_adapt - to enforce adaptation strategy for a given problem 
  pdr_get_mat_data - to get material data from material structures
  pdr_zzhu_error - to get Zienkiewicz-Zhu error estimator
  pdr_exact_sol - to return values and derivatives at a point
	          for functions used as exact solutions for test problems
  pdr_fa_coeff - to return coefficients for face integrals
  pdr_get_bc_type - to get BC type given BC flag from mesh data structure
                    !!! according to some adopted convention !!!
  pdr_pde_coeff - to return coefficients of the original pdes
  pdr_bc_diri_coeff - to return DIRICHLET boundary coeficients
                     (routine substitutes only non-zero values)
  pdr_bc_neum_coeff - to return NEUMANN boundary coeficients
                     (routine substitutes only non-zero values)
  pdr_bc_mixed_coeff - to return ROBIN (mixed) boundary coeficients
  ut_d_zero - to zero a double vector of length Num
  ut_put_list - to put Num on the list List with length Ll
                  (filled with numbers and zeros at the end)

History:
	02.2002 - Krzysztof Banas, initial version
*************************************************************************/


/* constant parameter - number of equations = solution components */
#define PDC_NREQ 1

/* structure with control parameters */
typedef struct {
  int     name;		/* name (identifier for problem dependent routines) */
  int     mesh_id;      /* ID of the associated mesh */
  int     field_id;     /* ID of the associated approximation field */
  int     init_bvp;	/* type of problem: */
			/*     -1 - bvp only, no initial condition, */
                        /*          pure Neumann boundary conditions */
			/*	0 - bvp only, no initial condition */
			/*	1 - initial bvp or nonlinear bvp: */
			/*	    initial condition supplied */
  int	  nreq;		/* number of equations (solution components) */
  int	  nr_sol;	/* number of solution vectors stored by approximation */
                        /* module (for time dependent/nonlinear problems) */
  int     nr_mat;       /* number of materials (0 - no materials data) */
  int     slope;       	/* slope limiter: */
			/* 	1 - active */
			/* 	0 - not active */
} pdt_ctrls;


/* structure with adaptation parameters */
typedef struct {
  int	 type;		/* strategy number for adaptation */
  int	 interval;	/* number of time steps between adaptations */
  int	 maxgen;	/* maximum generation level for elements */
  int	 maxgendiff;	/* maximum difference of generation levels */
                        /* for neighboring elements */
  double eps;		/* coefficient for choosing elements to adapt */
  double ratio;		/* ratio of errors for derefinements */
  int    monitor;       /* monitoring level: */
                        /*   PDC_SILENT      0  */
                        /*   PDC_ERRORS      1  */
                        /*   PDC_INFO        2  */
                        /*   PDC_ALLINFO     3  */
} pdt_adpts;

/* material data type  */
typedef struct {
  double diff_coeff;
} pdt_material;


/*** GLOBAL VARIABLES for the whole module ***/
extern pdt_ctrls  pdv_ctrl;	/* structure with control parameters */
extern pdt_adpts  pdv_adpt;	/* structure with adaptation parameters */
extern pdt_material *pdv_material; /* pointers to material data */


/**--------------------------------------------------------
pdr_adapt - to enforce adaptation strategy for a given problem 
---------------------------------------------------------*/
extern int pdr_adapt( /* returns: >0 - success, <=0 - failure */
  int Problem_id,       /* in: problem data structure to be used */
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
  );

/**--------------------------------------------------------
pdr_get_mat_data - to get material data from material structures
----------------------------------------------------------*/
int pdr_get_mat_data(
        int Mat_num,   /* in: material number */
	double* Data1  /* out: first material data */
	);

/**--------------------------------------------------------
pdr_zzhu_error - to get Zienkiewicz-Zhu error estimator
----------------------------------------------------------*/
double pdr_zzhu_error(
  int Field_id,    /* in: approximation field ID  */
  FILE *interactive_output
);

/**--------------------------------------------------------
 * pdr_exact_sol - to return values and derivatives at a point
 * 	for functions used as exact solutions for test problems
 * 	----------------------------------------------------------*/
extern int pdr_exact_sol(
			/* returns: 1-found exact solution, 0-not found */
	int Mat_num,    /* in: material number (coefficients of pde) */
	double X,	/* in: coordinates of a point */
	double Y,
	double Z,
	double Time,	/* in: time instant */
	double *Exact,	/* out: values of solution and derivatives */
	double *Exact_x,
	double *Exact_y,
	double *Exact_z,
	double *Lapl	/* out: Laplacian of function */
	);

/**--------------------------------------------------------
pdr_fa_coeff - to return coefficients for face integrals
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
	);

/**-----------------------------------------------------------
  pdr_get_bc_type - to get BC type given BC flag from mesh data structure
                    !!! according to some adopted convention !!!
------------------------------------------------------------*/
int pdr_get_bc_type( /* returns: bc type for a face: */
		/*  	1 (PDC_INTERIOR) - interior face */
		/*  	2 (PDC_BC_DIRI) - Dirichlet boundary face */
		/*  	3 (PDC_BC_NEUM) - Neumann boundary face */
		/*  	4 (PDC_BC_MIXED) - Robin boundary face */
   int Fa_bc	/* in: BC flag (returned by mesh data structure) */
   );

/**--------------------------------------------------------
  pdr_pde_coeff - to return coefficients of the original pdes
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
	);

/**--------------------------------------------------------
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
	);

/**--------------------------------------------------------
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
	);

/**--------------------------------------------------------
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
	);


/*** utilities ***/

/**--------------------------------------------------------
ut_d_zero - to zero a double vector of length Num
---------------------------------------------------------*/
extern void ut_d_zero(
	double* Vec, 	/* in, out: vector to initialize */
	int Num		/* in: vector length */
	);

/**--------------------------------------------------------
ut_put_list - to put Num on the list List with length Ll
	(filled with numbers and zeros at the end)
---------------------------------------------------------*/
extern int ut_put_list( /* returns*/
		/*  >0 - position already occupied on the list */
             	/*  <0 - position at which put on the list */
            	/*   0 - list full, not found on the list */
	int Num, 	/* in: number to put on the list */
	int* List, 	/* in: list */
	int Ll		/* in: total list's lengths */
		 );

