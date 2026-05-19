from __future__ import annotations

from typing import Any


MEMORY_FAMILY = {
    "memory-bound",
    "irregular-memory-bound",
    "cache-bound",
    "tlb-bound",
    "latency-bound",
    "page-walk-bound",
}


def _to_float(value: object) -> float | None:
    try:
        if value is None:
            return None
        text = str(value).strip()
        if not text or text.lower() == "nan":
            return None
        return float(text)
    except Exception:
        return None


def normalize_bottleneck_family(label: str) -> str:
    key = str(label or "").strip().lower()
    if not key:
        return ""
    if key in MEMORY_FAMILY:
        return "memory"
    if key == "compute-bound":
        return "compute"
    if key == "synchronization-bound":
        return "synchronization"
    if key == "transfer-bound":
        return "transfer"
    if key == "mixed":
        return "mixed"
    return key


def classify_workload_class(*, benchmark_name: str, kernel_name: str, arithmetic_intensity: float | None) -> str:
    label = f"{benchmark_name} {kernel_name}".strip().lower()
    ai = arithmetic_intensity

    if any(token in label for token in ("pointer", "latency", "cache_latency")):
        return "latency-bound"
    if any(token in label for token in ("tlb", "page_walk", "page-walk")):
        return "latency-bound"
    if any(token in label for token in ("stream", "memcpy", "mem_copy", "copy", "bandwidth", "saxpy", "axpy")):
        return "bandwidth-bound"
    if "spmv" in label:
        return "irregular memory-bound"
    if "stencil" in label:
        return "mixed stencil"
    if any(token in label for token in ("gemm", "fma", "matmul")):
        return "compute-bound"
    if any(token in label for token in ("coreml", "neural_engine", "ne")) and "probe" in label:
        return "compute-bound"
    if any(token in label for token in ("reduction", "scan", "prefix")):
        return "bandwidth-bound"
    if any(token in label for token in ("assembly_like", "author_fem_assembly")):
        if ai is not None and ai < 0.35:
            return "bandwidth-bound"
        if ai is not None and ai >= 1.8:
            return "compute-bound"
        return "mixed / application-bound"
    if any(token in label for token in ("fem", "filip", "integration")):
        return "mixed / application-bound"
    if ai is None:
        return "mixed / application-bound"
    if ai < 0.25:
        return "bandwidth-bound"
    if ai < 1.5:
        return "mixed / application-bound"
    return "compute-bound"


def _fallback_predicted_label(*, benchmark_name: str, kernel_name: str, arithmetic_intensity: float | None) -> str:
    workload = classify_workload_class(
        benchmark_name=benchmark_name,
        kernel_name=kernel_name,
        arithmetic_intensity=arithmetic_intensity,
    )
    mapping = {
        "bandwidth-bound": "memory-bound",
        "latency-bound": "latency-bound",
        "compute-bound": "compute-bound",
        "irregular memory-bound": "irregular-memory-bound",
        "mixed stencil": "cache-bound",
        "mixed / application-bound": "mixed",
    }
    return mapping.get(workload, "mixed")


def classify_bottleneck(
    *,
    benchmark_name: str,
    kernel_name: str,
    arithmetic_intensity: float | None,
    measured_gflops: float | None,
    measured_bandwidth: float | None,
    predicted_gflops: float | None,
    latency_ns: float | None = None,
    tlb_signal: float | None = None,
) -> dict[str, Any]:
    label = f"{benchmark_name} {kernel_name}".strip().lower()
    ai = arithmetic_intensity
    measured_gflops = _to_float(measured_gflops)
    measured_bandwidth = _to_float(measured_bandwidth)
    predicted_gflops = _to_float(predicted_gflops)
    latency_ns = _to_float(latency_ns)
    tlb_signal = _to_float(tlb_signal)

    if any(token in label for token in ("tlb", "page_walk", "page-walk")) or (tlb_signal is not None and tlb_signal > 0.7):
        return {
            "bottleneck": "TLB-bound",
            "family": normalize_bottleneck_family("TLB-bound"),
            "confidence": 0.95,
            "reason": "Benchmark bezpośrednio mierzy koszty translacji adresów / page walk.",
        }

    if any(token in label for token in ("pointer", "latency", "cache_latency")):
        return {
            "bottleneck": "latency-bound",
            "family": normalize_bottleneck_family("latency-bound"),
            "confidence": 0.95,
            "reason": "Benchmark bezpośrednio mierzy opóźnienia łańcucha wskaźników lub cache latency.",
        }

    if "spmv" in label:
        return {
            "bottleneck": "irregular-memory-bound",
            "family": normalize_bottleneck_family("irregular-memory-bound"),
            "confidence": 0.9,
            "reason": "SpMV ma niską intensywność obliczeniową i nieregularny dostęp do pamięci.",
        }

    if any(token in label for token in ("stream", "memcpy", "mem_copy", "bandwidth", "saxpy", "axpy", "reduction")):
        return {
            "bottleneck": "memory-bound",
            "family": normalize_bottleneck_family("memory-bound"),
            "confidence": 0.9,
            "reason": "Kernel należy do klasy strumieniowej / bandwidth-bound.",
        }

    if "stencil" in label:
        confidence = 0.75 if (ai is not None and ai <= 0.25) else 0.65
        return {
            "bottleneck": "cache-bound",
            "family": normalize_bottleneck_family("cache-bound"),
            "confidence": confidence,
            "reason": "Stencil zależy od regularnego reuse danych i jakości wykorzystania cache.",
        }

    if any(token in label for token in ("gemm", "fma", "matmul")):
        return {
            "bottleneck": "compute-bound",
            "family": normalize_bottleneck_family("compute-bound"),
            "confidence": 0.92,
            "reason": "Kernel należy do klasy wysokiej intensywności obliczeniowej.",
        }

    if any(token in label for token in ("coreml", "neural_engine", "ai_accel")) and any(
        token in label for token in ("matmul", "mlp", "predict", "probe")
    ):
        return {
            "bottleneck": "compute-bound",
            "family": normalize_bottleneck_family("compute-bound"),
            "confidence": 0.78,
            "reason": "Sciezka AI acceleration (CoreML/matmul) jest klasyfikowana jako obliczeniowa.",
        }

    if any(token in label for token in ("fem", "filip", "integration")):
        if predicted_gflops and measured_gflops:
            achieved = measured_gflops / predicted_gflops if predicted_gflops else None
            if achieved is not None and ai is not None and ai >= 2.0 and achieved >= 0.55:
                return {
                    "bottleneck": "compute-bound",
                    "family": normalize_bottleneck_family("compute-bound"),
                    "confidence": 0.7,
                    "reason": "FEM osiąga znaczący ułamek przewidzianego roofline przy wysokiej intensywności obliczeniowej.",
                }
        return {
            "bottleneck": "mixed",
            "family": normalize_bottleneck_family("mixed"),
            "confidence": 0.75,
            "reason": "FEM / kod aplikacyjny łączy koszt pamięci, lokalność i część obliczeniową.",
        }

    if any(token in label for token in ("assembly_like", "author_fem_assembly")):
        if ai is not None and ai < 0.35:
            return {
                "bottleneck": "memory-bound",
                "family": normalize_bottleneck_family("memory-bound"),
                "confidence": 0.76,
                "reason": "Assembly-like ma niska intensywnosc obliczeniowa dla tej konfiguracji.",
            }
        if predicted_gflops and measured_gflops and ai is not None and ai >= 1.8:
            achieved = measured_gflops / max(predicted_gflops, 1e-12)
            if achieved >= 0.55:
                return {
                    "bottleneck": "compute-bound",
                    "family": normalize_bottleneck_family("compute-bound"),
                    "confidence": 0.72,
                    "reason": "Assembly-like osiaga wysoki procent przewidzianego roofline przy wysokiej AI.",
                }
        return {
            "bottleneck": "mixed",
            "family": normalize_bottleneck_family("mixed"),
            "confidence": 0.72,
            "reason": "Assembly-like laczy koszt obliczen i ruchu danych.",
        }

    if ai is not None:
        if latency_ns is not None and latency_ns > 80.0 and ai < 0.2:
            return {
                "bottleneck": "latency-bound",
                "family": normalize_bottleneck_family("latency-bound"),
                "confidence": 0.7,
                "reason": "Niska intensywność obliczeniowa przy wysokim opóźnieniu sugeruje ograniczenie latencją pamięci.",
            }
        if ai < 0.2:
            return {
                "bottleneck": "memory-bound",
                "family": normalize_bottleneck_family("memory-bound"),
                "confidence": 0.65,
                "reason": "Bardzo niska intensywność obliczeniowa wskazuje na ograniczenie przepustowością pamięci.",
            }
        if ai >= 2.0:
            if predicted_gflops and measured_gflops and measured_gflops / predicted_gflops >= 0.5:
                return {
                    "bottleneck": "compute-bound",
                    "family": normalize_bottleneck_family("compute-bound"),
                    "confidence": 0.72,
                    "reason": "Wysoka intensywność i sensowny procent roofline wskazują na ograniczenie obliczeniowe.",
                }
            return {
                "bottleneck": "mixed",
                "family": normalize_bottleneck_family("mixed"),
                "confidence": 0.58,
                "reason": "Wysoka intensywność bez wyraźnej dominacji roofline; klasyfikacja mieszana.",
            }

    fallback = _fallback_predicted_label(
        benchmark_name=benchmark_name,
        kernel_name=kernel_name,
        arithmetic_intensity=ai,
    )
    return {
        "bottleneck": fallback,
        "family": normalize_bottleneck_family(fallback),
        "confidence": 0.45,
        "reason": "Klasyfikacja heurystyczna na podstawie rodzaju kernela.",
    }
