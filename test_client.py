#!python3

import pytest
import types
import socket as socketlib

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

    def recv(self):
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


@pytest.fixture
def socket():
    socket = socketlib.socket(socketlib.AF_INET, socketlib.SOCK_STREAM)
    socket.connect(('localhost', 4480))

    yield Socket(socket)

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
    socket.send(b'ERRO msg \0 more garbage \r\nPING')
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
