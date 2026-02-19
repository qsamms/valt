from test.python.valt_test import ValtTestCase


class TestQueues(ValtTestCase):
    def test_queue_pub_sub(self):
        self.sendall("create_queue myqueue")
        reply = self.recv_str()
        self.assertEqual(reply, "OK")

        sock2 = self.getSocket()
        self.sendall("subscribe myqueue", sock=sock2)
        self.assertEqual(self.recv_str(sock=sock2), "OK")

        sock3 = self.getSocket()
        self.sendall("subscribe myqueue", sock=sock3)
        self.assertEqual(self.recv_str(sock=sock3), "OK")

        sock4 = self.getSocket()
        self.sendall("publish myqueue hello", sock=sock4)
        self.assertEqual(self.recv_str(sock=sock4), "OK")

        reply2 = self.recv_str(sock=sock2)
        reply3 = self.recv_str(sock=sock3)
        self.assertEqual(reply2, "hello")
        self.assertEqual(reply3, "hello")
