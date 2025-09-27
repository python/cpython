"""
Test encodings: filename encoding, os.fsencode(), device_encoding(), etc.
"""

import codecs
import locale
import os
import shutil
import sys
import unittest
from platform import win32_is_iot
from test.support import os_helper


@unittest.skipIf(sys.platform == "win32", "Posix specific tests")
class Pep383Tests(unittest.TestCase):
    def setUp(self):
        if os_helper.TESTFN_UNENCODABLE:
            self.dir = os_helper.TESTFN_UNENCODABLE
        elif os_helper.TESTFN_NONASCII:
            self.dir = os_helper.TESTFN_NONASCII
        else:
            self.dir = os_helper.TESTFN
        self.bdir = os.fsencode(self.dir)

        bytesfn = []
        def add_filename(fn):
            try:
                fn = os.fsencode(fn)
            except UnicodeEncodeError:
                return
            bytesfn.append(fn)
        add_filename(os_helper.TESTFN_UNICODE)
        if os_helper.TESTFN_UNENCODABLE:
            add_filename(os_helper.TESTFN_UNENCODABLE)
        if os_helper.TESTFN_NONASCII:
            add_filename(os_helper.TESTFN_NONASCII)
        if not bytesfn:
            self.skipTest("couldn't create any non-ascii filename")

        self.unicodefn = set()
        os.mkdir(self.dir)
        try:
            for fn in bytesfn:
                os_helper.create_empty_file(os.path.join(self.bdir, fn))
                fn = os.fsdecode(fn)
                if fn in self.unicodefn:
                    raise ValueError("duplicate filename")
                self.unicodefn.add(fn)
        except:
            shutil.rmtree(self.dir)
            raise

    def tearDown(self):
        shutil.rmtree(self.dir)

    def test_listdir(self):
        expected = self.unicodefn
        found = set(os.listdir(self.dir))
        self.assertEqual(found, expected)
        # test listdir without arguments
        current_directory = os.getcwd()
        try:
            # The root directory is not readable on Android, so use a directory
            # we created ourselves.
            os.chdir(self.dir)
            self.assertEqual(set(os.listdir()), expected)
        finally:
            os.chdir(current_directory)

    def test_open(self):
        for fn in self.unicodefn:
            f = open(os.path.join(self.dir, fn), 'rb')
            f.close()

    @unittest.skipUnless(hasattr(os, 'statvfs'),
                            "need os.statvfs()")
    def test_statvfs(self):
        # issue #9645
        for fn in self.unicodefn:
            # should not fail with file not found error
            fullname = os.path.join(self.dir, fn)
            os.statvfs(fullname)

    def test_stat(self):
        for fn in self.unicodefn:
            os.stat(os.path.join(self.dir, fn))


class FSEncodingTests(unittest.TestCase):
    def test_nop(self):
        self.assertEqual(os.fsencode(b'abc\xff'), b'abc\xff')
        self.assertEqual(os.fsdecode('abc\u0141'), 'abc\u0141')

    def test_identity(self):
        # assert fsdecode(fsencode(x)) == x
        for fn in ('unicode\u0141', 'latin\xe9', 'ascii'):
            try:
                bytesfn = os.fsencode(fn)
            except UnicodeEncodeError:
                continue
            self.assertEqual(os.fsdecode(bytesfn), fn)


class DeviceEncodingTests(unittest.TestCase):

    def test_bad_fd(self):
        # Return None when an fd doesn't actually exist.
        self.assertIsNone(os.device_encoding(123456))

    @unittest.skipUnless(os.isatty(0) and not win32_is_iot() and (sys.platform.startswith('win') or
            (hasattr(locale, 'nl_langinfo') and hasattr(locale, 'CODESET'))),
            'test requires a tty and either Windows or nl_langinfo(CODESET)')
    def test_device_encoding(self):
        encoding = os.device_encoding(0)
        self.assertIsNotNone(encoding)
        self.assertTrue(codecs.lookup(encoding))


if __name__ == "__main__":
    unittest.main()
