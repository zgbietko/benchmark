#include <vector>

#include "uth_log.h"
#include "uth_bc.h"


// HERE
std::vector<utt_bc_assignment>  new_bc_nums;

#ifdef __cplusplus
extern "C"
{
#endif

int utr_bc_read_assigments(const char * FilenameBC)
{
    // libconfig read
    return 0;
}

int utr_bc_get_n_assignments()
{
    return new_bc_nums.size();
}

int utr_bc_get_assignment(const int number, utt_bc_assignment * bc_num_to_set)
{
    mf_check_mem(bc_num_to_set);

    memcpy(bc_num_to_set, & new_bc_nums[number], sizeof(utt_bc_assignment) );

    return 0;
}

#ifdef __cplusplus
}
#endif
