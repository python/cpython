# It's most useful to run these tests with ThreadSanitizer enabled.
import sys
import functools
import threading
import time
import unittest

from test.support import threading_helper


class TestBase(unittest.TestCase):
    pass


def do_race(func1, func2):
    """Run func1() and func2() repeatedly in separate threads."""
    n = 1000

    barrier = threading.Barrier(2)

    def repeat(func):
        barrier.wait()
        for _i in range(n):
            func()

    threads = [
        threading.Thread(target=functools.partial(repeat, func1)),
        threading.Thread(target=functools.partial(repeat, func2)),
    ]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()


@threading_helper.requires_working_threading()
class TestRaces(TestBase):
    def test_racing_cell_set(self):
        """Test cell object gettr/settr properties."""

        def nested_func():
            x = 0

            def inner():
                nonlocal x
                x += 1

        # This doesn't race because LOAD_DEREF and STORE_DEREF on the
        # cell object use critical sections.
        do_race(nested_func, nested_func)

        def nested_func2():
            x = 0

            def inner():
                y = x
                frame = sys._getframe(1)
                frame.f_locals["x"] = 2

            return inner

        def mutate_func2():
            inner = nested_func2()
            cell = inner.__closure__[0]
            old_value = cell.cell_contents
            cell.cell_contents = 1000
            time.sleep(0)
            cell.cell_contents = old_value
            time.sleep(0)

        # This revealed a race with cell_set_contents() since it was missing
        # the critical section.
        do_race(nested_func2, mutate_func2)

    def test_racing_cell_cmp_repr(self):
        """Test cell object compare and repr methods."""

        def nested_func():
            x = 0
            y = 0

            def inner():
                return x + y

            return inner.__closure__

        cell_a, cell_b = nested_func()

        def mutate():
            cell_a.cell_contents += 1

        def access():
            cell_a == cell_b
            s = repr(cell_a)

        # cell_richcompare() and cell_repr used to have data races
        do_race(mutate, access)

    def test_racing_load_super_attr(self):
        """Test (un)specialization of LOAD_SUPER_ATTR opcode."""

        class C:
            def __init__(self):
                try:
                    super().__init__
                    super().__init__()
                except RuntimeError:
                    pass  #  happens if __class__ is replaced with non-type

        def access():
            C()

        def mutate():
            # Swap out the super() global with a different one
            real_super = super
            globals()["super"] = lambda s=1: s
            time.sleep(0)
            globals()["super"] = real_super
            time.sleep(0)
            # Swap out the __class__ closure value with a non-type
            cell = C.__init__.__closure__[0]
            real_class = cell.cell_contents
            cell.cell_contents = 99
            time.sleep(0)
            cell.cell_contents = real_class

        # The initial PR adding specialized opcodes for LOAD_SUPER_ATTR
        # had some races (one with the super() global changing and one
        # with the cell binding being changed).
        do_race(access, mutate)


if __name__ == "__main__":
    unittest.main()
