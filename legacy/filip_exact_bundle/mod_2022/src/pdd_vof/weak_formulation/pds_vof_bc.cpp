/************************************************************************
File pds_vof_bc.c- definition of functions related to boundary conditions 
			   handling

Contains definition of routines:
  pdr_vof_get_bc_assign_count - get num of bc with conditions set in file
  pdr_vof_get_bc_type - get type of vof bc for boundary
  pdr_vof_get_bc_data - get vof bc data
  pdr_vof_update_timedep_bc - update timedependent boundary conditions
  pdr_vof_get_vf_at_point - calculates vof flux at point

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include <math.h>

/* types and functions related to boundary conditions handling */
#include "../include/pdh_vof_bc.h"		/* IMPLEMENTS */

#define PDC_VOF_PI 3.141592654
#define PDC_VOF_SQRT_PI 1.772453851
#define PDC_VOF_SQRT_3 1.732050808

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_vof_get_bc_assign_count - get num of boundaries with conditions set in file
------------------------------------------------------------*/
int pdr_vof_get_bc_assign_count(const pdt_vof_bc * Bc_db)
{
  return Bc_db->bc_assignments_count;
}

/*------------------------------------------------------------
pdr_vof_get_bc_type - get type of vof for boundary
------------------------------------------------------------*/
pdt_vof_bctype pdr_vof_get_bc_type(const pdt_vof_bc * Bc_db, int Bnum)
{
  int i;
  for (i = 0; i < Bc_db->bc_assignments_count; ++i) {
    if (Bc_db->bc_assignments[i].bnum == Bnum)
      return Bc_db->bc_assignments[i].bc_vof;
  }

  return BC_VOF_NONE;
}

/*------------------------------------------------------------
pdr_vof_get_bc_data - get vof bc data
------------------------------------------------------------*/
void *pdr_vof_get_bc_data(const pdt_vof_bc * Bc_db, int Bnum /*, void *bc_data */ )
{
  int i;
  for (i = 0; i < Bc_db->bc_assignments_count; ++i) {
    if (Bc_db->bc_assignments[i].bnum == Bnum) {
      switch (Bc_db->bc_assignments[i].bc_vof) {
      case BC_VOF_ISOVOF:
	return &(Bc_db->bc_isovof[Bc_db->bc_assignments[i].bc_vof_data_idx]);
      case BC_VOF_NORMAL_VOF_FLUX:
	return &(Bc_db->bc_normal_vof_flux[Bc_db->bc_assignments[i].bc_vof_data_idx]);
      case BC_VOF_SOURCE:
	return &(Bc_db->bc_vof_source[Bc_db->bc_assignments[i].bc_vof_data_idx]);
      /* case BC_HEAT_RADCONV: */
      /* 	return &(Bc_db->bc_radconv[Bc_db->bc_assignments[i].bc_heat_data_idx]); */
      default:
	return NULL;
      }
    }
  }
  return NULL;
}

/*------------------------------------------------------------
pdr_vof_update_timedep_bc - update time dependent boundary conditions
------------------------------------------------------------*/
int pdr_vof_update_timedep_bc(const pdt_vof_bc * Bc_db, double Time)
{

  /* int i; */
  /* pdt_heat_bc_goldak_source *bc_goldak; */
  /* for (i = 0; i < Bc_db->bc_assignments_count; ++i) { */
  /*   if (Bc_db->bc_assignments[i].bc_heat == BC_HEAT_GOLDAK_HEAT_SOURCE) { */
  /*     bc_goldak = &(Bc_db->bc_goldak[Bc_db->bc_assignments[i].bc_heat_data_idx]); */
  /*     bc_goldak->current_pos[0] = bc_goldak->init_pos[0] + (bc_goldak->v[0] * Time); */
  /*     bc_goldak->current_pos[1] = bc_goldak->init_pos[1] + (bc_goldak->v[1] * Time); */
  /*     bc_goldak->current_pos[2] = bc_goldak->init_pos[2] + (bc_goldak->v[2] * Time); */
  /*   } */
  /* } */

  return 0;
}

/*------------------------------------------------------------
pdr_vof_bc_get_vf_at_point - calculates vof flux at point
------------------------------------------------------------*/
double pdr_vof_bc_get_vf_at_point(
  double X, double Y, double Z, 
  const pdt_vof_bc_vof_source * Bc_vof, 
  const double *Vec_norm)
{
  // see "A new Finite Element Model for Welding Heat Sources"
  //        J.Goldak, A.Chakravarti and M.Bibby
  //        June 1984
  //        Equation : [14]
  //

// return q [W/m^3]
//        Equation : [16,17]
  /* if (X >= Bc_goldak->current_pos[0]) */
  /*   return ((6.0 * PDC_HEAT_SQRT_3 * Bc_goldak->ff * Bc_goldak->Q) / (Bc_goldak->a * Bc_goldak->b * Bc_goldak->c1 * PDC_HEAT_PI * PDC_HEAT_SQRT_PI) * */
  /* 	    exp(-3.0 * (((Y - Bc_goldak->current_pos[1]) * (Y - Bc_goldak->current_pos[1])) / (Bc_goldak->a  * Bc_goldak->a) + */
  /* 			((Z - Bc_goldak->current_pos[2]) * (Z - Bc_goldak->current_pos[2])) / (Bc_goldak->b  * Bc_goldak->b) + */
  /* 			((X - Bc_goldak->current_pos[0]) * (X - Bc_goldak->current_pos[0])) / (Bc_goldak->c1 * Bc_goldak->c1)))); */
  /* else */
  /*   return ((6.0 * PDC_HEAT_SQRT_3 * Bc_goldak->fr * Bc_goldak->Q) / (Bc_goldak->a * Bc_goldak->b * Bc_goldak->c2 * PDC_HEAT_PI * PDC_HEAT_SQRT_PI) * */
  /* 	    exp(-3.0 * (((Y - Bc_goldak->current_pos[1]) * (Y - Bc_goldak->current_pos[1])) / (Bc_goldak->a  * Bc_goldak->a) + */
  /* 			((Z - Bc_goldak->current_pos[2]) * (Z - Bc_goldak->current_pos[2])) / (Bc_goldak->b  * Bc_goldak->b) + */
  /* 			((X - Bc_goldak->current_pos[0]) * (X - Bc_goldak->current_pos[0])) / (Bc_goldak->c2 * Bc_goldak->c2)))); */

  return 0;
  
//        Equation : [14]
/*  return ((6.0 * PDC_HEAT_SQRT_3 * Bc_goldak->Q) / (Bc_goldak->a * Bc_goldak->b * Bc_goldak->c1 * PDC_HEAT_PI * PDC_HEAT_SQRT_PI) *
      exp(-3.0 * (((Y - Bc_goldak->current_pos[0]) * (Y - Bc_goldak->current_pos[0])) / ( Bc_goldak->a  * Bc_goldak->a) +
                  ((Z - Bc_goldak->current_pos[1]) * (Z - Bc_goldak->current_pos[1])) / ( Bc_goldak->b  * Bc_goldak->b) +
                  ((X - Bc_goldak->current_pos[2]) * (X - Bc_goldak->current_pos[2])) / ( Bc_goldak->c1 * Bc_goldak->c1))));
*/
}
