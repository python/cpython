from test import test_support
import unittest
import select
import os
import sys

@unittest.skipIf(sys.platform[:3] in ('win', 'os2', 'riscos'),
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

    def test_returned_list_identity(self):
        # See issue #8329
        r, w, x = select.select([], [], [], 1)
        self.assertIsNot(r, w)
        self.assertIsNot(r, x)
        self.assertIsNot(w, x)

    def test_select(self):
        cmd = 'for i in 0 1 2 3 4 5 6 7 8 9; do echo testing...; sleep 1; done'
        p = os.popen(cmd, 'r')
        for tout in (0, 1, 2, 4, 8, 16) + (None,)*10:
            if test_support.verbose:
                print 'timeout =', tout
            rfd, wfd, xfd = select.select([p], [], [], tout)
            if (rfd, wfd, xfd) == ([], [], []):
                continue
            if (rfd, wfd, xfd) == ([p], [], []):
                line = p.readline()
                if test_support.verbose:
                    print repr(line)
                if not line:
                    if test_support.verbose:
                        print 'EOF'
                    break
                continue
            self.fail('Unexpected return values from select():', rfd, wfd, xfd)
        p.close()


def test_main():
    test_support.run_unittest(SelectTestCase)
    test_support.reap_children()

if __name__ == "__main__":
    test_main()
