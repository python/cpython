# It's most useful to run these tests with ThreadSanitizer enabled.
import sys
import functools
import threading
import time
import unittest
import _testinternalcapi
import warnings

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

    def test_racing_to_bool(self):

        seq = [1]

        class C:
            def __bool__(self):
                return False

        def access():
            if seq:
                return 1
            else:
                return 2

        def mutate():
            nonlocal seq
            seq = [1]
            time.sleep(0)
            seq = C()
            time.sleep(0)

        do_race(access, mutate)

    def test_racing_store_attr_slot(self):
        class C:
            __slots__ = ['x', '__dict__']

        c = C()

        def set_slot():
            for i in range(10):
                c.x = i
            time.sleep(0)

        def change_type():
            def set_x(self, x):
                pass

            def get_x(self):
                pass

            C.x = property(get_x, set_x)
            time.sleep(0)
            del C.x
            time.sleep(0)

        do_race(set_slot, change_type)

        def set_getattribute():
            C.__getattribute__ = lambda self, x: x
            time.sleep(0)
            del C.__getattribute__
            time.sleep(0)

        do_race(set_slot, set_getattribute)

    def test_racing_store_attr_instance_value(self):
        class C:
            pass

        c = C()

        def set_value():
            for i in range(100):
                c.x = i

        set_value()

        def read():
            x = c.x

        def mutate():
            # Adding a property for 'x' should unspecialize it.
            C.x = property(lambda self: None, lambda self, x: None)
            time.sleep(0)
            del C.x
            time.sleep(0)

        do_race(read, mutate)

    def test_racing_store_attr_with_hint(self):
        class C:
            pass

        c = C()
        for i in range(29):
            setattr(c, f"_{i}", None)

        def set_value():
            for i in range(100):
                c.x = i

        set_value()

        def read():
            x = c.x

        def mutate():
            # Adding a property for 'x' should unspecialize it.
            C.x = property(lambda self: None, lambda self, x: None)
            time.sleep(0)
            del C.x
            time.sleep(0)

        do_race(read, mutate)

    def make_shared_key_dict(self):
        class C:
            pass

        a = C()
        a.x = 1
        return a.__dict__

    def test_racing_store_attr_dict(self):
        """Test STORE_ATTR with various dictionary types."""
        class C:
            pass

        c = C()

        def set_value():
            for i in range(20):
                c.x = i

        def mutate():
            nonlocal c
            c.x = 1
            self.assertTrue(_testinternalcapi.has_inline_values(c))
            for i in range(30):
                setattr(c, f"_{i}", None)
            self.assertFalse(_testinternalcapi.has_inline_values(c.__dict__))
            c.__dict__ = self.make_shared_key_dict()
            self.assertTrue(_testinternalcapi.has_split_table(c.__dict__))
            c.__dict__[1] = None
            self.assertFalse(_testinternalcapi.has_split_table(c.__dict__))
            c = C()

        do_race(set_value, mutate)

    def test_racing_recursion_limit(self):
        def something_recursive():
            def count(n):
                if n > 0:
                    return count(n - 1) + 1
                return 0

            count(50)

        def set_recursion_limit():
            for limit in range(100, 200):
                sys.setrecursionlimit(limit)

        do_race(something_recursive, set_recursion_limit)


@threading_helper.requires_working_threading()
class TestWarningsRaces(TestBase):
    def setUp(self):
        self.saved_filters = warnings.filters[:]
        warnings.resetwarnings()
        # Add multiple filters to the list to increase odds of race.
        for lineno in range(20):
            warnings.filterwarnings('ignore', message='not matched', category=Warning, lineno=lineno)
        # Override showwarning() so that we don't actually show warnings.
        def showwarning(*args):
            pass
        warnings.showwarning = showwarning

    def tearDown(self):
        warnings.filters[:] = self.saved_filters
        warnings.showwarning = warnings._showwarning_orig

    def test_racing_warnings_filter(self):
        # Modifying the warnings.filters list while another thread is using
        # warn() should not crash or race.
        def modify_filters():
            time.sleep(0)
            warnings.filters[:] = [('ignore', None, UserWarning, None, 0)]
            time.sleep(0)
            warnings.filters[:] = self.saved_filters

        def emit_warning():
            warnings.warn('dummy message', category=UserWarning)

        do_race(modify_filters, emit_warning)


if __name__ == "__main__":
    unittest.main()
