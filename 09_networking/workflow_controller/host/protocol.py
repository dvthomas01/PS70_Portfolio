"""
JSON message helpers for the ESP32 <-> macOS WebSocket protocol.

Schema version 1 (see firmware snprintf payloads and this module).
"""

from __future__ import annotations

from typing import Any, Mapping

SCHEMA_VERSION = 1


def validate_envelope(data: Mapping[str, Any]) -> bool:
    """Return True if ``data`` declares a supported ``schema_version``."""
    try:
        return int(data.get("schema_version", -1)) == SCHEMA_VERSION
    except (TypeError, ValueError):
        return False
