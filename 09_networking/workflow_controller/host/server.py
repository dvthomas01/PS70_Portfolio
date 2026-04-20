"""
WebSocket server for the AI Workflow Command Center (listens on all interfaces).
"""

from __future__ import annotations

import asyncio
import json
import logging
from typing import Any

import websockets
from websockets.exceptions import ConnectionClosed

from macos_integration import prime_system_volume_for_pot_session

from actions import dispatch_message

HOST = "0.0.0.0"
PORT = 8765

logger = logging.getLogger(__name__)


async def client_handler(websocket: Any) -> None:
    peer = getattr(websocket, "remote_address", None)
    logger.debug("Client connected: %s", peer)
    try:
        async for message in websocket:
            if not isinstance(message, str):
                logger.debug("Ignoring non-text frame")
                continue
            try:
                payload = json.loads(message)
            except json.JSONDecodeError:
                logger.warning("Invalid JSON from %s: %s", peer, message)
                continue
            if not isinstance(payload, dict):
                logger.warning("JSON must be an object: %s", payload)
                continue
            # AppleScript / subprocess in dispatch_message blocks for ~100–400ms; run in a
            # thread so the ESP can stream pot packets without each one queueing behind the last.
            await asyncio.to_thread(dispatch_message, payload)
    except ConnectionClosed as exc:
        logger.debug("WebSocket closed from %s: %s", peer, exc)
    except OSError as exc:
        logger.debug("TCP connection dropped from %s: %s", peer, exc)
    finally:
        logger.debug("Client session ended: %s", peer)


async def run_server() -> None:
    async with websockets.serve(client_handler, HOST, PORT):
        logger.info("Listening ws://%s:%s/", HOST, PORT)
        await asyncio.Future()


def main() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    logging.getLogger("websockets.server").setLevel(logging.WARNING)
    prime_system_volume_for_pot_session()
    asyncio.run(run_server())


if __name__ == "__main__":
    main()
