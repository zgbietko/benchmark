/************************************************************************
File pds_ns_supg_bc.c- definition of functions related to boundary conditions 
			   handling

Contains definition of routines:
  pdr_ns_supg_get_pressure_pins_count
  pdr_ns_supg_get_velocity_pins_count
  pdr_ns_supg_get_bc_assign_count - get num of bc with conditions set in file
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

#include <math.h>

/* types and functions related to boundary conditions handling */
#include "../include/pdh_ns_supg_bc.h"		/* IMPLEMENTS */


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

// everywhere below: Bc_db is boundary conditions database  
// (definition in pdh_ns_supg_bc.h)

/*------------------------------------------------------------
pdr_ns_supg_get_pressure_pins_count
------------------------------------------------------------*/
int pdr_ns_supg_get_pressure_pins_count(const pdt_ns_supg_bc * Bc_db)
{
  return Bc_db->pin_pressure_count;
}

/*------------------------------------------------------------
pdr_ns_supg_get_velocity_pins_count
------------------------------------------------------------*/
int pdr_ns_supg_get_velocity_pins_count(const pdt_ns_supg_bc * Bc_db)
{
  return Bc_db->pin_velocity_count;
}

/*------------------------------------------------------------
pdr_ns_supg_get_bc_assign_count - get num of bc with conditions set in file
------------------------------------------------------------*/
int pdr_ns_supg_get_bc_assign_count(const pdt_ns_supg_bc * Bc_db)
{
  return Bc_db->bc_assignments_count;
}

/*------------------------------------------------------------
pdr_ns_supg_get_pressure_pin - get pressure pin data
------------------------------------------------------------*/
pdt_ns_supg_pin_pressure *pdr_ns_supg_get_pressure_pin(
  const pdt_ns_supg_bc * Bc_db, 
  int Idx /*, pdt_ns_supg_pin_pressure* pin_data */ )
{
  /*
     pin_data->p = Bc_db->pin_pressure[idx].p;
     pin_data->pin_node_coor[0] = Bc_db->pin_pressure[idx].pin_node_coor[0];
     pin_data->pin_node_coor[1] = Bc_db->pin_pressure[idx].pin_node_coor[1];
     pin_data->pin_node_coor[2] = Bc_db->pin_pressure[idx].pin_node_coor[2];
   */
  return &(Bc_db->pin_pressure[Idx]);
}

/*------------------------------------------------------------
pdr_ns_supg_get_velocity_pin - get velocity pin data
------------------------------------------------------------*/
pdt_ns_supg_pin_velocity *pdr_ns_supg_get_velocity_pin(
  const pdt_ns_supg_bc * Bc_db, 
  int idx /*, pdt_ns_supg_pin_velocity* pin_data */ )
{
  return &(Bc_db->pin_velocity[idx]);
}

/*------------------------------------------------------------
pdr_ns_supg_get_bc_type - get type of flow bc for boundary
------------------------------------------------------------*/
pdt_ns_supg_bctype pdr_ns_supg_get_bc_type(
  const pdt_ns_supg_bc * Bc_db, 
  int Bnum
)
{
  int i;
  for (i = 0; i < Bc_db->bc_assignments_count; ++i) {
    if (Bc_db->bc_assignments[i].bnum == Bnum)
      return Bc_db->bc_assignments[i].bc_ns_supg;
  }

  return BC_NS_SUPG_NONE;
}


/*------------------------------------------------------------
pdr_ns_supg_get_bc_data - get flow bc data
------------------------------------------------------------*/
void *pdr_ns_supg_get_bc_data(
  const pdt_ns_supg_bc * Bc_db, 
  int Bnum /*, void *bc_data */ )
{
  int i;
  for (i = 0; i < Bc_db->bc_assignments_count; ++i) {
    if (Bc_db->bc_assignments[i].bnum == Bnum) {
      switch (Bc_db->bc_assignments[i].bc_ns_supg) {
      case BC_NS_SUPG_INFLOW_CIRCLE3D:
	return &(Bc_db->bc_inflow_circle_3d[Bc_db->bc_assignments[i].bc_ns_supg_data_idx]);
      case BC_NS_SUPG_INFLOW_RECT2D:
	return &(Bc_db->bc_inflow_rect_2d[Bc_db->bc_assignments[i].bc_ns_supg_data_idx]);
      case BC_NS_SUPG_INFLOW_LINEAR2D:
	return &(Bc_db->bc_inflow_linear_2d[Bc_db->bc_assignments[i].bc_ns_supg_data_idx]);
      case BC_NS_SUPG_VELOCITY:
	return &(Bc_db->bc_velocity[Bc_db->bc_assignments[i].bc_ns_supg_data_idx]);
      case BC_NS_SUPG_MARANGONI:
        return &(Bc_db->bc_marangoni[Bc_db->bc_assignments[i].bc_ns_supg_data_idx]);
      case BC_NS_SUPG_NOSLIP:
      case BC_NS_SUPG_OUTFLOW:
      case BC_NS_SUPG_SYMMETRY:
      default:
	return NULL;
      }
    }
  }
  return NULL;
}


/*------------------------------------------------------------
pdr_ns_supg_update_timedep_bc - update timedependent boundary conditions
------------------------------------------------------------*/
int pdr_ns_supg_update_timedep_bc(const pdt_ns_supg_bc * Bc_db, double Time)
{

  return 0;
}

