"""HTTP client for communicating with the MPX-Dog robot.

Portable — uses only the Python standard library (urllib).
Works on any platform with Python 3.10+.
"""

from __future__ import annotations

import json
import os
import sys
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import quote, urljoin
from urllib.request import Request, urlopen

# ── Defaults (overridable via environment variables) ───────────────
DEFAULT_HOST = os.environ.get("MPX_HOST", "192.168.2.1")
DEFAULT_PORT = int(os.environ.get("MPX_PORT", "80"))


class RobotError(Exception):
    """Raised when the robot returns a non-OK response."""


class RobotClient:
    """HTTP client for the MPX-Dog robot's REST API.

    All methods raise `RobotError` on failure. Use the `host` and `port`
    properties to inspect the target.

    API reference (from the ESP32 firmware):
      GET  /v1/skills/list                    → list skills
      POST /v1/skills/run                     → run a skill
      POST /v1/skills/upload?name=<filename>  → upload a .wasm
      GET  /v1/fs/list                        → list all files
      GET  /v1/fs/info                        → filesystem stats
      POST /v1/fs/delete                      → delete a file
      GET  /v1/robot/status                   → robot state
    """

    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT) -> None:
        self.host = host
        self.port = port
        self._base = f"http://{host}:{port}"

    # ── Low-level helpers ────────────────────────────────────────

    def _url(self, path: str) -> str:
        return urljoin(self._base, path)

    def _request(
        self,
        method: str,
        path: str,
        data: bytes | None = None,
        headers: dict[str, str] | None = None,
    ) -> Any:
        """Send an HTTP request and return the parsed JSON response.

        Returns either a ``dict`` or ``list`` depending on the endpoint.
        """
        url = self._url(path)
        req = Request(url, data=data, method=method)
        if headers:
            for k, v in headers.items():
                req.add_header(k, v)
        try:
            with urlopen(req, timeout=15) as resp:
                body = resp.read().decode("utf-8")
                if resp.status == 200 and body:
                    return json.loads(body)
                return {"ok": resp.status == 200}
        except HTTPError as e:
            # HTTPError is a subclass of URLError, so without this branch every
            # 4xx/5xx was reported as a *connection* failure — and the robot's
            # own explanation, which is in the response body, was thrown away.
            # "409 Conflict" is far less useful than "a skill is already
            # running", and the two are the same event.
            detail = ""
            try:
                raw = e.read().decode("utf-8", "replace").strip()
                if raw:
                    try:
                        parsed = json.loads(raw)
                        detail = str(parsed.get("output")
                                     or parsed.get("error")
                                     or raw)
                    except json.JSONDecodeError:
                        detail = raw
            except Exception:
                pass
            suffix = f": {detail}" if detail else ""
            raise RobotError(f"Robot returned HTTP {e.code}{suffix}") from e
        except URLError as e:
            raise RobotError(f"Connection to {self.host}:{self.port} failed: {e}") from e
        except json.JSONDecodeError as e:
            raise RobotError(f"Invalid JSON from robot: {e}") from e

    def _request_raw(
        self,
        method: str,
        path: str,
        data: bytes | None = None,
        headers: dict[str, str] | None = None,
    ) -> bytes:
        url = self._url(path)
        req = Request(url, data=data, method=method)
        if headers:
            for k, v in headers.items():
                req.add_header(k, v)
        try:
            with urlopen(req, timeout=15) as resp:
                return resp.read()
        except URLError as e:
            raise RobotError(f"Connection to {self.host}:{self.port} failed: {e}") from e

    # ── Skills API ───────────────────────────────────────────────

    def list_skills(self) -> list[dict[str, Any]]:
        """List all .wasm skills installed on the robot."""
        result = self._request("GET", "/v1/skills/list")
        if isinstance(result, list):
            return result
        if "skills" in result:
            return list(result["skills"])
        return []

    def upload_skill(self, wasm_path: str) -> dict[str, Any]:
        """Upload a .wasm file to the robot.

        The filename is passed as a query parameter; the body is raw binary.
        """
        filename = os.path.basename(wasm_path)
        if not filename.endswith(".wasm"):
            print("⚠️  Warning: file does not end with .wasm", file=sys.stderr)

        with open(wasm_path, "rb") as f:
            data = f.read()

        if len(data) == 0:
            raise RobotError("Empty file — nothing to upload")
        if len(data) > 256 * 1024:
            raise RobotError(f"File too large ({len(data)} bytes) — max is 256 KB")

        headers = {"Content-Type": "application/octet-stream"}
        return self._request(
            "POST",
            f"/v1/skills/upload?name={filename}",
            data=data,
            headers=headers,
        )

    def upload_skill_bytes(self, data: bytes, filename: str,
                           skill_id: str = "", version: str = "",
                           title: str = "") -> dict[str, Any]:
        """Upload bytes already in memory, optionally recording where they came from.

        ``upload_skill`` reads from disk, which is right for `deploy`. `install`
        has the bytes from the gateway and never wants them on local disk.

        The provenance parameters are what let the ROBOT hold the record of
        what is installed. That record used to live only in a browser's
        localStorage, so a second device could not see an install and
        uninstalling from one deleted nothing on the other.
        """
        if not data:
            raise RobotError("Empty skill — nothing to upload")
        if len(data) > 256 * 1024:
            raise RobotError(f"Skill too large ({len(data)} bytes) — max is 256 KB")

        query = f"/v1/skills/upload?name={quote(filename, safe='')}"
        for key, value in (("skill_id", skill_id), ("version", version), ("title", title)):
            if value:
                query += f"&{key}={quote(str(value), safe='')}"

        return self._request(
            "POST", query, data=data,
            headers={"Content-Type": "application/octet-stream"},
        )

    def installed_skills(self) -> list[dict[str, Any]]:
        """What the robot itself records as installed, and from where."""
        result = self._request("GET", "/v1/skills/installed")
        if isinstance(result, dict):
            return list(result.get("skills", []))
        return []

    def uninstall_skill(self, skill_id: str) -> dict[str, Any]:
        """Delete an installed skill's file AND its manifest entry.

        Both, together — the old flow could drop an entitlement while leaving
        the .wasm runnable, or delete files while leaving a phantom install.
        """
        payload = json.dumps({"skill_id": skill_id}, separators=(",", ":")).encode("utf-8")
        return self._request("POST", "/v1/skills/uninstall", data=payload,
                             headers={"Content-Type": "application/json"})

    def run_skill(self, skill_name: str) -> dict[str, Any]:
        """Execute a skill by filename on the robot.

        Uses compact JSON (no spaces) because the firmware parses request
        bodies with ``strstr`` rather than a full JSON library.
        """
        payload = json.dumps({"skill": skill_name}, separators=(",", ":")).encode("utf-8")
        headers = {"Content-Type": "application/json"}
        return self._request("POST", "/v1/skills/run", data=payload, headers=headers)

    def skill_status(self) -> dict[str, Any]:
        """Is a skill running, and how did the last one finish?

        ``run_skill`` only ever reports ``started``: the robot spawns the skill
        on its own task and answers immediately, because a skill may run for up
        to 60 s and blocking the HTTP handler that long exhausts the socket
        pool. The outcome therefore arrives here, not there.

        Returns ``{"running": bool, "name"?, "elapsed_ms"?, "last"?}`` where
        ``last`` is ``{"name", "result", "ok", "message", "duration_ms"}``.
        """
        return self._request("GET", "/v1/skills/status")

    def logs(self, since: int = 0, max_lines: int = 200) -> dict[str, Any]:
        """Fetch captured log lines newer than ``since``.

        Returns ``{"next": int, "lines": [str]}``. Feed ``next`` back as
        ``since`` to read incrementally with no repeats and no gaps.
        """
        return self._request("GET", f"/v1/logs?since={since}&max={max_lines}")

    # ── File System API ──────────────────────────────────────────

    def list_files(self) -> list[dict[str, Any]]:
        """List all files on the robot's LittleFS."""
        result = self._request("GET", "/v1/fs/list")
        if isinstance(result, list):
            return result
        if "files" in result:
            return list(result["files"])
        return []

    def fs_info(self) -> dict[str, Any]:
        """Get filesystem total/used stats."""
        return self._request("GET", "/v1/fs/info")

    def delete_file(self, path: str) -> dict[str, Any]:
        """Delete a file from the robot's LittleFS.

        The path should be just the filename (e.g. ``test_skill.wasm``).

        Uses compact JSON (no spaces) because the firmware parses request
        bodies with ``strstr`` rather than a full JSON library.
        """
        payload = json.dumps({"path": path}, separators=(",", ":")).encode("utf-8")
        headers = {"Content-Type": "application/json"}
        return self._request("POST", "/v1/fs/delete", data=payload, headers=headers)

    # ── Robot Status ─────────────────────────────────────────────

    def get_status(self) -> dict[str, Any]:
        """Get current robot status (gait mode, config, offsets)."""
        return self._request("GET", "/v1/robot/status")

    def __repr__(self) -> str:
        return f"RobotClient(host={self.host!r}, port={self.port})"
