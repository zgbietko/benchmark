// cpu/lib/microbench.h
#ifndef MICROBENCH_H
#define MICROBENCH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------
// Memory bandwidth
// --------------------

// dst[i] = src[i]
void mem_copy_kernel(float *dst, const float *src, size_t n);

// Multi-threaded copy (pthreads)
void mem_copy_kernel_mt(float *dst, const float *src, size_t n, int num_threads);

// --------------------
// Pointer-chasing latency
// --------------------
// buf: permutation/cycle of indices [0..n-1], iters: number of dependent steps
void pointer_chase_kernel(uint32_t *buf, size_t n, size_t iters);

// --------------------
// STREAM-like kernels (bandwidth-oriented)
// --------------------
// All kernels operate on FP32 arrays.
// NOTE: For STREAM bandwidth, use bytes-per-iteration:
//  - copy  : 2 * n * sizeof(float)
//  - scale : 2 * n * sizeof(float)
//  - add   : 3 * n * sizeof(float)
//  - triad : 3 * n * sizeof(float)

// a[i] = b[i]
void stream_copy_kernel(float *a, const float *b, size_t n);

// a[i] = scalar * b[i]
void stream_scale_kernel(float *a, const float *b, float scalar, size_t n);

// a[i] = b[i] + c[i]
void stream_add_kernel(float *a, const float *b, const float *c, size_t n);

// a[i] = b[i] + scalar * c[i]
void stream_triad_kernel(float *a, const float *b, const float *c, float scalar, size_t n);

// Multi-threaded variants (pthreads)
void stream_copy_kernel_mt(float *a, const float *b, size_t n, int num_threads);
void stream_scale_kernel_mt(float *a, const float *b, float scalar, size_t n, int num_threads);
void stream_add_kernel_mt(float *a, const float *b, const float *c, size_t n, int num_threads);
void stream_triad_kernel_mt(float *a, const float *b, const float *c, float scalar, size_t n, int num_threads);

// --------------------
// Compute: FMA
// --------------------
// a[i] = a[i] * b[i] + c[i], repeated 'iters' times (single-thread)
void fma_kernel(float *a, const float *b, const float *c, size_t n, size_t iters);

// Peak FMA: maximum compute pressure (multi-thread, tiny in-cache data)
void fma_peak_mt(size_t iters, int num_threads);

#ifdef __cplusplus
}
#endif

#endif // MICROBENCH_H
