#!/usr/bin/env python3
"""Everything CI should run. `python tools/check.py`

Four things, in the order they are cheapest to fix:

  1. generated files are in sync with the firmware and the CLI
  2. every relative link in the docs points at something that exists
  3. every example has the three files it needs
  4. every example compiles for wasm32, and every import it uses is in the ABI

(4) is skipped with a note when no wasm-capable clang is on PATH, so this is
still useful on a machine without the toolchain.

The link check exists because the previous docs cited `examples/walk-with-gains`
as the worked example while that example sat in a folder named `_to_delete/`.
Nothing catches that except checking.
"""
from __future__ import annotations

import json
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
FAILURES: list[str] = []


def section(title: str) -> None:
    print(f"\n== {title}")


def fail(msg: str) -> None:
    FAILURES.append(msg)
    print(f"   FAIL  {msg}")


def ok(msg: str) -> None:
    print(f"   ok    {msg}")


# ── 1. generated files ──────────────────────────────────────────────────────
section("generated files")
for script in ("gen_abi.py", "gen_gaits.py", "gen_geometry.py",
               "gen_params.py", "gen_coverage.py", "gen_docs.py"):
    r = subprocess.run([sys.executable, str(ROOT / "tools" / script), "--check"],
                       capture_output=True, text=True, cwd=ROOT)
    (ok if r.returncode == 0 else fail)(
        f"{script} --check" + ("" if r.returncode == 0 else f"\n{r.stdout}{r.stderr}"))

# ── 2. links ────────────────────────────────────────────────────────────────
section("documentation links")
LINK = re.compile(r"\[[^\]]*\]\(([^)#]+?)(?:#[^)]*)?\)")
checked = broken = 0
for md in sorted(ROOT.rglob("*.md")):
    if "_to_delete" in md.parts or "node_modules" in md.parts:
        continue
    for target in LINK.findall(md.read_text(encoding="utf-8")):
        if target.startswith(("http://", "https://", "mailto:")):
            continue
        checked += 1
        if not (md.parent / target).resolve().exists():
            broken += 1
            fail(f"{md.relative_to(ROOT)} -> {target}")
if not broken:
    ok(f"{checked} relative links, all resolve")

# The one number every example must agree with. Read, never typed: this
# check existed and still said 3 the day the firmware moved to 4.
SDK_ABI = json.loads((ROOT / "abi" / "host_functions.json")
                     .read_text(encoding="utf-8"))["abi_version"]

# Every maker-facing page must be reachable from docs/README.md in one hop.
# The previous docs grew to 28 files because nothing noticed when a page
# stopped being linked; four unread pages are worse than one read one.
index = (ROOT / "docs" / "README.md").read_text(encoding="utf-8")
orphans = [p.name for p in sorted((ROOT / "docs").glob("*.md"))
           if p.name != "README.md" and p.name not in index]
(ok if not orphans else fail)(
    "every docs page is linked from the index"
    + ("" if not orphans else f" — orphaned: {', '.join(orphans)}"))

# ── 3. example structure ────────────────────────────────────────────────────
section("example structure")
for d in sorted((ROOT / "examples").iterdir()):
    if not d.is_dir():
        continue
    manifest = d / "manifest.json"
    if not manifest.is_file():
        fail(f"{d.name}: no manifest.json")
        continue
    m = json.loads(manifest.read_text(encoding="utf-8"))
    problems = []
    if not (d / "README.md").is_file():
        problems.append("no README.md")
    if not any((d / "src" / f"{m['slug']}{e}").is_file() for e in (".c", ".cc", ".ts", ".wat")):
        problems.append(f"no src/{m['slug']}.*")
    if m.get("abi") != SDK_ABI:
        problems.append(f"abi is {m.get('abi')}, expected {SDK_ABI}")
    (ok if not problems else fail)(f"{d.name}" + ("" if not problems else ": " + ", ".join(problems)))

# ── 4. compile ──────────────────────────────────────────────────────────────
section("compile every example for wasm32")
clang = shutil.which("clang") or shutil.which("/opt/wasi-sdk/bin/clang")
if not clang:
    print("   SKIPPED — no clang on PATH. This is the check that catches a header\n"
          "            change breaking every example; CI runs it, you are not.\n"
          "            apt install clang lld   (needs a wasm32 target)")
else:
    abi = json.loads((ROOT / "abi" / "host_functions.json").read_text(encoding="utf-8"))
    allowed = {s["name"] for s in abi["symbols"]}
    inc = ROOT / "sdk" / "include"
    outdir = ROOT / "build" / "_check"
    outdir.mkdir(parents=True, exist_ok=True)

    for src in sorted((ROOT / "examples").glob("*/src/*.c")):
        out = outdir / (src.stem + ".wasm")
        r = subprocess.run(
            [clang, "--target=wasm32", "-nostdlib", "-O2",
             "-Wall", "-Wextra", "-Wconversion", "-Wno-sign-conversion",
             "-Wl,--no-entry", "-Wl,--export=on_start",
             "-Wl,--export-if-defined=on_stop", "-Wl,--export-if-defined=on_tick", "-Wl,--import-undefined",
             "-I", str(inc), "-o", str(out), str(src)],
            capture_output=True, text=True)
        if r.returncode != 0 or r.stderr.strip():
            fail(f"{src.parent.parent.name}\n{r.stderr[:1200]}")
            continue
        data = out.read_bytes()
        unknown = {n.decode() for n in
                   re.findall(rb"\x03env.([a-z_][a-z_0-9]{2,31})", data)} - allowed
        if unknown:
            fail(f"{src.parent.parent.name}: imports not in the ABI: {sorted(unknown)}")
        else:
            ok(f"{src.parent.parent.name:<16} {len(data):>6} bytes")

# ── verdict ─────────────────────────────────────────────────────────────────
print()
if FAILURES:
    print(f"{len(FAILURES)} problem(s)")
    sys.exit(1)
print("all checks passed")
