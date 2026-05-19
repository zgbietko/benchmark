from __future__ import annotations

from pathlib import Path
from typing import Any

from validation.correctness.contract import aggregate_validation_records


def summarize_replay_validation(root: Path) -> dict[str, Any]:
    root = Path(root).expanduser().resolve()
    paths = sorted(root.rglob('validation.json'))
    return aggregate_validation_records(paths, scope='replay')
