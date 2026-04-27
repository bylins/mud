"""Extract C++ enum entries into a flat list keyed by bit_index.

The Bylins flag enums use a "planar" packing:
    kFoo  = 1u << N            -> plane 0
    kBar  = kIntOne   | (1<<N) -> plane 1
    kBaz  = kIntTwo   | (1<<N) -> plane 2
    kQux  = kIntThree | (1<<N) -> plane 3

bit_index = plane * 30 + N

This module reads an enum body from a header file and returns
{bit_index: enum_name} so callers can build flag-name tables without
hand-mirroring (and drifting from) the C++ source.
"""

from __future__ import annotations

import re
from pathlib import Path

PLANE_MARKER = {
    "kIntOne":   1 << 30,
    "kIntTwo":   2 << 30,
    "kIntThree": 3 << 30,
}


def _strip_comments(text: str) -> str:
    """Remove both /* ... */ and // ... \\n comments from C++ source text.

    Done before any other tokenization, because line comments often contain
    commas (Russian descriptions) which would otherwise corrupt our naïve
    comma-split entry parser.
    """
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def _evaluate(expr: str) -> int:
    """Eval a C++ literal/operator expression on integers."""
    expr = expr.strip().rstrip(",")
    expr = expr.replace("u<<", "<<").replace("U<<", "<<")
    # Drop trailing 'u' / 'U' on integer literals: 1u << 5 -> 1 << 5
    expr = re.sub(r"\b(\d+)[uU]\b", r"\1", expr)
    # Restricted globals: only the plane markers and basic int ops.
    return int(eval(expr, {"__builtins__": None}, dict(PLANE_MARKER)))


def _extract_body(text: str, enum_name: str) -> str:
    pat = re.compile(
        rf"enum\s+(?:class\s+)?{re.escape(enum_name)}\b[^{{]*\{{(?P<body>.*?)\}}",
        re.DOTALL,
    )
    m = pat.search(text)
    if not m:
        raise ValueError(f"enum {enum_name} not found")
    return m.group("body")


def parse_flag_enum(header_path: str | Path, enum_name: str) -> dict[int, str]:
    """Return {bit_index: enum_name} for a Bylins flag enum.

    Entries without ``= <expr>`` are skipped (we only handle explicit
    bit-index assignments — that's how every flag enum in the project is
    written).
    """
    text = Path(header_path).read_text(encoding="koi8-r", errors="replace")
    body = _strip_comments(_extract_body(text, enum_name))

    out: dict[int, str] = {}
    for raw in body.split(","):
        line = raw.strip()
        if not line:
            continue
        m = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(.+)$", line, re.DOTALL)
        if not m:
            continue  # skip plain-list entries; we want explicit assignments
        name, expr = m.group(1), m.group(2)
        try:
            value = _evaluate(expr)
        except Exception:
            continue
        if value <= 0:
            continue  # skip kUndefined/sentinels
        plane = value >> 30
        bit_in_plane_mask = value & ((1 << 30) - 1)
        if bit_in_plane_mask == 0 or (bit_in_plane_mask & (bit_in_plane_mask - 1)) != 0:
            continue  # not a single-bit flag (composite/legacy macro)
        bit_in_plane = bit_in_plane_mask.bit_length() - 1
        bit_index = plane * 30 + bit_in_plane
        # First definition wins — enums never have legitimate duplicates.
        out.setdefault(bit_index, name)
    return out


def parse_flag_enum_as_list(header_path: str | Path, enum_name: str,
                             max_bit_index: int = 120) -> list[str]:
    """Return a dense list[bit_index] -> enum name.

    Gaps in the enum become "UNUSED_<idx>" so the result can be fed
    straight into convert_to_yaml.py's existing helpers (which already
    filter such entries out of dictionaries).
    """
    table = parse_flag_enum(header_path, enum_name)
    if not table:
        return []
    if max_bit_index <= 0:
        max_bit_index = max(table) + 1
    return [table.get(i, f"UNUSED_{i}") for i in range(max_bit_index)]


def parse_value_enum(header_path: str | Path, enum_name: str) -> dict[int, str]:
    """Return {value: name} for an ordinary (non-bitfield) C++ enum.

    Entries without an explicit ``= <expr>`` get the previous-value+1, exactly
    like the C++ rule.
    """
    text = Path(header_path).read_text(encoding="koi8-r", errors="replace")
    body = _strip_comments(_extract_body(text, enum_name))

    out: dict[int, str] = {}
    next_value = 0
    for raw in body.split(","):
        line = raw.strip()
        if not line:
            continue
        m = re.match(r"\s*([A-Za-z_]\w*)\s*(?:=\s*(.+))?$", line, re.DOTALL)
        if not m:
            continue
        name, expr = m.group(1), m.group(2)
        if expr is not None:
            try:
                value = _evaluate(expr)
            except Exception:
                continue
        else:
            value = next_value
        out.setdefault(value, name)
        next_value = value + 1
    return out


def parse_value_enum_as_list(header_path: str | Path, enum_name: str,
                              max_value: int = 0) -> list[str]:
    """Return [name or 'UNUSED_<i>' for value 0..max_value-1]."""
    table = parse_value_enum(header_path, enum_name)
    if not table:
        return []
    if max_value <= 0:
        max_value = max(table) + 1
    return [table.get(i, f"UNUSED_{i}") for i in range(max_value)]
