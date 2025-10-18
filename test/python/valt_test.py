import unittest
import subprocess
import socket
import time
from functools import wraps


HOST = "127.0.0.1"
PORT = 9999
SERVER_EXECUTABLE_PATH = "build/valt"


def valt_test_class():
    def class_decorator(cls):
        def make_wrapper(attr):
            @wraps(attr)
            def wrapper(self, *args, **kwargs):
                with socket.create_connection((HOST, PORT)) as sock:
                    kwargs["sock"] = sock
                    try:
                        attr(self, *args, **kwargs)
                    finally:
                        sock.sendall(b"flush\n")
                        sock.recv(1024)

            return wrapper

        for name, attr in cls.__dict__.items():
            if callable(attr) and name.startswith("test"):
                setattr(cls, name, make_wrapper(attr))
        return cls

    return class_decorator


class ValtTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.proc = subprocess.Popen([SERVER_EXECUTABLE_PATH])
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
