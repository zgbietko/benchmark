from __future__ import annotations

from contextlib import nullcontext
from dataclasses import dataclass
import json
from pathlib import Path
import re
import time
from typing import Any

import numpy as np

try:
    import Metal  # type: ignore
except Exception:  # pragma: no cover
    Metal = None  # type: ignore

try:
    import objc  # type: ignore
except Exception:  # pragma: no cover
    objc = None  # type: ignore


@dataclass(frozen=True)
class ReplayDump:
    dump_dir: Path
    metadata: dict[str, Any]
    execution_parameters: np.ndarray
    gauss_dat: np.ndarray
    shape_fun_ref: np.ndarray
    el_data_in: np.ndarray
    el_data_out_expected: np.ndarray | None


def resolve_dump_root(path: Path) -> Path:
    candidate = path.expanduser().resolve()
    launch_dumps = candidate / "launch_dumps"
    return launch_dumps if launch_dumps.exists() else candidate


def option_dump_dir(root: Path, case_name: str, variant: str, option_index: int) -> Path:
    return resolve_dump_root(root) / case_name / variant / f"opt_{int(option_index):03d}"


def load_replay_dump(dump_dir: Path) -> ReplayDump:
    dump_dir = dump_dir.expanduser().resolve()
    meta = json.loads((dump_dir / "launch_meta.json").read_text(encoding="utf-8"))
    execution_parameters = np.frombuffer((dump_dir / "execution_parameters.bin").read_bytes(), dtype=np.int32).copy()
    gauss_dat = np.frombuffer((dump_dir / "gauss_dat.bin").read_bytes(), dtype=np.float32).copy()
    shape_fun_ref = np.frombuffer((dump_dir / "shape_fun_ref.bin").read_bytes(), dtype=np.float32).copy()
    el_data_in = np.frombuffer((dump_dir / "el_data_in.bin").read_bytes(), dtype=np.float32).copy()
    out_path = dump_dir / "el_data_out.bin"
    el_data_out_expected = np.frombuffer(out_path.read_bytes(), dtype=np.float32).copy() if out_path.exists() else None
    return ReplayDump(
        dump_dir=dump_dir,
        metadata=meta,
        execution_parameters=execution_parameters,
        gauss_dat=gauss_dat,
        shape_fun_ref=shape_fun_ref,
        el_data_in=el_data_in,
        el_data_out_expected=el_data_out_expected,
    )


class FilipMetalReplayRunner:
    def __init__(self, device_index: int = 0) -> None:
        if Metal is None:
            raise RuntimeError("PyObjC Metal module is not available.")
        self.device = self._select_device(device_index)
        self.device_index = int(device_index)
        self.device_name = str(self.device.name())
        self.command_queue = self.device.newCommandQueue()
        self._pipelines: dict[tuple[str, str, tuple[int, ...], int], Any] = {}

    @staticmethod
    def _list_devices() -> list[Any]:
        if hasattr(Metal, "MTLCopyAllDevices"):
            arr = Metal.MTLCopyAllDevices()
            if arr is not None:
                return list(arr)
        dev = Metal.MTLCreateSystemDefaultDevice()
        return [dev] if dev is not None else []

    @classmethod
    def _select_device(cls, index: int) -> Any:
        devices = cls._list_devices()
        if not devices:
            raise RuntimeError("No Metal-compatible device found.")
        if index < 0 or index >= len(devices):
            index = 0
        return devices[index]

    @staticmethod
    def _make_size(x: int, y: int = 1, z: int = 1):
        if hasattr(Metal, "MTLSizeMake"):
            return Metal.MTLSizeMake(int(x), int(y), int(z))
        return Metal.MTLSize(int(x), int(y), int(z))

    @staticmethod
    def _resource_options() -> Any:
        return Metal.MTLResourceOptions(Metal.MTLResourceStorageModeShared)

    def _make_buffer(self, num_bytes: int) -> Any:
        return self.device.newBufferWithLength_options_(int(max(4, num_bytes)), self._resource_options())

    def _buffer_from_numpy(self, arr: np.ndarray) -> Any:
        arr_c = np.ascontiguousarray(arr)
        num_bytes = int(arr_c.nbytes)
        if num_bytes <= 0:
            return self._make_buffer(4)
        return self.device.newBufferWithBytes_length_options_(arr_c, num_bytes, self._resource_options())

    @staticmethod
    def _autorelease_pool():
        if objc is not None and hasattr(objc, "autorelease_pool"):
            return objc.autorelease_pool()
        return nullcontext()

    @staticmethod
    def _problem_define(problem_macro: str) -> str:
        token = str(problem_macro or "").strip().upper()
        if token == "LAPLACE":
            return "LAPLACE"
        if token in {"TEST_SCALAR", "TEST_NUMINT"}:
            return "TEST_NUMINT"
        raise RuntimeError(f"Unsupported Filip replay problem macro: {problem_macro}")

    @classmethod
    def _translate_opencl_to_metal(
        cls,
        *,
        opencl_source: str,
        problem_macro: str,
        option_row: list[int],
        work_group_size: int,
    ) -> str:
        body = opencl_source
        body = re.sub(
            r"kernel void\s+tmr_ocl_num_int_el\s*\((.*?)\)\s*\{",
            (
                "kernel void tmr_ocl_num_int_el(\n"
                "  constant int* execution_parameters [[buffer(0)]],\n"
                "  constant float* gauss_dat [[buffer(1)]],\n"
                "  constant float* shpfun_ref [[buffer(2)]],\n"
                "  device float* el_data_in [[buffer(3)]],\n"
                "  device float* stiff_mat_out [[buffer(4)]],\n"
                "  uint thread_id [[thread_index_in_threadgroup]],\n"
                "  uint3 tg_pos [[threadgroup_position_in_grid]],\n"
                "  uint3 tg_count [[threadgroups_per_grid]]\n"
                "){\n"
                "  const int group_id = (int)tg_pos.x;\n"
                "  const int nr_work_groups = (int)tg_count.x;"
            ),
            body,
            count=1,
            flags=re.S,
        )
        body = body.replace("__constant ", "constant ")
        body = body.replace("__global ", "device ")
        body = body.replace("__local ", "threadgroup ")
        body = body.replace("barrier(CLK_LOCAL_MEM_FENCE)", "threadgroup_barrier(mem_flags::mem_threadgroup)")
        body = body.replace("const int group_id = get_group_id(0);", "")
        body = body.replace("const int thread_id = get_local_id(0);", "")
        body = body.replace("const int nr_work_groups = get_num_groups(0);", "")

        defines = [
            "#include <metal_stdlib>",
            "using namespace metal;",
            f"#define WORK_GROUP_SIZE {int(work_group_size)}",
            f"#define WORKSPACE_PADDING {1 if int(option_row[8]) else 0}",
            f"#define {cls._problem_define(problem_macro)}",
        ]
        if int(option_row[0]):
            defines.append("#define COAL_READ")
        if int(option_row[1]):
            defines.append("#define COAL_WRITE")
        if int(option_row[2]):
            defines.append("#define COMPUTE_ALL_SHAPE_FUN_DER")
        if int(option_row[3]):
            defines.append("#define USE_WORKSPACE_FOR_PDE_COEFF")
        if int(option_row[4]):
            defines.append("#define USE_WORKSPACE_FOR_GEO_DATA")
        if int(option_row[5]):
            defines.append("#define USE_WORKSPACE_FOR_SHAPE_FUN")
        if int(option_row[6]):
            defines.append("#define USE_WORKSPACE_FOR_STIFF_MAT")

        return "\n".join(defines) + "\n\n" + body

    def _pipeline(
        self,
        *,
        kernel_path: Path,
        problem_macro: str,
        variant: str,
        option_row: list[int],
        work_group_size: int,
        debug_source_path: Path | None,
    ) -> Any:
        key = (
            str(problem_macro),
            str(variant).lower(),
            tuple(int(v) for v in option_row),
            int(work_group_size),
        )
        if key in self._pipelines:
            return self._pipelines[key]

        translated = self._translate_opencl_to_metal(
            opencl_source=kernel_path.read_text(encoding="utf-8", errors="replace"),
            problem_macro=problem_macro,
            option_row=option_row,
            work_group_size=work_group_size,
        )
        if debug_source_path is not None:
            debug_source_path.parent.mkdir(parents=True, exist_ok=True)
            debug_source_path.write_text(translated, encoding="utf-8")
        library, err = self.device.newLibraryWithSource_options_error_(translated, None, None)
        if err is not None or library is None:
            raise RuntimeError(
                f"Metal compilation failed for {kernel_path.name} ({problem_macro}, {variant}): {err}. "
                f"Translated source: {debug_source_path if debug_source_path is not None else 'not written'}"
            )
        fn = library.newFunctionWithName_("tmr_ocl_num_int_el")
        if fn is None:
            raise RuntimeError("Translated Metal library does not expose tmr_ocl_num_int_el.")
        pipeline, err = self.device.newComputePipelineStateWithFunction_error_(fn, None)
        if err is not None:
            raise RuntimeError(f"Failed to create Metal pipeline for replay kernel: {err}")
        self._pipelines[key] = pipeline
        return pipeline

    def replay(
        self,
        *,
        dump: ReplayDump,
        kernel_path: Path,
        variant: str,
        option_row: list[int],
        repeats: int = 1,
        debug_source_path: Path | None = None,
        verify_tol: float = 1e-5,
    ) -> dict[str, Any]:
        meta = dump.metadata
        work_group_size = int(meta["work_group_size"])
        nr_work_groups = int(meta["nr_work_groups"])
        pipeline = self._pipeline(
            kernel_path=kernel_path,
            problem_macro=str(meta.get("problem_macro", "")),
            variant=variant,
            option_row=option_row,
            work_group_size=work_group_size,
            debug_source_path=debug_source_path,
        )

        input_t0 = time.perf_counter()
        with self._autorelease_pool():
            exec_buf = self._buffer_from_numpy(dump.execution_parameters.astype(np.int32, copy=False))
            gauss_buf = self._buffer_from_numpy(dump.gauss_dat.astype(np.float32, copy=False))
            shape_buf = self._buffer_from_numpy(dump.shape_fun_ref.astype(np.float32, copy=False))
            in_buf = self._buffer_from_numpy(dump.el_data_in.astype(np.float32, copy=False))
            out_bytes = int(max(4, int(meta.get("el_data_out_bytes", 0))))
            out_buf = self._make_buffer(out_bytes)
            np.frombuffer(out_buf.contents().as_buffer(out_bytes), dtype=np.uint8)[:] = 0
        input_time_s = time.perf_counter() - input_t0

        threadgroups = self._make_size(nr_work_groups, 1, 1)
        threads_per_group = self._make_size(work_group_size, 1, 1)

        wall_times: list[float] = []
        internal_times: list[float] = []
        repeats = max(1, int(repeats))
        for _ in range(repeats):
            with self._autorelease_pool():
                np.frombuffer(out_buf.contents().as_buffer(out_bytes), dtype=np.uint8)[:] = 0
                command_buffer = self.command_queue.commandBuffer()
                encoder = command_buffer.computeCommandEncoder()
                encoder.setComputePipelineState_(pipeline)
                encoder.setBuffer_offset_atIndex_(exec_buf, 0, 0)
                encoder.setBuffer_offset_atIndex_(gauss_buf, 0, 1)
                encoder.setBuffer_offset_atIndex_(shape_buf, 0, 2)
                encoder.setBuffer_offset_atIndex_(in_buf, 0, 3)
                encoder.setBuffer_offset_atIndex_(out_buf, 0, 4)
                encoder.dispatchThreadgroups_threadsPerThreadgroup_(threadgroups, threads_per_group)
                encoder.endEncoding()

                t0 = time.perf_counter()
                command_buffer.commit()
                command_buffer.waitUntilCompleted()
                t1 = time.perf_counter()
                wall_times.append(t1 - t0)

                internal = float("nan")
                try:
                    kernel_start = float(command_buffer.kernelStartTime())
                    kernel_end = float(command_buffer.kernelEndTime())
                    if kernel_end > kernel_start > 0.0:
                        internal = kernel_end - kernel_start
                except Exception:
                    pass
                if not np.isfinite(internal):
                    try:
                        gpu_start = float(command_buffer.GPUStartTime())
                        gpu_end = float(command_buffer.GPUEndTime())
                        if gpu_end > gpu_start > 0.0:
                            internal = gpu_end - gpu_start
                    except Exception:
                        pass
                if not np.isfinite(internal):
                    internal = wall_times[-1]
                internal_times.append(internal)

        output_t0 = time.perf_counter()
        output = np.frombuffer(out_buf.contents().as_buffer(out_bytes), dtype=np.float32).copy()
        output_time_s = time.perf_counter() - output_t0

        validation: dict[str, Any] = {"expected_output_present": bool(dump.el_data_out_expected is not None)}
        if dump.el_data_out_expected is not None:
            if output.shape != dump.el_data_out_expected.shape:
                raise RuntimeError(
                    f"Replay output shape mismatch for {dump.dump_dir}: got {output.shape}, "
                    f"expected {dump.el_data_out_expected.shape}"
                )
            diff = np.abs(output - dump.el_data_out_expected)
            max_abs_diff = float(diff.max(initial=0.0))
            rms_diff = float(np.sqrt(np.mean(np.square(diff)))) if diff.size else 0.0
            validation.update(
                {
                    "max_abs_diff": max_abs_diff,
                    "rms_diff": rms_diff,
                    "within_tolerance": bool(max_abs_diff <= float(verify_tol)),
                    "verify_tol": float(verify_tol),
                }
            )
        else:
            validation.update({"max_abs_diff": None, "rms_diff": None, "within_tolerance": None, "verify_tol": float(verify_tol)})

        el_data_in_bytes = int(meta.get("el_data_in_bytes", int(dump.el_data_in.nbytes)))
        el_data_out_bytes = int(meta.get("el_data_out_bytes", int(output.nbytes)))
        return {
            "device": self.device_name,
            "input_time_s": float(input_time_s),
            "input_bw_gbps": (el_data_in_bytes / max(input_time_s, 1e-12) / 1e9) if input_time_s > 0.0 else float("inf"),
            "kernel_time_s": float(np.mean(wall_times)),
            "internal_time_s": float(np.mean(internal_times)),
            "output_time_s": float(output_time_s),
            "output_bw_gbps": (el_data_out_bytes / max(output_time_s, 1e-12) / 1e9) if output_time_s > 0.0 else float("inf"),
            "validation": validation,
            "translated_source_path": str(debug_source_path) if debug_source_path is not None else "",
            "output": output,
        }
