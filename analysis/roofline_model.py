#!/usr/bin/env python3
"""analysis/roofline_model.py

Roofline estimate based on measured peaks from microbench CSV files.
Supports CPU/GPU and export to JSON/CSV.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


ROOT = Path(__file__).resolve().parents[1]
CPU_DIR = ROOT / "data" / "cpu"
GPU_DIR = ROOT / "data" / "gpu"
RUNS_DIR = ROOT / "data" / "runs"


def _to_float(v: object) -> float:
    try:
        if v is None:
            return float("nan")
        s = str(v).strip()
        if not s:
            return float("nan")
        return float(s)
    except Exception:
        return float("nan")


def _read_rows(path: Path) -> Iterable[Dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as f:
        r = csv.DictReader(f)
        yield from r


def _latest_session_dir() -> Path | None:
    latest = RUNS_DIR / "latest"
    if latest.exists():
        try:
            p = latest.resolve()
            if p.exists():
                return p
        except Exception:
            pass
    latest_txt = RUNS_DIR / "latest.txt"
    if latest_txt.exists():
        name = latest_txt.read_text(encoding="utf-8").strip()
        if name:
            p = RUNS_DIR / name
            if p.exists():
                return p
    return None


def _data_dirs(scope: str, session: str) -> Tuple[Path, Path]:
    if scope == "global":
        return CPU_DIR, GPU_DIR
    if session and session != "latest":
        p = RUNS_DIR / session
        return p / "cpu", p / "gpu"
    latest = _latest_session_dir()
    if latest is not None:
        return latest / "cpu", latest / "gpu"
    return CPU_DIR, GPU_DIR


@dataclass
class PeakRecord:
    target: str
    model: str
    backend: str
    peak_bw_gbps: float
    peak_gflops: float


def _cpu_peaks(cpu_dir: Path) -> Dict[str, PeakRecord]:
    out: Dict[str, PeakRecord] = {}
    for p in sorted(cpu_dir.glob("*.csv")):
        for row in _read_rows(p):
            model = str(row.get("cpu_model", "")).strip() or "unknown_cpu"
            bw = _to_float(row.get("gbps") or row.get("throughput_gbps"))
            gf = _to_float(row.get("gflops") or row.get("throughput_gflops"))
            rec = out.get(model)
            if rec is None:
                rec = PeakRecord("cpu", model, "cpu", float("nan"), float("nan"))
                out[model] = rec
            if not math.isnan(bw):
                if bw <= 0 or bw > 1e5:
                    bw = float("nan")
            if not math.isnan(gf):
                if gf <= 0 or gf > 1e7:
                    gf = float("nan")
            if not math.isnan(bw):
                rec.peak_bw_gbps = bw if math.isnan(rec.peak_bw_gbps) else max(rec.peak_bw_gbps, bw)
            if not math.isnan(gf):
                rec.peak_gflops = gf if math.isnan(rec.peak_gflops) else max(rec.peak_gflops, gf)
    return out


def _gpu_peaks(gpu_dir: Path) -> Dict[Tuple[str, str], PeakRecord]:
    out: Dict[Tuple[str, str], PeakRecord] = {}
    for p in sorted(gpu_dir.glob("*.csv")):
        for row in _read_rows(p):
            backend = str(row.get("backend", "unknown")).strip().lower() or "unknown"
            model = str(row.get("gpu_model", "unknown")).strip() or "unknown_gpu"
            key = (backend, model)
            rec = out.get(key)
            if rec is None:
                rec = PeakRecord("gpu", model, backend, float("nan"), float("nan"))
                out[key] = rec

            bw = _to_float(row.get("throughput_gbps") or row.get("gbps"))
            gf = _to_float(
                row.get("gflops_peak")
                or row.get("gflops")
                or row.get("throughput_gflops")
                or row.get("gflops_mean")
            )
            if not math.isnan(bw):
                if bw <= 0 or bw > 1e5:
                    bw = float("nan")
            if not math.isnan(gf):
                if gf <= 0 or gf > 1e7:
                    gf = float("nan")
            if not math.isnan(bw):
                rec.peak_bw_gbps = bw if math.isnan(rec.peak_bw_gbps) else max(rec.peak_bw_gbps, bw)
            if not math.isnan(gf):
                rec.peak_gflops = gf if math.isnan(rec.peak_gflops) else max(rec.peak_gflops, gf)
    return out


def _pick_record(
    target: str,
    cpu_records: Dict[str, PeakRecord],
    gpu_records: Dict[Tuple[str, str], PeakRecord],
    backend: str,
    model_contains: str,
) -> PeakRecord | None:
    filt = model_contains.strip().lower()
    if target == "cpu":
        cand = [r for r in cpu_records.values() if (not filt or filt in r.model.lower())]
    elif target == "gpu":
        cand = [
            r
            for r in gpu_records.values()
            if (not backend or r.backend == backend.lower())
            and (not filt or filt in r.model.lower())
        ]
    else:
        raise ValueError(target)
    if not cand:
        return None
    cand = [r for r in cand if not math.isnan(r.peak_bw_gbps) and not math.isnan(r.peak_gflops)]
    if not cand:
        return None
    return max(cand, key=lambda r: (r.peak_gflops, r.peak_bw_gbps))


def _estimate(rec: PeakRecord, ai: float, bytes_moved: float, flops_total: float) -> Dict[str, float | str]:
    bw_limited = ai * rec.peak_bw_gbps
    attainable = min(rec.peak_gflops, bw_limited)
    regime = "memory-bound" if bw_limited < rec.peak_gflops else "compute-bound"
    est_time = flops_total / (attainable * 1e9) if attainable > 0 else float("inf")
    return {
        "ai_flop_per_byte": ai,
        "bytes_moved": bytes_moved,
        "flops_total": flops_total,
        "peak_bw_gbps": rec.peak_bw_gbps,
        "peak_gflops": rec.peak_gflops,
        "bw_limited_gflops": bw_limited,
        "attainable_gflops": attainable,
        "regime": regime,
        "estimated_time_s": est_time,
    }


def _print_estimate(title: str, rec: PeakRecord, est: Dict[str, float | str]) -> None:
    print(f"\n=== {title} ===")
    print(f"model               : {rec.model}")
    print(f"backend             : {rec.backend}")
    print(f"AI                  : {est['ai_flop_per_byte']:.6f} FLOP/byte")
    print(f"bytes               : {est['bytes_moved']:.3e} B")
    print(f"flops               : {est['flops_total']:.3e}")
    print(f"peak_bw             : {est['peak_bw_gbps']:.3f} GB/s")
    print(f"peak_compute        : {est['peak_gflops']:.3f} GFLOP/s")
    print(f"bw_ceiling          : {est['bw_limited_gflops']:.3f} GFLOP/s")
    print(f"attainable          : {est['attainable_gflops']:.3f} GFLOP/s")
    print(f"regime              : {est['regime']}")
    print(f"estimated_time      : {est['estimated_time_s']:.6f} s")


def _export(export_dir: Path, payload: Dict[str, object]) -> None:
    export_dir.mkdir(parents=True, exist_ok=True)
    json_path = export_dir / "roofline_result.json"
    csv_path = export_dir / "roofline_result.csv"
    json_path.write_text(json.dumps(payload, indent=2, ensure_ascii=True), encoding="utf-8")

    rows: List[Dict[str, object]] = []
    for item in payload.get("results", []):  # type: ignore[arg-type]
        row = dict(item)
        rec = row.pop("record", {})
        est = row.pop("estimate", {})
        row.update({f"record_{k}": v for k, v in rec.items()})
        row.update({f"estimate_{k}": v for k, v in est.items()})
        rows.append(row)
    if rows:
        with csv_path.open("w", encoding="utf-8", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)
    print(f"[OK] export json: {json_path}")
    print(f"[OK] export csv : {csv_path}")


def main() -> None:
    ap = argparse.ArgumentParser(description="Roofline model from measured peaks.")
    ap.add_argument("--target", choices=["cpu", "gpu", "both"], required=True)
    ap.add_argument("--backend", default="", help="dla target=gpu/both: cuda|metal|hip|opencl")
    ap.add_argument("--model-contains", default="", help="filtr substring na model")
    ap.add_argument("--ai", type=float, required=True, help="Arithmetic intensity [FLOP/byte]")
    ap.add_argument("--bytes", type=float, default=1e9, help="Przetwarzane bajty [B]")
    ap.add_argument("--flops", type=float, default=-1.0, help="FLOPs total; if <0 then ai*bytes")
    ap.add_argument("--scope", choices=["global", "session"], default="global")
    ap.add_argument("--session", default="latest", help="session name in data/runs (for --scope session)")
    ap.add_argument("--export-dir", default="", help="optional export directory for json/csv")
    args = ap.parse_args()

    bytes_moved = float(args.bytes)
    flops_total = float(args.flops) if args.flops > 0 else float(args.ai) * bytes_moved

    cpu_dir, gpu_dir = _data_dirs(scope=args.scope, session=args.session)
    cpu_records = _cpu_peaks(cpu_dir) if cpu_dir.exists() else {}
    gpu_records = _gpu_peaks(gpu_dir) if gpu_dir.exists() else {}

    payload: Dict[str, object] = {
        "input": {
            "target": args.target,
            "backend": args.backend,
            "model_contains": args.model_contains,
            "ai": args.ai,
            "bytes": bytes_moved,
            "flops": flops_total,
            "scope": args.scope,
            "session": args.session,
            "cpu_dir": str(cpu_dir),
            "gpu_dir": str(gpu_dir),
        },
        "results": [],
    }

    targets = ["cpu", "gpu"] if args.target == "both" else [args.target]
    for t in targets:
        rec = _pick_record(
            target=t,
            cpu_records=cpu_records,
            gpu_records=gpu_records,
            backend=args.backend,
            model_contains=args.model_contains,
        )
        if rec is None:
            print(f"[WARN] Brak danych peak dla target={t}")
            continue
        est = _estimate(rec, ai=float(args.ai), bytes_moved=bytes_moved, flops_total=flops_total)
        _print_estimate(title=f"ROOFLINE ESTIMATE ({t.upper()})", rec=rec, est=est)
        payload["results"].append(
            {
                "target": t,
                "record": asdict(rec),
                "estimate": est,
            }
        )

    if args.target == "both" and len(payload["results"]) == 2:
        a, b = payload["results"]  # type: ignore[index]
        ta = float(a["estimate"]["estimated_time_s"])  # type: ignore[index]
        tb = float(b["estimate"]["estimated_time_s"])  # type: ignore[index]
        faster = a["target"] if ta < tb else b["target"]
        ratio = (max(ta, tb) / max(min(ta, tb), 1e-12))
        print(f"\n[COMPARE] faster={faster}, speedup~{ratio:.3f}x")

    if args.export_dir:
        _export(Path(args.export_dir), payload)


if __name__ == "__main__":
    main()
