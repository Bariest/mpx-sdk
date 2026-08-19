"""mpx-cli doctor — is this setup going to work?

Every question a maker asks in their first hour is some version of "is it me or
is it broken", and each one used to need a different command and some knowledge
of how the pieces fit. This answers all of them at once and, where something is
wrong, says what to type.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from mpx_cli.sdk.toolchain import detect_all, sdk_include_dir
from mpx_cli.sdk.project import find_project

OK, WARN, BAD = "ok  ", "warn", "FAIL"


def _env_hint(line: str) -> str:
    """The right way to write a .env for the shell the user is actually in.

    PowerShell's `>` writes UTF-16LE. mpx-cli reads that correctly now, but
    telling someone to run a command that produces a file in an encoding
    nothing else on their machine expects is still bad advice.
    """
    if os.name == "nt":
        return f"'{line}' | Out-File -Encoding utf8 .env"
    return f"echo '{line}' > .env"


def add_doctor_parser(sub: argparse._SubParsersAction) -> None:
    from mpx_cli.cli import robot_opts
    p = sub.add_parser("doctor", parents=[robot_opts()],
                       help="Check that everything needed to build and deploy is present")
    p.add_argument("--no-robot", action="store_true",
                   help="Skip the robot connectivity check")


def _line(status: str, label: str, detail: str = "", fix: str = "") -> bool:
    print(f"  {status}  {label}" + (f"  {detail}" if detail else ""))
    if fix:
        for ln in fix.splitlines():
            print(f"          {ln}")
    return status != BAD


def cmd_doctor(args: argparse.Namespace) -> None:
    good = True
    print("mpx-cli doctor\n")

    # ── The SDK headers ───────────────────────────────────────────────────
    print("SDK")
    inc = sdk_include_dir()
    if inc:
        via = "MPX_SDK_INCLUDE" if os.environ.get("MPX_SDK_INCLUDE") else "found by searching upward"
        good &= _line(OK, "headers", f"{inc}  ({via})")
    else:
        good &= _line(BAD, "headers", "not found",
                      "Set it explicitly:\n"
                      "  export MPX_SDK_INCLUDE=/path/to/mpx-sdk/sdk/include\n"
                      "or run mpx-cli from inside the SDK checkout.")

    sdk_abi = None
    if inc:
        abi_json = inc.parent.parent / "abi" / "host_functions.json"
        if abi_json.is_file():
            try:
                d = json.loads(abi_json.read_text(encoding="utf-8"))
                sdk_abi = int(d["abi_version"])
                good &= _line(OK, "ABI", f"v{sdk_abi}, {d['symbol_count']} host functions")
            except Exception as exc:
                good &= _line(WARN, "ABI", f"could not read {abi_json.name}: {exc}")
        else:
            good &= _line(WARN, "ABI", "abi/host_functions.json not found")

        for h in ("mpx.h", "mpx/motion.h", "mpx/math.h", "mpx/gaits.h"):
            if not (inc / h).is_file():
                good &= _line(BAD, f"missing {h}", "",
                              "The SDK checkout looks incomplete.")

    # ── Toolchains ────────────────────────────────────────────────────────
    print("\nToolchains")
    tcs = detect_all()
    any_compiler = False
    for tc in tcs.values():
        if tc.bin:
            any_compiler = True
            _line(OK, tc.name, f"{tc.version or ''}".strip())
        else:
            # If we found a compiler that simply cannot emit wasm32, SAY SO.
            # "not found" sent people looking for a clang they already had,
            # while the real problem was that the one on PATH was ESP-IDF's.
            wrong = [t for t in tc.tried if "NO wasm32 target" in t]
            if wrong:
                _line(WARN, tc.name, "found, but it cannot build skills",
                      wrong[0] + "\n"
                      "  mpx-cli setup        installs the right one (~100 MB, once)")
                continue
            _line(WARN, tc.name, "not found",
                  {"WASI SDK": "For C skills — the usual choice.\n"
                               "  https://github.com/WebAssembly/wasi-sdk/releases -> /opt/wasi-sdk",
                   "WABT": "Only needed for .wat skills.",
                   "AssemblyScript": "Only needed for .ts skills.  npm i -g assemblyscript",
                   }.get(tc.name, ""))
    if not any_compiler:
        good &= _line(BAD, "no compiler at all", "",
                      "Nothing can be built yet. One command fixes it:\n"
                      "  mpx-cli setup        installs the WASI SDK (~100 MB, once)")

    # ── This project ──────────────────────────────────────────────────────
    print("\nProject")
    project = find_project()
    if project is None:
        _line(WARN, "no manifest.json here", "",
              "Not a problem — you are just not standing in a skill.\n"
              "  mpx-cli init my_move")
    else:
        _line(OK, "slug", project.slug)
        src = project.source
        if src:
            _line(OK, "source", str(src))
        else:
            good &= _line(BAD, "source", f"src/{project.slug}.* not found",
                          "The manifest's slug and the filename disagree.")
        declared = project.manifest.get("abi")
        if declared is None:
            _line(WARN, "manifest abi", "not set",
                  f'Add  "abi": {sdk_abi or 3}  so a mismatch is caught before upload.')
        elif sdk_abi is not None and int(declared) != sdk_abi:
            good &= _line(BAD, "manifest abi", f"{declared}, but this SDK is v{sdk_abi}",
                          "The robot will load the module and then trap on its first\n"
                          "host call. Flash matching firmware, then update the manifest.")
        else:
            _line(OK, "manifest abi", f"v{declared}")
        params = project.manifest.get("params") or []
        if params:
            _line(OK, "parameters", ", ".join(p.get("name", "?") for p in params))

    # ── The robot ─────────────────────────────────────────────────────────
    if not args.no_robot:
        print("\nRobot")
        host = getattr(args, "ip", None) or os.environ.get("MPX_HOST", "192.168.2.1")
        port = getattr(args, "port", None) or int(os.environ.get("MPX_PORT", "80"))
        try:
            from mpx_cli.sdk.connection import RobotClient
            client = RobotClient(host, port)
            skills = client.list_skills()
            n = len(skills.get("files", skills) if isinstance(skills, dict) else skills)
            _line(OK, "reachable", f"{host}:{port} — {n} skill(s) installed")
        except Exception as exc:
            # exc, not type(exc).__name__. RobotError already carries a
            # written explanation from _unreachable_hint(); printing the class
            # name threw it away and left the user with the word "RobotError".
            detail = str(exc).strip().splitlines()[0] if str(exc).strip() else type(exc).__name__
            _line(WARN, "not reachable", f"{host}:{port} — {detail}",
                  "Join the robot's Wi-Fi (MPX-Dog), or point at it directly:\n"
                  "  mpx-cli doctor --ip 192.168.1.42        (just this once)\n"
                  "  " + _env_hint("MPX_HOST=192.168.1.42") + "   (from now on)")

    # ── Verdict ───────────────────────────────────────────────────────────
    print()
    if good:
        print("Looks good. Try:  mpx-cli deploy")
    else:
        print("Something above needs fixing before a build will work.")
        sys.exit(1)
