import re
import textwrap
import unittest
from test import support
from test.support import python_is_optimized

from .util import setup_module, DebuggerTests


def setUpModule():
    setup_module()


@unittest.skipIf(python_is_optimized(),
                 "Python was compiled with optimizations")
@support.requires_resource('cpu')
class CFunctionTests(DebuggerTests):
    # Some older versions of gdb will fail with
    #  "Cannot find new threads: generic error"
    # unless we add LD_PRELOAD=PATH-TO-libpthread.so.1 as a workaround
    #
    # gdb will also generate many erroneous errors such as:
    #     Function "meth_varargs" not defined.
    # This is because we are calling functions from an "external" module
    # (_testcapimodule) rather than compiled-in functions. It seems difficult
    # to suppress these. See also the comment in DebuggerTests.get_stack_trace
    def test_pycfunction(self):
        'Verify that "py-bt" displays invocations of PyCFunction instances'
        # bpo-46600: If the compiler inlines _null_to_none() in meth_varargs()
        # (ex: clang -Og), _null_to_none() is the frame #1. Otherwise,
        # meth_varargs() is the frame #1.
        expected_frame = r'#(1|2)'
        # Various optimizations multiply the code paths by which these are
        # called, so test a variety of calling conventions.
        for func_name, args in (
            ('meth_varargs', ''),
            ('meth_varargs_keywords', ''),
            ('meth_o', '[]'),
            ('meth_noargs', ''),
            ('meth_fastcall', ''),
            ('meth_fastcall_keywords', ''),
        ):
            for obj in (
                '_testcapi',
                '_testcapi.MethClass',
                '_testcapi.MethClass()',
                '_testcapi.MethStatic()',

                # XXX: bound methods don't yet give nice tracebacks
                # '_testcapi.MethInstance()',
            ):
                with self.subTest(f'{obj}.{func_name}'):
                    cmd = textwrap.dedent(f'''
                        import _testcapi
                        def foo():
                            {obj}.{func_name}({args})
                        def bar():
                            foo()
                        bar()
                    ''')
                    # Verify with "py-bt":
                    gdb_output = self.get_stack_trace(
                        cmd,
                        breakpoint=func_name,
                        cmds_after_breakpoint=['bt', 'py-bt'],
                        # bpo-45207: Ignore 'Function "meth_varargs" not
                        # defined.' message in stderr.
                        ignore_stderr=True,
                    )
                    self.assertIn(f'<built-in method {func_name}', gdb_output)

                    # Verify with "py-bt-full":
                    gdb_output = self.get_stack_trace(
                        cmd,
                        breakpoint=func_name,
                        cmds_after_breakpoint=['py-bt-full'],
                        # bpo-45207: Ignore 'Function "meth_varargs" not
                        # defined.' message in stderr.
                        ignore_stderr=True,
                    )
                    regex = expected_frame
                    regex += re.escape(f' <built-in method {func_name}')
                    self.assertRegex(gdb_output, regex)
