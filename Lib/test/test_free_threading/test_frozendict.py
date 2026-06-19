import gc
import unittest
from threading import Event, Thread

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestFrozenDict(unittest.TestCase):
    def test_racing_reads_during_construction(self):
        # gh-151722: a frozendict is GC-tracked before it is fully
        # populated, so a half-built instance is observable from other
        # threads.  Reading it with len()/repr()/hash() must not race
        # with the construction-time table and ma_used writes.
        NUM_KEYS = 8192
        NUM_ROUNDS = 40

        latest = [frozendict()]  # main -> reader handoff, never empty
        done = Event()

        class Evil:
            def keys(self):
                return [f"k{i}" for i in range(NUM_KEYS)]

            def __getitem__(self, key):
                if latest[0] is empty:
                    for obj in gc.get_objects():
                        if (isinstance(obj, frozendict)
                                and 0 < len(obj) < NUM_KEYS):
                            latest[0] = obj  # leak the half-built object
                            break
                return 1

        empty = latest[0]

        def reader():
            while not done.is_set():
                fd = latest[0]
                len(fd)
                repr(fd)
                hash(fd)

        readers = [Thread(target=reader) for _ in range(3)]
        for t in readers:
            t.start()
        try:
            for _ in range(NUM_ROUNDS):
                latest[0] = empty
                frozendict(Evil())
        finally:
            done.set()
            for t in readers:
                t.join()


if __name__ == "__main__":
    unittest.main()
