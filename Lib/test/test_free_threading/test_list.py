import threading
import unittest

from threading import Thread, Barrier
from unittest import TestCase

from test.support import threading_helper


NTHREAD = 10
OBJECT_COUNT = 5_000


class C:
    def __init__(self, v):
        self.v = v


@threading_helper.requires_working_threading()
class TestList(TestCase):
    def test_racing_iter_append(self):
        l = []

        barrier = Barrier(NTHREAD + 1)
        def writer_func(l):
            barrier.wait()
            for i in range(OBJECT_COUNT):
                l.append(C(i + OBJECT_COUNT))

        def reader_func(l):
            barrier.wait()
            while True:
                count = len(l)
                for i, x in enumerate(l):
                    self.assertEqual(x.v, i + OBJECT_COUNT)
                if count == OBJECT_COUNT:
                    break

        writer = Thread(target=writer_func, args=(l,))
        readers = []
        for x in range(NTHREAD):
            reader = Thread(target=reader_func, args=(l,))
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_racing_iter_extend(self):
        l = []

        barrier = Barrier(NTHREAD + 1)
        def writer_func():
            barrier.wait()
            for i in range(OBJECT_COUNT):
                l.extend([C(i + OBJECT_COUNT)])

        def reader_func():
            barrier.wait()
            while True:
                count = len(l)
                for i, x in enumerate(l):
                    self.assertEqual(x.v, i + OBJECT_COUNT)
                if count == OBJECT_COUNT:
                    break

        writer = Thread(target=writer_func)
        readers = []
        for x in range(NTHREAD):
            reader = Thread(target=reader_func)
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_store_list_int(self):
        def copy_back_and_forth(b, l):
            b.wait()
            for _ in range(100):
                l[0] = l[1]
                l[1] = l[0]

        l = [0, 1]
        barrier = Barrier(NTHREAD)
        threads = [Thread(target=copy_back_and_forth, args=(barrier, l))
                   for _ in range(NTHREAD)]
        with threading_helper.start_threads(threads):
            pass

    def test_reverse(self):
        def reverse_list(b, l):
            b.wait()
            for _ in range(100):
                l.reverse()

        def reader_list(b, l):
            b.wait()
            for _ in range(100):
                for i in range(10):
                    self.assertTrue(0 <= l[i] < 10)

        l = list(range(10))
        barrier = Barrier(2)
        threads = [Thread(target=reverse_list, args=(barrier, l)),
                   Thread(target=reader_list, args=(barrier, l))]
        with threading_helper.start_threads(threads):
            pass

    def test_slice_assignment1(self):
        def assign_slice(b, l):
            b.wait()
            for _ in range(100):
                l[2:5] = [7, 8, 9]

        def reader_list(b, l):
            b.wait()
            for _ in range(100):
                self.assertIn(l[2], (2, 7))
                self.assertIn(l[3], (3, 8))
                self.assertIn(l[4], (4, 9))

        l = list(range(10))
        barrier = Barrier(2)
        threads = [Thread(target=assign_slice, args=(barrier, l)),
                   Thread(target=reader_list, args=(barrier, l))]
        with threading_helper.start_threads(threads):
            pass

    def test_slice_assignment2(self):
        def assign_slice(b, l):
            b.wait()
            for _ in range(100):
                l[::2] = [10, 11, 12, 13, 14]

        def reader_list(b, l):
            b.wait()
            for _ in range(100):
                for i in range(0, 10, 2):
                    self.assertIn(l[i], (i, 10 + i // 2))

        l = list(range(10))
        barrier = Barrier(2)
        threads = [Thread(target=assign_slice, args=(barrier, l)),
                   Thread(target=reader_list, args=(barrier, l))]
        with threading_helper.start_threads(threads):
            pass

    # gh-145036: race condition with list.__sizeof__()
    def test_list_sizeof_free_threaded_build(self):
        L = []

        def mutate_function():
            for _ in range(100):
                L.append(1)
                L.pop()

        def size_function():
            for _ in range(100):
                L.__sizeof__()

        threads = []
        for _ in range(4):
            threads.append(Thread(target=mutate_function))
            threads.append(Thread(target=size_function))

        with threading_helper.start_threads(threads):
            pass


    def test_richcompare_stale_element_list1(self) -> None:
        # gh-148442: list_richcompare_impl must keep references to the
        # captured items for the final ordering comparison, not re-read the
        # list slots after the critical section may have been suspended.
        #
        # list1 = [x, 1], list2 = [0, 0] with x > 0.
        # During list1 > list2, list1[0] is mutated from x to 0.
        # The result must be True (x > 0), not False (0 > 0).

        class Value:
            def __init__(self, v: int) -> None:
                self.v = v

            def __eq__(self, other: object) -> bool:
                if isinstance(other, Value):
                    return self.v == other.v
                return NotImplemented

            def __gt__(self, other: object) -> bool:
                if isinstance(other, Value):
                    return self.v > other.v
                return NotImplemented

            def __hash__(self) -> int:
                return hash(self.v)

        eq_started = threading.Event()
        swap_done = threading.Event()

        class SignalingValue(Value):
            # Value whose __eq__ signals eq_started then waits for swap_done,
            # giving another thread the window to mutate the list slot.
            def __eq__(self, other: object) -> bool:
                eq_started.set()
                swap_done.wait()
                return super().__eq__(other)

        x = SignalingValue(5)  # list1[0]; x > 0
        list1: list[Value] = [x, Value(1)]
        list2: list[Value] = [Value(0), Value(0)]

        result: list[bool] = []

        def compare() -> None:
            result.append(list1 > list2)

        def swap() -> None:
            eq_started.wait()
            list1[0] = Value(0)  # replace x(5) with 0 -- must not change result
            swap_done.set()

        t_cmp = Thread(target=compare)
        t_swp = Thread(target=swap)
        t_cmp.start()
        t_swp.start()
        t_cmp.join()
        t_swp.join()

        # x(5) > Value(0) is True; old code would compare Value(0) > Value(0) -> False
        self.assertTrue(result[0])

    def test_richcompare_stale_element_list2(self) -> None:
        # Same as test_richcompare_stale_element_list1 but list2[0] is mutated
        # to a value larger than x, which would flip the result under the old code.
        #
        # list1 = [x, 1], list2 = [0, 0] with x > 0.
        # During list1 > list2, list2[0] is mutated from 0 to 100 (> x).
        # The result must be True (x > 0), not False (x > 100).

        class Value:
            def __init__(self, v: int) -> None:
                self.v = v

            def __eq__(self, other: object) -> bool:
                if isinstance(other, Value):
                    return self.v == other.v
                return NotImplemented

            def __gt__(self, other: object) -> bool:
                if isinstance(other, Value):
                    return self.v > other.v
                return NotImplemented

            def __hash__(self) -> int:
                return hash(self.v)

        eq_started = threading.Event()
        swap_done = threading.Event()

        class SignalingValue(Value):
            def __eq__(self, other: object) -> bool:
                eq_started.set()
                swap_done.wait()
                return super().__eq__(other)

        x = SignalingValue(5)  # list1[0]; x > 0
        list1: list[Value] = [x, Value(1)]
        list2: list[Value] = [Value(0), Value(0)]

        result: list[bool] = []

        def compare() -> None:
            result.append(list1 > list2)

        def swap() -> None:
            eq_started.wait()
            list2[0] = Value(100)  # replace 0 with 100 (> x) -- must not change result
            swap_done.set()

        t_cmp = Thread(target=compare)
        t_swp = Thread(target=swap)
        t_cmp.start()
        t_swp.start()
        t_cmp.join()
        t_swp.join()

        # x(5) > Value(0) is True; old code would compare Value(5) > Value(100) -> False
        self.assertTrue(result[0])


if __name__ == "__main__":
    unittest.main()
