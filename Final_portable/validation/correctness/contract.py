from __future__ import annotations

import json
from pathlib import Path
from typing import Any


def default_validation_payload(*, scope: str, reason: str = "Brak walidacji numerycznej dla tego benchmarku.") -> dict[str, Any]:
    return {
        "scope": scope,
        "status": "not_run",
        "pass_count": 0,
        "warning_count": 0,
        "fail_count": 0,
        "blocked": False,
        "reason": reason,
        "records": [],
    }


def aggregate_validation_records(paths: list[Path], *, scope: str) -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    for path in paths:
        try:
            obj = json.loads(path.read_text(encoding="utf-8"))
        except Exception:
            continue
        if isinstance(obj, dict):
            rec = dict(obj)
            rec["path"] = str(path)
            records.append(rec)
    if not records:
        return default_validation_payload(scope=scope)
    pass_count = sum(1 for r in records if str(r.get("status", "")).lower() == "pass")
    warning_count = sum(1 for r in records if str(r.get("status", "")).lower() == "warning")
    fail_count = sum(1 for r in records if str(r.get("status", "")).lower() == "fail")
    status = "pass" if fail_count == 0 and warning_count == 0 else ("warning" if fail_count == 0 else "fail")
    return {
        "scope": scope,
        "status": status,
        "pass_count": pass_count,
        "warning_count": warning_count,
        "fail_count": fail_count,
        "blocked": fail_count > 0,
        "reason": "Agregacja plików validation.json",
        "records": records,
    }
