#include "uth_mesh.h"
#include "uth_log.h"
#include "uth_mat.h"
#include "mmh_intf.h"


#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ctime>
#include <cmath>
//#include <GL/gl.h>
#include <assert.h>
#include <vector>
using namespace std;


#include "uth_bc.h"

//namespace MESH_UTILS {

#include "../mmd_remesh/ticooMesh3D.h"
//}

void utr_mesh_read_ticco_from_intf(const int Mesh_id, TicooMesh3D & mymesh)
{
	
//	int inode = 0;
//	while (0 != (mmr_get_next_node_all(Mesh_id, inode)) {
		
//    }

//	int iface = 0;
//	while (0 != (mmr_get_next_face_all(Mesh_id, iface)) {

//    }

}

#ifdef __cplusplus
extern "C" {
#endif

/**
/** \defgroup UTM_MESH Mesh Utilities
/** \ingroup UTM
/** @{
/** */


///
/// \brief utr_mesh_insert_BC_contact To insert contact faces into the given mesh.
///
/// \param Mesh_id mesh in: to update
/// \param N_contacts in: number of contacts
/// \param BC_nums[N_contacts] in: array of BC numbers
/// \param groupIDs[2*N_contacts] in: ID between which contact BC will be implemeted
/// \param Types[N_contacts] in: type specyfing IDs type
/// \return number of inserted BC contacts
///
int utr_mesh_insert_BC_contact(
		const char *Workdir,
		const int Mesh_id,
        const int N_contacts,
        const int *BC_nums,
        const int *IDs,
        const utt_mesh_bc_type *Types)
{
    int n_inserted_contacts=0;

    if(N_contacts > 0) {
        mf_check_mem(BC_nums);
        mf_check_mem(IDs);
        mf_check_mem(Types);


        int n_groups = mmr_groups_number(Mesh_id);

        mf_log_info("Found %d groups in mesh.", n_groups);

        std::vector<int>    groups(n_groups), blocks, mats, bcs;
		std::vector<double> tempB;
		
        mmr_groups_ids(Mesh_id, groups.data(), n_groups);
        mf_log_info("Groups: %d, %d",groups[0],groups[1]);

		for(int i=0,ile = utr_temp2block_size();i<ile;++i){
			
				tempB.push_back(utr_temp2block_id(i));
		}
		
        for(int i=0; i < n_groups; ++i) {
            blocks.push_back(groups[i]);
            blocks.push_back(utr_mat_get_blockID(groups[i]));

            mats.push_back(groups[i]);
            mats.push_back(utr_mat_get_matID(groups[i]));
        }


        for(int i=0; i < N_contacts; ++i) {
            bcs.push_back(Types[i]);
            bcs.push_back(IDs[2*i]);
            bcs.push_back(IDs[2*i+1]);
			bcs.push_back(BC_nums[i]);
        }
		
		
		
		mmr_split_into_blocks_add_contact(Workdir,Mesh_id,mats.data(), mats.size(),blocks.data(), blocks.size(),bcs.data(), bcs.size(),tempB.data(),tempB.size());
		
 //mf_log_info("Groups: %f, %f, %f, %f",tempB[0],tempB[1],tempB[2],tempB[3]);
 
		

		//mf_log_info("Groups: %f, %f",tempB[2],tempB[3]);
		//mf_log_info("Groups: %d, %d",blocks[2],blocks[3]);
		//TicooMesh3D mes3d_(50);

//		// wczytanie?

		
		//double tB[4] ={1.0,1102.0,2.0,295.0};
		//int lB=4;
		
		/*
		mes3d_.mmr_split_into_blocks_add_contact(
			mats.data(), mats.size(),
			blocks.data(), blocks.size(),
			bcs.data(), bcs.size(),tempB.data(),tempB.size());
*/



		

		// zapis

		n_inserted_contacts = N_contacts;


    }
    return n_inserted_contacts;
}



int utr_mesh_insert_new_bcnums(const int Mesh_id,
                               const int N_bcnums_to_set,
                               int * Bcnums,
                               int * Ids,
                               utt_mesh_bc_type * Bc_types)
{
    // HERE IS MAGIC
	return 0;
}

/**@}*/


#ifdef __cplusplus
}
#endif
