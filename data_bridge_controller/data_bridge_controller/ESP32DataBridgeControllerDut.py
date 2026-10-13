"""Same controller API, but the serial port is owned by a pytest-embedded DUT.

Put this file next to ESP32DataBridgeController.py. Framing, CRC, sequence matching and
retries are all inherited; only the I/O is replaced:
  - writes go through dut.write()
  - reads use dut.expect() with a binary regex for one response frame
"""
import re
import time

import pexpect

from .ESP32DataBridgeController import (
    ESP32DataBridgeController,
    PACKET_SIZE,
    SOF,
    crc8,
)

# Response frame = SOF + len + body (seq + packet) + crc
_BODY_LEN = 1 + PACKET_SIZE
_FRAME_RE = re.compile(
    b"\\x%02x\\x%02x.{%d}" % (SOF, _BODY_LEN, _BODY_LEN + 1),  # body + crc byte
    re.DOTALL,
)


class _DutPort:
    """Minimal stand-in for the pyserial object the base class expects."""

    is_open = True

    def __init__(self, dut):
        self._dut = dut

    def write(self, data: bytes):
        self._dut.write(data)

    def reset_input_buffer(self):
        # pytest-embedded owns the stream; stale replies are discarded by sequence number
        pass

    def close(self):
        pass


class ESP32DataBridgeControllerDut(ESP32DataBridgeController):
    def __init__(self, dut, timeout_s=1.0, retries=3):
        self.dut = dut
        super().__init__(com="<pytest-embedded dut>", timeout_s=timeout_s, retries=retries)

    # ---------- replaced port handling ----------

    def _open(self):
        self.ser = _DutPort(self.dut)
        self.rx.clear()

    def _hard_reset_bridge(self):
        # The DUT owns the port, so there is no RTS reset from here. Just give it a moment.
        time.sleep(0.5)

    def close(self):
        pass  # pytest-embedded closes the port

    # ---------- replaced frame reader ----------

    def _read_frame(self, timeout_s):
        """Return a CRC-valid response body, or None on timeout."""
        deadline = time.monotonic() + timeout_s
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            try:
                match = self.dut.expect(_FRAME_RE, timeout=remaining)
            except pexpect.TIMEOUT:
                return None

            raw = match.group(0)
            if isinstance(raw, str):
                raw = raw.encode("latin-1")

            length = raw[1]
            body = raw[2:2 + length]
            if crc8(raw[1:2 + length]) == raw[2 + length]:
                return body
            # CRC mismatch: false SOF match inside other output, keep looking
