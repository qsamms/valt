import time

from test.python.valt_test import ValtTestCase, valt_test_class


@valt_test_class()
class TestCommands(ValtTestCase):
    def test_set_get(self, sock):
        sock.sendall(b"set key 10\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"10\n")

    def test_setex(self, sock):
        sock.sendall(b"setex key 10 2\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"10\n")

        expired = False
        for i in range(10):
            sock.sendall(b"get key\n")
            reply = sock.recv(1024)
            if reply == b"ERR: key not found\n":
                expired = True
                break
            time.sleep(1)
        self.assertTrue(expired)

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

    def test_expire(self, sock):
        sock.sendall(b"set key 10\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"expire key 1\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        expired = False
        for i in range(10):
            sock.sendall(b"get key\n")
            reply = sock.recv(1024)
            if reply == b"ERR: key not found\n":
                expired = True
                break
            time.sleep(1)
        self.assertTrue(expired)

    def test_persist(self, sock):
        sock.sendall(b"set key 10\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"expire key 3\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"persist key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        time.sleep(3)
        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"10\n")

    def test_flush(self, sock):
        sock.sendall(b"set key 10\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"set key2 10\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"flush\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"OK\n")

        sock.sendall(b"get key\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"ERR: key not found\n")

        sock.sendall(b"get key2\n")
        reply = sock.recv(1024)
        self.assertEqual(reply, b"ERR: key not found\n")
