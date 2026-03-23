from __future__ import annotations

from dataclasses import dataclass, asdict
from typing import Any, Iterable, List


@dataclass
class DeviceDescriptor:
    backend: str
    device_index: int
    device_name: str
    label: str
    vendor: str = ""
    platform_name: str = ""
    device_type: str = ""
    details: dict[str, Any] | None = None

    def to_dict(self) -> dict[str, Any]:
        payload = asdict(self)
        if self.details is None:
            payload["details"] = {}
        return payload


@dataclass
class DiscoveryResult:
    backend: str
    devices: list[DeviceDescriptor]
    available: bool
    error: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "backend": self.backend,
            "available": bool(self.available),
            "error": str(self.error),
            "devices": [item.to_dict() for item in self.devices],
        }


def _cpu_devices() -> list[DeviceDescriptor]:
    return [
        DeviceDescriptor(
            backend="cpu",
            device_index=0,
            device_name="cpu",
            label="cpu | dev0 | Host CPU",
            vendor="host",
            device_type="cpu",
        )
    ]


def _cuda_devices() -> list[DeviceDescriptor]:
    from gpu.cuda.cuda_backend import get_device_count, init_cuda, get_device_info

    out: list[DeviceDescriptor] = []
    count = int(get_device_count())
    for idx in range(count):
        ctx = init_cuda(idx)
        info = get_device_info(ctx)
        out.append(
            DeviceDescriptor(
                backend="cuda",
                device_index=idx,
                device_name=info.name,
                label=(
                    f"cuda | dev{idx} | {info.name} | "
                    f"CC {info.compute_capability} | {info.global_mem_gb:.1f} GB"
                ),
                vendor="nvidia",
                device_type="gpu",
                details={
                    "compute_capability": info.compute_capability,
                    "global_mem_gb": round(info.global_mem_gb, 3),
                },
            )
        )
    return out


def _hip_devices() -> list[DeviceDescriptor]:
    from gpu.hip.hip_backend import get_device_count, init_hip, get_device_info

    out: list[DeviceDescriptor] = []
    count = int(get_device_count())
    for idx in range(count):
        ctx = init_hip(idx)
        info = get_device_info(ctx)
        out.append(
            DeviceDescriptor(
                backend="hip",
                device_index=idx,
                device_name=info.name,
                label=(
                    f"hip | dev{idx} | {info.name} | "
                    f"arch {info.arch} | {info.global_mem_gb:.1f} GB"
                ),
                vendor="amd",
                device_type="gpu",
                details={
                    "arch": info.arch,
                    "global_mem_gb": round(info.global_mem_gb, 3),
                },
            )
        )
    return out


def _opencl_devices() -> list[DeviceDescriptor]:
    from gpu.opencl.opencl_backend import list_device_infos

    out: list[DeviceDescriptor] = []
    for info in list_device_infos():
        vendor = str(info.device_vendor or info.platform_vendor or "").strip()
        out.append(
            DeviceDescriptor(
                backend="opencl",
                device_index=int(info.index),
                device_name=str(info.device_name),
                label=(
                    f"opencl | dev{int(info.index)} | {info.device_name} | "
                    f"{vendor or 'unknown vendor'} | {info.platform_name}"
                ),
                vendor=vendor,
                platform_name=str(info.platform_name),
                device_type=str(info.device_type),
                details={
                    "platform_vendor": str(info.platform_vendor),
                },
            )
        )
    return out


def _normalize_vendor_key(value: str) -> str:
    txt = str(value or "").strip().lower()
    if txt in ("intel", "intel_gpu", "intel_igpu", "intel_arc"):
        return "intel"
    if txt in ("amd", "rocm", "ati"):
        return "amd"
    if txt in ("nvidia", "cuda", "geforce"):
        return "nvidia"
    return txt


def _matches_vendor(item: DeviceDescriptor, vendor_key: str) -> bool:
    key = _normalize_vendor_key(vendor_key)
    hay = " ".join(
        [
            str(item.vendor or ""),
            str(item.platform_name or ""),
            str(item.device_name or ""),
            str((item.details or {}).get("platform_vendor", "") or ""),
        ]
    ).lower()
    if key == "intel":
        return "intel" in hay
    if key == "amd":
        return "amd" in hay or "advanced micro devices" in hay or "radeon" in hay
    if key == "nvidia":
        return "nvidia" in hay or "geforce" in hay or "quadro" in hay or "tesla" in hay
    return key in hay


def _opencl_vendor_devices(alias_backend: str, vendor_key: str) -> list[DeviceDescriptor]:
    out: list[DeviceDescriptor] = []
    for item in _opencl_devices():
        if not _matches_vendor(item, vendor_key):
            continue
        out.append(
            DeviceDescriptor(
                backend=alias_backend,
                device_index=int(item.device_index),
                device_name=item.device_name,
                label=(
                    f"{alias_backend} | dev{int(item.device_index)} | {item.device_name} | "
                    f"{item.vendor or 'unknown vendor'} | {item.platform_name}"
                ),
                vendor=item.vendor,
                platform_name=item.platform_name,
                device_type=item.device_type,
                details=dict(item.details or {}),
            )
        )
    return out


def _metal_devices() -> list[DeviceDescriptor]:
    from gpu.metal.metal_backend import MetalBackend

    devices = MetalBackend._list_devices()
    out: list[DeviceDescriptor] = []
    for idx, device in enumerate(devices):
        name = str(device.name())
        out.append(
            DeviceDescriptor(
                backend="metal",
                device_index=idx,
                device_name=name,
                label=f"metal | dev{idx} | {name}",
                vendor="apple",
                device_type="gpu",
            )
        )
    return out


def list_devices_for_backend(backend: str) -> list[DeviceDescriptor]:
    b = str(backend).strip().lower()
    if b == "cpu":
        return _cpu_devices()
    if b == "cuda":
        return _cuda_devices()
    if b == "hip":
        return _hip_devices()
    if b == "opencl":
        return _opencl_devices()
    if b == "intel":
        return _opencl_vendor_devices("intel", "intel")
    if b == "amd":
        return _opencl_vendor_devices("amd", "amd")
    if b == "metal":
        return _metal_devices()
    raise ValueError(f"Unsupported backend for discovery: {backend}")


def list_devices(backends: Iterable[str]) -> list[DeviceDescriptor]:
    out: list[DeviceDescriptor] = []
    seen: set[tuple[str, int]] = set()
    for backend in backends:
        try:
            items = list_devices_for_backend(backend)
        except Exception:
            continue
        for item in items:
            key = (item.backend, int(item.device_index))
            if key in seen:
                continue
            seen.add(key)
            out.append(item)
    return out


def list_devices_dicts(backends: Iterable[str]) -> List[dict[str, Any]]:
    return [item.to_dict() for item in list_devices(backends)]


def discover_backend(backend: str) -> DiscoveryResult:
    try:
        devices = list_devices_for_backend(backend)
        return DiscoveryResult(
            backend=str(backend).strip().lower(),
            devices=devices,
            available=bool(devices),
            error="" if devices else "no_devices_found",
        )
    except Exception as exc:
        return DiscoveryResult(
            backend=str(backend).strip().lower(),
            devices=[],
            available=False,
            error=f"{type(exc).__name__}: {exc}",
        )


def discover_backends(backends: Iterable[str]) -> list[DiscoveryResult]:
    return [discover_backend(backend) for backend in backends]
