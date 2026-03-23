#ifndef UTH_BC_H_
#define UTH_BC_H_

#include "uth_mesh.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    int  BC_num;
    int  Ids[2];
    utt_mesh_bc_type  Id_type;   //group, block, material
} utt_bc_assignment;


int utr_bc_read_assignments(const char * FilenameBC);
int utr_bc_get_n_assignments();
int utr_bc_get_assignment(const int number, utt_bc_assignment * bc_num_to_set);

#ifdef __cplusplus
}
#endif

#endif // UTH_BC_H_
