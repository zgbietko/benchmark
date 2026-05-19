#ifndef UTH_MESH_H
#define UTH_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
/** \defgroup UTM_MESH Mesh Utilities
/** \ingroup UTM
/** @{
/** */

typedef enum {
    UTE_MESH_BC_GROUP=0,
    UTE_MESH_BC_BLOCK=1,
    UTE_MESH_BC_MATERIAL=2   
} utt_mesh_bc_type;        //group, block, material


///
/// \brief utr_mesh_insert_BC_contact To insert contact faces into the given mesh.
///
/// \param Mesh_id mesh in: to update
/// \param N_contacts in: number of contacts //group, block, material
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
        const utt_mesh_bc_type *Types);



int utr_mesh_insert_new_bcnums(const int Mesh_id,
                               const int N_bcnums_to_set,
                               int * Bcnums,
                               int * Ids,
                               utt_mesh_bc_type * Bc_types);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif // UTH_MESH_H
