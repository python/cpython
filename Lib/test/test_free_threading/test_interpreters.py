import threading
import unittest

from test import support
from test.support import import_helper
from test.support import threading_helper
_interpreters = import_helper.import_module('_interpreters')


@threading_helper.requires_working_threading()
class StressTests(unittest.TestCase):

    @support.requires_resource('cpu')
    def test_subinterpreter_thread_safety(self):
        # GH-126644: _interpreters had thread safety problems on the free-threaded build
        interp = _interpreters.create()
        threads = [threading.Thread(target=_interpreters.run_string, args=(interp, "1")) for _ in range(1000)]
        threads.extend([threading.Thread(target=_interpreters.destroy, args=(interp,)) for _ in range(1000)])
        # This will spam all kinds of subinterpreter errors, but we don't care.
        # We just want to make sure that it doesn't crash.
        with threading_helper.start_threads(threads):
            pass


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
