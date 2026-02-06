import unittest
import subprocess
import socket
import time
import struct


HOST = "127.0.0.1"
PORT = 9999
SERVER_EXECUTABLE_PATH = "build/valt"


class ValtTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.proc = subprocess.Popen([SERVER_EXECUTABLE_PATH])
        for _ in range(10):
            try:
                cls.sock = socket.create_connection((HOST, PORT), timeout=0.5)
                break
            except (ConnectionRefusedError, socket.timeout):
                time.sleep(1)
        else:
            cls.proc.kill()
            raise RuntimeError("Server failed to start")

    def tearDown(self):
        message = "flush".encode("utf-8")
        length = struct.pack(">I", len(message))
        self.sock.sendall(length + message)

    @classmethod
    def tearDownClass(cls):
        cls.proc.kill()
