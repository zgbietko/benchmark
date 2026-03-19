from __future__ import annotations

import platform
import re
import socket
import os
import pwd
from datetime import datetime
from pathlib import Path
from typing import Any, Dict

# ROOT katalogu projektu (tam, gdzie jest ten plik)
ROOT = Path(__file__).resolve().parent


def slugify_name(name: str) -> str:
    """Sanityzuje nazwy (GPU/CPU) do postaci przyjaznej systemowi plików."""
    if not name:
        return "unknown"
    s = name.strip().lower()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    s = re.sub(r"_+", "_", s).strip("_")
    return s or "unknown"


def make_gpu_specific_csv_path(
    benchmark_name: str,
    data_dir: Path,
    gpu_backend: str,
    gpu_name: str,
    device_id: int,
) -> Path:
    """
    Buduje ścieżkę wyników GPU w schemacie bezkolizyjnym:
      data/gpu/{benchmark}__backend-{backend}__gpu-{gpu_slug}__dev{device}.csv
    """
    gpu_slug = slugify_name(gpu_name)
    fname = f"{benchmark_name}__backend-{gpu_backend}__gpu-{gpu_slug}__dev{device_id}.csv"
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        gpu_dir = Path(run_root) / "gpu"
        gpu_dir.mkdir(parents=True, exist_ok=True)
        path = gpu_dir / fname
    else:
        path = data_dir / fname
    try:
        ensure_writable_path(path)
        return path
    except PermissionError:
        # Nie przerywaj benchmarku, jeśli główny plik jest np. root-owned
        # po wcześniejszym uruchomieniu z sudo. Tworzymy wariant zapisywalny.
        fallback = _make_fallback_csv_path(
            data_dir=data_dir,
            benchmark_name=benchmark_name,
            gpu_backend=gpu_backend,
            gpu_slug=gpu_slug,
            device_id=device_id,
        )
        ensure_writable_path(fallback)
        print(f"[WARN] Brak zapisu do: {path}")
        print(f"[WARN] Używam fallback CSV: {fallback}")
        return fallback


def _make_fallback_csv_path(
    data_dir: Path,
    benchmark_name: str,
    gpu_backend: str,
    gpu_slug: str,
    device_id: int,
) -> Path:
    user = slugify_name(os.environ.get("USER", "") or os.environ.get("LOGNAME", "") or "user")
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    fname = (
        f"{benchmark_name}__backend-{gpu_backend}__gpu-{gpu_slug}__dev{device_id}"
        f"__user-{user}__ts-{ts}.csv"
    )
    return data_dir / fname


def ensure_writable_path(path: Path) -> None:
    """
    Upewnia się, że ścieżka (lub jej katalog nadrzędny) jest zapisywalna.
    Jeśli uruchomiono z sudo, próbuje przejąć własność na SUDO_USER.
    """
    path.parent.mkdir(parents=True, exist_ok=True)

    # Jeśli plik już istnieje i jest zapisywalny — OK.
    if path.exists() and os.access(path, os.W_OK):
        return

    # Jeśli plik nie istnieje, ale katalog jest zapisywalny — OK.
    if not path.exists() and os.access(path.parent, os.W_OK):
        return

    # Próba naprawy uprawnień przy sudo
    if os.geteuid() == 0:
        sudo_user = os.environ.get("SUDO_USER")
        if sudo_user:
            try:
                pw = pwd.getpwnam(sudo_user)
                uid, gid = pw.pw_uid, pw.pw_gid
                # Zmień właściciela katalogu i pliku (jeśli istnieje)
                try:
                    os.chown(path.parent, uid, gid)
                except Exception:
                    pass
                if path.exists():
                    try:
                        os.chown(path, uid, gid)
                    except Exception:
                        pass
                # Dodaj uprawnienia zapisu dla właściciela
                try:
                    os.chmod(path.parent, 0o755)
                except Exception:
                    pass
                if path.exists():
                    try:
                        os.chmod(path, 0o644)
                    except Exception:
                        pass
            except Exception:
                pass

    # Sprawdź ponownie po próbie naprawy
    if path.exists() and os.access(path, os.W_OK):
        return
    if not path.exists() and os.access(path.parent, os.W_OK):
        return

    raise PermissionError(
        f"Brak uprawnień do zapisu: {path}. "
        f"Rozwiąż przez: sudo chown -R $(whoami) {path.parent}"
    )


# Backward compatibility: stary format (bez device_id)
def make_gpu_csv_path(root: Path, bench_name: str, backend: str, gpu_model: str) -> Path:
    """Stary helper (zostawiony dla kompatybilności)."""
    return root / "data" / "gpu" / f"{bench_name}_{backend}_{slugify_name(gpu_model)}.csv"


def common_system_metadata(backend: str) -> Dict[str, Any]:
    """Wspólne metadane środowiska (dla CSV)."""
    return {
        "backend": backend,
        "system": platform.system(),
        "arch": platform.machine(),
        "hostname": socket.gethostname(),
        "python_version": platform.python_version(),
    }


def common_gpu_metadata(backend: str, gpu_name: str, device_id: int) -> Dict[str, Any]:
    """Wspólne metadane GPU (dla CSV)."""
    return {
        **common_system_metadata(backend),
        "gpu_model": gpu_name,
        "gpu_index": device_id,
    }
