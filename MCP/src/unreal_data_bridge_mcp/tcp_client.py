"""TCP client for communicating with the UE plugin's TCP server."""

import socket
import json
import logging
import time

logger = logging.getLogger(__name__)

_CONNECT_TIMEOUT = 5.0
_RECV_TIMEOUT = 60.0
_RECONNECT_DELAY = 0.5


class UEConnection:
    """Manages TCP connection to the Unreal Data Bridge plugin."""

    def __init__(self, host: str = "127.0.0.1", port: int = 8742):
        self.host = host
        self.port = port
        self._socket: socket.socket | None = None

    @property
    def connected(self) -> bool:
        return self._socket is not None

    def connect(self) -> None:
        """Connect to the UE plugin TCP server. Raises ConnectionError if unavailable."""
        if self._socket is not None:
            return

        logger.debug("Connecting to Unreal Editor at %s:%d", self.host, self.port)
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(_CONNECT_TIMEOUT)
            sock.connect((self.host, self.port))
            sock.settimeout(_RECV_TIMEOUT)
            self._socket = sock
            logger.info("Connected to Unreal Editor at %s:%d", self.host, self.port)
        except (ConnectionRefusedError, TimeoutError, OSError) as e:
            self._socket = None
            raise ConnectionError(
                f"Cannot connect to Unreal Editor at {self.host}:{self.port}. "
                f"Is the editor running with UnrealDataBridge plugin enabled? Error: {e}"
            ) from e

    def disconnect(self) -> None:
        """Close the TCP connection."""
        if self._socket:
            try:
                self._socket.close()
            except OSError:
                pass
            self._socket = None

    def send_command(self, command: str, params: dict | None = None) -> dict:
        """Send a command to the UE plugin and return the response.

        On connection failure, automatically retries once after a brief delay.
        Raises ConnectionError if not connected after retry.
        Raises RuntimeError if the command fails.
        """
        last_error: Exception | None = None

        for attempt in range(2):
            try:
                self.connect()
                return self._send_and_receive(command, params)
            except ConnectionError as e:
                last_error = e
                self.disconnect()
                if attempt == 0:
                    logger.debug(
                        "Connection lost during '%s', reconnecting in %.1fs",
                        command,
                        _RECONNECT_DELAY,
                    )
                    time.sleep(_RECONNECT_DELAY)

        raise last_error

    def _send_and_receive(self, command: str, params: dict | None = None) -> dict:
        """Send a command and read the response. Internal method, no retry logic."""
        request = json.dumps({"command": command, "params": params or {}}) + "\n"
        start = time.monotonic()
        try:
            self._socket.sendall(request.encode("utf-8"))

            # Read line-delimited response (buffer until we get \n)
            buffer = b""
            while b"\n" not in buffer:
                chunk = self._socket.recv(65536)
                if not chunk:
                    self.disconnect()
                    raise ConnectionError("Connection closed by Unreal Editor")
                buffer += chunk

            elapsed = time.monotonic() - start
            logger.debug("Command '%s' completed in %.3fs", command, elapsed)

            line = buffer.split(b"\n", 1)[0]
            response = json.loads(line.decode("utf-8"))

            if not response.get("success"):
                error = response.get("error", {})
                raise RuntimeError(
                    f"UE command '{command}' failed: {error.get('message', 'Unknown error')} "
                    f"(code: {error.get('code', 'UNKNOWN')})"
                )

            return response

        except (BrokenPipeError, ConnectionResetError, OSError) as e:
            self.disconnect()
            raise ConnectionError(f"Lost connection to Unreal Editor: {e}") from e
