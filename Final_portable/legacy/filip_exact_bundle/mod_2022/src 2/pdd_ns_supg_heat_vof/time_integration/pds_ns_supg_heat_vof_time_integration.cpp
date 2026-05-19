/************************************************************************
File pds_ns_supg_heat_vof_time_integration.c - time stepping procedures

Contains definition of routines:
  pdr_ns_supg_heat_vof_comp_sol_diff_norm - returns max difference between 
                                        current and old solutions vectors
  pdr_ns_supg_heat_vof_time_integration - time integration driver

------------------------------
History:
	2002    - Krzysztof Banas (pobanas@cyf-kr.edu.pl) for conv-diff
	2011    - Przemyslaw Plaszewski (pplaszew@agh.edu.pl)
	2011    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
	2012    - Krzysztof Banas (pobanas@cyf-kr.edu.pl)
	2013    - Aleksander Siwek (Aleksander.Siwek@agh.edu.pl)
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include<string.h>
#include<assert.h>
#include "voro++.hh"

/* interface for all mesh manipulation modules */
#include "mmh_intf.h"		/* USES */
/* interface for all approximation modules */
#include "aph_intf.h"		/* USES */
/* interface for all solver modules */
#include "sih_intf.h"		/* USES */
#include "uth_intf.h"

/* problem dependent module interface */
#include "pdh_intf.h"		/* USES */
/* interface for control parameters and some general purpose functions */
/* from problem dependent module */
#include "pdh_control_intf.h"	/* USES */

#ifdef PARALLEL
#include<mpi.h>
/* interface of parallel mesh manipulation modules */
#include "mmph_intf.h"		/* USES */
/* interface for all parallel approximation modules */
#include "apph_intf.h"		/* USES */
/* interface for parallel communication modules */
#include "pch_intf.h"		/* USES */
#endif

/* problem module's types and functions */
#include "../include/pdh_ns_supg_heat_vof.h"	/* USES & IMPLEMENTS */
/* types and functions related to problem structures */
#include "../../pdd_ns_supg/include/pdh_ns_supg_problem.h" 
#include "../../pdd_heat/include/pdh_heat_problem.h" 
#include "../../pdd_vof/include/pdh_vof_problem.h" 
// bc and material header files are included in problem header files

/* functions related to weak formulation (here for computing CFL number) */
#include "../../pdd_ns_supg/include/pdh_ns_supg_weakform.h" 

using namespace std;
using namespace voro;

/**************************************/
/* INTERNAL PROCEDURES                */
/**************************************/
/* Rules:
/* - name always begins with pdr_ */
/* - argument names start uppercase */

double rnd2() {return double(rand())/RAND_MAX;}

/*------------------------------------------------------------
pdr_ns_supg_heat_vof_sol_diff_norm - returns max difference in current and old
			  solutions vectors
------------------------------------------------------------*/
void pdr_ns_supg_heat_vof_sol_diff_norm(
  int Current, // in: current vector id 
  int Old,     // in: old vector id
  double* sol_diff_norm_ns_supg_p, // norm of difference current-old for ns_supg
  double* sol_diff_norm_heat_p // norm of difference current-old for heat problem
  )
{

  double sol_dofs_current[APC_MAXELSD];	/* solution dofs */
  double sol_dofs_old[APC_MAXELSD];	/* solution dofs */
  int field_id, mesh_id;
  int node_id = 0;
  double temp, norm_ns_supg = 0.0, norm_heat = 0.0;
  int i;

/*++++++++++++++++ executable statements ++++++++++++++++*/
  
  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;  // ns_supg problem
  i=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,i);
  mesh_id = apr_get_mesh_id(field_id);
  
  while ((node_id = mmr_get_next_node_all(mesh_id, node_id)) != 0) {
    if (apr_get_ent_pdeg(field_id, APC_VERTEX, node_id) > 0) {
      int numdofs = apr_get_ent_nrdofs(field_id, APC_VERTEX, node_id);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Current, sol_dofs_current);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Old, sol_dofs_old);
      i = 0; // for ns_supg we take into account velocities ONLY!!!
      for (i = 0; i < numdofs-1; ++i) {

/*kbw
  double norm=0.0;
  printf("node_id %d\ti %d\tcurrent %lf\told %lf\tnorm %lf\n",node_id,i,sol_dofs_current[i],sol_dofs_old[i],norm);
  norm += (sol_dofs_current[i] - sol_dofs_old[i]) * (sol_dofs_current[i] - sol_dofs_old[i]);
/*kew*/

	temp = fabs(sol_dofs_current[i] - sol_dofs_old[i]);
	if (norm_ns_supg < temp) norm_ns_supg = temp;
      }
    }
  }
  
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;  // heat problem
  i=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,i);
  mesh_id = apr_get_mesh_id(field_id);
  
  while ((node_id = mmr_get_next_node_all(mesh_id, node_id)) != 0) {
    if (apr_get_ent_pdeg(field_id, APC_VERTEX, node_id) > 0) {
      int numdofs = apr_get_ent_nrdofs(field_id, APC_VERTEX, node_id);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Current, sol_dofs_current);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Old, sol_dofs_old);
      i = 0;
      for (i = 0; i < numdofs; ++i) {
	
/*kbw
  printf("el_id %d\ti %d\tcurrent %lf\told %lf\tnorm %lf\n",el_id,i,sol_dofs_current[i],sol_dofs_old[i],norm);
  norm += (sol_dofs_current[i] - sol_dofs_old[i]) * (sol_dofs_current[i] - sol_dofs_old[i]);
/*kew*/

	temp = fabs(sol_dofs_current[i] - sol_dofs_old[i]);
	if (norm_heat < temp){
	  norm_heat = temp;
	}
	
      }
    }
  }
  
  
#ifdef PARALLEL
  pcr_allreduce_max_double(1, &norm_ns_supg, &temp);
  norm_ns_supg = temp;
  pcr_allreduce_max_double(1, &norm_heat, &temp);
  norm_heat = temp;
#endif

  *sol_diff_norm_ns_supg_p = norm_ns_supg;
  *sol_diff_norm_heat_p = norm_heat;
  
  return;
}

/*------------------------------------------------------------
  pdr_rewr_heat_dtdt_sol - to rewrite solution from one vector to another
------------------------------------------------------------*/
int pdr_rewr_heat_dtdt_sol(
  int Field_id,      /* in: data structure to be used  */
  int Sol_from,      /* in: ID of vector to read solution from */
  int Sol_to         /* in: ID of vector to write solution to */
  )
{

  int nel, mesh_id, num_eq;
  double dofs_loc[APC_MAXELSD];

/*++++++++++++++++ executable statements ++++++++++++++++*/

  mesh_id = apr_get_mesh_id(Field_id);

  nel=0;
  num_eq = pdv_heat_dtdt_problem.ctrl.nreq;
  while((nel=mmr_get_next_act_elem(mesh_id,nel))!=0){
    printf("\n\tAS: nel = %d", nel);
    apr_read_ent_dofs(Field_id, APC_ELEMENT, nel, num_eq, Sol_from, dofs_loc);
    apr_write_ent_dofs(Field_id, APC_ELEMENT, nel, num_eq, Sol_to, dofs_loc);

  }

  return(0);
}

/*------------------------------------------------------------
pdr_heat_dtdt - returns heating-cooling rate field between
		    current and old heat solution vector
------------------------------------------------------------*/
void pdr_heat_dtdt(
	int Current,	// in: current vector id 
	int Old			// in: old vector id
	//const utt_patches * pdv_patches_mat	// in: patch
  )
{

  double sol_dofs_current[APC_MAXELSD];	/* solution dofs */
  double sol_dofs_old[APC_MAXELSD];	/* solution dofs */
  double sol_dofs_dtdt[APC_MAXELSD];	/* solution dofs */
  double VOF_node[APC_MAXELSD];
  int field_id, mesh_id;
  int field_vof_id, field_dtdt_id;
  int node_id = 0;
  double dtdt;
  int i;	//, j, k;
  int num_eq;
  //pdt_ns_supg_times *time_ns_supg = &pdv_ns_supg_problem.time;
  //pdt_heat_times *time_heat = &pdv_heat_problem.time;
  //pdt_vof_times *time_vof = &pdv_vof_problem.time;
  pdt_heat_material_query_params query_params;
  pdt_heat_material_query_result query_result;
  double VOF_old, VOF_cur, latent_heat_of_fusion, tsol, tliq, temp_cur, temp_old;
  pdt_heat_problem *problem_heat = &pdv_heat_problem;
  double Xcoor[3];

/*++++++++++++++++ executable statements ++++++++++++++++*/

  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;  // heat problem
  i=3; field_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,i);
  pdv_ns_supg_heat_vof_current_problem_id = PDC_VOF_ID;  // vof problem
  i=3; field_vof_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,i);
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_DTDT_ID;  // heating-cooling problem
  i=3; field_dtdt_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,i);
  mesh_id = apr_get_mesh_id(field_id);

  while ((node_id = mmr_get_next_node_all(mesh_id, node_id)) != 0) {
    if (apr_get_ent_pdeg(field_id, APC_VERTEX, node_id) > 0) {
      int numdofs = apr_get_ent_nrdofs(field_id, APC_VERTEX, node_id);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Current, sol_dofs_current);
      apr_read_ent_dofs(field_id, APC_VERTEX, node_id, numdofs, 
			Old, sol_dofs_old);
		temp_cur = sol_dofs_current[0];
		temp_old = sol_dofs_old[0];
		query_params.temperature = sol_dofs_old[0];
		query_params.query_type = QUERY_HEAT_NODE;
		numdofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, node_id);
		apr_read_ent_dofs(field_vof_id, APC_VERTEX, node_id, numdofs, Old, VOF_node);
		query_params.aux = VOF_node[0];
		pdr_heat_material_query(&problem_heat->materials, 
				  &query_params, &query_result);
		latent_heat_of_fusion = query_result.latent_heat_of_fusion;
		tsol = query_result.temp_solidus;
		tliq = query_result.temp_liquidus;
		VOF_old = query_result.VOF;
		query_params.temperature = sol_dofs_current[0];
		query_params.query_type = QUERY_HEAT_NODE;
		numdofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, node_id);
		apr_read_ent_dofs(field_vof_id, APC_VERTEX, node_id, numdofs, Current, VOF_node);
		query_params.aux = VOF_node[0];
		pdr_heat_material_query(&problem_heat->materials, 
				  &query_params, &query_result);
		VOF_cur = query_result.VOF;
		// specific heat equivalent Ceq
		// Traidia_Roger_IntJHeatMassTrans_2011
		if ( VOF_old != VOF_cur ) {
		//if ( (temp_cur >= tsol) && (temp_cur <= tliq) || (temp_old >= tsol) && (temp_old <= tliq)) {
		  sol_dofs_dtdt[0] = latent_heat_of_fusion * (VOF_cur - VOF_old) / (temp_cur - temp_old);
      } else {
		  sol_dofs_dtdt[0] = 0.0;
      }
		// specific heat equivalent Ceq
		// Guen_in_IntJHeatMassTrans_2011
// 		if ( (temp_cur >= tsol) && (temp_cur <= tliq) ) {
// 			sol_dofs_dtdt[i] = latent_heat_of_fusion * 
// 				exp(-(pow((temp_cur - tsol), 2)) / (pow((tliq - tsol), 2))) / 
// 				(sqrt(M_PI) * (tliq - tsol));
// 		} else {
// 			sol_dofs_dtdt[i] = 0.0;
// 		}
      num_eq = pdv_heat_dtdt_problem.ctrl.nreq;
      apr_write_ent_dofs(field_dtdt_id, APC_VERTEX, node_id, num_eq, Current, sol_dofs_dtdt);
    }
  }
  
  return;
}

/*------------------------------------------------------------
pdr_vof_dt - vof field update at next time step
-------------------------------------------------------------*/

//	 Maliska_in_CompMethApplMechEng_2000.pdf (1-5)

void pdr_vof_dt(
	int Current,	// in: current vector id 
	int Old			// in: old vector id
	//const utt_patches * pdv_patches_mat		// in: patch
  )
{

  double sol_dofs_current[APC_MAXELSD];
  double sol_dofs_old[APC_MAXELSD];
  double sol_dofs[APC_MAXELSD];
  double node_coor[3 * MMC_MAXELVNO];
  int problem_id, mesh_id;
  int field_ns_supg_id, field_heat_id, field_vof_id;
  int node_id, node_id_max, neigh_node_id, cell_id, cell;
  int i, j, k, l, iaux;
  int found_cell_id;		// found common cell for two nodes
  double x_min, x_max, y_min, y_max, z_min, z_max;
  int n_x, n_y, n_z;
  double x, y, z;
  double xv, yv, zv;
  int num_vof_dofs, num_vof_eq;
  int num_ns_supg_dofs, num_ns_supg_eq, num_heat_dofs;
  vector<int> neigh, f_vert;
  voronoicell_neighbor c_n;
  voronoicell c_v;
  vector<double> v_v, v_a, v_n;
  double gami, FP, FPs1, FPs2, FP0, Fi, VPi, VP;
  double Pi_X[3 * MMC_MAXELVNO];
  int list_el[20];
  double xg[3];
  double u_val[PDC_MAXEQ], u_val_i[PDC_MAXEQ];	/* computed solution */
  double u_val_T[PDC_MAXEQ], u_val_VOF[PDC_MAXEQ];
  double u_abs;
  pdt_ns_supg_times *time_ns_supg = &pdv_ns_supg_problem.time;
  pdt_vof_problem *problem_vof = (pdt_vof_problem *)pdr_get_problem_structure(PDC_VOF_ID);
  pdt_vof_material_query_params qparams;
  pdt_vof_material_query_result qresult;
  double Tsol;

  problem_id = PDC_NS_SUPG_ID;
  i = 3; field_ns_supg_id = pdr_ctrl_i_params(problem_id, i);
  mesh_id = apr_get_mesh_id(field_ns_supg_id);
  node_id_max = mmr_get_max_node_id(mesh_id);
  i=5; num_ns_supg_eq = pdr_ctrl_i_params(problem_id, i);
  problem_id = PDC_HEAT_ID;
  i = 3; field_heat_id = pdr_ctrl_i_params(problem_id,i);
  problem_id = PDC_VOF_ID;
  i = 3; field_vof_id = pdr_ctrl_i_params(problem_id,i);
  i = 0;
  while( (i = mmr_get_next_node_all(mesh_id, i)) != 0) {
    if ( mmr_node_status(mesh_id, i) == MMC_ACTIVE ) {
  //for(i = 1; i <= node_id_max; i++) {
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
  }
  pre_container pcon(x_min, x_max, y_min, y_max, z_min, z_max, false, false, false);
  i = 0;
  while( (i = mmr_get_next_node_all(mesh_id, i)) != 0) {
  //for(i = 1; i <= node_id_max; i++) {
    if ( mmr_node_status(mesh_id, i) == MMC_ACTIVE ) {
		mmr_node_coor(mesh_id, i, node_coor);
		x = node_coor[0];
		y = node_coor[1];
		z = node_coor[2];
		pcon.put(i, x, y, z);
    }
  }

  pcon.guess_optimal(n_x,n_y,n_z);
  container con(x_min, x_max, y_min, y_max, z_min, z_max, n_x, n_y, n_z, false, false, false, 8);
  pcon.setup(con);
  c_loop_all cl(con);
  if(cl.start()) {
	 do {
		if(con.compute_cell(c_v,cl)) {
			cell_id = cl.pid();
			node_id = abs(cell_id);
			if ( mmr_node_status(mesh_id, node_id) == MMC_ACTIVE ) {
				if ( apr_get_ent_pdeg(field_heat_id, APC_VERTEX, node_id) > 0 ) {
				  num_heat_dofs = apr_get_ent_nrdofs(field_heat_id, APC_VERTEX, node_id);
				  apr_read_ent_dofs(field_heat_id, APC_VERTEX, node_id, num_heat_dofs, 
					 Current, u_val_T);
				  qparams.temperature = u_val_T[0];
				  num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, node_id);
				  apr_read_ent_dofs(field_vof_id, APC_VERTEX, node_id, num_vof_dofs, 
				    Old, u_val_VOF);
				  qparams.VOF_fd = u_val_VOF[0];
				  qparams.query_type = QUERY_VOF_NODE;
				  qparams.node_id = node_id;
				  pdr_vof_material_query(&problem_vof->materials, &qparams, &qresult);
				  Tsol = qresult.temp_solidus;
				  if ( u_val_T[0] > Tsol ) {
					 cl.pos(xv,yv,zv);
					 mmr_node_coor(mesh_id, node_id, node_coor);
					 assert( xv == node_coor[0] );
					 assert( yv == node_coor[1] );
					 assert( zv == node_coor[2] );					 
					 c_v.face_areas(v_a);
					 //c_v.face_vertices(f_vert);
					 //c_v.vertices(x, y, z, v_v);
					 c_v.normals(v_n);
					 con.compute_cell(c_n,cl);
					 c_n.neighbors(neigh);
					 //c_n.face_orders(f_vert);
					 VP = c_v.volume();
					 FPs1 = 0.0;
					 FPs2 = 0.0;
					 FP0 = u_val_VOF[0];
					 num_ns_supg_dofs = apr_get_ent_nrdofs(field_ns_supg_id, APC_VERTEX, node_id);
					 apr_read_ent_dofs(field_ns_supg_id, APC_VERTEX, node_id, num_ns_supg_dofs, 
						Current, u_val);
					 for( i = 0; i < neigh.size(); i++ ) {
						neigh_node_id = abs(neigh[i]);
						if ( apr_get_ent_pdeg(field_heat_id, APC_VERTEX, neigh_node_id) > 0 ) {
						  num_heat_dofs = apr_get_ent_nrdofs(field_heat_id, APC_VERTEX, neigh_node_id);
						  apr_read_ent_dofs(field_heat_id, APC_VERTEX, neigh_node_id, num_heat_dofs, 
								Current, u_val_T);
						  num_ns_supg_dofs = apr_get_ent_nrdofs(field_ns_supg_id, APC_VERTEX, neigh_node_id);
						  apr_read_ent_dofs(field_ns_supg_id, APC_VERTEX, neigh_node_id, num_ns_supg_dofs, 
							 Current, u_val_i);
						  qparams.temperature = u_val_T[0];
						  num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, neigh_node_id);
						  apr_read_ent_dofs(field_vof_id, APC_VERTEX, neigh_node_id, num_vof_dofs, 
							 Old, u_val_VOF);
						  qparams.VOF_fd = u_val_VOF[0];
						  qparams.query_type = QUERY_VOF_NODE;
						  qparams.node_id = neigh_node_id;
						  pdr_vof_material_query(&problem_vof->materials, &qparams, &qresult);
						  Tsol = qresult.temp_solidus;
						  if ( u_val_T[0] > Tsol ) {
							 VPi = (v_n[3*i] 	 * (u_val[0] + u_val_i[0]) + 
									  v_n[3*i+1] * (u_val[1] + u_val_i[1]) + 
									  v_n[3*i+2] * (u_val[2] + u_val_i[2]))/2;
							 if (  VPi > 0.0 ) {
								  gami = -0.5;
							 } else {
								  gami = 0.5;
							 }
							 Fi = u_val_VOF[0];
							 FPs1 += VPi * (0.5 - gami) * v_a[i];
							 FPs2 += VPi * (0.5 + gami) * Fi * v_a[i];
						  }
						}
					 }
					 FP = (FP0 * VP - FPs2 * time_ns_supg->cur_dtime) / (VP + FPs1 * time_ns_supg->cur_dtime);
					 if ( FP > 1.0 ) FP = 1.0;
					 if ( FP < 1.0E-4 ) FP= 0.0;
					 sol_dofs_current[0] = FP;
					 num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, node_id);
					 apr_write_ent_dofs(field_vof_id, APC_VERTEX, node_id, num_vof_dofs, 
						Current, sol_dofs_current);
				  }
				}
			}
		}
	 } while(cl.inc());
  }
  return;
}

/*------------------------------------------------------------
pdr_vof_SV_dt - vof field update at next time step
-------------------------------------------------------------*/

//	 Maliska_in_CompMethApplMechEng_2000.pdf Swaminathan and Voller (6-10, 43-45)

void pdr_vof_SV_dt(
	int Current,	// in: current vector id 
	int Old,			// in: old vector id
	FILE *Interactive_output
	//const utt_patches * pdv_patches_mat		// in: patch
  )
{

  double sol_dofs_current[APC_MAXELSD];
  double sol_dofs_old[APC_MAXELSD];
  double sol_dofs[APC_MAXELSD];
  double node_coor[3 * MMC_MAXELVNO];
  int problem_id, mesh_id;
  int field_ns_supg_id, field_heat_id, field_vof_id, field_mat_id;
  int node_id, node_id_max, neigh_node_id, cell_id, cell;
  int i, j, k, l, iaux;
  int found_cell_id;		// found common cell for two nodes
  double x_min, x_max, y_min, y_max, z_min, z_max;
  int n_x, n_y, n_z;
  double x, y, z;
  double xv, yv, zv;
  int num_vof_dofs, num_vof_eq;
  int num_mat_dofs;
  int num_ns_supg_dofs, num_ns_supg_eq, num_heat_dofs;
  vector<int> neigh, f_vert;
  voronoicell_neighbor c_n;
  voronoicell c_v;
  vector<double> v_v, v_a, v_n;
  double gami, FP, FPm, FPs1, FPs2, FP0, Fi, VPi, VP;
  double norm_FP, norm_FP_prev, dFP;
  double error_ave;
  int nonl_iter;
  double GP, Gi;
  double Pi_X[3 * MMC_MAXELVNO];
  int list_el[20];
  double xg[3];
  double u_val[PDC_MAXEQ], u_val_i[PDC_MAXEQ];	/* computed solution */
  double u_val_T[PDC_MAXEQ], u_val_VOF[PDC_MAXEQ], u_val_mat[PDC_MAXEQ];
  double u_abs;
  pdt_ns_supg_times *time_ns_supg = &pdv_ns_supg_problem.time;
  pdt_vof_problem *problem_vof = (pdt_vof_problem *)pdr_get_problem_structure(PDC_VOF_ID);
  pdt_vof_material_query_params qparams;
  pdt_vof_material_query_result qresult;
  pdt_vof_nonls *nonl_vof = &pdv_vof_problem.nonl;
  double Tsol;

  problem_id = PDC_NS_SUPG_ID;
  i = 3; field_ns_supg_id = pdr_ctrl_i_params(problem_id, i);
  mesh_id = apr_get_mesh_id(field_ns_supg_id);
  node_id_max = mmr_get_max_node_id(mesh_id);
  i=5; num_ns_supg_eq = pdr_ctrl_i_params(problem_id, i);
  problem_id = PDC_HEAT_ID;
  i = 3; field_heat_id = pdr_ctrl_i_params(problem_id,i);
  problem_id = PDC_VOF_ID;
  i = 3; field_vof_id = pdr_ctrl_i_params(problem_id,i);
  problem_id = PDC_MAT_ID;
  i = 3; field_mat_id = pdr_ctrl_i_params(problem_id,i);
  i = 0;
  while( (i = mmr_get_next_node_all(mesh_id, i)) != 0) {
    if ( mmr_node_status(mesh_id, i) == MMC_ACTIVE ) {
  //for(i = 1; i <= node_id_max; i++) {
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
  }
  pre_container pcon(x_min, x_max, y_min, y_max, z_min, z_max, false, false, false);
  i = 0;
  while( (i = mmr_get_next_node_all(mesh_id, i)) != 0) {
  //for(i = 1; i <= node_id_max; i++) {
    if ( mmr_node_status(mesh_id, i) == MMC_ACTIVE ) {
		mmr_node_coor(mesh_id, i, node_coor);
		x = node_coor[0];
		y = node_coor[1];
		z = node_coor[2];
		pcon.put(i, x, y, z);
    }
  }

  pcon.guess_optimal(n_x,n_y,n_z);
  container con(x_min, x_max, y_min, y_max, z_min, z_max, n_x, n_y, n_z, false, false, false, 8);
  pcon.setup(con);

  nonl_iter = 0;
  do {

	 apr_rewr_sol(field_mat_id, 1, 2);
	 apr_rewr_sol(field_vof_id, 1, 2);
	 c_loop_all cl(con);
	 // first iteration for GP (43)
	 if(cl.start()) {
		do {
		  if(con.compute_cell(c_v,cl)) {
			 cell_id = cl.pid();
			 node_id = abs(cell_id);
			 if ( mmr_node_status(mesh_id, node_id) == MMC_ACTIVE ) {
				  if ( apr_get_ent_pdeg(field_heat_id, APC_VERTEX, node_id) > 0 ) {
					 num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, node_id);
					 apr_read_ent_dofs(field_vof_id, APC_VERTEX, node_id, num_vof_dofs, 
						Old, u_val_VOF);
					 FP0 = u_val_VOF[0];
					 apr_read_ent_dofs(field_vof_id, APC_VERTEX, node_id, num_vof_dofs, 
						Current, u_val_VOF);
					 FP = u_val_VOF[0];
					 qparams.VOF_fd = u_val_VOF[0];
					 num_mat_dofs = apr_get_ent_nrdofs(field_mat_id, APC_VERTEX, node_id);
					 apr_read_ent_dofs(field_mat_id, APC_VERTEX, node_id, num_mat_dofs, 
						2, u_val_mat);
					 Gi = u_val_mat[0];
					 num_heat_dofs = apr_get_ent_nrdofs(field_heat_id, APC_VERTEX, node_id);
					 apr_read_ent_dofs(field_heat_id, APC_VERTEX, node_id, num_heat_dofs, 
						Current, u_val_T);
					 qparams.temperature = u_val_T[0];
					 qparams.query_type = QUERY_VOF_NODE;
					 qparams.node_id = node_id;
					 pdr_vof_material_query(&problem_vof->materials, &qparams, &qresult);
					 Tsol = qresult.temp_solidus;
					 /*if ( u_val_T[0] > Tsol )*/ {
						cl.pos(xv,yv,zv);
						mmr_node_coor(mesh_id, node_id, node_coor);
						assert( xv == node_coor[0] );
						assert( yv == node_coor[1] );
						assert( zv == node_coor[2] );					 
						c_v.face_areas(v_a);
						//c_v.face_vertices(f_vert);
						//c_v.vertices(x, y, z, v_v);
						c_v.normals(v_n);
						assert( ((v_n[0]*v_n[0]+v_n[1]*v_n[1]+v_n[2]*v_n[2]) <= 1.001) &&
						  ((v_n[0]*v_n[0]+v_n[1]*v_n[1]+v_n[2]*v_n[2]) >= 0.999) );
						con.compute_cell(c_n,cl);
						c_n.neighbors(neigh);
						//c_n.face_orders(f_vert);
						VP = c_v.volume();
						FPs1 = 0.0;
						FPs2 = 0.0;
						num_ns_supg_dofs = apr_get_ent_nrdofs(field_ns_supg_id, APC_VERTEX, node_id);
						apr_read_ent_dofs(field_ns_supg_id, APC_VERTEX, node_id, num_ns_supg_dofs, 
						  Current, u_val);
						for( i = 0; i < neigh.size(); i++ ) {
						  neigh_node_id = abs(neigh[i]);
						  if ( apr_get_ent_pdeg(field_heat_id, APC_VERTEX, neigh_node_id) > 0 ) {
							 num_heat_dofs = apr_get_ent_nrdofs(field_heat_id, APC_VERTEX, neigh_node_id);
							 apr_read_ent_dofs(field_heat_id, APC_VERTEX, neigh_node_id, num_heat_dofs, 
								  Current, u_val_T);
							 qparams.temperature = u_val_T[0];
							 num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, neigh_node_id);
							 apr_read_ent_dofs(field_vof_id, APC_VERTEX, neigh_node_id, num_vof_dofs, 
								Current, u_val_VOF);
							 Fi = u_val_VOF[0];
							 qparams.VOF_fd = u_val_VOF[0];
							 qparams.query_type = QUERY_VOF_NODE;
							 qparams.node_id = neigh_node_id;
							 pdr_vof_material_query(&problem_vof->materials, &qparams, &qresult);
							 Tsol = qresult.temp_solidus;
							 /*if ( u_val_T[0] > Tsol )*/ {
								num_ns_supg_dofs = apr_get_ent_nrdofs(field_ns_supg_id, APC_VERTEX, neigh_node_id);
								apr_read_ent_dofs(field_ns_supg_id, APC_VERTEX, neigh_node_id, num_ns_supg_dofs, 
								  Current, u_val_i);
								VPi = (v_n[3*i] 	 * (u_val[0] + u_val_i[0]) + 
										v_n[3*i+1] * (u_val[1] + u_val_i[1]) + 
										v_n[3*i+2] * (u_val[2] + u_val_i[2]))/2;
								if (  VPi > 0.0 ) {
									 gami = -0.5;
								} else {
									 gami = 0.5;
								}
								FPs1 += VPi * (0.5 - gami) * v_a[i] * Gi;
								FPs2 += VPi * (0.5 + gami) * v_a[i];
							 }
						  }
						}
						if ( FPs2 != 0 ) {
// 						if ( (fabs(FPs2) > 1.0E-4) && (fabs(FPs1) > 1.0E-4) ) {
						  GP = ((FP0 - FP) * VP / time_ns_supg->cur_dtime - FPs1) / FPs2;
// 						  assert( (GP >= 0.0) && (GP <= 1.0) );
// 						  if ( GP > 1.0 ) {
// 							 GP = 1.0;
// 							 fprintf(Interactive_output, "GP1.0#");
// 						  }
// 						  if ( GP < 0.0 ) {
// 							 GP= 0.0;
// 							 fprintf(Interactive_output, "GP0.0#");
// 						  }
						  sol_dofs_current[0] = GP;
						  num_mat_dofs = apr_get_ent_nrdofs(field_mat_id, APC_VERTEX, node_id);
						  apr_write_ent_dofs(field_mat_id, APC_VERTEX, node_id, num_mat_dofs, 
							 Current, sol_dofs_current);
						} /*else {
						  fprintf(Interactive_output, "\n\t\t\tDivision by 0 !");
						}*/
					 }
				  }
			 }
		  }
		} while(cl.inc());
	 }

	 // second iteration for update FP (45)
	 norm_FP_prev = norm_FP;
	 norm_FP = 0.0;
	 iaux = 0;
	 error_ave = 0.0;
	 if(cl.start()) {
		do {
		  if(con.compute_cell(c_v,cl)) {
			 cell_id = cl.pid();
			 node_id = abs(cell_id);
			 if ( mmr_node_status(mesh_id, node_id) == MMC_ACTIVE ) {
				  if ( apr_get_ent_pdeg(field_heat_id, APC_VERTEX, node_id) > 0 ) {
					 num_heat_dofs = apr_get_ent_nrdofs(field_heat_id, APC_VERTEX, node_id);
					 apr_read_ent_dofs(field_heat_id, APC_VERTEX, node_id, num_heat_dofs, 
						Current, u_val_T);
					 qparams.temperature = u_val_T[0];
					 qparams.query_type = QUERY_VOF_NODE;
					 qparams.node_id = node_id;
					 pdr_vof_material_query(&problem_vof->materials, &qparams, &qresult);
					 Tsol = qresult.temp_solidus;
					 if ( u_val_T[0] > Tsol ) {
						++iaux;
						cl.pos(xv,yv,zv);
						mmr_node_coor(mesh_id, node_id, node_coor);
						assert( xv == node_coor[0] );
						assert( yv == node_coor[1] );
						assert( zv == node_coor[2] );					 
						c_v.face_areas(v_a);
						//c_v.face_vertices(f_vert);
						//c_v.vertices(x, y, z, v_v);
						c_v.normals(v_n);
						con.compute_cell(c_n,cl);
						c_n.neighbors(neigh);
						//c_n.face_orders(f_vert);
						VP = c_v.volume();
						num_mat_dofs = apr_get_ent_nrdofs(field_mat_id, APC_VERTEX, node_id);
						apr_read_ent_dofs(field_mat_id, APC_VERTEX, node_id, num_mat_dofs, 
						  Current, u_val_mat);
						GP = u_val_mat[0];
						num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, neigh_node_id);
						apr_read_ent_dofs(field_vof_id, APC_VERTEX, node_id, num_vof_dofs, 
						  2, u_val_VOF);		// previous nonlinear iteration value of VOF (FPm)
						FPm = u_val_VOF[0];
						FPs1 = 0.0;
						num_ns_supg_dofs = apr_get_ent_nrdofs(field_ns_supg_id, APC_VERTEX, node_id);
						apr_read_ent_dofs(field_ns_supg_id, APC_VERTEX, node_id, num_ns_supg_dofs, 
						  Current, u_val);
						dFP = 0.0;
						for( i = 0; i < neigh.size(); i++ ) {
						  neigh_node_id = abs(neigh[i]);
						  if ( apr_get_ent_pdeg(field_heat_id, APC_VERTEX, neigh_node_id) > 0 ) {
							 num_heat_dofs = apr_get_ent_nrdofs(field_heat_id, APC_VERTEX, neigh_node_id);
							 apr_read_ent_dofs(field_heat_id, APC_VERTEX, neigh_node_id, num_heat_dofs, 
								  Current, u_val_T);
							 qparams.temperature = u_val_T[0];
							 num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, neigh_node_id);
							 apr_read_ent_dofs(field_vof_id, APC_VERTEX, neigh_node_id, num_vof_dofs, 
								2, u_val_VOF);
							 qparams.VOF_fd = u_val_VOF[0];
							 qparams.query_type = QUERY_VOF_NODE;
							 qparams.node_id = neigh_node_id;
							 pdr_vof_material_query(&problem_vof->materials, &qparams, &qresult);
							 Tsol = qresult.temp_solidus;
							 if ( u_val_T[0] > Tsol ) {
								num_ns_supg_dofs = apr_get_ent_nrdofs(field_ns_supg_id, APC_VERTEX, neigh_node_id);
								apr_read_ent_dofs(field_ns_supg_id, APC_VERTEX, neigh_node_id, num_ns_supg_dofs, 
								  Current, u_val_i);
								VPi = (v_n[3*i] 	 * (u_val[0] + u_val_i[0]) + 
										v_n[3*i+1] * (u_val[1] + u_val_i[1]) + 
										v_n[3*i+2] * (u_val[2] + u_val_i[2]))/2;
								if (  VPi > 0.0 ) {
									 gami = -0.5;
								} else {
									 gami = 0.5;
								}
								FPs1 += VPi * (0.5 + gami) * v_a[i];
							 }
						  }
						}
						dFP = GP * FPs1 * time_ns_supg->cur_dtime / VP;
						error_ave += fabs(dFP);
						FP = FPm + dFP;
// 						if ( FP > 1.0 ) {
// 						  FP = 1.0;
// 						  fprintf(Interactive_output, "FP1.0#");
// 						}
// 						if ( FP < 0.0 ) {
// 						  FP = 0.0;
// 						  fprintf(Interactive_output, "FP0.0#");
// 						}
// 						assert( (FP >= 0.0) && (FP <= 1.0) );
						sol_dofs_current[0] = FP;
						num_vof_dofs = apr_get_ent_nrdofs(field_vof_id, APC_VERTEX, node_id);
						apr_write_ent_dofs(field_vof_id, APC_VERTEX, node_id, num_vof_dofs, 
						  Current, sol_dofs_current);
						if ( norm_FP < abs(dFP) ) norm_FP = abs(dFP);
					 }
				  }
			 }
		  }
		} while(cl.inc());
	 }
	 if ( iaux > 0) {
		error_ave /= iaux;
	 }

	 // third iteration for update GP (7)
	 if(cl.start()) {
		do {
		  if(con.compute_cell(c_v,cl)) {
			 cell_id = cl.pid();
			 node_id = abs(cell_id);
			 if ( mmr_node_status(mesh_id, node_id) == MMC_ACTIVE ) {
				  if ( apr_get_ent_pdeg(field_heat_id, APC_VERTEX, node_id) > 0 ) {
					 apr_read_ent_dofs(field_vof_id, APC_VERTEX, node_id, num_vof_dofs, 
						Current, u_val_VOF);
					 FP = u_val_VOF[0];
					 if ( FP < 1.0 ) {
						if ( GP != 0.0 ) {
						  fprintf(Interactive_output, "\n\t\t-> GP(%d) = %10.6lf -> 0.0", node_id, GP);
						  GP = 0.0;
						}
					 }
					 sol_dofs_current[0] = GP;
					 num_mat_dofs = apr_get_ent_nrdofs(field_mat_id, APC_VERTEX, node_id);
					 apr_write_ent_dofs(field_mat_id, APC_VERTEX, node_id, num_mat_dofs, 
						Current, sol_dofs_current);
				  }
			 }
		  }
		} while(cl.inc());
	 }
// 	 apr_rewr_sol(field_mat_id, 1, 2);
// 	 apr_rewr_sol(field_vof_id, 1, 2);
					 
	 ++nonl_iter;
	 fprintf(Interactive_output, "\n\tSolving VOF for nonlin. (nonl. iter step %3d) convergence (time step %3d).", nonl_iter, time_ns_supg->cur_step);
	 fprintf(Interactive_output, "--> Norm for VOF (u_k, prev. u_k): max = %10.6lf ave(%d) = %10.6lf", norm_FP, iaux, error_ave);

  } while ( (norm_FP > 1.0E-05) && (nonl_iter < nonl_vof->max_iter) /*&& (norm_FP != norm_FP_prev)*/ );
  fprintf(Interactive_output, "\n");

  return;
}

/*------------------------------------------------------------
pdr_ns_supg_heat_vof_time_integration - time integration driver
------------------------------------------------------------*/
void pdr_ns_supg_heat_vof_time_integration(
  char* Work_dir,
  FILE *Interactive_input, 
  FILE *Interactive_output
)
{

  /* one instance of mesh, three instances of fields, two instances of solvers */
  int mesh_id;
  int field_ns_supg_id, field_heat_id;
  int field_heat_dtdt_id, field_vof_id, field_mat_id;
  int  solver_ns_supg_id, solver_heat_id;
  double sol_norm_uk_ns_supg, sol_norm_un_ns_supg;
  double sol_norm_uk_heat, sol_norm_un_heat;
  char autodump_filename[300];
  int iadapt = 0;
  char solver_ns_supg_filename[300], solver_heat_filename[300];;
  int nr_iter, nonl_iter, monitor;
  double conv_meas, conv_rate;
  int i, iaux;
  double cfl_min=1000000, cfl_max=0, cfl_ave=0.0;
  //int nr_patches;
  //utt_patches * pdv_patches_mat;

  // both problems use the same time integration parameters
  /*  Navier_Stokes problem parameters */
  pdt_ns_supg_problem *problem_ns_supg = &pdv_ns_supg_problem;
  pdt_ns_supg_ctrls *ctrl_ns_supg = &pdv_ns_supg_problem.ctrl;
  pdt_ns_supg_times *time_ns_supg = &pdv_ns_supg_problem.time;
  pdt_ns_supg_nonls *nonl_ns_supg = &pdv_ns_supg_problem.nonl;
  pdt_ns_supg_linss *lins_ns_supg = &pdv_ns_supg_problem.lins;
  pdt_ns_supg_adpts *adpt_ns_supg = &pdv_ns_supg_problem.adpt;

  /*  heat problem parameters */
  pdt_heat_problem *problem_heat = &pdv_heat_problem;
  pdt_heat_ctrls *ctrl_heat = &pdv_heat_problem.ctrl;
  pdt_heat_times *time_heat = &pdv_heat_problem.time;
  pdt_heat_nonls *nonl_heat = &pdv_heat_problem.nonl;
  pdt_heat_linss *lins_heat = &pdv_heat_problem.lins;
  pdt_heat_adpts *adpt_heat = &pdv_heat_problem.adpt;

  /*  vof problem parameters */
  pdt_vof_times *time_vof = &pdv_vof_problem.time;
  pdt_vof_nonls *nonl_vof = &pdv_vof_problem.nonl;

  /*  heating-cooling problem parameters */
  pdt_heat_dtdt_problem *problem_heat_dtdt = &pdv_heat_dtdt_problem;
  pdt_heat_dtdt_ctrls *ctrl_heat_dtdt = &pdv_heat_dtdt_problem.ctrl;

  /* time adaptation */
  double initial_dtime = time_ns_supg->prev_dtime;
  double final_dtime = time_ns_supg->cur_dtime;
  double dtime_adapt, time_adapt;
  double daux;

  /* time adaptation based on specified CFL number */
  int cfl_control = time_ns_supg->CFL_control;
  double cfl_limit = time_ns_supg->CFL_limit;
  //double reference_time_step_length = time_ns_supg->reference_time_step_length;
  double time_step_length_mult = time_ns_supg->time_step_length_mult;

/*++++++++++++++++ executable statements ++++++++++++++++*/

  
  // get mesh and field parameters
  /* get mesh id for ns_supg problem (the same is for heat and vof problem) */
  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
  i = 2; mesh_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,i);
  
  pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;  // ns_supg problem
  field_ns_supg_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,3);
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;  // heat problem
  field_heat_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,3);
  pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_DTDT_ID;  // heating-cooling problem
  field_heat_dtdt_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,3);
  pdv_ns_supg_heat_vof_current_problem_id = PDC_VOF_ID;  // vof problem
  field_vof_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,3);
  pdv_ns_supg_heat_vof_current_problem_id = PDC_MAT_ID;  // mat problem
  field_mat_id = pdr_ctrl_i_params(pdv_ns_supg_heat_vof_current_problem_id,3);



  // both problems use the same time integration parameters
  // we manipulate using mainly ns_supg structures
  if (time_ns_supg->cur_time >= time_ns_supg->final_time) {
    
    fprintf(Interactive_output, 
	    "\nCurrent time: %lf is bigger than final time: %lf.\n\n", 
	    time_ns_supg->cur_time, time_ns_supg->final_time);
    
    fprintf(Interactive_output, 
	    "\nCurrent time-step: %d, current time step length: %lf.\n\n", 
	    time_ns_supg->cur_step, time_ns_supg->cur_dtime);
    
    if(Interactive_input == stdin){
      
#ifdef PARALLEL
      if (pcv_my_proc_id == pcr_print_master()) {
#endif
	printf("How many steps to perform?\n");
	scanf("%d",&iaux);
#ifdef PARALLEL
      }
      pcr_bcast_int(pcr_print_master(), 1, &iaux);
#endif
      
#ifdef PARALLEL
      if (pcv_my_proc_id == pcr_print_master()) {
#endif
	printf("Set new time step length (or CFL limit if < 0):\n");
	scanf("%lf",&daux);
#ifdef PARALLEL
      }
      pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
      if(daux>0){
	time_ns_supg->CFL_control = 0;
	time_ns_supg->CFL_limit = 1.e10;
	time_ns_supg->time_step_length_mult = 1.0;
	//time_ns_supg->reference_time_step_length = daux;
	//cfl_control = 0;
	//cfl_limit = 1.e10;
	//time_step_length_mult = 1.0;
	time_ns_supg->cur_dtime = daux;
	time_heat->cur_dtime = time_ns_supg->cur_dtime;
	time_vof->cur_dtime = time_ns_supg->cur_dtime;
      }
      else{
	time_ns_supg->CFL_control = 1;
	time_ns_supg->CFL_limit = -daux;
	//time_ns_supg->reference_time_step_length = time_ns_supg->cur_dtime;
	//cfl_control = 1;
	//cfl_limit = -daux;
#ifdef PARALLEL
    if (pcv_my_proc_id == pcr_print_master()) {
#endif
	  printf("Set new time step length multiplier:\n");
	  scanf("%lf",&daux);
#ifdef PARALLEL
	}
	pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
	time_ns_supg->time_step_length_mult = daux;
	//time_step_length_mult = daux;
      }
    } else{
      
      fprintf(Interactive_output, "\nExiting!\n\n");
      exit(0);;
      
    }
    
    time_ns_supg->final_step = time_ns_supg->cur_step + iaux;
    time_heat->final_step = time_ns_supg->final_step;
    time_vof->final_step = time_ns_supg->final_step;
    
    time_ns_supg->final_time = time_ns_supg->cur_time + 
      iaux*time_ns_supg->cur_dtime;
    time_heat->final_time = time_ns_supg->final_time;
    time_vof->final_time = time_ns_supg->final_time;
  }
  
  if (time_ns_supg->cur_step >= time_ns_supg->final_step) {
    
    fprintf(Interactive_output, 
	    "\nCurrent time-step: %d is bigger than final step: %d\n\n", 
	    time_ns_supg->cur_step, time_ns_supg->final_step);
    
    fprintf(Interactive_output, 
	    "\nCurrent time: %lf, current time step length: %lf.\n\n", 
	    time_ns_supg->cur_time, time_ns_supg->cur_dtime);
    
    if(Interactive_input == stdin){
      
#ifdef PARALLEL
      if (pcv_my_proc_id == pcr_print_master()) {
#endif
	printf("How many steps to perform?\n");
	scanf("%d",&iaux);
#ifdef PARALLEL
      }
      pcr_bcast_int(pcr_print_master(), 1, &iaux);
#endif
      
#ifdef PARALLEL
      if (pcv_my_proc_id == pcr_print_master()) {
#endif
	printf("Set new time step length (or CFL limit if < 0):\n");
	scanf("%lf",&daux);
#ifdef PARALLEL
      }
      pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
      if(daux>0){
	time_ns_supg->CFL_control = 0;
	time_ns_supg->CFL_limit = 1.e10;
	time_ns_supg->time_step_length_mult = 1.0;
	//time_ns_supg->reference_time_step_length = daux;
	//cfl_control = 0;
	//cfl_limit = 1.e10;
	//time_step_length_mult = 1.0;
	time_ns_supg->cur_dtime = daux;
	time_heat->cur_dtime = time_ns_supg->cur_dtime;
	time_vof->cur_dtime = time_ns_supg->cur_dtime;
      }
      else{
	time_ns_supg->CFL_control = 1;
	time_ns_supg->CFL_limit = -daux;
	//time_ns_supg->reference_time_step_length = time_ns_supg->cur_dtime;
	//cfl_control = 1;
	//cfl_limit = -daux;
#ifdef PARALLEL
    if (pcv_my_proc_id == pcr_print_master()) {
#endif
	  printf("Set new time step length multiplier:\n");
	  scanf("%lf",&daux);
#ifdef PARALLEL
	}
	pcr_bcast_double(pcr_print_master(), 1, &daux);
#endif
	time_ns_supg->time_step_length_mult = daux;
	//time_step_length_mult = daux;
      }
      
    } else{
      
      fprintf(Interactive_output, "\nExiting!\n\n");
      exit(0);;
      
    }
    
    time_ns_supg->final_step = time_ns_supg->cur_step + iaux;
    time_heat->final_step = time_ns_supg->final_step;
    time_vof->final_step = time_ns_supg->final_step;
    
    time_ns_supg->final_time = time_ns_supg->cur_time + 
      iaux*time_ns_supg->cur_dtime;
    time_heat->final_time = time_ns_supg->final_time;
    time_vof->final_time = time_ns_supg->final_time;
    
  } 
  
  /* print some info */
  fprintf(Interactive_output, "\nTime integration will stop whichever comes first:\n");
  fprintf(Interactive_output, "final_time: %lf\n", time_ns_supg->final_time);
  fprintf(Interactive_output, "final_timestep: %d\n", time_ns_supg->final_step);
  fprintf(Interactive_output, "error less than time_integration_tolerance: %lf\n\n", time_ns_supg->conv_meas);
  
#ifndef PARALLEL
  if(Interactive_input == stdin) {
	printf("Type [Ctrl-C] to manually break time integration.\n");
  }
#endif

  sprintf(solver_ns_supg_filename, "%s/%s", 
	  ctrl_ns_supg->work_dir, ctrl_ns_supg->solver_filename);
  sprintf(solver_heat_filename, "%s/%s", 
	  ctrl_heat->work_dir, ctrl_heat->solver_filename);
  
  if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver

    pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;  // ns_supg problem
    int problem_id = pdv_ns_supg_heat_vof_current_problem_id;

    //fprintf(Interactive_output, "ns_supg solver parameters: maxiter %d, error_type %d, error_meas %.15lf, monitor %d\n",
    //			pdr_lins_i_params(problem_id, 2), // max_iter
    //			pdr_lins_i_params(problem_id, 3), // error_type
    //			pdr_lins_d_params(problem_id, 4), // error_tolerance
    //			pdr_lins_i_params(problem_id, 5)  // monitoring level
    //				 );


#ifndef PARALLEL
    solver_ns_supg_id = sir_init(SIC_SEQUENTIAL, solver_ns_supg_filename,
			pdr_lins_i_params(problem_id, 2), // max_iter
			pdr_lins_i_params(problem_id, 3), // error_type
			pdr_lins_d_params(problem_id, 4), // error_tolerance
			pdr_lins_i_params(problem_id, 5)  // monitoring level
				 );
    ctrl_ns_supg->solver_id = solver_ns_supg_id;
#endif
#ifdef PARALLEL
    solver_ns_supg_id = sir_init(SIC_PARALLEL, solver_ns_supg_filename,
			pdr_lins_i_params(problem_id, 2), // max_iter
			pdr_lins_i_params(problem_id, 3), // error_type
			pdr_lins_d_params(problem_id, 4), // error_tolerance
			pdr_lins_i_params(problem_id, 5)  // monitoring level
				 ); 
    ctrl_ns_supg->solver_id = solver_ns_supg_id;
#endif
  }

  if(pdr_lins_i_params(PDC_HEAT_ID, 1) > 0){ // iterative solver
    pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;  // heat problem
    int problem_id = pdv_ns_supg_heat_vof_current_problem_id;

    // fprintf(Interactive_output, "heat solver parameters: maxiter %d, error_type %d, error_meas %.15lf, monitor %d\n",
    //			pdr_lins_i_params(problem_id, 2), // max_iter
    //			pdr_lins_i_params(problem_id, 3), // error_type
    //			pdr_lins_d_params(problem_id, 4), // error_tolerance
    //			pdr_lins_i_params(problem_id, 5)  // monitoring level
    //				 );

#ifndef PARALLEL
    solver_heat_id = sir_init(SIC_SEQUENTIAL, solver_heat_filename,
			pdr_lins_i_params(problem_id, 2), // max_iter
			pdr_lins_i_params(problem_id, 3), // error_type
			pdr_lins_d_params(problem_id, 4), // error_tolerance
			pdr_lins_i_params(problem_id, 5)  // monitoring level
			      ); 
    ctrl_heat->solver_id = solver_heat_id;
    fprintf(Interactive_output, "\nAssigned solver IDs: ns_supg %d, heat %d\n", 
	    solver_ns_supg_id, solver_heat_id);
#endif
#ifdef PARALLEL
    solver_heat_id = sir_init(SIC_PARALLEL, solver_heat_filename,
			pdr_lins_i_params(problem_id, 2), // max_iter
			pdr_lins_i_params(problem_id, 3), // error_type
			pdr_lins_d_params(problem_id, 4), // error_tolerance
			pdr_lins_i_params(problem_id, 5)  // monitoring level
			      ); 
    ctrl_heat->solver_id = solver_heat_id;
    fprintf(Interactive_output, "Assigned solver IDs: ns_supg %d, heat %d\n", 
	    solver_ns_supg_id, solver_heat_id);
#endif
  }
  
  /* set parameter indicating new mesh */
  iadapt = 1;

  /* if(ctrl_ns_supg->name == 110){ // for flux_radconv_isothermal problem  */
  /*   dtime_adapt = initial_dtime; */
  /*   time_ns_supg->cur_dtime = dtime_adapt; */
  /*   if ( dtime_adapt < final_dtime ) { */
  /*     time_adapt = log10(final_dtime / dtime_adapt); */
  /*   } else { */
  /*     time_adapt = -1.0; */
  /*   } */
  /*   fprintf(Interactive_output, */
  /* 	    "\n\tAS: Time adaptation (1) --> dtime_adapt = %1.12lf\tfinal_dtime = %1.12lf\ttime_adapt in %.0lf steps\n", dtime_adapt, final_dtime, time_adapt); */
  /* } */

  /* start loop over time steps */
  while (pdv_ns_supg_SIGINT_not_caught) {

    /* update time step and current time */
    ++(time_ns_supg->cur_step);
    ++(time_heat->cur_step);
    ++(time_vof->cur_step);

    time_ns_supg->prev_dtime = time_ns_supg->cur_dtime;
    time_heat->prev_dtime = time_ns_supg->prev_dtime;
    time_vof->prev_dtime = time_ns_supg->prev_dtime;

    /* if(ctrl_ns_supg->name != 110){ // for general problem  */
          
    /*   time_ns_supg->prev_dtime = time_ns_supg->cur_dtime; */
    /*   time_heat->prev_dtime = time_ns_supg->prev_dtime; */
      
    /* } */
    /*   */
      
    /* else  */
    /*   if(ctrl_ns_supg->name == 110){ // for flux_radconv_isothermal problem  */

    /*   /\* if ( time_ns_supg->cur_step <= time_adapt ) { *\/ */
    /* 	if ( time_ns_supg->cur_dtime < final_dtime ) { */
    /* 	  /\* time_ns_supg->cur_dtime = pow(10.0, (time_ns_supg->cur_step - 1) ) * dtime_adapt; *\/ */
    /* 	  /\* time_ns_supg->prev_dtime = time_ns_supg->cur_dtime; *\/ */
    /* 	  /\* time_ns_supg->cur_dtime *= 10.0; *\/ */
    /* 	  fprintf(Interactive_output, */
    /* 		  "\n\tAS: Time adaptation (2) --> prev_dtime = %1.12lf\t\tcur_dtime = %1.12lf\n", time_ns_supg->prev_dtime, time_ns_supg->cur_dtime); */
    /* 	} /\* else { *\/ */
    /* 	/\* if ( time_ns_supg->cur_step > time_adapt ) { *\/ */
    /* 	/\*   time_ns_supg->cur_dtime = final_dtime; *\/ */
    /* 	/\* } *\/ */
    /* 	/\* fprintf(Interactive_output, "\n\tAS: time step adaptation (test): prev_dtime = %lf, cur_dtime = %lf\n", *\/ */
    /* 	/\*   time_ns_supg->prev_dtime, time_ns_supg->cur_dtime); *\/ */
    /*   } */

    // here possible time step length adaptations !!!

    // compute maximal, minimal and average CFL number for ns_supg_problem
    if(time_ns_supg->CFL_control == 1){

      cfl_limit = time_ns_supg->CFL_limit;
      time_step_length_mult = time_ns_supg->time_step_length_mult;

      fprintf(Interactive_output,
	      "\nCFL numbers before time step length control:\n");
      pdr_ns_supg_compute_CFL(PDC_NS_SUPG_ID, &cfl_min, &cfl_max, &cfl_ave);
      //fprintf(Interactive_output,
      //	    "CFL_min = %lf, CFL_max = %lf, CFL_average = %lf\n",
      //	    cfl_min,cfl_max,cfl_ave);
#ifdef PARALLEL
      MPI_Allreduce(&cfl_max, &cfl_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
#endif
      fprintf(Interactive_output,
	      "CFL_min = %lf, CFL_max = %lf, CFL_average = %lf\n",
	      cfl_min,cfl_max,cfl_ave);

      // check whether CFL is not too big
      if(cfl_max > cfl_limit){
	time_ns_supg->cur_dtime = time_ns_supg->cur_dtime/(4*time_step_length_mult);
	time_heat->cur_dtime = time_ns_supg->cur_dtime;
	time_vof->cur_dtime = time_ns_supg->cur_dtime;
      }
      // check whether CFL is not too small
      if(cfl_max < cfl_limit/(time_step_length_mult) ){
	if(cfl_max > 1.e-3){ // to exclude pure heat diffusion problems
	  time_ns_supg->cur_dtime=time_ns_supg->cur_dtime*time_step_length_mult;
	  time_heat->cur_dtime = time_ns_supg->cur_dtime;
	  time_vof->cur_dtime = time_ns_supg->cur_dtime;
	}
      }
    }

    time_ns_supg->cur_time += time_ns_supg->cur_dtime;
    time_heat->cur_time = time_ns_supg->cur_time;
    time_vof->cur_time = time_ns_supg->cur_time;

    fprintf(Interactive_output, "\n\nSolving time step %d (cur_dtime: %1.12lf, cur_time (t_(n+1)): %1.12lf)\n", 
      time_ns_supg->cur_step, time_ns_supg->cur_dtime, time_ns_supg->cur_time);
    
    // compute maximal, minimal and average CFL number for ns_supg_problem
    pdr_ns_supg_compute_CFL(PDC_NS_SUPG_ID, &cfl_min, &cfl_max, &cfl_ave);
#ifdef PARALLEL
    MPI_Allreduce(&cfl_max, &cfl_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
#endif
    fprintf(Interactive_output,
	    "CFL_min = %lf, CFL_max = %lf, CFL_average = %lf\n",
	    cfl_min,cfl_max,cfl_ave);

    // rewrite solution: 
    // current from previous time step becomes previous for current time step
    // soldofs_1 are the most recent solution dofs
    // at the beginning of time step they are rewritten to soldofs_3
    // that holds the solution from the previous time step
    apr_rewr_sol(field_ns_supg_id, 1, 3);	//rewrite current -> u_n (3)
    apr_rewr_sol(field_heat_id, 1, 3);	//rewrite current -> u_n (3)
    apr_rewr_sol(field_heat_dtdt_id, 1, 3);	//rewrite current -> u_n (3)
//     apr_rewr_sol(field_vof_id, 1, 3);	//rewrite current -> u_n (3)
// 	 apr_rewr_sol(field_mat_id, 1, 3);	//rewrite current -> u_n (3)

    //pdr_rewr_heat_dtdt_sol(field_heat_dtdt_id, 1, 2);	//rewrite current (1) to previous (2)
    /* update time dependent boundary conditions for NS problem*/
    pdv_ns_supg_heat_vof_current_problem_id = PDC_NS_SUPG_ID;
    pdr_ns_supg_update_timedep_bc(&problem_ns_supg->bc, 
				     time_ns_supg->cur_time);
    /* update time dependent boundary conditions for thermal problem */
    pdv_ns_supg_heat_vof_current_problem_id = PDC_HEAT_ID;
    pdr_heat_update_timedep_bc(&pdv_heat_problem.bc, time_heat->cur_time); 
    
    nonl_iter = 0;
    do {
      fprintf(Interactive_output, "\nSolving for nonlin. (nonl. iter step %d) convergence (time step %d).\n", nonl_iter+1, time_ns_supg->cur_step);
      
      // when forming linear system - soldofs_1 are equal to soldofs_2
      // after solving the system soldofs_1 are different than soldofs_2
      // before starting new solution soldofs_1 are rewritten to soldofs_2
      // simply: soldofs_1 are u_k+1, soldofs_2 are u_k and soldofs_3 are u_n
      apr_rewr_sol(field_ns_supg_id, 1, 2);	//rewrite current -> u_k (2)	
      apr_rewr_sol(field_heat_id, 1, 2);	//rewrite current -> u_k (2)
      //apr_rewr_sol(field_heat_dtdt_id, 1, 2);	//rewrite current -> u_k (2)
      //apr_rewr_sol(field_vof_id, 1, 2);	//rewrite current -> u_k (2)

      if (iadapt == 1) {
#ifdef PARALLEL
    int tone[2] = {1,1};
	/* initiate exchange tables for DOFs - for two fields, one level */
    appr_init_exchange_tables(pcv_nr_proc, pcv_my_proc_id, 2, tone);
#endif
	if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
	  sir_create(solver_ns_supg_id, PDC_NS_SUPG_ID); 
	}
	if(pdr_lins_i_params(PDC_HEAT_ID, 1) > 0){ // iterative solver
	  sir_create(solver_heat_id, PDC_HEAT_ID); 
	}
	iadapt = 0;
      }
      
      if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
	
	int ini_guess = 1; // get initial guess from data structure
	nr_iter = pdr_lins_i_params(PDC_NS_SUPG_ID, 2);
	conv_meas = pdr_lins_d_params(PDC_NS_SUPG_ID, 4);
	monitor =  pdr_lins_i_params(PDC_NS_SUPG_ID, 5);
	/*------------ CALLING ITERATIVE SOLVER ------------------*/
	sir_solve(solver_ns_supg_id, SIC_SOLVE, ini_guess, monitor, 
		  &nr_iter, &conv_meas, &conv_rate);
	fprintf(Interactive_output, 
		"\nAfter %d iterations of linear solver for ns_supg problem\n", 
		nr_iter); 
	fprintf(Interactive_output, 
		"Convergence measure: %15.12lf, convergence rate %lf\n", 
		conv_meas, conv_rate); 

      } else {

	sir_direct_solve_lin_sys(PDC_NS_SUPG_ID, SIC_SEQUENTIAL, 
				 solver_ns_supg_filename);	
      }

      if(pdr_lins_i_params(PDC_HEAT_ID, 1) > 0){ // iterative solver

	int ini_guess = 1; //  get initial guess from data structure 
	nr_iter = pdr_lins_i_params(PDC_HEAT_ID, 2);
	conv_meas = pdr_lins_d_params(PDC_HEAT_ID, 4);
	monitor =  pdr_lins_i_params(PDC_HEAT_ID, 5);
	/*------------ CALLING ITERATIVE SOLVER ------------------*/
	sir_solve(solver_heat_id, SIC_SOLVE, ini_guess, monitor, 
		  &nr_iter, &conv_meas, &conv_rate);
	fprintf(Interactive_output, 
		"\nAfter %d iterations of linear solver for heat problem\n", 
		nr_iter); 
	fprintf(Interactive_output, 
		"Convergence measure: %15.12lf, convergence rate %lf\n", 
		conv_meas, conv_rate); 
	
      } else {

	sir_direct_solve_lin_sys(PDC_HEAT_ID, SIC_SEQUENTIAL, 
				 solver_heat_filename);	
      }
      
      
      /* post-process solution using slope limiter */
      //if(slope) {
      //  iaux=pdr_slope_limit(problem_id); 
      // pdr_slope_limit for DG is archived in pdd_conv_diff/approx_dg/..._util.c
      //  if(iaux<0) { printf("\nError in slope!\n");getchar();}
      //}
      

      pdr_ns_supg_heat_vof_sol_diff_norm(1,2,&sol_norm_uk_ns_supg,&sol_norm_uk_heat);
      fprintf(Interactive_output, "Norm for ns_supg(u_k, prev. u_k): %10.8lf\n", 
	      sol_norm_uk_ns_supg);
      fprintf(Interactive_output, "Norm for heat (u_k, prev. u_k)  : %10.8lf\n", 
	      sol_norm_uk_heat);
      
      ++nonl_iter;
      
      if (nonl_iter >= nonl_ns_supg->max_iter
	  || nonl_iter >= nonl_heat->max_iter) {
	fprintf(Interactive_output, 
		"\nMax nonlinear iterations reached - breaking.\n");
	break;
      }
    } while (sol_norm_uk_ns_supg > nonl_ns_supg->conv_meas 
	     || sol_norm_uk_heat > nonl_heat->conv_meas );

    pdr_ns_supg_heat_vof_sol_diff_norm(1,3,&sol_norm_un_ns_supg,&sol_norm_un_heat);
    fprintf(Interactive_output, "\nNorm ns_supg (u_n, prev. u_n): %lf\n", 
	    sol_norm_un_ns_supg);
    fprintf(Interactive_output, "Norm heat (u_n, prev. u_n): %lf\n", 
	    sol_norm_un_heat);

   // create patches for material data query
    //nr_patches = utr_create_patches_small(PDC_NS_SUPG_ID, &pdv_patches_mat);
    fprintf(Interactive_output, "\tSolving heating-cooling rate\n");
	 //apr_rewr_sol(field_heat_dtdt_id, 1, 3);	//rewrite current -> u_k (2)
    pdr_heat_dtdt(1, 3); 	// compute and write Current=1 heat_dtdt field
    fprintf(Interactive_output, "\tSolving VOF \n");
    //apr_rewr_sol(field_vof_id, 1, 3);	//rewrite current -> u_n (3)
    //pdr_vof_dt(1, 3);						// compute and write Current=1 vof field from Old=3 vof field
    apr_rewr_sol(field_vof_id, 1, 3);	//rewrite current -> u_n (3)
	 apr_rewr_sol(field_mat_id, 1, 3);	//rewrite current -> u_n (3)
    pdr_vof_SV_dt(1, 3, Interactive_output);						// compute and write Current=1 vof field from Old=3 vof field
    //free(pdv_patches_mat);

    /* graphics data dumps */
    if (time_ns_supg->intv_graph > 0 && 
	time_ns_supg->cur_step % time_ns_supg->intv_graph == 0) {

      fprintf(Interactive_output, "(Writing field to disk (graphics)...)\n"); 
       
      pdr_ns_supg_heat_vof_write_paraview(Work_dir, 
			   Interactive_input, Interactive_output);
    }
    
    /* full data dumps (for restarting) */
    if (time_ns_supg->intv_dumpout > 0 
	&& time_ns_supg->cur_step % time_ns_supg->intv_dumpout == 0) {
      
      fprintf(Interactive_output, "\n(Writing field to disk - not implemented)\n");
      
      pdr_ns_supg_heat_vof_dump_data(Work_dir, 
			   Interactive_input, Interactive_output);

    }
    
   
    /* check for stop conditions */
    
    /* stop when final time reached */
    if (time_ns_supg->cur_time >= time_ns_supg->final_time) {
      fprintf(Interactive_output, "\nFinal time reached. Stopping.\n");
      break;
    }    
    
    /* cur_dtime could change through simulation (time step adaptation) */
    /* thus we need to check also if cur_step not bigger than final_step */
    if (time_ns_supg->cur_step >= time_ns_supg->final_step) {
      fprintf(Interactive_output, "\nFinal step reached. Stopping.\n");
      break;
    }
    
    /* stop if convergence reached (for stationary problems) */
    if (sol_norm_un_ns_supg < time_ns_supg->conv_meas
	&& sol_norm_un_heat < time_heat->conv_meas) {
      fprintf(Interactive_output, 
	      "\nNorm ns_supg (u_n, prev. u_n) below n_epsilon: %lf. Stopping.\n", 
	      time_ns_supg->conv_meas);
      fprintf(Interactive_output, 
	      "Norm heat (u_n, prev. u_n) below n_epsilon: %lf. Stopping.\n", 
	      time_heat->conv_meas);
      break;
    }
    
    // when time for adaptation
    if( (adpt_ns_supg->type>0 && adpt_ns_supg->interval>0 &&
	  (time_ns_supg->cur_step+1)%adpt_ns_supg->interval==0)
	||(adpt_heat->type>0 && adpt_heat->interval>0 &&
	   (time_heat->cur_step+1)%adpt_heat->interval==0)){
      
#ifdef PARALLEL
      /* free exchange tables for DOFs - for both fields: ns_supg and heat */
      appr_free_exchange_tables(2);
#endif
      
      if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
	/* free solver data structures */
	sir_free(solver_ns_supg_id);
      }
      if(pdr_lins_i_params(PDC_HEAT_ID, 1) > 0){ // iterative solver
 	sir_free(solver_heat_id);
      }


      pdr_ns_supg_heat_vof_adapt(Work_dir,
			     Interactive_input, Interactive_output);
      /* indicate to recreate block structure */
      iadapt=1;

    }

  }  //end loop over timesteps


  if (iadapt == 0) {
#ifdef PARALLEL
    /* free exchange tables for DOFs - for both fields: ns_supg and heat */
    appr_free_exchange_tables(2);
#endif
    
    
    if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
      //if(strcmp(solver_name, "KRYLOW_BLITER")==0){
      sir_free(solver_ns_supg_id); 
    }
    if(pdr_lins_i_params(PDC_HEAT_ID, 1) > 0){ // iterative solver
      sir_free(solver_heat_id); 
    }
    
  
  }
  
  if(pdr_lins_i_params(PDC_HEAT_ID, 1) > 0){ // iterative solver
    sir_destroy(solver_heat_id);
  }
  if(pdr_lins_i_params(PDC_NS_SUPG_ID, 1) > 0){ // iterative solver
    sir_destroy(solver_ns_supg_id);
  }

  return;
}
