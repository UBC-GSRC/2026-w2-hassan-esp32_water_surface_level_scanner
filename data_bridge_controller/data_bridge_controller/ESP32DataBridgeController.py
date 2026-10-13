import struct
import time
import serial

SOF = 0xAF
MAX_BODY = 32
PACKET_RULE = "<BB??H"          # node_id, sensor_id, trigger_shutter, measure_distance, distance_mm
PACKET_SIZE = struct.calcsize(PACKET_RULE)   # 6
NODE_ID_SELF = 0


def crc8(data: bytes) -> int:
    c = 0
    for b in data:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x07) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


class BridgeError(Exception):
    pass


class ESP32DataBridgeController:
    def __init__(self, com="COM4", baud=115200, timeout_s=1.0, retries=3):
        self.com = com
        self.baud = baud
        self.timeout_s = timeout_s
        self.retries = retries
        self.seq = 0
        self.rx = bytearray()
        self.ser = None
        self._open()

    # ---------- port handling ----------

    def _open(self):
        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = serial.Serial(self.com, baudrate=self.baud, timeout=0.02, write_timeout=1.0)
        self.rx.clear()
        self.ser.reset_input_buffer()

    def _hard_reset_bridge(self):
        """Best-effort reset of an ESP32 over USB-Serial-JTAG via RTS (esptool-style)."""
        try:
            self.ser.dtr = False
            self.ser.rts = True
            time.sleep(0.1)
            self.ser.rts = False
            time.sleep(1.5)  # wait for boot
        except Exception:
            pass
        self._open()

    def close(self):
        if self.ser is not None and self.ser.is_open:
            self.ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    # ---------- framing ----------

    @staticmethod
    def _frame(body: bytes) -> bytes:
        length = bytes([len(body)])
        return bytes([SOF]) + length + body + bytes([crc8(length + body)])

    def _read_frame(self, timeout_s):
        """Return a validated body, or None on timeout. Resyncs past garbage and log text."""
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            chunk = self.ser.read(self.ser.in_waiting or 1)
            if chunk:
                self.rx += chunk

            while True:
                i = self.rx.find(bytes([SOF]))
                if i < 0:
                    self.rx.clear()
                    break
                del self.rx[:i]
                if len(self.rx) < 2:
                    break
                n = self.rx[1]
                if n == 0 or n > MAX_BODY:
                    del self.rx[0]          # not a real frame start, resync
                    continue
                if len(self.rx) < n + 3:
                    break                    # need more bytes
                body = bytes(self.rx[2:2 + n])
                if crc8(bytes([n]) + body) == self.rx[2 + n]:
                    del self.rx[:n + 3]
                    return body
                del self.rx[0]               # bad CRC, resync
        return None

    # ---------- request/response ----------

    def _transact(self, packet: bytes):
        self.seq = (self.seq + 1) & 0xFF
        seq = self.seq
        frame = self._frame(bytes([seq]) + packet)

        last_err = "timeout"
        for attempt in range(self.retries):
            try:
                self.ser.reset_input_buffer()
                self.rx.clear()
                self.ser.write(frame)
                deadline = time.monotonic() + self.timeout_s
                while time.monotonic() < deadline:
                    body = self._read_frame(deadline - time.monotonic())
                    if body is None:
                        break
                    if len(body) == 1 + PACKET_SIZE and body[0] == seq:
                        return struct.unpack(PACKET_RULE, body[1:])
                    # wrong seq or length: stale reply, keep waiting
            except serial.SerialException as e:
                last_err = str(e)
                time.sleep(0.5)
                try:
                    self._open()
                except Exception:
                    pass

            if attempt == self.retries - 2:
                self._hard_reset_bridge()   # last resort before the final attempt

        raise BridgeError(f"No valid response after {self.retries} attempts ({last_err})")

    # ---------- public API ----------

    def trigger_camera(self) -> bool:
        # Sets all relays to open
        pkt = struct.pack(PACKET_RULE, NODE_ID_SELF, 0, True, False, 0)
        _, _, camera_triggered, _, _ = self._transact(pkt)

        time.sleep(1)

        # Sets all relays back to closed
        pkt = struct.pack(PACKET_RULE, NODE_ID_SELF, 0, False, False, 0)
        _, _, _, _, _ = self._transact(pkt)
        return camera_triggered

    def get_distance(self, sensor_id: int):
        pkt = struct.pack(PACKET_RULE, NODE_ID_SELF, sensor_id, False, True, 0)
        _, _, _, _, distance = self._transact(pkt)
        return -1 if distance == 0xFFFF else distance / 10


if __name__ == "__main__":
    counter = 0
    with ESP32DataBridgeController("COM31") as bridge:
        while counter < 2000:
            for sid in (1, 2, 3, 4):
                counter += 1
                d = bridge.get_distance(sid)
                print(f"{counter}: sensor {sid} -> {d}")
