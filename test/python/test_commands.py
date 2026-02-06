import time
import struct

from test.python.valt_test import ValtTestCase


class TestCommands(ValtTestCase):
    def sendall(self, message: str, includePrefix=True):
        message_bytes = message.encode("utf-8")
        length_bytes = struct.pack(">I", len(message_bytes))
        to_send_message = None
        if includePrefix:
            to_send_message = length_bytes + message_bytes
        else:
            to_send_message = message_bytes
        self.sock.sendall(to_send_message)

    def recv_str(self, bufsize=1024) -> str:
        data = self.sock.recv(bufsize)
        return data.decode("utf-8")

    def test_set_get(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10\n")

    def test_setex(self):
        self.sendall("setex key 10 2")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10\n")

        expired = False
        for _ in range(10):
            self.sendall("get key")
            reply = self.recv_str()
            if reply == "ERR: key not found\n":
                expired = True
                break
            time.sleep(1)
        self.assertTrue(expired)

    def test_split_segments(self):
        self.sock.setblocking(False)
        self.sendall("set ")
        with self.assertRaises(BlockingIOError):
            self.recv_str()

        time.sleep(1)
        self.sendall("key", includePrefix=False)
        with self.assertRaises(BlockingIOError):
            self.recv_str()

        time.sleep(1)
        self.sock.setblocking(True)
        self.sendall(" 10", includePrefix=False)
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10\n")

    def test_delete(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10\n")

        self.sendall("delete key")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "ERR: key not found\n")

    def test_expire(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("expire key 1")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        expired = False
        for _ in range(10):
            self.sendall("get key")
            reply = self.recv_str()
            if reply == "ERR: key not found\n":
                expired = True
                break
            time.sleep(1)
        self.assertTrue(expired)

    def test_persist(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("expire key 3")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("persist key")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        time.sleep(3)
        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10\n")

    def test_flush(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("set key2 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("flush")
        reply = self.recv_str()
        self.assertEqual(reply, "OK\n")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "ERR: key not found\n")

        self.sendall("get key2")
        reply = self.recv_str()
        self.assertEqual(reply, "ERR: key not found\n")

