/************************************************************************
File pds_heat_bc_io.c - definition of functions related to boundary conditions 
			   read and management.

Contains definition of routines:
  pdr_heat_bc_read - read bc data from config file
  pdr_heat_bc_free - free bc resources
  *other are local to this file

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>

/* types and functions related to boundary conditions handling */
#include "../include/pdh_heat_bc.h" /* IMPLEMENTS */
#include "uth_log.h"
//#include "uth_log.h"


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_heat_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_heat_bc_config_zeroall
------------------------------------------------------------*/
int pdr_heat_bc_config_zeroall(pdt_heat_bc *Bc_db)
{
  Bc_db->bc_assignments = 0;
  Bc_db->bc_assignments_count = 0;
  Bc_db->bc_isothermal = 0;
  Bc_db->bc_isothermal_count = 0;
  Bc_db->bc_normal_heat_flux = 0;
  Bc_db->bc_normal_heat_flux_count = 0;
  Bc_db->bc_goldak = 0;
  Bc_db->bc_goldak_count = 0;
  Bc_db->bc_radconv = 0;
  Bc_db->bc_radconv_count =0;
  Bc_db->bc_outflow = 0;
  Bc_db->bc_outflow_count =0;
  Bc_db->bc_contact = 0;
  Bc_db->bc_contact_count = 0;
  Bc_db->bc_set_bcnum = 0;
  Bc_db->bc_set_bcnum_count = 0;
  return 0;
}

/*------------------------------------------------------------
pdr_heat_bc_free
------------------------------------------------------------*/
int pdr_heat_bc_free(pdt_heat_bc *Bc_db)
{
  if(Bc_db->bc_assignments != 0)
    free(Bc_db->bc_assignments);
  if(Bc_db->bc_isothermal != 0)
    free(Bc_db->bc_isothermal);
  if(Bc_db->bc_normal_heat_flux != 0)
    free(Bc_db->bc_normal_heat_flux);
  if(Bc_db->bc_goldak != 0)
    free(Bc_db->bc_goldak);
  if(Bc_db->bc_radconv != 0)
    free(Bc_db->bc_radconv);  
   if(Bc_db->bc_outflow != 0)
    free(Bc_db->bc_outflow); 
   if(Bc_db->bc_set_bcnum != 0)
    free(Bc_db->bc_set_bcnum); 
   if (Bc_db->bc_contact != NULL)
	   free(Bc_db->bc_contact);
  pdr_heat_bc_config_zeroall(Bc_db);
  
  return 0;
}

/*------------------------------------------------------------
pdr_heat_bc_config_alloc
------------------------------------------------------------*/
int pdr_heat_bc_config_alloc(const config_t *Cfg, pdt_heat_bc *Bc_db)
{
  int i;
  config_setting_t *root_setting;
  
  root_setting = config_lookup(Cfg, "bc");
  if(root_setting != NULL)
  {
    int count = config_setting_length(root_setting);
    if(count > 0)
      Bc_db->bc_assignments = (pdt_heat_bc_assignments*) malloc(sizeof(pdt_heat_bc_assignments)*count);
    Bc_db->bc_assignments_count = count;
  }  
  
  for(i=0; i<Bc_db->bc_assignments_count; ++i)
  {
    config_setting_t *bc_sett = config_setting_get_elem(root_setting, i);
    if(NULL != config_setting_get_member(bc_sett, "isothermal")) ++(Bc_db->bc_isothermal_count);
    if(NULL != config_setting_get_member(bc_sett, "normal_heat_flux")) ++(Bc_db->bc_normal_heat_flux_count);
    if(NULL != config_setting_get_member(bc_sett, "goldak_heat_source")) ++(Bc_db->bc_goldak_count);    
    if(NULL != config_setting_get_member(bc_sett, "radconv")) ++(Bc_db->bc_radconv_count);
	if(NULL != config_setting_get_member(bc_sett, "outflow")) ++(Bc_db->bc_outflow_count);
	if(NULL != config_setting_get_member(bc_sett, "contact")) ++(Bc_db->bc_contact_count);
	if(NULL != config_setting_get_member(bc_sett, "set_bcnum")) ++(Bc_db->bc_contact_count);
  }
  
  
  if(Bc_db->bc_isothermal_count > 0)
    Bc_db->bc_isothermal = (pdt_heat_bc_isothermal*)malloc(sizeof(pdt_heat_bc_isothermal)                   
								 * Bc_db->bc_isothermal_count);
  
  if(Bc_db->bc_normal_heat_flux_count > 0)
    Bc_db->bc_normal_heat_flux = (pdt_heat_bc_normal_flux*)malloc(sizeof(pdt_heat_bc_normal_flux)       
								  * Bc_db->bc_normal_heat_flux_count);
  
  if(Bc_db->bc_goldak_count > 0)
    Bc_db->bc_goldak = (pdt_heat_bc_goldak_source*)malloc(sizeof(pdt_heat_bc_goldak_source)     
								  * Bc_db->bc_goldak_count);
  
  if(Bc_db->bc_radconv_count > 0)
    Bc_db->bc_radconv = (pdt_heat_bc_radconv*)malloc(sizeof(pdt_heat_bc_radconv)                          
							     * Bc_db->bc_radconv_count);  

  if(Bc_db->bc_outflow_count > 0)
    Bc_db->bc_outflow = (pdt_heat_bc_outflow*)malloc(sizeof(pdt_heat_bc_outflow)                          
							     * Bc_db->bc_outflow_count); 

  if (Bc_db->bc_contact_count > 0)
	  Bc_db->bc_contact = (pdt_heat_bc_contact*)malloc(sizeof(pdt_heat_bc_contact)
	  * Bc_db->bc_contact_count);

  if (Bc_db->bc_set_bcnum_count > 0)
	  Bc_db->bc_set_bcnum = (pdt_heat_bc_set_bcnum*)malloc(sizeof(pdt_heat_bc_set_bcnum)
	  * Bc_db->bc_set_bcnum_count);
	  

  return 0;
}

/*------------------------------------------------------------
pdr_heat_bc_read - read bc data from config file
------------------------------------------------------------*/
int pdr_heat_bc_read(char *Work_dir, char *Filename, FILE *Interactive_output, pdt_heat_bc *Bc_db)
{
  int i,j;
  config_t cfg;
  config_setting_t *root_setting;
  config_setting_t *setting;
  config_setting_t *bc_sett;
  config_setting_t *alfa_bc;
  const char *str;
  char bc_file[300];
  int isothermal_pos = 0;
  int normal_heat_flux_pos = 0;
  int goldak_heat_source_pos = 0;
  int radconv_pos = 0;
  int outflow_pos = 0;
  int contact_pos = 0;
  int set_bcnum_pos = 0;
  

  pdr_heat_bc_config_zeroall(Bc_db);
  
  sprintf(bc_file, "%s/%s", Work_dir, Filename);

  //printf("reading bc for heat from file %s\n", bc_file);

  config_init(&cfg);

  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, bc_file))
  {
//    fprintf(Interactive_output, "Boundary conditions file error: %s:%d - %s\n", config_error_file(&cfg),
//            config_error_line(&cfg), config_error_text(&cfg));
//    config_destroy(&cfg);
    return(EXIT_FAILURE);
  }
  
  pdr_heat_bc_config_alloc(&cfg, Bc_db);
  
  
  root_setting = config_lookup(&cfg, "bc");
  for(i=0; i<Bc_db->bc_assignments_count; ++i)
  {
    
    //printf("reading bc %d for heat\n", i);

    setting = config_setting_get_elem(root_setting, i);
    //pdt_heat_bc_assignments *bcass = bc_db->bc_assignments[i];
    Bc_db->bc_assignments[i].bnum = config_setting_get_int(config_setting_get_member(setting, "bcnum"));
    
    Bc_db->bc_assignments[i].bc_heat = BC_HEAT_NONE;
    Bc_db->bc_assignments[i].bc_heat_data_idx = -1;
    
    if(NULL != (bc_sett = config_setting_get_member(setting, "symmetry"))) 
    {
      Bc_db->bc_assignments[i].bc_heat = BC_HEAT_SYMMETRY;
    }
    else if(NULL != (bc_sett = config_setting_get_member(setting, "outflow"))) 
    {
      Bc_db->bc_assignments[i].bc_heat = BC_HEAT_OUTFLOW;
	  Bc_db->bc_assignments[i].bc_heat_data_idx = outflow_pos;
	  Bc_db->bc_isothermal[outflow_pos].bnum = Bc_db->bc_assignments[i].bnum;
	  ++outflow_pos;
    }      
	else if (NULL != (bc_sett = config_setting_get_member(setting, "isothermal")))
    {
      Bc_db->bc_assignments[i].bc_heat = BC_HEAT_ISOTHERMAL;
      Bc_db->bc_assignments[i].bc_heat_data_idx = isothermal_pos;
      Bc_db->bc_isothermal[isothermal_pos].bnum = Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_isothermal[isothermal_pos].temp = config_setting_get_float(config_setting_get_member(bc_sett, "temp"));
      ++isothermal_pos;
    }
	else if (NULL != (bc_sett = config_setting_get_member(setting, "normal_heat_flux")))
    {
      Bc_db->bc_assignments[i].bc_heat = BC_HEAT_NORMAL_HEAT_FLUX;
      Bc_db->bc_assignments[i].bc_heat_data_idx = normal_heat_flux_pos;
      Bc_db->bc_normal_heat_flux[normal_heat_flux_pos].bnum = Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_normal_heat_flux[normal_heat_flux_pos].flux = config_setting_get_float(config_setting_get_member(bc_sett, "flux"));
      ++normal_heat_flux_pos;
    }    
	else if (NULL != (bc_sett = config_setting_get_member(setting, "goldak_heat_source")))
    {
      Bc_db->bc_assignments[i].bc_heat = BC_HEAT_GOLDAK_HEAT_SOURCE;
      Bc_db->bc_assignments[i].bc_heat_data_idx = goldak_heat_source_pos;
      
      Bc_db->bc_goldak[goldak_heat_source_pos].bnum = Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_goldak[goldak_heat_source_pos].Q =  config_setting_get_float(config_setting_get_member(bc_sett, "Q"));
      Bc_db->bc_goldak[goldak_heat_source_pos].ff = config_setting_get_float(config_setting_get_member(bc_sett, "ff"));
      Bc_db->bc_goldak[goldak_heat_source_pos].fr = config_setting_get_float(config_setting_get_member(bc_sett, "fr"));
      Bc_db->bc_goldak[goldak_heat_source_pos].a =  config_setting_get_float(config_setting_get_member(bc_sett, "a"));
      Bc_db->bc_goldak[goldak_heat_source_pos].b =  config_setting_get_float(config_setting_get_member(bc_sett, "b"));
      Bc_db->bc_goldak[goldak_heat_source_pos].c1 = config_setting_get_float(config_setting_get_member(bc_sett, "c1"));
      Bc_db->bc_goldak[goldak_heat_source_pos].c2 = config_setting_get_float(config_setting_get_member(bc_sett, "c2"));
      Bc_db->bc_goldak[goldak_heat_source_pos].h = config_setting_get_float(config_setting_get_member(bc_sett, "h"));
      Bc_db->bc_goldak[goldak_heat_source_pos].eps = config_setting_get_float(config_setting_get_member(bc_sett, "eps"));
      
      for(j=0; j<3; ++j)
      {
        Bc_db->bc_goldak[goldak_heat_source_pos].v[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "v"), j);
        Bc_db->bc_goldak[goldak_heat_source_pos].init_pos[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "init_pos"), j);
        Bc_db->bc_goldak[goldak_heat_source_pos].versor[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "versor"), j);
        //set current position to initial position
        Bc_db->bc_goldak[goldak_heat_source_pos].current_pos[j] = Bc_db->bc_goldak[goldak_heat_source_pos].init_pos[j];
      }      
      
      ++goldak_heat_source_pos;
    }  
	else if (NULL != (bc_sett = config_setting_get_member(setting, "radconv")))
    {
		
      Bc_db->bc_assignments[i].bc_heat = BC_HEAT_RADCONV;
      Bc_db->bc_assignments[i].bc_heat_data_idx = radconv_pos;
      
      Bc_db->bc_radconv[radconv_pos].bnum = Bc_db->bc_assignments[i].bnum;
	  Bc_db->bc_radconv[radconv_pos].t_inf = 0.0;
	  
      if(NULL != config_setting_get_member(bc_sett, "t_inf")){Bc_db->bc_radconv[radconv_pos].t_inf =   config_setting_get_float(config_setting_get_member(bc_sett, "t_inf"));}
	  Bc_db->bc_radconv[radconv_pos].eps = 0.0;
      if(NULL != config_setting_get_member(bc_sett, "eps")){Bc_db->bc_radconv[radconv_pos].eps = config_setting_get_float(config_setting_get_member(bc_sett, "eps"));}
	  Bc_db->bc_radconv[radconv_pos].alfa = 0.0;
	  
	  const double default_alfa = 10e6;
	  
	  if (NULL != (alfa_bc = config_setting_get_member(bc_sett, "alfaFor"))){ 
			int setting_length = config_setting_length(alfa_bc),j=0;
		
			Bc_db->bc_radconv[radconv_pos].setting_length = setting_length;
				
			Bc_db->bc_radconv[radconv_pos].alfaFor =
		  	(double*) malloc(sizeof(double)*setting_length);
			Bc_db->bc_radconv[radconv_pos].alfaTemp =
		  	(double*) malloc(sizeof(double)*setting_length);
				
			
				
			for(j = 0; j < setting_length; ++j)
		  	{

		   		Bc_db->bc_radconv[radconv_pos].alfaTemp[j] =
		      		(double)config_setting_get_float_elem(config_setting_get_elem(alfa_bc, j), 0);
				Bc_db->bc_radconv[radconv_pos].alfaFor[j] =
		      		(double)config_setting_get_float_elem(config_setting_get_elem(alfa_bc, j), 1);
					
		  	}
			
		} 
		else if(NULL != config_setting_get_member(bc_sett, "alfa")){
			
			Bc_db->bc_radconv[radconv_pos].alfa = (double)config_setting_get_float(config_setting_get_member(bc_sett, "alfa"));
			Bc_db->bc_radconv[radconv_pos].setting_length = 1;
			}
		else {
			Bc_db->bc_radconv[radconv_pos].alfa = default_alfa;
			Bc_db->bc_radconv[radconv_pos].setting_length = 1;
			mf_log_warn("'contact' boundary condition wtih no 'alfa' parameter found. Assuming alfa=%lf", default_alfa);
		}
	  
      
      ++radconv_pos;
    }
	else if (NULL != (bc_sett = config_setting_get_member(setting, "contact")))
	{
		
		Bc_db->bc_assignments[i].bc_heat = BC_HEAT_CONTACT;
		Bc_db->bc_assignments[i].bc_heat_data_idx = contact_pos;

		Bc_db->bc_contact[contact_pos].bnum = Bc_db->bc_assignments[i].bnum;

		const double default_alfa = 10e6;

		//TODO: flag: materialID vs groupID

		if (NULL != config_setting_get_member(bc_sett, "ID1")){
            Bc_db->bc_contact[contact_pos].IDs[0] = config_setting_get_int(config_setting_get_member(bc_sett, "ID1"));
		}
		else {
			mf_fatal_err("'contact' boundary condition wtih no 'ID1' parameter found.");
		}


		if (NULL != config_setting_get_member(bc_sett, "ID2")){
            Bc_db->bc_contact[contact_pos].IDs[1] = config_setting_get_int(config_setting_get_member(bc_sett, "ID2"));
		}
		else {
			mf_fatal_err("'contact' boundary condition wtih no 'ID2' parameter found.");
		}

		
		if (NULL != (alfa_bc = config_setting_get_member(bc_sett, "alfaFor"))){ 
			int setting_length = config_setting_length(alfa_bc),j=0;
			
				Bc_db->bc_contact[contact_pos].setting_length = setting_length;
				
				Bc_db->bc_contact[contact_pos].alfaFor =
		  		(double*) malloc(sizeof(double)*setting_length);
				Bc_db->bc_contact[contact_pos].alfaTemp =
		  		(double*) malloc(sizeof(double)*setting_length);
				
			
				
			for(j = 0; j < setting_length; ++j)
		  	{

		   		Bc_db->bc_contact[contact_pos].alfaTemp[j] =
		      		(double)config_setting_get_float_elem(config_setting_get_elem(alfa_bc, j), 0);
				Bc_db->bc_contact[contact_pos].alfaFor[j] =
		      		(double)config_setting_get_float_elem(config_setting_get_elem(alfa_bc, j), 1);
					
		  	}
			
		} 
		else if(NULL != config_setting_get_member(bc_sett, "alfa")){
			Bc_db->bc_contact[contact_pos].alfa = (double)config_setting_get_float(config_setting_get_member(bc_sett, "alfa"));
			Bc_db->bc_contact[contact_pos].setting_length = 1;
			}
		else {
			Bc_db->bc_contact[contact_pos].alfa = default_alfa;
			Bc_db->bc_contact[contact_pos].setting_length = 1;
			mf_log_warn("'contact' boundary condition wtih no 'alfa' parameter found. Assuming alfa=%lf", default_alfa);
		}

        Bc_db->bc_contact[contact_pos].type = UTE_MESH_BC_GROUP;
        if (NULL != config_setting_get_member(bc_sett, "concerns")){
            if(0 == strcmp("material",config_setting_get_string(config_setting_get_member(bc_sett, "concerns")) )) {
                Bc_db->bc_contact[contact_pos].type = UTE_MESH_BC_MATERIAL;
            }
            else if(0 == strcmp("block",config_setting_get_string(config_setting_get_member(bc_sett, "concerns")) )) {
                Bc_db->bc_contact[contact_pos].type = UTE_MESH_BC_BLOCK;
            }
            else if(0 != strcmp("group",config_setting_get_string(config_setting_get_member(bc_sett, "concerns")) )) {
                mf_log_err("'contact' boundary condition wtih invalid 'type' parameter found (%s). Assuming IDs are referencing to group IDs.",
                           config_setting_get_string(config_setting_get_member(bc_sett, "concerns")));
            }
        }
        else {
            mf_log_warn("'contact' boundary condition wtih no 'type' parameter found. Assuming IDs are referencing to group IDs");
        }

		++contact_pos;
	}
	else if (NULL != (bc_sett = config_setting_get_member(setting, "set_bcnum")))
	{
		//Bc_db->bc_assignments[i].bc_heat = BC_HEAT_CONTACT;
		Bc_db->bc_assignments[i].bc_heat_data_idx = set_bcnum_pos;

		Bc_db->bc_contact[set_bcnum_pos].bnum = Bc_db->bc_assignments[i].bnum;

		const double default_alfa = 10e6;

		//TODO: flag: materialID vs groupID

		if (NULL != config_setting_get_member(bc_sett, "ID1")){
            Bc_db->bc_contact[set_bcnum_pos].IDs[0] = config_setting_get_int(config_setting_get_member(bc_sett, "ID1"));
		}
		else {
			mf_fatal_err("'contact' boundary condition wtih no 'ID1' parameter found.");
		}


		if (NULL != config_setting_get_member(bc_sett, "ID2")){
            Bc_db->bc_contact[set_bcnum_pos].IDs[1] = config_setting_get_int(config_setting_get_member(bc_sett, "ID2"));
		}
		else {
			mf_fatal_err("'contact' boundary condition wtih no 'ID2' parameter found.");
		}


        Bc_db->bc_contact[set_bcnum_pos].type = UTE_MESH_BC_GROUP;
        if (NULL != config_setting_get_member(bc_sett, "concerns")){
            if(0 == strcmp("material",config_setting_get_string(config_setting_get_member(bc_sett, "concerns")) )) {
                Bc_db->bc_contact[set_bcnum_pos].type = UTE_MESH_BC_MATERIAL;
            }
            else if(0 == strcmp("block",config_setting_get_string(config_setting_get_member(bc_sett, "concerns")) )) {
                Bc_db->bc_contact[set_bcnum_pos].type = UTE_MESH_BC_BLOCK;
            }
            else if(0 != strcmp("group",config_setting_get_string(config_setting_get_member(bc_sett, "concerns")) )) {
                mf_log_err("'contact' boundary condition wtih invalid 'type' parameter found (%s). Assuming IDs are referencing to group IDs.",
                           config_setting_get_string(config_setting_get_member(bc_sett, "concerns")));
            }
        }
        else {
            mf_log_warn("'set_bnum' boundary condition wtih no 'type' parameter found. Assuming IDs are referencing to group IDs");
        }

		++set_bcnum_pos;
	}

  }
    

  config_destroy(&cfg);
  return(EXIT_SUCCESS);
}
