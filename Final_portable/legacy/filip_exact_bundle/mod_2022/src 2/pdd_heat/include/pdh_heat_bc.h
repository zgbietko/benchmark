/************************************************************************
File pdh_heat_bc.h - types and functions related to boundary conditions 
			   for heat transfer equations
			   
Contains definition of types used by bcs			   

Contains declarations of routines:
  pdr_heat_bc_read - read bc data from config file
  pdr_heat_bc_free - free bc resources
  pdr_heat_get_bc_type - get type of heat bc for boundary
  pdr_heat_get_bc_data - get heat bc data
  pdr_heat_update_timedep_bc - update timedependent boundary conditions
  pdr_heat_bc_get_goldak_hf_at_point - calculates goldak heat flux at point

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#ifndef PDH_HEAT_BC_H
#define PDH_HEAT_BC_H

#include <stdio.h>
#include "uth_mesh.h"
#include "uth_mat.h"

/**************************************/
/* TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always with pdt_heat_ */


#ifdef __cplusplus
extern "C" {
#endif

/* types of heat bcs */
typedef enum {
  BC_HEAT_NONE,
  BC_HEAT_SYMMETRY,
  BC_HEAT_OUTFLOW,
  BC_HEAT_ISOTHERMAL,
  BC_HEAT_NORMAL_HEAT_FLUX,
  BC_HEAT_GOLDAK_HEAT_SOURCE,
  BC_HEAT_RADCONV,
  BC_HEAT_CONV,
  BC_HEAT_CONTACT
} pdt_heat_bctype;



/* HEAT BCs */

/*
every structure contains data related to
particular heat bc
*/

typedef struct {
  int bnum;
  double Q; //energy input rate [W]
  double v[3]; //welding speed [m/s]
  //double v_dir[3]; //welding direction
  double ff; //heat input fraction - front
  double fr; //heat input fraction - rear (sum of both rear and front must be 2.0)
  double a; //semi-axes
  double b;
  double c1;
  double c2;
  double init_pos[3]; //initial position (point in the middle where c1 and c2 meet)
  double current_pos[3];
  
  double h; //boundary conv. coeff
  double eps; //boundary rad. coeff
  double versor[3]; //unit vector orientation of the heat source
} pdt_heat_bc_goldak_source; //double ellipsoid model

typedef struct {
  int bnum;
  double *alfaFor; 		
  double *alfaTemp;
  int setting_length;
  double t_inf; //temperature
  double eps; //boundary rad. coeff
  double alfa; //heat transfer coefficient  //boundary conv. coeff & rad. coeff
} pdt_heat_bc_radconv;

typedef struct {
  int bnum;
  double temp;  
} pdt_heat_bc_isothermal;

typedef struct {
  int bnum;
  double flux;  
} pdt_heat_bc_normal_flux;

typedef struct {
  int bnum;
    
} pdt_heat_bc_outflow;

typedef struct {
	int bnum;
	double *alfaFor; 		
	double *alfaTemp; 
	double alfa;
	int setting_length;
    int IDs[2];
	utt_mesh_bc_type type;
} pdt_heat_bc_contact;


typedef struct {
	int bnum; 
    int IDs[2];
	utt_mesh_bc_type type;
} pdt_heat_bc_set_bcnum;

/* utility types - not for direct access!*/

typedef struct {
  int bnum;
  pdt_heat_bctype bc_heat;
  int bc_heat_data_idx;
} pdt_heat_bc_assignments;

/* main structure containing bc data */
typedef struct {
  pdt_heat_bc_assignments *bc_assignments;
  int bc_assignments_count;
  
  pdt_heat_bc_isothermal *bc_isothermal;
  int bc_isothermal_count;

  pdt_heat_bc_normal_flux *bc_normal_heat_flux;
  int bc_normal_heat_flux_count;
  
  pdt_heat_bc_goldak_source *bc_goldak;
  int bc_goldak_count;
  
  pdt_heat_bc_radconv *bc_radconv;
  int bc_radconv_count;
  
  pdt_heat_bc_outflow *bc_outflow;
  int bc_outflow_count;

  pdt_heat_bc_contact *bc_contact;
  int bc_contact_count; 
  
  pdt_heat_bc_set_bcnum *bc_set_bcnum;
  int bc_set_bcnum_count;
  
} pdt_heat_bc;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_heat_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
pdr_heat_bc_read - read bc data from config file
---------------------------------------------------------*/
int pdr_heat_bc_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_heat_bc *Bc_db);

/**--------------------------------------------------------
pdr_heat_bc_free - free bc resources
---------------------------------------------------------*/
int pdr_heat_bc_free(pdt_heat_bc *Bc_db);

/**--------------------------------------------------------
pdr_heat_contact_get_alfa_in_the_temp - get_alfa_in_the_temp bc_contact
---------------------------------------------------------*/
double pdr_heat_contact_get_alfa_in_the_temp(const pdt_heat_bc_contact *bc,const double temp);
double pdr_heat_radconv_get_alfa_in_the_temp(const pdt_heat_bc_radconv *bc,const double temp);

/**--------------------------------------------------------
pdr_heat_get_bc_count - get num of boundaries with conditions set in file
---------------------------------------------------------*/
int pdr_heat_get_bc_count(const pdt_heat_bc *Bc_db); 
  /* in: pdt_bc structure to read from */


/**--------------------------------------------------------
pdr_heat_get_bc_type - get type of heat bc for boundary
---------------------------------------------------------*/
pdt_heat_bctype pdr_heat_get_bc_type(
  const pdt_heat_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Bnum /* in: boundary number */
  );


/**--------------------------------------------------------
pdr_heat_get_bc_data - get heat bc data
---------------------------------------------------------*/
void* pdr_heat_get_bc_data(
 const pdt_heat_bc *Bc_db, /* in: pdt_bc structure to read from */
 int Bnum /* in: boundary number */
 );

/**--------------------------------------------------------
pdr_heat_update_timedep_bc - update timedependent boundary conditions
---------------------------------------------------------*/
int pdr_heat_update_timedep_bc(
  const pdt_heat_bc *Bc_db, /* in: pdt_bc structure to read from */
  double Time
  );

/**--------------------------------------------------------
pdr_heat_bc_get_goldak_hf_at_point - calculates goldak heat flux at point
---------------------------------------------------------*/
double pdr_heat_bc_get_goldak_hf_at_point(
  double X,
  double Y,
  double Z,
  const pdt_heat_bc_goldak_source *Bc_goldak, 
  /* in: goldak bc data - get it with pdr_heat_get_bc_data */
  const double *Vec_norm
  );


#ifdef __cplusplus
}
#endif

#endif
