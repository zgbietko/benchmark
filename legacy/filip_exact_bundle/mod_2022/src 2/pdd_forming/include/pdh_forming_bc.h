/************************************************************************
File pdh_forming_bc.h - types and functions related to boundary conditions 
			   for Forming approximation
			   
Contains definition of types used by bcs			   

Contains declarations of routines:
  pdr_forming_bc_read - read bc data from config file
  pdr_forming_bc_free - free bc resources
  pdr_forming_get_bc_count - get num of boundaries with conditions set in file
  pdr_forming_get_pressure_pins_count
  pdr_forming_get_velocity_pins_count
  pdr_forming_get_pressure_pin - get pressure pin data
  pdr_forming_get_velocity_pin - get velocity pin data
  pdr_forming_get_bc_type - get type of flow bc for boundary
  pdr_forming_get_bc_data - get flow bc data
  pdr_forming_update_timedep_bc - update timedependent boundary conditions

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl) (ns_supg)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
    2015    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl) (forming)
*************************************************************************/

#ifndef PDH_FORMING_BC_H
#define PDH_FORMING_BC_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" 
{
#endif

/**************************************/
/* TYPES                              */
/**************************************/
/* Rules:
/* - type name starts always with pdt_ */

/* types of forming bcs */
typedef enum {
  BC_FORMING_NONE,
  BC_FORMING_TRACTION,
  BC_FORMING_VELOCITY,
  BC_FORMING_TOOL_CONTACT
} pdt_forming_bctype;

  typedef enum {
	BC_FORMING_VEL_ORIENT_GLOB=0,
	BC_FORMING_VEL_ORIENT_LOC=1
  } pdt_forming_bc_vel_orient;

/* FLOW BCs and pins */

/*
every structure contains data related to
particular flow bc
*/

//symm - don't need struct for symm

typedef struct {
  int bnum;
  double f[3];
  int f_orient;
} pdt_forming_bc_traction;

typedef struct {
  int bnum;
  double v[3];
  int vel_orient;
} pdt_forming_bc_velocity;

typedef struct {
  int bnum;
  double v_tool[3];
  int vel_tool_orient;
} pdt_forming_bc_tool_contact;

/* utility types - not for direct access!*/

typedef struct {
  int bnum;
  pdt_forming_bctype bc_forming;
  int bc_forming_data_idx;
} pdt_forming_bc_assignments;

/* main structure containing bc data */
typedef struct {
  pdt_forming_bc_assignments *bc_assignments;
  int bc_assignments_count;

  pdt_forming_bc_traction *bc_traction;
  int bc_traction_count;

  pdt_forming_bc_velocity *bc_velocity;
  int bc_velocity_count;

  pdt_forming_bc_tool_contact *bc_tool_contact;
  int bc_tool_contact_count;

} pdt_forming_bc;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
pdr_forming_bc_read - read bc data from config file
---------------------------------------------------------*/
int pdr_forming_bc_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_forming_bc *Bc_db);

/**--------------------------------------------------------
pdr_forming_bc_free - free bc resources
---------------------------------------------------------*/
int pdr_forming_bc_free(pdt_forming_bc *Bc_db);

/**--------------------------------------------------------
pdr_forming_get_bc_assign_count - get num of boundaries with conditions set in file
---------------------------------------------------------*/
int pdr_forming_get_bc_assign_count(const pdt_forming_bc *Bc_db); 
  /* in: pdt_bc structure to read from */

/**--------------------------------------------------------
pdr_formig_get_bc_type - get type of flow bc for boundary
---------------------------------------------------------*/
pdt_formig_bctype pdr_formig_get_bc_type(
  const pdt_forming_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Bnum /* in: boundary number */
  );


/**--------------------------------------------------------
pdr_forming_get_bc_data - get flow bc data
---------------------------------------------------------*/
void* pdr_forming_get_bc_data(
  const pdt_forming_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Bnum /* in: boundary number */
  );


/**--------------------------------------------------------
pdr_forming_update_timedep_bc - update timedependent boundary conditions
---------------------------------------------------------*/
int pdr_forming_update_timedep_bc(
  const pdt_forming_bc *Bc_db, /* in: pdt_bc structure to read from */
  double Time
  );


#ifdef __cplusplus
}
#endif

#endif
