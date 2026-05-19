#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from device_catalog import discover_backends


DEFAULT_BACKENDS = ["cpu", "cuda", "hip", "opencl", "intel", "amd", "metal"]


def _parse_backends(raw: str) -> list[str]:
    txt = str(raw).strip().lower()
    if txt in ("", "auto", "all"):
        return list(DEFAULT_BACKENDS)
    out: list[str] = []
    for chunk in txt.split(","):
        backend = chunk.strip().lower()
        if not backend:
            continue
        if backend not in DEFAULT_BACKENDS:
            raise ValueError(f"Unsupported backend: {backend}")
        if backend not in out:
            out.append(backend)
    return out or list(DEFAULT_BACKENDS)


def main() -> None:
    ap = argparse.ArgumentParser(description="List available devices across benchmark backends.")
    ap.add_argument("--backends", default="auto", help="auto|all or CSV from: cpu,cuda,hip,opencl,intel,amd,metal")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    backends = _parse_backends(args.backends)
    discovery = discover_backends(backends)
    payload = {
        "requested_backends": backends,
        "backends": [item.to_dict() for item in discovery],
        "devices": [dev for item in discovery for dev in item.to_dict().get("devices", [])],
    }
    if args.json:
        print(json.dumps(payload, indent=2, ensure_ascii=True))
        return

    print("=== AVAILABLE DEVICES ===")
    for item in discovery:
        print(f"[{item.backend}]")
        if not item.devices:
            print(f"  none ({item.error or 'no_devices_found'})")
            continue
        for dev in item.devices:
            print(f"  - {dev.label}")


if __name__ == "__main__":
    main()
