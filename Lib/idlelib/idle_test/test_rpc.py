"Test rpc, coverage 20%."

from idlelib import rpc
import socket
import struct
import unittest


class SocketIOTest(unittest.TestCase):

    def test_reconnect_discards_partial_packet(self):
        # gh-89544: RPCClient.accept() reinitializes the SocketIO after
        # a restart; a partially received packet must not be kept.
        old_sock, old_peer = socket.socketpair()
        new_sock, new_peer = socket.socketpair()
        with old_sock, old_peer, new_sock, new_peer:
            sockio = rpc.SocketIO(old_sock)
            old_peer.sendall(struct.pack('<i', 100) + b'x' * 10)
            self.assertIsNone(sockio.pollpacket(1))
            sockio.close()
            rpc.SocketIO.__init__(sockio, new_sock)
            new_peer.sendall(struct.pack('<i', 3) + b'abc')
            self.assertEqual(sockio.pollpacket(1), b'abc')



class CodePicklerTest(unittest.TestCase):

    def test_pickle_unpickle(self):
        def f(): return a + b + c
        func, (cbytes,) = rpc.pickle_code(f.__code__)
        self.assertIs(func, rpc.unpickle_code)
        self.assertIn(b'test_rpc.py', cbytes)
        code = rpc.unpickle_code(cbytes)
        self.assertEqual(code.co_names, ('a', 'b', 'c'))

    def test_code_pickler(self):
        self.assertIn(type((lambda:None).__code__),
                      rpc.CodePickler.dispatch_table)

    def test_dumps(self):
        def f(): pass
        # The main test here is that pickling code does not raise.
        self.assertIn(b'test_rpc.py', rpc.dumps(f.__code__))


if __name__ == '__main__':
    unittest.main(verbosity=2)
