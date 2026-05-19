/************************************************************************
File pds_ns_supg_bc_io.c - definition of functions related to boundary conditions
			   reading and management.

Contains definition of routines:
  pdr_ns_supg_bc_read - read bc data from config file
  pdr_ns_supg_bc_free - free bc resources
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
#include "../include/pdh_ns_supg_bc.h" /* IMPLEMENTS */


/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
   /* - name always begins with pdr_ns_supg_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
  pdr_ns_supg_bc_config_zeroall
  ------------------------------------------------------------*/
int pdr_ns_supg_bc_config_zeroall(pdt_ns_supg_bc *Bc_db)
{
  Bc_db->bc_assignments = 0;
  Bc_db->bc_assignments_count = 0;
  Bc_db->bc_inflow_circle_3d = 0;
  Bc_db->bc_inflow_circle_3d_count = 0;
  Bc_db->bc_inflow_rect_2d = 0;
  Bc_db->bc_inflow_rect_2d_count = 0;
  Bc_db->bc_inflow_linear_2d = 0;
  Bc_db->bc_inflow_linear_2d_count = 0;
  Bc_db->bc_velocity = 0;
  Bc_db->bc_velocity_count = 0;
  Bc_db->pin_pressure = 0;
  Bc_db->pin_pressure_count = 0;
  Bc_db->pin_velocity = 0;
  Bc_db->pin_velocity_count = 0;
  Bc_db->bc_marangoni = 0;
  Bc_db->bc_marangoni_count = 0;
  return 0;
}

/*------------------------------------------------------------
  pdr_ns_supg_bc_free
  ------------------------------------------------------------*/
int pdr_ns_supg_bc_free(pdt_ns_supg_bc *Bc_db)
{
  if(Bc_db->bc_assignments != 0) free(Bc_db->bc_assignments);
  if(Bc_db->bc_inflow_circle_3d != 0) free(Bc_db->bc_inflow_circle_3d);
  if(Bc_db->bc_inflow_rect_2d != 0) free(Bc_db->bc_inflow_rect_2d);
  if(Bc_db->bc_inflow_linear_2d != 0) free(Bc_db->bc_inflow_rect_2d);
  if(Bc_db->bc_velocity != 0) free(Bc_db->bc_velocity);
  if(Bc_db->pin_pressure != 0) free(Bc_db->pin_pressure);
  if(Bc_db->pin_velocity != 0) free(Bc_db->pin_velocity);
  if(Bc_db->bc_marangoni != 0) free(Bc_db->bc_marangoni);
  
  pdr_ns_supg_bc_config_zeroall(Bc_db);
  
  return 0;
}

/*------------------------------------------------------------
  pdr_ns_supg_bc_config_alloc
------------------------------------------------------------*/
int pdr_ns_supg_bc_config_alloc(const config_t *Cfg, pdt_ns_supg_bc *Bc_db)
{
  int i;
  config_setting_t *root_setting;
  root_setting = config_lookup(Cfg, "pressure_pins");
  if(root_setting != NULL)
    {
      int count = config_setting_length(root_setting);
      if(count > 0)
	Bc_db->pin_pressure = (pdt_ns_supg_pin_pressure*) 
                               malloc(sizeof(pdt_ns_supg_pin_pressure)*count);
      Bc_db->pin_pressure_count = count;
    }
  root_setting = config_lookup(Cfg, "velocity_pins");
  if(root_setting != NULL)
    {
      int count = config_setting_length(root_setting);
      if(count > 0)
	Bc_db->pin_velocity = (pdt_ns_supg_pin_velocity*) 
	                        malloc(sizeof(pdt_ns_supg_pin_velocity)*count);
      Bc_db->pin_velocity_count = count;
    }  
  
  root_setting = config_lookup(Cfg, "bc");
  if(root_setting != NULL)
    {
      int count = config_setting_length(root_setting);
      if(count > 0)
	Bc_db->bc_assignments = (pdt_ns_supg_bc_assignments*) 
                               malloc(sizeof(pdt_ns_supg_bc_assignments)*count);
      Bc_db->bc_assignments_count = count;
    }  
  
  for(i=0; i<Bc_db->bc_assignments_count; ++i)
    {
      config_setting_t *bc_sett = config_setting_get_elem(root_setting, i);
      if(NULL != config_setting_get_member(bc_sett, "inflow_rect_2d")) 
	                                    ++(Bc_db->bc_inflow_rect_2d_count);
	  if(NULL != config_setting_get_member(bc_sett, "inflow_linear_2d")) 
	                                    ++(Bc_db->bc_inflow_linear_2d_count);
      if(NULL != config_setting_get_member(bc_sett, "inflow_circle_3d")) 
	                                    ++(Bc_db->bc_inflow_circle_3d_count);
      if(NULL != config_setting_get_member(bc_sett, "velocity")) 
                                            ++(Bc_db->bc_velocity_count);
      if(NULL != config_setting_get_member(bc_sett, "outflow")) 
                                            ++(Bc_db->bc_outflow_count);
      if(NULL != config_setting_get_member(bc_sett, "marangoni"))
	                                    ++(Bc_db->bc_marangoni_count);

    }
  
  if(Bc_db->bc_inflow_rect_2d_count > 0)
    Bc_db->bc_inflow_rect_2d = 
     (pdt_ns_supg_bc_inflow_rect_2d*)malloc(sizeof(pdt_ns_supg_bc_inflow_rect_2d)
					    * Bc_db->bc_inflow_rect_2d_count);
						
  if(Bc_db->bc_inflow_linear_2d_count > 0)
    Bc_db->bc_inflow_linear_2d = 
     (pdt_ns_supg_bc_inflow_linear_2d*)malloc(sizeof(pdt_ns_supg_bc_inflow_linear_2d)
					    * Bc_db->bc_inflow_linear_2d_count);
						
  if(Bc_db->bc_inflow_circle_3d_count > 0)
    Bc_db->bc_inflow_circle_3d = 
      (pdt_ns_supg_bc_inflow_circle_3d*)malloc(sizeof(pdt_ns_supg_bc_inflow_circle_3d)
					     * Bc_db->bc_inflow_circle_3d_count);
  
  if(Bc_db->bc_velocity_count > 0)
    Bc_db->bc_velocity = 
      (pdt_ns_supg_bc_velocity*)malloc(sizeof(pdt_ns_supg_bc_velocity)
				                    * Bc_db->bc_velocity_count);
  
  if(Bc_db->bc_outflow_count > 0)
    Bc_db->bc_outflow = 
      (pdt_ns_supg_bc_outflow*)malloc(sizeof(pdt_ns_supg_bc_outflow)
				                    * Bc_db->bc_outflow_count);
  
  if(Bc_db->bc_marangoni_count > 0)
    Bc_db->bc_marangoni = 
      (pdt_ns_supg_bc_marangoni*)malloc(sizeof(pdt_ns_supg_bc_marangoni)
					           * Bc_db->bc_marangoni_count);
  
  return 0;
}

/*------------------------------------------------------------
pdr_ns_supg_bc_read - read bc data from config file
------------------------------------------------------------*/
int pdr_ns_supg_bc_read(
  char *Work_dir, 
  char *Filename, 
  FILE *Interactive_output, 
  pdt_ns_supg_bc *Bc_db)
{
  int i,j;
  config_t cfg;
  config_setting_t *root_setting;
  config_setting_t *setting;
  config_setting_t *bc_sett;
  const char *str;
  char bc_file[300];
  int inflow_rect_2d_pos = 0;
  int inflow_linear_2d_pos = 0;
  int inflow_circle_3d_pos = 0;
  int outflow_pos = 0;
  int velocity_pos = 0;
  int marangoni_pos = 0;
  
  pdr_ns_supg_bc_config_zeroall(Bc_db);
  
  sprintf(bc_file, "%s/%s", Work_dir, Filename);
  
  config_init(&cfg);
  
  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, bc_file)) {
    //fprintf(Interactive_output, "Boundary conditions file error: %s:%d - %s\n", 
//	    config_error_file(&cfg),
//	    config_error_line(&cfg), config_error_text(&cfg));
      config_destroy(&cfg);
      return(EXIT_FAILURE);
    }
  
  pdr_ns_supg_bc_config_alloc(&cfg, Bc_db);
  
  root_setting = config_lookup(&cfg, "pressure_pins");
  for(i=0; i<Bc_db->pin_pressure_count; ++i) {
    setting = config_setting_get_elem(root_setting, i);
    Bc_db->pin_pressure[i].p = config_setting_get_float(
				       config_setting_get_member(setting, "p"));
    for(j=0; j<3; ++j) Bc_db->pin_pressure[i].pin_node_coor[j] = 
			 config_setting_get_float_elem(
                            config_setting_get_member(setting, "node_coor"), j);
/*kbw*/
    printf("read pressure pin %d with value %lf\n", i,
	   Bc_db->pin_pressure[i].p);
    printf("node_coor: %lf, %lf, %lf\n",
	   Bc_db->pin_pressure[i].pin_node_coor[0],
	   Bc_db->pin_pressure[i].pin_node_coor[1],
	   Bc_db->pin_pressure[i].pin_node_coor[2]);
/*kew*/

  }
  
  root_setting = config_lookup(&cfg, "velocity_pins");
  for(i=0; i<Bc_db->pin_velocity_count; ++i) {
    setting = config_setting_get_elem(root_setting, i);
    for(j=0; j<3; ++j) {
      Bc_db->pin_velocity[i].v[j] = config_setting_get_float_elem(
                                    config_setting_get_member(setting, "v"), j);
      Bc_db->pin_velocity[i].pin_node_coor[j] = config_setting_get_float_elem(
                            config_setting_get_member(setting, "node_coor"), j);
    }
/*kbw*/
    printf("read velocity pin %d with values %lf %lf %lf\n", i,
	   Bc_db->pin_velocity[i].v[0],Bc_db->pin_velocity[i].v[1],
	   Bc_db->pin_velocity[i].v[2]);
    printf("node_coor: %lf, %lf, %lf\n",
	   Bc_db->pin_velocity[i].pin_node_coor[0],
	   Bc_db->pin_velocity[i].pin_node_coor[1],
	   Bc_db->pin_velocity[i].pin_node_coor[2]);
/*kew*/
  }  
  
  
  root_setting = config_lookup(&cfg, "bc");
  for(i=0; i<Bc_db->bc_assignments_count; ++i) {
    
    setting = config_setting_get_elem(root_setting, i);
    //pdt_ns_supg_bc_assignments *bcass = bc_db->bc_assignments[i];
    Bc_db->bc_assignments[i].bnum = (int) config_setting_get_int(
                                  config_setting_get_member(setting, "bcnum"));
    
    Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_NONE;
    Bc_db->bc_assignments[i].bc_ns_supg_data_idx = -1;
    
    if(NULL != (bc_sett = config_setting_get_member(setting, "inflow_rect_2d"))){
      Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_INFLOW_RECT2D;
      Bc_db->bc_assignments[i].bc_ns_supg_data_idx = inflow_rect_2d_pos;
      Bc_db->bc_inflow_rect_2d[inflow_rect_2d_pos].bnum = 
	                                          Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_inflow_rect_2d[inflow_rect_2d_pos].v = 
	       config_setting_get_float(config_setting_get_member(bc_sett, "v"));
      for(j=0; j<3; ++j) {
	Bc_db->bc_inflow_rect_2d[inflow_rect_2d_pos].n1[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "n1"), j);
	Bc_db->bc_inflow_rect_2d[inflow_rect_2d_pos].n2[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "n2"), j);
	Bc_db->bc_inflow_rect_2d[inflow_rect_2d_pos].n3[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "n3"), j);
	Bc_db->bc_inflow_rect_2d[inflow_rect_2d_pos].n4[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "n4"), j);
      }
      ++inflow_rect_2d_pos;
    }  
    if(NULL != (bc_sett = config_setting_get_member(setting, "inflow_linear_2d"))){
      Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_INFLOW_LINEAR2D;
      Bc_db->bc_assignments[i].bc_ns_supg_data_idx = inflow_linear_2d_pos;
      Bc_db->bc_inflow_linear_2d[inflow_linear_2d_pos].bnum = 
	                                          Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_inflow_linear_2d[inflow_linear_2d_pos].v = 
	       config_setting_get_float(config_setting_get_member(bc_sett, "v"));
      for(j=0; j<3; ++j) {
	Bc_db->bc_inflow_linear_2d[inflow_linear_2d_pos].n1[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "n1"), j);
	Bc_db->bc_inflow_linear_2d[inflow_linear_2d_pos].n2[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "n2"), j);
      }
      ++inflow_linear_2d_pos;
    }  
    if(NULL!=(bc_sett = config_setting_get_member(setting,"inflow_circle_3d"))){
      Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_INFLOW_CIRCLE3D;
      Bc_db->bc_assignments[i].bc_ns_supg_data_idx = inflow_circle_3d_pos;
      Bc_db->bc_inflow_circle_3d[inflow_circle_3d_pos].bnum = 
                                                   Bc_db->bc_assignments[i].bnum;
      Bc_db->bc_inflow_circle_3d[inflow_circle_3d_pos].v = 
               config_setting_get_float(config_setting_get_member(bc_sett, "v"));
      Bc_db->bc_inflow_circle_3d[inflow_circle_3d_pos].radius = 
          config_setting_get_float(config_setting_get_member(bc_sett, "radius"));
      for(j=0; j<3; ++j){
	Bc_db->bc_inflow_circle_3d[inflow_circle_3d_pos].center[j] = 
  config_setting_get_float_elem(config_setting_get_member(bc_sett, "center"), j);
      }
      ++inflow_circle_3d_pos;
    }
    if(NULL != (bc_sett = config_setting_get_member(setting, "velocity"))) {
	  int orient=0;
	  Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_VELOCITY;
      Bc_db->bc_assignments[i].bc_ns_supg_data_idx = velocity_pos;
      Bc_db->bc_velocity[velocity_pos].bnum = Bc_db->bc_assignments[i].bnum;
      for(j=0; j<3; ++j)
		{
	  Bc_db->bc_velocity[velocity_pos].v[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "v"), j);
		}
	  config_setting_lookup_int(bc_sett,"vel_orient",&orient);
	  Bc_db->bc_velocity[velocity_pos].vel_orient =  orient;
	
	  /*kbw*/
    printf("read velocity bc %d with values %lf %lf %lf orient %d \n", velocity_pos,
	   Bc_db->bc_velocity[velocity_pos].v[0],
	   Bc_db->bc_velocity[velocity_pos].v[1],
		   Bc_db->bc_velocity[velocity_pos].v[2],
		   Bc_db->bc_velocity[velocity_pos].vel_orient
		   );
/*kew*/
      ++velocity_pos;
    }
    if(NULL != (bc_sett = config_setting_get_member(setting, "symmetry"))) {
      //printf("read symmetry bc!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_SYMMETRY;
    }
    if(NULL != (bc_sett = config_setting_get_member(setting, "outflow"))) {
      Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_OUTFLOW;
      Bc_db->bc_assignments[i].bc_ns_supg_data_idx = outflow_pos;
      Bc_db->bc_outflow[outflow_pos].bnum = Bc_db->bc_assignments[i].bnum;
      config_setting_t *set = config_setting_get_member(bc_sett, "pressure");
      Bc_db->bc_outflow[outflow_pos].pressure = 
	                                 (double)config_setting_get_float(set);
      outflow_pos++; 

    }
    if(NULL != (bc_sett = config_setting_get_member(setting, "noslip"))) {
      Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_NOSLIP;
    }
      
    if(NULL != (bc_sett = config_setting_get_member(setting, "marangoni"))) {
      Bc_db->bc_assignments[i].bc_ns_supg = BC_NS_SUPG_MARANGONI;
      Bc_db->bc_assignments[i].bc_ns_supg_data_idx = marangoni_pos;
      Bc_db->bc_marangoni[marangoni_pos].bnum = Bc_db->bc_assignments[i].bnum;
	  
      for(j=0; j<3; ++j){
	Bc_db->bc_marangoni[marangoni_pos].n[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "n"), j);
	Bc_db->bc_marangoni[marangoni_pos].node_coor[j] = config_setting_get_float_elem(config_setting_get_member(bc_sett, "node_coor"), j);
	//printf("\n\tAS: n[%d] = %f\tx[%d] = %f", Bc_db->bc_marangoni[marangoni_pos].n[j], j, Bc_db->bc_marangoni[marangoni_pos].node_coor[j], j);
      }
      
      ++marangoni_pos;
    }
  }
  

  config_destroy(&cfg);
  return(EXIT_SUCCESS);
}
