import textwrap
import unittest

from test import support
from test.support import import_helper, script_helper


# Make sure _testinternalcapi is available before running the test.
import_helper.import_module('_testinternalcapi')


class InterpreterTeardownTests(unittest.TestCase):
    def test_shared_interned_strings_survive_teardown(self):
        import_helper.import_module('_testcapi')
        script = textwrap.dedent("""
            import _testcapi
            import gc
            import sys

            for i in range(10):
                code = f'import sys; sys.intern("retained_{i}_" + "x" * 5000)'
                assert _testcapi.run_in_subinterp(code) == 0
                gc.collect()
                expected = f"retained_{i}_" + "x" * 5000
                assert sys.intern(expected) == expected
        """)
        script_helper.assert_python_ok('-c', script)

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

    @support.requires_resource('cpu')
    def test_destroy_subinterpreter_with_abandoned_pages_does_not_abort(self):
        # gh-159462: Py_EndInterpreter() clears a non-current thread state.
        script = textwrap.dedent("""
            import _testinternalcapi

            interpid = _testinternalcapi.create_interpreter()
            _testinternalcapi.exec_interpreter(
                interpid, "objs = [[i] for i in range(400_000)]; del objs")
            _testinternalcapi.destroy_interpreter(interpid, basic=True)
        """)
        script_helper.assert_python_ok('-c', script)


if __name__ == "__main__":
    unittest.main()
