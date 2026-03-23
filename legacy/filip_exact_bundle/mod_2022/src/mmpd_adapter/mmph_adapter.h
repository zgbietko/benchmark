#ifndef _mmph_adapter_
#define _mmph_adapter_

#include <metis.h>
#include <parmetis.h>
#include <unordered_map>
#include <vector>
#include <set>

#include "PCsr.hpp"
#include "uth_intf.h"

#ifdef __cplusplus
extern "C"{
#endif

/*** CONSTANTS ***/

#define MAC_USE_KWAY_GRAPH_PART_TOOL   0   /* partitioning tool. Better quality, much time to compute */
#define MAC_USE_RB_GRAPH_PART_TOOL   1   /* partitioning tool. Worse quality, less time to compute */

#define MAC_MAX_ELEMENT_FACES   6   /* max supported facas that that element may have */
#define MAC_MAX_ELEMENT_EGDES   12  /* max supported edges that that element may have. */
#define MAC_MAX_ELEMENT_NODES   8  /* max supported edges that that element may have. */
#define MAC_MAX_NODES_ON_FACE   4  /* max supported edges that that element may have. */
#define MAC_MAX_NODES_COOR_ON_FACE   12  /* max supported edges that that element may have. */

#define MAC_MAX_ELEMENTS_CONNECTED_BY_NODE   384  /* max supported edges that that element may have. 6 faces x 3 genlev (8 el) ^ 2 */
#define MAC_MAX_PARTITIONS   128  /* max supported edges that that element may have. */

#define MAC_PARALLEL_MESH_CORASING   1  /* use OpenMP to speedup lookup_table building */

#define MAC_PRISM_OPTIMIZATION  1  /* use weights during decomposition that indicates amount of data to exchange betweene each face in prism elements */

#define MMC_MAX_NUM_MESH   10   //* maximal number of meshes

#define MMPC_CLASSIC_PART_METHOD   0   //* use classic (old) partition method
#define MMPC_METIS_PART_METHOD   1   //* use Metis and ParMetis libraries to partition a mesh

/* type IPID - InterProcessorID - main type for parallel modules */
typedef struct {
  int owner;        /* owner ID (from 1 to nrproc) */
  int id_at_owner;
} mmpt_ipid,
  mmpt_node,
  mmpt_edge,
  mmpt_face,
  mmpt_elem;

/* info about refinements - data exchanged between subdoamins */
typedef struct{
  /* for each refined element (prism or hexahedron):
     element_ID, element_type, new_node_ID (if any),
     new_edges_IDs, new_faces_IDs (horizontal+vertical), new_elems_IDs (sons)
     1 + 1 + [1] + 3..6 + 10..12 + 8 = 23..29
  */
  int* elem;
  /* for each refined face:
     face_ID, face_type,
     new_node_ID (if any), new_edges_IDs, new_faces_IDs (sons)
     1 + 1 + [1] + 3..4 + 4 = 9..11
  */
  int* face;
  /* for each refined edge:
     edge_ID, new_node_ID, new_edges_IDs (sons)
     1 + 1 + 2 = 4
  */
  int* edge;

  void clear() {
      UTM_SAFE_FREE_PTR(elem);
      UTM_SAFE_FREE_PTR(face);
      UTM_SAFE_FREE_PTR(edge);
  }

} mmpt_ref_info;

typedef struct{
  int* face; /* list of deleted owned faces */
  int* edge; /* list of deleted owned edges */

  void clear() {
      UTM_SAFE_FREE_PTR(face);
      UTM_SAFE_FREE_PTR(edge);
  }
} mmpt_del_info;

///*
// * data structure to handle parallel adaptivity
// */

///* structure with arrays of overlap entities' local and alien IDs */
///* (alien ID is an ID on another processor) */
///* for each processor, sharing entities with the given one, there is */
///* a separate set of lists (to speed up searching of lists, they should be */
///* stored in increasing order of local IDs !!!!!*/
//typedef struct{
//  int nr_elem_alien[MMPC_MAX_NUM_SUB];
//  int nr_face_alien[MMPC_MAX_NUM_SUB];
//  int nr_edge_alien[MMPC_MAX_NUM_SUB];
//  int nr_vert_alien[MMPC_MAX_NUM_SUB];
//  int nr_elem_ovl[MMPC_MAX_NUM_SUB];
//  int* l_elem_ovl_loc[MMPC_MAX_NUM_SUB];
//  int* l_elem_ovl_ali[MMPC_MAX_NUM_SUB];
//  int nr_face_ovl[MMPC_MAX_NUM_SUB];
//  int* l_face_ovl_loc[MMPC_MAX_NUM_SUB];
//  int* l_face_ovl_ali[MMPC_MAX_NUM_SUB];
//  int nr_edge_ovl[MMPC_MAX_NUM_SUB];
//  int* l_edge_ovl_loc[MMPC_MAX_NUM_SUB];
//  int* l_edge_ovl_ali[MMPC_MAX_NUM_SUB];
//  int nr_vert_ovl[MMPC_MAX_NUM_SUB];
//  int* l_vert_ovl_loc[MMPC_MAX_NUM_SUB];
//  int* l_vert_ovl_ali[MMPC_MAX_NUM_SUB];

//  void clear() {

//  }
//} mmpt_mesh_ovl;

/*** DATA TYPES ***/

//typedef struct
//{
//  idx_t *xadj, /* input CSR format. Indicates on amount of adjected verticles*/
//    *adjncy,   /* input CSR format. Indicates on adjected verticles */
//    *adjwgt,   /* input CSR format. Indicates weights of the edges */
//    *vwgt;     /*  input CRS format. Indicates weights of the vertices in xadj*/
//} mmpt_CSR_graph;
typedef CSR<idx_t> mmpt_CSR;
typedef PCSR<idx_t> mmpt_PCSR;

typedef struct
{
  idx_t options[METIS_NOPTIONS]; /* allow to fine-tune and modify various aspects of the internal algorithms used by METIS */
  mmpt_PCSR  PCSR; // one graph for each sub-domain

  // idx_t* vtxdist; /* describes how the vertices of the graph are distributed among the processors */
  idx_t* vsize;
//  idx_t** part_local ; /* upon successful completion stores the local partition vector of the mesh (ids for each element) */
  std::vector<real_t> tpwgts ; /* fraction of vertex weight that should be distributed to each sub-domain for each balance constraint */
  real_t ubvec ; /* specify the imbalance tolerance for each vertex weight, with 1 being perfect balance and nparts being perfect imbalance. A value of 1.05 recommended */
  idx_t wgtflag ; /* indicate if the graph is weighted. 0 - No weights, 1 - weights on edges */
  idx_t numflag ; /* numbering scheme that is used for the vtxdist, xadj, adjncy, and part. 0 C-style numbering */
  real_t itr ;  /* ratio of inter-processor communication time compared to data redistribution time. Value of 1000.0 is recommended*/

  MPI_Comm comm; /* pointer to the MPI communicator of the processes that call PARMETIS */

  /* Metis and Parmetis */
  std::vector<idx_t> part; /* upon successful completion stores the partition vector of the mesh (ids for each element) */
  idx_t nparts ; /* The number of parts to partition the mesh */
  idx_t objval ; /* after complition: Stores the edge-cut or the total communication volume of the partitioning solution  */
  idx_t ncon ; /* the number of balancing constraints */
  idx_t met_parmet_result ; /* the value returned by metis or parmetis routines */
  idx_t edgecut;

  void clear() {
     UTM_SAFE_FREE_PTR(vsize);
     tpwgts.clear();
     PCSR.clear();
     part.clear();
  }

} mmpt_partitioning_data;

typedef struct
{
  mmpt_partitioning_data mp; // metis and parmetis data

  int mesh_id ;  /* actual mesh */
  idx_t nae ; /* the number of active elements in the mesh */
  idx_t ne ; /* the number of elements in the mesh */
  idx_t nn ; /* the number of nodes in the mesh */
//  int is_parmetis ; /* 1 - use parmetis routines. 0 - use metis routins */
//  int elements_connections_no ;  /* amount of connections (edges) between elments treated as graph node */
  //element_lookup* lookup_table;  /* table contains connection betwen mesh elements. Expensive to generate. */
  int parition_tool;  /* used partition tool. Can be set to MAC_USE_KWAY_GRAPH_PART_TOOL or MAC_USE_RB_GRAPH_PART_TOOL */
  //part_with_overlap * part_final;
  typedef std::unordered_map<int,int> ipc_type;
  typedef std::unordered_map<int, mmpt_ipid> ipc_links_type;
  std::set<int>  boundary_elems_snapshot;
  ipc_type  ipc; // map <proc_id,no_of_links>
  ipc_links_type ipc_links; // map <local face_id, ipid of element on the other side of inter subdomain boundary>

  void clear() {
      mp.clear();
      boundary_elems_snapshot.clear();
      ipc.clear();
      ipc_links.clear();
  }

} mmpt_data;

/* structure with mesh data */
// This structure is inteded to exist (complete) on each proces(or) during computation.

typedef struct {
  typedef std::unordered_map<int,mmpt_ipid> mmpt_owners_map;
  mmpt_owners_map ovl_nodes,ovl_edges,ovl_faces,ovl_elems;
  mmpt_ref_info ref_loc;
  mmpt_ref_info ref_ali;
  mmpt_del_info del_loc;
  std::vector<int> boundary_faces;
  std::vector<char> bnd_vtx_flags;
  //mmpt_mesh_ovl mesh_ovls;
  mmpt_data data;
  bool isInvalid = false;

  void clear() {
      ovl_nodes.erase(ovl_nodes.begin(),ovl_nodes.end());
      ovl_edges.erase(ovl_edges.begin(),ovl_edges.end());
      ovl_faces.erase(ovl_faces.begin(),ovl_faces.end());
      ovl_elems.erase(ovl_elems.begin(),ovl_elems.end());
      ref_loc.clear();
      ref_ali.clear();
      del_loc.clear();
      boundary_faces.clear();
      //mesh_ovls.clear();
      data.clear();
  }

} mmpt_mesh;


/*** GLOBAL VARIABLES for the whole module ***/

extern int       mmpv_partition_method;   /*  method that is used to generate partitions */
extern std::vector<mmpt_mesh>  mmpv_meshes;        /* array of meshes */

/*** LOCAL PROCEDURES  ***/

/**--------------------------------------------------------
  mmpr_select_mesh - to select the proper mesh
---------------------------------------------------------*/
mmpt_mesh* mmpr_select_mesh( /* returns pointer to the chosen mesh */
               /* to avoid errors if input is not valid */
               /* it returns the pointer to the current mesh */
  int Mesh_id    /* in: mesh ID or 0 (MMC_CUR_MESH_ID) for the current mesh */
  );


#ifdef __cplusplus
}
#endif

#endif
