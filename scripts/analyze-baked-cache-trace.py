#!/usr/bin/env python3
"""Summarize bounded IL2CACHE producer-to-SRV telemetry."""

from __future__ import annotations

import argparse
import collections
import gzip
import re
from dataclasses import dataclass, field
from pathlib import Path


EVENT_RE = re.compile(r"IL2CACHE (enabled:|create |srv |copy_image |copy_buffer_image |copy_resource |barrier |enhanced_barrier |destroy |suppressed )")
FIELD_RE = re.compile(r"([a-z_]+)=([^\s.]+)")


@dataclass
class Resource:
    cookie: int
    format: str = "unknown"
    allocation: str = "unknown"
    flags: str = "unknown"
    created_at: int | None = None
    destroyed_at: int | None = None
    srv_lines: list[int] = field(default_factory=list)
    incoming: list[tuple[int, dict[str, str]]] = field(default_factory=list)
    outgoing: list[tuple[int, dict[str, str]]] = field(default_factory=list)
    barriers: list[tuple[int, dict[str, str]]] = field(default_factory=list)


def read_lines(path: Path):
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt", encoding="utf-8", errors="replace") as stream:
        yield from stream


def fields(line: str) -> dict[str, str]:
    return dict(FIELD_RE.findall(line))


def as_int(value: str | None) -> int | None:
    if value is None:
        return None
    try:
        return int(value, 0)
    except ValueError:
        return None


def resource_for(resources: dict[int, Resource], cookie: int) -> Resource:
    return resources.setdefault(cookie, Resource(cookie=cookie))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    resources: dict[int, Resource] = {}
    marker_count = 0
    suppressed_count = 0
    copy_count = 0
    barrier_count = 0

    for line_number, line in enumerate(read_lines(args.log), 1):
        match = EVENT_RE.search(line)
        if not match:
            continue
        event = match.group(1).strip()
        values = fields(line)

        if event == "enabled:":
            marker_count += 1
            continue
        if event == "suppressed":
            suppressed_count += 1
            continue

        cookie = as_int(values.get("cookie"))
        if event == "create" and cookie is not None:
            resource = resource_for(resources, cookie)
            resource.format = values.get("format", "unknown")
            resource.allocation = values.get("allocation", "unknown")
            resource.flags = values.get("resource_flags", "unknown")
            resource.created_at = line_number
        elif event == "srv" and cookie is not None:
            resource_for(resources, cookie).srv_lines.append(line_number)
        elif event in {"barrier", "enhanced_barrier"} and cookie is not None:
            resource_for(resources, cookie).barriers.append((line_number, values))
            barrier_count += 1
        elif event == "destroy" and cookie is not None:
            resource_for(resources, cookie).destroyed_at = line_number
        elif event.startswith("copy_"):
            copy_count += 1
            src_cookie = as_int(values.get("src_cookie"))
            dst_cookie = as_int(values.get("dst_cookie"))
            if src_cookie is not None:
                resource_for(resources, src_cookie).outgoing.append((line_number, values))
            if dst_cookie is not None:
                resource_for(resources, dst_cookie).incoming.append((line_number, values))

    created = [resource for resource in resources.values() if resource.created_at is not None]
    bc3 = [resource for resource in created if resource.format.lower() == "0x4d"]
    bc3_no_incoming = [resource for resource in bc3 if not resource.incoming]
    bc3_no_srv = [resource for resource in bc3 if not resource.srv_lines]
    bc3_srv_before_copy = [
        resource for resource in bc3
        if resource.srv_lines and resource.incoming and min(resource.srv_lines) < min(item[0] for item in resource.incoming)
    ]
    bc3_srv_without_copy = [resource for resource in bc3 if resource.srv_lines and not resource.incoming]
    full_writes = sum(
        1 for resource in bc3 for _, values in resource.incoming
        if values.get("full_resource") == "1" or values.get("full_subresource") == "1"
    )

    format_counts = collections.Counter(resource.format for resource in created)
    signatures = collections.Counter(
        (resource.format, len(resource.incoming), len(resource.outgoing),
         len(resource.srv_lines), len(resource.barriers), resource.destroyed_at is not None)
        for resource in created
    )

    output: list[str] = [
        "# D06 baked-cache trace analysis",
        "",
        f"- Instrumentation markers: {marker_count}",
        f"- Matching resource creates: {len(created)}",
        f"- Copy events involving the class: {copy_count}",
        f"- Barrier events involving the class: {barrier_count}",
        f"- Suppression markers: {suppressed_count}",
        f"- BC3 (`0x4d`) cache resources: {len(bc3)}",
        f"- BC3 resources with no incoming logged copy: {len(bc3_no_incoming)}",
        f"- BC3 resources with no SRV creation: {len(bc3_no_srv)}",
        f"- BC3 resources first exposed as SRV before their first incoming copy: {len(bc3_srv_before_copy)}",
        f"- BC3 resources with an SRV but no incoming copy: {len(bc3_srv_without_copy)}",
        f"- Incoming BC3 copies marked full-resource/full-subresource: {full_writes}",
        "",
        "## Formats",
        "",
        "| DXGI format | Resources |",
        "|---|---:|",
    ]
    output.extend(f"| `{fmt}` | {count} |" for fmt, count in sorted(format_counts.items()))

    output.extend([
        "",
        "## Lifecycle signatures",
        "",
        "| Format | Incoming copies | Outgoing copies | SRVs | Barriers | Destroyed | Resources |",
        "|---|---:|---:|---:|---:|---|---:|",
    ])
    for signature, count in sorted(signatures.items()):
        fmt, incoming, outgoing, srvs, barriers, destroyed = signature
        output.append(f"| `{fmt}` | {incoming} | {outgoing} | {srvs} | {barriers} | {'yes' if destroyed else 'no'} | {count} |")

    suspicious = sorted(
        set(resource.cookie for resource in bc3_srv_without_copy + bc3_srv_before_copy)
    )
    output.extend([
        "",
        "## Ordering candidates",
        "",
        "BC3 cookies with an SRV but no incoming copy, or an SRV observed before",
        "their first incoming copy:",
        "",
        ", ".join(f"`{cookie}`" for cookie in suspicious) if suspicious else "None.",
        "",
        "This trace proves API ordering only. SRV creation does not prove that a draw",
        "sampled the descriptor, and unlogged render/UAV writes would require a later",
        "producer-specific extension of the diagnostic.",
        "",
    ])

    rendered = "\n".join(output)
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0 if marker_count == 1 else 1


if __name__ == "__main__":
    raise SystemExit(main())
