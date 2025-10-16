import unittest
import socket
import subprocess
import time

HOST = "127.0.0.1"
PORT = 9999
SERVER_EXECUTABLE_PATH = "build/valt"


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

    def test_set_get(self):
        with socket.create_connection((HOST, PORT)) as s:
            s.sendall(b"set key 10\n")
            reply = s.recv(1024)
            self.assertEqual(reply, b"OK\n")

            s.sendall(b"get key\n")
            reply = s.recv(1024)
            self.assertEqual(reply, b"10\n")

    def test_split_segments(self):
        with socket.create_connection((HOST, PORT)) as s:
            s.setblocking(False)
            s.sendall(b"set ")
            with self.assertRaises(BlockingIOError):
                s.recv(1024)
            time.sleep(1)  # wait between to ensure segments don't get grouped
            s.sendall(b"key")
            with self.assertRaises(BlockingIOError):
                s.recv(1024)
            time.sleep(1)
            s.sendall(b" 10\n")
            s.setblocking(True)
            reply = s.recv(1024)
            self.assertEqual(reply, b"OK\n")

            s.sendall(b"get key\n")
            reply = s.recv(1024)
            self.assertEqual(reply, b"10\n")
