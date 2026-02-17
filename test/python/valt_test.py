import unittest
import subprocess
import socket
import time
import struct


HOST = "127.0.0.1"
PORT = 1738
SERVER_EXECUTABLE_PATH = "build/valt"


class ValtTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tempsockets = set()
        try:
            cls.attemptConnection()
        except RuntimeError:
            cls.proc = subprocess.Popen([SERVER_EXECUTABLE_PATH])
            cls.attemptConnection()

        
    @classmethod
    def attemptConnection(cls):
        for _ in range(10):
            try:
                with socket.create_connection((HOST, PORT), timeout=0.5):
                    break
            except (ConnectionRefusedError, socket.timeout):
                time.sleep(1)
        else:
            raise RuntimeError("Server failed to start")
        
    def getSocket(self):
        sock = socket.create_connection((HOST, PORT), timeout=0.5)
        self.tempsockets.add(sock)
        return sock

    def sendall(self, message: str, sock=None, includePrefix=True):
        if sock is None:
            sock = self.sock
        message_bytes = message.encode("utf-8")
        length_bytes = struct.pack(">I", len(message_bytes))
        sock.sendall(length_bytes + message_bytes if includePrefix else message_bytes)

    def recv_str(self, sock=None) -> str:
        if sock is None:
            sock = self.sock

        length_bytes = b""
        while len(length_bytes) < 4:
            chunk = sock.recv(4 - len(length_bytes))
            if not chunk:
                raise ConnectionError("Socket closed while reading length prefix")
            length_bytes += chunk

        message_length = struct.unpack(">I", length_bytes)[0]

        data = b""
        while len(data) < message_length:
            chunk = sock.recv(message_length - len(data))
            if not chunk:
                raise ConnectionError("Socket closed while reading message")
            data += chunk

        return data.decode("utf-8")
    
    def setUp(self):
        self.sock = socket.create_connection((HOST, PORT), timeout=0.5)

    def tearDown(self):
        self.sendall("flush")
        self.recv_str()
        self.sock.close()
        for sock in self.tempsockets:
            sock.close()
        self.tempsockets.clear()

    @classmethod
    def tearDownClass(cls):
        if getattr(cls, "proc", None):
            cls.proc.kill()
