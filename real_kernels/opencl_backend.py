from __future__ import annotations

from dataclasses import dataclass

from .mapped_gpu_backend import MappedGpuFemBackend


@dataclass
class OpenCLRealBackend(MappedGpuFemBackend):
    backend: str = "opencl"
