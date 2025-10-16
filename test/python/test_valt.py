import unittest
import socket
import subprocess
import time
import sys

HOST = "127.0.0.1"
PORT = 9999
SERVER_EXECUTABLE_PATH = "build/valt"


class TestValt(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.proc = subprocess.Popen(
            [SERVER_EXECUTABLE_PATH],
            stdout=sys.stdout,
            stderr=sys.stderr,
        )
        for _ in range(10):
            try:
                with socket.create_connection((HOST, PORT), timeout=0.5):
                    break
            except (ConnectionRefusedError, socket.timeout):
                time.sleep(1)
        else:
            self.proc.kill()
            raise RuntimeError("Server failed to start")

    @classmethod
    def tearDownClass(self):
        self.proc.kill()

    def test_set_get(self):
        with socket.create_connection((HOST, PORT)) as s:
            s.sendall(b"set key 10\n")
            reply = s.recv(1024)
            self.assertEqual(reply, b"OK\n")

            s.sendall(b"get key\n")
            reply = s.recv(1024)
            self.assertEqual(reply, b"10\n")

    def test_set_broken_up(self):
        with socket.create_connection((HOST, PORT)) as s:
            s.setblocking(False)
            s.sendall(b"set ")
            with self.assertRaises(BlockingIOError):
                s.recv(1024)
            s.sendall(b"key")
            with self.assertRaises(BlockingIOError):
                s.recv(1024)
            s.sendall(b" 10\n")
            s.setblocking(True)
            reply = s.recv(1024)
            self.assertEqual(reply, b"OK\n")
