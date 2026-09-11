import inspect
import multiprocessing
import unittest
from multiprocessing import dummy


class TestMultiprocessingDummy(unittest.TestCase):
    def test_signatures(self):
        """Check that the signatures in multiprocessing.dummy match these of multiprocessing."""

        common = set(dummy.__all__) & set(multiprocessing.__all__)

        # Skip Locks (these have no parameters anyway)
        common.remove("Lock")
        common.remove("RLock")

        for name in common:
            print(name)

            dummy_obj = getattr(dummy, name)
            multiprocessing_obj = getattr(multiprocessing, name)

            dummy_sig = inspect.signature(dummy_obj)
            multiprocessing_sig = inspect.signature(multiprocessing_obj)

            assert dummy_sig == multiprocessing_sig, f"{name}: {dummy_sig} != {multiprocessing_sig}"

if __name__ == '__main__':
    unittest.main()
