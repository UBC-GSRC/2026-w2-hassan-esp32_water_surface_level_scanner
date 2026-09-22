from pytest_embedded import Dut

def test_unity(dut: Dut):
    dut.expect_exact("DONE.")