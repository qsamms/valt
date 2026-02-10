import time
import struct

from test.python.valt_test import ValtTestCase


class TestDB(ValtTestCase): 
    def test_set_get(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10")

    def test_setex(self):
        self.sendall("setex key 10 2")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10")

        expired = False
        for _ in range(10):
            self.sendall("get key")
            reply = self.recv_str()
            if reply == "ERR_KEY_NOT_FOUND":
                expired = True
                break
            time.sleep(1)
        self.assertTrue(expired)

    def test_split_segments(self):
        entire_message = "set key 10"
        length_bytes = struct.pack(">I", len(entire_message))

        self.sock.setblocking(False)
        self.sock.sendall(length_bytes + "set ".encode('utf-8'))
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
        self.assertEqual(reply, "OK")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10")

    def test_delete(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10")

        self.sendall("delete key")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "ERR_KEY_NOT_FOUND")

    def test_expire(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("expire key 1")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        expired = False
        for _ in range(10):
            self.sendall("get key")
            reply = self.recv_str()
            if reply == "ERR_KEY_NOT_FOUND":
                expired = True
                break
            time.sleep(1)
        self.assertTrue(expired)

    def test_persist(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("expire key 3")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("persist key")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        time.sleep(3)
        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "10")

    def test_flush(self):
        self.sendall("set key 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("set key2 10")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("flush")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        self.sendall("get key")
        reply = self.recv_str()
        self.assertEqual(reply, "ERR_KEY_NOT_FOUND")

        self.sendall("get key2")
        reply = self.recv_str()
        self.assertEqual(reply, "ERR_KEY_NOT_FOUND")

