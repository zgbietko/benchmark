from __future__ import annotations

from pathlib import Path
from typing import Any
import yaml

ROOT = Path(__file__).resolve().parent


def load_profile(name: str) -> dict[str, Any]:
    path = ROOT / f"{name}.yaml"
    if not path.exists():
        raise FileNotFoundError(f"Unknown experiment profile: {name}")
    payload = yaml.safe_load(path.read_text(encoding='utf-8')) or {}
    if not isinstance(payload, dict):
        raise ValueError(f"Invalid profile payload in {path}")
    payload.setdefault('profile', name)
    return payload
