from pymodbus.client import ModbusSerialClient

PORT = "COM27"
BAUDRATE = 19200


def wait_for_enter(message):
    input(f"\n{message}\nPress ENTER to continue...")


def verify_address(client, address):
    try:
        result = client.read_holding_registers(
            address=2,          # Address register
            count=1,
            device_id=address
        )

        if result.isError():
            return False

        print(
            f"Found sensor at 0x{address:02X}"
        )

        return True

    except Exception:
        return False


print("=====================================")
print("URM14 Address Programming Tool")
print("=====================================")

while True:
    addr_text = input(
        "\nEnter desired sensor address "
        "(hex format, e.g. 0x11): "
    ).strip()

    try:
        NEW_ADDRESS = int(addr_text, 16)
        break
    except ValueError:
        print("Invalid hex address.")

print(
    f"\nTarget address: 0x{NEW_ADDRESS:02X}"
)

wait_for_enter(
    "Connect ONE URM14 sensor to the RainbowLink."
)

client = ModbusSerialClient(
    port=PORT,
    baudrate=BAUDRATE,
    parity="N",
    stopbits=1,
    bytesize=8,
    timeout=1,
)

if not client.connect():
    raise RuntimeError(
        f"Failed to open {PORT}"
    )

print(
    f"\nProgramming sensor to "
    f"0x{NEW_ADDRESS:02X}"
)

client.write_register(
    address=2,                  # Address register
    value=NEW_ADDRESS,
    device_id=0,                # Broadcast
    no_response_expected=True
)

client.close()

print(
    "\nAddress change command sent."
)

wait_for_enter(
    "Unplug power from the sensor, "
    "then reconnect it."
)

client = ModbusSerialClient(
    port=PORT,
    baudrate=BAUDRATE,
    parity="N",
    stopbits=1,
    bytesize=8,
    timeout=1,
)

if not client.connect():
    raise RuntimeError(
        f"Failed to open {PORT}"
    )

print(
    f"\nVerifying sensor at "
    f"0x{NEW_ADDRESS:02X}..."
)

if verify_address(
    client,
    NEW_ADDRESS
):
    print(
        "\n====================================="
    )
    print("SUCCESS")
    print(
        f"Sensor responds at "
        f"0x{NEW_ADDRESS:02X}"
    )
    print(
        "====================================="
    )
else:
    print(
        "\n====================================="
    )
    print("FAILED")
    print(
        f"Could not verify address "
        f"0x{NEW_ADDRESS:02X}"
    )
    print(
        "====================================="
    )

client.close()