from __future__ import annotations

from pathlib import Path
from typing import Any


def summarize_reference_outputs(root: Path) -> dict[str, Any]:
    root = Path(root).expanduser().resolve()
    outputs = sorted(root.rglob('el_data_out.bin'))
    previews = sorted(root.rglob('output_preview.json'))
    return {
        'root': str(root),
        'output_buffers': len(outputs),
        'preview_jsons': len(previews),
        'paths': [str(p) for p in outputs[:16]],
    }
