"""mpx-cli — MPX-Dog Skill Development Kit.

This package auto-loads environment variables from ``.env`` files before
the CLI modules read defaults. Supported locations:

* the current working directory: ``./.env``
* the installed package directory: ``src/mpx_cli/.env`` during development

Environment variables defined in the real process environment always win.
"""

from __future__ import annotations

import os
from pathlib import Path


def _read_text_any_encoding(path: Path) -> str | None:
    """Read a .env whatever encoding the shell that made it chose.

    This exists because of one specific, silent Windows failure. `mpx-cli
    doctor` tells you to run

        echo MPX_HOST=192.168.1.42 > .env

    and in PowerShell `>` writes **UTF-16LE with a BOM**, not UTF-8. Read that
    with the locale default (cp1252 on a Windows box) and every byte still
    decodes -- to garbage. The key becomes "\ufeffM\x00P\x00X\x00_..." which
    matches nothing, so MPX_HOST was never set, the robot stayed unreachable,
    and there was no error anywhere to explain why. Following our own printed
    instructions produced a file we could not read.
    """
    raw = path.read_bytes()
    if raw.startswith((b"\xff\xfe", b"\xfe\xff")):        # UTF-16, PowerShell `>`
        encodings = ("utf-16",)
    elif raw.startswith(b"\xef\xbb\xbf"):                  # UTF-8 BOM, Notepad
        encodings = ("utf-8-sig",)
    else:
        encodings = ("utf-8", "utf-8-sig", "latin-1")
    for enc in encodings:
        try:
            return raw.decode(enc)
        except (UnicodeDecodeError, LookupError):
            continue
    return None


def _load_env_file(path: Path) -> None:
    if not path.exists():
        return

    try:
        text = _read_text_any_encoding(path)
        if text is None:
            return
        for line in text.splitlines():
            stripped = line.strip()
            if not stripped or stripped.startswith("#") or "=" not in stripped:
                continue

            key, value = stripped.split("=", 1)
            key = key.strip().lstrip("\ufeff")
            value = value.strip().strip('"').strip("'")
            if key and key not in os.environ:
                os.environ[key] = value
    except (OSError, UnicodeError):
        return


_PACKAGE_DIR = Path(__file__).resolve().parent
_load_env_file(Path.cwd() / ".env")
_load_env_file(_PACKAGE_DIR / ".env")

__version__ = "0.2.0"
