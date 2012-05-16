import unittest
import os
import stat
from test.support import TESTFN, run_unittest


def get_mode(fname=TESTFN):
    return stat.filemode(os.lstat(fname).st_mode)


class TestFilemode(unittest.TestCase):

    def setUp(self):
        try:
            os.remove(TESTFN)
        except OSError:
            try:
                os.rmdir(TESTFN)
            except OSError:
                pass
    tearDown = setUp

    def test_mode(self):
        with open(TESTFN, 'w'):
            pass
        if os.name == 'posix':
            os.chmod(TESTFN, 0o700)
            self.assertEqual(get_mode(), '-rwx------')
            os.chmod(TESTFN, 0o070)
            self.assertEqual(get_mode(), '----rwx---')
            os.chmod(TESTFN, 0o007)
            self.assertEqual(get_mode(), '-------rwx')
            os.chmod(TESTFN, 0o444)
            self.assertEqual(get_mode(), '-r--r--r--')
        else:
            os.chmod(TESTFN, 0o700)
            self.assertEqual(get_mode()[:3], '-rw')

    def test_directory(self):
        os.mkdir(TESTFN)
        os.chmod(TESTFN, 0o700)
        if os.name == 'posix':
            self.assertEqual(get_mode(), 'drwx------')
        else:
            self.assertEqual(get_mode()[0], 'd')

    @unittest.skipUnless(hasattr(os, 'symlink'), 'os.symlink not available')
    def test_link(self):
        try:
            os.symlink(os.getcwd(), TESTFN)
        except (OSError, NotImplementedError) as err:
            raise unittest.SkipTest(str(err))
        else:
            self.assertEqual(get_mode()[0], 'l')

    @unittest.skipUnless(hasattr(os, 'mkfifo'), 'os.mkfifo not available')
    def test_fifo(self):
        os.mkfifo(TESTFN, 0o700)
        self.assertEqual(get_mode(), 'prwx------')


def test_main():
    run_unittest(TestFilemode)

if __name__ == '__main__':
    test_main()
