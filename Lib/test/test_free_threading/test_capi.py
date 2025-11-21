import ctypes
import sys
import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently


_PyImport_AddModuleRef = ctypes.pythonapi.PyImport_AddModuleRef
_PyImport_AddModuleRef.argtypes = (ctypes.c_char_p,)
_PyImport_AddModuleRef.restype = ctypes.py_object


@threading_helper.requires_working_threading()
class TestImportCAPI(unittest.TestCase):
    def test_pyimport_addmoduleref_thread_safe(self):
        # gh-137422: Concurrent calls to PyImport_AddModuleRef with the same
        # module name must return the same module object.

        NUM_ITERS = 10
        NTHREADS = 4

        module_name = f"test_free_threading_addmoduleref_{id(self)}"
        module_name_bytes = module_name.encode()
        sys.modules.pop(module_name, None)
        results = []

        def worker():
            module = _PyImport_AddModuleRef(module_name_bytes)
            results.append(module)

        for _ in range(NUM_ITERS):
            try:
                run_concurrently(worker_func=worker, nthreads=NTHREADS)
                self.assertEqual(len(results), NTHREADS)
                reference = results[0]
                for module in results[1:]:
                    self.assertIs(module, reference)
                self.assertIn(module_name, sys.modules)
                self.assertIs(sys.modules[module_name], reference)
            finally:
                results.clear()
                sys.modules.pop(module_name, None)


if __name__ == "__main__":
    unittest.main()
