import pickle
import threading
import unittest

from test.support import threading_helper

NTHREADS = 8

# Fresh objects expose one-time races under ThreadSanitizer.
ROUNDS = 20
ITERS = 20

HASH_DATA_TEMPLATE = bytes(range(256))
HASH_DATA_REPEAT = 256

READ_OPS = [
    lambda mv: mv.tobytes(),
    lambda mv: mv.hex(),
    lambda mv: mv.tolist(),
    lambda mv: mv.cast("B"),
    lambda mv: mv.toreadonly(),
    lambda mv: memoryview(mv),
    lambda mv: bytes(mv),
    lambda mv: pickle.PickleBuffer(mv).release(),
    lambda mv: mv.obj,
    lambda mv: mv.format,
    lambda mv: mv.c_contiguous,
    lambda mv: mv[0],
    lambda mv: mv[0:4],
    lambda mv: mv.count(0),
    lambda mv: mv.index(0),
    lambda mv: mv == mv,
    lambda mv: list(mv),
    lambda mv: len(mv),
]


def run_racy(func, *args):
    try:
        func(*args)
    except (ValueError, BufferError):
        pass


@threading_helper.requires_working_threading()
class TestMemoryViewRaces(unittest.TestCase):
    def assert_exporter_free(self, buf):
        buf.append(0)
        del buf[-1]

    def test_concurrent_slicing_keeps_export_count(self):
        for _ in range(ROUNDS):
            mv = memoryview(bytes(64))
            slices = []
            lock = threading.Lock()

            def make_slices():
                local = [mv[0:4] for _ in range(ITERS)]
                local += [memoryview(mv) for _ in range(ITERS)]
                with lock:
                    slices.extend(local)

            threading_helper.run_concurrently(make_slices, nthreads=NTHREADS)
            del slices

            self.assertEqual(bytes(mv[0:4]), b"\x00" * 4)

    def test_concurrent_release(self):
        buf = bytearray(64)

        for _ in range(ROUNDS):
            views = [memoryview(buf) for _ in range(NTHREADS)]

            def release(views=views):
                for mv in views:
                    run_racy(mv.release)

            threading_helper.run_concurrently(release, nthreads=NTHREADS)

        self.assert_exporter_free(buf)

    def test_release_races_with_reads(self):
        for _ in range(ROUNDS):
            buf = bytearray(64)
            cell = [memoryview(buf)]
            lock = threading.Lock()

            def releaser():
                for _ in range(ITERS):
                    mv = cell[0]
                    run_racy(mv.release)
                    with lock:
                        cell[0] = memoryview(buf)

            def reader():
                for _ in range(ITERS):
                    mv = cell[0]
                    for op in READ_OPS:
                        run_racy(op, mv)

            threading_helper.run_concurrently(
                [releaser] * (NTHREADS // 2) + [reader] * (NTHREADS // 2),
                nthreads=NTHREADS,
            )

            cell[0].release()
            self.assert_exporter_free(buf)

    def test_read_keeps_exporter_alive_after_release(self):
        size = 1 << 20
        for _ in range(ROUNDS):
            exporter = [bytearray(size)]
            view = memoryview(exporter[0])
            stale = []

            def reader():
                try:
                    for _ in range(4):
                        data = view.tobytes()
                        if data.count(0) != len(data):
                            stale.append(True)
                            return
                except ValueError:
                    pass

            def releaser():
                view.release()
                exporter.clear()
                for _ in range(8):
                    bytearray(b"\xdb" * size)

            threading_helper.run_concurrently([reader, releaser])
            self.assertFalse(stale)

    def test_release_races_with_writes(self):
        for _ in range(ROUNDS):
            buf = bytearray(8 * NTHREADS)
            cell = [memoryview(buf)]
            lock = threading.Lock()

            def releaser():
                for _ in range(ITERS):
                    mv = cell[0]
                    run_racy(mv.release)
                    with lock:
                        cell[0] = memoryview(buf)

            def writer(slot):
                start = slot * 8
                for _ in range(ITERS):
                    mv = cell[0]
                    run_racy(mv.__setitem__, start, 1)
                    run_racy(mv.__setitem__, slice(start, start + 4), b"abcd")

            workers = [releaser] * (NTHREADS // 2)
            workers += [lambda s=s: writer(s) for s in range(NTHREADS // 2)]
            threading_helper.run_concurrently(workers, nthreads=NTHREADS)

            cell[0].release()
            self.assert_exporter_free(buf)

    def test_release_races_with_buffer_exports(self):
        for _ in range(ROUNDS):
            buf = bytearray(64)
            mv = memoryview(buf)

            def exporter():
                for _ in range(ITERS):
                    run_racy(lambda: pickle.PickleBuffer(mv).release())

            def releaser():
                for _ in range(ITERS):
                    run_racy(mv.release)

            threading_helper.run_concurrently(
                [exporter] * (NTHREADS - 1) + [releaser], nthreads=NTHREADS
            )

            mv.release()
            self.assert_exporter_free(buf)

    def test_release_with_live_export(self):
        buf = bytearray(64)

        for _ in range(ROUNDS):
            mv = memoryview(buf)
            held = pickle.PickleBuffer(mv)

            def release(mv=mv):
                try:
                    mv.release()
                except BufferError:
                    pass

            threading_helper.run_concurrently(release, nthreads=NTHREADS)

            self.assertEqual(bytes(mv[0:4]), b"\x00" * 4)
            held.release()
            mv.release()

        self.assert_exporter_free(buf)

    def test_compare_two_views_races_with_release(self):
        for _ in range(ROUNDS):
            buf = bytearray(64)
            cell = [memoryview(buf), memoryview(buf)]
            lock = threading.Lock()

            def releaser(slot):
                for _ in range(ITERS):
                    mv = cell[slot]
                    run_racy(mv.release)
                    with lock:
                        cell[slot] = memoryview(buf)

            def comparer():
                for _ in range(ITERS):
                    left, right = cell[0], cell[1]
                    run_racy(lambda: left == right)
                    run_racy(lambda: left != right)

            threading_helper.run_concurrently(
                [lambda: releaser(0), lambda: releaser(1)] + [comparer] * 6,
                nthreads=NTHREADS,
            )

            for mv in cell:
                mv.release()
            self.assert_exporter_free(buf)

    def test_release_parent_keeps_child_valid(self):
        for _ in range(ROUNDS):
            buf = bytearray(range(64))
            parent = memoryview(buf)
            child = parent[0:32]

            def use_child():
                for _ in range(ITERS):
                    self.assertEqual(child[0], 0)
                    child.tobytes()

            def release_parent():
                parent.release()

            threading_helper.run_concurrently(
                [release_parent] + [use_child] * (NTHREADS - 1),
                nthreads=NTHREADS,
            )

            self.assertEqual(child.tobytes(), bytes(range(32)))
            child.release()
            self.assert_exporter_free(buf)

    def test_concurrent_iteration(self):
        for _ in range(ROUNDS):
            buf = bytearray(range(64))
            cell = [memoryview(buf)]
            lock = threading.Lock()

            def releaser():
                for _ in range(ITERS):
                    mv = cell[0]
                    run_racy(mv.release)
                    with lock:
                        cell[0] = memoryview(buf)

            def iterator():
                for _ in range(ITERS):
                    run_racy(list, cell[0])

            threading_helper.run_concurrently(
                [releaser] * (NTHREADS // 2) + [iterator] * (NTHREADS // 2),
                nthreads=NTHREADS,
            )

            cell[0].release()
            self.assert_exporter_free(buf)

    def test_iterator_exhaustion_drops_last_reference(self):
        def loop():
            for _ in range(ROUNDS * ITERS):
                self.assertEqual(list(iter(memoryview(b"ab"))), [97, 98])

        threading_helper.run_concurrently(loop, nthreads=NTHREADS)

    def test_concurrent_hash(self):
        for _ in range(ROUNDS):
            data = HASH_DATA_TEMPLATE * HASH_DATA_REPEAT
            mv = memoryview(data)
            results = []
            lock = threading.Lock()

            def hasher():
                local = {hash(mv) for _ in range(ITERS)}
                with lock:
                    results.append(local)

            threading_helper.run_concurrently(hasher, nthreads=NTHREADS)
            self.assertEqual({h for s in results for h in s}, {hash(data)})

    def test_concurrent_hash_and_release(self):
        for _ in range(ROUNDS):
            mv = memoryview(HASH_DATA_TEMPLATE * HASH_DATA_REPEAT)

            def work(mv=mv):
                run_racy(hash, mv)
                run_racy(mv.release)

            threading_helper.run_concurrently(work, nthreads=NTHREADS)


if __name__ == "__main__":
    unittest.main()
