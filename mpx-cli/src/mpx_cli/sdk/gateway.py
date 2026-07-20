"""HTTP client for communicating with the MPX marketplace gateway.

Portable — uses only the Python standard library (urllib).
Works on any platform with Python 3.10+.

Mirrors the ``RobotClient`` pattern from ``connection.py`` but targets
the REST API of the gateway/marketplace server.
"""

from __future__ import annotations

import json
import os
from typing import Any
from urllib.error import URLError
from urllib.parse import urljoin
from urllib.request import Request, urlopen

# ── Defaults (overridable via environment variables) ───────────────
DEFAULT_GATEWAY_URL = os.environ.get("MPX_GATEWAY_URL", "http://localhost:8080")


class GatewayError(Exception):
    """Raised when the gateway returns a non-OK response or is unreachable."""


class GatewayClient:
    """HTTP client for the MPX marketplace gateway REST API.

    All methods raise ``GatewayError`` on failure.
    """

    def __init__(self, gateway_url: str | None = None) -> None:
        self._base = (gateway_url or DEFAULT_GATEWAY_URL).rstrip("/")

    # ── Low-level helpers ────────────────────────────────────────

    def _url(self, path: str) -> str:
        return urljoin(self._base + "/", path.lstrip("/"))

    def _request(
        self,
        method: str,
        path: str,
        data: dict[str, Any] | None = None,
        token: str | None = None,
    ) -> Any:
        """Send an HTTP request and return the parsed JSON response."""
        url = self._url(path)
        body = json.dumps(data, separators=(",", ":")).encode("utf-8") if data else None
        req = Request(url, data=body, method=method)
        req.add_header("Content-Type", "application/json")
        if token:
            req.add_header("Authorization", f"Bearer {token}")

        try:
            with urlopen(req, timeout=30) as resp:
                raw = resp.read().decode("utf-8")
                if raw:
                    return json.loads(raw)
                return {}
        except URLError as e:
            raise GatewayError(f"Connection to gateway at {self._base} failed: {e}") from e
        except json.JSONDecodeError as e:
            raise GatewayError(f"Invalid JSON from gateway: {e}") from e

    def _request_with_status(
        self,
        method: str,
        path: str,
        data: dict[str, Any] | None = None,
        token: str | None = None,
    ) -> tuple[int, Any]:
        """Send an HTTP request and return ``(status_code, parsed_body)``."""
        url = self._url(path)
        body = json.dumps(data, separators=(",", ":")).encode("utf-8") if data else None
        req = Request(url, data=body, method=method)
        req.add_header("Content-Type", "application/json")
        if token:
            req.add_header("Authorization", f"Bearer {token}")

        try:
            with urlopen(req, timeout=30) as resp:
                raw = resp.read().decode("utf-8")
                parsed = json.loads(raw) if raw else {}
                return resp.status, parsed
        except URLError as e:
            # Try to extract HTTP status from the error
            if hasattr(e, "code") and e.code is not None:
                try:
                    raw_body = e.read().decode("utf-8") if hasattr(e, "read") else "{}"
                    parsed = json.loads(raw_body) if raw_body else {}
                    return e.code, parsed
                except Exception:
                    return e.code, {"error": str(e)}
            raise GatewayError(
                f"Connection to gateway at {self._base} failed: {e}"
            ) from e
        except json.JSONDecodeError as e:
            raise GatewayError(f"Invalid JSON from gateway: {e}") from e

    # ── Auth endpoints ───────────────────────────────────────────

    def signup(self, username: str, password: str) -> dict[str, Any]:
        """Register a new developer account.

        POST /v1/auth/signup
        """
        return self._request("POST", "/v1/auth/signup", {
            "username": username,
            "password": password,
        })

    def login(self, username: str, password: str) -> dict[str, Any]:
        """Authenticate and receive a JWT.

        POST /v1/auth/login  →  ``{"token": "..."}``
        """
        return self._request("POST", "/v1/auth/login", {
            "username": username,
            "password": password,
        })

    def refresh_token(self, token: str) -> dict[str, Any]:
        """Refresh an expiring JWT.

        POST /v1/auth/refresh
        """
        return self._request("POST", "/v1/auth/refresh", token=token)

    # ── Skill discovery endpoints ────────────────────────────────

    def list_skills(self) -> list[dict[str, Any]]:
        """List all marketplace skills.

        GET /v1/skills
        """
        result = self._request("GET", "/v1/skills")
        if isinstance(result, list):
            return result
        if "skills" in result:
            return list(result["skills"])
        return []

    def get_skill(self, skill_id: str) -> dict[str, Any]:
        """Get details for a single skill.

        GET /v1/skills/:id
        """
        return self._request("GET", f"/v1/skills/{skill_id}")

    def get_versions(self, skill_id: str) -> list[dict[str, Any]]:
        """List all published versions of a skill.

        GET /v1/skills/:id/versions
        """
        result = self._request("GET", f"/v1/skills/{skill_id}/versions")
        if isinstance(result, list):
            return result
        if "versions" in result:
            return list(result["versions"])
        return []

    def get_manifest(self, skill_id: str, version: str | None = None) -> dict[str, Any]:
        """Get the manifest/readme for a skill.

        GET /v1/skills/:id/manifest
        Optionally query ``?version=<version>``.
        """
        path = f"/v1/skills/{skill_id}/manifest"
        if version:
            path += f"?version={version}"
        return self._request("GET", path)

    def check_slug(self, slug: str, token: str) -> dict[str, Any]:
        """Check whether a slug is available.

        GET /v1/skills/check?slug=<slug>
        Requires authentication.
        """
        return self._request("GET", f"/v1/skills/check?slug={slug}", token=token)

    # ── Publish endpoint ─────────────────────────────────────────

    def publish(
        self,
        skill_id: str,
        title: str,
        version: str,
        artifact_b64: str,
        manifest: dict[str, Any],
        token: str,
    ) -> tuple[int, dict[str, Any]]:
        """Publish or update a skill version.

        POST /v1/publish

        Returns ``(status_code, response_body)`` so the caller can handle
        HTTP 409 (version conflict) and other statuses.

        Args:
            skill_id: Fully qualified skill ID (``{username}~{slug}``).
            title: Human-readable title.
            version: Semver string (e.g. ``"1.0.0"``).
            artifact_b64: Base64-encoded WASM binary.
            manifest: Full skill manifest dict.
            token: JWT for authentication.
        """
        body: dict[str, Any] = {
            "skill_id": skill_id,
            "title": title,
            "skill_type": "WASM",
            "source_language": manifest.get("source_language", "c"),
            "version": version,
            "artifact": artifact_b64,
            "manifest": manifest,
        }
        return self._request_with_status("POST", "/v1/publish", data=body, token=token)

    # ── Robot endpoints ──────────────────────────────────────────

    def get_robot(self, uuid: str) -> dict[str, Any]:
        """Get robot info.

        GET /v1/robots/:uuid
        """
        return self._request("GET", f"/v1/robots/{uuid}")

    def get_robot_skills(self, uuid: str) -> list[dict[str, Any]]:
        """List skills assigned to a robot.

        GET /v1/robots/:uuid/skills
        """
        result = self._request("GET", f"/v1/robots/{uuid}/skills")
        if isinstance(result, list):
            return result
        if "skills" in result:
            return list(result["skills"])
        return []
