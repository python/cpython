import unittest
import os
from test.support import TESTFN, run_unittest, import_fresh_module
import stat

class TestFilemode(unittest.TestCase):
    file_flags = {'SF_APPEND', 'SF_ARCHIVED', 'SF_IMMUTABLE', 'SF_NOUNLINK',
                  'SF_SNAPSHOT', 'UF_APPEND', 'UF_COMPRESSED', 'UF_HIDDEN',
                  'UF_IMMUTABLE', 'UF_NODUMP', 'UF_NOUNLINK', 'UF_OPAQUE'}

    formats = {'S_IFBLK', 'S_IFCHR', 'S_IFDIR', 'S_IFIFO', 'S_IFLNK',
               'S_IFREG', 'S_IFSOCK'}

    format_funcs = {'S_ISBLK', 'S_ISCHR', 'S_ISDIR', 'S_ISFIFO', 'S_ISLNK',
                    'S_ISREG', 'S_ISSOCK'}

    stat_struct = {
        'ST_MODE': 0,
        'ST_INO': 1,
        'ST_DEV': 2,
        'ST_NLINK': 3,
        'ST_UID': 4,
        'ST_GID': 5,
        'ST_SIZE': 6,
        'ST_ATIME': 7,
        'ST_MTIME': 8,
        'ST_CTIME': 9}

    # permission bit value are defined by POSIX
    permission_bits = {
        'S_ISUID': 0o4000,
        'S_ISGID': 0o2000,
        'S_ENFMT': 0o2000,
        'S_ISVTX': 0o1000,
        'S_IRWXU': 0o700,
        'S_IRUSR': 0o400,
        'S_IREAD': 0o400,
        'S_IWUSR': 0o200,
        'S_IWRITE': 0o200,
        'S_IXUSR': 0o100,
        'S_IEXEC': 0o100,
        'S_IRWXG': 0o070,
        'S_IRGRP': 0o040,
        'S_IWGRP': 0o020,
        'S_IXGRP': 0o010,
        'S_IRWXO': 0o007,
        'S_IROTH': 0o004,
        'S_IWOTH': 0o002,
        'S_IXOTH': 0o001}

    def setUp(self):
        try:
            os.remove(TESTFN)
        except OSError:
            try:
                os.rmdir(TESTFN)
            except OSError:
                pass
    tearDown = setUp

    def get_mode(self, fname=TESTFN, lstat=True):
        if lstat:
            st_mode = os.lstat(fname).st_mode
        else:
            st_mode = os.stat(fname).st_mode
        modestr = stat.filemode(st_mode)
        return st_mode, modestr

    def assertS_IS(self, name, mode):
        # test format, lstrip is for S_IFIFO
        fmt = getattr(stat, "S_IF" + name.lstrip("F"))
        self.assertEqual(stat.S_IFMT(mode), fmt)
        # test that just one function returns true
        testname = "S_IS" + name
        for funcname in self.format_funcs:
            func = getattr(stat, funcname, None)
            if func is None:
                if funcname == testname:
                    raise ValueError(funcname)
                continue
            if funcname == testname:
                self.assertTrue(func(mode))
            else:
                self.assertFalse(func(mode))

    def test_mode(self):
        with open(TESTFN, 'w'):
            pass
        if os.name == 'posix':
            os.chmod(TESTFN, 0o700)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr, '-rwx------')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(stat.S_IMODE(st_mode),
                             stat.S_IRWXU)

            os.chmod(TESTFN, 0o070)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr, '----rwx---')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(stat.S_IMODE(st_mode),
                             stat.S_IRWXG)

            os.chmod(TESTFN, 0o007)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr, '-------rwx')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(stat.S_IMODE(st_mode),
                             stat.S_IRWXO)

            os.chmod(TESTFN, 0o444)
            st_mode, modestr = self.get_mode()
            self.assertS_IS("REG", st_mode)
            self.assertEqual(modestr, '-r--r--r--')
            self.assertEqual(stat.S_IMODE(st_mode), 0o444)
        else:
            os.chmod(TESTFN, 0o700)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr[:3], '-rw')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(stat.S_IFMT(st_mode),
                             stat.S_IFREG)

    def test_directory(self):
        os.mkdir(TESTFN)
        os.chmod(TESTFN, 0o700)
        st_mode, modestr = self.get_mode()
        self.assertS_IS("DIR", st_mode)
        if os.name == 'posix':
            self.assertEqual(modestr, 'drwx------')
        else:
            self.assertEqual(modestr[0], 'd')

    @unittest.skipUnless(hasattr(os, 'symlink'), 'os.symlink not available')
    def test_link(self):
        try:
            os.symlink(os.getcwd(), TESTFN)
        except (OSError, NotImplementedError) as err:
            raise unittest.SkipTest(str(err))
        else:
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr[0], 'l')
            self.assertS_IS("LNK", st_mode)

    @unittest.skipUnless(hasattr(os, 'mkfifo'), 'os.mkfifo not available')
    def test_fifo(self):
        os.mkfifo(TESTFN, 0o700)
        st_mode, modestr = self.get_mode()
        self.assertEqual(modestr, 'prwx------')
        self.assertS_IS("FIFO", st_mode)

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def test_devices(self):
        if os.path.exists(os.devnull):
            st_mode, modestr = self.get_mode(os.devnull, lstat=False)
            self.assertEqual(modestr[0], 'c')
            self.assertS_IS("CHR", st_mode)
        # Linux block devices, BSD has no block devices anymore
        for blockdev in ("/dev/sda", "/dev/hda"):
            if os.path.exists(blockdev):
                st_mode, modestr = self.get_mode(blockdev, lstat=False)
                self.assertEqual(modestr[0], 'b')
                self.assertS_IS("BLK", st_mode)
                break

    def test_module_attributes(self):
        for key, value in self.stat_struct.items():
            modvalue = getattr(stat, key)
            self.assertEqual(value, modvalue, key)
        for key, value in self.permission_bits.items():
            modvalue = getattr(stat, key)
            self.assertEqual(value, modvalue, key)
        for key in self.file_flags:
            modvalue = getattr(stat, key)
            self.assertIsInstance(modvalue, int)
        for key in self.formats:
            modvalue = getattr(stat, key)
            self.assertIsInstance(modvalue, int)
        for key in self.format_funcs:
            func = getattr(stat, key)
            self.assertTrue(callable(func))
            self.assertEqual(func(0), 0)


def test_main():
    run_unittest(TestFilemode)

if __name__ == '__main__':
    test_main()
