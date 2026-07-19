import struct
import threading
import unittest
from test.support import threading_helper


threading_helper.requires_working_threading(module=True)


class TestStructIterUnpack(unittest.TestCase):
    def test_shared_iteration_is_exactly_once(self):
        # Every record must be handed to exactly one thread: no crash, no
        # duplicated record, and no skipped record.
        nthreads = 8
        nrecords = 100_000
        s = struct.Struct("i")
        data = b"".join(s.pack(i) for i in range(nrecords))

        for _ in range(5):
            it = s.iter_unpack(data)
            collected = []
            lock = threading.Lock()

            def worker():
                local = [value for (value,) in it]
                with lock:
                    collected.extend(local)

            threading_helper.run_concurrently(
                worker, nthreads=nthreads
            )

            self.assertEqual(len(collected), nrecords)
            self.assertEqual(sorted(collected), list(range(nrecords)))

if __name__ == "__main__":
    unittest.main()
