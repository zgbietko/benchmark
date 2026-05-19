#ifndef _petsc_lah_
#define _petsc_lah_

#include "lah_block.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void pets_allocate_SM_and_LV(int Nrdof_glob);
extern void petsc_initialize_SM_and_LV();
extern void petsc_perform_BJ_iterations(double* V, double* B, int Ndof, int Nr_prec);
extern void fill_crs_matrix(itt_matrices *matrix);

#ifdef __cplusplus
}
#endif

#endif
