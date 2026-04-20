"""
Map incoming protocol messages to macOS actions.
"""

from __future__ import annotations

import logging
import subprocess
from typing import Any, Callable, Dict, Mapping

from macos_integration import handle_potentiometer_value

from protocol import validate_envelope

logger = logging.getLogger(__name__)

Handler = Callable[[], None]

# Explicit Chrome so Automation + AppleScript URL reads work (default browser may be Safari).
_CHROME_APP = "Google Chrome"


def _open_url_in_google_chrome(url: str) -> None:
    """Open ``url`` in Google Chrome (not the system default browser)."""
    result = subprocess.run(
        ["open", "-a", _CHROME_APP, url],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        logger.warning(
            "Could not open URL in Chrome (%s): %s",
            result.returncode,
            (result.stderr or result.stdout or "").strip(),
        )


def open_chatgpt() -> None:
    _open_url_in_google_chrome("https://chatgpt.com/")


def open_visual_studio_code() -> None:
    subprocess.run(["open", "-a", "Visual Studio Code"], check=False)


def open_autodesk_fusion() -> None:
    for app_name in ("Autodesk Fusion", "Autodesk Fusion 360"):
        result = subprocess.run(
            ["open", "-a", app_name],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode == 0:
            return
    logger.error("Could not open Autodesk Fusion (tried common app names).")


def open_spotify_web() -> None:
    _open_url_in_google_chrome("https://open.spotify.com/")


def open_youtube_web() -> None:
    _open_url_in_google_chrome("https://www.youtube.com/")


CHORD_HANDLERS: Dict[str, Handler] = {
    "b1": open_chatgpt,
    "b2": open_visual_studio_code,
    "b3": open_autodesk_fusion,
    "b1b2": open_spotify_web,
    "b2b3": open_youtube_web,
}


def dispatch_message(data: Mapping[str, Any]) -> None:
    """Handle one decoded JSON object from the device."""
    if not validate_envelope(data):
        logger.warning("Unsupported or missing schema_version: %s", data)
        return

    message_type = data.get("type")
    if message_type == "button_chord":
        chord = data.get("chord")
        if not isinstance(chord, str):
            logger.warning("Invalid chord payload: %s", data)
            return
        handler = CHORD_HANDLERS.get(chord)
        if handler is None:
            logger.warning("Unknown chord: %s", chord)
            return
        logger.info("Action: chord %s", chord)
        handler()
        return

    if message_type == "potentiometer":
        raw = data.get("value")
        try:
            value = int(raw)
        except (TypeError, ValueError):
            logger.warning("Invalid potentiometer value: %s", data)
            return
        if value < 0 or value > 4095:
            logger.warning("Potentiometer out of range: %s", value)
            return
        handle_potentiometer_value(value)
        return

    logger.debug("Ignored message type: %s", message_type)
