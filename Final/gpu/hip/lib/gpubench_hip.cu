#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstddef>

extern "C" {

/**
 * Returns the number of HIP devices visible to the process.
 * On success returns device count (>=0).
 * On failure returns negative hipError_t code.
 */
int gpu_hip_get_device_count() {
    int count = 0;
    hipError_t err = hipGetDeviceCount(&count);
    if (err != hipSuccess) {
        return -static_cast<int>(err);
    }
    return count;
}

/**
 * Writes device name to name_buf (null-terminated).
 * buf_len is the size of the buffer in bytes.
 * Returns 0 on success, or negative hipError_t code on failure.
 */
int gpu_hip_get_device_name(int device_index, char* name_buf, int buf_len) {
    if (!name_buf || buf_len <= 0) {
        return -1;
    }
    hipDeviceProp_t prop;
    hipError_t err = hipGetDeviceProperties(&prop, device_index);
    if (err != hipSuccess) {
        return -static_cast<int>(err);
    }
    std::snprintf(name_buf, static_cast<std::size_t>(buf_len), "%s", prop.name);
    return 0;
}

/**
 * Returns basic device properties:
 *  - major/minor (for NVIDIA via HIP; on AMD may be 0/0)
 *  - global memory size in bytes
 *
 * Any of the output pointers may be nullptr if the caller is not interested in a value.
 * Returns 0 on success, or negative hipError_t code on failure.
 */
int gpu_hip_get_device_props(
    int device_index,
    int* major_out,
    int* minor_out,
    std::size_t* global_mem_bytes_out
) {
    hipDeviceProp_t prop;
    hipError_t err = hipGetDeviceProperties(&prop, device_index);
    if (err != hipSuccess) {
        return -static_cast<int>(err);
    }

    if (major_out) *major_out = prop.major;
    if (minor_out) *minor_out = prop.minor;
    if (global_mem_bytes_out) *global_mem_bytes_out = static_cast<std::size_t>(prop.totalGlobalMem);
    return 0;
}

__global__ void mem_copy_kernel(float* __restrict__ dst, const float* __restrict__ src, std::size_t n) {
    std::size_t tid = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    std::size_t stride = static_cast<std::size_t>(blockDim.x) * gridDim.x;
    for (std::size_t i = tid; i < n; i += stride) {
        dst[i] = src[i];
    }
}

__global__ void fma_kernel(float* __restrict__ out, const float* __restrict__ a, const float* __restrict__ b,
                           std::size_t n, int iters) {
    std::size_t tid = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    std::size_t stride = static_cast<std::size_t>(blockDim.x) * gridDim.x;
    for (std::size_t i = tid; i < n; i += stride) {
        float x = a[i];
        float y = b[i];
        float z = out[i];
        #pragma unroll 4
        for (int k = 0; k < iters; k++) {
            z = x * y + z;
            x = x * 1.00000011920928955078125f + 0.000000059604644775390625f; // small perturbation
        }
        out[i] = z;
    }
}

__global__ void pointer_chase_kernel(
    const unsigned int* __restrict__ next_idx,
    unsigned int start_idx,
    int iters,
    unsigned int* __restrict__ out
) {
    unsigned int idx = start_idx;
    for (int k = 0; k < iters; ++k) {
        idx = next_idx[idx];
    }
    if (out) {
        out[0] = idx;
    }
}

/**
 * Device-to-device copy throughput using a simple kernel.
 * On success returns 0 and writes elapsed time [ms] to *elapsed_ms_out.
 * On failure returns negative hipError_t code.
 */
int gpu_hip_memcpy_bandwidth(
    int device_index,
    std::size_t bytes,
    int iters,
    double* elapsed_ms_out
) {
    if (!elapsed_ms_out) {
        return -1;
    }
    if (iters <= 0) {
        return -2;
    }

    hipError_t err = hipSetDevice(device_index);
    if (err != hipSuccess) {
        return -static_cast<int>(err);
    }

    std::size_t n = bytes / sizeof(float);
    if (n == 0) n = 1;

    float* d_src = nullptr;
    float* d_dst = nullptr;

    err = hipMalloc(&d_src, n * sizeof(float));
    if (err != hipSuccess) return -static_cast<int>(err);
    err = hipMalloc(&d_dst, n * sizeof(float));
    if (err != hipSuccess) { hipFree(d_src); return -static_cast<int>(err); }

    // warm-up init
    err = hipMemset(d_src, 1, n * sizeof(float));
    if (err != hipSuccess) { hipFree(d_src); hipFree(d_dst); return -static_cast<int>(err); }
    err = hipMemset(d_dst, 0, n * sizeof(float));
    if (err != hipSuccess) { hipFree(d_src); hipFree(d_dst); return -static_cast<int>(err); }

    int block = 256;
    int grid = 0;
    // basic occupancy-like heuristic
    grid = static_cast<int>((n + block - 1) / block);
    if (grid > 65535) grid = 65535;

    // warm-up
    hipLaunchKernelGGL(mem_copy_kernel, dim3(grid), dim3(block), 0, 0, d_dst, d_src, n);
    err = hipDeviceSynchronize();
    if (err != hipSuccess) { hipFree(d_src); hipFree(d_dst); return -static_cast<int>(err); }

    hipEvent_t start, stop;
    hipEventCreate(&start);
    hipEventCreate(&stop);

    hipEventRecord(start, 0);
    for (int i = 0; i < iters; i++) {
        hipLaunchKernelGGL(mem_copy_kernel, dim3(grid), dim3(block), 0, 0, d_dst, d_src, n);
    }
    hipEventRecord(stop, 0);
    err = hipEventSynchronize(stop);
    if (err != hipSuccess) {
        hipEventDestroy(start);
        hipEventDestroy(stop);
        hipFree(d_src); hipFree(d_dst);
        return -static_cast<int>(err);
    }

    float ms = 0.0f;
    hipEventElapsedTime(&ms, start, stop);
    *elapsed_ms_out = static_cast<double>(ms);

    hipEventDestroy(start);
    hipEventDestroy(stop);

    hipFree(d_src);
    hipFree(d_dst);
    return 0;
}

/**
 * Host-to-device memcpy bandwidth using hipMemcpyAsync.
 * On success returns 0 and writes elapsed time [ms] to *elapsed_ms_out.
 */
int gpu_hip_memcpy_h2d_bandwidth(
    int device_index,
    std::size_t bytes,
    int iters,
    double* elapsed_ms_out
) {
    if (!elapsed_ms_out) return -1;
    if (iters <= 0) return -2;

    hipError_t err = hipSetDevice(device_index);
    if (err != hipSuccess) return -static_cast<int>(err);

    if (bytes == 0) bytes = sizeof(float);

    void* h_src = nullptr;
    void* d_dst = nullptr;

    err = hipHostMalloc(&h_src, bytes, hipHostMallocDefault);
    if (err != hipSuccess) return -static_cast<int>(err);
    err = hipMalloc(&d_dst, bytes);
    if (err != hipSuccess) { hipHostFree(h_src); return -static_cast<int>(err); }

    // Warm-up
    err = hipMemcpy(d_dst, h_src, bytes, hipMemcpyHostToDevice);
    if (err != hipSuccess) {
        hipHostFree(h_src);
        hipFree(d_dst);
        return -static_cast<int>(err);
    }

    hipEvent_t start, stop;
    hipEventCreate(&start);
    hipEventCreate(&stop);

    hipEventRecord(start, 0);
    for (int i = 0; i < iters; i++) {
        hipMemcpyAsync(d_dst, h_src, bytes, hipMemcpyHostToDevice, 0);
    }
    hipEventRecord(stop, 0);
    err = hipEventSynchronize(stop);
    if (err != hipSuccess) {
        hipEventDestroy(start);
        hipEventDestroy(stop);
        hipHostFree(h_src);
        hipFree(d_dst);
        return -static_cast<int>(err);
    }

    float ms = 0.0f;
    hipEventElapsedTime(&ms, start, stop);
    *elapsed_ms_out = static_cast<double>(ms);

    hipEventDestroy(start);
    hipEventDestroy(stop);
    hipHostFree(h_src);
    hipFree(d_dst);
    return 0;
}

/**
 * Device-to-host memcpy bandwidth using hipMemcpyAsync.
 * On success returns 0 and writes elapsed time [ms] to *elapsed_ms_out.
 */
int gpu_hip_memcpy_d2h_bandwidth(
    int device_index,
    std::size_t bytes,
    int iters,
    double* elapsed_ms_out
) {
    if (!elapsed_ms_out) return -1;
    if (iters <= 0) return -2;

    hipError_t err = hipSetDevice(device_index);
    if (err != hipSuccess) return -static_cast<int>(err);

    if (bytes == 0) bytes = sizeof(float);

    void* h_dst = nullptr;
    void* d_src = nullptr;

    err = hipHostMalloc(&h_dst, bytes, hipHostMallocDefault);
    if (err != hipSuccess) return -static_cast<int>(err);
    err = hipMalloc(&d_src, bytes);
    if (err != hipSuccess) { hipHostFree(h_dst); return -static_cast<int>(err); }
    err = hipMemset(d_src, 0, bytes);
    if (err != hipSuccess) { hipHostFree(h_dst); hipFree(d_src); return -static_cast<int>(err); }

    // Warm-up
    err = hipMemcpy(h_dst, d_src, bytes, hipMemcpyDeviceToHost);
    if (err != hipSuccess) {
        hipHostFree(h_dst);
        hipFree(d_src);
        return -static_cast<int>(err);
    }

    hipEvent_t start, stop;
    hipEventCreate(&start);
    hipEventCreate(&stop);

    hipEventRecord(start, 0);
    for (int i = 0; i < iters; i++) {
        hipMemcpyAsync(h_dst, d_src, bytes, hipMemcpyDeviceToHost, 0);
    }
    hipEventRecord(stop, 0);
    err = hipEventSynchronize(stop);
    if (err != hipSuccess) {
        hipEventDestroy(start);
        hipEventDestroy(stop);
        hipHostFree(h_dst);
        hipFree(d_src);
        return -static_cast<int>(err);
    }

    float ms = 0.0f;
    hipEventElapsedTime(&ms, start, stop);
    *elapsed_ms_out = static_cast<double>(ms);

    hipEventDestroy(start);
    hipEventDestroy(stop);
    hipHostFree(h_dst);
    hipFree(d_src);
    return 0;
}

/**
 * FMA throughput kernel.
 * On success returns 0 and writes elapsed time [ms] to *elapsed_ms_out.
 * On failure returns negative hipError_t code.
 */
int gpu_hip_fma_throughput(
    int device_index,
    std::size_t n_elems,
    int iters_inner,
    double* elapsed_ms_out
) {
    if (!elapsed_ms_out) return -1;
    if (iters_inner <= 0) return -2;

    hipError_t err = hipSetDevice(device_index);
    if (err != hipSuccess) return -static_cast<int>(err);

    if (n_elems == 0) n_elems = 1;

    float* d_a = nullptr;
    float* d_b = nullptr;
    float* d_out = nullptr;

    err = hipMalloc(&d_a, n_elems * sizeof(float));
    if (err != hipSuccess) return -static_cast<int>(err);
    err = hipMalloc(&d_b, n_elems * sizeof(float));
    if (err != hipSuccess) { hipFree(d_a); return -static_cast<int>(err); }
    err = hipMalloc(&d_out, n_elems * sizeof(float));
    if (err != hipSuccess) { hipFree(d_a); hipFree(d_b); return -static_cast<int>(err); }

    err = hipMemset(d_a, 1, n_elems * sizeof(float));
    if (err != hipSuccess) { hipFree(d_a); hipFree(d_b); hipFree(d_out); return -static_cast<int>(err); }
    err = hipMemset(d_b, 2, n_elems * sizeof(float));
    if (err != hipSuccess) { hipFree(d_a); hipFree(d_b); hipFree(d_out); return -static_cast<int>(err); }
    err = hipMemset(d_out, 0, n_elems * sizeof(float));
    if (err != hipSuccess) { hipFree(d_a); hipFree(d_b); hipFree(d_out); return -static_cast<int>(err); }

    int block = 256;
    int grid = static_cast<int>((n_elems + block - 1) / block);
    if (grid > 65535) grid = 65535;

    // warm-up
    hipLaunchKernelGGL(fma_kernel, dim3(grid), dim3(block), 0, 0, d_out, d_a, d_b, n_elems, iters_inner);
    err = hipDeviceSynchronize();
    if (err != hipSuccess) { hipFree(d_a); hipFree(d_b); hipFree(d_out); return -static_cast<int>(err); }

    hipEvent_t start, stop;
    hipEventCreate(&start);
    hipEventCreate(&stop);

    hipEventRecord(start, 0);
    hipLaunchKernelGGL(fma_kernel, dim3(grid), dim3(block), 0, 0, d_out, d_a, d_b, n_elems, iters_inner);
    hipEventRecord(stop, 0);
    err = hipEventSynchronize(stop);
    if (err != hipSuccess) {
        hipEventDestroy(start);
        hipEventDestroy(stop);
        hipFree(d_a); hipFree(d_b); hipFree(d_out);
        return -static_cast<int>(err);
    }

    float ms = 0.0f;
    hipEventElapsedTime(&ms, start, stop);
    *elapsed_ms_out = static_cast<double>(ms);

    hipEventDestroy(start);
    hipEventDestroy(stop);

    hipFree(d_a); hipFree(d_b); hipFree(d_out);
    return 0;
}

/**
 * Pointer-chasing latency benchmark (dependent loads, single thread).
 * On success writes elapsed time [ms] for `iters` dependent loads.
 */
int gpu_hip_pointer_chase_latency(
    int device_index,
    std::size_t n,
    int iters,
    double* elapsed_ms_out
) {
    if (!elapsed_ms_out) return -1;
    if (iters <= 0) return -2;
    if (n == 0) n = 1;

    hipError_t err = hipSetDevice(device_index);
    if (err != hipSuccess) return -static_cast<int>(err);

    unsigned int* h_next = new unsigned int[n];
    const unsigned int stride = 17u;
    for (std::size_t i = 0; i < n; ++i) {
        h_next[i] = static_cast<unsigned int>((i + stride) % n);
    }

    unsigned int* d_next = nullptr;
    unsigned int* d_out = nullptr;

    err = hipMalloc(&d_next, n * sizeof(unsigned int));
    if (err != hipSuccess) {
        delete[] h_next;
        return -static_cast<int>(err);
    }
    err = hipMalloc(&d_out, sizeof(unsigned int));
    if (err != hipSuccess) {
        hipFree(d_next);
        delete[] h_next;
        return -static_cast<int>(err);
    }

    err = hipMemcpy(d_next, h_next, n * sizeof(unsigned int), hipMemcpyHostToDevice);
    delete[] h_next;
    if (err != hipSuccess) {
        hipFree(d_next);
        hipFree(d_out);
        return -static_cast<int>(err);
    }

    hipEvent_t start, stop;
    hipEventCreate(&start);
    hipEventCreate(&stop);

    // warm-up
    hipLaunchKernelGGL(pointer_chase_kernel, dim3(1), dim3(1), 0, 0, d_next, 0u, iters, d_out);
    err = hipDeviceSynchronize();
    if (err != hipSuccess) {
        hipEventDestroy(start);
        hipEventDestroy(stop);
        hipFree(d_next);
        hipFree(d_out);
        return -static_cast<int>(err);
    }

    hipEventRecord(start, 0);
    hipLaunchKernelGGL(pointer_chase_kernel, dim3(1), dim3(1), 0, 0, d_next, 0u, iters, d_out);
    hipEventRecord(stop, 0);
    err = hipEventSynchronize(stop);
    if (err != hipSuccess) {
        hipEventDestroy(start);
        hipEventDestroy(stop);
        hipFree(d_next);
        hipFree(d_out);
        return -static_cast<int>(err);
    }

    float ms = 0.0f;
    hipEventElapsedTime(&ms, start, stop);
    *elapsed_ms_out = static_cast<double>(ms);

    hipEventDestroy(start);
    hipEventDestroy(stop);
    hipFree(d_next);
    hipFree(d_out);
    return 0;
}

} // extern "C"
