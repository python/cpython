
import os
import tempfile
import textwrap
import unittest

from test import support
from test.support.script_helper import (
    assert_python_ok, assert_python_failure, interpreter_requires_environment,
)


class TestLLTrace(unittest.TestCase):

    def test_lltrace_does_not_crash_on_subscript_operator(self):
        # If this test fails, it will reproduce a crash reported as
        # bpo-34113. The crash happened at the command line console of
        # debug Python builds with __ltrace__ enabled (only possible in console),
        # when the interal Python stack was negatively adjusted
        with tempfile.NamedTemporaryFile(mode='w+', delete=False) as tmp_script:
            self.addCleanup(os.remove, tmp_script.name)
            tmp_script.write(textwrap.dedent("""\
            import code

            console = code.InteractiveConsole()
            console.push('__ltrace__ = 1')
            console.push('a = [1, 2, 3]')
            console.push('a[0] = 1')
            print('unreachable if bug exists')
            """))
            tmp_script.flush()

            status, stdout, stderr = assert_python_ok(tmp_script.name)
            self.assertEqual(status, 0)


def test_main():
    support.run_unittest(
        TestLLTrace,
    )

if __name__ == "__main__":
    test_main()
