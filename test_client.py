#!python3

import pytest
import socket as socketlib
import time

BUFFER_SIZE = 100

sstp_msg_lengths = {
    b'ERRO': 40,
    b'SOLN': 90,
    b'WORK': 93,
}

def to_sstp(msg):
    if type(msg) != bytes:
        msg = bytes(str(msg), encoding='ascii')
    header = msg[:4]
    if header in sstp_msg_lengths:
        length = sstp_msg_lengths[header] + 5
        msg_len = len(msg)
        # pad if necessary
        if msg_len < length:
            msg += b'\0' * (length - msg_len)
        # truncate if necessary
        elif msg_len > length:
            msg = msg[:length]
    return msg + b'\r\n'

class Socket:

    def __init__(self, socket):
        self.socket = socket
        self.recv_sleep = 0

    def recv(self):
        if self.recv_sleep > 0:
            time.sleep(self.recv_sleep)
        data = self.socket.recv(BUFFER_SIZE)
        if len(data) == 0:
            raise RuntimeError("Disconnected")
        return data

    def send(self, msg):
        total_len = len(msg)
        total_sent = 0
        while total_sent < total_len:
            sent = self.socket.send(msg[total_sent:])
            if sent == 0:
                raise RuntimeError("Disconnected")
            total_sent += sent


# decide what to connect to
addr, port = ('localhost', pytest.config.getoption('--port') or 4480)
recv_sleep = 0.05
if pytest.config.getoption('--digitalis'):
    addr = 'digitalis.eng.unimelb.edu.au'
    recv_sleep = 1
elif pytest.config.getoption('--digitalis2'):
    addr = 'digitalis2.eng.unimelb.edu.au'
    recv_sleep = 1

@pytest.fixture
def socket():
    socket = socketlib.socket(socketlib.AF_INET, socketlib.SOCK_STREAM)
    wrapper = Socket(socket)

    socket.connect((addr, port))
    wrapper.recv_sleep = recv_sleep

    yield wrapper

    socket.close()


def test_perfect_message(socket):
    socket.send(b'PING\r\n')
    assert socket.recv() == b'PONG\r\n'

def test_two_part_message(socket):
    socket.send(b'PIN')
    socket.send(b'G\r\n')
    assert socket.recv() == b'PONG\r\n'

def test_partial_message(socket):
    socket.send(b'PING\r\nPI')
    socket.send(b'NG\r\n')
    assert socket.recv() == b'PONG\r\nPONG\r\n'

def test_double_message(socket):
    socket.send(b'PING\r\nPING\r\n')
    assert socket.recv() == b'PONG\r\nPONG\r\n'

def test_too_sort_message(socket):
    socket.send(b'ERRO unpadded short message\r\n')
    assert socket.recv() == to_sstp('ERRO Malformed message.')
    # still works after that though
    socket.send(b'PING\r\n')
    assert socket.recv() == b'PONG\r\n'

def test_too_long_message(socket):
    socket.send(b'PING' + 2000 * b' ' + b'\r\n')
    assert socket.recv() == to_sstp('ERRO Malformed message.')
    # still works after that though
    socket.send(b'PING\r\n')
    assert socket.recv() == b'PONG\r\n'

def test_too_long_false_valid(socket):
    # exploit that relies on the knowing exactly how much to overflow
    socket.send((4 + 1 + 93 + 2) * b'0' + b'PING\r\n')
    assert socket.recv() == to_sstp(b'ERRO Malformed message.')

def test_repeated_send_read(socket):
    for _ in range(10):
        socket.send(b'PING\r\n')
        assert socket.recv() == b'PONG\r\n'

def test_null_terminator_before_delimiter(socket):
    socket.send(b'ERRO msg' + (sstp_msg_lengths[b'ERRO']-3) * b'\0' + b'\r\nPING')
    socket.send(b'\r\n')
    assert socket.recv() == (
        to_sstp('ERRO ERRO msgs are reserved for the server.')
        + b'PONG\r\n'
    )

def test_basic_sstp_responses(socket):
    socket.send(b'PING\r\n')
    assert socket.recv() == b'PONG\r\n'

    socket.send(b'PONG\r\n')
    assert socket.recv() == to_sstp('ERRO PONG msgs are reserved for the server.')

    socket.send(b'OKAY\r\n')
    assert socket.recv() == to_sstp('ERRO OKAY msgs are reserved for the server.')

    socket.send(to_sstp('ERRO false error'))
    assert socket.recv() == to_sstp('ERRO ERRO msgs are reserved for the server.')

def test_correct_soln_1(socket):
    socket.send(b'SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212147\r\n')
    assert socket.recv() == b'OKAY\r\n'

def test_correct_soln_2(socket):
    socket.send(b'SOLN 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002321ed8f\r\n')
    assert socket.recv() == b'OKAY\r\n'

def test_correct_soln_3(socket):
    socket.send(b'SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605\r\n')
    assert socket.recv() == b'OKAY\r\n'

def test_incorrect_soln(socket):
    socket.send(b'SOLN 1fffffff 1000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605\r\n')
    assert socket.recv() == to_sstp(b'ERRO Not a valid solution.')

def test_work_1(socket):
    socket.send(b'WORK 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212000 01\r\n')
    assert socket.recv() == b'SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212147\r\n'

def test_work_2(socket):
    socket.send(b'WORK 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 01\r\n')
    assert socket.recv() == b'SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605\r\n'

def test_work_3(socket):
    socket.send(b'WORK 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 04\r\n')
    assert socket.recv() == b'SOLN 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002321ed8f\r\n'

@pytest.mark.skip
def test_work_4(socket):
    socket.send(b'WORK 1dffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 01\r\n')
    assert socket.recv() == b'SOLN 1dffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023f6c072\r\n'

@pytest.mark.skip
def test_work_5(socket):
    socket.send(b'WORK 1d29ffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 04\r\n')
    assert socket.recv() == b'SOLN 1d29ffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000026b9c904\r\n'

def test_immediate_response_while_working(socket):
    socket.send(b'WORK 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 04\r\n')
    socket.recv_sleep = 0
    for _ in range(5):
        socket.send(b'PING\r\n')
        assert socket.recv() == b'PONG\r\n'
    assert socket.recv() == b'SOLN 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002321ed8f\r\n'
    time.sleep(0.05)
    socket.send(b'PING\r\n')
    assert socket.recv() == b'PONG\r\n'
