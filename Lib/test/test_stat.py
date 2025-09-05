import unittest
import os
import socket
import sys
from test.support import is_apple, os_helper, socket_helper
from test.support.import_helper import import_fresh_module
from test.support.os_helper import TESTFN


c_stat = import_fresh_module('stat', fresh=['_stat'])
py_stat = import_fresh_module('stat', blocked=['_stat'])

class TestFilemode:
    statmod = None

    file_flags = {'SF_APPEND', 'SF_ARCHIVED', 'SF_IMMUTABLE', 'SF_NOUNLINK',
                  'SF_SNAPSHOT', 'SF_SETTABLE', 'SF_RESTRICTED', 'SF_FIRMLINK',
                  'SF_DATALESS', 'UF_APPEND', 'UF_COMPRESSED', 'UF_HIDDEN',
                  'UF_IMMUTABLE', 'UF_NODUMP', 'UF_NOUNLINK', 'UF_OPAQUE',
                  'UF_SETTABLE', 'UF_TRACKED', 'UF_DATAVAULT'}

    formats = {'S_IFBLK', 'S_IFCHR', 'S_IFDIR', 'S_IFIFO', 'S_IFLNK',
               'S_IFREG', 'S_IFSOCK', 'S_IFDOOR', 'S_IFPORT', 'S_IFWHT'}

    format_funcs = {'S_ISBLK', 'S_ISCHR', 'S_ISDIR', 'S_ISFIFO', 'S_ISLNK',
                    'S_ISREG', 'S_ISSOCK', 'S_ISDOOR', 'S_ISPORT', 'S_ISWHT'}

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

    # defined by the Windows API documentation
    file_attributes = {
        'FILE_ATTRIBUTE_ARCHIVE': 32,
        'FILE_ATTRIBUTE_COMPRESSED': 2048,
        'FILE_ATTRIBUTE_DEVICE': 64,
        'FILE_ATTRIBUTE_DIRECTORY': 16,
        'FILE_ATTRIBUTE_ENCRYPTED': 16384,
        'FILE_ATTRIBUTE_HIDDEN': 2,
        'FILE_ATTRIBUTE_INTEGRITY_STREAM': 32768,
        'FILE_ATTRIBUTE_NORMAL': 128,
        'FILE_ATTRIBUTE_NOT_CONTENT_INDEXED': 8192,
        'FILE_ATTRIBUTE_NO_SCRUB_DATA': 131072,
        'FILE_ATTRIBUTE_OFFLINE': 4096,
        'FILE_ATTRIBUTE_READONLY': 1,
        'FILE_ATTRIBUTE_REPARSE_POINT': 1024,
        'FILE_ATTRIBUTE_SPARSE_FILE': 512,
        'FILE_ATTRIBUTE_SYSTEM': 4,
        'FILE_ATTRIBUTE_TEMPORARY': 256,
        'FILE_ATTRIBUTE_VIRTUAL': 65536}

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
        modestr = self.statmod.filemode(st_mode)
        return st_mode, modestr

    def assertS_IS(self, name, mode):
        # test format, lstrip is for S_IFIFO
        fmt = getattr(self.statmod, "S_IF" + name.lstrip("F"))
        self.assertEqual(self.statmod.S_IFMT(mode), fmt)
        # test that just one function returns true
        testname = "S_IS" + name
        for funcname in self.format_funcs:
            func = getattr(self.statmod, funcname, None)
            if func is None:
                if funcname == testname:
                    raise ValueError(funcname)
                continue
            if funcname == testname:
                self.assertTrue(func(mode))
            else:
                self.assertFalse(func(mode))

    @os_helper.skip_unless_working_chmod
    def test_mode(self):
        with open(TESTFN, 'w'):
            pass
        if os.name == 'posix':
            os.chmod(TESTFN, 0o700)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr, '-rwx------')
            self.assertS_IS("REG", st_mode)
            imode = self.statmod.S_IMODE(st_mode)
            self.assertEqual(imode,
                             self.statmod.S_IRWXU)
            self.assertEqual(self.statmod.filemode(imode),
                             '?rwx------')

            os.chmod(TESTFN, 0o070)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr, '----rwx---')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(self.statmod.S_IMODE(st_mode),
                             self.statmod.S_IRWXG)

            os.chmod(TESTFN, 0o007)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr, '-------rwx')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(self.statmod.S_IMODE(st_mode),
                             self.statmod.S_IRWXO)

            os.chmod(TESTFN, 0o444)
            st_mode, modestr = self.get_mode()
            self.assertS_IS("REG", st_mode)
            self.assertEqual(modestr, '-r--r--r--')
            self.assertEqual(self.statmod.S_IMODE(st_mode), 0o444)
        else:
            os.chmod(TESTFN, 0o500)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr[:3], '-r-')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(self.statmod.S_IMODE(st_mode), 0o444)

            os.chmod(TESTFN, 0o700)
            st_mode, modestr = self.get_mode()
            self.assertStartsWith(modestr, '-rw')
            self.assertS_IS("REG", st_mode)
            self.assertEqual(self.statmod.S_IFMT(st_mode),
                             self.statmod.S_IFREG)
            self.assertEqual(self.statmod.S_IMODE(st_mode), 0o666)

    @os_helper.skip_unless_working_chmod
    def test_directory(self):
        os.mkdir(TESTFN)
        os.chmod(TESTFN, 0o700)
        st_mode, modestr = self.get_mode()
        self.assertS_IS("DIR", st_mode)
        if os.name == 'posix':
            self.assertEqual(modestr, 'drwx------')
        else:
            self.assertEqual(modestr[0], 'd')

    @os_helper.skip_unless_symlink
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
        if sys.platform == "vxworks":
            fifo_path = os.path.join("/fifos/", TESTFN)
        else:
            fifo_path = TESTFN
        self.addCleanup(os_helper.unlink, fifo_path)
        try:
            os.mkfifo(fifo_path, 0o700)
        except PermissionError as e:
            self.skipTest('os.mkfifo(): %s' % e)
        st_mode, modestr = self.get_mode(fifo_path)
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

    @socket_helper.skip_unless_bind_unix_socket
    def test_socket(self):
        with socket.socket(socket.AF_UNIX) as s:
            s.bind(TESTFN)
            st_mode, modestr = self.get_mode()
            self.assertEqual(modestr[0], 's')
            self.assertS_IS("SOCK", st_mode)

    def test_module_attributes(self):
        for key, value in self.stat_struct.items():
            modvalue = getattr(self.statmod, key)
            self.assertEqual(value, modvalue, key)
        for key, value in self.permission_bits.items():
            modvalue = getattr(self.statmod, key)
            self.assertEqual(value, modvalue, key)
        for key in self.file_flags:
            modvalue = getattr(self.statmod, key)
            self.assertIsInstance(modvalue, int)
        for key in self.formats:
            modvalue = getattr(self.statmod, key)
            self.assertIsInstance(modvalue, int)
        for key in self.format_funcs:
            func = getattr(self.statmod, key)
            self.assertTrue(callable(func))
            self.assertEqual(func(0), 0)

    def test_flags_consistent(self):
        self.assertFalse(self.statmod.UF_SETTABLE & self.statmod.SF_SETTABLE)

        for flag in self.file_flags:
            if flag.startswith("UF"):
                self.assertTrue(getattr(self.statmod, flag) & self.statmod.UF_SETTABLE, f"{flag} not in UF_SETTABLE")
            elif is_apple and self.statmod is c_stat and flag == 'SF_DATALESS':
                self.assertTrue(self.statmod.SF_DATALESS & self.statmod.SF_SYNTHETIC, "SF_DATALESS not in SF_SYNTHETIC")
                self.assertFalse(self.statmod.SF_DATALESS & self.statmod.SF_SETTABLE, "SF_DATALESS in SF_SETTABLE")
            else:
                self.assertTrue(getattr(self.statmod, flag) & self.statmod.SF_SETTABLE, f"{flag} notin SF_SETTABLE")

    @unittest.skipUnless(sys.platform == "win32",
                         "FILE_ATTRIBUTE_* constants are Win32 specific")
    def test_file_attribute_constants(self):
        for key, value in sorted(self.file_attributes.items()):
            self.assertHasAttr(self.statmod, key)
            modvalue = getattr(self.statmod, key)
            self.assertEqual(value, modvalue, key)

    @unittest.skipUnless(sys.platform == "darwin", "macOS system check")
    def test_macosx_attribute_values(self):
        self.assertEqual(self.statmod.UF_SETTABLE, 0x0000ffff)
        self.assertEqual(self.statmod.UF_NODUMP, 0x00000001)
        self.assertEqual(self.statmod.UF_IMMUTABLE, 0x00000002)
        self.assertEqual(self.statmod.UF_APPEND, 0x00000004)
        self.assertEqual(self.statmod.UF_OPAQUE, 0x00000008)
        self.assertEqual(self.statmod.UF_COMPRESSED, 0x00000020)
        self.assertEqual(self.statmod.UF_TRACKED, 0x00000040)
        self.assertEqual(self.statmod.UF_DATAVAULT, 0x00000080)
        self.assertEqual(self.statmod.UF_HIDDEN, 0x00008000)

        if self.statmod is c_stat:
            self.assertEqual(self.statmod.SF_SUPPORTED, 0x009f0000)
            self.assertEqual(self.statmod.SF_SETTABLE, 0x3fff0000)
            self.assertEqual(self.statmod.SF_SYNTHETIC, 0xc0000000)
        else:
            self.assertEqual(self.statmod.SF_SETTABLE, 0xffff0000)
        self.assertEqual(self.statmod.SF_ARCHIVED, 0x00010000)
        self.assertEqual(self.statmod.SF_IMMUTABLE, 0x00020000)
        self.assertEqual(self.statmod.SF_APPEND, 0x00040000)
        self.assertEqual(self.statmod.SF_RESTRICTED, 0x00080000)
        self.assertEqual(self.statmod.SF_NOUNLINK, 0x00100000)
        self.assertEqual(self.statmod.SF_FIRMLINK, 0x00800000)
        self.assertEqual(self.statmod.SF_DATALESS, 0x40000000)

        self.assertFalse(isinstance(self.statmod.S_IFMT, int))
        self.assertEqual(self.statmod.S_IFIFO, 0o010000)
        self.assertEqual(self.statmod.S_IFCHR, 0o020000)
        self.assertEqual(self.statmod.S_IFDIR, 0o040000)
        self.assertEqual(self.statmod.S_IFBLK, 0o060000)
        self.assertEqual(self.statmod.S_IFREG, 0o100000)
        self.assertEqual(self.statmod.S_IFLNK, 0o120000)
        self.assertEqual(self.statmod.S_IFSOCK, 0o140000)

        if self.statmod is c_stat:
            self.assertEqual(self.statmod.S_IFWHT, 0o160000)

        self.assertEqual(self.statmod.S_IRWXU, 0o000700)
        self.assertEqual(self.statmod.S_IRUSR, 0o000400)
        self.assertEqual(self.statmod.S_IWUSR, 0o000200)
        self.assertEqual(self.statmod.S_IXUSR, 0o000100)
        self.assertEqual(self.statmod.S_IRWXG, 0o000070)
        self.assertEqual(self.statmod.S_IRGRP, 0o000040)
        self.assertEqual(self.statmod.S_IWGRP, 0o000020)
        self.assertEqual(self.statmod.S_IXGRP, 0o000010)
        self.assertEqual(self.statmod.S_IRWXO, 0o000007)
        self.assertEqual(self.statmod.S_IROTH, 0o000004)
        self.assertEqual(self.statmod.S_IWOTH, 0o000002)
        self.assertEqual(self.statmod.S_IXOTH, 0o000001)
        self.assertEqual(self.statmod.S_ISUID, 0o004000)
        self.assertEqual(self.statmod.S_ISGID, 0o002000)
        self.assertEqual(self.statmod.S_ISVTX, 0o001000)

        self.assertNotHasAttr(self.statmod, "S_ISTXT")
        self.assertEqual(self.statmod.S_IREAD, self.statmod.S_IRUSR)
        self.assertEqual(self.statmod.S_IWRITE, self.statmod.S_IWUSR)
        self.assertEqual(self.statmod.S_IEXEC, self.statmod.S_IXUSR)



@unittest.skipIf(c_stat is None, 'need _stat extension')
class TestFilemodeCStat(TestFilemode, unittest.TestCase):
    statmod = c_stat


class TestFilemodePyStat(TestFilemode, unittest.TestCase):
    statmod = py_stat


if __name__ == '__main__':
    unittest.main()
