"Test rpc, coverage 20%."

from idlelib import rpc
import threading
import unittest
from unittest import mock


class SocketIOTest(unittest.TestCase):

    def test_getresponse_interrupted(self):
        # gh-74112: an interrupted wait must release the lock and forget
        # the sequence number, so that a late response is discarded.
        sockio = rpc.SocketIO(mock.Mock(), debugging=False)
        sockio.sockthread = None  # Not the current thread.
        cvar = sockio.cvars[7] = threading.Condition()
        with mock.patch.object(cvar, 'wait', side_effect=KeyboardInterrupt):
            with self.assertRaises(KeyboardInterrupt):
                sockio._getresponse(7, 0.05)
        self.assertNotIn(7, sockio.cvars)
        self.assertTrue(cvar.acquire(blocking=False))
        cvar.release()


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
