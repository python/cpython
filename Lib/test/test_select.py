from test import support
import unittest
import select
import os
import sys

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

    def test_select(self):
        if sys.platform[:3] in ('win', 'mac', 'os2', 'riscos'):
            if support.verbose:
                print("Can't test select easily on", sys.platform)
            return
        cmd = 'for i in 0 1 2 3 4 5 6 7 8 9; do echo testing...; sleep 1; done'
        p = os.popen(cmd, 'r')
        for tout in (0, 1, 2, 4, 8, 16) + (None,)*10:
            if support.verbose:
                print('timeout =', tout)
            rfd, wfd, xfd = select.select([p], [], [], tout)
            if (rfd, wfd, xfd) == ([], [], []):
                continue
            if (rfd, wfd, xfd) == ([p], [], []):
                line = p.readline()
                if support.verbose:
                    print(repr(line))
                if not line:
                    if support.verbose:
                        print('EOF')
                    break
                continue
            self.fail('Unexpected return values from select():', rfd, wfd, xfd)
        p.close()

def test_main():
    support.run_unittest(SelectTestCase)
    support.reap_children()

if __name__ == "__main__":
    test_main()
