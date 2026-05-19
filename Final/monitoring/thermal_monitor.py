from __future__ import annotations

from pathlib import Path
import platform
import re
import subprocess
from typing import Any


def _safe_run(cmd: list[str]) -> str:
    try:
        proc = subprocess.run(cmd, text=True, capture_output=True, check=False)
    except Exception:
        return ""
    return proc.stdout.strip() if proc.returncode == 0 else ""


def capture_thermal_snapshot() -> dict[str, Any]:
    system = platform.system()
    result: dict[str, Any] = {"supported": False, "source": "unsupported", "system": system}
    if system == "Linux":
        temps = []
        for path in Path("/sys/class/thermal").glob("thermal_zone*/temp"):
            try:
                val = float(path.read_text().strip())
                if val > 1000.0:
                    val /= 1000.0
                if 0.0 < val < 150.0:
                    temps.append(val)
            except Exception:
                continue
        if temps:
            result.update(
                {
                    "supported": True,
                    "source": "linux_sysfs",
                    "cpu_temp_c": float(sum(temps) / len(temps)),
                    "max_temp_c": float(max(temps)),
                    "zones": len(temps),
                }
            )
            return result
        sensors_out = _safe_run(["sensors"])
        matches = [float(m) for m in re.findall(r"\+([0-9]+(?:\.[0-9]+)?)°C", sensors_out)]
        if matches:
            result.update(
                {
                    "supported": True,
                    "source": "lm_sensors",
                    "cpu_temp_c": float(sum(matches) / len(matches)),
                    "max_temp_c": float(max(matches)),
                }
            )
            return result
    if system == "Darwin":
        therm = _safe_run(["pmset", "-g", "therm"])
        if therm:
            state = "normal"
            if "CPU_Speed_Limit" in therm and "100" not in therm:
                state = "possible_throttle"
            result.update({"supported": True, "source": "pmset", "thermal_state": state, "raw": therm})
            return result
    return result
