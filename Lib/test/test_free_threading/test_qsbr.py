import threading
import unittest

from test.support import threading_helper


class Holder:
    pass


def make_shared(func, *args):
    t = threading.Thread(target=func, args=args)
    t.start()
    t.join()


@threading_helper.requires_working_threading()
class TestDelayedFreeReentrancy(unittest.TestCase):
    ROUNDS = 1000

    def queue_finalizers(self, callback):
        class Finalizer:
            def __del__(self):
                callback()

        holders = [Holder() for _ in range(self.ROUNDS)]
        for h in holders:
            h.__dict__ = {"f": Finalizer()}
        return holders

    def test_dict_resize(self):
        d = {}
        make_shared(d.get, 0)
        holders = self.queue_finalizers(lambda: d.__setitem__(object(), 0))
        for n, h in enumerate(holders):
            h.__dict__ = {}
            if n % 2:
                h.__dict__ = {}
            for k in list(d):
                del d[k]
            for i in range(6):
                d[i] = i
            self.assertEqual(len(d), len(list(d)))

    def test_list_resize(self):
        L = []
        make_shared(L.append, 0)
        holders = self.queue_finalizers(L.clear)
        for n, h in enumerate(holders):
            h.__dict__ = {}
            if n % 2:
                h.__dict__ = {}
            L.clear()
            for i in range(9):
                L.insert(len(L), i)
            self.assertEqual(len(L), len(list(L)))

    def test_type_cache_invalidation(self):
        class C:
            x = None

        holders = self.queue_finalizers(lambda: C.x)
        for h in holders:
            h.__dict__ = {}
            C.x
            new = object()
            C.x = new
            self.assertIs(C.x, new)


if __name__ == "__main__":
    unittest.main()
