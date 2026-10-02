import pytest

@pytest.mark.parametrize('count', [2], indirect=True)
def test_espnow(dut):
    dut0, dut1 = dut

    dut0.expect("READY_SERIAL")
    dut1.expect("READY_SERIAL")