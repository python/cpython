import threading
import unittest

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestMemoryViewSliceRace(unittest.TestCase):
    def test_concurrent_slicing_keeps_export_count(self):
        # gh-155606: slicing registers a new view on the shared managed buffer,
        # and mbuf_add_view() bumped that buffer's export count with a plain
        # ++.  Concurrent slices of a single memoryview therefore lost
        # increments, the count reached zero while views were still alive, and
        # the underlying buffer was released early.
        #
        # The slices are created concurrently but only dropped afterwards, on
        # one thread, so this covers the increment on its own.
        mv = memoryview(bytes(2 ** 16))
        slices = []
        lock = threading.Lock()

        def make_slices():
            local = [mv[0:64] for _ in range(2000)]
            with lock:
                slices.extend(local)

        threading_helper.run_concurrently(make_slices, nthreads=8)
        del slices

        # An early release makes this raise "operation forbidden on released
        # memoryview object".
        self.assertEqual(bytes(mv[0:4]), b"\x00" * 4)


if __name__ == "__main__":
    unittest.main()
