/************************************************************************
File pdh_ns_supg_bc.h - types and functions related to boundary conditions 
			   for Navier-Stokes SUPG approximation
			   
Contains definition of types used by bcs			   

Contains declarations of routines:
  pdr_ns_supg_bc_read - read bc data from config file
  pdr_ns_supg_bc_free - free bc resources
  pdr_ns_supg_get_bc_count - get num of boundaries with conditions set in file
  pdr_ns_supg_get_pressure_pins_count
  pdr_ns_supg_get_velocity_pins_count
  pdr_ns_supg_get_pressure_pin - get pressure pin data
  pdr_ns_supg_get_velocity_pin - get velocity pin data
  pdr_ns_supg_get_bc_type - get type of flow bc for boundary
  pdr_ns_supg_get_bc_data - get flow bc data
  pdr_ns_supg_update_timedep_bc - update timedependent boundary conditions

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#ifndef PDH_NS_SUPG_BC_H
#define PDH_NS_SUPG_BC_H

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

/* types of flow bcs */
typedef enum {
  BC_NS_SUPG_NONE,
  BC_NS_SUPG_INFLOW_RECT2D,
  BC_NS_SUPG_INFLOW_LINEAR2D,
  BC_NS_SUPG_INFLOW_CIRCLE3D,
  BC_NS_SUPG_OUTFLOW,
  BC_NS_SUPG_NOSLIP,
  BC_NS_SUPG_SYMMETRY,
  BC_NS_SUPG_VELOCITY,
  BC_NS_SUPG_MARANGONI
} pdt_ns_supg_bctype;

  typedef enum {
	BC_NS_SUPG_VEL_ORIENT_GLOB=0,
	BC_NS_SUPG_VEL_ORIENT_LOC=1
  } pdt_ns_supg_bc_vel_orient;
  
/* FLOW BCs and pins */

/*
every structure contains data related to
particular flow bc
*/

typedef struct {
  int bnum;
  double n1[3];
  double n2[3];
  double n3[3];
  double n4[3];
  double v; //inflow normal peak velocity
} pdt_ns_supg_bc_inflow_rect_2d;

typedef struct {
  int bnum;
  double n1[3];
  double n2[3];
  double v; //inflow normal peak velocity
} pdt_ns_supg_bc_inflow_linear_2d;

typedef struct {
  int bnum;
  double center[3];
  double radius;
  double v; //inflow normal peak velocity
} pdt_ns_supg_bc_inflow_circle_3d;

//noslip - don't need struct for noslip
//symm - don't need struct for symm

typedef struct {
  int bnum;
  double pressure;  
} pdt_ns_supg_bc_outflow;

typedef struct {
  int bnum;
  double v[3];
  int vel_orient;
} pdt_ns_supg_bc_velocity;

typedef struct {
  double p;
  double pin_node_coor[3];
} pdt_ns_supg_pin_pressure;

typedef struct {
  double v[3];
  double pin_node_coor[3]; 
} pdt_ns_supg_pin_velocity;

typedef struct {
  int bnum;
  double n[3];	 //versor (normalized vector) normal to the (>free<)surface
  double node_coor[3];	//(>free<)surface node coordinate
} pdt_ns_supg_bc_marangoni;


/* utility types - not for direct access!*/

typedef struct {
  int bnum;
  pdt_ns_supg_bctype bc_ns_supg;
  int bc_ns_supg_data_idx;
} pdt_ns_supg_bc_assignments;

/* main structure containing bc data */
typedef struct {
  pdt_ns_supg_bc_assignments *bc_assignments;
  int bc_assignments_count;
  
  pdt_ns_supg_bc_inflow_circle_3d *bc_inflow_circle_3d;
  int bc_inflow_circle_3d_count;

  pdt_ns_supg_bc_inflow_rect_2d *bc_inflow_rect_2d;
  int bc_inflow_rect_2d_count;
  
  pdt_ns_supg_bc_inflow_linear_2d *bc_inflow_linear_2d;
  int bc_inflow_linear_2d_count;

  pdt_ns_supg_bc_outflow *bc_outflow;
  int bc_outflow_count;

  pdt_ns_supg_bc_velocity *bc_velocity;
  int bc_velocity_count;
  
  pdt_ns_supg_bc_marangoni *bc_marangoni;
  int bc_marangoni_count;

  pdt_ns_supg_pin_pressure *pin_pressure;
  int pin_pressure_count;

  pdt_ns_supg_pin_velocity *pin_velocity;
  int pin_velocity_count;


} pdt_ns_supg_bc;


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/**--------------------------------------------------------
pdr_ns_supg_bc_read - read bc data from config file
---------------------------------------------------------*/
int pdr_ns_supg_bc_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_ns_supg_bc *Bc_db);

/**--------------------------------------------------------
pdr_ns_supg_bc_free - free bc resources
---------------------------------------------------------*/
int pdr_ns_supg_bc_free(pdt_ns_supg_bc *Bc_db);

/**--------------------------------------------------------
pdr_ns_supg_get_pressure_pins_count
---------------------------------------------------------*/
int pdr_ns_supg_get_pressure_pins_count(const pdt_ns_supg_bc *Bc_db); 
  /* in: pdt_bc structure to read from */

/**--------------------------------------------------------
pdr_ns_supg_get_velocity_pins_count
---------------------------------------------------------*/
int pdr_ns_supg_get_velocity_pins_count(const pdt_ns_supg_bc *Bc_db); 
  /* in: pdt_bc structure to read from */

/**--------------------------------------------------------
pdr_ns_supg_get_bc_assign_count - get num of boundaries with conditions set in file
---------------------------------------------------------*/
int pdr_ns_supg_get_bc_assign_count(const pdt_ns_supg_bc *Bc_db); 
  /* in: pdt_bc structure to read from */

/**--------------------------------------------------------
pdr_ns_supg_get_pressure_pin - get pressure pin data
---------------------------------------------------------*/
pdt_ns_supg_pin_pressure* pdr_ns_supg_get_pressure_pin(
  const pdt_ns_supg_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Idx  /* in: pin pressure no. */
  );

/**--------------------------------------------------------
pdr_ns_supg_get_velocity_pin - get velocity pin data
---------------------------------------------------------*/
pdt_ns_supg_pin_velocity* pdr_ns_supg_get_velocity_pin(
  const pdt_ns_supg_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Idx /* in: pin pressure no. */
  );

/**--------------------------------------------------------
pdr_ns_supg_get_bc_type - get type of flow bc for boundary
---------------------------------------------------------*/
pdt_ns_supg_bctype pdr_ns_supg_get_bc_type(
  const pdt_ns_supg_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Bnum /* in: boundary number */
  );


/**--------------------------------------------------------
pdr_ns_supg_get_bc_data - get flow bc data
---------------------------------------------------------*/
void* pdr_ns_supg_get_bc_data(
  const pdt_ns_supg_bc *Bc_db, /* in: pdt_bc structure to read from */
  int Bnum /* in: boundary number */
  );


/**--------------------------------------------------------
pdr_ns_supg_update_timedep_bc - update timedependent boundary conditions
---------------------------------------------------------*/
int pdr_ns_supg_update_timedep_bc(
  const pdt_ns_supg_bc *Bc_db, /* in: pdt_bc structure to read from */
  double Time
  );


#ifdef __cplusplus
}
#endif

#endif
