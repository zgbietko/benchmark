/************************************************************************
File pds_vof_problem_io.c - problem data read

Contains definition of routines:
  pdr_vof_problem_read - read problem data
  pdr_vof_problem_clear - clear problem data
  pdr_mat_problem_clear - clear problem data
  pdr_vof_problem_voronoi - create Voronoi diagram for cell nodes

------------------------------
History:
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <vector>
#include "voro++.hh"

/* problem module's types and functions */
#include "../include/pdh_vof_problem.h" /* USES & IMPLEMENTS */
/* interface for all mesh manipulation modules */
#include "mmh_intf.h"
/* problem module's types and functions */
#include "../../pdd_ns_supg_heat_vof/include/pdh_ns_supg_heat_vof.h"
/* from problem dependent module */
#include "pdh_control_intf.h"
/* interface for all approximation modules */
#include "aph_intf.h"

using namespace std;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_vof_ */
/* - argument names start uppercase */

/*------------------------------------------------------------
pdr_vof_problem_read - read problem data
------------------------------------------------------------*/
int pdr_vof_problem_read(
  char *Work_dir,
  char *Filename,
  FILE *Interactive_output,
  pdt_vof_problem *Problem,
  int Nr_sol // nr_sol is time integration dependent - specified by main
)
{
  config_t cfg;
  config_setting_t *root_setting;
  //const char *str;
  char problem_file[300];
  
  Problem->ctrl.nreq = PDC_VOF_NREQ;
  Problem->ctrl.nr_sol = Nr_sol;

  sprintf(problem_file, "%s/%s", Work_dir, Filename);
  fprintf(Interactive_output, "\nOpening VOF problem file %s\n",
	  problem_file);

  config_init(&cfg);

  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, problem_file))
  {
    fprintf(Interactive_output, "Problem file error: %s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return(EXIT_FAILURE);
  }
  
  root_setting = config_lookup(&cfg, "problem");
  if(root_setting != NULL)
  {
    int count = config_setting_length(root_setting);
    int i, j;
    int setting_length;
    //const char *name;
    
    if(count == 0)
    {
      fprintf(Interactive_output, "Problem file error: Problem not defined\n");
      config_destroy(&cfg);
      return(EXIT_FAILURE);
    }
    
    if(count > 1)
    {
      fprintf(Interactive_output, "Problem file error: Currently only one problem supported\n");
      config_destroy(&cfg);
      return(EXIT_FAILURE);
    }
    
    
    for(i = 0; i < count; ++i)
    {
      config_setting_t *problem_sett = config_setting_get_elem(root_setting, i);
      config_setting_t *setting;   
      const char* str;
      
/* CONTROL PARAMETERS - GENERIC: FIELDS REQUIRED FOR ALL PROBLEMS */

      setting = config_setting_get_member(problem_sett, "name");      
      Problem->ctrl.name = (int)config_setting_get_int(setting); 

      config_setting_lookup_string(problem_sett, "mesh_type", &str);
      strcpy(Problem->ctrl.mesh_type, str);
      
      config_setting_lookup_string(problem_sett, "mesh_file_in", &str);
      strcpy(Problem->ctrl.mesh_filename, str);
      
      config_setting_lookup_string(problem_sett, "field_file_in", &str);
      strcpy(Problem->ctrl.field_filename, str);
      
      config_setting_lookup_string(problem_sett, "materials_file", &str);
      strcpy(Problem->ctrl.material_filename, str);
      
      setting = config_setting_get_member(problem_sett, "materials_used");
      if(config_setting_is_list(setting) == CONFIG_TRUE)
      {
        setting_length = config_setting_length(setting);
        // Problem->ctrl.materials_used[0] -> materials used in the model
        Problem->ctrl.materials_used = (int*) malloc(sizeof(int)*(setting_length + 1));
        Problem->ctrl.materials_used[0] = setting_length;
        for(j = 0; j < setting_length; ++j)
        {
          Problem->ctrl.materials_used[j+1] = (int)config_setting_get_int_elem(setting, j);//(int)config_setting_get_int_elem(config_setting_get_elem(setting, j), 0);
        }
      }
      else
      {
        Problem->ctrl.materials_used = (int*) malloc(sizeof(int)*(1 + 1));
        Problem->ctrl.materials_used[0] = 1;
        Problem->ctrl.materials_used[1] = (int)config_setting_get_int(setting);
      }

      config_setting_lookup_string(problem_sett, "bc_file", &str);
      strcpy(Problem->ctrl.bc_filename, str);
      
      config_setting_lookup_string(problem_sett, "mesh_file_out", &str);
      strcpy(Problem->ctrl.mesh_dmp_filepattern, str);
      
      config_setting_lookup_string(problem_sett, "field_file_out", &str);
      strcpy(Problem->ctrl.field_dmp_filepattern, str);
      
/* CONTROL PARAMETERS - SPECIFIC TO NS_SUPG PROBLEMS */

      setting = config_setting_get_member(problem_sett, "penalty");
      Problem->ctrl.penalty = (double)config_setting_get_float(setting); 

    
      /* setting = config_setting_get_member(problem_sett, "reference_temperature"); */
      /* Problem->ctrl.ref_temperature =  */
      /* 	                             (double)config_setting_get_float(setting);  */
      /* setting = config_setting_get_member(problem_sett, "ambient_temperature"); */
      /* Problem->ctrl.ambient_temperature =  */
      /* 	                             (double)config_setting_get_float(setting);  */

       
/* TIME INTEGRATION PARAMETERS - GENERIC: FOR ALL TYPES OF PROBLEMS */

      setting = config_setting_get_member(problem_sett, "time_integration_type");
      Problem->time.type = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett,"implicitness_parameter");
      Problem->time.alpha = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "current_timestep");
      Problem->time.cur_step = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "current_time");
      Problem->time.cur_time = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "current_timestep_length");
      Problem->time.cur_dtime = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "previous_timestep_length");
      Problem->time.prev_dtime = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "final_timestep");
      Problem->time.final_step = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "final_time");
      Problem->time.final_time = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "time_error_type");
      Problem->time.conv_type = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "time_error_tolerance");
      Problem->time.conv_meas = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "time_monitoring_level");
      Problem->time.monitor = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "full_dump_intv");
      Problem->time.intv_dumpout = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "graph_dump_intv");
      Problem->time.intv_graph = (int)config_setting_get_int(setting); 


/* NONLINEAR SOLVER PARAMETERS - GENERIC: FOR ALL PROBLEMS */

      setting = config_setting_get_member(problem_sett, "nonlinear_solver_type");
      Problem->nonl.type = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "max_nonl_iter");
      Problem->nonl.max_iter = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "nonl_error_type");
      Problem->nonl.conv_type = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "nonl_error_tolerance");
      Problem->nonl.conv_meas = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "nonl_monitoring_level");
      Problem->nonl.monitor = (int)config_setting_get_int(setting); 

/* LINEAR SOLVER PARAMETERS - GENERIC: FOR ALL PROBLEMS */

      setting = config_setting_get_member(problem_sett, "linear_solver_type");
      Problem->lins.type = (int)config_setting_get_int(setting); 

      config_setting_lookup_string(problem_sett, "solver_file", &str);
      strcpy(Problem->ctrl.solver_filename, str);

      setting = config_setting_get_member(problem_sett, "max_linear_solver_iter");
      Problem->lins.max_iter = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "linear_solver_error_type");
      Problem->lins.conv_type = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "linear_solver_error_tolerance");
      Problem->lins.conv_meas = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "lin_solv_monitoring_level");
      Problem->lins.monitor = (int)config_setting_get_int(setting); 

/* ADAPTATION PARAMETERS - GENERIC: FOR ALL PROBLEMS */	      

      setting = config_setting_get_member(problem_sett, "adapt_type");
      Problem->adpt.type = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "adapt_interval");
      Problem->adpt.interval = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "adapt_maxgen");
      Problem->adpt.maxgen = (int)config_setting_get_int(setting); 

      setting = config_setting_get_member(problem_sett, "adapt_tolerance");
      Problem->adpt.eps = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett, "adapt_deref_ratio");
      Problem->adpt.ratio = (double)config_setting_get_float(setting); 

      setting = config_setting_get_member(problem_sett,"adapt_monitoring_level");
      Problem->adpt.monitor = (int)config_setting_get_int(setting); 


    }
    
  }

  config_destroy(&cfg);
  
  
  return(EXIT_SUCCESS);   
}

void draw_polygon(FILE *fp,vector<int> &f_vert,vector<double> &v,int j);

// This function returns a random double between 0 and 1
    double rnd() {return double(rand())/RAND_MAX;}

/*------------------------------------------------------------
pdr_vof_problem_voronoi - create Voronoi diagram for cell nodes
------------------------------------------------------------*/
int pdr_vof_problem_voronoi(pdt_vof_problem *Problem)
{
    int problem_id, field_id, mesh_id;
    int i, j, el_id, node_id, cell_id, node_id_max, node_id_min = 1, cell_id_max;
    long int k;
    //double kd;
    int nr_patches, nr_nodes;
    utt_patches * pdv_patches;
    int el_nodes[MMC_MAXELVNO+1];
    double node_coor[3 * MMC_MAXELVNO];
    const double x_min = 0.0, x_max = 0.04;
    const double y_min = -0.01, y_max = 0.01;
    const double z_min = -0.002, z_max = 0.0;
    const double x0 = 0.02, y0 = 0.0, z0 = 0.0;
    double xs_min = 1.0, xs_max = -1.0, ys_min = 1.0, ys_max = -1.0, zs_min = 1.0, zs_max = -1.0;
    int n_x, n_y, n_z;
    double x, y, z, r, r_min = 1.001;
    vector<int> neigh,f_vert;
    vector<double> v;

    printf(":::voro++ container\n");
    using namespace voro;
    voronoicell c_v;
    particle_order p_o;
    voronoicell_neighbor c_n;
    
    n_x = 46; n_y = 23; n_z = 3;
    container con(x_min, x_max, y_min, y_max, z_min, z_max, n_x, n_y, n_z, false, false, false, 8);
    //pre_container pcon(x_min, x_max, y_min, y_max, z_min, z_max, false, false, false);
    problem_id = PDC_VOF_ID;
    i = 3; field_id = pdr_ctrl_i_params(problem_id, i);
    mesh_id = apr_get_mesh_id(field_id);
    node_id_max = mmr_get_max_node_id(mesh_id);
    printf("\tNode id max = %d\n", node_id_max);
    int nodes[node_id_max];
    for(i = 1; i <= node_id_max; i++){
	nodes[i] = 0;
    }
    cell_id_max = mmr_get_max_elem_id(mesh_id);
    printf("\tCell id max = %d\n", cell_id_max);
    int cells[cell_id_max];
    for(i = 1; i <= cell_id_max; i++){
	cells[i] = 0;
    }
    //node_id_min = 1;
/*  j = 0;
    for(i = 1; i <= node_id_max; i++){
	if(mmr_node_status(mesh_id, i) == MMC_ACTIVE) {
	    mmr_node_coor(mesh_id, i, node_coor);
	    x = node_coor[0];
	    y = node_coor[1];
	    z = node_coor[2];
	    r = sqrt( (x - x0) * (x - x0) + (y - y0) * (y - y0) + (z - z0) * (z - z0) );
	    if( r <= r_min ) {
		r_min = r;
		node_id_min = i;
	    }
	}
    }
    printf("\tNode id (r_min) = %d\n", node_id_min);
*/  nr_patches = utr_create_patches_small(PDC_NS_SUPG_ID, &pdv_patches);
    printf("\tpatches = %d\n", nr_patches);
    cell_id = 53482;
    printf("\tcell_id = %d\n", cell_id);
    for( i = 1; i <= nr_patches; i++) {
	//printf("\t\tpatch = %d\telems = %d\tnodes = %d\n", i, pdv_patches[i].nr_elems, pdv_patches[i].nr_nodes);
	for( j = 0; j < pdv_patches[i].nr_elems; j++) {
	    if( pdv_patches[i].elems[j] == cell_id) 
	    {
		for( k = 0; k < pdv_patches[i].nr_nodes; k++ ) {
		    nodes[pdv_patches[i].nodes[k]] = 1;
		    printf("node %d::", pdv_patches[i].nodes[k]);
		    mmr_node_coor(mesh_id, pdv_patches[i].nodes[k], node_coor);
		    if( node_coor[0] < xs_min ) xs_min = node_coor[0];
		    if( node_coor[0] > xs_max ) xs_max = node_coor[0];
		    if( node_coor[1] < ys_min ) ys_min = node_coor[1];
		    if( node_coor[1] > ys_max ) ys_max = node_coor[1];
		    if( node_coor[2] < zs_min ) zs_min = node_coor[2];
		    if( node_coor[2] > zs_max ) zs_max = node_coor[2];
		}
		printf("\n\tpatch = %d\telems = %d\tnodes = %d\n", i, pdv_patches[i].nr_elems, pdv_patches[i].nr_nodes);
		break;
	    }
	}
    }
    printf("\t\t xs_min = %g\txs_max = %g\n", xs_min, xs_max);
    printf("\t\t ys_min = %g\tys_max = %g\n", ys_min, ys_max);
    printf("\t\t zs_min = %g\tzs_max = %g\n", zs_min, zs_max);
/*  el_id = 0;
    kd = 0.0;
    while( (el_id = mmr_get_next_act_elem(mesh_id, el_id)) != 0 ) {
	nr_nodes = mmr_el_node_coor(mesh_id, el_id, el_nodes, NULL);
	for( i = 1; i <= nr_nodes; i++ ) {
	    if( el_nodes[i] == node_id_min ) {
		cells[el_id] = 1;
		kd++;
		printf("\t\t(%6.0f) in cell = %d->", kd, el_id);
		for( j = 1; j <= nr_nodes; j++) {
		    nodes[el_nodes[j]] = 1;
		    printf("\t%d", el_nodes[j]);
		}
		printf("\n");
		break;
	    }
	}
    }
    printf("\tcells = %6.0f\n", kd);
*/
    node_id_max = mmr_get_max_node_id(mesh_id);
    printf("\tNode id max = %d\n", node_id_max);
    j = 0;
    for(i = 1; i <= node_id_max; i++){
	mmr_node_coor(mesh_id, i, node_coor);
	x = node_coor[0];
	y = node_coor[1];
	z = node_coor[2];
	if( nodes[i] == 1 ) {
	    j++;
	    con.put(p_o, i, x, y, z);
	    printf("\tnode = %d\tx = %g\ty = %g\tz = %g\n", i, x, y, z);
	} else {
	    con.put(i, x, y, z);
	}
    }
    //pcon.guess_optimal(n_x,n_y,n_z);
    //printf("\tn_x = %d\tn_y = %d\tn_z = %d\n", n_x, n_y, n_z);
    //container con(x_min, x_max, y_min, y_max, z_min, z_max, n_x, n_y, n_z, false, false, false, 8);
    //pcon.setup(con);
    printf("\tnr of nodes = %d\n", j);
    con.draw_particles("cell_points_p.gnu");
    con.draw_cells_gnuplot("cell_points_v.gnu");
    FILE *f1=safe_fopen("loops2_m.pov","w");
    FILE *f2=safe_fopen("loops2_v.pov","w");
    //c_loop_subset cls(con);
    //cls.setup_box(xs_min, xs_max, ys_min, ys_max, zs_min, zs_max, false);
    c_loop_order clo(con,p_o);
    if(clo.start()) {
	do {
	    if(con.compute_cell(c_v,clo)) {
		clo.pos(x,y,z);
		//cell_id = clo.pid();
		//printf("\t cell id = %d\tr = %g\tx = %g\ty = %g\tz = %g\n", cell_id, r, x, y, z);
		// ( (x >= xs_min) && (x <= xs_max) && (y >= ys_min) && (y <= ys_max) && (z >= zs_min) && (z <= zs_max) ) {
		    //printf("\t----->\n");
		c_v.draw_pov_mesh(x,y,z,f1);
		c_v.draw_pov(x,y,z,f2);
/*
	printf("Total vertices      : %d\n",c_v.p);
	printf("Vertex positions    : ");c_v.output_vertices();puts("");
	printf("Vertex orders       : ");c_v.output_vertex_orders();puts("");
	printf("Max rad. sq. vertex : %g\n\n",0.25*c_v.max_radius_squared());
	printf("Total edges         : %d\n",c_v.number_of_edges());
	printf("Total edge distance : %g\n",c_v.total_edge_distance());
	printf("Face perimeters     : ");c_v.output_face_perimeters();puts("\n");
	printf("Total faces         : %d\n",c_v.number_of_faces());
	printf("Surface area        : %g\n",c_v.surface_area());
	printf("Face freq. table    : ");c_v.output_face_freq_table();puts("");
	printf("Face orders         : ");c_v.output_face_orders();puts("");
	printf("Face areas          : ");c_v.output_face_areas();puts("");
	printf("Face normals        : ");c_v.output_normals();puts("");
	printf("Face vertices       : ");c_v.output_face_vertices();puts("\n");
	c_v.centroid(x,y,z);
	printf("Volume              : %g\n"
	       "Centroid vector     : (%g,%g,%g)\n",c_v.volume(),x,y,z);
*/
	    }
	} while(clo.inc());
    }
    fclose(f1);
    fclose(f2);

    FILE *f3=safe_fopen("loops3_v.pov","w");
    //c_loop_all cl(con);
    c_loop_order cl(con,p_o);
    if(cl.start()) {
	do {
	    if(con.compute_cell(c_n,cl)) {
		cl.pos(x,y,z);
		cell_id = cl.pid();
		c_n.neighbors(neigh);
		c_n.face_areas(v);
		//if( nodes[cell_id] == 1 ) {
		if( cell_id == 6408 )
		{
		    printf("voro:::cell id = %d\tx = %g\ty = %g\tz = %g\n", cell_id, x, y, z);
		    mmr_node_coor(mesh_id, cell_id, node_coor);
		    x = node_coor[0];
		    y = node_coor[1];
		    z = node_coor[2];
		    printf("modfem:cell id = %d\tx = %g\ty = %g\tz = %g\n", cell_id, x, y, z);
		    for( i = 0; i < neigh.size(); i++ ) {
			printf("\tngh[%d] = %d\tface = %g\n", i, neigh[i], v[i]);
		    }
		    printf("\n");
		}
		c_n.face_vertices(f_vert);
		c_n.vertices(x,y,z,v);
		//if( nodes[cell_id] == 1 ) {
		if( cell_id == 6408 ) {
		    for( i = 0, j = 0; i < neigh.size(); i++ ) {
			//if( neigh[i] > cell_id )
			{
			    draw_polygon(f3,f_vert,v,j);
			}
			j += f_vert[j] + 1;
		    }
		}
	    }
	} while(cl.inc());
    }
    fclose(f3);


    free(pdv_patches);
    printf(":::voro++ container\n");

    return(EXIT_SUCCESS);   
}

/*------------------------------------------------------------
pdr_vof_problem_vof - update vof field
------------------------------------------------------------*/
int pdr_vof_problem_vof(pdt_vof_problem *Problem)
{
    int problem_id, field_id, mesh_id;
    int i, j, cell_id, node_id_max, cell_id_max;
    int nr_nodes;
    int el_nodes[MMC_MAXELVNO+1];
    double node_coor[3 * MMC_MAXELVNO];
    double x_min, x_max, y_min, y_max, z_min, z_max;
    int n_x, n_y, n_z;
    double x, y, z;
    vector<int> neigh, f_vert;
    vector<double> v;

    printf(":::voro++ container\n");

    using namespace voro;
    voronoicell c_v;
    //particle_order p_o;
    voronoicell_neighbor c_n;

    problem_id = PDC_VOF_ID;
    i = 3; field_id = pdr_ctrl_i_params(problem_id, i);
    mesh_id = apr_get_mesh_id(field_id);

    node_id_max = mmr_get_max_node_id(mesh_id);
    printf("\tNode id max = %d\n", node_id_max);
    int nodes[node_id_max];
    for(i = 1; i <= node_id_max; i++){
	nodes[i] = 0;
    }
    cell_id_max = mmr_get_max_elem_id(mesh_id);
    printf("\tCell id max = %d\n", cell_id_max);
    int cells[cell_id_max];
    for(i = 1; i <= cell_id_max; i++){
	cells[i] = 0;
    }

    node_id_max = mmr_get_max_node_id(mesh_id);
    printf("\tNode id max = %d\n", node_id_max);
    printf("\tCell id max = %d\n", cell_id_max);

    for(i = 1; i <= node_id_max; i++) {
	mmr_node_coor(mesh_id, i, node_coor);
	x = node_coor[0];
	y = node_coor[1];
	z = node_coor[2];
	if ( i == 1) {
	    x_min = x;
	    x_max = x;
	    y_min = y;
	    y_max = y;
	    z_min = z;
	    z_max = z;
	}
	if( x < x_min ) x_min = x;
	if( x > x_max ) x_max = x;
	if( y < y_min ) y_min = y;
	if( y > y_max ) y_max = y;
	if( z < z_min ) z_min = z;
	if( z > z_max ) z_max = z;
    }
    pre_container pcon(x_min, x_max, y_min, y_max, z_min, z_max, false, false, false);
    for(i = 1; i <= node_id_max; i++) {
	mmr_node_coor(mesh_id, i, node_coor);
	x = node_coor[0];
	y = node_coor[1];
	z = node_coor[2];
	pcon.put(i, x, y, z);
    }
    pcon.guess_optimal(n_x,n_y,n_z);
    printf("\tn_x = %d\tn_y = %d\tn_z = %d\n", n_x, n_y, n_z);
    container con(x_min, x_max, y_min, y_max, z_min, z_max, n_x, n_y, n_z, false, false, false, 8);
    pcon.setup(con);

    FILE *f3=safe_fopen("loops3_v.pov","w");
    c_loop_all cl(con);
    if(cl.start()) {
	do {
	    if(con.compute_cell(c_n,cl)) {
		cl.pos(x,y,z);
		cell_id = cl.pid();
		c_n.neighbors(neigh);
		c_n.face_areas(v);
		if( cell_id == 6408 )
		{
		    printf("voro:::cell id = %d\tx = %g\ty = %g\tz = %g\n", cell_id, x, y, z);
		    mmr_node_coor(mesh_id, cell_id, node_coor);
		    x = node_coor[0];
		    y = node_coor[1];
		    z = node_coor[2];
		    printf("modfem:cell id = %d\tx = %g\ty = %g\tz = %g\n", cell_id, x, y, z);
		    for( i = 0; i < neigh.size(); i++ ) {
			printf("\tngh[%d] = %d\tface = %g\n", i, neigh[i], v[i]);
		    }
		    printf("\n");
		}
		c_n.face_vertices(f_vert);
		c_n.vertices(x, y, z, v);
		if( cell_id == 6408 ) {
		    for( i = 0, j = 0; i < neigh.size(); i++ ) {
			{
			    draw_polygon(f3,f_vert,v,j);
			}
			j += f_vert[j] + 1;
		    }
		}
		for( i = 0; i < neigh.size(); i++ ) {
		    
		}
	    }
	} while(cl.inc());
    }
    fclose(f3);

    printf(":::voro++ container\n");

    return(EXIT_SUCCESS);   
}

void draw_polygon(FILE *fp,vector<int> &f_vert,vector<double> &v,int j) {
	static char s[6][128];
	int k,l,n=f_vert[j];

	// Create POV-Ray vector strings for each of the vertices
	for(k=0;k<n;k++) {
		l=3*f_vert[j+k+1];
		sprintf(s[k],"<%g,%g,%g>",v[l],v[l+1],v[l+2]);
	}

	// Draw the interior of the polygon
	fputs("union{\n",fp);
	for(k=2;k<n;k++) fprintf(fp,"\ttriangle{%s,%s,%s}\n",s[0],s[k-1],s[k]);
	fputs("\ttexture{t1}\n}\n",fp);

	// Draw the outline of the polygon
	fputs("union{\n",fp);
	for(k=0;k<n;k++) {
		l=(k+1)%n;
		fprintf(fp,"\tcylinder{%s,%s,r}\n\tsphere{%s,r}\n",
			s[k],s[l],s[l]);
	}
	fputs("\ttexture{t2}\n}\n",fp);
}

/*------------------------------------------------------------
pdr_vof_problem_clear - clear vof problem data
------------------------------------------------------------*/
int pdr_vof_problem_clear(pdt_vof_problem *Problem)
{
  Problem->adpt.eps = 0.0;
  Problem->adpt.interval = 0;
  Problem->adpt.maxgen = 0;
  Problem->adpt.monitor = 0;
  Problem->adpt.ratio = 0.0;
  Problem->adpt.type = 0;
  
  /* Problem->ctrl.ambient_temperature = 0.0; */
  Problem->ctrl.field_id = 0;
  //Problem->ctrl.gravity_field[0] = 0.0;
  //Problem->ctrl.gravity_field[1] = 0.0;
  //Problem->ctrl.gravity_field[2] = 0.0;
  Problem->ctrl.mesh_id = 0;
  Problem->ctrl.nr_sol = 0;
  Problem->ctrl.name = 0;
  Problem->ctrl.nreq = 0;
  Problem->ctrl.solver_id = 0;
  Problem->ctrl.penalty = 0.0;
  /* Problem->ctrl.ref_temperature = 0.0; */
  
  Problem->lins.conv_meas = 0.0;
  Problem->lins.conv_type = 0;
  Problem->lins.max_iter = 0;
  Problem->lins.monitor = 0;
  Problem->lins.type = 0;
  
  Problem->nonl.conv_meas = 0.0;
  Problem->nonl.conv_type = 0;
  Problem->nonl.max_iter = 0;
  Problem->nonl.monitor = 0;
  Problem->nonl.type = 0;
    
  Problem->time.conv_meas = 0.0;
  Problem->time.cur_dtime = 0.0;
  Problem->time.cur_step = 0;
  Problem->time.cur_time = 0.0;
  Problem->time.final_step = 0;
  Problem->time.final_time = 0.0;
  Problem->time.intv_dumpout = 0;
  Problem->time.intv_graph = 0;
  Problem->time.intv_graph_accu = 0;
  Problem->time.monitor = 0;
  Problem->time.prev_dtime = 0.0;
  
  return 0;
}

/*------------------------------------------------------------
pdr_mat_problem_clear - clear vof problem mat data
------------------------------------------------------------*/
int pdr_mat_problem_clear(pdt_mat_problem *Problem)
{
  /* Problem->adpt.eps = 0.0; */
  /* Problem->adpt.interval = 0; */
  /* Problem->adpt.maxgen = 0; */
  /* Problem->adpt.monitor = 0; */
  /* Problem->adpt.ratio = 0.0; */
  /* Problem->adpt.type = 0; */
  
  /* Problem->ctrl.ambient_temperature = 0.0; */
  Problem->ctrl.field_id = 0;
  //Problem->ctrl.gravity_field[0] = 0.0;
  //Problem->ctrl.gravity_field[1] = 0.0;
  //Problem->ctrl.gravity_field[2] = 0.0;
  Problem->ctrl.mesh_id = 0;
  Problem->ctrl.nr_sol = 0;
  Problem->ctrl.name = 0;
  Problem->ctrl.nreq = 0;
  Problem->ctrl.solver_id = 0;
  /* Problem->ctrl.penalty = 0.0; */
  /* Problem->ctrl.ref_temperature = 0.0; */
  
  /* Problem->lins.conv_meas = 0.0; */
  /* Problem->lins.conv_type = 0; */
  /* Problem->lins.max_iter = 0; */
  /* Problem->lins.monitor = 0; */
  /* Problem->lins.type = 0; */
  
  /* Problem->nonl.conv_meas = 0.0; */
  /* Problem->nonl.conv_type = 0; */
  /* Problem->nonl.max_iter = 0; */
  /* Problem->nonl.monitor = 0; */
  /* Problem->nonl.type = 0; */
    
  /* Problem->time.conv_meas = 0.0; */
  /* Problem->time.cur_dtime = 0.0; */
  /* Problem->time.cur_step = 0; */
  /* Problem->time.cur_time = 0.0; */
  /* Problem->time.final_step = 0; */
  /* Problem->time.final_time = 0.0; */
  /* Problem->time.intv_dumpout = 0; */
  /* Problem->time.intv_graph = 0; */
  /* Problem->time.intv_graph_accu = 0; */
  /* Problem->time.monitor = 0; */
  /* Problem->time.prev_dtime = 0.0; */
  
  return 0;
}

/*------------------------------------------------------------
pdr_dtdt_problem_clear - clear problem data
------------------------------------------------------------*/
/* int pdr_heat_dtdt_problem_clear(pdt_heat_dtdt_problem *Problem) */
/* { */
/*   /\* Problem->adpt.eps = 0.0; *\/ */
/*   /\* Problem->adpt.interval = 0; *\/ */
/*   /\* Problem->adpt.maxgen = 0; *\/ */
/*   /\* Problem->adpt.monitor = 0; *\/ */
/*   /\* Problem->adpt.ratio = 0.0; *\/ */
/*   /\* Problem->adpt.type = 0; *\/ */
  
/*   /\* Problem->ctrl.ambient_temperature = 0.0; *\/ */
/*   Problem->ctrl.field_id = 0; */
/*   //Problem->ctrl.gravity_field[0] = 0.0; */
/*   //Problem->ctrl.gravity_field[1] = 0.0; */
/*   //Problem->ctrl.gravity_field[2] = 0.0; */
/*   Problem->ctrl.mesh_id = 0; */
/*   Problem->ctrl.nr_sol = 0; */
/*   Problem->ctrl.name = 0; */
/*   Problem->ctrl.nreq = 0; */
/*   Problem->ctrl.solver_id = 0; */
/*   /\* Problem->ctrl.penalty = 0.0; *\/ */
/*   /\* Problem->ctrl.ref_temperature = 0.0; *\/ */
  
/*   /\* Problem->lins.conv_meas = 0.0; *\/ */
/*   /\* Problem->lins.conv_type = 0; *\/ */
/*   /\* Problem->lins.max_iter = 0; *\/ */
/*   /\* Problem->lins.monitor = 0; *\/ */
/*   /\* Problem->lins.type = 0; *\/ */
  
/*   /\* Problem->nonl.conv_meas = 0.0; *\/ */
/*   /\* Problem->nonl.conv_type = 0; *\/ */
/*   /\* Problem->nonl.max_iter = 0; *\/ */
/*   /\* Problem->nonl.monitor = 0; *\/ */
/*   /\* Problem->nonl.type = 0; *\/ */
    
/*   /\* Problem->time.conv_meas = 0.0; *\/ */
/*   /\* Problem->time.cur_dtime = 0.0; *\/ */
/*   /\* Problem->time.cur_step = 0; *\/ */
/*   /\* Problem->time.cur_time = 0.0; *\/ */
/*   /\* Problem->time.final_step = 0; *\/ */
/*   /\* Problem->time.final_time = 0.0; *\/ */
/*   /\* Problem->time.intv_dumpout = 0; *\/ */
/*   /\* Problem->time.intv_graph = 0; *\/ */
/*   /\* Problem->time.intv_graph_accu = 0; *\/ */
/*   /\* Problem->time.monitor = 0; *\/ */
/*   /\* Problem->time.prev_dtime = 0.0; *\/ */
  
/*   return 0; */
/* } */

/*------------------------------------------------------------
pdr_vof_get_vof_at_point - to provide the vof and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_vof_weakform.c)
------------------------------------------------------------*/
int pdr_vof_get_vof_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Vof, // volume of fluid
  double *DVof_dx, // x-derivative of temperature
  double *DVof_dy, // y-derivative of temperature
  double *DVof_dz // z-derivative of temperature
					 )
{
  pdt_vof_problem *problem = 
    (pdt_vof_problem *)pdr_get_problem_structure(Problem_id);

  int field_id = problem->ctrl.field_id;
  int mesh_id = problem->ctrl.mesh_id;

  int nel = El_id;

  /* find degree of polynomial and number of element scalar dofs */
  int pdeg = 0;
  int base = apr_get_base_type(field_id, nel);
  apr_get_el_pdeg(field_id, nel, &pdeg);
  int num_shap = apr_get_el_pdeg_numshap(field_id, nel, &pdeg);
  int nreq=apr_get_nreq(field_id);
  
  /* get the coordinates of the nodes of nel in the right order */
  int el_nodes[MMC_MAXELVNO+1];    
  double node_coor[3*MMC_MAXELVNO]; 
  mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
  
  /* get the most recent solution degrees of freedom */
  int sol_vec_id = 1;
  double dofs_loc[APC_MAXELSD]={0}; /* element solution dofs */
  apr_get_el_dofs(field_id, nel, sol_vec_id, dofs_loc);
  
  if(DVof_dx==NULL && Base_dphix==NULL){

    // get vof 
    int iaux = 1; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,NULL,dofs_loc,
		     Base_phi,NULL,NULL,NULL,
		     NULL,Vof,NULL,NULL,NULL,NULL);
    
  }
  else{

    // get vof and its derivatives
    int iaux = 2; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,node_coor,dofs_loc,
		     Base_phi,Base_dphix,Base_dphiy,Base_dphiz,
		     NULL, Vof, DVof_dx, DVof_dy, DVof_dz, NULL);
    
  }

  return(0);
}

/*------------------------------------------------------------
pdr_vof_get_mat_at_point - to provide the mat and its
  gradient at a particular point with local coordinates within an element
MODULE PROVIDES IMPLEMENTATION FOR ALL OTHER MODULES (in pds_vof_weakform.c)
------------------------------------------------------------*/
int pdr_vof_get_mat_at_point(
  int Problem_id,
  int El_id, // element
  double *X_loc, // local coordinates of point
  double *Base_phi, // shape functions at point (if available - to speed up)
  double *Base_dphix, // derivatives of shape functions at point
  double *Base_dphiy, // derivatives of shape functions at point
  double *Base_dphiz, // derivatives of shape functions at point
  double *Mat, // volume of fluid
  double *DMat_dx, // x-derivative of temperature
  double *DMat_dy, // y-derivative of temperature
  double *DMat_dz // z-derivative of temperature
					 )
{
  pdt_mat_problem *problem = 
    (pdt_mat_problem *)pdr_get_problem_structure(Problem_id);

  int field_id = problem->ctrl.field_id;
  int mesh_id = problem->ctrl.mesh_id;

  int nel = El_id;

  /* find degree of polynomial and number of element scalar dofs */
  int pdeg = 0;
  int base = apr_get_base_type(field_id, nel);
  apr_get_el_pdeg(field_id, nel, &pdeg);
  int num_shap = apr_get_el_pdeg_numshap(field_id, nel, &pdeg);
  int nreq=apr_get_nreq(field_id);
  
  /* get the coordinates of the nodes of nel in the right order */
  int el_nodes[MMC_MAXELVNO+1];    
  double node_coor[3*MMC_MAXELVNO]; 
  mmr_el_node_coor(mesh_id,nel,el_nodes,node_coor);
  
  /* get the most recent solution degrees of freedom */
  int sol_vec_id = 1;
  double dofs_loc[APC_MAXELSD]={0}; /* element solution dofs */
  apr_get_el_dofs(field_id, nel, sol_vec_id, dofs_loc);
  
  if(DMat_dx==NULL && Base_dphix==NULL){

    // get vof 
    int iaux = 1; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,NULL,dofs_loc,
		     Base_phi,NULL,NULL,NULL,
		     NULL,Mat,NULL,NULL,NULL,NULL);
    
  }
  else{

    // get vof and its derivatives
    int iaux = 2; 
    apr_elem_calc_3D(iaux, nreq, &pdeg, base,
		     X_loc,node_coor,dofs_loc,
		     Base_phi,Base_dphix,Base_dphiy,Base_dphiz,
		     NULL, Mat, DMat_dx, DMat_dy, DMat_dz, NULL);
    
  }

  return(0);
}
