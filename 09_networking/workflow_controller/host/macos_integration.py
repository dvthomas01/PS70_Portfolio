"""
macOS system volume control for the workflow controller potentiometer.

Uses AppleScript (``osascript``) — no extra GUI dependencies.
"""

from __future__ import annotations

import logging
import os
import subprocess
from typing import Optional

logger = logging.getLogger(__name__)

_OSASCRIPT = "/usr/bin/osascript"

_POT_ADC_MIN = int(os.environ.get("WORKFLOW_POT_ADC_MIN", "0"))
_POT_ADC_MAX = int(os.environ.get("WORKFLOW_POT_ADC_MAX", "4095"))


def _env_truthy(name: str, *, default_when_unset: bool) -> bool:
    raw = os.environ.get(name)
    if raw is None or not str(raw).strip():
        return default_when_unset
    return str(raw).strip().lower() in ("1", "true", "yes", "on")


_VERIFY_VOLUME_READBACK = _env_truthy("WORKFLOW_VOLUME_VERIFY", default_when_unset=False)


def prime_system_volume_for_pot_session() -> None:
    """Unmute output once when the WebSocket server starts."""
    result = subprocess.run(
        [_OSASCRIPT, "-e", "set volume without output muted"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        msg = (result.stderr or result.stdout or "").strip()
        logger.warning("Startup unmute (AppleScript) failed (%s): %s", result.returncode, msg)
    else:
        logger.debug("Output unmuted at server startup.")


def adc_to_system_volume_percent(adc_value: int) -> int:
    """
    Map firmware ADC (0..4095 after onboard calibration) to macOS output volume 0..100.
    """
    lo = min(_POT_ADC_MIN, _POT_ADC_MAX)
    hi = max(_POT_ADC_MIN, _POT_ADC_MAX)
    if hi == lo:
        v = max(0, min(4095, adc_value))
        return (v * 100) // 4095
    x = max(lo, min(hi, adc_value))
    return int(round((x - lo) * 100.0 / float(hi - lo)))


def _read_output_volume_percent_osascript() -> Optional[int]:
    result = subprocess.run(
        [_OSASCRIPT, "-e", "output volume of (get volume settings)"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        return None
    try:
        return int(result.stdout.strip())
    except ValueError:
        return None


def set_output_volume_percent(percent: int) -> None:
    """Set system output volume to ``percent`` in ``0..100`` (unmute + level)."""
    percent = max(0, min(100, int(percent)))
    one_line = f"set volume output volume {percent} without output muted"
    result = subprocess.run(
        [_OSASCRIPT, "-e", one_line],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        err = (result.stderr or result.stdout or "").strip()
        logger.warning("set volume failed (%s): %s", result.returncode, err)
        result = subprocess.run(
            [
                _OSASCRIPT,
                "-e",
                "set volume without output muted",
                "-e",
                f"set volume output volume {percent}",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            logger.warning(
                "set volume fallback failed (%s): %s",
                result.returncode,
                (result.stderr or result.stdout or "").strip(),
            )
            return

    if _VERIFY_VOLUME_READBACK:
        verify = _read_output_volume_percent_osascript()
        if verify is not None and abs(verify - percent) > 5:
            logger.warning(
                "Volume set to %d%% but system reports %d%% (output device / rounding).",
                percent,
                verify,
            )
        logger.debug("Volume %d%% (read-back %s)", percent, verify)


def handle_potentiometer_value(value: int) -> None:
    """Map pot value to macOS system output volume."""
    value = max(0, min(4095, int(value)))
    set_output_volume_percent(adc_to_system_volume_percent(value))
