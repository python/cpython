import textwrap
import unittest
from test import support
from test.support import python_is_optimized

from .util import setup_module, DebuggerTests, CET_PROTECTION, SAMPLE_SCRIPT


def setUpModule():
    setup_module()


class PyBtTests(DebuggerTests):
    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_bt(self):
        'Verify that the "py-bt" command works'
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-bt'])
        self.assertMultilineMatches(bt,
                                    r'''^.*
Traceback \(most recent call first\):
  <built-in method id of module object .*>
  File ".*gdb_sample.py", line 10, in baz
    id\(42\)
  File ".*gdb_sample.py", line 7, in bar
    baz\(a, b, c\)
  File ".*gdb_sample.py", line 4, in foo
    bar\(a=a, b=b, c=c\)
  File ".*gdb_sample.py", line 12, in <module>
    foo\(1, 2, 3\)
''')

    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_bt_full(self):
        'Verify that the "py-bt-full" command works'
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-bt-full'])
        self.assertMultilineMatches(bt,
                                    r'''^.*
#[0-9]+ Frame 0x-?[0-9a-f]+, for file .*gdb_sample.py, line 7, in bar \(a=1, b=2, c=3\)
    baz\(a, b, c\)
#[0-9]+ Frame 0x-?[0-9a-f]+, for file .*gdb_sample.py, line 4, in foo \(a=1, b=2, c=3\)
    bar\(a=a, b=b, c=c\)
#[0-9]+ Frame 0x-?[0-9a-f]+, for file .*gdb_sample.py, line 12, in <module> \(\)
    foo\(1, 2, 3\)
''')

    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    @support.requires_gil_enabled()
    @support.requires_resource('cpu')
    def test_threads(self):
        'Verify that "py-bt" indicates threads that are waiting for the GIL'
        cmd = '''
from threading import Thread

class TestThread(Thread):
    # These threads would run forever, but we'll interrupt things with the
    # debugger
    def run(self):
        i = 0
        while 1:
             i += 1

t = {}
for i in range(4):
   t[i] = TestThread()
   t[i].start()

# Trigger a breakpoint on the main thread
id(42)

'''
        # Verify with "py-bt":
        gdb_output = self.get_stack_trace(cmd,
                                          cmds_after_breakpoint=['thread apply all py-bt'])
        self.assertIn('Waiting for the GIL', gdb_output)

        # Verify with "py-bt-full":
        gdb_output = self.get_stack_trace(cmd,
                                          cmds_after_breakpoint=['thread apply all py-bt-full'])
        self.assertIn('Waiting for the GIL', gdb_output)

    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    # Some older versions of gdb will fail with
    #  "Cannot find new threads: generic error"
    # unless we add LD_PRELOAD=PATH-TO-libpthread.so.1 as a workaround
    def test_gc(self):
        'Verify that "py-bt" indicates if a thread is garbage-collecting'
        cmd = ('from gc import collect\n'
               'id(42)\n'
               'def foo():\n'
               '    collect()\n'
               'def bar():\n'
               '    foo()\n'
               'bar()\n')
        # Verify with "py-bt":
        gdb_output = self.get_stack_trace(cmd,
                                          cmds_after_breakpoint=['break update_refs', 'continue', 'py-bt'],
                                          )
        self.assertIn('Garbage-collecting', gdb_output)

        # Verify with "py-bt-full":
        gdb_output = self.get_stack_trace(cmd,
                                          cmds_after_breakpoint=['break update_refs', 'continue', 'py-bt-full'],
                                          )
        self.assertIn('Garbage-collecting', gdb_output)

    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_wrapper_call(self):
        cmd = textwrap.dedent('''
            class MyList(list):
                def __init__(self):
                    super(*[]).__init__()   # wrapper_call()

            id("first break point")
            l = MyList()
        ''')
        cmds_after_breakpoint = ['break wrapper_call', 'continue']
        if CET_PROTECTION:
            # bpo-32962: same case as in get_stack_trace():
            # we need an additional 'next' command in order to read
            # arguments of the innermost function of the call stack.
            cmds_after_breakpoint.append('next')
        cmds_after_breakpoint.append('py-bt')

        # Verify with "py-bt":
        gdb_output = self.get_stack_trace(cmd,
                                          cmds_after_breakpoint=cmds_after_breakpoint)
        self.assertRegex(gdb_output,
                         r"<method-wrapper u?'__init__' of MyList object at ")
