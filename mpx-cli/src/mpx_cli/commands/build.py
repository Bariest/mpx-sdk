"""mpx-cli build — compile source files to WASM."""

from __future__ import annotations

import argparse
from pathlib import Path
from mpx_cli.sdk.toolchain import (
    compile_file,
    detect_all,
    inspect_wasm,
    validate_wasm,
)
from mpx_cli.sdk.project import MANIFEST_NAME, describe_missing, find_project


def add_build_parser(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("build", help="Compile a C/WAT/TS source file to .wasm")
    p.add_argument(
        "source", nargs="?",
        help="Source file (default: src/<slug>.<ext> from manifest.json)",
    )
    p.add_argument(
        "-o", "--output", default=None,
        help="Output .wasm path (default: build/<name>.wasm)",
    )
    p.add_argument(
        "--validate", action="store_true",
        help="Run wasm-validate on the output",
    )
    p.add_argument(
        "--inspect", action="store_true",
        help="Show imports/exports via wasm-objdump",
    )
    p.add_argument(
        "--show-toolchains", action="store_true",
        help="List detected toolchains and exit",
    )


def _resolve_source(args: argparse.Namespace) -> Path:
    """Explicit path wins; otherwise derive it from the project manifest.

    A DIRECTORY is accepted too, and means "that project". `mpx-cli deploy
    four-ways` is the obvious thing to type from one level up, and it used to
    fail with "Unsupported extension ''" — an error about a file extension,
    when the real answer is that a directory is a perfectly reasonable thing
    to point at.
    """
    if args.source:
        given = Path(args.source)

        if given.is_dir():
            # Deliberately not find_project(given): that walks *up*, so
            # pointing at a directory which is merely inside a project would
            # silently build the parent's skill instead of saying "there is
            # nothing here".
            project = (
                find_project(given)
                if (given / MANIFEST_NAME).is_file()
                else None
            )
            if project is None:
                raise RuntimeError(
                    f"'{given}' has no {MANIFEST_NAME}, so there is no skill "
                    f"here to build.\n"
                    f"   Pass a source file instead, or run 'mpx-cli init' to "
                    f"create a project."
                )
            source = project.source
            if source is None:
                raise RuntimeError(
                    f"'{given}' declares slug '{project.slug}' but has no "
                    f"matching source under {project.root / 'src'}.\n"
                    f"   Expected one of: {project.slug}.c / .cc / .cpp / .wat / .ts"
                )
            print(f"📦 {project.slug} (from {project.root / MANIFEST_NAME})")
            return source

        return given

    project = find_project()
    if project is None:
        raise RuntimeError(describe_missing("source file"))

    source = project.source
    if source is None:
        raise RuntimeError(
            f"No source file matching slug '{project.slug}' under "
            f"{project.root / 'src'}.\n"
            f"   Expected one of: {project.slug}.c / .cc / .cpp / .wat / .ts"
        )

    print(f"📦 {project.slug} (from {project.root / 'manifest.json'})")
    return source


def _default_build_output(source: Path) -> Path:
    """Default output path: ``build/<stem>.wasm`` alongside the source."""
    return source.parent.parent / "build" / f"{source.stem}.wasm"


def cmd_build(args: argparse.Namespace) -> None:
    if args.show_toolchains:
        print("🔧 Available toolchains:")
        for key, tc in detect_all().items():
            if tc.bin:
                status = f"✅ {tc.bin} ({tc.version})"
            else:
                status = "❌ not found"
            print(f"  {tc.name:<20s} {status}")
        return

    source = _resolve_source(args)

    if not source.exists():
        # Raise rather than print-and-return: cli.py turns an exception into
        # exit code 1, and a silent success here is what lets
        # `make build && make upload` push a stale binary.
        raise RuntimeError(f"Source file not found: {source}")

    # Default output: build/<name>.wasm (a sibling of src/)
    output_path = args.output or str(_default_build_output(source))
    Path(output_path).parent.mkdir(parents=True, exist_ok=True)

    result = compile_file(str(source), output_path)
    print(result.message)

    if not result.success:
        if result.stderr:
            print(result.stderr, end="")
        raise RuntimeError("Build failed")

    if result.output_path:
        wasm_path = Path(result.output_path)
        size_kb = wasm_path.stat().st_size / 1024
        print(f"   📦 Size: {size_kb:.1f} KB")

    if args.validate and result.output_path:
        print()
        vr = validate_wasm(result.output_path)
        print(vr.message)

    if args.inspect and result.output_path:
        print()
        ir = inspect_wasm(result.output_path)
        print(ir.message)