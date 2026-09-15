import gc
import threading
import unittest
from abc import ABC, ABCMeta

from test.support import threading_helper


class TestABC(unittest.TestCase):

    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_concurrent_checks(self):
        # isinstance()/issubclass() from many threads while other threads
        # register classes, clear the caches and create and destroy
        # subclasses.
        class A(ABC):
            pass
        class B(A):
            pass
        class C(B):
            pass
        class Registered:
            pass
        class Other:
            pass
        A.register(Registered)
        instances = [B(), C(), Registered(), Other()]
        num_threads = 8
        iterations = 2000
        barrier = threading.Barrier(num_threads + 2)
        stop = threading.Event()
        errors = []

        def checker():
            barrier.wait()
            try:
                for _ in range(iterations):
                    for cls in (A, B, C):
                        self.assertTrue(issubclass(C, cls))
                        self.assertFalse(issubclass(Other, cls))
                        self.assertFalse(isinstance(instances[3], cls))
                    self.assertTrue(isinstance(instances[0], A))
                    self.assertTrue(isinstance(instances[1], A))
                    self.assertTrue(isinstance(instances[2], A))
                    self.assertTrue(issubclass(Registered, A))
                    self.assertFalse(isinstance(instances[0], C))
            except Exception as exc:
                errors.append(exc)
            finally:
                stop.set()

        def mutator():
            barrier.wait()
            i = 0
            while not stop.is_set():
                # Transient classes get cached, registered and destroyed.
                Sub = type(f'Sub{i}', (B,), {})
                Plain = type(f'Plain{i}', (), {})
                self.assertTrue(issubclass(Sub, A))
                self.assertFalse(issubclass(Plain, A))
                A.register(Plain)
                self.assertTrue(issubclass(Plain, A))
                if i % 10 == 0:
                    A._abc_caches_clear()
                    B._abc_caches_clear()
                if i % 50 == 0:
                    gc.collect()
                i += 1

        threads = [threading.Thread(target=checker) for _ in range(num_threads)]
        threads += [threading.Thread(target=mutator) for _ in range(2)]
        with threading_helper.start_threads(threads):
            pass
        self.assertEqual(errors, [])

    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_concurrent_registry_clear(self):
        class A(ABC):
            pass
        classes = [type(f'R{i}', (), {}) for i in range(32)]
        barrier = threading.Barrier(4)

        def worker():
            barrier.wait()
            for _ in range(500):
                for cls in classes:
                    A.register(cls)
                    issubclass(cls, A)
                A._abc_registry_clear()
                A._abc_caches_clear()
                A._dump_registry(open('/dev/null', 'w'))

        threads = [threading.Thread(target=worker) for _ in range(4)]
        with threading_helper.start_threads(threads):
            pass


if __name__ == "__main__":
    unittest.main()
