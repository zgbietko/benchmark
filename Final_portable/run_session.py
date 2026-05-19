from __future__ import annotations

import json
import os
import platform
import socket
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict


ROOT = Path(__file__).resolve().parent
RUNS_ROOT = ROOT / "data" / "runs"


def create_session_dir(profile: str) -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    name = f"{ts}__profile-{profile}"
    path = RUNS_ROOT / name
    path.mkdir(parents=True, exist_ok=True)
    (path / "cpu").mkdir(parents=True, exist_ok=True)
    (path / "gpu").mkdir(parents=True, exist_ok=True)
    (path / "real_kernels").mkdir(parents=True, exist_ok=True)
    latest_link = RUNS_ROOT / "latest"
    try:
        if latest_link.exists() or latest_link.is_symlink():
            latest_link.unlink()
        latest_link.symlink_to(path.name)
    except Exception:
        # fallback gdy symlink nie działa
        try:
            (RUNS_ROOT / "latest.txt").write_text(path.name + "\n", encoding="utf-8")
        except Exception:
            pass
    return path


def manifest_base(profile: str) -> Dict[str, Any]:
    return {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "profile": profile,
        "hostname": socket.gethostname(),
        "system": platform.system(),
        "arch": platform.machine(),
        "python_version": platform.python_version(),
        "user": os.environ.get("USER") or os.environ.get("LOGNAME") or "unknown",
    }


def write_manifest(session_dir: Path, data: Dict[str, Any]) -> Path:
    out = session_dir / "manifest.json"
    out.write_text(json.dumps(data, indent=2, ensure_ascii=True), encoding="utf-8")
    return out
