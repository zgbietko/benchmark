/************************************************************************
File: pdh_elast.h - headers of internal routines for problem depending
                   elasticity module

Contains declarations of routines:
  pdr_exact_sol - to return values and derivatives at a point
	          for functions used as exact solutions for test problems
  ut_d_zero - to zero a double vector of length Num
  ut_put_list - to put Num on the list List with length Ll
                  (filled with numbers and zeros at the end)

History:
	02.2002 - Krzysztof Banas, initial version
*************************************************************************/


/* constant parameter - number of equations = solution components */
#define PDC_NREQ 3

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

typedef struct {
	int en;
	double hsize;
	int adpt;
	int save;
	int err;
	double multip;
}pdt_grains;

/* problem definition data structure */
typedef struct {
	int name;
	int mesh_id;
	int nreq;
	int nr_sol;
	int field_id;
	int sigma;
	int inv;
	pdt_adpts adpt;
	pdt_grains gr;
} pdt_problem;

/* material data type  */
typedef struct {
	int id;
	double Eyoung;
	double Poisni;
} pdt_material;

/*struct for boundary data*/
typedef struct {
	int type;
	double vec[3];
}pdt_boundary;

/**--------------------------------------------------------
procedures for adaptation
----------------------------------------------------------*/
/*struct for patches*/
typedef struct {
	int size;
	int *patch;
	int size_stiff_mat;
	int *posglob;
	double *sigma;
} pdt_patches_old;

extern pdt_patches_old *lista;

extern int pdr_create_patch(
		int Field_id
		);

extern int pdr_free_list();

extern int pdr_get_zzhu_error(
		int Field_id
		);

extern int pdr_zzhu_error(
		int Field_id
		);

extern int pdr_compute_eyoung(
		int Field_id
		);

extern int pdr_get_sigma(
		int field_id
		);

extern int pdr_write_sigma(
		int field_id
		);

/**--------------------------------------------------------
pdr_adapt - to enforce adaptation strategy for a given problem
---------------------------------------------------------*/
extern int pdr_adapt( /* returns: >0 - success, <=0 - failure */
  int Field_id      /* in: problem data structure to be used */
  );

extern int pdr_adapt2( /* returns: >0 - success, <=0 - failure */
  int Field_id      /* in: problem data structure to be used */
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
	double *Exact_z
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

extern int pointintriangle(
		double *p,
		double *a,
		double *b,
		double *c
		);

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

/*** GLOBAL VARIABLES for the whole module ***/
pdt_material *pdv_material; /* pointers to material data */
pdt_boundary *pdv_bc; /*pointers to bc data*/
pdt_problem pdv_problem;

char work_dir[200]; /* the name of the working directory */
int matnum; /*amount of materials used*/
int bcnum; /*amount of bc used*/
