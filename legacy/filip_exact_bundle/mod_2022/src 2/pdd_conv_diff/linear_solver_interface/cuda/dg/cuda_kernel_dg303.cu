#include <stdlib.h>
#include <stdio.h>
#include <cuda.h>
//#include <cuda_runtime_api.h>

__constant__ float shp_fun_reference[40*48*4];

#include "apr_cuda_num_int_el_dg_reference_one_el_one_workgroup_prism_parts303.cu"

// interface of cuda implementation
//#include"../../../tmd_opencl/tmh_ocl.h"

//#define SCALAR double
#define SCALAR float


#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

//invokeJacKernel( gridSize,
//              blockSize,
//              execution_parameters_devCUDA,
//              gauss_dat_devCUDA,
//              cmDevBufInCUDA_JAC,
//              cmDevBuf_JAC);

extern "C" int invokeJacKernel(
        int gridSize,
        int blockSize,
        int* execution_parameters,
        SCALAR* gauss_dat, // integration points data of elements having given p
        SCALAR* el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
        //SCALAR* jac_dat, // computed jacobian data
		SCALAR* shpfun_ref, //output_added shp_fun_ref_comp
		SCALAR* shp_dfun
)
{
    dim3 blocks(gridSize, 1, 1);
    dim3 threadsPerBlock(blockSize, 1, 1);
//    cuFunction function = tmv_cuda_struct.function;
//    int shr_size;
//    void *kernelParams[5] = { execution_parameters,
//    								 gauss_dat,
//    								 shpfun_ref,
//    								 el_data_in,
//    								 stiff_mat_out };

    cudaMemcpyToSymbol(shp_fun_reference, shpfun_ref, 40*48*4*sizeof(SCALAR), 0);

    printf("\n\nInvoking JAC kernel with Blocks: %d threads per block: %d",blocks.x,threadsPerBlock.x);
    tmr_cuda_prepare_jacobian_data<<<blocks,threadsPerBlock>>>(execution_parameters,
						 gauss_dat,
						 el_data_in,
						 shp_dfun);
//
//    cuFuncGetAttribute	(&shr_size,CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES,function);
//
//    cuLaunchKernel(function,gridSize, 1, 1, blockSize, 1, 1,shr_size,NULL,kernelParams,NULL);

//    gpuErrchk( cudaPeekAtLastError() );

    if(cudaDeviceSynchronize() == ::cudaSuccess)
    {
        printf("\nKERNEL SUCCEEDED\n\n");
	return 1;
    }
    else
    {
        printf("\nKERNEL FAIL, error: %d : %s",
	       cudaPeekAtLastError(), cudaGetErrorString(cudaPeekAtLastError()));
	return 0;
    }
}

//invokeKernel( gridSize,
//                   blockSize,
//                   execution_parameters_devCUDA,
//                   shape_fun_devCUDA,
//                   cmDevBufInCUDA,
//					cmDevBuf_JAC,
//                   cmDevBufOutCUDA);

extern "C" int invokeKernel(
        int gridSize, 
        int blockSize,
        int* execution_parameters,
        //SCALAR* shpfun_ref, // shape functions on a reference element
        SCALAR* el_data_in, // data for integration of NR_ELEMS_THIS_KERCALL elements
		SCALAR* shp_dfun, // computed jacobian data
        SCALAR* stiff_mat_out // result of integration of NR_ELEMS_THIS_KERCALL elements
)
{
    dim3 blocks(gridSize, 1, 1);
    dim3 threadsPerBlock(blockSize, 1, 1);


//    cuFunction function = tmv_cuda_struct.function;
//    int shr_size;
//    void *kernelParams[5] = { execution_parameters,
//    								 gauss_dat,
//    								 shpfun_ref,
//    								 el_data_in,
//    								 stiff_mat_out };
    
    cudaFuncSetCacheConfig(tmr_cuda_num_int_el, cudaFuncCachePreferL1);

    printf("\n\nInvoking kernel with Blocks: %d threads per block: %d",blocks.x,threadsPerBlock.x);
    tmr_cuda_num_int_el<<<blocks,threadsPerBlock>>>(execution_parameters,
						 //shpfun_ref,
						 el_data_in,
						 shp_dfun,
						 stiff_mat_out);
//
//    cuFuncGetAttribute	(&shr_size,CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES,function);
//
//    cuLaunchKernel(function,gridSize, 1, 1, blockSize, 1, 1,shr_size,NULL,kernelParams,NULL);

//    gpuErrchk( cudaPeekAtLastError() );
    
    if(cudaDeviceSynchronize() == ::cudaSuccess)
    {
        printf("\nKERNEL SUCCEEDED\n\n");
	return 1;
    }
    else
    {
        printf("\nKERNEL FAIL, error: %d : %s",
	       cudaPeekAtLastError(), cudaGetErrorString(cudaPeekAtLastError()));
	return 0;
    }
}


