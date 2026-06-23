import gc
import unittest
from threading import Event, Thread

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestFrozenDict(unittest.TestCase):
    def test_racing_reads_during_fromkeys(self):
        # gh-151722: frozendict.fromkeys() builds its result from a generic
        # iterable in a fill loop; the result must not be GC-tracked until it
        # is fully populated, otherwise a half-built instance is observable
        # from other threads.  Reading it with len()/repr()/hash() must not
        # race with the fill-time table and ma_used writes.
        NUM_KEYS = 5000
        NUM_ROUNDS = 20
        SENTINEL = "test_racing_reads_during_fromkeys_0"

        empty = frozendict()
        latest = [empty]  # main -> reader handoff, never empty
        done = Event()
        errors = []

        def find_half_built():
            for obj in gc.get_objects():
                if (isinstance(obj, frozendict)
                        and obj is not empty
                        and 0 < len(obj) < NUM_KEYS
                        and SENTINEL in obj):
                    return obj
            return None

        class EvilIter:
            def __iter__(self):
                yield SENTINEL
                for i in range(1, NUM_KEYS):
                    if (i & 0x3FF) == 0 and latest[0] is empty:
                        obj = find_half_built()
                        if obj is not None:
                            latest[0] = obj  # leak the half-built object
                    yield f"k{i}"

        def reader():
            while not done.is_set():
                fd = latest[0]
                try:
                    len(fd)
                    repr(fd)
                    hash(fd)
                except Exception as exc:
                    errors.append(exc)

        readers = [Thread(target=reader) for _ in range(5)]
        for t in readers:
            t.start()
        try:
            for _ in range(NUM_ROUNDS):
                latest[0] = empty
                frozendict.fromkeys(EvilIter(), 0)
        finally:
            done.set()
            for t in readers:
                t.join()
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
