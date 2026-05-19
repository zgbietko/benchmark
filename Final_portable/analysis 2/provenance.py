from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import socket
import subprocess
import sys
from typing import Any, Iterable


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_json(payload: Any) -> str:
    canonical = json.dumps(payload, sort_keys=True, ensure_ascii=True, separators=(",", ":")).encode("utf-8")
    return sha256_bytes(canonical)


def sha256_file(path: Path, *, chunk_size: int = 1024 * 1024) -> str:
    h = hashlib.sha256()
    with Path(path).open("rb") as handle:
        while True:
            chunk = handle.read(chunk_size)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def file_manifest(path: Path) -> dict[str, Any]:
    p = Path(path).expanduser().resolve()
    return {
        "path": str(p),
        "bytes": int(p.stat().st_size),
        "sha256": sha256_file(p),
    }


def _safe_cmd(args: list[str], cwd: Path | None = None) -> str:
    try:
        proc = subprocess.run(args, cwd=cwd, text=True, capture_output=True, check=False)
    except Exception:
        return ""
    if proc.returncode != 0:
        return ""
    return proc.stdout.strip()


def git_provenance(root: Path) -> dict[str, Any]:
    root = Path(root).expanduser().resolve()
    top = _safe_cmd(["git", "rev-parse", "--show-toplevel"], cwd=root)
    if not top:
        return {"available": False}
    top_path = Path(top)
    return {
        "available": True,
        "repo_root": str(top_path),
        "commit": _safe_cmd(["git", "rev-parse", "HEAD"], cwd=top_path),
        "short_commit": _safe_cmd(["git", "rev-parse", "--short", "HEAD"], cwd=top_path),
        "branch": _safe_cmd(["git", "branch", "--show-current"], cwd=top_path),
        "status_short": _safe_cmd(["git", "status", "--short"], cwd=top_path).splitlines(),
        "dirty": bool(_safe_cmd(["git", "status", "--short"], cwd=top_path).strip()),
    }


IMPORTANT_ENV_KEYS = (
    "PATH",
    "LD_LIBRARY_PATH",
    "DYLD_LIBRARY_PATH",
    "OCL_ICD_FILENAMES",
    "PYTHONPATH",
)


def collect_runtime_provenance(root: Path | None = None, *, extra_files: dict[str, Path] | None = None) -> dict[str, Any]:
    payload: dict[str, Any] = {
        "created_at_utc": datetime.now(timezone.utc).isoformat(),
        "hostname": socket.gethostname(),
        "platform": {
            "system": platform.system(),
            "release": platform.release(),
            "version": platform.version(),
            "machine": platform.machine(),
            "processor": platform.processor(),
            "python": sys.version,
            "python_executable": sys.executable,
        },
        "env": {key: os.environ.get(key, "") for key in IMPORTANT_ENV_KEYS if os.environ.get(key)},
    }
    if root is not None:
        payload["git"] = git_provenance(root)
    if extra_files:
        manifests: dict[str, Any] = {}
        for key, path in extra_files.items():
            p = Path(path)
            if p.exists() and p.is_file():
                manifests[str(key)] = file_manifest(p)
        payload["files"] = manifests
    payload["provenance_hash"] = sha256_json(payload)
    return payload


def attach_hashes(items: Iterable[tuple[str, Path]]) -> dict[str, Any]:
    out: dict[str, Any] = {}
    for key, path in items:
        p = Path(path)
        if p.exists() and p.is_file():
            out[str(key)] = file_manifest(p)
    return out
