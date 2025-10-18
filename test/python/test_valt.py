import unittest
import socket
import subprocess
import time
from test.python.utils import valt_test_class

HOST = "127.0.0.1"
PORT = 9999
SERVER_EXECUTABLE_PATH = "build/valt"

@valt_test_class(HOST, PORT)
class TestValt(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.valt_out = open("valt.out", "w")
        self.proc = subprocess.Popen(
            [SERVER_EXECUTABLE_PATH],
            stdout=self.valt_out,
            stderr=subprocess.STDOUT,
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
        self.valt_out.close()

    def setUp(self):
        self.valt_out.write(
            f"\nTest: {self._testMethodName} ---------------------------\n"
        )
        self.valt_out.flush()

    def test_set_get(self, sock):
        sock.sendall(b"set key 10\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"10\n")

    def test_split_segments(self, sock):
        sock.setblocking(False)
        sock.sendall(b"set ")
        with self.assertRaises(BlockingIOError):
            sock.recv(1024)
        time.sleep(1)  # wait between to ensure segments don't get grouped
        sock.sendall(b"key")
        with self.assertRaises(BlockingIOError):
            sock.recv(1024)
        time.sleep(1)
        sock.sendall(b" 10\n")
        sock.setblocking(True)
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"10\n")

    def test_delete(self, sock):
        sock.sendall(b"set key 10\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"10\n")

        sock.sendall(b"delete key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"ERR: key not found\n")
