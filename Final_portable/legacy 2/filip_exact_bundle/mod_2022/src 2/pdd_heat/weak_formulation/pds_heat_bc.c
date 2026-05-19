/************************************************************************
File pds_heat_bc.c - definition of functions related to boundary conditions 
			   handling

Contains definition of routines:
  pdr_heat_get_bc_assign_count - get num of bc with conditions set in file
  pdr_heat_get_bc_type - get type of heat bc for boundary
  pdr_heat_get_bc_data - get heat bc data
  pdr_heat_update_timedep_bc - update timedependent boundary conditions
  pdr_heat_get_goldak_hf_at_point - calculates goldak heat flux at point

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#include <math.h>

/* types and functions related to boundary conditions handling */
#include "../include/pdh_heat_bc.h"		/* IMPLEMENTS */
#include "uth_log.h"



#ifdef __cplusplus
extern "C" {
#endif

#define PDC_HEAT_PI 3.141592654
#define PDC_HEAT_SQRT_PI 1.772453851
#define PDC_HEAT_SQRT_3 1.732050808

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_heat_get_bc_assign_count - get num of boundaries with conditions set in file
------------------------------------------------------------*/
int pdr_heat_get_bc_assign_count(const pdt_heat_bc * Bc_db)
{
  return Bc_db->bc_assignments_count;
}

/*------------------------------------------------------------
pdr_heat_get_bc_type - get type of heat bc for boundary
------------------------------------------------------------*/
pdt_heat_bctype pdr_heat_get_bc_type(const pdt_heat_bc * Bc_db, int Bnum)
{
  int i;
  for (i = 0; i < Bc_db->bc_assignments_count; ++i) {
    if (Bc_db->bc_assignments[i].bnum == Bnum)
      return Bc_db->bc_assignments[i].bc_heat;
  }

  return BC_HEAT_NONE;
}

/*------------------------------------------------------------
pdr_heat_get_bc_data - get heat bc data
------------------------------------------------------------*/
void *pdr_heat_get_bc_data(const pdt_heat_bc * Bc_db, int Bnum /*, void *bc_data */ )
{
  int i;
  for (i = 0; i < Bc_db->bc_assignments_count; ++i) {
    if (Bc_db->bc_assignments[i].bnum == Bnum) {
      switch (Bc_db->bc_assignments[i].bc_heat) {
      case BC_HEAT_ISOTHERMAL:
	return &(Bc_db->bc_isothermal[Bc_db->bc_assignments[i].bc_heat_data_idx]);
      case BC_HEAT_NORMAL_HEAT_FLUX:
	return &(Bc_db->bc_normal_heat_flux[Bc_db->bc_assignments[i].bc_heat_data_idx]);
      case BC_HEAT_GOLDAK_HEAT_SOURCE:
	return &(Bc_db->bc_goldak[Bc_db->bc_assignments[i].bc_heat_data_idx]);
      case BC_HEAT_RADCONV:
	return &(Bc_db->bc_radconv[Bc_db->bc_assignments[i].bc_heat_data_idx]);
		case BC_HEAT_OUTFLOW:
	return &(Bc_db->bc_outflow[Bc_db->bc_assignments[i].bc_heat_data_idx]);
		case BC_HEAT_CONTACT:
	return &(Bc_db->bc_contact[Bc_db->bc_assignments[i].bc_heat_data_idx]);
	
      default:
		  mf_log_warn("Not found BC flag!");
	return NULL;
      }
    }
  }
  return NULL;
}

/*------------------------------------------------------------
pdr_heat_contact_get_alfa_in_the_temp
------------------------------------------------------------*/
double pdr_heat_contact_get_alfa_in_the_temp(const pdt_heat_bc_contact *bc,const double temp){

if(bc->setting_length==1){return bc->alfa;}

int lenght = bc->setting_length,flag=1;

double alfaReturn;
if( temp <= bc->alfaTemp[0] ){alfaReturn =bc->alfaFor[0];flag=0;}
if( temp >= bc->alfaTemp[lenght-1] ){alfaReturn =bc->alfaFor[lenght-1];flag=0;}

//printf("   wsp1             %lf %lf    %lf %lf \n",bc->alfaTemp[10],bc->alfaFor[10],bc->alfaTemp[0],bc->alfaFor[0]);


if(flag){
		int i=1;
        for(;i<lenght;++i){
               if(temp < bc->alfaTemp[i]){
									//t2>tx>t1 ((t2-tx)/(t2-t1))*val_t1 + ((tx-t1)/(t2-t1))*val_t2
					alfaReturn = ((bc->alfaTemp[i]-temp)/(bc->alfaTemp[i]-bc->alfaTemp[i-1]))*bc->alfaFor[i-1]+((temp-bc->alfaTemp[i-1])/(bc->alfaTemp[i]-bc->alfaTemp[i-1]))*bc->alfaFor[i]; 
					break;
                
				}
        }
}


return alfaReturn;

}
/*------------------------------------------------------------
pdr_heat_radconv_get_alfa_in_the_temp
------------------------------------------------------------*/
double pdr_heat_radconv_get_alfa_in_the_temp(const pdt_heat_bc_radconv *bc,const double temp){

if(bc->setting_length==1){return bc->alfa;}

int lenght = bc->setting_length,flag=1;

double alfaReturn;
if( temp <= bc->alfaTemp[0] ){alfaReturn =bc->alfaFor[0];flag=0;}
if( temp >= bc->alfaTemp[lenght-1] ){alfaReturn =bc->alfaFor[lenght-1];flag=0;}

//printf("   wsp1             %lf %lf    %lf %lf \n",bc->alfaTemp[10],bc->alfaFor[10],bc->alfaTemp[0],bc->alfaFor[0]);


if(flag){
		int i=1;
        for(;i<lenght;++i){
               if(temp < bc->alfaTemp[i]){
									//t2>tx>t1 ((t2-tx)/(t2-t1))*val_t1 + ((tx-t1)/(t2-t1))*val_t2
					alfaReturn = ((bc->alfaTemp[i]-temp)/(bc->alfaTemp[i]-bc->alfaTemp[i-1]))*bc->alfaFor[i-1]+((temp-bc->alfaTemp[i-1])/(bc->alfaTemp[i]-bc->alfaTemp[i-1]))*bc->alfaFor[i]; 
					break;
                
				}
        }
}


return alfaReturn;

}

/*------------------------------------------------------------
pdr_heat_update_timedep_bc - update timedependent boundary conditions
------------------------------------------------------------*/
int pdr_heat_update_timedep_bc(const pdt_heat_bc * Bc_db, double Time)
{
  //currently only goldak needs to be updated (moved)

  int i;
  pdt_heat_bc_goldak_source *bc_goldak;
  for (i = 0; i < Bc_db->bc_assignments_count; ++i) {
    if (Bc_db->bc_assignments[i].bc_heat == BC_HEAT_GOLDAK_HEAT_SOURCE) {
      bc_goldak = &(Bc_db->bc_goldak[Bc_db->bc_assignments[i].bc_heat_data_idx]);
      bc_goldak->current_pos[0] = bc_goldak->init_pos[0] + (bc_goldak->v[0] * Time);
      bc_goldak->current_pos[1] = bc_goldak->init_pos[1] + (bc_goldak->v[1] * Time);
      bc_goldak->current_pos[2] = bc_goldak->init_pos[2] + (bc_goldak->v[2] * Time);
    }
  }

  return 0;
}

/*------------------------------------------------------------
pdr_heat_bc_get_goldak_hf_at_point - calculates goldak heat flux at point
------------------------------------------------------------*/
double pdr_heat_bc_get_goldak_hf_at_point(
  double X, double Y, double Z, 
  const pdt_heat_bc_goldak_source * Bc_goldak, 
  const double *Vec_norm)
{
  // see "A new Finite Element Model for Welding Heat Sources"
  //        J.Goldak, A.Chakravarti and M.Bibby
  //        June 1984
  //        Equation : [14]
  //

// return q [W/m^3]
//        Equation : [16,17]
  if (X >= Bc_goldak->current_pos[0])
    return ((6.0 * PDC_HEAT_SQRT_3 * Bc_goldak->ff * Bc_goldak->Q) / (Bc_goldak->a * Bc_goldak->b * Bc_goldak->c1 * PDC_HEAT_PI * PDC_HEAT_SQRT_PI) *
	    exp(-3.0 * (((Y - Bc_goldak->current_pos[1]) * (Y - Bc_goldak->current_pos[1])) / (Bc_goldak->a  * Bc_goldak->a) +
			((Z - Bc_goldak->current_pos[2]) * (Z - Bc_goldak->current_pos[2])) / (Bc_goldak->b  * Bc_goldak->b) +
			((X - Bc_goldak->current_pos[0]) * (X - Bc_goldak->current_pos[0])) / (Bc_goldak->c1 * Bc_goldak->c1))));
  else
    return ((6.0 * PDC_HEAT_SQRT_3 * Bc_goldak->fr * Bc_goldak->Q) / (Bc_goldak->a * Bc_goldak->b * Bc_goldak->c2 * PDC_HEAT_PI * PDC_HEAT_SQRT_PI) *
	    exp(-3.0 * (((Y - Bc_goldak->current_pos[1]) * (Y - Bc_goldak->current_pos[1])) / (Bc_goldak->a  * Bc_goldak->a) +
			((Z - Bc_goldak->current_pos[2]) * (Z - Bc_goldak->current_pos[2])) / (Bc_goldak->b  * Bc_goldak->b) +
			((X - Bc_goldak->current_pos[0]) * (X - Bc_goldak->current_pos[0])) / (Bc_goldak->c2 * Bc_goldak->c2))));
  
//        Equation : [14]
/*  return ((6.0 * PDC_HEAT_SQRT_3 * Bc_goldak->Q) / (Bc_goldak->a * Bc_goldak->b * Bc_goldak->c1 * PDC_HEAT_PI * PDC_HEAT_SQRT_PI) *
      exp(-3.0 * (((Y - Bc_goldak->current_pos[0]) * (Y - Bc_goldak->current_pos[0])) / ( Bc_goldak->a  * Bc_goldak->a) +
                  ((Z - Bc_goldak->current_pos[1]) * (Z - Bc_goldak->current_pos[1])) / ( Bc_goldak->b  * Bc_goldak->b) +
                  ((X - Bc_goldak->current_pos[2]) * (X - Bc_goldak->current_pos[2])) / ( Bc_goldak->c1 * Bc_goldak->c1))));
*/
}


#ifdef __cplusplus
}
#endif
