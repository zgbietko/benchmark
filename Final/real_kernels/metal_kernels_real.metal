#include <metal_stdlib>
using namespace metal;

kernel void real_gemm_kernel(
    device const float* a [[buffer(0)]],
    device const float* b [[buffer(1)]],
    device float* c [[buffer(2)]],
    constant uint& m [[buffer(3)]],
    constant uint& n [[buffer(4)]],
    constant uint& k [[buffer(5)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint col = gid.x;
    uint row = gid.y;
    if (row >= m || col >= n) {
        return;
    }

    float acc = 0.0f;
    uint a_base = row * k;
    for (uint i = 0; i < k; ++i) {
        acc += a[a_base + i] * b[i * n + col];
    }
    c[row * n + col] = acc;
}

kernel void real_reduction_kernel(
    device const float* x [[buffer(0)]],
    device float* partial [[buffer(1)]],
    constant uint& n [[buffer(2)]],
    constant uint& total_threads [[buffer(3)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= total_threads) {
        return;
    }
    uint chunk = (n + total_threads - 1) / total_threads;
    uint start = gid * chunk;
    uint end = min(start + chunk, n);

    float s = 0.0f;
    for (uint i = start; i < end; ++i) {
        s += x[i];
    }
    partial[gid] = s;
}

kernel void real_saxpy_kernel(
    device const float* x [[buffer(0)]],
    device float* y [[buffer(1)]],
    constant float& a [[buffer(2)]],
    constant uint& n [[buffer(3)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= n) {
        return;
    }
    y[gid] = a * x[gid] + y[gid];
}

kernel void real_stencil2d_kernel(
    device const float* src [[buffer(0)]],
    device float* dst [[buffer(1)]],
    constant uint& h [[buffer(2)]],
    constant uint& w [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint x = gid.x;
    uint y = gid.y;
    if (x >= w || y >= h) {
        return;
    }
    uint idx = y * w + x;
    if (x == 0 || y == 0 || x + 1 >= w || y + 1 >= h) {
        dst[idx] = src[idx];
        return;
    }
    float c = src[idx];
    float n = src[(y - 1) * w + x];
    float s = src[(y + 1) * w + x];
    float l = src[y * w + (x - 1)];
    float r = src[y * w + (x + 1)];
    dst[idx] = 0.5f * c + 0.125f * (n + s + l + r);
}

kernel void real_spmv_csr_kernel(
    device const uint* row_ptr [[buffer(0)]],
    device const uint* col_idx [[buffer(1)]],
    device const float* vals [[buffer(2)]],
    device const float* x [[buffer(3)]],
    device float* y [[buffer(4)]],
    constant uint& n [[buffer(5)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= n) {
        return;
    }
    uint start = row_ptr[gid];
    uint end = row_ptr[gid + 1];
    float s = 0.0f;
    for (uint j = start; j < end; ++j) {
        s += vals[j] * x[col_idx[j]];
    }
    y[gid] = s;
}

kernel void real_stencil3d_kernel(
    device const float* src [[buffer(0)]],
    device float* dst [[buffer(1)]],
    constant uint& d [[buffer(2)]],
    constant uint& h [[buffer(3)]],
    constant uint& w [[buffer(4)]],
    uint3 gid [[thread_position_in_grid]]
) {
    uint x = gid.x;
    uint y = gid.y;
    uint z = gid.z;
    if (x >= w || y >= h || z >= d) {
        return;
    }
    uint idx = z * h * w + y * w + x;
    if (x == 0 || y == 0 || z == 0 || x + 1 >= w || y + 1 >= h || z + 1 >= d) {
        dst[idx] = src[idx];
        return;
    }
    float c  = src[idx];
    float xm = src[idx - 1];
    float xp = src[idx + 1];
    float ym = src[idx - w];
    float yp = src[idx + w];
    uint plane = h * w;
    float zm = src[idx - plane];
    float zp = src[idx + plane];
    dst[idx] = 0.5f * c + (1.0f / 12.0f) * (xm + xp + ym + yp + zm + zp);
}

kernel void real_fem_element_kernel(
    device const float* jac [[buffer(0)]],      // [n_elem, 9]
    device const float* coeff [[buffer(1)]],    // [n_qp, 9]
    device float* out [[buffer(2)]],            // [n_elem]
    constant uint& n_elem [[buffer(3)]],
    constant uint& n_qp [[buffer(4)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= n_elem) {
        return;
    }
    float acc = 0.0f;
    uint jbase = gid * 9;
    for (uint q = 0; q < n_qp; ++q) {
        uint cbase = q * 9;
        for (uint i = 0; i < 9; ++i) {
            acc += jac[jbase + i] * coeff[cbase + i];
        }
    }
    out[gid] = acc;
}
