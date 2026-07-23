import textwrap
import unittest

from test.support import import_helper, script_helper


# Make sure _testinternalcapi is available before running the test.
import_helper.import_module('_testinternalcapi')


class InterpreterTeardownTests(unittest.TestCase):
    def test_destroy_subinterpreter_does_not_abort(self):
        # gh-153176: destroy_interpreter(basic=True) used to call
        # PyThreadState_Clear() on a non-current thread state, which on a
        # free-threaded debug build reclaimed mimalloc pages into a heap not
        # owned by the current thread and aborted the process.  Run the
        # reproduction in a subprocess so that a regression surfaces as a
        # non-zero exit / SIGABRT instead of killing the test runner.
        script = textwrap.dedent("""
            import _testinternalcapi

            interpid = _testinternalcapi.create_interpreter()
            _testinternalcapi.destroy_interpreter(interpid, basic=True)
        """)
        script_helper.assert_python_ok('-c', script)


if __name__ == "__main__":
    unittest.main()
