from __future__ import annotations

from typing import Any


def _opencl_preferred_vendor(backend: str, platform_profile: str = "auto") -> str | None:
    b = str(backend).strip().lower()
    profile = str(platform_profile).strip().lower()

    if b == "intel" or profile in ("intel_igpu", "intel_arc"):
        return "intel"
    if b == "amd" or profile == "amd":
        return "amd"
    if profile == "nvidia":
        return "nvidia"
    return None


def resolve_device_index(
    backend: str,
    requested_index: int,
    *,
    platform_profile: str = "auto",
) -> tuple[int, str]:
    preferred_vendor = _opencl_preferred_vendor(backend, platform_profile)
    b = str(backend).strip().lower()
    if b not in ("opencl", "intel", "amd"):
        return int(requested_index), "requested_index_kept"

    try:
        from gpu.opencl.opencl_backend import resolve_device_index as _resolve_opencl_device_index

        return _resolve_opencl_device_index(
            int(requested_index),
            preferred_vendor=preferred_vendor,
            prefer_gpu=True,
        )
    except Exception as exc:
        return int(requested_index), f"opencl_resolution_unavailable:{type(exc).__name__}"


def list_opencl_devices() -> list[dict[str, Any]]:
    try:
        from gpu.opencl.opencl_backend import list_device_infos

        out: list[dict[str, Any]] = []
        for info in list_device_infos():
            out.append(
                {
                    "index": int(info.index),
                    "platform_name": str(info.platform_name),
                    "platform_vendor": str(info.platform_vendor),
                    "device_name": str(info.device_name),
                    "device_vendor": str(info.device_vendor),
                    "device_type": str(info.device_type),
                }
            )
        return out
    except Exception:
        return []
