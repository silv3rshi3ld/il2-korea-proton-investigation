#!/usr/bin/env python3
"""Compare two or more collected IL-2 Proton runs without loading logs in memory."""

from __future__ import annotations

import argparse
import gzip
import json
import re
import sys
from collections import Counter
from pathlib import Path
from typing import TextIO


CATEGORIES: dict[str, re.Pattern[str]] = {
    "split END_ONLY": re.compile(
        r"split barrier.*END_ONLY|END_ONLY.*split barrier", re.IGNORECASE
    ),
    "VKD3D warning": re.compile(r"warn:.*vkd3d|vkd3d.*warn", re.IGNORECASE),
    "Wine error": re.compile(r"(?:^|:|\s)err:", re.IGNORECASE),
    "device lost/hang": re.compile(
        r"device[_ ]lost|VK_ERROR_DEVICE_LOST|GPU (?:hang|reset)", re.IGNORECASE
    ),
    "out of memory": re.compile(
        r"out of (?:device |host )?memory|VK_ERROR_OUT_OF_.*_MEMORY", re.IGNORECASE
    ),
    "descriptor buffer": re.compile(
        r"descriptor[._ ]buffer|VK_EXT_descriptor_buffer", re.IGNORECASE
    ),
    "host-visible upload": re.compile(
        r"no_upload_hvv|host.visible.*upload|upload.*host.visible|UPLOAD.*DEVICE_LOCAL",
        re.IGNORECASE,
    ),
    "sparse/residency": re.compile(r"sparse|residen", re.IGNORECASE),
    "queue selection": re.compile(
        r"queue family|queue.*(?:compute|transfer|graphics)|single.queue",
        re.IGNORECASE,
    ),
    "NUMA/OpenMP": re.compile(
        r"GetNuma|NUMA|OpenMP|libiomp|KMP_", re.IGNORECASE
    ),
}

MODULE_PATTERNS: dict[str, re.Pattern[str]] = {
    "D3D12": re.compile(r"d3d12(?:core)?\.dll", re.IGNORECASE),
    "DXGI": re.compile(r"dxgi\.dll", re.IGNORECASE),
    "D3D11": re.compile(r"d3d11\.dll", re.IGNORECASE),
    "game backend": re.compile(r"dxBackend12\.dll", re.IGNORECASE),
}

SEVERITY = re.compile(r"(?:^|:|\s)(warn|err|fixme):", re.IGNORECASE)
HEX_ADDRESS = re.compile(r"\b0x[0-9a-fA-F]{6,}\b")
WINE_PREFIX = re.compile(
    r"^(?:\d+(?:\.\d+)?:)?(?:[0-9a-fA-F]+:){0,2}(?:trace|warn|err|fixme):",
    re.IGNORECASE,
)
WHITESPACE = re.compile(r"\s+")


def parse_key_values(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.is_file():
        return values
    with path.open("rt", encoding="utf-8", errors="replace") as handle:
        for raw_line in handle:
            key, separator, value = raw_line.rstrip("\n").partition("=")
            if separator and key:
                values[key] = value
    return values


def choose_log(path: Path) -> tuple[Path, Path | None]:
    if path.is_file():
        return path, path.parent if (path.parent / "metadata.txt").is_file() else None
    if not path.is_dir():
        raise FileNotFoundError(path)
    # Prefer the exact compressed source. filtered.log is only a generated
    # convenience subset and may omit evidence needed by a later hypothesis.
    for name in ("proton.log.gz", "steam-247970.log", "filtered.log"):
        candidate = path / name
        if candidate.is_file():
            return candidate, path
    raise FileNotFoundError(f"no collected log found under {path}")


def open_text(path: Path) -> TextIO:
    if path.suffix == ".gz":
        return gzip.open(path, "rt", encoding="utf-8", errors="replace")
    return path.open("rt", encoding="utf-8", errors="replace")


def warning_signature(line: str) -> str | None:
    if not SEVERITY.search(line):
        return None
    normalized = WINE_PREFIX.sub("", line.strip())
    normalized = HEX_ADDRESS.sub("<address>", normalized)
    normalized = WHITESPACE.sub(" ", normalized)
    return normalized[:500]


def analyze(input_path: Path, top_count: int) -> dict[str, object]:
    log_path, run_dir = choose_log(input_path)
    counts = Counter({name: 0 for name in CATEGORIES})
    modules = Counter({name: 0 for name in MODULE_PATTERNS})
    signatures: Counter[str] = Counter()
    lines = 0

    with open_text(log_path) as handle:
        for line in handle:
            lines += 1
            for name, pattern in CATEGORIES.items():
                if pattern.search(line):
                    counts[name] += 1
            for name, pattern in MODULE_PATTERNS.items():
                if pattern.search(line):
                    modules[name] += 1
            signature = warning_signature(line)
            if signature:
                signatures[signature] += 1

    module_log = run_dir / "modules.log" if run_dir else None
    if module_log and module_log.is_file() and module_log != log_path:
        modules = Counter({name: 0 for name in MODULE_PATTERNS})
        with module_log.open("rt", encoding="utf-8", errors="replace") as handle:
            for line in handle:
                for name, pattern in MODULE_PATTERNS.items():
                    if pattern.search(line):
                        modules[name] += 1

    metadata: dict[str, str] = {}
    summary: dict[str, str] = {}
    launch_options = ""
    variant = ""
    if run_dir:
        metadata = parse_key_values(run_dir / "metadata.txt")
        summary = parse_key_values(run_dir / "summary.txt")
        if (run_dir / "launch-options.txt").is_file():
            launch_options = (run_dir / "launch-options.txt").read_text(
                encoding="utf-8", errors="replace"
            ).strip()
        if (run_dir / "variant.txt").is_file():
            variant = (run_dir / "variant.txt").read_text(
                encoding="utf-8", errors="replace"
            ).strip()

    label = metadata.get("run_id") or (run_dir.name if run_dir else log_path.name)
    return {
        "label": label,
        "input": str(input_path.resolve()),
        "log": str(log_path.resolve()),
        "variant": variant or metadata.get("variant", "unknown"),
        "launch_options": launch_options,
        "line_count_scanned": lines,
        "categories": dict(counts),
        "modules": dict(modules),
        "collector_summary": summary,
        "top_warning_signatures": [
            {"count": count, "text": signature}
            for signature, count in signatures.most_common(top_count)
        ],
    }


def markdown_table(results: list[dict[str, object]], section: str) -> list[str]:
    labels = [str(result["label"]) for result in results]
    lines = [
        f"## {section}",
        "",
        "| Signal | " + " | ".join(labels) + " |",
        "|---|" + "|".join("---:" for _ in labels) + "|",
    ]
    source_key = "categories" if section == "Diagnostic counts" else "modules"
    keys = list(results[0][source_key].keys())  # type: ignore[index, union-attr]
    for key in keys:
        values = [str(result[source_key][key]) for result in results]  # type: ignore[index]
        lines.append(f"| {key} | " + " | ".join(values) + " |")
    lines.append("")
    return lines


def render_markdown(results: list[dict[str, object]]) -> str:
    lines = ["# Proton log comparison", ""]
    for result in results:
        lines.extend(
            [
                f"- **{result['label']}**: variant `{result['variant']}`, "
                f"scanned `{result['line_count_scanned']}` lines from `{result['log']}`",
            ]
        )
    lines.append("")
    lines.extend(markdown_table(results, "Diagnostic counts"))
    lines.extend(markdown_table(results, "Module mentions"))
    lines.extend(["## Most frequent warning/error signatures", ""])
    for result in results:
        lines.append(f"### {result['label']}")
        lines.append("")
        signatures = result["top_warning_signatures"]
        if not signatures:
            lines.append("No warning/error signatures found in the scanned log.")
        else:
            for item in signatures:  # type: ignore[union-attr]
                text = str(item["text"]).replace("`", "\\`")
                lines.append(f"- {item['count']} × `{text}`")
        lines.append("")
    lines.extend(
        [
            "Counts show log differences, not visual outcomes or causality. Compare "
            "them with the two-run visual classification in the experiment matrix.",
            "",
        ]
    )
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare collected run directories or Proton log files."
    )
    parser.add_argument("inputs", nargs="+", type=Path, help="run directory or log")
    parser.add_argument(
        "--top", type=int, default=12, help="warning signatures per input (default: 12)"
    )
    parser.add_argument("--json", action="store_true", help="emit JSON instead of Markdown")
    parser.add_argument("--output", type=Path, help="write to this file instead of stdout")
    args = parser.parse_args()
    if len(args.inputs) < 2:
        parser.error("at least two run directories or logs are required")
    if args.top < 0:
        parser.error("--top must not be negative")
    return args


def main() -> int:
    args = parse_args()
    try:
        results = [analyze(path, args.top) for path in args.inputs]
    except (OSError, UnicodeError) as error:
        print(f"compare-logs.py: {error}", file=sys.stderr)
        return 1

    if args.json:
        output = json.dumps(results, indent=2, sort_keys=True) + "\n"
    else:
        output = render_markdown(results)

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        if args.output.exists():
            print(f"compare-logs.py: refusing to overwrite {args.output}", file=sys.stderr)
            return 1
        args.output.write_text(output, encoding="utf-8")
        print(f"Wrote {args.output}")
    else:
        sys.stdout.write(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
