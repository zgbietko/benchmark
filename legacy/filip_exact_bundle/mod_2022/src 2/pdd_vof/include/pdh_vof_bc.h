/************************************************************************
File pdh_vof_bc.h - types and functions related to boundary conditions 
			   for vof transfer equations
			   
Contains definition of types used by bcs			   

Contains declarations of routines:
  pdr_vof_bc_read - read bc data from config file
  pdr_vof_bc_free - free bc resources
  pdr_vof_get_bc_type - get type of vof bc for boundary
  pdr_vof_get_bc_data - get vof bc data
  pdr_vof_update_timedep_bc - update timedependent boundary conditions

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#ifndef PDH_VOF_BC_H
#define PDH_VOF_BC_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


/**************************************/
/* TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always with pdt_vof_ */

/* types of vof bcs */
typedef enum {
  BC_VOF_NONE,
  BC_VOF_SYMMETRY,
  BC_VOF_ISOVOF,
  BC_VOF_NORMAL_VOF_FLUX,
  BC_VOF_SOURCE,
  /* BC_HEAT_RADCONV */
} pdt_vof_bctype;



/* VOF BCs */

/*
  every structure contains data related to
  particular vof bc
*/

typedef struct {
  /* int bnum; */
  /* double Q; //energy input rate [W] */
  /* double v[3]; //welding speed [m/s] */
  /* //double v_dir[3]; //welding direction */
  /* double ff; //heat input fraction - front */
  /* double fr; //heat input fraction - rear (sum of both rear and front must be 2.0) */
  /* double a; //semi-axes */
  /* double b; */
  /* double c1; */
  /* double c2; */
  /* double init_pos[3]; //initial position (point in the middle where c1 and c2 meet) */
  /* double current_pos[3]; */
  
  /* double h; //boundary conv. coeff */
  /* double eps; //boundary rad. coeff */
  /* double versor[3]; //unit vector orientation of the heat source */
} pdt_vof_bc_vof_source;

/* typedef struct { */
/*   int bnum; */
/*   double h; //boundary conv. coeff */
/*   double eps; //boundary rad. coeff */
/* } pdt_heat_bc_radconv; */

typedef struct {
  int bnum;
  double temp;  
} pdt_vof_bc_isovof;

typedef struct {
  int bnum;
  double flux;
} pdt_vof_bc_normal_vof_flux;


/* utility types - not for direct access!*/

typedef struct {
  int bnum;
  pdt_vof_bctype bc_vof;
  int bc_vof_data_idx;
} pdt_vof_bc_assignments;

/* main structure containing bc data */
typedef struct {
  pdt_vof_bc_assignments *bc_assignments;
  int bc_assignments_count;
  
  pdt_vof_bc_isovof *bc_isovof;
  int bc_isovof_count;

  pdt_vof_bc_normal_vof_flux *bc_normal_vof_flux;
  int bc_normal_vof_flux_count;
  
  pdt_vof_bc_vof_source *bc_vof_source;
  int bc_vof_source_count;
  
  /* pdt_heat_bc_radconv *bc_radconv; */
  /* int bc_radconv_count; */
  
} pdt_vof_bc;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_vof_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
pdr_vof_bc_read - read bc data from config file
---------------------------------------------------------*/
int pdr_vof_bc_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_vof_bc *Bc_db);

/**--------------------------------------------------------
pdr_vof_bc_free - free bc resources
---------------------------------------------------------*/
int pdr_vof_bc_free(pdt_vof_bc *Bc_db);


/**--------------------------------------------------------
pdr_vof_get_bc_count - get num of boundaries with conditions set in file
---------------------------------------------------------*/
int pdr_vof_get_bc_count(const pdt_vof_bc *Bc_db); 
  /* in: pdt_bc structure to read from */


/**--------------------------------------------------------
pdr_vof_get_bc_type - get type of vof bc for boundary
---------------------------------------------------------*/
pdt_vof_bctype pdr_vof_get_bc_type(
  const pdt_vof_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Bnum /* in: boundary number */
  );


/**--------------------------------------------------------
pdr_vof_get_bc_data - get vof bc data
---------------------------------------------------------*/
void* pdr_vof_get_bc_data(
 const pdt_vof_bc *Bc_db, /* in: pdt_bc structure to read from */
 int Bnum /* in: boundary number */
 );

/**--------------------------------------------------------
pdr_vof_update_timedep_bc - update timedependent boundary conditions
---------------------------------------------------------*/
int pdr_vof_update_timedep_bc(
  const pdt_vof_bc *Bc_db, /* in: pdt_bc structure to read from */
  double Time
  );

/**--------------------------------------------------------
pdr_vof_bc_get_voff_at_point - calculates vof flux at point
---------------------------------------------------------*/
double pdr_vof_bc_get_vf_at_point(
  double X,
  double Y,
  double Z,
  const pdt_vof_bc_vof_source *Bc_vof, 
  /* in: vof bc data - get it with pdr_vof_get_bc_data */
  const double *Vec_norm
  );

#ifdef __cplusplus
}
#endif

#endif
