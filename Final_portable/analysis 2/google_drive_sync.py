from __future__ import annotations

import json
import os
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


DEFAULT_SUBDIR = "benchmark_results"


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _env_default(name: str, fallback: str = "") -> str:
    return str(os.environ.get(name, fallback) or fallback).strip()


def _final_env_default(final_name: str, legacy_name: str, fallback: str = "") -> str:
    value = _env_default(final_name, "")
    if value:
        return value
    return _env_default(legacy_name, fallback)


def default_sync_mode() -> str:
    return _final_env_default("FINAL_GOOGLE_DRIVE_SYNC", "V5_GOOGLE_DRIVE_SYNC", "off").lower()


def default_google_drive_dir() -> str:
    return _final_env_default("FINAL_GOOGLE_DRIVE_DIR", "V5_GOOGLE_DRIVE_DIR", "")


def default_rclone_remote() -> str:
    return _final_env_default("FINAL_GOOGLE_DRIVE_RCLONE_REMOTE", "V5_GOOGLE_DRIVE_RCLONE_REMOTE", "")


def default_google_drive_subdir() -> str:
    return _final_env_default("FINAL_GOOGLE_DRIVE_SUBDIR", "V5_GOOGLE_DRIVE_SUBDIR", DEFAULT_SUBDIR)


def _candidate_google_drive_dirs() -> list[Path]:
    home = Path.home()
    candidates: list[Path] = []

    explicit = default_google_drive_dir()
    if explicit:
        candidates.append(Path(explicit).expanduser())

    cloud_storage = home / "Library" / "CloudStorage"
    if cloud_storage.exists():
        for item in sorted(cloud_storage.iterdir()):
            name = item.name.lower()
            if item.is_dir() and "google" in name and "drive" in name:
                candidates.append(item)

    for raw in (
        home / "Google Drive",
        home / "GoogleDrive",
        home / "My Drive",
        home / "GoogleDriveSync",
        home / "Documents" / "Google Drive",
    ):
        candidates.append(raw)

    seen: set[str] = set()
    out: list[Path] = []
    for candidate in candidates:
        key = str(candidate.expanduser())
        if key in seen:
            continue
        seen.add(key)
        if candidate.expanduser().exists():
            out.append(candidate.expanduser().resolve())
    return out


def detect_google_drive_dir() -> Path | None:
    candidates = _candidate_google_drive_dirs()
    return candidates[0] if candidates else None


def _relative_sync_path(source_dir: Path, *, root: Path | None) -> Path:
    source_dir = source_dir.resolve()
    if root is not None:
        try:
            return source_dir.relative_to(root.resolve())
        except Exception:
            pass
    if "data" in source_dir.parts:
        idx = source_dir.parts.index("data")
        return Path(*source_dir.parts[idx:])
    return Path(source_dir.name)


def _safe_json_update(path: Path, field: str, payload: dict[str, Any]) -> None:
    if not path.exists():
        return
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if isinstance(data, dict):
            data[field] = payload
            path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
    except Exception:
        return


def _write_sync_record(source_dir: Path, payload: dict[str, Any]) -> None:
    record_path = source_dir / "google_drive_sync.json"
    record_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    _safe_json_update(source_dir / "run_manifest.json", "google_drive_sync", payload)
    _safe_json_update(source_dir / "summary.json", "google_drive_sync", payload)


def _folder_copy(source_dir: Path, dest_dir: Path) -> tuple[bool, str]:
    dest_dir.parent.mkdir(parents=True, exist_ok=True)
    rsync = shutil.which("rsync")
    if rsync:
        cmd = [rsync, "-a", f"{source_dir}/", f"{dest_dir}/"]
        rc = subprocess.run(cmd, check=False).returncode
        return rc == 0, " ".join(cmd)
    shutil.copytree(source_dir, dest_dir, dirs_exist_ok=True)
    return True, "shutil.copytree"


def _rclone_copy(source_dir: Path, remote_target: str) -> tuple[bool, str]:
    rclone = shutil.which("rclone")
    if not rclone:
        return False, "rclone missing"
    cmd = [rclone, "copy", str(source_dir), remote_target, "--create-empty-src-dirs"]
    rc = subprocess.run(cmd, check=False).returncode
    return rc == 0, " ".join(cmd)


def sync_artifacts_to_google_drive(
    *,
    source_dir: Path,
    mode: str = "off",
    google_drive_dir: str = "",
    rclone_remote: str = "",
    subdir: str = DEFAULT_SUBDIR,
    root: Path | None = None,
) -> dict[str, Any]:
    source_dir = Path(source_dir).expanduser().resolve()
    requested_mode = str(mode or "off").strip().lower()
    subdir = str(subdir or DEFAULT_SUBDIR).strip().strip("/").strip()
    if not subdir:
        subdir = DEFAULT_SUBDIR

    payload: dict[str, Any] = {
        "provider": "google_drive",
        "requested_mode": requested_mode,
        "timestamp_utc": _now_iso(),
        "source_dir": str(source_dir),
        "subdir": subdir,
        "status": "disabled",
    }

    if requested_mode in {"", "off", "disabled", "none"}:
        return payload

    relative_path = _relative_sync_path(source_dir, root=root)
    payload["relative_path"] = relative_path.as_posix()

    resolved_mode = requested_mode
    if requested_mode == "auto":
        if google_drive_dir or detect_google_drive_dir() is not None:
            resolved_mode = "folder"
        elif rclone_remote or default_rclone_remote():
            resolved_mode = "rclone"
        else:
            payload["status"] = "skipped"
            payload["reason"] = "Nie znaleziono lokalnego katalogu Google Drive ani konfiguracji rclone."
            _write_sync_record(source_dir, payload)
            return payload

    payload["resolved_mode"] = resolved_mode

    if resolved_mode == "folder":
        explicit_dir = str(google_drive_dir).strip()
        if explicit_dir:
            base_dir = Path(explicit_dir).expanduser().resolve()
            base_dir.mkdir(parents=True, exist_ok=True)
        else:
            base_dir = detect_google_drive_dir()
        if base_dir is None or not base_dir.exists():
            payload["status"] = "failed"
            payload["reason"] = "Brak lokalnego katalogu Google Drive."
            _write_sync_record(source_dir, payload)
            return payload
        dest_dir = (base_dir / subdir / relative_path).resolve()
        ok, command = _folder_copy(source_dir, dest_dir)
        payload.update(
            {
                "status": "ok" if ok else "failed",
                "destination": str(dest_dir),
                "command": command,
            }
        )
        _write_sync_record(source_dir, payload)
        return payload

    if resolved_mode == "rclone":
        remote = str(rclone_remote or default_rclone_remote()).strip()
        if not remote:
            payload["status"] = "failed"
            payload["reason"] = "Brak skonfigurowanego remote rclone."
            _write_sync_record(source_dir, payload)
            return payload
        remote_target = f"{remote.rstrip('/')}/{subdir}/{relative_path.as_posix()}"
        ok, command = _rclone_copy(source_dir, remote_target)
        payload.update(
            {
                "status": "ok" if ok else "failed",
                "destination": remote_target,
                "command": command,
            }
        )
        _write_sync_record(source_dir, payload)
        return payload

    payload["status"] = "failed"
    payload["reason"] = f"Nieobsługiwany tryb synchronizacji: {requested_mode}"
    _write_sync_record(source_dir, payload)
    return payload
