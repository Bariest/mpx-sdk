#!/usr/bin/env python3
"""One source of truth for the host ABI, and a check that nothing has drifted.

WHY THIS EXISTS

The host ABI is spelled out in five places that had no mechanical relationship
to each other:

    mangdang/main/sdk/wasm_host_functions.h   NATIVE_SYMBOLS[] — the real ABI
    mpx-cli/.../resource/mpx_host.h           the C bindings
    mpx-cli/.../resource/mpx_env.ts           the AssemblyScript bindings
    mpx-cli/.../resource/host_functions_wat.md the WAT reference
    HOST_FUNCTIONS.md                          the language-agnostic reference

Every one was edited by hand, at different times, and they drifted. An audit
found three symbols missing from the AssemblyScript bindings entirely, one
missing from the docs, gait enums that stopped eight entries short of what the
firmware accepted, and five reader functions documented as working that return
a hardcoded -1. None of that was carelessness — it is what five hand-maintained
copies of one list always does.

`abi/host_functions.json` is now the list. This script checks the others
against it and fails loudly when they disagree, so drift becomes a build error
rather than a bug someone finds on hardware six weeks later.

USAGE

    python tools/gen_abi.py --check      # verify everything agrees (exit 1 on drift)
    python tools/gen_abi.py --extract    # re-derive the table FROM the firmware
    python tools/gen_abi.py --table      # print the NATIVE_SYMBOLS[] block

WHY --check RATHER THAN FULL CODE GENERATION

mpx_host.h is not just declarations; it carries ~600 lines of hand-written
ergonomic helpers (robot_walk_forward, robot_gait_enum, pose structs) that are
genuinely better written by a person. Generating the whole file would mean
either losing those or embedding them in a generator, both worse than what
exists. So the generator owns the *table*, and verifies the *bindings* — which
is where all the real drift was.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
ABI_JSON = REPO / "abi" / "host_functions.json"

# The firmware lives beside the SDK in the normal checkout. Overridable because
# not everyone keeps them as siblings.
DEFAULT_FIRMWARE = REPO.parent / "mangdang" / "main" / "sdk" / "wasm_host_functions.h"

RES = REPO / "mpx-cli" / "src" / "mpx_cli" / "commands" / "resource"
C_HEADER = RES / "mpx_host.h"
TS_BINDINGS = RES / "mpx_env.ts"
WAT_DOC = RES / "host_functions_wat.md"
MD_DOC = REPO / "HOST_FUNCTIONS.md"


# ── Parsing ───────────────────────────────────────────────────────────────

def parse_firmware_table(header: Path) -> list[dict]:
    """Read NATIVE_SYMBOLS[] — the only definition that is actually executed."""
    text = header.read_text(encoding="utf-8")
    try:
        block = text[text.index("NATIVE_SYMBOLS[] = {"):]
        block = block[: block.index("};")]
    except ValueError:
        raise SystemExit(f"error: no NATIVE_SYMBOLS[] block in {header}")

    out, category = [], "core"
    for line in block.splitlines():
        heading = re.match(r"\s*//\s*[─\-]*\s*(.+?)\s*$", line)
        if heading and "{" not in line:
            category = heading.group(1).strip()
            continue
        row = re.match(r'\s*\{\s*"([a-z_0-9]+)",\s*\(void \*\)(\w+),\s*"([^"]*)",', line)
        if row:
            name, fn, sig = row.groups()
            out.append({
                "name": name,
                "host_fn": fn,
                "signature": sig,
                "params": list(sig[sig.index("(") + 1: sig.index(")")]),
                "result": sig[sig.index(")") + 1:] or None,
                "category": category,
            })
    return out


def load_abi() -> dict:
    if not ABI_JSON.exists():
        raise SystemExit(f"error: {ABI_JSON} missing — run --extract first")
    return json.loads(ABI_JSON.read_text(encoding="utf-8"))


# ── Checks ────────────────────────────────────────────────────────────────

def check(firmware: Path) -> int:
    abi = load_abi()
    symbols = abi["symbols"]
    by_name = {s["name"]: s for s in symbols}
    problems: list[str] = []

    # 1. The firmware IS the ABI. If it disagrees with the table, the table is
    #    stale and every other check below is measuring the wrong thing.
    if firmware.exists():
        fw = {s["name"]: s for s in parse_firmware_table(firmware)}
        for name, s in by_name.items():
            if name not in fw:
                problems.append(f"{name}: in abi.json but NOT registered in the firmware")
            elif fw[name]["signature"] != s["signature"]:
                problems.append(
                    f"{name}: signature drift — firmware '{fw[name]['signature']}' "
                    f"vs abi.json '{s['signature']}'")
        for name in fw:
            if name not in by_name:
                problems.append(f"{name}: registered in the firmware but missing from abi.json")
    else:
        print(f"note: firmware not found at {firmware} — skipping the table check")

    # 2. Bindings must declare every symbol. This is what actually broke: three
    #    symbols were absent from mpx_env.ts for the life of the SDK.
    c_text = C_HEADER.read_text(encoding="utf-8") if C_HEADER.exists() else ""
    ts_text = TS_BINDINGS.read_text(encoding="utf-8") if TS_BINDINGS.exists() else ""
    wat_text = WAT_DOC.read_text(encoding="utf-8") if WAT_DOC.exists() else ""
    md_text = MD_DOC.read_text(encoding="utf-8") if MD_DOC.exists() else ""

    for name, s in by_name.items():
        if c_text and not re.search(rf"\bextern\s+\w+\s+{re.escape(name)}\s*\(", c_text):
            problems.append(f"{name}: not declared in mpx_host.h")
        if ts_text and f'@external("env", "{name}")' not in ts_text:
            problems.append(f"{name}: not declared in mpx_env.ts")
        if wat_text and f'"{name}"' not in wat_text:
            problems.append(f"{name}: not in host_functions_wat.md")
        if md_text and name not in md_text:
            problems.append(f"{name}: not in HOST_FUNCTIONS.md")

    # 3. A void return is how seventeen error codes became unreachable. Never
    #    again silently: any symbol without a result must be deliberate.
    for name, s in by_name.items():
        if not s["result"]:
            problems.append(
                f"{name}: registered with NO result — its error code cannot reach "
                f"a skill. This is the ABI v1 bug; add 'i' to the signature.")

    # 4. C return type must agree with the signature's result.
    #
    #    Strip comments first. mpx_host.h documents each import as
    #    "Wasm import:  extern int robot_gait(...)" inside the doc block, so a
    #    naive search finds the comment and reports on prose rather than on the
    #    declaration. (It did, on the first run — and the comments turned out to
    #    be stale too, so both got fixed.)
    c_code = re.sub(r"/\*.*?\*/", "", c_text, flags=re.S)
    c_code = re.sub(r"//[^\n]*", "", c_code)
    for name, s in by_name.items():
        m = re.search(rf"\bextern\s+(\w+)\s+{re.escape(name)}\s*\(", c_code)
        if m and s["result"] == "i" and m.group(1) == "void":
            problems.append(f"{name}: signature returns i32 but mpx_host.h declares void")

    if problems:
        print(f"✗ {len(problems)} ABI problem(s):\n")
        for p in problems:
            print(f"   {p}")
        return 1

    print(f"✓ ABI v{abi['abi_version']}: {len(symbols)} symbols consistent across "
          f"the firmware table, mpx_host.h, mpx_env.ts and both references")
    return 0


def emit_table(abi: dict) -> str:
    """The NATIVE_SYMBOLS[] block, as the firmware should contain it."""
    width_n = max(len(s["name"]) for s in abi["symbols"]) + 3
    width_f = max(len(s["host_fn"]) for s in abi["symbols"]) + 2
    lines, cat = ["static NativeSymbol NATIVE_SYMBOLS[] = {"], None
    for s in abi["symbols"]:
        if s["category"] != cat:
            cat = s["category"]
            lines.append(f"\t// {cat}")
        name = f'"{s["name"]}",'.ljust(width_n)
        fn = f'(void *){s["host_fn"]},'.ljust(width_f + 9)
        sig = f'"{s["signature"]}",'.ljust(11)
        lines.append(f"\t{{ {name}{fn}{sig}nullptr }},")
    lines.append("};")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--check", action="store_true",
                    help="Verify every copy of the ABI agrees (exit 1 on drift)")
    ap.add_argument("--extract", action="store_true",
                    help="Re-derive abi/host_functions.json from the firmware")
    ap.add_argument("--table", action="store_true",
                    help="Print the NATIVE_SYMBOLS[] block")
    ap.add_argument("--firmware", type=Path, default=DEFAULT_FIRMWARE,
                    help="Path to wasm_host_functions.h")
    args = ap.parse_args()

    if args.extract:
        syms = parse_firmware_table(args.firmware)
        abi = load_abi() if ABI_JSON.exists() else {"abi_version": 2}
        abi["symbols"] = syms
        abi["symbol_count"] = len(syms)
        ABI_JSON.write_text(json.dumps(abi, indent=2) + "\n", encoding="utf-8")
        print(f"✓ extracted {len(syms)} symbols -> {ABI_JSON.relative_to(REPO)}")
        return 0

    if args.table:
        print(emit_table(load_abi()))
        return 0

    return check(args.firmware)


if __name__ == "__main__":
    sys.exit(main())
