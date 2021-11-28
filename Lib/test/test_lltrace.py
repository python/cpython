import os
import textwrap
import unittest

from test.support import os_helper
from test.support.script_helper import assert_python_ok


class TestLLTrace(unittest.TestCase):

    def test_lltrace_does_not_crash_on_subscript_operator(self):
        # If this test fails, it will reproduce a crash reported as
        # bpo-34113. The crash happened at the command line console of
        # debug Python builds with __ltrace__ enabled (only possible in console),
        # when the internal Python stack was negatively adjusted
        with open(os_helper.TESTFN, 'w', encoding='utf-8') as fd:
            self.addCleanup(os_helper.unlink, os_helper.TESTFN)
            fd.write(textwrap.dedent("""\
            import code

            console = code.InteractiveConsole()
            console.push('__ltrace__ = 1')
            console.push('a = [1, 2, 3]')
            console.push('a[0] = 1')
            print('unreachable if bug exists')
            """))

            assert_python_ok(os_helper.TESTFN)

if __name__ == "__main__":
    unittest.main()
