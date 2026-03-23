/************************************************************************
File dds_metis_adapter_intf.c - implementation of interface routines for 
								Metis and ParMetis - mesh partitioning libraries 

Contains definitions of interface routines: 
	
  mar_partition_mesh - to to decompose the mesh and create subdomains by Metis library
  mar_adaptive_repartition - to balance the work load of a graph that corresponds to an adaptively refined mesh
  mar_refine_partition - to improve the quality of an existing a k-way partitioning
  mar_get_parts - get partition vector for the elements of the mesh. Indexes - active element number. values - procesors ids
  mar_get_parts_with_overlap - get partition vectors for the elements of the mesh with overlap info

  mar_set_metis_options - to specify metis behaviour
  mar_set_parmetis_options - to specify parmetis behaviour
  mar_initialize_work - to initialize the internal data structures
  mar_end_work - free memory for internal data structures
  mar_mesh_to_graph - fill lookup_table. Collapse elements into nodes.
  mar_mesh_to_CSR - fill xadj and adjncy data structures (in CSR format)
  mar_mesh_to_distributed_CSR - fill xadj and adjncy data structures (in Distributed CSR format)
  mar_gather_part_local - gather local partitions and merge them to global
  mar_scatter_part_local - satter local partitions from proc 0 to all other
  mar_check_mpi_flags - check is parallel enviroment variables
  mar_minimalize_communication - reorder newly created partion vector to maximalize match to old partiton vector
  mar_set_proc_ids_to_objects - distribute partitions ids to mesh objects
  mar_create_overlap - create overlap, fill data structure, set internal
  mar_find_el_conn_by_nodes - recurent procedure that find neighbours of org_el by his node ids
  mar_set_subdomains_boundaries - to find and set domain boundaries (faces set)

------------------------------  			
History:
    10.2012 - Kazimierz Michali, incorporating into ModFem
	08.2012 - Kamil Wachala, initial version		
*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<omp.h>
#include<algorithm>
#include<cassert>
#include<metis.h>
#include<parmetis.h>
#include<numeric>
#include<vector>
#include<deque>
//#include<boost/foreach.hpp>
#include <sstream>
#include<iostream>

/* internal header file for the metis adapter module */
#include "mmh_intf.h"
#include "uth_intf.h"
#include "pch_intf.h"
#include "ddh_metis_adapter.h"
#include "../mmph_adapter.h"
#include "uth_log.h" //#include "dbg.h"

#ifdef __cplusplus
extern "C"{
#endif


/* GLOBAL VARIABLES */
//mmpt_data pmesh.data;


// /* Metis */
// idx_t options[METIS_NOPTIONS]; /* allow to fine-tune and modify various aspects of the internal algorithms used by METIS */
// idx_t* xadj = nullptr; /* input CSR format. Indicates on amount of adjected verticles*/
// idx_t* adjncy = nullptr; /* input CSR format. Indicates on adjected verticles */
// idx_t* adjwgt = nullptr; /* weights of the edges */

// /* Parmetis */
// idx_t* vtxdist = nullptr; /* describes how the vertices of the graph are distributed among the processors */
// idx_t** part_local = nullptr; /* upon successful completion stores the local partition vector of the mesh (ids for each element) */
// idx_t** xadj_local = nullptr; /* local input CSR format. Indicates on amount of adjected verticles*/
// idx_t** adjncy_local = nullptr; /* local input CSR format. Indicates on adjected verticles */
// idx_t** adjwgt_local = nullptr; /* local weights of the edges */
// real_t* tpwgts = nullptr; /* fraction of vertex weight that should be distributed to each sub-domain for each balance constraint */
// real_t* ubvec = nullptr; /* specify the imbalance tolerance for each vertex weight, with 1 being perfect balance and nparts being perfect imbalance. A value of 1.05 recommended */
// idx_t wgt_flag = 0,*wgtflag = &wgt_flag; /* indicate if the graph is weighted. 0 - No weights, 1 - weights on edges */
// idx_t num_flag = 0, *numflag = &num_flag; /* numbering scheme that is used for the vtxdist, xadj, adjncy, and part. 0 C-style numbering */
// real_t i_tr = 0, *itr = &i_tr; /* ratio of inter-processor communication time compared to data redistribution time. Value of 1000.0 is recommended*/

// MPI_Comm comm_ = MPI_COMM_WORLD, *comm = &comm_; /* pointer to the MPI communicator of the processes that call PARMETIS */

// /* Metis and Parmetis */
// idx_t* part = nullptr; /* upon successful completion stores the partition vector of the mesh (ids for each element) */
// idx_t n_parts = 0, *nparts = &n_parts; /* The number of parts to partition the mesh */
// idx_t obj_val = 0, *objval = &obj_val; /* after complition: Stores the edge-cut or the total communication volume of the partitioning solution  */
// idx_t n_con = 0, *ncon = &n_con; /* the number of balancing constraints */
// idx_t met_parmet_result = 0; /* the value returned by metis or parmetis routines */

// /* Internal */
// // replaced with mmpv_mmpv_my_proc_id  //int mmpv_my_proc_id = 0; /* currently processor id that execude code */
// // replaced with mmpv_nr_sub  //int mmpv_nr_sub = 0; /* total amount of anavabile processors */


const int MMPC_COPYING_MSG_ID ='c',
MMPC_TRANSFER_MSG_ID = 't',
MMPC_DISTIRB_MSG_ID = 'd';


///*------------------------------------------------------------
//  mmpr_recreate_owner_tables - to initialize the internal data structures
//------------------------------------------------------------*/
//void mmpr_recreate_owner_tables(int Mesh_id)
//{
//    mmpt_mesh& pmesh = *mmpr_select_mesh(Mesh_id);
//    pmesh.elems.resize(mmr_get_max_elem_max(Mesh_id)+1);
//    pmesh.faces.resize(mmr_get_max_face_max(Mesh_id)+1);
//    pmesh.edges.resize(mmr_get_max_edge_max(Mesh_id)+1);
//    pmesh.nodes.resize(mmr_get_max_node_max(Mesh_id)+1);

//  /* allocate space for nodes' structures */
//    assert(pcr_my_proc_id()!=1 || !pmesh.elems.empty()); // there is no mesh without elements... (at least for master proc)

//   // at least master proc must have mesh
//    assert(pcr_my_proc_id()!=1 || !pmesh.elems.empty());
//    assert(pcr_my_proc_id()!=1 || !pmesh.faces.empty());
//    assert(pcr_my_proc_id()!=1 || !pmesh.edges.empty());
//    assert(pcr_my_proc_id()!=1 || !pmesh.nodes.empty());

//    // Setting up initial ownership info.

//    int my_proc_id = pcr_my_proc_id();
//    for (int Nel=0; (Nel=mmr_get_next_elem_all(Mesh_id,Nel));) {
//        //int rc = mmpr_el_set_owner(Mesh_id,Nel,my_proc_id);
//        //rc = mmpr_el_set_id_at_owner(Mesh_id,Nel,Nel);
//        pmesh.elems[Nel].owner = MMPC_MY_OWNERSHIP;
//        pmesh.elems[Nel].id_at_owner = Nel;
//    }
//    for (int Nfa=0; (Nfa=mmr_get_next_face_all(Mesh_id,Nfa));) {
////        int rc = mmpr_fa_set_owner(Mesh_id,Nfa,my_proc_id);
////        assert(rc >= 0);
////        rc = mmpr_fa_set_id_at_owner(Mesh_id,Nfa,Nfa);
////        assert(rc >= 0);
//        pmesh.faces[Nfa].owner = MMPC_MY_OWNERSHIP;
//        pmesh.faces[Nfa].id_at_owner = Nfa;

//    }
//    for (int Ned=0; (Ned=mmr_get_next_edge_all(Mesh_id,Ned));) {
////        int rc = mmpr_ed_set_owner(Mesh_id,Ned,my_proc_id);
////        assert(rc >= 0);
////        rc = mmpr_ed_set_id_at_owner(Mesh_id,Ned,Ned);
////        assert(rc >= 0);
//        pmesh.edges[Ned].owner = MMPC_MY_OWNERSHIP;
//        pmesh.edges[Ned].id_at_owner = Ned;
//    }
//    for (int Nno=0; (Nno=mmr_get_next_node_all(Mesh_id,Nno));) {
////        int rc = mmpr_ve_set_owner(Mesh_id,Nno,my_proc_id);
////        assert(rc >= 0);
////        rc = mmpr_ve_set_id_at_owner(Mesh_id,Nno,Nno);
////        assert(rc >= 0);
//        pmesh.nodes[Nno].owner = MMPC_MY_OWNERSHIP;
//        pmesh.nodes[Nno].id_at_owner = Nno;
//    }
//}


/* ROUTINES */

void ddr_print_ownership(const int Mesh_id) {
    int n=mmr_get_nr_node(Mesh_id);
    int i=0;
    std::cout << "No. of nodes: " << n << "\n";
    while((i=mmr_get_next_node_all(Mesh_id,i)) != 0) {
        std::cout << "Node " << i << " owner: " << mmpr_ve_owner(Mesh_id,i)
                  << " id_at_owner: " << mmpr_ve_id_at_owner(Mesh_id,i) << "\n";
    }

    n=mmr_get_nr_elem(Mesh_id);
    i=0;
    std::cout << "No. of elems: " << n << "\n";
    while((i=mmr_get_next_elem_all(Mesh_id,i)) != 0) {
        std::cout << "Elem " << i << " owner: " << mmpr_el_owner(Mesh_id,i)
                  << " id_at_owner: " << mmpr_el_id_at_owner(Mesh_id,i) << "\n";
    }

}

 // Filling VtxDist table.
int ddr_PCSR_update_vtxdist(const int Mesh_id,idx_t * Vtx_dist_table)
{
    assert(Vtx_dist_table != nullptr);
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    std::vector<int> vtx_counts(pcv_nr_proc,0);

    pmesh.data.ne = mmr_get_nr_elem(Mesh_id);

    pcr_allgather_int( & pmesh.data.ne , 1, vtx_counts.data(), 1 );
    std::fill_n(Vtx_dist_table,pcv_nr_proc+1,0);
    // because VtxDist[0]=0
    std::partial_sum(vtx_counts.begin(),vtx_counts.end(),Vtx_dist_table+1);
    assert(Vtx_dist_table[0]==0);
    return 0;
}

int ddr_PCSR_distribute_elems_to_procs(const int Mesh_id,
                                  const int Source_proc_id)
{

    //ddr_print_ownership(Mesh_id);

    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    mmpt_PCSR &Pcsr = pmesh.data.mp.PCSR;
    int n_distributed_elems=0;
    std::vector<int> new_glob_pos;

    if(pcr_my_proc_id() == Source_proc_id) {
        mf_debug("ddr_distribute_elems_to_procs: in sedner proc");
        assert(Pcsr.empty() == false);


        // mark boundary faces
        pmesh.bnd_vtx_flags.resize(mmr_get_nr_node(Mesh_id)+1,0);
        int fa=0;
        while( 0 != (fa=mmr_get_next_face_all(Mesh_id,fa)) ) {
            int neighs[2]={0},neigsSubdomains[2]={0};
            mmr_fa_neig(Mesh_id,fa,neighs,NULL,NULL,NULL,NULL,NULL);


            if(neighs[1] != 0) {//ignore external Boundary Conditions
                if(Pcsr.part_.at(neighs[0]-1) != Pcsr.part_.at(neighs[1]-1)) {
                    pmesh.boundary_faces.push_back(fa);
                    int nodes[5]={0};
                    mmr_fa_node_coor(Mesh_id,fa,nodes,NULL);
                    for(int i=1; i <= nodes[0];++i) {
                        if(pmesh.bnd_vtx_flags[nodes[i]]!=1) {
                            pmesh.bnd_vtx_flags[nodes[i]]=1;
                            mfp_debug("Vertex %d is at boundary.",nodes[i]);
                        }
                   }
                }
            }
        }

        pmesh.boundary_faces.erase(std::unique(pmesh.boundary_faces.begin(),pmesh.boundary_faces.end()), pmesh.boundary_faces.end());
        mfp_debug("\nNo. of boundary faces=%d", pmesh.boundary_faces.size());

        // Splitting initial PCSR into per-proc-PCSRs
        // Waring: element indexes in per-proc-PCSRs will be different!
        typedef mmpt_CSR::CsrNodeInternal cNode;
        std::vector<mmpt_PCSR> procs_pcsr(pcr_nr_proc());
        std::for_each(Pcsr.begin(),Pcsr.end(), [&](cNode & n) {
            // split into subvectors
            procs_pcsr[*n.ppart].push_back(n);
        });



        new_glob_pos.resize(Pcsr.size(),0);
        int new_pos=Pcsr.vtxDist()[pcv_my_proc_id-1];
        for(int p=0; p < procs_pcsr.size();++p) {
            // Creating mapping array: new_glob_ids[old_id]=new_id
            if(procs_pcsr[p].size() > 0) {
                std::for_each(procs_pcsr[p].begin(),procs_pcsr[p].end(),
                              [&](const cNode & n) {
                                new_glob_pos[(*n.pel_id)-1]=new_pos;
                                ++new_pos;
                              });
            }

        }

        for(int p=0; p < procs_pcsr.size();++p) {
            // Establish correct identyfiers in PCSR adjncy tables.
            for(idx_t & neigh_pos : procs_pcsr[p].adjncy_) {
                neigh_pos = new_glob_pos[neigh_pos]; // +1 for id, not pos
            }

            // Transfering elements (using pre-created lists)
            if(p+1 != Source_proc_id) {
                ddr_transfer_full_elems(Mesh_id,Source_proc_id,p+1,
                                        procs_pcsr[p].size(), procs_pcsr[p].el_loc_id_.data(),DDE_MOVE );
                n_distributed_elems+=procs_pcsr[p].size();

                procs_pcsr[p].resize_n_proc(pcv_nr_proc);

                std::vector<idx_t> buf;
                procs_pcsr[p].write(buf);

                //mf_debug("ss: %s",ss.str().data());
                const int n_nums = buf.size();
                mf_debug("ddr_distribute_elems_to_procs: sending archived PCSR with size %d",n_nums);
                const int buffer = pcr_send_buffer_open(MMPC_DISTIRB_MSG_ID,PCC_DEFAULT_BUFFER_SIZE);

                int rc = pcr_buffer_pack_int(MMPC_DISTIRB_MSG_ID,buffer,1,&n_nums);
                assert(rc == MPI_SUCCESS);
                rc = pcr_buffer_pack_int(MMPC_DISTIRB_MSG_ID,buffer,n_nums, buf.data());
                assert(rc == MPI_SUCCESS);

                rc = pcr_buffer_send(MMPC_DISTIRB_MSG_ID,buffer,p+1);
                assert(rc == MPI_SUCCESS);
            }
        }
        assert(new_pos == Pcsr.size());

        // Removing transfered elements from this proc structures.
        Pcsr = std::move(procs_pcsr[pcv_my_proc_id-1]);
    }
    else {
        mf_debug("ddr_distribute_elems_to_procs: in recv proc");
        mf_check_debug(Source_proc_id < pcv_my_proc_id,"Wrong source proc number in distrib (%d)", Source_proc_id);
        mf_check_debug(Source_proc_id > 0,"Wrong source proc number in distrib (%d)", Source_proc_id);
        ddr_transfer_full_elems( Mesh_id, Source_proc_id, pcr_my_proc_id(), 0, nullptr, DDE_MOVE );

        int n_nums=0;
        const int buffer = pcr_buffer_receive(MMPC_DISTIRB_MSG_ID, Source_proc_id , PCC_DEFAULT_BUFFER_SIZE);
        int rc = pcr_buffer_unpack_int(MMPC_DISTIRB_MSG_ID, buffer,1,&n_nums);
        //int rc=pcr_receive_int(Source_proc_id,MMPC_DISTIRB_MSG_ID,1,&n_bytes);
        assert(rc == MPI_SUCCESS);
        mf_debug("ddr_distribute_elems_to_procs: reciving PCSR archive size: %d",n_nums);
        assert(n_nums > 0);

        std::vector<idx_t> v;
        v.resize(n_nums, 0);
        rc = pcr_buffer_unpack_int(MMPC_DISTIRB_MSG_ID,buffer,n_nums, const_cast<idx_t*>(v.data()));
        assert(rc == MPI_SUCCESS);

        Pcsr.clear();
        Pcsr.read(v);
        assert( !Pcsr.empty());

        // now in this proc graph nodes represeting elements have new ids
        for(int e=0; e < Pcsr.size(); ++e) {
            Pcsr.el_loc_id_[e]=e+1;
        }

        pcr_recv_buffer_close(MMPC_DISTIRB_MSG_ID,buffer);
    }

    mmr_test_mesh(Mesh_id);

    //ddr_print_ownership(Mesh_id);

    return n_distributed_elems;
}

int ddr_transfer_full_elems(const int Mesh_id,
        const int Source_proc_id,
        const int Dest_proc_id,
        const int N_transfer_elems,
        const int * Transfer_elem_ids,
        const DDE_TRANSFER_POLICY T_policy)
{
    assert(Source_proc_id > 0 || Source_proc_id==MPI_ANY_SOURCE);
    assert(Source_proc_id <= pcr_nr_proc() || Source_proc_id==MPI_ANY_SOURCE);
    assert(Dest_proc_id > 0);
    assert(Dest_proc_id <= pcr_nr_proc());
    assert(Dest_proc_id != Source_proc_id);

    using std::vector;
    int n_transfered_mesh_entities=0;
    int pcr_rc=0;
    mmpt_mesh& pmesh = *mmpr_select_mesh(Mesh_id);
    int msg=0;

    if(pcr_my_proc_id() == Source_proc_id) {
        assert(Transfer_elem_ids != nullptr);
        assert(N_transfer_elems > 0);

        const int buffer = pcr_send_buffer_open(MMPC_COPYING_MSG_ID,PCC_DEFAULT_BUFFER_SIZE);

        // sending N_copied_elems
        // >>>>>>>>>>>>> Send 1.
        //pcr_rc = pcr_send_int(Dest_proc_id,0,1,& N_copied_elems);
        pcr_rc = pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,1,&N_transfer_elems);

        mf_debug("ddr_copy_full_elems: 1 sended %d",pcr_rc);

        if(N_transfer_elems > 0) {
            // collecting what will be sended
            vector<int> elem_verts,vertices,elem_bcs;
            vertices.reserve(N_transfer_elems*4); // vertices used by elements to send
            elem_verts.reserve( vertices.capacity() );
            elem_bcs.reserve(N_transfer_elems*MMC_MAXELFAC);
            int nodes[10]={0};
            for(int e=0; e < N_transfer_elems; ++e) {
                int el_id = Transfer_elem_ids[e];
                //mf_debug("Coping el %d",el_id);
                mmr_el_node_coor(Mesh_id,el_id,nodes,nullptr);
                assert(nodes[0]>0);
                elem_verts.insert( elem_verts.end(), nodes, nodes+1+nodes[0] ); // no of nodes + nodes ids
                vertices.insert( vertices.end(), nodes+1, nodes+1+nodes[0] );  // only nodes ids
                int faces[MMC_MAXELFAC+1]={0};
                mmr_el_faces(Mesh_id,el_id,faces,nullptr);
                elem_bcs.push_back(faces[0]);


                for(int f=1; f <= faces[0]; ++f) {
                    elem_bcs.push_back( mmr_fa_bc(Mesh_id,faces[f]) );

                    if(T_policy == DDE_MOVE) {
                        mmr_init_ref(Mesh_id); // To make sure that mmr_del_elem will work.
                        // set face neig to MMC_SUB_BND
                        int neig[2]={0};
                        mmr_fa_neig(Mesh_id,faces[f],neig,NULL,NULL,NULL,NULL,NULL);

                        if(neig[0]==el_id) {
                            mmr_fa_set_sub_bnd(Mesh_id,faces[f],0);
                        }
                        else if(neig[1]==el_id){
                            mmr_fa_set_sub_bnd(Mesh_id,faces[f],1);
                        }
                        else {
                            mf_log_err("Wrong face neigs!");
                        }

                        if( (neig[0] == MMC_BOUNDARY || neig[0]==MMC_SUB_BND)
                        && (neig[1] == MMC_BOUNDARY || neig[1]==MMC_SUB_BND)  ){
                            // face is 'hanging' without element
                                mmr_del_face(Mesh_id,faces[f]);
                        }
                    }

                }


            }

            // remove duplicate entries
            std::sort(vertices.begin(),vertices.end());
            vertices.resize( std::distance( vertices.begin(), std::unique(vertices.begin(),vertices.end()) ) );

            // sort-out boundary vertices at the end of collection
            std::sort(vertices.begin(),vertices.end(), [&](int a, int b){ return pmesh.bnd_vtx_flags[a]< pmesh.bnd_vtx_flags[b];});

            vector<char> vtx_subdomain_flags;
            for( const int& ve : vertices) {
                vtx_subdomain_flags.push_back(pmesh.bnd_vtx_flags[ve]);
            }

            // sending number of vertices
            int vertices_size = vertices.size();
            // >>>>>>>>>>>>> Send 2.
            //pcr_rc=pcr_send_int(Dest_proc_id,0,1, & vertices_size );
            pcr_rc = pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,1, & vertices_size );
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 2 sended %d",pcr_rc);
            // sending vertices identyfiers
            // >>>>>>>>>>>>> Send 3.
            //pcr_rc=pcr_send_int(Dest_proc_id,0, vertices.size(), vertices.data() );
            pcr_rc = pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,vertices.size(), vertices.data());
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 3 sended %d",pcr_rc);

            vector<double> vts_coords;
            vts_coords.resize(vertices.size()*3);

            vector<int>::const_iterator v_it(vertices.begin()), v_end(vertices.end() );
            vector<double>::iterator v_coor_it(vts_coords.begin());
            for(;v_it != v_end; ++v_it) {
                mmr_node_coor(Mesh_id,*v_it, & (*v_coor_it) );
                v_coor_it += 3;
            }

            // sending verts coordinates
            // >>>>>>>>>>>>> Send 4.
            //pcr_rc=pcr_send_double(Dest_proc_id,0,vts_coords.size(), vts_coords.data() );
            pcr_rc = pcr_buffer_pack_double(MMPC_COPYING_MSG_ID,buffer,vts_coords.size(), vts_coords.data());
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 4 sended %d",pcr_rc);

            // sending el_ids_at_owner
            // >>>>>>>>>>>>> Send 5.
            //pcr_send_int(Source_proc_id,0,N_copied_elems,Copied_elem_ids);
            pcr_rc=pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,N_transfer_elems,Transfer_elem_ids);
            mf_debug("ddr_transfer_full_elems: 5 sended %d",pcr_rc);

            // copying BC info
            // >>>>>>>>>>>>> Send 6.
            int elem_bcs_size = elem_bcs.size();
            pcr_rc = pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,1,& elem_bcs_size);
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 8 sended %d",pcr_rc);
            // >>>>>>>>>>>>> Send 7.
            //pcr_rc=pcr_send_int(Dest_proc_id,0,elem_verts.size(), elem_verts.data() );
            pcr_rc = pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,elem_bcs.size(), elem_bcs.data());
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 9 sended %d",pcr_rc);

            // sending number of elem vertices
            int elem_verts_size = elem_verts.size();
            // >>>>>>>>>>>>> Send 8.
            //pcr_rc=pcr_send_int(Dest_proc_id,0,1, & elem_verts_size );
            pcr_rc = pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,1,& elem_verts_size);
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 6 sended %d",pcr_rc);
            // sending elem vertices
            // >>>>>>>>>>>>> Send 9.
            //pcr_rc=pcr_send_int(Dest_proc_id,0,elem_verts.size(), elem_verts.data() );
            pcr_rc = pcr_buffer_pack_int(MMPC_COPYING_MSG_ID,buffer,elem_verts.size(), elem_verts.data());
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 7 sended %d",pcr_rc);

            // >>>>>>>>>>>>> Send 10.
            mf_check_debug(vtx_subdomain_flags.size() == vertices.size(),
                           "Incorrect number of flags(%d) for vertices(%d)!", vtx_subdomain_flags.size(), vertices.size());
            pcr_rc = pcr_buffer_pack_char(MMPC_COPYING_MSG_ID,buffer,vtx_subdomain_flags.size(), vtx_subdomain_flags.data());
            //mf_print_array(vtx_subdomain_flags,vertices.size(),"%d");
            //mf_print_array(vertices.data(), vertices.size(), "%d");
            assert(pcr_rc == MPI_SUCCESS);

            n_transfered_mesh_entities = N_transfer_elems + vertices.size();

            pcr_rc = pcr_buffer_send(MMPC_COPYING_MSG_ID,buffer,Dest_proc_id);
            mf_check(pcr_rc == MPI_SUCCESS, "ddr_transfer_full_elems: sending buffer!");


             if(T_policy == DDE_TRANSFER_POLICY::DDE_MOVE) {
                mmr_init_ref(Mesh_id);
                 for(int i=0; i < N_transfer_elems; ++i) {
                     mmr_del_elem(Mesh_id,Transfer_elem_ids[i]); // removing element from source proc
                     //mmpr_el_set_owner(Mesh_id,Transfer_elem_ids[i],0);
                     //mmpr_el_set_id_at_owner(Mesh_id,Transfer_elem_ids[i],0);
         //            pmesh.ovl_elems[Transfer_elem_ids[i]].owner = -1;
         //            pmesh.ovl_elems[Transfer_elem_ids[i]].id_at_owner = -1;
                 }
                 ddr_IPC_update(Mesh_id,Transfer_elem_ids,N_transfer_elems);
                 mmr_is_ready_for_proj_dof_ref(Mesh_id);
                 mmr_final_ref(Mesh_id);
             }
        }


    } else if(pcr_my_proc_id() == Dest_proc_id) {
        // recieving data
        // recieving n_copied_elems
        const int buffer = pcr_buffer_receive(MMPC_COPYING_MSG_ID,Source_proc_id,PCC_DEFAULT_BUFFER_SIZE);
        int source_proc_id = Source_proc_id;
        if(source_proc_id == MPI_ANY_SOURCE) {
            source_proc_id = pcr_buffer_source_id(buffer);
        }
        int n_copied_elems=0;
        // >>>>>>>>>>>>> Recv 1.
        //pcr_rc = pcr_receive_int(Source_proc_id,0,1,& n_copied_elems);
        pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,1,& n_copied_elems);
        assert(pcr_rc >= 0);
        mf_debug("ddr_transfer_full_elems: 1 recived %d",pcr_rc);

        if(n_copied_elems > 0) {
            // recieving number of vertices
            int n_vertices=0;
            // >>>>>>>>>>>>> Recv 2.
            //pcr_rc=pcr_receive_int(Source_proc_id,0,1,& n_vertices);
            pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,1,& n_vertices);
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 2 recived %d",pcr_rc);
            // recieving vertices
            vector<int> vertices(n_vertices,0);
            // >>>>>>>>>>>>> Recv 3.
            //pcr_rc=pcr_receive_int(Source_proc_id,0,n_vertices, vertices.data() );
            pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,n_vertices, vertices.data());
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 3 recived %d",pcr_rc);

            // recieving vertices coords
            vector<double> vts_coords(n_vertices*3, 0.0);
            // >>>>>>>>>>>>> Recv 4.
            //pcr_rc=pcr_receive_double(Source_proc_id,0,vts_coords.size(),vts_coords.data());
            pcr_rc = pcr_buffer_unpack_double(MMPC_COPYING_MSG_ID,buffer,vts_coords.size(),vts_coords.data());
            assert(pcr_rc == MPI_SUCCESS);
            mf_debug("ddr_transfer_full_elems: 4 recived %d",pcr_rc);

            // Resizing mesh struct to hold new enitities.
            mmr_init_read(Mesh_id,
                        mmr_get_max_node_id(Mesh_id)+vertices.size(),
                        mmr_get_max_edge_id(Mesh_id)+n_copied_elems*MMC_MAXELEDGES,
                        mmr_get_max_face_id(Mesh_id)+n_copied_elems*MMC_MAXELFAC,
                        mmr_get_max_elem_id(Mesh_id)+n_copied_elems);

            vector<int>    vertices_newIds;
            vertices_newIds.resize( *std::max_element(vertices.begin(),vertices.end()) + 1, 0 );
            vector<int>::const_iterator v_it(vertices.begin()),v_end(vertices.end());
            vector<double>::iterator v_coor_it(vts_coords.begin());
            for(;v_it != v_end; ++ v_it) {
                mmpt_mesh::mmpt_owners_map::const_iterator it(std::find_if(pmesh.ovl_nodes.begin(),pmesh.ovl_nodes.end(),
                                   [&](const mmpt_mesh::mmpt_owners_map::value_type & mv){ return mv.second.owner==source_proc_id && mv.second.id_at_owner==*v_it;}));
                if(pmesh.ovl_nodes.end() ==  it) {
                    // if vertex is not found we can add it
                    vertices_newIds[*v_it] = mmr_add_node(Mesh_id, MMC_AUTO_GENERATE_ID, &(*v_coor_it) );

                }
                else {
                    vertices_newIds[*v_it] = it->first; // setup with exisiting local id
                }
                v_coor_it += 3;
            }

            // recieving elem ids at owner
            vector<int> elem_ids_at_owner;
            elem_ids_at_owner.resize(n_copied_elems,0);
            // >>>>>>>>>>>>> Recv 5.
            //pcr_receive_int(Source_proc_id,0,n_copied_elems, elem_ids_at_owner.data() );
            pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,n_copied_elems, elem_ids_at_owner.data());
            mf_debug("ddr_transfer_full_elems: 5 recived %d",pcr_rc);

            // recv BC info!
            int n_el_bcs=0;
            // >>>>>>>>>>>>> Recv 6.
            //pcr_rc=pcr_receive_int(Source_proc_id,0,1,& n_el_verts);
            pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,1,& n_el_bcs);
            assert(pcr_rc >= 0);
            assert( n_el_bcs > 0);
            mf_debug("ddr_transfer_full_elems: 8 recived %d",pcr_rc);

            vector<int>  elem_bcs(n_el_bcs,0);
            // >>>>>>>>>>>>> Recv 7.
            //pcr_rc=pcr_receive_int(Source_proc_id,0,n_el_verts, elem_verts.data() );
            pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,n_el_bcs, elem_bcs.data() );
            assert(pcr_rc >= 0);
            mf_debug("ddr_transfer_full_elems: 9 recived %d",pcr_rc);

            // recieving number of elems verts
            int n_el_verts=0;
            // >>>>>>>>>>>>> Recv 8.
            //pcr_rc=pcr_receive_int(Source_proc_id,0,1,& n_el_verts);
            pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,1,& n_el_verts);
            assert(pcr_rc >= 0);
            assert( n_el_verts > 0);
            mf_debug("ddr_transfer_full_elems: 6 recived %d",pcr_rc);

            vector<int>  elem_verts(n_el_verts,0);
            // >>>>>>>>>>>>> Recv 9.
            //pcr_rc=pcr_receive_int(Source_proc_id,0,n_el_verts, elem_verts.data() );
            pcr_rc = pcr_buffer_unpack_int(MMPC_COPYING_MSG_ID,buffer,n_el_verts, elem_verts.data() );
            assert(pcr_rc >= 0);
            mf_debug("ddr_transfer_full_elems: 7 recived %d",pcr_rc);

            // >>>>>>>>>>>>> Recv 10.
            pmesh.bnd_vtx_flags.resize(n_el_verts,0);
            pcr_rc = pcr_buffer_unpack_char(MMPC_COPYING_MSG_ID,buffer,n_vertices, pmesh.bnd_vtx_flags.data());
            assert(pcr_rc == MPI_SUCCESS);
            mf_print_array(pmesh.bnd_vtx_flags,n_vertices,"%d");
            mf_print_array(vertices.data(),n_vertices, "%d");


            for(int i=0; i < n_vertices; ++i) {
                mf_check_debug(source_proc_id > 0,"Source proc number invalid (%d)", source_proc_id);
                mf_check_debug(source_proc_id != pcv_my_proc_id,"Source proc number invalid (%d)", source_proc_id);
                if(((source_proc_id < pcv_my_proc_id) && (pmesh.bnd_vtx_flags[i]==1)) || (T_policy==DDE_COPY)) {
                    pmesh.ovl_nodes[vertices_newIds[vertices[i]]].owner = source_proc_id;
                    pmesh.ovl_nodes[vertices_newIds[vertices[i]]].id_at_owner = vertices[i];
                    mf_log_info("Overlap node %d (old id %d), owner %d",vertices_newIds[vertices[i]],vertices[i],source_proc_id);
                }
            }


            // making room for info for new elements
            //pmesh.elems.resize(pmesh.elems.size() + elem_ids_at_owner.size());
            mmr_init_ref(Mesh_id);
            vector<int> elems_new;
            elems_new.reserve(n_copied_elems);
            vector<int>::const_iterator bc_it= elem_bcs.begin();
            vector<int>::const_iterator it = elem_verts.begin(), end = elem_verts.end(), it_elem_ids_at_owner(elem_ids_at_owner.begin());
            for(; it != end; ++it_elem_ids_at_owner) {
                int n_of_vts = *it;
                int elem_verts_loc[8]={0};

                ++it;
                for(int i=0; i < n_of_vts; ++i) {
                    mf_check_debug(vertices_newIds[*it] != 0, "Incorrect vertex number!");
                    elem_verts_loc[i] = vertices_newIds[*it];
                    mf_check_debug(elem_verts_loc[i] != 0,"Incorrect mapping from external vertex id(%d) to local id(%d)",vertices_newIds[*it],elem_verts_loc[i]);

                    ++it;
                }

                const int el_type_from_n_vts[]={0,0,0,0, MMC_TETRA,0, MMC_PRISM};

                int new_el_id = mmr_add_elem(Mesh_id,MMC_AUTO_GENERATE_ID, el_type_from_n_vts[n_of_vts], elem_verts_loc, nullptr, 0 );
                elems_new.push_back(new_el_id);
                //TODO material transfering!!!

                int faces[MMC_MAXELFAC+1]={0};
                mmr_el_faces(Mesh_id,new_el_id,faces,nullptr);
                mf_check_debug(*bc_it == faces[0], "Error with transmiting BC! (recv:%d expected:%d)",*bc_it,faces[0]);
                ++bc_it;
                for(int f=1; f <= faces[0]; ++f, ++bc_it) {
                    mmr_fa_set_bc(Mesh_id,faces[f],*bc_it);
                }


                //mmpr_el_set_owner(Mesh_id,new_el_id,Source_proc_id);
                //mmpr_el_set_id_at_owner(Mesh_id,new_el_id, *it_elem_ids_at_owner);
                if(T_policy == DDE_TRANSFER_POLICY::DDE_COPY) {
                    pmesh.ovl_elems[new_el_id].owner = source_proc_id;
                    pmesh.ovl_elems[new_el_id].id_at_owner = *it_elem_ids_at_owner;
                }

                assert(new_el_id != 0);
            }

            mfp_check_debug(n_copied_elems == elems_new.size(), "No. of re-created elements does not match no. of recived elems.!");
            ddr_IPC_update(Mesh_id, elems_new.data(), n_copied_elems);
            mmr_is_ready_for_proj_dof_ref(Mesh_id);
            mmr_final_ref(Mesh_id);


            n_transfered_mesh_entities = n_copied_elems + vertices.size();

            mmr_test_mesh(Mesh_id);

            // Inter subdomain boundaries
        }
        pcr_recv_buffer_close(MMPC_COPYING_MSG_ID,buffer);
        //ddr_print_ownership(Mesh_id);

    }
    return n_transfered_mesh_entities;
}

/*------------------------------------------------------------
  ddr_copy_full_elem - to copy (original remains) element from one process (mesh) to another process (mesh)
  NOTE: this not alter the ownership information and parallel mesh distribution
  MOVE sematic is implemented in ddr_transfer_full_elem.
  Change ownership of element itself is implemented in ddr_chown_full_elem
------------------------------------------------------------*/
//int ddr_copy_full_elems(
//        const int Mesh_id,
//        const int Source_proc_id,
//        const int Dest_proc_id,
//        const int N_copied_elems,
//        const int *Copied_elem_ids)
//{

//}

//void mmpr_free_CRS(mmpt_CSR_graph * crs)
//{
//  if(crs != nullptr) {
	
//    UTM_SAFE_FREE_PTR(crs->xadj_);
//    UTM_SAFE_FREE_PTR(crs->adjncy_);
//    UTM_SAFE_FREE_PTR(crs->adjwgt_);
//    UTM_SAFE_FREE_PTR(crs->vwgt_);
	
//  }
//}

//--------------------------------------------------------
// ddr_mesh_to_CRS_graph - to save initial mesh (generation=level=1) as graph in compressed storage format
//-------------------------------------------------------
/*
  All of the graph partitioning and sparse matrix ordering routines in METIS take as input the adjacency structure of the
graph and the weights of the vertices and edges (if any).
  The adjacency structure of the graph is stored using the compressed storage format (CSR). The CSR format is a
	 widely used scheme for storing sparse graphs. In this format the adjacency structure of a graph with n vertices and
																						  m edges is represented using two arrays xadj and adjncy. The xadj array is of size n + 1 whereas the adjncy
																						  array is of size 2m (this is because for each edge between vertices v and u we actually store both (v; u) and (u; v)).
		  The adjacency structure of the graph is stored as follows. Assuming that vertex numbering starts from 0 (C style),
		  then the adjacency list of vertex i is stored in array adjncy starting at index xadj[i] and ending at (but not
																												 including) index xadj[i+1] (i.e., adjncy[xadj[i]] through and including adjncy[xadj[i+1]-1]). That
		  is, for each vertex i, its adjacency list is stored in consecutive locations in the array adjncy, and the array xadj
  is used to point to where it begins and where it ends.
  The weights of the vertices (if any) are stored in an additional array called vwgt. If ncon is the number of weights
  associated with each vertex, the array vwgt contains n ncon elements (recall that n is the number of vertices). The
  weights of the ith vertex are stored in ncon consecutive entries starting at location vwgt[i ncon]. Note that if
  each vertex has only a single weight, then vwgt will contain n elements, and vwgt[i] will store the weight of the
  ith vertex. The vertex-weights must be integers greater or equal to zero. If all the vertices of the graph have the same
  weight (i.e., the graph is unweighted), then the vwgt can be set to nullptr.
*/
//-------------------------------------------------------
bool ddr_is_greater0(const int i) { return i > 0; }
int ddr_mesh_to_CRSGraph(// return >=0 - number of vertices in CSR graph, <0 - error
                         const int Mesh_id // IN: mesh to compress
                         )
{
    mmpt_CSR &Csr = mmpr_select_mesh(Mesh_id)->data.mp.PCSR;
    Csr.clear();
    Csr.reserve(mmr_get_nr_elem(Mesh_id));

    std::vector<int> sons(mmr_get_max_gen(Mesh_id)*MMC_MAXELSONS, 0);
    // loop over all elements, choose only initial elems
    mmpt_CSR::CsrNode gh_nd;
    while( (gh_nd.el_id  = mmr_get_next_elem_all( Mesh_id, gh_nd.el_id )) > 0 ) {
        assert(gh_nd.el_id > 0);
        if( mmr_el_gen( Mesh_id, gh_nd.el_id) == MMC_INIT_GEN_LEVEL ) {
            // Determine how many sub-elements are in this initial elem.
            int father = mmr_el_fam_all( Mesh_id, gh_nd.el_id, sons.data() );
            assert(father == MMC_NO_FATH);

            // Weight = number of sub-elements (created during refinement)
            gh_nd.vwgt = sons[0]+1;

            // Gather neigbours info and count real neighbours (filter out boundary conditions flags).
            int neigs[MMC_MAXELFAC+1] = {0};
            int neigs_no[MMC_MAXELFAC+1] = {0,1,2,3,4,5};
            int rc=mmr_el_eq_neig( Mesh_id, gh_nd.el_id, neigs, nullptr);
            assert(rc >= 1); // success
            // below: +1 is because (neighs[0] == no. of all neigs), not neig id!
            gh_nd.n_neighs = std::distance(
                        gh_nd.neighs,
                        std::copy_if(neigs+1, neigs+1+neigs[0], gh_nd.neighs, ddr_is_greater0)
                    );

            if(gh_nd.n_neighs != neigs[0]) {
                for(int i=1,i2=0; i <= neigs[0]; ++i) {
                    if(neigs[i] != 0) { //bc_cond
                        gh_nd.neig_no[i2] = neigs_no[i-1];
                        ++i2;
                    }
                }
            }
            else {
                std::copy(neigs_no,neigs_no+gh_nd.n_neighs,gh_nd.neig_no);
            }

            // Because we number elements from 1, and storing from 0
            // so neighs must be decresed by 1.
            std::transform(gh_nd.neighs, gh_nd.neighs+gh_nd.n_neighs, gh_nd.neighs ,[](mmpt_CSR::Tind & n){ return --n;});
            Csr.push_back(gh_nd);
            //std::cout << Csr;
            assert(Csr.size() <= mmr_get_nr_elem(Mesh_id));
        }//!if
    }//!while

    ddr_check_CSR(Mesh_id);

    return Csr.size();
}

int ddr_check_CSR(const int Mesh_id)
{
    mmpt_CSR &Csr = mmpr_select_mesh(Mesh_id)->data.mp.PCSR;

    mf_check(Csr.size() == mmr_get_nr_elem(Mesh_id), "Incorrect size(=%d) of CSR (should be %d)",Csr.size(),mmr_get_nr_elem(Mesh_id));

    int nel=0;
    int neig[6]={0};

    const idx_t* it_xadj = Csr.xadj();
    const idx_t* it_adjncy = Csr.adjncy();


    while( (nel = mmr_get_next_act_elem(Mesh_id,nel)) != 0 ) {
        mmr_el_eq_neig(Mesh_id,nel,neig,nullptr);

        // NOTE: in pcsr bc_cond neigbors (=0) are omitted, therefore pcsr neighbors table can by shorter
        const int n_neigs = *(it_xadj+1) - *(it_xadj);
        assert(n_neigs == neig[0] - std::count(neig+1,neig+1+neig[0],0));

        for(int i=0; i < neig[0]; ++i) {
            if(neig[1+i] != 0) {
                assert(neig[1+i] == ((*it_adjncy) + 1));
                ++it_adjncy;
            }
        }

        ++it_xadj;
    }
    return 0;
}

//--------------------------------------------------------
// ddr_mesh_to_distributed_CRSGraph - to save initial mesh(es) from all procs
// (generation=level=1) as graph in parallel compressed storage format
//-------------------------------------------------------
int ddr_mesh_to_distributed_CRSGraph( // return >=0 - number of vertices in CSR graph for local proc, <0 - error
                                      const int Mesh_id) // IN: mesh to compress
{
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    mmpt_PCSR & Pcsr = pmesh.data.mp.PCSR;

    assert(pmesh.data.ne >= 0);
    Pcsr.clear();

//#ifdef DEBUG_MMM
//    printf("ddr_mesh_to_distributed_CRSGraph: starting for Mesh_id %d, N_elems %d\n",Mesh_id,N_elems);
//#endif

    // Actual filling tables with CSR.
    ddr_mesh_to_CRSGraph(Mesh_id);

    Pcsr.resize_n_proc(pcv_nr_proc);
    ddr_PCSR_update_vtxdist(Mesh_id,Pcsr.vtxDist());

    ddr_check_CSR(Mesh_id);

    assert(Pcsr.empty() == false);
    return Pcsr.size();
}

/*------------------------------------------------------------
  ddr_expand_overlap - to expand overlap regions by one element layer
------------------------------------------------------------*/
int ddr_PCSR_create_initial_distribution(
        const int Mesh_id)

{
// Assume that CSR and PSCR are correct.
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    mmpt_PCSR & Pcsr = pmesh.data.mp.PCSR;
    assert( !pmesh.data.mp.PCSR.empty() );

    pmesh.isInvalid = true;

    const int n_procs = pcr_nr_proc();
    std::vector<int> vtcs_distrib(n_procs,0);
    pcr_allgather_int( & pmesh.data.ne , 1, vtcs_distrib.data(), 1 );

    // There is a possibilty that mesh has poor initial vertex distribution
    // (there are some procs with 0 verteices at begining).
    // Unfortunetly ParMETIS requires vertices to be pre-distributed.
    // So we have to create some basic distribution to
    // assign each proc at least one vertex.
    // So we are looking for procs with no vertices.
    // When found, previous proc mesh is partitioned.
    //
    // We're begining from 1, becouse first process always have mesh.
    for( int p_rank=1 ; p_rank < pmesh.data.mp.nparts ; ++p_rank) {
        if(vtcs_distrib[p_rank] == 0) {
            // so p_id-th proc is first proc with no vertices
            // last proc with vertices
            const int proc_w_mesh_id=p_rank; // previous proc

            if(proc_w_mesh_id == pcr_my_proc_id()) {
                // use METIS to sequentialy partition mesh
                idx_t n_novert_procs = n_procs-p_rank+1;
                assert(n_novert_procs > 1);
                assert(n_novert_procs <= pcv_nr_proc);
                assert(Pcsr.empty() == false);
                assert(! pmesh.data.mp.tpwgts.empty());
                assert(pmesh.data.mp.options != nullptr);
                pmesh.data.mp.met_parmet_result
                        = METIS_PartGraphKway(  & pmesh.data.ne,
                                                & pmesh.data.mp.ncon,
                                                Pcsr.xadj(),
                                                Pcsr.adjncy(),
                                                Pcsr.vwgt(),
                                                Pcsr.vwgt(),//pmesh.data.mp.vsize,
                                                nullptr, // all edges in graph have same weight
                                                & n_novert_procs,
                                                pmesh.data.mp.tpwgts.data(),
                                                & pmesh.data.mp.ubvec,
                                                pmesh.data.mp.options,
                                                & pmesh.data.mp.objval,
                                                pmesh.data.mp.PCSR.part());
                assert(pmesh.data.mp.met_parmet_result == METIS_OK);
                assert(!pmesh.data.mp.PCSR.empty());
            }

            if(pcr_my_proc_id() >= proc_w_mesh_id) {
                // distribute initial mesh from proc_w_mesh
                ddr_PCSR_distribute_elems_to_procs(Mesh_id, proc_w_mesh_id);
            }
            // All done. Forcing end of the loop.
            p_rank = pmesh.data.mp.nparts;
        }
    }

    Pcsr.resize_n_proc(pcv_nr_proc);
    ddr_PCSR_update_vtxdist(Mesh_id,Pcsr.vtxDist());

    // shifting adjncy table accorind to vertex distribution described in VtxDist
//    const int adjncy_size = (pmesh.data.ne+1)*MMC_MAXELFAC;
//    const int my_proc_nr = pcr_my_proc_id()-1;
//    for(int i=0; i < adjncy_size; ++i) {
//        Pcsr.adjncy()[i]+=Pcsr.vtxDist()[my_proc_nr];
//    }

    // If there is more the 1 mesh file, we have to combine information from csr. files.
    const int n_meshprocs = n_procs - std::count(vtcs_distrib.begin(),vtcs_distrib.end(),0);
    if(n_meshprocs > 1) {
        assert(!"Not yet implemented!");
        // Xadj is ok, Vtxdist is ok, but Adjncy is ok only for local nodes.
        // We have to expand adjency
        // TODO:

    }

//    mmpr_recreate_owner_tables(Mesh_id);

    return pmesh.data.mp.met_parmet_result;
}

/*------------------------------------------------------------
  ddr_create_inter_subdomain_connectivity - to initially create ipc
------------------------------------------------------------*/
int ddr_PCSR_create_IPC(// return >=0 - ok, <0 - error
                         const int Mesh_id // IN: mesh id
                         )
{
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    int N_elems = pmesh.data.ne;
    assert(N_elems >= 0);
    const idx_t* Xadj = pmesh.data.mp.PCSR.xadj();
    //const idx_t* Adjwgt = pmesh.data.mp.PCSR.adjwgt();
    const idx_t* Adjncy = pmesh.data.mp.PCSR.adjncy();
    const idx_t* Adj_neig = pmesh.data.mp.PCSR.adj_neig_no();
    const idx_t* Vwgt = pmesh.data.mp.PCSR.vwgt();
    const idx_t* VtxDist = pmesh.data.mp.PCSR.vtxDist();
    const idx_t* El_id = pmesh.data.mp.PCSR.el_id();
    mmpt_data::ipc_type & ipc = pmesh.data.ipc;
    mmpt_data::ipc_links_type & ipc_links = pmesh.data.ipc_links;

    int retval=0;

    // Iterate throu adjency table, and cout neighbours from other subdomains.
    const idx_t*  it_xadj=Xadj;
    const idx_t*  it_neig_el=Adjncy;
    const idx_t*  it_vwgt=Vwgt;
    const idx_t* found_in_vtxdist=nullptr; // offset used to identity
    const idx_t* end_vtxdist = VtxDist+pcr_nr_proc()+1;
    const idx_t* min_el_id=VtxDist+pcr_my_proc_id()-1;
    const idx_t* max_el_id=VtxDist+pcr_my_proc_id();

    // Additional auxiliary table.
    std::vector<int> elem_seq;
    elem_seq.reserve(N_elems);
    int nel=0;
    while((nel=mmr_get_next_act_elem(Mesh_id,nel)) != 0) {
        elem_seq.push_back(nel);
    }

   // std::cout << pmesh.data.mp.PCSR;

    assert(elem_seq.size() == N_elems);

    for(int e=0; e < N_elems; ++e, ++it_xadj, ++it_vwgt) {
        for(const idx_t* it_adjency_proc_end = & Adjncy[*(it_xadj+1)];
        it_neig_el != it_adjency_proc_end; ++ it_neig_el) {
            // Compare neigbouring element id with VtxDist,
            // to find wich proc owns this element.
            // Proc with smaller id?
            found_in_vtxdist = nullptr;


            if((*it_neig_el) < (*min_el_id) ) {
                found_in_vtxdist = std::upper_bound( VtxDist+1, min_el_id, (*it_neig_el) ) -1;
                assert(found_in_vtxdist  < min_el_id);
                assert(found_in_vtxdist  >= VtxDist);
            } // or proc with greater id?
            else if((*it_neig_el) > (*max_el_id)) {
                found_in_vtxdist = std::upper_bound( max_el_id, end_vtxdist, (*it_neig_el) ) -1;
                assert(found_in_vtxdist  >= max_el_id);
                assert(found_in_vtxdist  < end_vtxdist);
            }

            if(found_in_vtxdist != nullptr) {
                // Compute owning_proc_id from relative distance from begining of VtxDist.
                const int owning_proc_id = std::distance(VtxDist, found_in_vtxdist )+1; // +1 to make id not rank
                assert(owning_proc_id > 0);

                mf_debug("Neigh %d owner %d",(*it_neig_el)+1,owning_proc_id);

                // Update inter process connectivity (including weight)
                ipc[owning_proc_id]+=Vwgt[e];
                assert(ipc.at(owning_proc_id) > 0);

                // Save information which local face points to the other side of subdomain.
                // Store info about which proc it is and what is local id of element on the other side.
                int faces[MMC_MAXELFAC+1]={0};
                mmr_el_faces(Mesh_id, El_id[e], faces, nullptr);

                //int el_neigs[MMC_MAXELFAC+1] = {0};
                //mmr_el_eq_neig(Mesh_id,El_id[e],el_neigs, nullptr);

                int nth_face = std::distance(& Adjncy[*it_xadj], it_neig_el);
                assert(nth_face >= 0);
                nth_face = (Adj_neig+(*it_xadj))[ nth_face ];
                // const int nth_face = std::distance(el_neigs+1, std::find(el_neigs+1,el_neigs+1+el_neigs[0],(*it_neig_el)+1) );
                assert(nth_face <= faces[0]);
                assert(nth_face >= 0);
                const int local_face_id = faces[ 1+ nth_face ];

                assert(local_face_id > 0);
                assert(local_face_id <= mmr_get_max_face_id(Mesh_id));


                // Checking whether result is correct
#ifndef NDEBUG
                int neig[2]={0};
                mmr_fa_neig(Mesh_id,local_face_id,neig, nullptr, nullptr, nullptr, nullptr, nullptr) ;
                //assert(neig[1] == 0);
                //assert(neig[0] == El_id[e]);
#endif
                assert(std::distance(& Adjncy[*it_xadj], it_neig_el) < MMC_MAXELFAC);
                assert(*it_neig_el >= *found_in_vtxdist);

                ipc_links[local_face_id].owner = owning_proc_id;
                // !!!!!!!!!!!!!!!!!!!! TODO: here is an error !!!!!!!!!!!!!
                ipc_links[local_face_id].id_at_owner = (*it_neig_el - VtxDist[owning_proc_id-1]) + 1; //+1 to make proper id [1..n] not [0..n-1]

                mf_check_debug(ipc_links[local_face_id].owner > 0,"Owner proc id cannot be <0 (now %d)",ipc_links[local_face_id].owner);
                mf_check_debug(ipc_links[local_face_id].id_at_owner > 0,"Id at owner cannot be <0 (now %d)",ipc_links[local_face_id].id_at_owner);

                mf_debug("Cross boundary link from el=%d face=%d to proc=%d el=%d",
                            El_id[e],local_face_id,owning_proc_id,ipc_links[local_face_id].id_at_owner);
            }
        }
    }

    mf_debug("No. of cross boundary links=%d",ipc_links.size());

    return retval;
}

/*------------------------------------------------------------
  ddr_update_IPC - to update IPC connectivity structures according to real state of mesh
------------------------------------------------------------*/
int ddr_IPC_update(const int Mesh_id, const int* ElemsChanged, const int nElems)
{

    mf_check_mem(ElemsChanged);
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    int rc=1;

    std::vector<bool>   faceChecked(mmr_get_nr_face(Mesh_id),false);

    // Find unset or ill-set face neighs.
    int faces[1+MMC_MAXELFAC]={0};
    for(int i=0; i < nElems; ++i) {
        rc = mmr_el_faces(Mesh_id,ElemsChanged[i],faces,NULL);

        for(int f=1; f < faces[0]; ++f) {
            if( !faces[f] ) {
                faceChecked[faces[f]]=true;
                int neigs[2]={0};
                mmr_fa_neig(Mesh_id,faces[f],neigs,NULL,NULL,NULL,NULL,NULL);

                // what to do next??
            }
        }

    }

    assert(pmesh.data.mp.PCSR.empty() == false);

}

/*------------------------------------------------------------
  ddr_expand_overlap - to expand overlap regions by one element layer
------------------------------------------------------------*/
// NOTE: if overlap does not exist at all, it will be created as 1 element overlap.
int ddr_IPC_expand_overlap(const int Mesh_id)
{
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);

    assert(pmesh.data.ipc.empty() == false);
    assert(pmesh.data.ipc_links.empty() == false);

    int retval=0;
    // We base on inter process connecitivty data (and assume that it is correct)
    // For each neigbouring proces, we seek list of elements, that we want to copy TO us.
    // But we also know, which elements this process want to copy FROM us.
    // NOTE: this 1-element overlap is based on the rule that smaller-id-proc
    // does't hava a copy of overlap elems - it just own them as very own.
    // So, proc have a copy of elements only FROM smaller-id-proc, and own elements
    // that are copied from it by the bigger-id-proc.
    // E.g. First proc will never copy anything from any proc, and no proc will copy from last-id-proc.

    std::map< int, std::vector<int> > elem_to_send_to_procs;

    for(const mmpt_data::ipc_links_type::value_type & it_ipc_links : pmesh.data.ipc_links) {
        if(it_ipc_links.second.owner > pcv_my_proc_id) {
            int neigs[2]={0};

            mmr_fa_eq_neig(Mesh_id,it_ipc_links.first,neigs,nullptr,nullptr);
            assert(neigs[0] != 0);
            elem_to_send_to_procs[it_ipc_links.second.owner].push_back(neigs[0]);
        }
    }

    for(const mmpt_data::ipc_type::value_type & it_ipc : pmesh.data.ipc) {

        if(it_ipc.first < pcr_my_proc_id()) { // smaller ids = bigger priority
//            // First send.
//            ddr_copy_full_elems(Mesh_id, pcr_my_proc_id(), it_ipc.first,
//                                elems_to_send.size(),
//                                elems_to_send.data());
            // Then recieve.
            ddr_transfer_full_elems(Mesh_id, MPI_ANY_SOURCE, pcr_my_proc_id(),
                                0,nullptr, DDE_COPY);
        }
        else { // it_ipc.first > pcr_my_proc_id() // bigger ids = smaller priority
            std::vector<int> & elems_to_send = elem_to_send_to_procs.at(it_ipc.first);
            // Duplicate entires are possible, so remove them.
            std::sort(elems_to_send.begin(),elems_to_send.end());
            elems_to_send.resize( std::distance( elems_to_send.begin(), std::unique( elems_to_send.begin(), elems_to_send.end()) ) );

            //            // First recieve.
//            ddr_copy_full_elems(Mesh_id, it_ipc.first, pcr_my_proc_id(),
//                                0,
//                                nullptr);
            // Then send.
            ddr_transfer_full_elems(Mesh_id, pcr_my_proc_id(), it_ipc.first,
                                elems_to_send.size(),
                                elems_to_send.data(), DDE_COPY);
        }
        mmr_test_mesh(Mesh_id);

        // After that, we have to update inter process connecitivty data.
        // Remove faces no longer at boundary from ipc data.
        // Put overlap faces into ipc data.
        // (validate that, they have MMC_SUB_BND flags set as neigs)
        // const mmpt_PCSR& csr = pmesh.data.mp.PCSR;
    }

    mmr_test_mesh(Mesh_id);

    return retval;
}




/*------------------------------------------------------------
  mmpr_partition_mesh - to to decompose the mesh and create subdomains by Metis (SFC - Parmetis) library
------------------------------------------------------------*/
int ddr_PCSR_partition_mesh(
  int Mesh_id, /* ID of the current mesh */
  int Part_amount, /* The number of parts to partition the mesh  */
  int Parition_tool /* Used partition tool. Can be set to MAC_USE_KWAY_GRAPH_PART_TOOL or MAC_USE_RB_GRAPH_PART_TOOL */
  )
{
  //int i;
    //idx_t *old_part=nullptr;
	/*++++++++++++++++ executable statements ++++++++++++++++*/

#ifdef DEBUG_MMM
    printf("mmpr_partition_mesh: starting for for Mesh_id %d and Part_amount %d \n", Mesh_id,Part_amount);
#endif

	if(1 == Part_amount) { // no partitioning needed
	  return 0;
	}

    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    pmesh.data.mp.nparts = Part_amount;
	
//	if(mmpv_my_proc_id != PCC_MASTER_PROC_ID)
//		return (-5); // (-5) - skip routine for other process than 0

	if((Parition_tool != MAC_USE_KWAY_GRAPH_PART_TOOL)
	   && (Parition_tool != MAC_USE_RB_GRAPH_PART_TOOL))			
		return (-6); // (-6) - incorrect partition tool

	switch(Parition_tool)
	{
		case MAC_USE_KWAY_GRAPH_PART_TOOL:

#ifdef DEBUG_MMM
    printf("mmpr_partition_mesh: calling ParMETIS_V3_PartKway for Mesh_id %d and Part_amount %d \n", Mesh_id,Part_amount);
#endif
    pmesh.data.mp.met_parmet_result = ParMETIS_V3_PartKway(
                pmesh.data.mp.PCSR.vtxDist(),
                pmesh.data.mp.PCSR.xadj(),
                pmesh.data.mp.PCSR.adjncy(),
                pmesh.data.mp.PCSR.vwgt(),
                pmesh.data.mp.PCSR.adjwgt(),
                & pmesh.data.mp.wgtflag,
                & pmesh.data.mp.numflag,
                & pmesh.data.mp.ncon,
                & pmesh.data.mp.nparts,
                pmesh.data.mp.tpwgts.data(),
                & pmesh.data.mp.ubvec,
                pmesh.data.mp.options,
                & pmesh.data.mp.objval,
                pmesh.data.mp.PCSR.part(),
                & pmesh.data.mp.comm
                );
        break;

//		case MAC_USE_RB_GRAPH_PART_TOOL:

//		  pmesh.data.mp.met_parmet_result = METIS_PartGraphRecursive(
//					& pmesh.data.nae,
//					& pmesh.data.mp.ncon,
//					pmesh.data.mp.CRS.xadj,
//					pmesh.data.mp.CRS.adjncy,
//					nullptr,
//					nullptr,
//					pmesh.data.mp.CRS.adjwgt,
//					& pmesh.data.mp.nparts,
//					nullptr,
//					nullptr,
//					pmesh.data.mp.options,
//					& pmesh.data.mp.objval,
//					pmesh.data.mp.part);
//			break;
	}

    mf_check(pmesh.data.mp.met_parmet_result>=0,"Failed to partition mesh!");

    //mmpr_recreate_owner_tables(Mesh_id);

    return pmesh.data.mp.met_parmet_result;
}

/*------------------------------------------------------------
 mmpr_improve_partitioning - to improve the quality of an existing a k-way partitioning
------------------------------------------------------------*/
int ddr_PCSR_improve_partitioning( /* returns: 1 - success, <0 - error code */
 int Mesh_id /* ID of the current mesh */
 )
{
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
#ifdef DEBUG_MMM
    printf("mmpr_improve_partitioning: calling ParMETIS_V3_RefineKway for Mesh_id %d",Mesh_id);
#endif

    int rc = pmesh.data.mp.met_parmet_result = ParMETIS_V3_RefineKway(
        pmesh.data.mp.PCSR.vtxDist(),
        pmesh.data.mp.PCSR.xadj(),
        pmesh.data.mp.PCSR.adjncy(),
        pmesh.data.mp.PCSR.vwgt(),
        pmesh.data.mp.PCSR.adjwgt(),
        & pmesh.data.mp.wgtflag,
        & pmesh.data.mp.numflag,
        & pmesh.data.mp.ncon,
        & pmesh.data.mp.nparts,
        pmesh.data.mp.tpwgts.data(),
        & pmesh.data.mp.ubvec,
        pmesh.data.mp.options,
        & pmesh.data.mp.objval,
        pmesh.data.mp.PCSR.part(),
        & pmesh.data.mp.comm
        );

    return rc;
}


/* INTERNAL ROUTINES (TREATED AS PRIVATE) */

/*------------------------------------------------------------
  mmpr_set_metis_options - to specify metis behaviour
------------------------------------------------------------*/
void mmpr_set_metis_options(const int Mesh_id, idx_t options[])
{
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    pmesh.data.mp.ncon = 1; // one constraint. Default value
    pmesh.data.mp.objval = 0; // out parameter. Reset.
    pmesh.data.mp.itr = 1000; // default value
    pmesh.data.mp.wgtflag = 1; // weights on the edges only

	/*++++++++++++++++ executable statements ++++++++++++++++*/

	METIS_SetDefaultOptions(options); // reset to defalult options for metis

	options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
	/*
		Specifies the type of objective. Possible values are:
		METIS_OBJTYPE_CUT Edge-cut minimization.
		METIS_OBJTYPE_VOL Total communication volume minimization.
	*/

	options[METIS_OPTION_CTYPE] = METIS_CTYPE_SHEM;
	/*
		Specifies the matching scheme to be used during coarsening. Possible values are:
		METIS_CTYPE_RM Random matching.
		METIS_CTYPE_SHEM Sorted heavy-edge matching.
	*/

	options[METIS_OPTION_IPTYPE] = METIS_IPTYPE_GROW;
	/*
		Determines the algorithm used during initial partitioning. Possible values are:
		METIS_IPTYPE_GROW Grows a bisection using a greedy strategy.
		METIS_IPTYPE_RANDOM Computes a bisection at random followed by a refinement.
		METIS_IPTYPE_EDGE Derives a separator from an edge cut.
		METIS_IPTYPE_NODE Grow a bisection using a greedy node-based strategy.
	*/

	options[METIS_OPTION_RTYPE] = METIS_RTYPE_FM;
	/*
		Determines the algorithm used for refinement. Possible values are:
		METIS_RTYPE_FM FM-based cut refinement.
		METIS_RTYPE_GREEDY Greedy-based cut and volume refinement.
		METIS_RTYPE_SEP2SIDED Two-sided node FM refinement.
		METIS_RTYPE_SEP1SIDED One-sided node FM refinement.
	*/

    options[METIS_OPTION_MINCONN] = 1;
    /*
     *Specifies that the partitioning routines should try to minimize the maximum degree of the subdomain graph,
    i.e., the graph in which each partition is a node, and edges connect subdomains with a shared interface.
    0 Does not explicitly minimize the maximum connectivity.
    1 Explicitly minimize the maximum connectivity.
    */

    options[METIS_OPTION_CONTIG] = 1;
    /*
     *Specifies that the partitioning routines should try to produce partitions that are contiguous. Note that if the
    input graph is not connected this option is ignored.
    0 Does not force contiguous partitions.
    1 Forces contiguous partitions.
    */

    options[METIS_OPTION_DBGLVL] = METIS_DBG_INFO | METIS_DBG_MOVEINFO;
    /*
    Specifies the amount of progress/debugging information will be printed during the execution of the algo-
    rithms. The default value is 0 (no debugging/progress information). A non-zero value can be supplied that
    is obtained by a bit-wise OR of the following values.
    METIS_DBG_INFO (1) Prints various diagnostic messages.
    METIS_DBG_TIME (2) Performs timing analysis.
    METIS_DBG_COARSEN (4) Displays various statistics during coarsening.
    METIS_DBG_REFINE (8) Displays various statistics during refinement.
    METIS_DBG_IPART (16) Displays various statistics during initial partitioning.
    METIS_DBG_MOVEINFO (32) Displays detailed information about vertex moves during refine-
                           ment.
    METIS_DBG_SEPINFO (64) Displays information about vertex separators.
    METIS_DBG_CONNINFO (128) Displays information related to the minimization of subdomain
                            connectivity.
    METIS_DBG_CONTIGINFO (256) Displays information related to the elimination of connected com-
                              ponents.
    Note that the numeric values are provided for use with the -dbglvl option of M ETIS’ stand-alone pro-
    grams. For the API routines it is sufficient to OR the above constants.
    */
}

/*------------------------------------------------------------
  mmpr_set_parmetis_options - to specify parmetis behaviour
------------------------------------------------------------*/
void mmpr_set_parmetis_options(const int Mesh_id, idx_t options[])
{
	/*++++++++++++++++ executable statements ++++++++++++++++*/
    mmpt_mesh & pmesh = *mmpr_select_mesh(Mesh_id);
    mmpr_set_metis_options(Mesh_id,options);

    pmesh.data.mp.ubvec = 1.05f;

	options[0] = 1; // disable default options
	options[1] = 0; // random number seed
	options[2] = PARMETIS_PSR_COUPLED; // # of partitions == # of processors ==  # of processes 
}



void mmpr_init_data(
  int Mesh_id, /* ID of the current mesh */
  int Part_amount /* The number of parts to partition the mesh  */
)
{
    /* select the proper mesh data structure */
      mmpt_mesh& pmesh = *mmpr_select_mesh(Mesh_id);

      pmesh.data.mp.comm = MPI_COMM_WORLD;

      //mmpr_recreate_owner_tables(Mesh_id);

    /// At this point each process:
    /// 1. Already readed own unique mesh file (if exist).
    /// So each process can have own unique mesh part readed from file or no mesh at all.
    /// 2. Have correct knowlege (in (mmpt_mesh*) pmesh object) about total number of all mesh enities
    /// NOT including all other unique files readed by other processes.
    ///
    /// 1. Now mesh have to be repartitioned.
    /// 2. Than parts distribution must be balanced between processes.
    /// 3. After balancing, the overlap have to be created.
	
    pmesh.data.mesh_id = Mesh_id;
    pmesh.data.mp.nparts = Part_amount;
    //pmesh.data.parition_tool = Parition_tool;

    pmesh.data.ne = mmr_get_max_elem_id(Mesh_id);
    pmesh.data.nae = mmr_get_nr_elem(Mesh_id);
    pmesh.data.nn = mmr_get_max_node_id(Mesh_id);
    pmesh.data.mp.tpwgts.resize(Part_amount,1.0/static_cast<double>(Part_amount));

    mmpr_set_parmetis_options(Mesh_id, pmesh.data.mp.options);

    ddr_mesh_to_distributed_CRSGraph(Mesh_id);

    assert(pcr_is_parallel_initialized() == 1);
    assert(!pmesh.data.mp.PCSR.empty());
    assert(pmesh.data.mp.options != nullptr );
    assert(!pmesh.data.mp.PCSR.empty());
}

///*------------------------------------------------------------
//  mmpr_end_work - free memory for internal data structures
//------------------------------------------------------------*/
//void mmpr_end_work()
//{
//	/*++++++++++++++++ executable statements ++++++++++++++++*/

////  mmpr_free_CRS(& pmesh.data.mp.PCSR);
  
////	UTM_SAFE_FREE_PTR(pmesh.data.mp.vtxdist);
////	UTM_SAFE_FREE_PTR(pmesh.data.mp.tpwgts);
////	UTM_SAFE_FREE_PTR(pmesh.data.mp.part_local);
//	//UTM_SAFE_FREE_PTR(pmesh.data.mp.adjncy_local);
//	//UTM_SAFE_FREE_PTR(pmesh.data.mp.adjwgt_local);
//	//UTM_SAFE_FREE_PTR(pmesh.data.mp.xadj_local);
//}




///*------------------------------------------------------------
//  mmpr_create_overlap - create overlap, fill data structure, set internal
//------------------------------------------------------------*/
//void mmpr_create_overlap(const int MeshID)
//{
//    /*
//        NOTE:
//                when creating overlap the rule is:
//                    the subdoamin with bigger ID contains border elements of smaller id subdomains.
//    */
	

//}



#ifdef __cplusplus
}
#endif
