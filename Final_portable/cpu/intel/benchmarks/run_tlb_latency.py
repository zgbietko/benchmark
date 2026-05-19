from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from cpu.benchmarks import run_tlb_latency as _bench  # type: ignore


def main() -> None:
    _bench.main()


if __name__ == "__main__":
    main()
