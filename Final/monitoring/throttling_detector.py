from __future__ import annotations

from typing import Any


def detect_throttling(before: dict[str, Any], after: dict[str, Any]) -> dict[str, Any]:
    throttled = False
    reasons: list[str] = []
    before_max = float(before.get("max_temp_c", before.get("cpu_temp_c", 0.0)) or 0.0)
    after_max = float(after.get("max_temp_c", after.get("cpu_temp_c", 0.0)) or 0.0)
    if after.get("thermal_state") == "possible_throttle":
        throttled = True
        reasons.append("pmset zgłosił ograniczenie termiczne")
    if before_max and after_max and (after_max - before_max) >= 20.0:
        throttled = True
        reasons.append("duży wzrost temperatury")
    return {
        "throttling_detected": throttled,
        "temperature_before_c": before.get("cpu_temp_c") or before.get("max_temp_c"),
        "temperature_after_c": after.get("cpu_temp_c") or after.get("max_temp_c"),
        "reasons": reasons,
        "confidence": 0.8 if throttled else 0.4,
    }
