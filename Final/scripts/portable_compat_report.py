#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from platform_support import build_host_support_report, render_host_support_markdown


def build_report() -> dict[str, object]:
    return build_host_support_report(ROOT)


def render_markdown(report: dict[str, object]) -> str:
    return render_host_support_markdown(report)


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate portable compatibility report for Final.")
    ap.add_argument("--json-out", default="")
    ap.add_argument("--md-out", default="")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    report = build_report()
    markdown = render_markdown(report)

    if args.json_out:
        path = Path(args.json_out).expanduser().resolve()
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    if args.md_out:
        path = Path(args.md_out).expanduser().resolve()
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(markdown, encoding="utf-8")

    if not args.quiet:
        print(json.dumps(report, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
