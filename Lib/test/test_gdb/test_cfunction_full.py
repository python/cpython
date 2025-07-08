"""
Similar to test_cfunction but test "py-bt-full" command.
"""

import re

from .util import setup_module
from .test_cfunction import CFunctionTests


def setUpModule():
    setup_module()


class CFunctionFullTests(CFunctionTests):
    def check(self, func_name, cmd):
        # Verify with "py-bt-full":
        gdb_output = self.get_stack_trace(
            cmd,
            breakpoint=func_name,
            cmds_after_breakpoint=['bt', 'py-bt-full'],
            # bpo-45207: Ignore 'Function "meth_varargs" not
            # defined.' message in stderr.
            ignore_stderr=True,
        )

        # bpo-46600: If the compiler inlines _null_to_none() in
        # meth_varargs() (ex: clang -Og), _null_to_none() is the
        # frame #1. Otherwise, meth_varargs() is the frame #1.
        regex = r'#(1|2)'
        regex += re.escape(f' <built-in method {func_name}')
        self.assertRegex(gdb_output, regex)


# Delete the test case, otherwise it's executed twice
del CFunctionTests
