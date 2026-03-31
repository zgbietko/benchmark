/************************************************************************
File pds_vof_bc_io.c - definition of functions related to boundary conditions 
			   read and management.

Contains definition of routines:
  pdr_vof_bc_read - read bc data from config file
  pdr_vof_bc_free - free bc resources
  *other are local to this file

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>

/* types and functions related to boundary conditions handling */
#include "../include/pdh_vof_bc.h" /* IMPLEMENTS */


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_vof_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_vof_bc_config_zeroall
------------------------------------------------------------*/
int pdr_vof_bc_config_zeroall(pdt_vof_bc *Bc_db)
{
  Bc_db->bc_assignments = 0;
  Bc_db->bc_assignments_count = 0;
  Bc_db->bc_isovof = 0;
  Bc_db->bc_isovof_count = 0;
  Bc_db->bc_normal_vof_flux = 0;
  Bc_db->bc_normal_vof_flux_count = 0;
  /* Bc_db->bc_goldak = 0; */
  /* Bc_db->bc_goldak_count = 0; */
  /* Bc_db->bc_radconv = 0; */
  /* Bc_db->bc_radconv_count =0; */
  return 0;
}

/*------------------------------------------------------------
pdr_vof_bc_free
------------------------------------------------------------*/
int pdr_vof_bc_free(pdt_vof_bc *Bc_db)
{
  if(Bc_db->bc_assignments != 0)
    free(Bc_db->bc_assignments);
  if(Bc_db->bc_isovof != 0)
    free(Bc_db->bc_isovof);
  if(Bc_db->bc_normal_vof_flux != 0)
    free(Bc_db->bc_normal_vof_flux);
  if(Bc_db->bc_vof_source != 0)
    free(Bc_db->bc_vof_source);
  /* if(Bc_db->bc_radconv != 0) */
  /*   free(Bc_db->bc_radconv);   */
  
  pdr_vof_bc_config_zeroall(Bc_db);
  
  return 0;
}

/*------------------------------------------------------------
pdr_vof_bc_config_alloc
------------------------------------------------------------*/
int pdr_vof_bc_config_alloc(const config_t *Cfg, pdt_vof_bc *Bc_db)
{
  int i;
  config_setting_t *root_setting;
  
  root_setting = config_lookup(Cfg, "bc");
  if(root_setting != NULL)
  {
    int count = config_setting_length(root_setting);
    if(count > 0)
      Bc_db->bc_assignments = (pdt_vof_bc_assignments*) malloc(sizeof(pdt_vof_bc_assignments)*count);
    Bc_db->bc_assignments_count = count;
  }  
  
  for(i=0; i<Bc_db->bc_assignments_count; ++i)
  {
    config_setting_t *bc_sett = config_setting_get_elem(root_setting, i);
    if(NULL != config_setting_get_member(bc_sett, "isovof")) ++(Bc_db->bc_isovof_count);
    if(NULL != config_setting_get_member(bc_sett, "normal_vof_flux")) ++(Bc_db->bc_normal_vof_flux_count);
    if(NULL != config_setting_get_member(bc_sett, "vof_source")) ++(Bc_db->bc_vof_source_count);    
    /* if(NULL != config_setting_get_member(bc_sett, "radconv")) ++(Bc_db->bc_radconv_count); */
  }
  
  
  if(Bc_db->bc_isovof_count > 0)
    Bc_db->bc_isovof       = (pdt_vof_bc_isovof*)malloc(sizeof(pdt_vof_bc_isovof)                   * Bc_db->bc_isovof_count);
  
  if(Bc_db->bc_normal_vof_flux_count > 0)
    Bc_db->bc_normal_vof_flux = (pdt_vof_bc_normal_vof_flux*)malloc(sizeof(pdt_vof_bc_normal_vof_flux)       * Bc_db->bc_normal_vof_flux_count);
  
  if(Bc_db->bc_vof_source_count > 0)
    Bc_db->bc_vof_source         = (pdt_vof_bc_vof_source*)malloc(sizeof(pdt_vof_bc_vof_source)     * Bc_db->bc_vof_source_count);
  
  /* if(Bc_db->bc_radconv_count > 0) */
  /*   Bc_db->bc_radconv         = (pdt_heat_bc_radconv*)malloc(sizeof(pdt_heat_bc_radconv)                          * Bc_db->bc_radconv_count);   */


  return 0;
}

/*------------------------------------------------------------
pdr_vof_bc_read - read bc data from config file
------------------------------------------------------------*/
int pdr_vof_bc_read(char *Work_dir, char *Filename, FILE *Interactive_output, pdt_vof_bc *Bc_db)
{
  int i,j;
  config_t cfg;
  config_setting_t *root_setting;
  config_setting_t *setting;
  config_setting_t *bc_sett;
  const char *str;
  char bc_file[300];
  int isovof_pos = 0;
  int normal_vof_flux_pos = 0;
  int vof_source_pos = 0;
  /* int radconv_pos = 0; */
  
  pdr_vof_bc_config_zeroall(Bc_db);
  
  sprintf(bc_file, "%s/%s", Work_dir, Filename);

  config_init(&cfg);

  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, bc_file))
  {
    fprintf(Interactive_output, "Boundary conditions file error: %s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return(EXIT_FAILURE);
  }
  
  pdr_vof_bc_config_alloc(&cfg, Bc_db);
  
  
  root_setting = config_lookup(&cfg, "bc");
  for(i=0; i<Bc_db->bc_assignments_count; ++i)
  {
    
    setting = config_setting_get_elem(root_setting, i);
    //pdt_heat_bc_assignments *bcass = bc_db->bc_assignments[i];
    Bc_db->bc_assignments[i].bnum = config_setting_get_int(config_setting_get_member(setting, "bcnum"));
    
    Bc_db->bc_assignments[i].bc_vof = BC_VOF_NONE;
    Bc_db->bc_assignments[i].bc_vof_data_idx = -1;
    
    if(NULL != (bc_sett = config_setting_get_member(setting, "symmetry"))) 
    {
      Bc_db->bc_assignments[i].bc_vof = BC_VOF_SYMMETRY;
    }
      
    if(NULL != (bc_sett = config_setting_get_member(setting, "isovof"))) 
    {
      Bc_db->bc_assignments[i].bc_vof = BC_VOF_ISOVOF;
      Bc_db->bc_assignments[i].bc_vof_data_idx = isovof_pos;
      Bc_db->bc_isovof[isovof_pos].bnum = Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_isovof[isovof_pos].temp = config_setting_get_float(config_setting_get_member(bc_sett, "vof"));
      ++isovof_pos;
    }
    if(NULL != (bc_sett = config_setting_get_member(setting, "normal_vof_flux")))
    {
      Bc_db->bc_assignments[i].bc_vof = BC_VOF_NORMAL_VOF_FLUX;
      Bc_db->bc_assignments[i].bc_vof_data_idx = normal_vof_flux_pos;
      Bc_db->bc_normal_vof_flux[normal_vof_flux_pos].bnum = Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_normal_vof_flux[normal_vof_flux_pos].flux = config_setting_get_float(config_setting_get_member(bc_sett, "flux"));
      ++normal_vof_flux_pos;
    }    
    if(NULL != (bc_sett = config_setting_get_member(setting, "vof_source")))
    {
      Bc_db->bc_assignments[i].bc_vof = BC_VOF_SOURCE;
      Bc_db->bc_assignments[i].bc_vof_data_idx = vof_source_pos;
      
      /* Bc_db->bc_goldak[goldak_heat_source_pos].bnum = Bc_db->bc_assignments[i].bnum; */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].Q =  config_setting_get_float(config_setting_get_member(bc_sett, "Q")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].ff = config_setting_get_float(config_setting_get_member(bc_sett, "ff")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].fr = config_setting_get_float(config_setting_get_member(bc_sett, "fr")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].a =  config_setting_get_float(config_setting_get_member(bc_sett, "a")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].b =  config_setting_get_float(config_setting_get_member(bc_sett, "b")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].c1 = config_setting_get_float(config_setting_get_member(bc_sett, "c1")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].c2 = config_setting_get_float(config_setting_get_member(bc_sett, "c2")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].h = config_setting_get_float(config_setting_get_member(bc_sett, "h")); */
      /* Bc_db->bc_goldak[goldak_heat_source_pos].eps = config_setting_get_float(config_setting_get_member(bc_sett, "eps")); */
      
      /* for(j=0; j<3; ++j) */
      /* { */
      /*   Bc_db->bc_goldak[goldak_heat_source_pos].v[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "v"), j); */
      /*   Bc_db->bc_goldak[goldak_heat_source_pos].init_pos[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "init_pos"), j); */
      /*   Bc_db->bc_goldak[goldak_heat_source_pos].versor[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "versor"), j); */
      /*   //set current position to initial position */
      /*   Bc_db->bc_goldak[goldak_heat_source_pos].current_pos[j] = Bc_db->bc_goldak[goldak_heat_source_pos].init_pos[j]; */
      /* }       */
      
      ++vof_source_pos;
    }  
    /* if(NULL != (bc_sett = config_setting_get_member(setting, "radconv"))) */
    /* { */
    /*   Bc_db->bc_assignments[i].bc_heat = BC_HEAT_RADCONV; */
    /*   Bc_db->bc_assignments[i].bc_heat_data_idx = radconv_pos; */
      
    /*   Bc_db->bc_radconv[radconv_pos].bnum = Bc_db->bc_assignments[i].bnum; */
    /*   Bc_db->bc_radconv[radconv_pos].h =   config_setting_get_float(config_setting_get_member(bc_sett, "h")); */
    /*   Bc_db->bc_radconv[radconv_pos].eps = config_setting_get_float(config_setting_get_member(bc_sett, "eps")); */
      
    /*   ++radconv_pos; */
    /* }       */

  }
    

  config_destroy(&cfg);
  return(EXIT_SUCCESS);
}
