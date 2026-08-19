"""mpx-cli setup — install the toolchain, so Docker is a choice and not a tax.

Every maker who tries this without the dev container hits the same wall: they
have a clang (ESP-IDF put one on PATH), `mpx-cli doctor` used to call it the
WASI SDK, and every build died with

    error: unable to create target: 'No available targets are compatible with
    triple "wasm32-unknown-wasip1"'

Detection tells the truth about that now, but the honest answer was still "go
and install a 100 MB toolchain yourself, correctly, and put it somewhere we
look". The dev container has always just DONE that -- one line in the
Dockerfile -- which is the real reason Docker "always works". This command is
that same line, for people who are not in a container.

VERSION_PINNED_TO_THE_DOCKERFILE on purpose: a native install and a container
install must produce the same compiler, or "works on my machine" comes back
wearing a different hat. tools/check.py asserts the two agree.
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path

# Keep in step with .devcontainer/Dockerfile — tools/check.py enforces it.
WASI_SDK_VERSION = "24"
WASI_SDK_FULL = "24.0"
BASE_URL = "https://github.com/WebAssembly/wasi-sdk/releases/download"


def install_root() -> Path:
    """Where we put toolchains. Not inside the repo: a clone should never
    carry 100 MB of compiler, and re-cloning should not mean re-downloading."""
    return Path(os.environ.get("MPX_HOME", Path.home() / ".mpx"))


def _asset_name() -> str | None:
    sys_name = platform.system()
    mach = platform.machine().lower()
    arch = "arm64" if mach in ("arm64", "aarch64") else "x86_64" if mach in ("x86_64", "amd64") else None
    osk = {"Windows": "windows", "Linux": "linux", "Darwin": "macos"}.get(sys_name)
    if not arch or not osk:
        return None
    return f"wasi-sdk-{WASI_SDK_FULL}-{arch}-{osk}.tar.gz"


def _download(url: str, dest: Path) -> None:
    print(f"  from {url}")
    with urllib.request.urlopen(url) as r, open(dest, "wb") as f:
        total = int(r.headers.get("Content-Length") or 0)
        done = 0
        while chunk := r.read(1 << 20):
            f.write(chunk)
            done += len(chunk)
            if total:
                pct = done * 100 // total
                print(f"\r  downloading {done >> 20} / {total >> 20} MB  {pct}%",
                      end="", flush=True)
    print()


def _extract(archive: Path, into: Path) -> Path:
    """Unpack, tolerating the symlinks Windows may refuse.

    wasi-sdk's bin/ has clang++ and friends as links to clang. On Windows
    without Developer Mode those raise OSError, and a hard failure here would
    mean "your toolchain did not install" over a file nobody calls. Copy
    instead and carry on.
    """
    into.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive) as tf:
        top = tf.getnames()[0].split("/")[0]
        members = tf.getmembers()
        kwargs = {"filter": "tar"} if sys.version_info >= (3, 12) else {}
        for m in members:
            try:
                tf.extract(m, into, **kwargs)
            except (OSError, NotImplementedError):
                if m.issym() or m.islnk():
                    src = (into / top / "bin" / Path(m.linkname).name)
                    dst = into / m.name
                    if src.exists():
                        dst.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copy2(src, dst)
                    continue
                raise
    return into / top


def _clang_in(root: Path) -> Path:
    return root / "bin" / ("clang.exe" if os.name == "nt" else "clang")


def run_setup(args: argparse.Namespace) -> None:
    target = install_root() / "wasi-sdk"
    clang = _clang_in(target)

    if clang.is_file() and not getattr(args, "force", False):
        print(f"WASI SDK already installed: {clang}")
        print("  mpx-cli doctor       to confirm")
        print("  mpx-cli setup --force  to reinstall")
        return

    asset = _asset_name()
    if not asset:
        print(f"No WASI SDK build for {platform.system()} {platform.machine()}.")
        print("  Install one by hand and point at it:")
        print("    setx WASI_CC <path-to>\\bin\\clang.exe" if os.name == "nt"
              else "    export WASI_CC=<path-to>/bin/clang")
        sys.exit(1)

    url = f"{BASE_URL}/wasi-sdk-{WASI_SDK_VERSION}/{asset}"
    print(f"Installing WASI SDK {WASI_SDK_FULL} into {target}")

    with tempfile.TemporaryDirectory() as tmp:
        archive = Path(tmp) / asset
        try:
            _download(url, archive)
        except Exception as exc:
            print(f"\n  download failed: {exc}")
            print(f"  Download it yourself and unpack to {target}:\n    {url}")
            sys.exit(1)

        print("  unpacking...")
        if target.exists():
            shutil.rmtree(target, ignore_errors=True)
        unpacked = _extract(archive, install_root())
        if unpacked != target:
            unpacked.rename(target)

    if not clang.is_file():
        print(f"  unpacked, but no compiler at {clang} — the archive layout changed.")
        sys.exit(1)
    if os.name != "nt":
        clang.chmod(clang.stat().st_mode | 0o111)

    # Never claim success on the strength of a file existing. That mistake is
    # exactly what reported ESP-IDF's clang as the WASI SDK. Build something.
    probe = subprocess.run(
        [str(clang), "--target=wasm32-wasip1", "-nostdlib", "-c", "-x", "c",
         "-o", os.devnull, "-"],
        input="int main(void){return 0;}", capture_output=True, text=True)
    if probe.returncode != 0:
        print("  installed, but it cannot emit wasm32:")
        print("   ", (probe.stderr or "").strip().splitlines()[:1])
        sys.exit(1)

    print(f"\nWASI SDK {WASI_SDK_FULL} installed and verified: it builds wasm32.")
    print("  mpx-cli doctor       should now be all green")
    print("  mpx-cli deploy       build, upload, run")


def add_setup_parser(sub: argparse._SubParsersAction) -> None:
    p = sub.add_parser("setup",
                       help="Download and install the WASI SDK (no Docker needed)")
    p.add_argument("--force", action="store_true",
                   help="Reinstall even if it is already there")
    p.set_defaults(func=run_setup)
