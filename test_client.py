#!python3

import pytest
import types
import socket as socketlib

BUFFER_SIZE = 100


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

def sstp(msg, length=-1):
    if length > 0:
        msg_len = len(msg)
        if msg_len < length:
            msg += b'\0' * (length - msg_len)
        elif msg_len > length:
            msg = msg[:length]
    return msg + b'\r\n'


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
    assert socket.recv() == sstp(b'ERRO Malformed message.', 45)
    # still works after that though
    socket.send(b'PING\r\n')
    assert socket.recv() == b'PONG\r\n'

def test_too_long_false_valid(socket):
    # exploit that relies on the knowing exactly how much to overflow
    socket.send((4 + 1 + 93 + 2) * b'0' + b'PING\r\n')
    assert socket.recv() == sstp(b'ERRO Malformed message.', 45)

def test_repeated_send_read(socket):
    for _ in range(10):
        socket.send(b'PING\r\n')
        assert socket.recv() == b'PONG\r\n'
