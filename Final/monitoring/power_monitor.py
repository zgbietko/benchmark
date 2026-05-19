from __future__ import annotations

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


def capture_power_snapshot() -> dict[str, Any]:
    system = platform.system()
    result: dict[str, Any] = {"supported": False, "source": "unsupported", "system": system}
    if system == "Linux":
        batt = _safe_run(["upower", "-e"])
        device = next((line.strip() for line in batt.splitlines() if "battery" in line.lower()), "")
        if device:
            info = _safe_run(["upower", "-i", device])
            state = re.search(r"state:\s*(\w+)", info)
            pct = re.search(r"percentage:\s*([0-9]+%)", info)
            result.update(
                {
                    "supported": True,
                    "source": "upower",
                    "battery_state": state.group(1) if state else "unknown",
                    "battery_percent": pct.group(1) if pct else "",
                    "on_ac_power": bool(state and state.group(1).lower() not in {"discharging"}),
                }
            )
            return result
    if system == "Darwin":
        batt = _safe_run(["pmset", "-g", "batt"])
        if batt:
            pct = re.search(r"(\d+)%", batt)
            result.update(
                {
                    "supported": True,
                    "source": "pmset",
                    "battery_percent": pct.group(1) + "%" if pct else "",
                    "on_ac_power": "AC Power" in batt,
                    "raw": batt,
                }
            )
            return result
    nvidia = _safe_run(["nvidia-smi", "--query-gpu=power.draw,power.limit", "--format=csv,noheader,nounits"])
    if nvidia:
        first = nvidia.splitlines()[0].split(",")
        if len(first) >= 2:
            try:
                result.update(
                    {
                        "supported": True,
                        "source": "nvidia-smi",
                        "avg_power_w": float(first[0].strip()),
                        "max_power_w": float(first[1].strip()),
                    }
                )
                return result
            except Exception:
                pass
    return result
