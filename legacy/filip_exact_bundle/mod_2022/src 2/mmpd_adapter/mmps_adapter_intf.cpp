/************************************************************************
File mmps_prism_intf.c - implementation of parallel interface routines for 
                        meshes of prismatic elements 

Contains definitions of interface routines:   
  mmpr_init_mesh - to initialize the parallel mesh data structure 

  mmpr_el_owner - to get an owning processor identifier of an element 
  mmpr_el_set_owner - to set an owning processor identifier of an element
  mmpr_fa_owner - to get an owning processor identifier of a face
  mmpr_fa_set_owner - to set an owning processor identifier of a face
  mmpr_ed_owner - to get an owning processor identifier of an edge 
  mmpr_ed_set_owner - to set an owning processor identifier of an edge
  mmpr_ve_owner - to get an owning processor identifier of a vertex
  mmpr_ve_set_owner - to set an owning processor identifier of a vertex
  mmpr_el_id_at_owner - to get an local identifier of an element 
  mmpr_el_set_id_at_owner - to set an local identifier of an element
  mmpr_fa_id_at_owner - to get an local identifier of a face
  mmpr_fa_set_id_at_owner - to set an local identifier of a face
  mmpr_ed_id_at_owner - to get an local identifier of an edge 
  mmpr_ed_set_id_at_owner - to set an local identifier of an edge
  mmr_ve_id_at_owner - to get an local identifier of a vertex
  mmpr_ve_set_id_at_owner - to set an local identifier of a vertex

  mmpr_init_ref - to initialize the parallel process of refinement
  mmpr_refine - to refine an element or the whole mesh in parallel
  mmpr_derefine - to derefine an element or the whole mesh in parallel
  mmpr_final_ref - to finalize the process of parallel refinement 
  mmpr_free_mesh - to free space allocated for parallel mesh data structure
------------------------------  			
History:        
	02.2002 - Krzysztof Banas, initial version
	09.2012 - Kazimierz Michalik, parmetis version
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<assert.h>
#include <boost/foreach.hpp>
#include<metis.h>
#include<parmetis.h>
#include<algorithm>
#include<stack>
#include<set>

/* interface for the mesh manipulation module */
#include "mmh_intf.h"	

/* interface for the paralle mesh manipulation module */
#include "mmph_intf.h"	

#include "uth_intf.h"
#include "pch_intf.h"

#include "mmph_adapter.h"

/* metis adapter */
#include "ddd_metis_adapter/ddh_metis_adapter.h"



#ifdef __cplusplus
extern "C"{
#endif
const int MMPC_ADAPT_REF_MSG_ID = 'R';
const int MMPC_ADAPT_HS_MSG_ID = 'H';
int       mmpv_partition_method = MMPC_METIS_PART_METHOD;   /*  method that is used to generate partitions */

std::vector<mmpt_mesh>  mmpv_meshes;        /* array of meshes */


int mmpv_nr_sub;
int mmpv_my_proc_id;

/*------------------------------------------------------------
  mmpr_init_mesh - to clear the parallel meshes data structures
------------------------------------------------------------*/
void mmpr_atexit_clear_meshes() {
    for( auto & m : mmpv_meshes) {
        m.clear();
    }
    mmpv_meshes.clear();
}

/*------------------------------------------------------------
  mmpr_init_mesh - to initialize the parallel mesh data structure
------------------------------------------------------------*/
int mmpr_init_mesh(  /* returns: >0 - Mesh ID, <0 - error code */
  int Control,
  int Mesh_id,
  int Nr_proc,
  int My_proc_id
  )
{

/*++++++++++++++++ executable statements ++++++++++++++++*/

  assert(pcr_is_parallel_initialized() == 1);

  // Register cleaning function.
  atexit(mmpr_atexit_clear_meshes);

  mmpv_nr_sub = Nr_proc;
  mmpv_my_proc_id = My_proc_id;

  mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
  pmesh.clear();

  mmpr_init_data(Mesh_id,Nr_proc);

//  mf_check(pmesh.nodes.size() == mmr_get_nr_node(Mesh_id)+1, "Incomplete paralle mesh data!");
//  mf_check(pmesh.edges.size() == mmr_get_nr_edge(Mesh_id)+1, "Incomplete paralle mesh data!");
//  mf_check(pmesh.faces.size() == mmr_get_nr_face(Mesh_id)+1, "Incomplete paralle mesh data!");
//  mf_check(pmesh.elems.size() == mmr_get_nr_elem(Mesh_id)+1, "Incomplete paralle mesh data!");

/// At this point each process:
/// 1. Already readed own unique mesh file (if exist).
/// So each process can have own unique mesh part readed from file or no mesh at all.
/// 2. Have correct knowlege (in (mmpt_mesh*) pmesh object) about total number of all mesh enities
/// including all other unique files readed by other processes.
///
/// 1. Now mesh have to be repartitioned.
/// 2. Than parts distribution must be done between processes.
/// 3. After improving, the overlay have to be created.

  mmpr_create_subdomains(Mesh_id, Control);

//  mf_check(pmesh.nodes.size() == mmr_get_nr_node(Mesh_id)+1, "Incomplete paralle mesh data!");
//  mf_check(pmesh.edges.size() == mmr_get_nr_edge(Mesh_id)+1, "Incomplete paralle mesh data!");
//  mf_check(pmesh.faces.size() == mmr_get_nr_face(Mesh_id)+1, "Incomplete paralle mesh data!");
//  mf_check(pmesh.elems.size() == mmr_get_nr_elem(Mesh_id)+1, "Incomplete paralle mesh data!");

  //mmpr_balance_load();

  return Mesh_id;
}


/*---------------------------------------------------------
  mmpr_create_subdomains - to decompose the mesh and create subdomains
---------------------------------------------------------*/
int mmpr_create_subdomains( /* returns: >=0 - success code, <0 - error code */
  int Mesh_id,      /* in: approximation field ID  */
  int Control        /* in: generation level as basis for decomposition */
  )
{
    assert(pcr_is_parallel_initialized() == 1);

#ifdef DEBUG_MMM
     printf("mmpr_create_subdomains: starting for Mesh_id %d, Control %d\n",Mesh_id,Control);
#endif

    // All partitioning, improving etc. is done first using PCSR mesh representation.
    // When all necessary steps are done, ddr_PCSR_update_mesh is called.
    // During that steps squential mesh is correct, but held in previous state (inconsistiend with PCSR).
    // Calling any "_PCSR_" function invalidates state of parallel mesh,
    // because of changes in state of parallel data structures.
    // Therefore calling any mmpr_"getters" before updating squential mesh to PCSR (ddr_PCSR_update_mesh)
    // may result in error.

    int	rc = ddr_PCSR_create_initial_distribution(Mesh_id);
    mf_check(rc>=0,"Failed to create initial mesh distribution!");

    //rc = ddr_PCSR_partition_mesh(Mesh_id, mmpv_nr_sub, MAC_USE_KWAY_GRAPH_PART_TOOL);
    //mf_check(rc>=0,"Failed to partition mesh!");

    //rc = ddr_PCSR_improve_partitioning(Mesh_id);
    //mf_check(rc>=0,"Failed to create improve partitioning!");

    rc = ddr_PCSR_create_IPC(Mesh_id);
    mf_check(rc>=0,"Failed to create inter_subdomain_connectivity!");

    ddr_IPC_expand_overlap(Mesh_id);
    mf_check(rc>=0,"Failed to expand mesh overlap!");

    return rc;
}


/*------------------------------------------------------------
  mmpr_balance_load - to balance load between processes using ParMetis
------------------------------------------------------------*/
int ddr_PCSR_balance_load(
  int Mesh_id  /* in: mesh ID */
  )
{
    // This method is needed only if more than 1 process is working.
    int rc=METIS_OK;
    mmpt_data & data = mmpr_select_mesh(Mesh_id)->data;
    if(pcr_nr_proc() > 1) {
        assert(pcr_is_parallel_initialized() == 1);
        assert(!data.mp.PCSR.empty());
        assert(data.mp.options != nullptr );
        assert(!data.mp.part.empty() );

#ifdef DEBUG_MMM
     printf("mmpr_balance_load: starting for Mesh_id %d\n",Mesh_id);
#endif


        // Once created structures are still valid.
        // This is because we create them for initial elements,
        // and that is NOT CHANGING during computations.
        // The only tables that needs updating
        // are vwgt(vertice weight) and vsize (v. redistribution cost) table.
        // For each initial element,
        // its' vsize[i] have to correspond with number
        // of DOFs for this initial element and all its children.
        // We assume that vsize = vwgt.
        //assert(mmpv_data.mp.vsize != nullptr );
        rc = ParMETIS_V3_AdaptiveRepart(
                    data.mp.PCSR.vtxDist(),
                    data.mp.PCSR.xadj(),
                    data.mp.PCSR.adjncy(),
                    data.mp.PCSR.vwgt(),
                    data.mp.PCSR.vwgt(),
                    data.mp.PCSR.adjwgt(),
                    & data.mp.wgtflag,
                    & data.mp.numflag,
                    & data.mp.ncon,
                    & data.mp.nparts,
                    data.mp.tpwgts.data(),
                    & data.mp.ubvec,
                    & data.mp.itr,
                    data.mp.options,
                    & data.mp.edgecut,
                    data.mp.part.data(),
                    & data.mp.comm);


          ddr_IPC_expand_overlap(Mesh_id);

    }
    return rc;
}

/*---------------------------------------------------------
  mmpr_select_mesh - to select the proper mesh   
---------------------------------------------------------*/
mmpt_mesh* mmpr_select_mesh( /* returns pointer to the chosen mesh */
			   /* to avoid errors if input is not valid */
			   /* it returns the pointer to the current mesh */
  int Mesh_id    /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  )
{

    assert(Mesh_id > 0);

    if(static_cast<unsigned int>(Mesh_id) >= mmpv_meshes.size()) {
        mmpv_meshes.resize(Mesh_id+1);
    }
    assert(static_cast<unsigned int>(Mesh_id) <= mmpv_meshes.size());
    return & mmpv_meshes[Mesh_id];
}


/*--------------------------------------------------------------------------
  mmpr_el_owner - to get an owning processor identifier of an element 
---------------------------------------------------------------------------*/
int mmpr_el_owner(/* returns: >0 -an owning processor identifier of an element */
	         /*          <0 - error code */ 
  int Mesh_id,	 /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int El         /* in: global (within the subdomain) element ID */
  )
{
    assert(Mesh_id > 0);
    assert(El > 0);
    assert(static_cast<unsigned int>(El) <= mmr_get_max_elem_id(Mesh_id));

    int owner = pcv_my_proc_id;
    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_elems;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(El);
    if( it != omap.end() ) {
        owner = it->second.owner;
    }
    mf_check_debug(owner > 0, "Incorrect element owner value (%d)", owner);
    return owner;
}

///*--------------------------------------------------------------------------
//  mmpr_el_set_owner - to set an owning processor identifier of an element
//---------------------------------------------------------------------------*/
//int mmpr_el_set_owner( /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,	/* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  int El,	/* in: global (within the subdomain) element ID */
//  int Owner /* in: owning processor identifier of the element */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(El > 0);
///* auxiliary variables */


///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mmpt_mesh& pmesh = *mmpr_select_mesh(Mesh_id);
//  assert(static_cast<unsigned int>(El) < pmesh.elems.size());
//  pmesh.elems[El].owner = Owner;

//  return(1);
//}

/*--------------------------------------------------------------------------
  mmpr_fa_owner - to get an owning processor identifier of a face
---------------------------------------------------------------------------*/
int mmpr_fa_owner(  /* returns: >0 - an owning processor identifier of a face */
	          /*          <0 - error code */ 
  int Mesh_id,	/* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int Fa        /* in: global (within the subdomain)  ID */
  )
{
    assert(Mesh_id > 0);
    assert(Fa > 0);
    assert(static_cast<unsigned int>(Fa) <= mmr_get_max_face_id(Mesh_id));

    int owner = pcv_my_proc_id;
    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_faces;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(Fa);
    if( it != omap.end() ) {
        owner =  it->second.owner;
    }
    mf_check_debug(owner > 0, "Incorrect face owner value (%d)", owner);
    return owner;
}

///*--------------------------------------------------------------------------
//  mmpr_fa_set_owner - to set an owning processor identifier of a face
//---------------------------------------------------------------------------*/
//int mmpr_fa_set_owner(  /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,  /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  int Fa,       /* in: global (within the subdomain) face ID */
//  int Owner      /* in: owning processor identifier of the face */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(Fa > 0);
///* auxiliary variables */
//  mmpt_mesh* mesh;

///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mesh = mmpr_select_mesh(Mesh_id);

//  assert(static_cast<unsigned int>(Fa) < mesh->faces.size());
//  mesh->faces[Fa].owner = Owner;

//  return(1);
//}

/*--------------------------------------------------------------------------
  mmpr_ed_owner - to get an owning processor identifier of an edge 
---------------------------------------------------------------------------*/
int mmpr_ed_owner(  /* returns: >0 an owning processor identifier of an edge */
	          /*          <0 - error code */ 
  int Mesh_id,    /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int Ed          /* in: global (within the subdomain) edge ID */
  )
{
    assert(Mesh_id > 0);
    assert(Ed > 0);
    assert(static_cast<unsigned int>(Ed) <= mmr_get_max_elem_id(Mesh_id));

    int owner = pcv_my_proc_id;
    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_edges;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(Ed);
    if( it != omap.end() ) {
        owner =  it->second.owner;
    }
    mf_check_debug(owner > 0, "Incorrect edge owner value (%d)", owner);
    return  owner;
}

///*--------------------------------------------------------------------------
//  mmpr_ed_set_owner - to set an owning processor identifier of an edge
//---------------------------------------------------------------------------*/
//int mmpr_ed_set_owner(  /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,  /* in: mesh ID or 0 (MMPC_CUR_MESH_ID) for the current mesh */
//  int Ed,       /* in: global (within the subdomain) edge ID */
//  int Owner      /* in: owning processor identifier of the edge */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(Ed > 0);
///* auxiliary variables */
//  mmpt_mesh* mesh;

///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mesh = mmpr_select_mesh(Mesh_id);
//  assert(static_cast<unsigned int>(Ed) < mesh->edges.size());
//  mesh->edges[Ed].owner = Owner;

//  return(1);
//}

/*--------------------------------------------------------------------------
  mmpr_ve_owner - to get an owning processor identifier of a vertex
---------------------------------------------------------------------------*/
int mmpr_ve_owner( /* returns: >0 - an owning processor identifier of a vertex */
	         /*          <0 - error code */ 
  int Mesh_id,  /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int Ve        /* in: global (within the subdomain) vertex ID */
  )
{
    assert(Mesh_id > 0);
    assert(Ve > 0);
    assert(static_cast<unsigned int>(Ve) <= mmr_get_max_node_id(Mesh_id));
    int owner = pcv_my_proc_id;
    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_nodes;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(Ve);
    if( it != omap.end() ) {
        owner = it->second.owner;
    }
    mf_check_debug(owner > 0, "Incorrect vertex owner value (%d)", owner);
    return owner;

}

///*--------------------------------------------------------------------------
//  mmpr_ve_set_owner - to set an owning processor identifier of a vertex
//---------------------------------------------------------------------------*/
//int mmpr_ve_set_owner(  /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,  /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  int Ve,       /* in: global (within the subdomain) vertex ID */
//  int Owner      /* in: owning processor identifier of the vertex */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(Ve > 0);
///* auxiliary variables */
//  mmpt_mesh* mesh;

///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mesh = mmpr_select_mesh(Mesh_id);
//  assert(static_cast<unsigned int>(Ve) < mesh->nodes.size());
//  mesh->nodes[Ve].owner = Owner;

//  return(1);
//}

/*--------------------------------------------------------------------------
  mmpr_el_id_at_owner - to get an local identifier of an element 
---------------------------------------------------------------------------*/
int mmpr_el_id_at_owner( /* returns: >0 - an local identifier of an element */
	         /*          <0 - error code */ 
  int Mesh_id,	 /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int El         /* in: global (within the subdomain) element ID */
  )
{
    assert(Mesh_id > 0);
    assert(El > 0);
    assert(static_cast<unsigned int>(El) <= mmr_get_max_elem_id(Mesh_id));

    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_elems;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(El);
    return  it == omap.end() ? El : it->second.id_at_owner;
}

///*--------------------------------------------------------------------------
//  mmpr_el_set_id_at_owner - to set an local identifier of an element
//---------------------------------------------------------------------------*/
//int mmpr_el_set_id_at_owner( /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,	/* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  int El,	/* in: global (within the subdomain) element ID */
//  int Id_At_Owner /* in: local identifier of the element */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(El > 0);
///* auxiliary variables */
//  mmpt_mesh* mesh;

///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mesh = mmpr_select_mesh(Mesh_id);
//  assert(static_cast<unsigned int>(El) < mesh->elems.size());
//  mesh->elems[El].id_at_owner = Id_At_Owner;

//  return(1);
//}

/*--------------------------------------------------------------------------
  mmpr_fa_id_at_owner - to get an local identifier of a face
---------------------------------------------------------------------------*/
int mmpr_fa_id_at_owner(  /* returns: >0 - an local identifier of a face */
	          /*          <0 - error code */ 
  int Mesh_id,	/* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int Fa        /* in: global (within the subdomain)  ID */
  )
{
    assert(Mesh_id > 0);
    assert(Fa > 0);
    assert(static_cast<unsigned int>(Fa) < mmr_get_max_face_id(Mesh_id));

    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_faces;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(Fa);
    return  it == omap.end() ? Fa : it->second.id_at_owner;
}

///*--------------------------------------------------------------------------
//  mmpr_fa_set_id_at_owner - to set an local identifier of a face
//---------------------------------------------------------------------------*/
//int mmpr_fa_set_id_at_owner(  /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,  /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  int Fa,       /* in: global (within the subdomain) face ID */
//  int Id_At_Owner      /* in: local identifier of the face */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(Fa > 0);
///* auxiliary variables */
//  mmpt_mesh* mesh;

///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mesh = mmpr_select_mesh(Mesh_id);
//  assert(static_cast<unsigned int>(Fa) < mesh->faces.size());
//  mesh->faces[Fa].id_at_owner = Id_At_Owner;

//  return(1);
//}

/*--------------------------------------------------------------------------
  mmpr_ed_id_at_owner - to get an local identifier of an edge 
---------------------------------------------------------------------------*/
int mmpr_ed_id_at_owner(  /* returns: >0 an local identifier of an edge */
	          /*          <0 - error code */ 
  int Mesh_id,    /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int Ed          /* in: global (within the subdomain) edge ID */
  )
{
    assert(Mesh_id > 0);
    assert(Ed > 0);
    assert(static_cast<unsigned int>(Ed) < mmr_get_max_edge_id(Mesh_id));

    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_edges;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(Ed);
    return  it == omap.end() ? Ed : it->second.id_at_owner;
}

///*--------------------------------------------------------------------------
//  mmpr_ed_set_id_at_owner - to set an local identifier of an edge
//---------------------------------------------------------------------------*/
//int mmpr_ed_set_id_at_owner(  /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,  /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  int Ed,       /* in: global (within the subdomain) edge ID */
//  int Id_At_Owner      /* in: local identifier of the edge */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(Ed > 0);
///* auxiliary variables */
//  mmpt_mesh* mesh;

///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mesh = mmpr_select_mesh(Mesh_id);
//  assert(static_cast<unsigned int>(Ed) < mesh->edges.size());
//  mesh->edges[Ed].id_at_owner = Id_At_Owner;

//  return(1);
//}

/*--------------------------------------------------------------------------
  mmr_ve_id_at_owner - to get an local identifier of a vertex
---------------------------------------------------------------------------*/
int mmpr_ve_id_at_owner( /* returns: >0 - an local identifier of a vertex */
	         /*          <0 - error code */ 
  int Mesh_id,  /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  int Ve        /* in: global (within the subdomain) vertex ID */
  )
{
    assert(Mesh_id > 0);
    assert(Ve > 0);
    assert(static_cast<unsigned int>(Ve) < mmr_get_max_edge_id(Mesh_id));

    const mmpt_mesh::mmpt_owners_map & omap = mmpr_select_mesh(Mesh_id)->ovl_nodes;
    const mmpt_mesh::mmpt_owners_map::const_iterator it = omap.find(Ve);
    return  it == omap.end() ? Ve : it->second.id_at_owner;
}

///*--------------------------------------------------------------------------
//  mmpr_ve_set_id_at_owner - to set an local identifier of a vertex
//---------------------------------------------------------------------------*/
//int mmpr_ve_set_id_at_owner(  /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id,  /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  int Ve,       /* in: global (within the subdomain) vertex ID */
//  int Id_At_Owner      /* in: local identifier of the vertex */
//  )
//{
//    assert(Mesh_id > 0);
//    assert(Ve > 0);
///* auxiliary variables */
//  mmpt_mesh* mesh;

///*++++++++++++++++ executable statements ++++++++++++++++*/

///* select the proper mesh data structure */
//  mesh = mmpr_select_mesh(Mesh_id);
//  assert(static_cast<unsigned int>(Ve) < mesh->nodes.size());
//  mesh->nodes[Ve].id_at_owner = Id_At_Owner;

//  return(1);
//}



/*------------------------------------------------------------
  mmpr_init_ref - to initialize the process of refinement
------------------------------------------------------------*/
int mmpr_init_ref(  /* returns: >=0 - success code, <0 - error code */
  int Mesh_id	/* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  )
{

/* local variables */
  mmpt_mesh&  pmesh=*mmpr_select_mesh(Mesh_id);
//  int iaux;

///*++++++++++++++++ executable statements ++++++++++++++++*/

//  /* select the proper mesh data structure */
//  par_mesh = mmpr_select_mesh( Mesh_id);

//  iaux=mmr_get_nr_elem(Mesh_id);
//  par_mesh->ref_loc.elem = (int*) malloc(29*iaux*sizeof(int));
// if(!par_mesh->ref_loc.elem){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->ref_loc.elem[0]=0;
//  par_mesh->ref_ali.elem = (int*) malloc(29*iaux*sizeof(int));
// if(!par_mesh->ref_ali.elem){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->ref_ali.elem[0]=0;

//  iaux=mmr_get_nr_face(Mesh_id);
//  par_mesh->ref_loc.face = (int*) malloc(11*iaux*sizeof(int));
// if(!par_mesh->ref_loc.face){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->ref_loc.face[0]=0;
//  par_mesh->ref_ali.face = (int*) malloc(11*iaux*sizeof(int));
// if(!par_mesh->ref_ali.face){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->ref_ali.face[0]=0;

//  par_mesh->del_loc.face = (int*) malloc(iaux*sizeof(int));
// if(!par_mesh->del_loc.face){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->del_loc.face[0]=0;

//  iaux=mmr_get_nr_edge(Mesh_id);
//  par_mesh->ref_loc.edge = (int*) malloc(4*iaux*sizeof(int));
// if(!par_mesh->ref_loc.edge){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->ref_loc.edge[0]=0;
//  par_mesh->ref_ali.edge = (int*) malloc(4*iaux*sizeof(int));
// if(!par_mesh->ref_ali.edge){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->ref_ali.edge[0]=0;

//  par_mesh->del_loc.edge = (int*) malloc(iaux*sizeof(int));
// if(!par_mesh->del_loc.edge){
//   printf("Not enough space for allocating mmpv_ref.... ! Exiting\n");
//   exit(-1);
// }
//  par_mesh->del_loc.edge[0]=0;

// Creating snapshot of boundary elements fam info.
  pmesh.data.boundary_elems_snapshot.clear();
  for(const auto & link : pmesh.data.ipc_links) {
      int neig[2]={0};
      int sons[MMC_MAXELSONS+1]={0};
      mmr_fa_eq_neig(Mesh_id,link.first,neig,NULL,NULL);
      //mmr_el_fam_all(Mesh_id,neig[0],sons);
      pmesh.data.boundary_elems_snapshot.insert(neig[0]);
      //pmesh.data.boundary_elems_snapshot[neig[0]] = sons[0];
      mfp_debug("Boundary snapshot of elem %d.",neig[0]);
  }

  return(0);
}

/*------------------------------------------------------------
mmpr_update_ref_list - to update list of refined elements due to irregularity
                      constraint using inter-processor communication
This function should be called BEFORE actual adaptation occurs.
------------------------------------------------------------*/
int mmpr_update_ref_list(int Mesh_id, int * Nr_ref, int **Ref_list)
{
    if(*Nr_ref < 1) {
        mf_log_warn("Empty refinement list passed to updateing!");
        return 0;
    }
    mf_check_mem(Ref_list);
    mf_check_mem(*Ref_list);

    if( (*Ref_list)[0] == MMC_DO_UNI_REF
     || (*Ref_list)[0] == MMC_DO_UNI_DEREF ) {
        return 0;
    }


    // At first we have to extend ref list to capture elements forced to break by 1--inregularity rules.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    int nr_ref = *Nr_ref;
    int *ref_list = *Ref_list;
    int max_gen_diff = mmr_get_max_gen_diff(Mesh_id);
    int max_gen = mmr_get_max_gen(Mesh_id);

    std::set<int>    new_refs(ref_list,ref_list+nr_ref);

    std::stack<int> to_check;
    for(int e=0; e < nr_ref; ++e) {
        to_check.push(ref_list[e]);

        // Check 1--irregularity constraints
        while(! to_check.empty()) {
            const int elem=to_check.top();
            to_check.pop();

            int parent = mmr_el_fam(Mesh_id, elem, NULL, NULL);

            if( (MMC_ACTIVE == mmr_el_status(Mesh_id,elem))
            &&  (max_gen > mmr_el_gen(Mesh_id,elem))
            &&  (parent != MMC_NO_FATH) ) {

                int el_edges[MMC_MAXELEDGES+1]={0};
                mmr_el_edges(Mesh_id, parent, el_edges);

                /* for all parent's edges */
                int nr_edges = el_edges[0];

                for(int ied=1;ied<=nr_edges;ied++){

                  int ned = el_edges[ied];

                  int edge_elems[20+1] = {0}; // with MMC_MAX_EDGE_ELEMS is not compiling...
                  mmr_edge_elems(Mesh_id, ned, edge_elems);

                  for(int i=1;i<=edge_elems[0];i++){

                    if( edge_elems[i]!=elem && mmr_el_status(Mesh_id,edge_elems[i])==MMC_ACTIVE) {
                      new_refs.insert(edge_elems[i]);
                      mfp_debug("Adding %d to ref list (irregularity)!",edge_elems[i]);
                      to_check.push(edge_elems[i]);


                    }
                  }
                }

            }

        }
    }

    // Now we have complete list of local refinements.
    // Check if boundary are is also changing.
    std::vector<int>   ref_bound_elems(pmesh.data.boundary_elems_snapshot.size());
    ref_bound_elems.erase(std::set_intersection(pmesh.data.boundary_elems_snapshot.begin(),pmesh.data.boundary_elems_snapshot.end(),
                          new_refs.begin(),new_refs.end(), ref_bound_elems.begin() ),
                          ref_bound_elems.end());




    // Finally ref list is extended to correctly capture overlap refinements.
    ///////////////////////////////////////////////////////////////////////////////////////////////////


    std::unordered_map<int,std::vector<mmpt_ipid> >    elems_to_ref_per_proc;
    // Create global id, which will be sended.
    for(const auto & boundary_el: ref_bound_elems ) {
        mmpt_ipid glob_id;
        auto it_ovl = pmesh.ovl_elems.find(boundary_el);
        if(it_ovl != pmesh.ovl_elems.end()) {
            glob_id.owner = it_ovl->second.owner;
            glob_id.id_at_owner = it_ovl->second.id_at_owner;
        }
        else {
            glob_id.owner = pcv_my_proc_id;
            glob_id.id_at_owner = boundary_el;
        }

        int faces[1+MMC_MAXELFAC]={0};
        mmr_el_faces(Mesh_id,boundary_el,faces,NULL);

        for(int f=1; f <= faces[0]; ++f) {
            auto it = pmesh.data.ipc_links.find(faces[f]);
            if(it != pmesh.data.ipc_links.end()) {
                elems_to_ref_per_proc[it->second.owner].push_back(glob_id);
                mfp_debug("Elem %d will be propagated to proc %d!",boundary_el,it->second.owner);
            }
        }
    }

    mfp_check_debug(pmesh.data.ipc[pcv_my_proc_id]==0,"Process cannot link to itself!");

    // Communication between processes.
    // Handshaking: telling neighbours how many we will send.
    std::unordered_map<int,int>     n_elems_to_recv;
    for(auto & link : pmesh.data.ipc) {
        if(link.first != pcv_my_proc_id) {
            int size = elems_to_ref_per_proc[link.first].size();
            mfp_debug("Sending MMPC_ADAPT_HS_MSG_ID with value %d to proc %d",size,link.first);
            pcr_send_int(link.first, MMPC_ADAPT_HS_MSG_ID, 1, &size );


            int to_recv=0;
            pcr_receive_int(link.first,MMPC_ADAPT_HS_MSG_ID,1,& to_recv);
            mfp_debug("Recieved MMPC_ADAPT_HS_MSG_ID from proc %d with value %d",link.first,to_recv);
            n_elems_to_recv[link.first] = to_recv;
        }
    }

    // Transfering.
    for( auto & elems_to_ref : elems_to_ref_per_proc ) {
        if(elems_to_ref.first != pcv_my_proc_id) {
            if( !elems_to_ref.second.empty() ) {
                // HACK to send mmpt_ipid's.
                mfp_debug("Sending elements ipids to proc %d",elems_to_ref.first);
                pcr_send_int( elems_to_ref.first, MMPC_ADAPT_REF_MSG_ID, 2*elems_to_ref.second.size(), reinterpret_cast<int*>(elems_to_ref.second.data()) );
            }

            if( n_elems_to_recv[elems_to_ref.first] > 0) {
                std::vector<mmpt_ipid>  elem2adapt(n_elems_to_recv[elems_to_ref.first]);
                // HACK to revc mmpt_ipid's
                pcr_receive_int(elems_to_ref.first, MMPC_ADAPT_REF_MSG_ID, 2*elem2adapt.size(),reinterpret_cast<int*>(elem2adapt.data())) ;
                mfp_debug("Recived ipids from %d",elems_to_ref.first);
                elems_to_ref.second = elem2adapt; // IMPORTANT!
            }
        }
    }

    pcr_barrier();

    // Extend refinement list with recived elem ids.
    for( auto & elems_to_ref : elems_to_ref_per_proc ) {
        if( (elems_to_ref.first != pcv_my_proc_id) && (!elems_to_ref.second.empty()) ) {
            for(mmpt_ipid & glob_id : elems_to_ref.second) {
                int el_id = glob_id.id_at_owner;
                if(glob_id.owner != pcv_my_proc_id) {
                    auto found = std::find_if(pmesh.ovl_elems.begin(),pmesh.ovl_elems.end(),
                                 [&glob_id]( mmpt_mesh::mmpt_owners_map::value_type & val) {
                       return val.second.owner==glob_id.owner && val.second.id_at_owner==glob_id.id_at_owner;
                    });
                    mfp_check_debug(!(found == pmesh.ovl_elems.end()),"Global id (%d,%d) not found!",
                                    glob_id.owner, glob_id.id_at_owner );
                }
                mfp_debug("Adding %d to ref list (from proc %d).",el_id,elems_to_ref.first);
                new_refs.insert(el_id);
            }
        }
    }


    if(! new_refs.empty()) {
        int new_size = new_refs.size();
        if(new_size > nr_ref) {
            ref_list = (int*)realloc(ref_list, new_size * sizeof(int));
            mf_check_mem(ref_list);

            std::copy(new_refs.begin(),new_refs.end(),ref_list);

            new_refs.clear();

            nr_ref = new_size;
            *Nr_ref = new_size;
            *Ref_list = ref_list;

            mf_check_mem(*Ref_list);
        }
    }
     // !if bound_elems_empty
    // Now list is ready to be passed to actual adaptation.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Note: after adaptation the corrections of overlap dofs ipid's have to be executed!
    // (mmpr_final_ref)
    return 0;
}

/*------------------------------------------------------------
  mmpr_final_ref - to finalize the process of parallel refinement 
------------------------------------------------------------*/
int mmpr_final_ref(  /* returns: >=0 - success code, <0 - error code */
  int Mesh_id 	/* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  )
{
/* local variables */
 // mmpt_mesh* par_mesh=nullptr;

/*++++++++++++++++ executable statements ++++++++++++++++*/

/* select the proper mesh data structure */
  //par_mesh = mmpr_select_mesh(Mesh_id);
    
  /* free lists with the last refinement info */
//  UTM_SAFE_FREE_PTR(par_mesh->ref_loc->elem);
//  UTM_SAFE_FREE_PTR(par_mesh->ref_loc->face);
//  UTM_SAFE_FREE_PTR(par_mesh->ref_loc->edge);
//  UTM_SAFE_FREE_PTR(par_mesh->ref_ali->elem);
//  UTM_SAFE_FREE_PTR(par_mesh->ref_ali->face);
//  UTM_SAFE_FREE_PTR(par_mesh->ref_ali->edge);
//  UTM_SAFE_FREE_PTR(par_mesh->del_loc->face);
//  UTM_SAFE_FREE_PTR(par_mesh->del_loc->edge);



  return(0);
}

///*------------------------------------------------------------
//  mmpr_free_mesh - to free space allocated for parallel mesh data structure
//------------------------------------------------------------*/
//int mmpr_free_mesh(  /* returns: >=0 - success code, <0 - error code */
//  int Mesh_id 	/* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
//  )
//{

/////* local variables */
////  mmpt_mesh* par_mesh=nullptr;

/////*++++++++++++++++ executable statements ++++++++++++++++*/

////  /* select the proper mesh data structure */
////  par_mesh = mmpr_select_mesh( Mesh_id);

//////  UTM_SAFE_FREE_PTR(par_mesh->nodes);
//////  UTM_SAFE_FREE_PTR(par_mesh->edges);
//////  UTM_SAFE_FREE_PTR(par_mesh->faces);
//////  UTM_SAFE_FREE_PTR(par_mesh->elems);
////  //if(par_mesh->ref_loc != nullptr) free(par_mesh->ref_loc);
////  //if(par_mesh->ref_ali != nullptr) free(par_mesh->ref_ali);
////  //if(par_mesh->del_loc != nullptr) free(par_mesh->del_loc);
////  //if(par_mesh->mesh_ovls != nullptr) free(par_mesh->mesh_ovls);

//  return(0);
//}

/*------------------------------------------------------------
  mmpr_ipc_n_connected_procs - to get number of neighburing subdomains assigned to other procs.
------------------------------------------------------------*/
int mmpr_ipc_n_connected_procs( // returns number of procs connected with caller proc
        const int Mesh_id //in: mesh id
        )
{
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    return pmesh.data.ipc.size();
}

/*------------------------------------------------------------
  mmpr_interproc_connectivity - to get detailed info about neighbouring procs
------------------------------------------------------------*/
int mmpr_interproc_connectivity( // returns number of procs connected with caller proc
        const int Mesh_id,  //in: mesh id
        int * Connected_proc_ids, //out:[mmpr_ipc_n_connected_procs]
                                   // connected negibouring procs ids
        int * Connected_proc_n_links //out:[mmpr_ipc_n_connected_procs](can be nullptr)
                                        // number of links (communication diameter)
                                        // for each of Connected_proc_ids
        )
{
    assert(Connected_proc_ids != nullptr);

    mmpt_data & data = mmpr_select_mesh(Mesh_id)->data;

    int * it_conected_proc_ids = Connected_proc_ids;
    int * it_connected_proc_n_links = Connected_proc_n_links;

    BOOST_FOREACH(const mmpt_data::ipc_type::value_type & i_ipc, data.ipc)
    {
        *it_conected_proc_ids = i_ipc.first;
        ++it_connected_proc_n_links;
        if(it_connected_proc_n_links != nullptr) {
            *it_connected_proc_n_links = i_ipc.second;
            ++it_connected_proc_n_links;
        }
    }

    return data.ipc.size();
}

/*------------------------------------------------------------
  mmpr_ipc_fa_crossboundary_neig - to get info about element on the other side of boundary
------------------------------------------------------------*/
// To use this function one have to know for which "Fa" ask.
int mmpr_ipc_other_subdomain_neigh( // returns >= 0 if "Fa" is id of face at subdomain boundary, otherwise < 0
        const int Mesh_id,
        const int Fa,
         int *Neig_el_id_at_owner,
         int *Neig_el_owner
        )
{
    assert(Neig_el_id_at_owner != nullptr);
    assert(Neig_el_owner != nullptr);

    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);

    int neigs[2]={0}, retval=-1;
    mmr_fa_eq_neig(Mesh_id,Fa,neigs,nullptr,nullptr);
    if( neigs[1] == MMC_SUB_BND ) {
        assert(pmesh.data.ipc_links.empty() == false);
        assert(pmesh.data.ipc_links.find(Fa) != pmesh.data.ipc_links.end());

        *Neig_el_id_at_owner = pmesh.data.ipc_links.at(Fa).id_at_owner;
        *Neig_el_owner = pmesh.data.ipc_links.at(Fa).owner;
        retval = 0;
    }

    return retval;
}

/*------------------------------------------------------------
  mmpr_ipc_faces_connected_to_proc - to get array of face (ids) connected with given proc id
------------------------------------------------------------*/
int mmpr_ipc_faces_connected_to_proc( // returns number of faces connected with Proc_id
        const int Mesh_id, //in: mesh id
        const int Proc_id, //in: proc id to check
        int * Faces //out: [upto Connected_proc_n_links for Proc_id]
        )
{
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    assert(pmesh.data.ipc.empty() == false);
    assert(pmesh.data.ipc_links.empty() == false);

    int * it_faces = Faces;

    BOOST_FOREACH(const mmpt_data::ipc_links_type::value_type & it_map, pmesh.data.ipc_links) {
        if(it_map.second.owner == Proc_id) {
            *it_faces = it_map.first;
            ++it_faces;
        }
    }
    assert(std::distance(Faces,it_faces) < pmesh.data.ipc.at(Proc_id));
    return std::distance(Faces,it_faces);
}

#ifdef __cplusplus
}
#endif
