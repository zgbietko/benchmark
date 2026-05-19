#ifndef __uth_log_h__
#define __uth_log_h__

/// This file is ONLY for debugging/logging/checking macros.
/// Do not add any other macros/functions/etc. which are used during computations.
/// K.Michalik - 10.2013 initial version
/// K.Michalik - 04.2014 Interactive_output access.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#ifdef PARALLEL
#include "pch_intf.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 \defgroup UTM_LOG Logging Utilities
 \ingroup UTM
 @{
 */

// For fast printing arrays.
#ifndef mf_print_array
#define mf_print_array(ar,size,typ) { for(int i=0; i < size; ++i){ fprintf(utv_log_out,typ,ar[i]); fprintf(utv_log_out,", "); } fprintf(utv_log_out,"\n"); }
#endif

// this macro is for showing only the filename, not full file path
#ifndef _WIN32
    #define MF_FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#else
    #define MF_FILE_NAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif


extern FILE* utv_log_out;

#ifdef NDEBUG
# define mf_debug(M, ...)
# define mf_check_debug(A, M, ...)
# define mfp_debug(M, ...)
# define mfp_check_debug(A, M, ...)
#define mf_log_err(M, ...) fprintf(utv_log_out, "[ERROR] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__)

#define mf_fatal_err(M, ...) { fprintf(utv_log_out, "[FATAL ERROR] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__); exit(-3); }

#define mf_log_warn(M, ...) fprintf(utv_log_out, "[WARN] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__)

#define mf_log_info(M, ...) fprintf(utv_log_out, "[INFO] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__)

#define mf_log_test_fail(M, ...) fprintf(utv_log_out, "[TEST FAILED] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__)

#define mf_check(A, M, ...) if(!(A)) { mf_log_err(M, ##__VA_ARGS__); assert(!M); exit(-1);}

#define mf_sentinel(M, ...)  { mf_log_err(M, ##__VA_ARGS__); }

#define mf_check_mem(A) mf_check((A), "Out of memory.")

#define mf_check_info(A, M, ...) if(!(A)) { mf_log_info(M,##__VA_ARGS__); }

#define mf_test(A, M, ...) if(!(A)) { mf_log_test_fail(M, ##__VA_ARGS__); assert(!M);}
#else
# define mf_debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__)
# define mf_check_debug(A, M, ...) if(!(A)) { mf_debug(M, ##__VA_ARGS__); assert(!M); }
# ifdef PARALLEL
  //#   define mfp_debug(M, ...) {int *idp = &pcv_my_proc_id; fprintf(stderr, "DEBUG[%d] %s:%d: " M "\n", *idp, MF_FILE_NAME, __LINE__, ##__VA_ARGS__); }
  //#   define mfp_check_debug(A, M, ...) if(!A) { mfp_debug(M, ##__VA_ARGS__); assert(!M); }
#	define mfp_debug mf_debug
#	define mfp_check_debug mf_check_debug
# else
#	define mfp_debug mf_debug
#	define mfp_check_debug mf_check_debug
# endif
#define mf_log_err(M, ...) fprintf(utv_log_out, "[ERROR] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__)

#define mf_fatal_err(M, ...) { fprintf(utv_log_out, "[FATAL ERROR] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__); exit(-3); }

#define mf_log_warn(M, ...) fprintf(utv_log_out, "[WARN]  " M "\n", ##__VA_ARGS__)

#define mf_log_info(M, ...) fprintf(utv_log_out, "[INFO]  " M "\n", ##__VA_ARGS__)

#define mf_log_test_fail(M, ...) fprintf(utv_log_out, "[TEST FAILED] (%s:%d ) " M "\n", MF_FILE_NAME, __LINE__, ##__VA_ARGS__)

#define mf_check(A, M, ...) if(!(A)) { mf_log_err(M, ##__VA_ARGS__); assert(!M); exit(-1);}

#define mf_sentinel(M, ...)  { mf_log_err(M, ##__VA_ARGS__); }

#define mf_check_mem(A) mf_check((A), "Out of memory.")

#define mf_check_info(A, M, ...) if(!(A)) { mf_log_info(M,##__VA_ARGS__); }

#define mf_test(A, M, ...) if(!(A)) { mf_log_test_fail(M, ##__VA_ARGS__); assert(!M);}
#endif



//#ifndef nullptr
//# define nullptr 0
//#endif


/** @} */ // end of group

#ifdef __cplusplus
}
#endif


#endif // __uth_log_h__
