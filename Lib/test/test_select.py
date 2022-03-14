import errno
import os
import select
import subprocess
import sys
import textwrap
import unittest
from test import support

@unittest.skipIf((sys.platform[:3]=='win'),
                 "can't easily test on this system")
class SelectTestCase(unittest.TestCase):

    class Nope:
        pass

    class Almost:
        def fileno(self):
            return 'fileno'

    def test_error_conditions(self):
        self.assertRaises(TypeError, select.select, 1, 2, 3)
        self.assertRaises(TypeError, select.select, [self.Nope()], [], [])
        self.assertRaises(TypeError, select.select, [self.Almost()], [], [])
        self.assertRaises(TypeError, select.select, [], [], [], "not a number")
        self.assertRaises(ValueError, select.select, [], [], [], -1)

    # Issue #12367: http://www.freebsd.org/cgi/query-pr.cgi?pr=kern/155606
    @unittest.skipIf(sys.platform.startswith('freebsd'),
                     'skip because of a FreeBSD bug: kern/155606')
    def test_errno(self):
        with open(__file__, 'rb') as fp:
            fd = fp.fileno()
            fp.close()
            try:
                select.select([fd], [], [], 0)
            except OSError as err:
                self.assertEqual(err.errno, errno.EBADF)
            else:
                self.fail("exception not raised")

    def test_returned_list_identity(self):
        # See issue #8329
        r, w, x = select.select([], [], [], 1)
        self.assertIsNot(r, w)
        self.assertIsNot(r, x)
        self.assertIsNot(w, x)

    @support.requires_fork()
    def test_select(self):
        code = textwrap.dedent('''
            import time
            for i in range(10):
                print("testing...", flush=True)
                time.sleep(0.050)
        ''')
        cmd = [sys.executable, '-I', '-c', code]
        with subprocess.Popen(cmd, stdout=subprocess.PIPE) as proc:
            pipe = proc.stdout
            for timeout in (0, 1, 2, 4, 8, 16) + (None,)*10:
                if support.verbose:
                    print(f'timeout = {timeout}')
                rfd, wfd, xfd = select.select([pipe], [], [], timeout)
                self.assertEqual(wfd, [])
                self.assertEqual(xfd, [])
                if not rfd:
                    continue
                if rfd == [pipe]:
                    line = pipe.readline()
                    if support.verbose:
                        print(repr(line))
                    if not line:
                        if support.verbose:
                            print('EOF')
                        break
                    continue
                self.fail('Unexpected return values from select():',
                          rfd, wfd, xfd)

    # Issue 16230: Crash on select resized list
    def test_select_mutated(self):
        a = []
        class F:
            def fileno(self):
                del a[-1]
                return sys.__stdout__.fileno()
        a[:] = [F()] * 10
        self.assertEqual(select.select([], a, []), ([], a[:5], []))

    def test_disallow_instantiation(self):
        support.check_disallow_instantiation(self, type(select.poll()))

        if hasattr(select, 'devpoll'):
            support.check_disallow_instantiation(self, type(select.devpoll()))

def tearDownModule():
    support.reap_children()

if __name__ == "__main__":
    unittest.main()
