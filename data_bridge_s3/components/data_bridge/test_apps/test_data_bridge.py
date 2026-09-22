from pytest_embedded import Dut
import struct 


# def test_debug(dut: Dut):
#     print(type(dut.serial.proc))
#     print(dir(dut.serial.proc))

def test_unity(dut: Dut):
    dut.expect_exact("PASS")

    dut.expect_exact("READY_SERIAL")
    packet = struct.pack(
    "<BB??H",
    1, # node_id
    2, # sensor_id
    1, # trigger_shutter
    0, # measure_distance
    1234 # distance_mm
    )
    # dut.write(packet)
    dut.write(packet)
    dut.expect_exact(
        "node=1 sensor=2 shutter=1 measure=0 distance=1234"
    )
    


