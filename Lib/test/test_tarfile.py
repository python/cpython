import errno
import sys
import os
import io
from hashlib import sha256
from contextlib import contextmanager, ExitStack
from random import Random
import pathlib
import shutil
import re
import warnings
import stat

import unittest
import unittest.mock
import tarfile

from test import archiver_tests
from test import support
from test.support import os_helper
from test.support import script_helper
from test.support import warnings_helper

# Check for our compression modules.
try:
    import gzip
except ImportError:
    gzip = None
try:
    import zlib
except ImportError:
    zlib = None
try:
    import bz2
except ImportError:
    bz2 = None
try:
    import lzma
except ImportError:
    lzma = None
try:
    from compression import zstd
except ImportError:
    zstd = None

def sha256sum(data):
    return sha256(data).hexdigest()

TEMPDIR = os.path.abspath(os_helper.TESTFN) + "-tardir"
tarextdir = TEMPDIR + '-extract-test'
tarname = support.findfile("testtar.tar", subdir="archivetestdata")
gzipname = os.path.join(TEMPDIR, "testtar.tar.gz")
bz2name = os.path.join(TEMPDIR, "testtar.tar.bz2")
xzname = os.path.join(TEMPDIR, "testtar.tar.xz")
zstname = os.path.join(TEMPDIR, "testtar.tar.zst")
tmpname = os.path.join(TEMPDIR, "tmp.tar")
dotlessname = os.path.join(TEMPDIR, "testtar")

sha256_regtype = (
    "e09e4bc8b3c9d9177e77256353b36c159f5f040531bbd4b024a8f9b9196c71ce"
)
sha256_sparse = (
    "4f05a776071146756345ceee937b33fc5644f5a96b9780d1c7d6a32cdf164d7b"
)


class TarTest:
    tarname = tarname
    suffix = ''
    open = io.FileIO
    taropen = tarfile.TarFile.taropen

    @property
    def mode(self):
        return self.prefix + self.suffix

@support.requires_gzip()
class GzipTest:
    tarname = gzipname
    suffix = 'gz'
    open = gzip.GzipFile if gzip else None
    taropen = tarfile.TarFile.gzopen

@support.requires_bz2()
class Bz2Test:
    tarname = bz2name
    suffix = 'bz2'
    open = bz2.BZ2File if bz2 else None
    taropen = tarfile.TarFile.bz2open

@support.requires_lzma()
class LzmaTest:
    tarname = xzname
    suffix = 'xz'
    open = lzma.LZMAFile if lzma else None
    taropen = tarfile.TarFile.xzopen

@support.requires_zstd()
class ZstdTest:
    tarname = zstname
    suffix = 'zst'
    open = zstd.ZstdFile if zstd else None
    taropen = tarfile.TarFile.zstopen

class ReadTest(TarTest):

    prefix = "r:"

    def setUp(self):
        self.tar = tarfile.open(self.tarname, mode=self.mode,
                                encoding="iso8859-1")

    def tearDown(self):
        self.tar.close()

class StreamModeTest(ReadTest):

    # Only needs to change how the tarfile is opened to set
    # stream mode
    def setUp(self):
        self.tar = tarfile.open(self.tarname, mode=self.mode,
                                encoding="iso8859-1",
                                stream=True)

class UstarReadTest(ReadTest, unittest.TestCase):

    def test_fileobj_regular_file(self):
        tarinfo = self.tar.getmember("ustar/regtype")
        with self.tar.extractfile(tarinfo) as fobj:
            data = fobj.read()
            self.assertEqual(len(data), tarinfo.size,
                    "regular file extraction failed")
            self.assertEqual(sha256sum(data), sha256_regtype,
                    "regular file extraction failed")

    def test_fileobj_readlines(self):
        self.tar.extract("ustar/regtype", TEMPDIR, filter='data')
        tarinfo = self.tar.getmember("ustar/regtype")
        with open(os.path.join(TEMPDIR, "ustar/regtype"), "r") as fobj1:
            lines1 = fobj1.readlines()

        with self.tar.extractfile(tarinfo) as fobj:
            fobj2 = io.TextIOWrapper(fobj)
            lines2 = fobj2.readlines()
            self.assertEqual(lines1, lines2,
                    "fileobj.readlines() failed")
            self.assertEqual(len(lines2), 114,
                    "fileobj.readlines() failed")
            self.assertEqual(lines2[83],
                    "I will gladly admit that Python is not the fastest "
                    "running scripting language.\n",
                    "fileobj.readlines() failed")

    def test_fileobj_iter(self):
        self.tar.extract("ustar/regtype", TEMPDIR, filter='data')
        tarinfo = self.tar.getmember("ustar/regtype")
        with open(os.path.join(TEMPDIR, "ustar/regtype"), "r") as fobj1:
            lines1 = fobj1.readlines()
        with self.tar.extractfile(tarinfo) as fobj2:
            lines2 = list(io.TextIOWrapper(fobj2))
            self.assertEqual(lines1, lines2,
                    "fileobj.__iter__() failed")

    def test_fileobj_seek(self):
        self.tar.extract("ustar/regtype", TEMPDIR,
                         filter='data')
        with open(os.path.join(TEMPDIR, "ustar/regtype"), "rb") as fobj:
            data = fobj.read()

        tarinfo = self.tar.getmember("ustar/regtype")
        with self.tar.extractfile(tarinfo) as fobj:
            text = fobj.read()
            fobj.seek(0)
            self.assertEqual(0, fobj.tell(),
                         "seek() to file's start failed")
            fobj.seek(2048, 0)
            self.assertEqual(2048, fobj.tell(),
                         "seek() to absolute position failed")
            fobj.seek(-1024, 1)
            self.assertEqual(1024, fobj.tell(),
                         "seek() to negative relative position failed")
            fobj.seek(1024, 1)
            self.assertEqual(2048, fobj.tell(),
                         "seek() to positive relative position failed")
            s = fobj.read(10)
            self.assertEqual(s, data[2048:2058],
                         "read() after seek failed")
            fobj.seek(0, 2)
            self.assertEqual(tarinfo.size, fobj.tell(),
                         "seek() to file's end failed")
            self.assertEqual(fobj.read(), b"",
                         "read() at file's end did not return empty string")
            fobj.seek(-tarinfo.size, 2)
            self.assertEqual(0, fobj.tell(),
                         "relative seek() to file's end failed")
            fobj.seek(512)
            s1 = fobj.readlines()
            fobj.seek(512)
            s2 = fobj.readlines()
            self.assertEqual(s1, s2,
                         "readlines() after seek failed")
            fobj.seek(0)
            self.assertEqual(len(fobj.readline()), fobj.tell(),
                         "tell() after readline() failed")
            fobj.seek(512)
            self.assertEqual(len(fobj.readline()) + 512, fobj.tell(),
                         "tell() after seek() and readline() failed")
            fobj.seek(0)
            line = fobj.readline()
            self.assertEqual(fobj.read(), data[len(line):],
                         "read() after readline() failed")

    def test_fileobj_text(self):
        with self.tar.extractfile("ustar/regtype") as fobj:
            fobj = io.TextIOWrapper(fobj)
            data = fobj.read().encode("iso8859-1")
            self.assertEqual(sha256sum(data), sha256_regtype)
            try:
                fobj.seek(100)
            except AttributeError:
                # Issue #13815: seek() complained about a missing
                # flush() method.
                self.fail("seeking failed in text mode")

    # Test if symbolic and hard links are resolved by extractfile().  The
    # test link members each point to a regular member whose data is
    # supposed to be exported.
    def _test_fileobj_link(self, lnktype, regtype):
        with self.tar.extractfile(lnktype) as a, \
             self.tar.extractfile(regtype) as b:
            self.assertEqual(a.name, b.name)

    def test_fileobj_link1(self):
        self._test_fileobj_link("ustar/lnktype", "ustar/regtype")

    def test_fileobj_link2(self):
        self._test_fileobj_link("./ustar/linktest2/lnktype",
                                "ustar/linktest1/regtype")

    def test_fileobj_symlink1(self):
        self._test_fileobj_link("ustar/symtype", "ustar/regtype")

    def test_fileobj_symlink2(self):
        self._test_fileobj_link("./ustar/linktest2/symtype",
                                "ustar/linktest1/regtype")

    def test_issue14160(self):
        self._test_fileobj_link("symtype2", "ustar/regtype")

    def test_add_dir_getmember(self):
        # bpo-21987
        self.add_dir_and_getmember('bar')
        self.add_dir_and_getmember('a'*101)

    @unittest.skipUnless(hasattr(os, "getuid") and hasattr(os, "getgid"),
                         "Missing getuid or getgid implementation")
    def add_dir_and_getmember(self, name):
        def filter(tarinfo):
            tarinfo.uid = tarinfo.gid = 100
            return tarinfo

        with os_helper.temp_cwd():
            with tarfile.open(tmpname, 'w') as tar:
                tar.format = tarfile.USTAR_FORMAT
                try:
                    os.mkdir(name)
                    tar.add(name, filter=filter)
                finally:
                    os.rmdir(name)
            with tarfile.open(tmpname) as tar:
                self.assertEqual(
                    tar.getmember(name),
                    tar.getmember(name + '/')
                )

class GzipUstarReadTest(GzipTest, UstarReadTest):
    pass

class Bz2UstarReadTest(Bz2Test, UstarReadTest):
    pass

class LzmaUstarReadTest(LzmaTest, UstarReadTest):
    pass

class ZstdUstarReadTest(ZstdTest, UstarReadTest):
    pass

class ListTest(ReadTest, unittest.TestCase):

    # Override setUp to use default encoding (UTF-8)
    def setUp(self):
        self.tar = tarfile.open(self.tarname, mode=self.mode)

    def test_list(self):
        tio = io.TextIOWrapper(io.BytesIO(), 'ascii', newline='\n')
        with support.swap_attr(sys, 'stdout', tio):
            self.tar.list(verbose=False)
        out = tio.detach().getvalue()
        self.assertIn(b'ustar/conttype', out)
        self.assertIn(b'ustar/regtype', out)
        self.assertIn(b'ustar/lnktype', out)
        self.assertIn(b'ustar' + (b'/12345' * 40) + b'67/longname', out)
        self.assertIn(b'./ustar/linktest2/symtype', out)
        self.assertIn(b'./ustar/linktest2/lnktype', out)
        # Make sure it puts trailing slash for directory
        self.assertIn(b'ustar/dirtype/', out)
        self.assertIn(b'ustar/dirtype-with-size/', out)
        # Make sure it is able to print unencodable characters
        def conv(b):
            s = b.decode(self.tar.encoding, 'surrogateescape')
            return s.encode('ascii', 'backslashreplace')
        self.assertIn(conv(b'ustar/umlauts-\xc4\xd6\xdc\xe4\xf6\xfc\xdf'), out)
        self.assertIn(conv(b'misc/regtype-hpux-signed-chksum-'
                           b'\xc4\xd6\xdc\xe4\xf6\xfc\xdf'), out)
        self.assertIn(conv(b'misc/regtype-old-v7-signed-chksum-'
                           b'\xc4\xd6\xdc\xe4\xf6\xfc\xdf'), out)
        self.assertIn(conv(b'pax/bad-pax-\xe4\xf6\xfc'), out)
        self.assertIn(conv(b'pax/hdrcharset-\xe4\xf6\xfc'), out)
        # Make sure it prints files separated by one newline without any
        # 'ls -l'-like accessories if verbose flag is not being used
        # ...
        # ustar/conttype
        # ustar/regtype
        # ...
        self.assertRegex(out, br'ustar/conttype ?\r?\n'
                              br'ustar/regtype ?\r?\n')
        # Make sure it does not print the source of link without verbose flag
        self.assertNotIn(b'link to', out)
        self.assertNotIn(b'->', out)

    def test_list_verbose(self):
        tio = io.TextIOWrapper(io.BytesIO(), 'ascii', newline='\n')
        with support.swap_attr(sys, 'stdout', tio):
            self.tar.list(verbose=True)
        out = tio.detach().getvalue()
        # Make sure it prints files separated by one newline with 'ls -l'-like
        # accessories if verbose flag is being used
        # ...
        # ?rw-r--r-- tarfile/tarfile     7011 2003-01-06 07:19:43 ustar/conttype
        # -rw-r--r-- tarfile/tarfile     7011 2003-01-06 07:19:43 ustar/regtype
        # drwxr-xr-x tarfile/tarfile        0 2003-01-05 15:19:43 ustar/dirtype/
        # ...
        #
        # Array of values to modify the regex below:
        #  ((file_type, file_permissions, file_length), ...)
        type_perm_lengths = (
            (br'\?', b'rw-r--r--', b'7011'), (b'-', b'rw-r--r--', b'7011'),
            (b'd', b'rwxr-xr-x', b'0'), (b'd', b'rwxr-xr-x', b'255'),
            (br'\?', b'rw-r--r--', b'0'), (b'l', b'rwxrwxrwx', b'0'),
            (b'b', b'rw-rw----', b'3,0'), (b'c', b'rw-rw-rw-', b'1,3'),
            (b'p', b'rw-r--r--', b'0'))
        self.assertRegex(out, b''.join(
            [(tp + (br'%s tarfile/tarfile\s+%s ' % (perm, ln) +
                    br'\d{4}-\d\d-\d\d\s+\d\d:\d\d:\d\d '
                    br'ustar/\w+type[/>\sa-z-]*\n')) for tp, perm, ln
             in type_perm_lengths]))
        # Make sure it prints the source of link with verbose flag
        self.assertIn(b'ustar/symtype -> regtype', out)
        self.assertIn(b'./ustar/linktest2/symtype -> ../linktest1/regtype', out)
        self.assertIn(b'./ustar/linktest2/lnktype link to '
                      b'./ustar/linktest1/regtype', out)
        self.assertIn(b'gnu' + (b'/123' * 125) + b'/longlink link to gnu' +
                      (b'/123' * 125) + b'/longname', out)
        self.assertIn(b'pax' + (b'/123' * 125) + b'/longlink link to pax' +
                      (b'/123' * 125) + b'/longname', out)

    def test_list_members(self):
        tio = io.TextIOWrapper(io.BytesIO(), 'ascii', newline='\n')
        def members(tar):
            for tarinfo in tar.getmembers():
                if 'reg' in tarinfo.name:
                    yield tarinfo
        with support.swap_attr(sys, 'stdout', tio):
            self.tar.list(verbose=False, members=members(self.tar))
        out = tio.detach().getvalue()
        self.assertIn(b'ustar/regtype', out)
        self.assertNotIn(b'ustar/conttype', out)


class GzipListTest(GzipTest, ListTest):
    pass


class Bz2ListTest(Bz2Test, ListTest):
    pass


class LzmaListTest(LzmaTest, ListTest):
    pass

class ZstdListTest(ZstdTest, ListTest):
    pass

class CommonReadTest(ReadTest):

    def test_is_tarfile_erroneous(self):
        with open(tmpname, "wb"):
            pass

        # is_tarfile works on filenames
        self.assertFalse(tarfile.is_tarfile(tmpname))

        # is_tarfile works on path-like objects
        self.assertFalse(tarfile.is_tarfile(os_helper.FakePath(tmpname)))

        # is_tarfile works on file objects
        with open(tmpname, "rb") as fobj:
            self.assertFalse(tarfile.is_tarfile(fobj))

        # is_tarfile works on file-like objects
        self.assertFalse(tarfile.is_tarfile(io.BytesIO(b"invalid")))

    def test_is_tarfile_valid(self):
        # is_tarfile works on filenames
        self.assertTrue(tarfile.is_tarfile(self.tarname))

        # is_tarfile works on path-like objects
        self.assertTrue(tarfile.is_tarfile(os_helper.FakePath(self.tarname)))

        # is_tarfile works on file objects
        with open(self.tarname, "rb") as fobj:
            self.assertTrue(tarfile.is_tarfile(fobj))

        # is_tarfile works on file-like objects
        with open(self.tarname, "rb") as fobj:
            self.assertTrue(tarfile.is_tarfile(io.BytesIO(fobj.read())))

    def test_is_tarfile_keeps_position(self):
        # Test for issue44289: tarfile.is_tarfile() modifies
        # file object's current position
        with open(self.tarname, "rb") as fobj:
            tarfile.is_tarfile(fobj)
            self.assertEqual(fobj.tell(), 0)

        with open(self.tarname, "rb") as fobj:
            file_like = io.BytesIO(fobj.read())
            tarfile.is_tarfile(file_like)
            self.assertEqual(file_like.tell(), 0)

    def test_empty_tarfile(self):
        # Test for issue6123: Allow opening empty archives.
        # This test checks if tarfile.open() is able to open an empty tar
        # archive successfully. Note that an empty tar archive is not the
        # same as an empty file!
        with tarfile.open(tmpname, self.mode.replace("r", "w")):
            pass
        try:
            tar = tarfile.open(tmpname, self.mode)
            tar.getnames()
        except tarfile.ReadError:
            self.fail("tarfile.open() failed on empty archive")
        else:
            self.assertListEqual(tar.getmembers(), [])
        finally:
            tar.close()

    def test_non_existent_tarfile(self):
        # Test for issue11513: prevent non-existent gzipped tarfiles raising
        # multiple exceptions.
        with self.assertRaisesRegex(FileNotFoundError, "xxx"):
            tarfile.open("xxx", self.mode)

    def test_null_tarfile(self):
        # Test for issue6123: Allow opening empty archives.
        # This test guarantees that tarfile.open() does not treat an empty
        # file as an empty tar archive.
        with open(tmpname, "wb"):
            pass
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname, self.mode)
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname)

    def test_ignore_zeros(self):
        # Test TarFile's ignore_zeros option.
        # generate 512 pseudorandom bytes
        data = Random(0).randbytes(512)
        for char in (b'\0', b'a'):
            # Test if EOFHeaderError ('\0') and InvalidHeaderError ('a')
            # are ignored correctly.
            with self.open(tmpname, "w") as fobj:
                fobj.write(char * 1024)
                tarinfo = tarfile.TarInfo("foo")
                tarinfo.size = len(data)
                fobj.write(tarinfo.tobuf())
                fobj.write(data)

            tar = tarfile.open(tmpname, mode="r", ignore_zeros=True)
            try:
                self.assertListEqual(tar.getnames(), ["foo"],
                    "ignore_zeros=True should have skipped the %r-blocks" %
                    char)
            finally:
                tar.close()

    def test_premature_end_of_archive(self):
        for size in (512, 600, 1024, 1200):
            with tarfile.open(tmpname, "w:") as tar:
                t = tarfile.TarInfo("foo")
                t.size = 1024
                tar.addfile(t, io.BytesIO(b"a" * 1024))

            with open(tmpname, "r+b") as fobj:
                fobj.truncate(size)

            with tarfile.open(tmpname) as tar:
                with self.assertRaisesRegex(tarfile.ReadError, "unexpected end of data"):
                    for t in tar:
                        pass

            with tarfile.open(tmpname) as tar:
                t = tar.next()

                with self.assertRaisesRegex(tarfile.ReadError, "unexpected end of data"):
                    tar.extract(t, TEMPDIR, filter='data')

                with self.assertRaisesRegex(tarfile.ReadError, "unexpected end of data"):
                    tar.extractfile(t).read()

    def test_length_zero_header(self):
        # bpo-39017 (CVE-2019-20907): reading a zero-length header should fail
        # with an exception
        with self.assertRaisesRegex(tarfile.ReadError, "file could not be opened successfully"):
            with tarfile.open(support.findfile('recursion.tar', subdir='archivetestdata')):
                pass

    def test_extractfile_attrs(self):
        # gh-74468: TarFile.name must name a file, not a parent archive.
        file = self.tar.getmember('ustar/regtype')
        with self.tar.extractfile(file) as fobj:
            self.assertEqual(fobj.name, 'ustar/regtype')
            self.assertRaises(AttributeError, fobj.fileno)
            self.assertEqual(fobj.mode, 'rb')
            self.assertIs(fobj.readable(), True)
            self.assertIs(fobj.writable(), False)
            if self.is_stream:
                self.assertRaises(AttributeError, fobj.seekable)
            else:
                self.assertIs(fobj.seekable(), True)
            self.assertIs(fobj.closed, False)
        self.assertIs(fobj.closed, True)
        self.assertEqual(fobj.name, 'ustar/regtype')
        self.assertRaises(AttributeError, fobj.fileno)
        self.assertEqual(fobj.mode, 'rb')
        self.assertIs(fobj.readable(), True)
        self.assertIs(fobj.writable(), False)
        if self.is_stream:
            self.assertRaises(AttributeError, fobj.seekable)
        else:
            self.assertIs(fobj.seekable(), True)


class MiscReadTestBase(CommonReadTest):
    is_stream = False

    def test_no_name_argument(self):
        with open(self.tarname, "rb") as fobj:
            self.assertIsInstance(fobj.name, str)
            with tarfile.open(fileobj=fobj, mode=self.mode) as tar:
                self.assertIsInstance(tar.name, str)
                self.assertEqual(tar.name, os.path.abspath(fobj.name))

    def test_no_name_attribute(self):
        with open(self.tarname, "rb") as fobj:
            data = fobj.read()
        fobj = io.BytesIO(data)
        self.assertRaises(AttributeError, getattr, fobj, "name")
        tar = tarfile.open(fileobj=fobj, mode=self.mode)
        self.assertIsNone(tar.name)

    def test_empty_name_attribute(self):
        with open(self.tarname, "rb") as fobj:
            data = fobj.read()
        fobj = io.BytesIO(data)
        fobj.name = ""
        with tarfile.open(fileobj=fobj, mode=self.mode) as tar:
            self.assertIsNone(tar.name)

    def test_int_name_attribute(self):
        # Issue 21044: tarfile.open() should handle fileobj with an integer
        # 'name' attribute.
        fd = os.open(self.tarname, os.O_RDONLY)
        with open(fd, 'rb') as fobj:
            self.assertIsInstance(fobj.name, int)
            with tarfile.open(fileobj=fobj, mode=self.mode) as tar:
                self.assertIsNone(tar.name)

    def test_bytes_name_attribute(self):
        tarname = os.fsencode(self.tarname)
        with open(tarname, 'rb') as fobj:
            self.assertIsInstance(fobj.name, bytes)
            with tarfile.open(fileobj=fobj, mode=self.mode) as tar:
                self.assertIsInstance(tar.name, bytes)
                self.assertEqual(tar.name, os.path.abspath(fobj.name))

    def test_pathlike_name(self, tarname=None):
        if tarname is None:
            tarname = self.tarname
        expected = os.path.abspath(tarname)
        tarname = os_helper.FakePath(tarname)
        with tarfile.open(tarname, mode=self.mode) as tar:
            self.assertEqual(tar.name, expected)
        with self.taropen(tarname) as tar:
            self.assertEqual(tar.name, expected)
        with tarfile.TarFile.open(tarname, mode=self.mode) as tar:
            self.assertEqual(tar.name, expected)
        if self.suffix == '':
            with tarfile.TarFile(tarname, mode='r') as tar:
                self.assertEqual(tar.name, expected)

    def test_pathlike_bytes_name(self):
        self.test_pathlike_name(os.fsencode(self.tarname))

    def test_illegal_mode_arg(self):
        with open(tmpname, 'wb'):
            pass
        with self.assertRaisesRegex(ValueError, 'mode must be '):
            tar = self.taropen(tmpname, 'q')
        with self.assertRaisesRegex(ValueError, 'mode must be '):
            tar = self.taropen(tmpname, 'rw')
        with self.assertRaisesRegex(ValueError, 'mode must be '):
            tar = self.taropen(tmpname, '')

    def test_fileobj_with_offset(self):
        # Skip the first member and store values from the second member
        # of the testtar.
        tar = tarfile.open(self.tarname, mode=self.mode)
        try:
            tar.next()
            t = tar.next()
            name = t.name
            offset = t.offset
            with tar.extractfile(t) as f:
                data = f.read()
        finally:
            tar.close()

        # Open the testtar and seek to the offset of the second member.
        with self.open(self.tarname) as fobj:
            fobj.seek(offset)

            # Test if the tarfile starts with the second member.
            with tar.open(self.tarname, mode="r:", fileobj=fobj) as tar:
                t = tar.next()
                self.assertEqual(t.name, name)
                # Read to the end of fileobj and test if seeking back to the
                # beginning works.
                tar.getmembers()
                self.assertEqual(tar.extractfile(t).read(), data,
                        "seek back did not work")

    def test_fail_comp(self):
        # For Gzip and Bz2 Tests: fail with a ReadError on an uncompressed file.
        self.assertRaises(tarfile.ReadError, tarfile.open, tarname, self.mode)
        with open(tarname, "rb") as fobj:
            self.assertRaises(tarfile.ReadError, tarfile.open,
                              fileobj=fobj, mode=self.mode)

    def test_v7_dirtype(self):
        # Test old style dirtype member (bug #1336623):
        # Old V7 tars create directory members using an AREGTYPE
        # header with a "/" appended to the filename field.
        tarinfo = self.tar.getmember("misc/dirtype-old-v7")
        self.assertEqual(tarinfo.type, tarfile.DIRTYPE,
                "v7 dirtype failed")

    def test_xstar_type(self):
        # The xstar format stores extra atime and ctime fields inside the
        # space reserved for the prefix field. The prefix field must be
        # ignored in this case, otherwise it will mess up the name.
        try:
            self.tar.getmember("misc/regtype-xstar")
        except KeyError:
            self.fail("failed to find misc/regtype-xstar (mangled prefix?)")

    def test_check_members(self):
        for tarinfo in self.tar:
            self.assertEqual(int(tarinfo.mtime), 0o7606136617,
                    "wrong mtime for %s" % tarinfo.name)
            if not tarinfo.name.startswith("ustar/"):
                continue
            self.assertEqual(tarinfo.uname, "tarfile",
                    "wrong uname for %s" % tarinfo.name)

    def test_find_members(self):
        self.assertEqual(self.tar.getmembers()[-1].name, "misc/eof",
                "could not find all members")

    @unittest.skipUnless(hasattr(os, "link"),
                         "Missing hardlink implementation")
    @os_helper.skip_unless_symlink
    def test_extract_hardlink(self):
        # Test hardlink extraction (e.g. bug #857297).
        with tarfile.open(tarname, errorlevel=1, encoding="iso8859-1") as tar:
            tar.extract("ustar/regtype", TEMPDIR, filter='data')
            self.addCleanup(os_helper.unlink, os.path.join(TEMPDIR, "ustar/regtype"))

            tar.extract("ustar/lnktype", TEMPDIR, filter='data')
            self.addCleanup(os_helper.unlink, os.path.join(TEMPDIR, "ustar/lnktype"))
            with open(os.path.join(TEMPDIR, "ustar/lnktype"), "rb") as f:
                data = f.read()
            self.assertEqual(sha256sum(data), sha256_regtype)

            tar.extract("ustar/symtype", TEMPDIR, filter='data')
            self.addCleanup(os_helper.unlink, os.path.join(TEMPDIR, "ustar/symtype"))
            with open(os.path.join(TEMPDIR, "ustar/symtype"), "rb") as f:
                data = f.read()
            self.assertEqual(sha256sum(data), sha256_regtype)

    @os_helper.skip_unless_working_chmod
    def test_extractall(self):
        # Test if extractall() correctly restores directory permissions
        # and times (see issue1735).
        tar = tarfile.open(tarname, encoding="iso8859-1")
        DIR = os.path.join(TEMPDIR, "extractall")
        os.mkdir(DIR)
        try:
            directories = [t for t in tar if t.isdir()]
            tar.extractall(DIR, directories, filter='fully_trusted')
            for tarinfo in directories:
                path = os.path.join(DIR, tarinfo.name)
                if sys.platform != "win32":
                    # Win32 has no support for fine grained permissions.
                    self.assertEqual(tarinfo.mode & 0o777,
                                     os.stat(path).st_mode & 0o777,
                                     tarinfo.name)
                def format_mtime(mtime):
                    if isinstance(mtime, float):
                        return "{} ({})".format(mtime, mtime.hex())
                    else:
                        return "{!r} (int)".format(mtime)
                file_mtime = os.path.getmtime(path)
                errmsg = "tar mtime {0} != file time {1} of path {2!a}".format(
                    format_mtime(tarinfo.mtime),
                    format_mtime(file_mtime),
                    path)
                self.assertEqual(tarinfo.mtime, file_mtime, errmsg)
        finally:
            tar.close()
            os_helper.rmtree(DIR)

    @staticmethod
    def test_extractall_default_filter():
        # Test that the default filter is now "data", and the other filter types are not used.
        DIR = pathlib.Path(TEMPDIR) / "extractall_default_filter"
        with (
            os_helper.temp_dir(DIR),
            tarfile.open(tarname, encoding="iso8859-1") as tar,
            unittest.mock.patch("tarfile.data_filter", wraps=tarfile.data_filter) as mock_data_filter,
            unittest.mock.patch("tarfile.tar_filter", wraps=tarfile.tar_filter) as mock_tar_filter,
            unittest.mock.patch("tarfile.fully_trusted_filter", wraps=tarfile.fully_trusted_filter) as mock_ft_filter
        ):
            directories = [t for t in tar if t.isdir()]
            tar.extractall(DIR, directories)

            mock_data_filter.assert_called()
            mock_ft_filter.assert_not_called()
            mock_tar_filter.assert_not_called()

    @os_helper.skip_unless_working_chmod
    def test_extract_directory(self):
        dirtype = "ustar/dirtype"
        DIR = os.path.join(TEMPDIR, "extractdir")
        os.mkdir(DIR)
        try:
            with tarfile.open(tarname, encoding="iso8859-1") as tar:
                tarinfo = tar.getmember(dirtype)
                tar.extract(tarinfo, path=DIR, filter='fully_trusted')
                extracted = os.path.join(DIR, dirtype)
                self.assertEqual(os.path.getmtime(extracted), tarinfo.mtime)
                if sys.platform != "win32":
                    self.assertEqual(os.stat(extracted).st_mode & 0o777, 0o755)
        finally:
            os_helper.rmtree(DIR)

    def test_extractall_pathlike_dir(self):
        DIR = os.path.join(TEMPDIR, "extractall")
        with os_helper.temp_dir(DIR), \
             tarfile.open(tarname, encoding="iso8859-1") as tar:
            directories = [t for t in tar if t.isdir()]
            tar.extractall(os_helper.FakePath(DIR), directories, filter='fully_trusted')
            for tarinfo in directories:
                path = os.path.join(DIR, tarinfo.name)
                self.assertEqual(os.path.getmtime(path), tarinfo.mtime)

    def test_extract_pathlike_dir(self):
        dirtype = "ustar/dirtype"
        DIR = os.path.join(TEMPDIR, "extractall")
        with os_helper.temp_dir(DIR), \
             tarfile.open(tarname, encoding="iso8859-1") as tar:
            tarinfo = tar.getmember(dirtype)
            tar.extract(tarinfo, path=os_helper.FakePath(DIR), filter='fully_trusted')
            extracted = os.path.join(DIR, dirtype)
            self.assertEqual(os.path.getmtime(extracted), tarinfo.mtime)

    def test_init_close_fobj(self):
        # Issue #7341: Close the internal file object in the TarFile
        # constructor in case of an error. For the test we rely on
        # the fact that opening an empty file raises a ReadError.
        empty = os.path.join(TEMPDIR, "empty")
        with open(empty, "wb") as fobj:
            fobj.write(b"")

        try:
            tar = object.__new__(tarfile.TarFile)
            try:
                tar.__init__(empty)
            except tarfile.ReadError:
                self.assertTrue(tar.fileobj.closed)
            else:
                self.fail("ReadError not raised")
        finally:
            os_helper.unlink(empty)

    def test_parallel_iteration(self):
        # Issue #16601: Restarting iteration over tarfile continued
        # from where it left off.
        with tarfile.open(self.tarname) as tar:
            for m1, m2 in zip(tar, tar):
                self.assertEqual(m1.offset, m2.offset)
                self.assertEqual(m1.get_info(), m2.get_info())

    @unittest.skipIf(zlib is None, "requires zlib")
    def test_zlib_error_does_not_leak(self):
        # bpo-39039: tarfile.open allowed zlib exceptions to bubble up when
        # parsing certain types of invalid data
        with unittest.mock.patch("tarfile.TarInfo.fromtarfile") as mock:
            mock.side_effect = zlib.error
            with self.assertRaises(tarfile.ReadError):
                tarfile.open(self.tarname)

    def test_next_on_empty_tarfile(self):
        fd = io.BytesIO()
        tf = tarfile.open(fileobj=fd, mode="w")
        tf.close()

        fd.seek(0)
        with tarfile.open(fileobj=fd, mode="r|") as tf:
            self.assertEqual(tf.next(), None)

        fd.seek(0)
        with tarfile.open(fileobj=fd, mode="r") as tf:
            self.assertEqual(tf.next(), None)

class MiscReadTest(MiscReadTestBase, unittest.TestCase):
    test_fail_comp = None

class GzipMiscReadTest(GzipTest, MiscReadTestBase, unittest.TestCase):
    pass

class Bz2MiscReadTest(Bz2Test, MiscReadTestBase, unittest.TestCase):
    pass

class LzmaMiscReadTest(LzmaTest, MiscReadTestBase, unittest.TestCase):
    pass

class ZstdMiscReadTest(ZstdTest, MiscReadTestBase, unittest.TestCase):
    pass

class StreamReadTest(CommonReadTest, unittest.TestCase):

    prefix="r|"
    is_stream = True

    def test_read_through(self):
        # Issue #11224: A poorly designed _FileInFile.read() method
        # caused seeking errors with stream tar files.
        for tarinfo in self.tar:
            if not tarinfo.isreg():
                continue
            with self.tar.extractfile(tarinfo) as fobj:
                while True:
                    try:
                        buf = fobj.read(512)
                    except tarfile.StreamError:
                        self.fail("simple read-through using "
                                  "TarFile.extractfile() failed")
                    if not buf:
                        break

    def test_fileobj_regular_file(self):
        tarinfo = self.tar.next() # get "regtype" (can't use getmember)
        with self.tar.extractfile(tarinfo) as fobj:
            data = fobj.read()
        self.assertEqual(len(data), tarinfo.size,
                "regular file extraction failed")
        self.assertEqual(sha256sum(data), sha256_regtype,
                "regular file extraction failed")

    def test_provoke_stream_error(self):
        tarinfos = self.tar.getmembers()
        with self.tar.extractfile(tarinfos[0]) as f: # read the first member
            self.assertRaises(tarfile.StreamError, f.read)

    def test_compare_members(self):
        tar1 = tarfile.open(tarname, encoding="iso8859-1")
        try:
            tar2 = self.tar

            while True:
                t1 = tar1.next()
                t2 = tar2.next()
                if t1 is None:
                    break
                self.assertIsNotNone(t2, "stream.next() failed.")

                if t2.islnk() or t2.issym():
                    with self.assertRaises(tarfile.StreamError):
                        tar2.extractfile(t2)
                    continue

                v1 = tar1.extractfile(t1)
                v2 = tar2.extractfile(t2)
                if v1 is None:
                    continue
                self.assertIsNotNone(v2, "stream.extractfile() failed")
                self.assertEqual(v1.read(), v2.read(),
                        "stream extraction failed")
        finally:
            tar1.close()

class GzipStreamReadTest(GzipTest, StreamReadTest):
    pass

class Bz2StreamReadTest(Bz2Test, StreamReadTest):
    pass

class LzmaStreamReadTest(LzmaTest, StreamReadTest):
    pass

class ZstdStreamReadTest(ZstdTest, StreamReadTest):
    pass

class TarStreamModeReadTest(StreamModeTest, unittest.TestCase):

    def test_stream_mode_no_cache(self):
        for _ in self.tar:
            pass
        self.assertEqual(self.tar.members, [])

class GzipStreamModeReadTest(GzipTest, TarStreamModeReadTest):
    pass

class Bz2StreamModeReadTest(Bz2Test, TarStreamModeReadTest):
    pass

class LzmaStreamModeReadTest(LzmaTest, TarStreamModeReadTest):
    pass

class ZstdStreamModeReadTest(ZstdTest, TarStreamModeReadTest):
    pass

class DetectReadTest(TarTest, unittest.TestCase):
    def _testfunc_file(self, name, mode):
        try:
            tar = tarfile.open(name, mode)
        except tarfile.ReadError as e:
            self.fail()
        else:
            tar.close()

    def _testfunc_fileobj(self, name, mode):
        try:
            with open(name, "rb") as f:
                tar = tarfile.open(name, mode, fileobj=f)
        except tarfile.ReadError as e:
            self.fail()
        else:
            tar.close()

    def _test_modes(self, testfunc):
        if self.suffix:
            with self.assertRaises(tarfile.ReadError):
                tarfile.open(tarname, mode="r:" + self.suffix)
            with self.assertRaises(tarfile.ReadError):
                tarfile.open(tarname, mode="r|" + self.suffix)
            with self.assertRaises(tarfile.ReadError):
                tarfile.open(self.tarname, mode="r:")
            with self.assertRaises(tarfile.ReadError):
                tarfile.open(self.tarname, mode="r|")
        testfunc(self.tarname, "r")
        testfunc(self.tarname, "r:" + self.suffix)
        testfunc(self.tarname, "r:*")
        testfunc(self.tarname, "r|" + self.suffix)
        testfunc(self.tarname, "r|*")

    def test_detect_file(self):
        self._test_modes(self._testfunc_file)

    def test_detect_fileobj(self):
        self._test_modes(self._testfunc_fileobj)

class GzipDetectReadTest(GzipTest, DetectReadTest):
    pass

class Bz2DetectReadTest(Bz2Test, DetectReadTest):
    def test_detect_stream_bz2(self):
        # Originally, tarfile's stream detection looked for the string
        # "BZh91" at the start of the file. This is incorrect because
        # the '9' represents the blocksize (900,000 bytes). If the file was
        # compressed using another blocksize autodetection fails.
        with open(tarname, "rb") as fobj:
            data = fobj.read()

        # Compress with blocksize 100,000 bytes, the file starts with "BZh11".
        with bz2.BZ2File(tmpname, "wb", compresslevel=1) as fobj:
            fobj.write(data)

        self._testfunc_file(tmpname, "r|*")

class LzmaDetectReadTest(LzmaTest, DetectReadTest):
    pass

class ZstdDetectReadTest(ZstdTest, DetectReadTest):
    pass

class GzipBrokenHeaderCorrectException(GzipTest, unittest.TestCase):
    """
    See: https://github.com/python/cpython/issues/107396
    """
    def runTest(self):
        f = io.BytesIO(
            b'\x1f\x8b'  # header
            b'\x08'  # compression method
            b'\x04'  # flags
            b'\0\0\0\0\0\0'  # timestamp, compression data, OS ID
            b'\0\x01'  # size
            b'\0\0\0\0\0'  # corrupt data (zeros)
        )
        with self.assertRaises(tarfile.ReadError):
            tarfile.open(fileobj=f, mode='r|gz')


class MemberReadTest(ReadTest, unittest.TestCase):

    def _test_member(self, tarinfo, chksum=None, **kwargs):
        if chksum is not None:
            with self.tar.extractfile(tarinfo) as f:
                self.assertEqual(sha256sum(f.read()), chksum,
                        "wrong sha256sum for %s" % tarinfo.name)

        kwargs["mtime"] = 0o7606136617
        kwargs["uid"] = 1000
        kwargs["gid"] = 100
        if "old-v7" not in tarinfo.name:
            # V7 tar can't handle alphabetic owners.
            kwargs["uname"] = "tarfile"
            kwargs["gname"] = "tarfile"
        for k, v in kwargs.items():
            self.assertEqual(getattr(tarinfo, k), v,
                    "wrong value in %s field of %s" % (k, tarinfo.name))

    def test_find_regtype(self):
        tarinfo = self.tar.getmember("ustar/regtype")
        self._test_member(tarinfo, size=7011, chksum=sha256_regtype)

    def test_find_conttype(self):
        tarinfo = self.tar.getmember("ustar/conttype")
        self._test_member(tarinfo, size=7011, chksum=sha256_regtype)

    def test_find_dirtype(self):
        tarinfo = self.tar.getmember("ustar/dirtype")
        self._test_member(tarinfo, size=0)

    def test_find_dirtype_with_size(self):
        tarinfo = self.tar.getmember("ustar/dirtype-with-size")
        self._test_member(tarinfo, size=255)

    def test_find_lnktype(self):
        tarinfo = self.tar.getmember("ustar/lnktype")
        self._test_member(tarinfo, size=0, linkname="ustar/regtype")

    def test_find_symtype(self):
        tarinfo = self.tar.getmember("ustar/symtype")
        self._test_member(tarinfo, size=0, linkname="regtype")

    def test_find_blktype(self):
        tarinfo = self.tar.getmember("ustar/blktype")
        self._test_member(tarinfo, size=0, devmajor=3, devminor=0)

    def test_find_chrtype(self):
        tarinfo = self.tar.getmember("ustar/chrtype")
        self._test_member(tarinfo, size=0, devmajor=1, devminor=3)

    def test_find_fifotype(self):
        tarinfo = self.tar.getmember("ustar/fifotype")
        self._test_member(tarinfo, size=0)

    def test_find_sparse(self):
        tarinfo = self.tar.getmember("ustar/sparse")
        self._test_member(tarinfo, size=86016, chksum=sha256_sparse)

    def test_find_gnusparse(self):
        tarinfo = self.tar.getmember("gnu/sparse")
        self._test_member(tarinfo, size=86016, chksum=sha256_sparse)

    def test_find_gnusparse_00(self):
        tarinfo = self.tar.getmember("gnu/sparse-0.0")
        self._test_member(tarinfo, size=86016, chksum=sha256_sparse)

    def test_find_gnusparse_01(self):
        tarinfo = self.tar.getmember("gnu/sparse-0.1")
        self._test_member(tarinfo, size=86016, chksum=sha256_sparse)

    def test_find_gnusparse_10(self):
        tarinfo = self.tar.getmember("gnu/sparse-1.0")
        self._test_member(tarinfo, size=86016, chksum=sha256_sparse)

    def test_find_umlauts(self):
        tarinfo = self.tar.getmember("ustar/umlauts-"
                                     "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")
        self._test_member(tarinfo, size=7011, chksum=sha256_regtype)

    def test_find_ustar_longname(self):
        name = "ustar/" + "12345/" * 39 + "1234567/longname"
        self.assertIn(name, self.tar.getnames())

    def test_find_regtype_oldv7(self):
        tarinfo = self.tar.getmember("misc/regtype-old-v7")
        self._test_member(tarinfo, size=7011, chksum=sha256_regtype)

    def test_find_pax_umlauts(self):
        self.tar.close()
        self.tar = tarfile.open(self.tarname, mode=self.mode,
                                encoding="iso8859-1")
        tarinfo = self.tar.getmember("pax/umlauts-"
                                     "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")
        self._test_member(tarinfo, size=7011, chksum=sha256_regtype)


class LongnameTest:

    def test_read_longname(self):
        # Test reading of longname (bug #1471427).
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        try:
            tarinfo = self.tar.getmember(longname)
        except KeyError:
            self.fail("longname not found")
        self.assertNotEqual(tarinfo.type, tarfile.DIRTYPE,
                "read longname as dirtype")

    def test_read_longlink(self):
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        longlink = self.subdir + "/" + "123/" * 125 + "longlink"
        try:
            tarinfo = self.tar.getmember(longlink)
        except KeyError:
            self.fail("longlink not found")
        self.assertEqual(tarinfo.linkname, longname, "linkname wrong")

    def test_truncated_longname(self):
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        tarinfo = self.tar.getmember(longname)
        offset = tarinfo.offset
        self.tar.fileobj.seek(offset)
        fobj = io.BytesIO(self.tar.fileobj.read(3 * 512))
        with self.assertRaises(tarfile.ReadError):
            tarfile.open(name="foo.tar", fileobj=fobj)

    def test_header_offset(self):
        # Test if the start offset of the TarInfo object includes
        # the preceding extended header.
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        offset = self.tar.getmember(longname).offset
        with open(tarname, "rb") as fobj:
            fobj.seek(offset)
            tarinfo = tarfile.TarInfo.frombuf(fobj.read(512),
                                              "iso8859-1", "strict")
            self.assertEqual(tarinfo.type, self.longnametype)

    def test_longname_directory(self):
        # Test reading a longlink directory. Issue #47231.
        longdir = ('a' * 101) + '/'
        with os_helper.temp_cwd():
            with tarfile.open(tmpname, 'w') as tar:
                tar.format = self.format
                try:
                    os.mkdir(longdir)
                    tar.add(longdir)
                finally:
                    os.rmdir(longdir.rstrip("/"))
            with tarfile.open(tmpname) as tar:
                self.assertIsNotNone(tar.getmember(longdir))
                self.assertIsNotNone(tar.getmember(longdir.removesuffix('/')))

class GNUReadTest(LongnameTest, ReadTest, unittest.TestCase):

    subdir = "gnu"
    longnametype = tarfile.GNUTYPE_LONGNAME
    format = tarfile.GNU_FORMAT

    # Since 3.2 tarfile is supposed to accurately restore sparse members and
    # produce files with holes. This is what we actually want to test here.
    # Unfortunately, not all platforms/filesystems support sparse files, and
    # even on platforms that do it is non-trivial to make reliable assertions
    # about holes in files. Therefore, we first do one basic test which works
    # an all platforms, and after that a test that will work only on
    # platforms/filesystems that prove to support sparse files.
    def _test_sparse_file(self, name):
        self.tar.extract(name, TEMPDIR, filter='data')
        filename = os.path.join(TEMPDIR, name)
        with open(filename, "rb") as fobj:
            data = fobj.read()
        self.assertEqual(sha256sum(data), sha256_sparse,
                "wrong sha256sum for %s" % name)

        if self._fs_supports_holes():
            s = os.stat(filename)
            self.assertLess(s.st_blocks * 512, s.st_size)

    def test_sparse_file_old(self):
        self._test_sparse_file("gnu/sparse")

    def test_sparse_file_00(self):
        self._test_sparse_file("gnu/sparse-0.0")

    def test_sparse_file_01(self):
        self._test_sparse_file("gnu/sparse-0.1")

    def test_sparse_file_10(self):
        self._test_sparse_file("gnu/sparse-1.0")

    @staticmethod
    def _fs_supports_holes():
        # Return True if the platform knows the st_blocks stat attribute and
        # uses st_blocks units of 512 bytes, and if the filesystem is able to
        # store holes of 4 KiB in files.
        #
        # The function returns False if page size is larger than 4 KiB.
        # For example, ppc64 uses pages of 64 KiB.
        if sys.platform.startswith(("linux", "android")):
            # Linux evidentially has 512 byte st_blocks units.
            name = os.path.join(TEMPDIR, "sparse-test")
            with open(name, "wb") as fobj:
                # Seek to "punch a hole" of 4 KiB
                fobj.seek(4096)
                fobj.write(b'x' * 4096)
                fobj.truncate()
            s = os.stat(name)
            os_helper.unlink(name)
            return (s.st_blocks * 512 < s.st_size)
        else:
            return False


class PaxReadTest(LongnameTest, ReadTest, unittest.TestCase):

    subdir = "pax"
    longnametype = tarfile.XHDTYPE
    format = tarfile.PAX_FORMAT

    def test_pax_global_headers(self):
        tar = tarfile.open(tarname, encoding="iso8859-1")
        try:
            tarinfo = tar.getmember("pax/regtype1")
            self.assertEqual(tarinfo.uname, "foo")
            self.assertEqual(tarinfo.gname, "bar")
            self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"),
                             "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")

            tarinfo = tar.getmember("pax/regtype2")
            self.assertEqual(tarinfo.uname, "")
            self.assertEqual(tarinfo.gname, "bar")
            self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"),
                             "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")

            tarinfo = tar.getmember("pax/regtype3")
            self.assertEqual(tarinfo.uname, "tarfile")
            self.assertEqual(tarinfo.gname, "tarfile")
            self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"),
                             "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")
        finally:
            tar.close()

    def test_pax_number_fields(self):
        # All following number fields are read from the pax header.
        tar = tarfile.open(tarname, encoding="iso8859-1")
        try:
            tarinfo = tar.getmember("pax/regtype4")
            self.assertEqual(tarinfo.size, 7011)
            self.assertEqual(tarinfo.uid, 123)
            self.assertEqual(tarinfo.gid, 123)
            self.assertEqual(tarinfo.mtime, 1041808783.0)
            self.assertEqual(type(tarinfo.mtime), float)
            self.assertEqual(float(tarinfo.pax_headers["atime"]), 1041808783.0)
            self.assertEqual(float(tarinfo.pax_headers["ctime"]), 1041808783.0)
        finally:
            tar.close()

    def test_pax_header_bad_formats(self):
        # The fields from the pax header have priority over the
        # TarInfo.
        pax_header_replacements = (
            b" foo=bar\n",
            b"0 \n",
            b"1 \n",
            b"2 \n",
            b"3 =\n",
            b"4 =a\n",
            b"1000000 foo=bar\n",
            b"0 foo=bar\n",
            b"-12 foo=bar\n",
            b"000000000000000000000000036 foo=bar\n",
        )
        pax_headers = {"foo": "bar"}

        for replacement in pax_header_replacements:
            with self.subTest(header=replacement):
                tar = tarfile.open(tmpname, "w", format=tarfile.PAX_FORMAT,
                                   encoding="iso8859-1")
                try:
                    t = tarfile.TarInfo()
                    t.name = "pax"  # non-ASCII
                    t.uid = 1
                    t.pax_headers = pax_headers
                    tar.addfile(t)
                finally:
                    tar.close()

                with open(tmpname, "rb") as f:
                    data = f.read()
                    self.assertIn(b"11 foo=bar\n", data)
                    data = data.replace(b"11 foo=bar\n", replacement)

                with open(tmpname, "wb") as f:
                    f.truncate()
                    f.write(data)

                with self.assertRaisesRegex(tarfile.ReadError, r"method tar: ReadError\('invalid header'\)"):
                    tarfile.open(tmpname, encoding="iso8859-1")


class WriteTestBase(TarTest):
    # Put all write tests in here that are supposed to be tested
    # in all possible mode combinations.

    def test_fileobj_no_close(self):
        fobj = io.BytesIO()
        with tarfile.open(fileobj=fobj, mode=self.mode) as tar:
            tar.addfile(tarfile.TarInfo("foo"))
        self.assertFalse(fobj.closed, "external fileobjs must never closed")
        # Issue #20238: Incomplete gzip output with mode="w:gz"
        data = fobj.getvalue()
        del tar
        support.gc_collect()
        self.assertFalse(fobj.closed)
        self.assertEqual(data, fobj.getvalue())

    def test_eof_marker(self):
        # Make sure an end of archive marker is written (two zero blocks).
        # tarfile insists on aligning archives to a 20 * 512 byte recordsize.
        # So, we create an archive that has exactly 10240 bytes without the
        # marker, and has 20480 bytes once the marker is written.
        with tarfile.open(tmpname, self.mode) as tar:
            t = tarfile.TarInfo("foo")
            t.size = tarfile.RECORDSIZE - tarfile.BLOCKSIZE
            tar.addfile(t, io.BytesIO(b"a" * t.size))

        with self.open(tmpname, "rb") as fobj:
            self.assertEqual(len(fobj.read()), tarfile.RECORDSIZE * 2)


class WriteTest(WriteTestBase, unittest.TestCase):

    prefix = "w:"

    def test_100_char_name(self):
        # The name field in a tar header stores strings of at most 100 chars.
        # If a string is shorter than 100 chars it has to be padded with '\0',
        # which implies that a string of exactly 100 chars is stored without
        # a trailing '\0'.
        name = "0123456789" * 10
        tar = tarfile.open(tmpname, self.mode)
        try:
            t = tarfile.TarInfo(name)
            tar.addfile(t)
        finally:
            tar.close()

        tar = tarfile.open(tmpname)
        try:
            self.assertEqual(tar.getnames()[0], name,
                    "failed to store 100 char filename")
        finally:
            tar.close()

    def test_tar_size(self):
        # Test for bug #1013882.
        tar = tarfile.open(tmpname, self.mode)
        try:
            path = os.path.join(TEMPDIR, "file")
            with open(path, "wb") as fobj:
                fobj.write(b"aaa")
            tar.add(path)
        finally:
            tar.close()
        self.assertGreater(os.path.getsize(tmpname), 0,
                "tarfile is empty")

    # The test_*_size tests test for bug #1167128.
    def test_file_size(self):
        tar = tarfile.open(tmpname, self.mode)
        try:
            path = os.path.join(TEMPDIR, "file")
            with open(path, "wb"):
                pass
            tarinfo = tar.gettarinfo(path)
            self.assertEqual(tarinfo.size, 0)

            with open(path, "wb") as fobj:
                fobj.write(b"aaa")
            tarinfo = tar.gettarinfo(path)
            self.assertEqual(tarinfo.size, 3)
        finally:
            tar.close()

    def test_directory_size(self):
        path = os.path.join(TEMPDIR, "directory")
        os.mkdir(path)
        try:
            tar = tarfile.open(tmpname, self.mode)
            try:
                tarinfo = tar.gettarinfo(path)
                self.assertEqual(tarinfo.size, 0)
            finally:
                tar.close()
        finally:
            os_helper.rmdir(path)

    # mock the following:
    #  os.listdir: so we know that files are in the wrong order
    def test_ordered_recursion(self):
        path = os.path.join(TEMPDIR, "directory")
        os.mkdir(path)
        open(os.path.join(path, "1"), "a").close()
        open(os.path.join(path, "2"), "a").close()
        try:
            tar = tarfile.open(tmpname, self.mode)
            try:
                with unittest.mock.patch('os.listdir') as mock_listdir:
                    mock_listdir.return_value = ["2", "1"]
                    tar.add(path)
                paths = []
                for m in tar.getmembers():
                    paths.append(os.path.split(m.name)[-1])
                self.assertEqual(paths, ["directory", "1", "2"]);
            finally:
                tar.close()
        finally:
            os_helper.unlink(os.path.join(path, "1"))
            os_helper.unlink(os.path.join(path, "2"))
            os_helper.rmdir(path)

    def test_gettarinfo_pathlike_name(self):
        with tarfile.open(tmpname, self.mode) as tar:
            path = os.path.join(TEMPDIR, "file")
            with open(path, "wb") as fobj:
                fobj.write(b"aaa")
            tarinfo = tar.gettarinfo(os_helper.FakePath(path))
            tarinfo2 = tar.gettarinfo(path)
            self.assertIsInstance(tarinfo.name, str)
            self.assertEqual(tarinfo.name, tarinfo2.name)
            self.assertEqual(tarinfo.size, 3)

    @unittest.skipUnless(hasattr(os, "link"),
                         "Missing hardlink implementation")
    def test_link_size(self):
        link = os.path.join(TEMPDIR, "link")
        target = os.path.join(TEMPDIR, "link_target")
        with open(target, "wb") as fobj:
            fobj.write(b"aaa")
        try:
            os.link(target, link)
        except PermissionError as e:
            self.skipTest('os.link(): %s' % e)
        try:
            tar = tarfile.open(tmpname, self.mode)
            try:
                # Record the link target in the inodes list.
                tar.gettarinfo(target)
                tarinfo = tar.gettarinfo(link)
                self.assertEqual(tarinfo.size, 0)
            finally:
                tar.close()
        finally:
            os_helper.unlink(target)
            os_helper.unlink(link)

    @os_helper.skip_unless_symlink
    def test_symlink_size(self):
        path = os.path.join(TEMPDIR, "symlink")
        os.symlink("link_target", path)
        try:
            tar = tarfile.open(tmpname, self.mode)
            try:
                tarinfo = tar.gettarinfo(path)
                self.assertEqual(tarinfo.size, 0)
            finally:
                tar.close()
        finally:
            os_helper.unlink(path)

    def test_add_self(self):
        # Test for #1257255.
        dstname = os.path.abspath(tmpname)
        tar = tarfile.open(tmpname, self.mode)
        try:
            self.assertEqual(tar.name, dstname,
                    "archive name must be absolute")
            tar.add(dstname)
            self.assertEqual(tar.getnames(), [],
                    "added the archive to itself")

            with os_helper.change_cwd(TEMPDIR):
                tar.add(dstname)
            self.assertEqual(tar.getnames(), [],
                    "added the archive to itself")
        finally:
            tar.close()

    def test_filter(self):
        tempdir = os.path.join(TEMPDIR, "filter")
        os.mkdir(tempdir)
        try:
            for name in ("foo", "bar", "baz"):
                name = os.path.join(tempdir, name)
                os_helper.create_empty_file(name)

            def filter(tarinfo):
                if os.path.basename(tarinfo.name) == "bar":
                    return
                tarinfo.uid = 123
                tarinfo.uname = "foo"
                return tarinfo

            tar = tarfile.open(tmpname, self.mode, encoding="iso8859-1")
            try:
                tar.add(tempdir, arcname="empty_dir", filter=filter)
            finally:
                tar.close()

            # Verify that filter is a keyword-only argument
            with self.assertRaises(TypeError):
                tar.add(tempdir, "empty_dir", True, None, filter)

            tar = tarfile.open(tmpname, "r")
            try:
                for tarinfo in tar:
                    self.assertEqual(tarinfo.uid, 123)
                    self.assertEqual(tarinfo.uname, "foo")
                self.assertEqual(len(tar.getmembers()), 3)
            finally:
                tar.close()
        finally:
            os_helper.rmtree(tempdir)

    # Guarantee that stored pathnames are not modified. Don't
    # remove ./ or ../ or double slashes. Still make absolute
    # pathnames relative.
    # For details see bug #6054.
    def _test_pathname(self, path, cmp_path=None, dir=False):
        # Create a tarfile with an empty member named path
        # and compare the stored name with the original.
        foo = os.path.join(TEMPDIR, "foo")
        if not dir:
            os_helper.create_empty_file(foo)
        else:
            os.mkdir(foo)

        tar = tarfile.open(tmpname, self.mode)
        try:
            tar.add(foo, arcname=path)
        finally:
            tar.close()

        tar = tarfile.open(tmpname, "r")
        try:
            t = tar.next()
        finally:
            tar.close()

        if not dir:
            os_helper.unlink(foo)
        else:
            os_helper.rmdir(foo)

        self.assertEqual(t.name, cmp_path or path.replace(os.sep, "/"))


    @os_helper.skip_unless_symlink
    def test_extractall_symlinks(self):
        # Test if extractall works properly when tarfile contains symlinks
        tempdir = os.path.join(TEMPDIR, "testsymlinks")
        temparchive = os.path.join(TEMPDIR, "testsymlinks.tar")
        os.mkdir(tempdir)
        try:
            source_file = os.path.join(tempdir,'source')
            target_file = os.path.join(tempdir,'symlink')
            with open(source_file,'w') as f:
                f.write('something\n')
            os.symlink(source_file, target_file)
            with tarfile.open(temparchive, 'w') as tar:
                tar.add(source_file, arcname="source")
                tar.add(target_file, arcname="symlink")
            # Let's extract it to the location which contains the symlink
            with tarfile.open(temparchive, errorlevel=2) as tar:
                # this should not raise OSError: [Errno 17] File exists
                try:
                    tar.extractall(path=tempdir,
                                   filter='fully_trusted')
                except OSError:
                    self.fail("extractall failed with symlinked files")
        finally:
            os_helper.unlink(temparchive)
            os_helper.rmtree(tempdir)

    def test_pathnames(self):
        self._test_pathname("foo")
        self._test_pathname(os.path.join("foo", ".", "bar"))
        self._test_pathname(os.path.join("foo", "..", "bar"))
        self._test_pathname(os.path.join(".", "foo"))
        self._test_pathname(os.path.join(".", "foo", "."))
        self._test_pathname(os.path.join(".", "foo", ".", "bar"))
        self._test_pathname(os.path.join(".", "foo", "..", "bar"))
        self._test_pathname(os.path.join(".", "foo", "..", "bar"))
        self._test_pathname(os.path.join("..", "foo"))
        self._test_pathname(os.path.join("..", "foo", ".."))
        self._test_pathname(os.path.join("..", "foo", ".", "bar"))
        self._test_pathname(os.path.join("..", "foo", "..", "bar"))

        self._test_pathname("foo" + os.sep + os.sep + "bar")
        self._test_pathname("foo" + os.sep + os.sep, "foo", dir=True)

    def test_abs_pathnames(self):
        if sys.platform == "win32":
            self._test_pathname("C:\\foo", "foo")
        else:
            self._test_pathname("/foo", "foo")
            self._test_pathname("///foo", "foo")

    def test_cwd(self):
        # Test adding the current working directory.
        with os_helper.change_cwd(TEMPDIR):
            tar = tarfile.open(tmpname, self.mode)
            try:
                tar.add(".")
            finally:
                tar.close()

            tar = tarfile.open(tmpname, "r")
            try:
                for t in tar:
                    if t.name != ".":
                        self.assertTrue(t.name.startswith("./"), t.name)
            finally:
                tar.close()

    def test_open_nonwritable_fileobj(self):
        for exctype in OSError, EOFError, RuntimeError:
            class BadFile(io.BytesIO):
                first = True
                def write(self, data):
                    if self.first:
                        self.first = False
                        raise exctype

            f = BadFile()
            with (
                warnings_helper.check_no_resource_warning(self),
                self.assertRaises(exctype),
            ):
                tarfile.open(tmpname, self.mode, fileobj=f,
                             format=tarfile.PAX_FORMAT,
                             pax_headers={'non': 'empty'})
            self.assertFalse(f.closed)

    def test_missing_fileobj(self):
        with tarfile.open(tmpname, self.mode) as tar:
            tarinfo = tar.gettarinfo(tarname)
            with self.assertRaises(ValueError):
                tar.addfile(tarinfo)


class GzipWriteTest(GzipTest, WriteTest):
    pass


class Bz2WriteTest(Bz2Test, WriteTest):
    pass


class LzmaWriteTest(LzmaTest, WriteTest):
    pass

class ZstdWriteTest(ZstdTest, WriteTest):
    pass

class StreamWriteTest(WriteTestBase, unittest.TestCase):

    prefix = "w|"
    decompressor = None

    def test_stream_padding(self):
        # Test for bug #1543303.
        tar = tarfile.open(tmpname, self.mode)
        tar.close()
        if self.decompressor:
            dec = self.decompressor()
            with open(tmpname, "rb") as fobj:
                data = fobj.read()
            data = dec.decompress(data)
            self.assertFalse(dec.unused_data, "found trailing data")
        else:
            with self.open(tmpname) as fobj:
                data = fobj.read()
        self.assertEqual(data.count(b"\0"), tarfile.RECORDSIZE,
                        "incorrect zero padding")

    @unittest.skipUnless(sys.platform != "win32" and hasattr(os, "umask"),
                         "Missing umask implementation")
    @unittest.skipIf(
        support.is_emscripten or support.is_wasi,
        "Emscripten's/WASI's umask is a stub."
    )
    def test_file_mode(self):
        # Test for issue #8464: Create files with correct
        # permissions.
        if os.path.exists(tmpname):
            os_helper.unlink(tmpname)

        original_umask = os.umask(0o022)
        try:
            tar = tarfile.open(tmpname, self.mode)
            tar.close()
            mode = os.stat(tmpname).st_mode & 0o777
            self.assertEqual(mode, 0o644, "wrong file permissions")
        finally:
            os.umask(original_umask)


class GzipStreamWriteTest(GzipTest, StreamWriteTest):
    def test_source_directory_not_leaked(self):
        """
        Ensure the source directory is not included in the tar header
        per bpo-41316.
        """
        tarfile.open(tmpname, self.mode).close()
        payload = pathlib.Path(tmpname).read_text(encoding='latin-1')
        assert os.path.dirname(tmpname) not in payload


class Bz2StreamWriteTest(Bz2Test, StreamWriteTest):
    decompressor = bz2.BZ2Decompressor if bz2 else None

class LzmaStreamWriteTest(LzmaTest, StreamWriteTest):
    decompressor = lzma.LZMADecompressor if lzma else None

class ZstdStreamWriteTest(ZstdTest, StreamWriteTest):
    decompressor = zstd.ZstdDecompressor if zstd else None

class _CompressedWriteTest(TarTest):
    # This is not actually a standalone test.
    # It does not inherit WriteTest because it only makes sense with gz,bz2
    source = (b"And we move to Bristol where they have a special, " +
              b"Very Silly candidate")

    def _compressed_tar(self, compresslevel):
        fobj = io.BytesIO()
        with tarfile.open(tmpname, self.mode, fobj,
                          compresslevel=compresslevel) as tarfl:
            tarfl.addfile(tarfile.TarInfo("foo"), io.BytesIO(self.source))
        return fobj

    def _test_bz2_header(self, compresslevel):
        fobj = self._compressed_tar(compresslevel)
        self.assertEqual(fobj.getvalue()[0:10],
                         b"BZh%d1AY&SY" % compresslevel)

    def _test_gz_header(self, compresslevel):
        fobj = self._compressed_tar(compresslevel)
        self.assertEqual(fobj.getvalue()[:3], b"\x1f\x8b\x08")

class Bz2CompressWriteTest(Bz2Test, _CompressedWriteTest, unittest.TestCase):
    prefix = "w:"
    def test_compression_levels(self):
        self._test_bz2_header(1)
        self._test_bz2_header(5)
        self._test_bz2_header(9)

class Bz2CompressStreamWriteTest(Bz2Test, _CompressedWriteTest,
        unittest.TestCase):
    prefix = "w|"
    def test_compression_levels(self):
        self._test_bz2_header(1)
        self._test_bz2_header(5)
        self._test_bz2_header(9)

class GzCompressWriteTest(GzipTest,  _CompressedWriteTest, unittest.TestCase):
    prefix = "w:"
    def test_compression_levels(self):
        self._test_gz_header(1)
        self._test_gz_header(5)
        self._test_gz_header(9)

class GzCompressStreamWriteTest(GzipTest, _CompressedWriteTest,
        unittest.TestCase):
    prefix = "w|"
    def test_compression_levels(self):
        self._test_gz_header(1)
        self._test_gz_header(5)
        self._test_gz_header(9)

class CompressLevelRaises(unittest.TestCase):
    def test_compresslevel_wrong_modes(self):
        compresslevel = 5
        fobj = io.BytesIO()
        with self.assertRaises(TypeError):
            tarfile.open(tmpname, "w:", fobj, compresslevel=compresslevel)

    @support.requires_bz2()
    def test_wrong_compresslevels(self):
        # BZ2 checks that the compresslevel is in [1,9]. gz does not
        fobj = io.BytesIO()
        with self.assertRaises(ValueError):
            tarfile.open(tmpname, "w:bz2", fobj, compresslevel=0)
        with self.assertRaises(ValueError):
            tarfile.open(tmpname, "w:bz2", fobj, compresslevel=10)
        with self.assertRaises(ValueError):
            tarfile.open(tmpname, "w|bz2", fobj, compresslevel=10)

class GNUWriteTest(unittest.TestCase):
    # This testcase checks for correct creation of GNU Longname
    # and Longlink extended headers (cp. bug #812325).

    def _length(self, s):
        blocks = len(s) // 512 + 1
        return blocks * 512

    def _calc_size(self, name, link=None):
        # Initial tar header
        count = 512

        if len(name) > tarfile.LENGTH_NAME:
            # GNU longname extended header + longname
            count += 512
            count += self._length(name)
        if link is not None and len(link) > tarfile.LENGTH_LINK:
            # GNU longlink extended header + longlink
            count += 512
            count += self._length(link)
        return count

    def _test(self, name, link=None):
        tarinfo = tarfile.TarInfo(name)
        if link:
            tarinfo.linkname = link
            tarinfo.type = tarfile.LNKTYPE

        tar = tarfile.open(tmpname, "w")
        try:
            tar.format = tarfile.GNU_FORMAT
            tar.addfile(tarinfo)

            v1 = self._calc_size(name, link)
            v2 = tar.offset
            self.assertEqual(v1, v2, "GNU longname/longlink creation failed")
        finally:
            tar.close()

        tar = tarfile.open(tmpname)
        try:
            member = tar.next()
            self.assertIsNotNone(member,
                    "unable to read longname member")
            self.assertEqual(tarinfo.name, member.name,
                    "unable to read longname member")
            self.assertEqual(tarinfo.linkname, member.linkname,
                    "unable to read longname member")
        finally:
            tar.close()

    def test_longname_1023(self):
        self._test(("longnam/" * 127) + "longnam")

    def test_longname_1024(self):
        self._test(("longnam/" * 127) + "longname")

    def test_longname_1025(self):
        self._test(("longnam/" * 127) + "longname_")

    def test_longlink_1023(self):
        self._test("name", ("longlnk/" * 127) + "longlnk")

    def test_longlink_1024(self):
        self._test("name", ("longlnk/" * 127) + "longlink")

    def test_longlink_1025(self):
        self._test("name", ("longlnk/" * 127) + "longlink_")

    def test_longnamelink_1023(self):
        self._test(("longnam/" * 127) + "longnam",
                   ("longlnk/" * 127) + "longlnk")

    def test_longnamelink_1024(self):
        self._test(("longnam/" * 127) + "longname",
                   ("longlnk/" * 127) + "longlink")

    def test_longnamelink_1025(self):
        self._test(("longnam/" * 127) + "longname_",
                   ("longlnk/" * 127) + "longlink_")


class DeviceHeaderTest(WriteTestBase, unittest.TestCase):

    prefix = "w:"

    def test_headers_written_only_for_device_files(self):
        # Regression test for bpo-18819.
        tempdir = os.path.join(TEMPDIR, "device_header_test")
        os.mkdir(tempdir)
        try:
            tar = tarfile.open(tmpname, self.mode)
            try:
                input_blk = tarfile.TarInfo(name="my_block_device")
                input_reg = tarfile.TarInfo(name="my_regular_file")
                input_blk.type = tarfile.BLKTYPE
                input_reg.type = tarfile.REGTYPE
                tar.addfile(input_blk)
                tar.addfile(input_reg)
            finally:
                tar.close()

            # devmajor and devminor should be *interpreted* as 0 in both...
            tar = tarfile.open(tmpname, "r")
            try:
                output_blk = tar.getmember("my_block_device")
                output_reg = tar.getmember("my_regular_file")
            finally:
                tar.close()
            self.assertEqual(output_blk.devmajor, 0)
            self.assertEqual(output_blk.devminor, 0)
            self.assertEqual(output_reg.devmajor, 0)
            self.assertEqual(output_reg.devminor, 0)

            # ...but the fields should not actually be set on regular files:
            with open(tmpname, "rb") as infile:
                buf = infile.read()
            buf_blk = buf[output_blk.offset:output_blk.offset_data]
            buf_reg = buf[output_reg.offset:output_reg.offset_data]
            # See `struct posixheader` in GNU docs for byte offsets:
            # <https://www.gnu.org/software/tar/manual/html_node/Standard.html>
            device_headers = slice(329, 329 + 16)
            self.assertEqual(buf_blk[device_headers], b"0000000\0" * 2)
            self.assertEqual(buf_reg[device_headers], b"\0" * 16)
        finally:
            os_helper.rmtree(tempdir)


class CreateTest(WriteTestBase, unittest.TestCase):

    prefix = "x:"

    file_path = os.path.join(TEMPDIR, "spameggs42")

    def setUp(self):
        os_helper.unlink(tmpname)

    @classmethod
    def setUpClass(cls):
        with open(cls.file_path, "wb") as fobj:
            fobj.write(b"aaa")

    @classmethod
    def tearDownClass(cls):
        os_helper.unlink(cls.file_path)

    def test_create(self):
        with tarfile.open(tmpname, self.mode) as tobj:
            tobj.add(self.file_path)

        with self.taropen(tmpname) as tobj:
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn('spameggs42', names[0])

    def test_create_existing(self):
        with tarfile.open(tmpname, self.mode) as tobj:
            tobj.add(self.file_path)

        with self.assertRaises(FileExistsError):
            tobj = tarfile.open(tmpname, self.mode)

        with self.taropen(tmpname) as tobj:
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn('spameggs42', names[0])

    def test_create_taropen(self):
        with self.taropen(tmpname, "x") as tobj:
            tobj.add(self.file_path)

        with self.taropen(tmpname) as tobj:
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn('spameggs42', names[0])

    def test_create_existing_taropen(self):
        with self.taropen(tmpname, "x") as tobj:
            tobj.add(self.file_path)

        with self.assertRaises(FileExistsError):
            with self.taropen(tmpname, "x"):
                pass

        with self.taropen(tmpname) as tobj:
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn("spameggs42", names[0])

    def test_create_pathlike_name(self):
        with tarfile.open(os_helper.FakePath(tmpname), self.mode) as tobj:
            self.assertIsInstance(tobj.name, str)
            self.assertEqual(tobj.name, os.path.abspath(tmpname))
            tobj.add(os_helper.FakePath(self.file_path))
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn('spameggs42', names[0])

        with self.taropen(tmpname) as tobj:
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn('spameggs42', names[0])

    def test_create_taropen_pathlike_name(self):
        with self.taropen(os_helper.FakePath(tmpname), "x") as tobj:
            self.assertIsInstance(tobj.name, str)
            self.assertEqual(tobj.name, os.path.abspath(tmpname))
            tobj.add(os_helper.FakePath(self.file_path))
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn('spameggs42', names[0])

        with self.taropen(tmpname) as tobj:
            names = tobj.getnames()
        self.assertEqual(len(names), 1)
        self.assertIn('spameggs42', names[0])


class GzipCreateTest(GzipTest, CreateTest):

    def test_create_with_compresslevel(self):
        with tarfile.open(tmpname, self.mode, compresslevel=1) as tobj:
            tobj.add(self.file_path)
        with tarfile.open(tmpname, 'r:gz', compresslevel=1) as tobj:
            pass


class Bz2CreateTest(Bz2Test, CreateTest):

    def test_create_with_compresslevel(self):
        with tarfile.open(tmpname, self.mode, compresslevel=1) as tobj:
            tobj.add(self.file_path)
        with tarfile.open(tmpname, 'r:bz2', compresslevel=1) as tobj:
            pass


class LzmaCreateTest(LzmaTest, CreateTest):

    # Unlike gz and bz2, xz uses the preset keyword instead of compresslevel.
    # It does not allow for preset to be specified when reading.
    def test_create_with_preset(self):
        with tarfile.open(tmpname, self.mode, preset=1) as tobj:
            tobj.add(self.file_path)


class ZstdCreateTest(ZstdTest, CreateTest):

    # Unlike gz and bz2, zstd uses the level keyword instead of compresslevel.
    # It does not allow for level to be specified when reading.
    def test_create_with_level(self):
        with tarfile.open(tmpname, self.mode, level=1) as tobj:
            tobj.add(self.file_path)

class CreateWithXModeTest(CreateTest):

    prefix = "x"

    test_create_taropen = None
    test_create_existing_taropen = None


@unittest.skipUnless(hasattr(os, "link"), "Missing hardlink implementation")
class HardlinkTest(unittest.TestCase):
    # Test the creation of LNKTYPE (hardlink) members in an archive.

    def setUp(self):
        self.foo = os.path.join(TEMPDIR, "foo")
        self.bar = os.path.join(TEMPDIR, "bar")

        with open(self.foo, "wb") as fobj:
            fobj.write(b"foo")

        try:
            os.link(self.foo, self.bar)
        except PermissionError as e:
            self.skipTest('os.link(): %s' % e)

        self.tar = tarfile.open(tmpname, "w")
        self.tar.add(self.foo)

    def tearDown(self):
        self.tar.close()
        os_helper.unlink(self.foo)
        os_helper.unlink(self.bar)

    def test_add_twice(self):
        # The same name will be added as a REGTYPE every
        # time regardless of st_nlink.
        tarinfo = self.tar.gettarinfo(self.foo)
        self.assertEqual(tarinfo.type, tarfile.REGTYPE,
                "add file as regular failed")

    def test_add_hardlink(self):
        tarinfo = self.tar.gettarinfo(self.bar)
        self.assertEqual(tarinfo.type, tarfile.LNKTYPE,
                "add file as hardlink failed")

    def test_dereference_hardlink(self):
        self.tar.dereference = True
        tarinfo = self.tar.gettarinfo(self.bar)
        self.assertEqual(tarinfo.type, tarfile.REGTYPE,
                "dereferencing hardlink failed")


class PaxWriteTest(GNUWriteTest):

    def _test(self, name, link=None):
        # See GNUWriteTest.
        tarinfo = tarfile.TarInfo(name)
        if link:
            tarinfo.linkname = link
            tarinfo.type = tarfile.LNKTYPE

        tar = tarfile.open(tmpname, "w", format=tarfile.PAX_FORMAT)
        try:
            tar.addfile(tarinfo)
        finally:
            tar.close()

        tar = tarfile.open(tmpname)
        try:
            if link:
                l = tar.getmembers()[0].linkname
                self.assertEqual(link, l, "PAX longlink creation failed")
            else:
                n = tar.getmembers()[0].name
                self.assertEqual(name, n, "PAX longname creation failed")
        finally:
            tar.close()

    def test_pax_global_header(self):
        pax_headers = {
                "foo": "bar",
                "uid": "0",
                "mtime": "1.23",
                "test": "\xe4\xf6\xfc",
                "\xe4\xf6\xfc": "test"}

        tar = tarfile.open(tmpname, "w", format=tarfile.PAX_FORMAT,
                pax_headers=pax_headers)
        try:
            tar.addfile(tarfile.TarInfo("test"))
        finally:
            tar.close()

        # Test if the global header was written correctly.
        tar = tarfile.open(tmpname, encoding="iso8859-1")
        try:
            self.assertEqual(tar.pax_headers, pax_headers)
            self.assertEqual(tar.getmembers()[0].pax_headers, pax_headers)
            # Test if all the fields are strings.
            for key, val in tar.pax_headers.items():
                self.assertIsNot(type(key), bytes)
                self.assertIsNot(type(val), bytes)
                if key in tarfile.PAX_NUMBER_FIELDS:
                    try:
                        tarfile.PAX_NUMBER_FIELDS[key](val)
                    except (TypeError, ValueError):
                        self.fail("unable to convert pax header field")
        finally:
            tar.close()

    def test_pax_extended_header(self):
        # The fields from the pax header have priority over the
        # TarInfo.
        pax_headers = {"path": "foo", "uid": "123"}

        tar = tarfile.open(tmpname, "w", format=tarfile.PAX_FORMAT,
                           encoding="iso8859-1")
        try:
            t = tarfile.TarInfo()
            t.name = "\xe4\xf6\xfc" # non-ASCII
            t.uid = 8**8 # too large
            t.pax_headers = pax_headers
            tar.addfile(t)
        finally:
            tar.close()

        tar = tarfile.open(tmpname, encoding="iso8859-1")
        try:
            t = tar.getmembers()[0]
            self.assertEqual(t.pax_headers, pax_headers)
            self.assertEqual(t.name, "foo")
            self.assertEqual(t.uid, 123)
        finally:
            tar.close()

    def test_create_pax_header(self):
        # The ustar header should contain values that can be
        # represented reasonably, even if a better (e.g. higher
        # precision) version is set in the pax header.
        # Issue #45863

        # values that should be kept
        t = tarfile.TarInfo()
        t.name = "foo"
        t.mtime = 1000.1
        t.size = 100
        t.uid = 123
        t.gid = 124
        info = t.get_info()
        header = t.create_pax_header(info, encoding="iso8859-1")
        self.assertEqual(info['name'], "foo")
        # mtime should be rounded to nearest second
        self.assertIsInstance(info['mtime'], int)
        self.assertEqual(info['mtime'], 1000)
        self.assertEqual(info['size'], 100)
        self.assertEqual(info['uid'], 123)
        self.assertEqual(info['gid'], 124)
        self.assertEqual(header,
            b'././@PaxHeader' + bytes(86) \
            + b'0000000\x000000000\x000000000\x0000000000020\x0000000000000\x00010205\x00 x' \
            + bytes(100) + b'ustar\x0000'+ bytes(247) \
            + b'16 mtime=1000.1\n' + bytes(496) + b'foo' + bytes(97) \
            + b'0000644\x000000173\x000000174\x0000000000144\x0000000001750\x00006516\x00 0' \
            + bytes(100) + b'ustar\x0000' + bytes(247))

        # values that should be changed
        t = tarfile.TarInfo()
        t.name = "foo\u3374" # can't be represented in ascii
        t.mtime = 10**10 # too big
        t.size = 10**10 # too big
        t.uid = 8**8 # too big
        t.gid = 8**8+1 # too big
        info = t.get_info()
        header = t.create_pax_header(info, encoding="iso8859-1")
        # name is kept as-is in info but should be added to pax header
        self.assertEqual(info['name'], "foo\u3374")
        self.assertEqual(info['mtime'], 0)
        self.assertEqual(info['size'], 0)
        self.assertEqual(info['uid'], 0)
        self.assertEqual(info['gid'], 0)
        self.assertEqual(header,
            b'././@PaxHeader' + bytes(86) \
            + b'0000000\x000000000\x000000000\x0000000000130\x0000000000000\x00010207\x00 x' \
            + bytes(100) + b'ustar\x0000' + bytes(247) \
            + b'15 path=foo\xe3\x8d\xb4\n16 uid=16777216\n' \
            + b'16 gid=16777217\n20 size=10000000000\n' \
            + b'21 mtime=10000000000\n'+ bytes(424) + b'foo?' + bytes(96) \
            + b'0000644\x000000000\x000000000\x0000000000000\x0000000000000\x00006540\x00 0' \
            + bytes(100) + b'ustar\x0000' + bytes(247))


class UnicodeTest:

    def test_iso8859_1_filename(self):
        self._test_unicode_filename("iso8859-1")

    def test_utf7_filename(self):
        self._test_unicode_filename("utf7")

    def test_utf8_filename(self):
        self._test_unicode_filename("utf-8")

    def _test_unicode_filename(self, encoding):
        tar = tarfile.open(tmpname, "w", format=self.format,
                           encoding=encoding, errors="strict")
        try:
            name = "\xe4\xf6\xfc"
            tar.addfile(tarfile.TarInfo(name))
        finally:
            tar.close()

        tar = tarfile.open(tmpname, encoding=encoding)
        try:
            self.assertEqual(tar.getmembers()[0].name, name)
        finally:
            tar.close()

    def test_unicode_filename_error(self):
        tar = tarfile.open(tmpname, "w", format=self.format,
                           encoding="ascii", errors="strict")
        try:
            tarinfo = tarfile.TarInfo()

            tarinfo.name = "\xe4\xf6\xfc"
            self.assertRaises(UnicodeError, tar.addfile, tarinfo)

            tarinfo.name = "foo"
            tarinfo.uname = "\xe4\xf6\xfc"
            self.assertRaises(UnicodeError, tar.addfile, tarinfo)
        finally:
            tar.close()

    def test_unicode_argument(self):
        tar = tarfile.open(tarname, "r",
                           encoding="iso8859-1", errors="strict")
        try:
            for t in tar:
                self.assertIs(type(t.name), str)
                self.assertIs(type(t.linkname), str)
                self.assertIs(type(t.uname), str)
                self.assertIs(type(t.gname), str)
        finally:
            tar.close()

    def test_uname_unicode(self):
        t = tarfile.TarInfo("foo")
        t.uname = "\xe4\xf6\xfc"
        t.gname = "\xe4\xf6\xfc"

        tar = tarfile.open(tmpname, mode="w", format=self.format,
                           encoding="iso8859-1")
        try:
            tar.addfile(t)
        finally:
            tar.close()

        tar = tarfile.open(tmpname, encoding="iso8859-1")
        try:
            t = tar.getmember("foo")
            self.assertEqual(t.uname, "\xe4\xf6\xfc")
            self.assertEqual(t.gname, "\xe4\xf6\xfc")

            if self.format != tarfile.PAX_FORMAT:
                tar.close()
                tar = tarfile.open(tmpname, encoding="ascii")
                t = tar.getmember("foo")
                self.assertEqual(t.uname, "\udce4\udcf6\udcfc")
                self.assertEqual(t.gname, "\udce4\udcf6\udcfc")
        finally:
            tar.close()


class UstarUnicodeTest(UnicodeTest, unittest.TestCase):

    format = tarfile.USTAR_FORMAT

    # Test whether the utf-8 encoded version of a filename exceeds the 100
    # bytes name field limit (every occurrence of '\xff' will be expanded to 2
    # bytes).
    def test_unicode_name1(self):
        self._test_ustar_name("0123456789" * 10)
        self._test_ustar_name("0123456789" * 10 + "0", ValueError)
        self._test_ustar_name("0123456789" * 9 + "01234567\xff")
        self._test_ustar_name("0123456789" * 9 + "012345678\xff", ValueError)

    def test_unicode_name2(self):
        self._test_ustar_name("0123456789" * 9 + "012345\xff\xff")
        self._test_ustar_name("0123456789" * 9 + "0123456\xff\xff", ValueError)

    # Test whether the utf-8 encoded version of a filename exceeds the 155
    # bytes prefix + '/' + 100 bytes name limit.
    def test_unicode_longname1(self):
        self._test_ustar_name("0123456789" * 15 + "01234/" + "0123456789" * 10)
        self._test_ustar_name("0123456789" * 15 + "0123/4" + "0123456789" * 10, ValueError)
        self._test_ustar_name("0123456789" * 15 + "012\xff/" + "0123456789" * 10)
        self._test_ustar_name("0123456789" * 15 + "0123\xff/" + "0123456789" * 10, ValueError)

    def test_unicode_longname2(self):
        self._test_ustar_name("0123456789" * 15 + "01\xff/2" + "0123456789" * 10, ValueError)
        self._test_ustar_name("0123456789" * 15 + "01\xff\xff/" + "0123456789" * 10, ValueError)

    def test_unicode_longname3(self):
        self._test_ustar_name("0123456789" * 15 + "01\xff\xff/2" + "0123456789" * 10, ValueError)
        self._test_ustar_name("0123456789" * 15 + "01234/" + "0123456789" * 9 + "01234567\xff")
        self._test_ustar_name("0123456789" * 15 + "01234/" + "0123456789" * 9 + "012345678\xff", ValueError)

    def test_unicode_longname4(self):
        self._test_ustar_name("0123456789" * 15 + "01234/" + "0123456789" * 9 + "012345\xff\xff")
        self._test_ustar_name("0123456789" * 15 + "01234/" + "0123456789" * 9 + "0123456\xff\xff", ValueError)

    def _test_ustar_name(self, name, exc=None):
        with tarfile.open(tmpname, "w", format=self.format, encoding="utf-8") as tar:
            t = tarfile.TarInfo(name)
            if exc is None:
                tar.addfile(t)
            else:
                self.assertRaises(exc, tar.addfile, t)

        if exc is None:
            with tarfile.open(tmpname, "r", encoding="utf-8") as tar:
                for t in tar:
                    self.assertEqual(name, t.name)
                    break

    # Test the same as above for the 100 bytes link field.
    def test_unicode_link1(self):
        self._test_ustar_link("0123456789" * 10)
        self._test_ustar_link("0123456789" * 10 + "0", ValueError)
        self._test_ustar_link("0123456789" * 9 + "01234567\xff")
        self._test_ustar_link("0123456789" * 9 + "012345678\xff", ValueError)

    def test_unicode_link2(self):
        self._test_ustar_link("0123456789" * 9 + "012345\xff\xff")
        self._test_ustar_link("0123456789" * 9 + "0123456\xff\xff", ValueError)

    def _test_ustar_link(self, name, exc=None):
        with tarfile.open(tmpname, "w", format=self.format, encoding="utf-8") as tar:
            t = tarfile.TarInfo("foo")
            t.linkname = name
            if exc is None:
                tar.addfile(t)
            else:
                self.assertRaises(exc, tar.addfile, t)

        if exc is None:
            with tarfile.open(tmpname, "r", encoding="utf-8") as tar:
                for t in tar:
                    self.assertEqual(name, t.linkname)
                    break


class GNUUnicodeTest(UnicodeTest, unittest.TestCase):

    format = tarfile.GNU_FORMAT

    def test_bad_pax_header(self):
        # Test for issue #8633. GNU tar <= 1.23 creates raw binary fields
        # without a hdrcharset=BINARY header.
        for encoding, name in (
                ("utf-8", "pax/bad-pax-\udce4\udcf6\udcfc"),
                ("iso8859-1", "pax/bad-pax-\xe4\xf6\xfc"),):
            with tarfile.open(tarname, encoding=encoding,
                              errors="surrogateescape") as tar:
                try:
                    t = tar.getmember(name)
                except KeyError:
                    self.fail("unable to read bad GNU tar pax header")


class PAXUnicodeTest(UnicodeTest, unittest.TestCase):

    format = tarfile.PAX_FORMAT

    # PAX_FORMAT ignores encoding in write mode.
    test_unicode_filename_error = None

    def test_binary_header(self):
        # Test a POSIX.1-2008 compatible header with a hdrcharset=BINARY field.
        for encoding, name in (
                ("utf-8", "pax/hdrcharset-\udce4\udcf6\udcfc"),
                ("iso8859-1", "pax/hdrcharset-\xe4\xf6\xfc"),):
            with tarfile.open(tarname, encoding=encoding,
                              errors="surrogateescape") as tar:
                try:
                    t = tar.getmember(name)
                except KeyError:
                    self.fail("unable to read POSIX.1-2008 binary header")


class AppendTestBase:
    # Test append mode (cp. patch #1652681).

    def setUp(self):
        self.tarname = tmpname
        if os.path.exists(self.tarname):
            os_helper.unlink(self.tarname)

    def _create_testtar(self, mode="w:"):
        with tarfile.open(tarname, encoding="iso8859-1") as src:
            t = src.getmember("ustar/regtype")
            t.name = "foo"
            with src.extractfile(t) as f:
                with tarfile.open(self.tarname, mode) as tar:
                    tar.addfile(t, f)

    def test_append_compressed(self):
        self._create_testtar("w:" + self.suffix)
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname, "a")

class AppendTest(AppendTestBase, unittest.TestCase):
    test_append_compressed = None

    def _add_testfile(self, fileobj=None):
        with tarfile.open(self.tarname, "a", fileobj=fileobj) as tar:
            tar.addfile(tarfile.TarInfo("bar"))

    def _test(self, names=["bar"], fileobj=None):
        with tarfile.open(self.tarname, fileobj=fileobj) as tar:
            self.assertEqual(tar.getnames(), names)

    def test_non_existing(self):
        self._add_testfile()
        self._test()

    def test_empty(self):
        tarfile.open(self.tarname, "w:").close()
        self._add_testfile()
        self._test()

    def test_empty_fileobj(self):
        fobj = io.BytesIO(b"\0" * 1024)
        self._add_testfile(fobj)
        fobj.seek(0)
        self._test(fileobj=fobj)

    def test_fileobj(self):
        self._create_testtar()
        with open(self.tarname, "rb") as fobj:
            data = fobj.read()
        fobj = io.BytesIO(data)
        self._add_testfile(fobj)
        fobj.seek(0)
        self._test(names=["foo", "bar"], fileobj=fobj)

    def test_existing(self):
        self._create_testtar()
        self._add_testfile()
        self._test(names=["foo", "bar"])

    # Append mode is supposed to fail if the tarfile to append to
    # does not end with a zero block.
    def _test_error(self, data):
        with open(self.tarname, "wb") as fobj:
            fobj.write(data)
        self.assertRaises(tarfile.ReadError, self._add_testfile)

    def test_null(self):
        self._test_error(b"")

    def test_incomplete(self):
        self._test_error(b"\0" * 13)

    def test_premature_eof(self):
        data = tarfile.TarInfo("foo").tobuf()
        self._test_error(data)

    def test_trailing_garbage(self):
        data = tarfile.TarInfo("foo").tobuf()
        self._test_error(data + b"\0" * 13)

    def test_invalid(self):
        self._test_error(b"a" * 512)

class GzipAppendTest(GzipTest, AppendTestBase, unittest.TestCase):
    pass

class Bz2AppendTest(Bz2Test, AppendTestBase, unittest.TestCase):
    pass

class LzmaAppendTest(LzmaTest, AppendTestBase, unittest.TestCase):
    pass

class ZstdAppendTest(ZstdTest, AppendTestBase, unittest.TestCase):
    pass

class LimitsTest(unittest.TestCase):

    def test_ustar_limits(self):
        # 100 char name
        tarinfo = tarfile.TarInfo("0123456789" * 10)
        tarinfo.tobuf(tarfile.USTAR_FORMAT)

        # 101 char name that cannot be stored
        tarinfo = tarfile.TarInfo("0123456789" * 10 + "0")
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.USTAR_FORMAT)

        # 256 char name with a slash at pos 156
        tarinfo = tarfile.TarInfo("123/" * 62 + "longname")
        tarinfo.tobuf(tarfile.USTAR_FORMAT)

        # 256 char name that cannot be stored
        tarinfo = tarfile.TarInfo("1234567/" * 31 + "longname")
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.USTAR_FORMAT)

        # 512 char name
        tarinfo = tarfile.TarInfo("123/" * 126 + "longname")
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.USTAR_FORMAT)

        # 512 char linkname
        tarinfo = tarfile.TarInfo("longlink")
        tarinfo.linkname = "123/" * 126 + "longname"
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.USTAR_FORMAT)

        # uid > 8 digits
        tarinfo = tarfile.TarInfo("name")
        tarinfo.uid = 0o10000000
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.USTAR_FORMAT)

    def test_gnu_limits(self):
        tarinfo = tarfile.TarInfo("123/" * 126 + "longname")
        tarinfo.tobuf(tarfile.GNU_FORMAT)

        tarinfo = tarfile.TarInfo("longlink")
        tarinfo.linkname = "123/" * 126 + "longname"
        tarinfo.tobuf(tarfile.GNU_FORMAT)

        # uid >= 256 ** 7
        tarinfo = tarfile.TarInfo("name")
        tarinfo.uid = 0o4000000000000000000
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.GNU_FORMAT)

    def test_pax_limits(self):
        tarinfo = tarfile.TarInfo("123/" * 126 + "longname")
        tarinfo.tobuf(tarfile.PAX_FORMAT)

        tarinfo = tarfile.TarInfo("longlink")
        tarinfo.linkname = "123/" * 126 + "longname"
        tarinfo.tobuf(tarfile.PAX_FORMAT)

        tarinfo = tarfile.TarInfo("name")
        tarinfo.uid = 0o4000000000000000000
        tarinfo.tobuf(tarfile.PAX_FORMAT)


class MiscTest(unittest.TestCase):

    def test_char_fields(self):
        self.assertEqual(tarfile.stn("foo", 8, "ascii", "strict"),
                         b"foo\0\0\0\0\0")
        self.assertEqual(tarfile.stn("foobar", 3, "ascii", "strict"),
                         b"foo")
        self.assertEqual(tarfile.nts(b"foo\0\0\0\0\0", "ascii", "strict"),
                         "foo")
        self.assertEqual(tarfile.nts(b"foo\0bar\0", "ascii", "strict"),
                         "foo")

    def test_read_number_fields(self):
        # Issue 13158: Test if GNU tar specific base-256 number fields
        # are decoded correctly.
        self.assertEqual(tarfile.nti(b"0000001\x00"), 1)
        self.assertEqual(tarfile.nti(b"7777777\x00"), 0o7777777)
        self.assertEqual(tarfile.nti(b"\x80\x00\x00\x00\x00\x20\x00\x00"),
                         0o10000000)
        self.assertEqual(tarfile.nti(b"\x80\x00\x00\x00\xff\xff\xff\xff"),
                         0xffffffff)
        self.assertEqual(tarfile.nti(b"\xff\xff\xff\xff\xff\xff\xff\xff"),
                         -1)
        self.assertEqual(tarfile.nti(b"\xff\xff\xff\xff\xff\xff\xff\x9c"),
                         -100)
        self.assertEqual(tarfile.nti(b"\xff\x00\x00\x00\x00\x00\x00\x00"),
                         -0x100000000000000)

        # Issue 24514: Test if empty number fields are converted to zero.
        self.assertEqual(tarfile.nti(b"\0"), 0)
        self.assertEqual(tarfile.nti(b"       \0"), 0)

    def test_write_number_fields(self):
        self.assertEqual(tarfile.itn(1), b"0000001\x00")
        self.assertEqual(tarfile.itn(0o7777777), b"7777777\x00")
        self.assertEqual(tarfile.itn(0o10000000, format=tarfile.GNU_FORMAT),
                         b"\x80\x00\x00\x00\x00\x20\x00\x00")
        self.assertEqual(tarfile.itn(0xffffffff, format=tarfile.GNU_FORMAT),
                         b"\x80\x00\x00\x00\xff\xff\xff\xff")
        self.assertEqual(tarfile.itn(-1, format=tarfile.GNU_FORMAT),
                         b"\xff\xff\xff\xff\xff\xff\xff\xff")
        self.assertEqual(tarfile.itn(-100, format=tarfile.GNU_FORMAT),
                         b"\xff\xff\xff\xff\xff\xff\xff\x9c")
        self.assertEqual(tarfile.itn(-0x100000000000000,
                                     format=tarfile.GNU_FORMAT),
                         b"\xff\x00\x00\x00\x00\x00\x00\x00")

        # Issue 32713: Test if itn() supports float values outside the
        # non-GNU format range
        self.assertEqual(tarfile.itn(-100.0, format=tarfile.GNU_FORMAT),
                         b"\xff\xff\xff\xff\xff\xff\xff\x9c")
        self.assertEqual(tarfile.itn(8 ** 12 + 0.0, format=tarfile.GNU_FORMAT),
                         b"\x80\x00\x00\x10\x00\x00\x00\x00")
        self.assertEqual(tarfile.nti(tarfile.itn(-0.1, format=tarfile.GNU_FORMAT)), 0)

    def test_number_field_limits(self):
        with self.assertRaises(ValueError):
            tarfile.itn(-1, 8, tarfile.USTAR_FORMAT)
        with self.assertRaises(ValueError):
            tarfile.itn(0o10000000, 8, tarfile.USTAR_FORMAT)
        with self.assertRaises(ValueError):
            tarfile.itn(-0x10000000001, 6, tarfile.GNU_FORMAT)
        with self.assertRaises(ValueError):
            tarfile.itn(0x10000000000, 6, tarfile.GNU_FORMAT)

    def test__all__(self):
        not_exported = {
            'version', 'grp', 'pwd', 'symlink_exception', 'NUL', 'BLOCKSIZE',
            'RECORDSIZE', 'GNU_MAGIC', 'POSIX_MAGIC', 'LENGTH_NAME',
            'LENGTH_LINK', 'LENGTH_PREFIX', 'REGTYPE', 'AREGTYPE', 'LNKTYPE',
            'SYMTYPE', 'CHRTYPE', 'BLKTYPE', 'DIRTYPE', 'FIFOTYPE', 'CONTTYPE',
            'GNUTYPE_LONGNAME', 'GNUTYPE_LONGLINK', 'GNUTYPE_SPARSE',
            'XHDTYPE', 'XGLTYPE', 'SOLARIS_XHDTYPE', 'SUPPORTED_TYPES',
            'REGULAR_TYPES', 'GNU_TYPES', 'PAX_FIELDS', 'PAX_NAME_FIELDS',
            'PAX_NUMBER_FIELDS', 'stn', 'nts', 'nti', 'itn', 'calc_chksums',
            'copyfileobj', 'filemode', 'EmptyHeaderError',
            'TruncatedHeaderError', 'EOFHeaderError', 'InvalidHeaderError',
            'SubsequentHeaderError', 'ExFileObject', 'main'}
        support.check__all__(self, tarfile, not_exported=not_exported)

    def test_useful_error_message_when_modules_missing(self):
        fname = os.path.join(os.path.dirname(__file__), 'archivetestdata', 'testtar.tar.xz')
        with self.assertRaises(tarfile.ReadError) as excinfo:
            error = tarfile.CompressionError('lzma module is not available'),
            with unittest.mock.patch.object(tarfile.TarFile, 'xzopen', side_effect=error):
                tarfile.open(fname)

        self.assertIn(
            "\n- method xz: CompressionError('lzma module is not available')\n",
            str(excinfo.exception),
        )


class CommandLineTest(unittest.TestCase):

    def tarfilecmd(self, *args, **kwargs):
        rc, out, err = script_helper.assert_python_ok('-m', 'tarfile', *args,
                                                      **kwargs)
        return out.replace(os.linesep.encode(), b'\n')

    def tarfilecmd_failure(self, *args):
        return script_helper.assert_python_failure('-m', 'tarfile', *args)

    def make_simple_tarfile(self, tar_name):
        files = [support.findfile('tokenize_tests.txt',
                                  subdir='tokenizedata'),
                 support.findfile('tokenize_tests-no-coding-cookie-'
                                  'and-utf8-bom-sig-only.txt',
                                  subdir='tokenizedata')]
        self.addCleanup(os_helper.unlink, tar_name)
        with tarfile.open(tar_name, 'w') as tf:
            for tardata in files:
                tf.add(tardata, arcname=os.path.basename(tardata))

    def make_evil_tarfile(self, tar_name):
        self.addCleanup(os_helper.unlink, tar_name)
        with tarfile.open(tar_name, 'w') as tf:
            benign = tarfile.TarInfo('benign')
            tf.addfile(benign, fileobj=io.BytesIO(b''))
            evil = tarfile.TarInfo('../evil')
            tf.addfile(evil, fileobj=io.BytesIO(b''))

    def test_bad_use(self):
        rc, out, err = self.tarfilecmd_failure()
        self.assertEqual(out, b'')
        self.assertIn(b'usage', err.lower())
        self.assertIn(b'error', err.lower())
        self.assertIn(b'required', err.lower())
        rc, out, err = self.tarfilecmd_failure('-l', '')
        self.assertEqual(out, b'')
        self.assertNotEqual(err.strip(), b'')

    def test_test_command(self):
        for tar_name in testtarnames:
            for opt in '-t', '--test':
                out = self.tarfilecmd(opt, tar_name)
                self.assertEqual(out, b'')

    def test_test_command_verbose(self):
        for tar_name in testtarnames:
            for opt in '-v', '--verbose':
                out = self.tarfilecmd(opt, '-t', tar_name,
                                      PYTHONIOENCODING='utf-8')
                self.assertIn(b'is a tar archive.\n', out)

    def test_test_command_invalid_file(self):
        zipname = support.findfile('zipdir.zip', subdir='archivetestdata')
        rc, out, err = self.tarfilecmd_failure('-t', zipname)
        self.assertIn(b' is not a tar archive.', err)
        self.assertEqual(out, b'')
        self.assertEqual(rc, 1)

        for tar_name in testtarnames:
            with self.subTest(tar_name=tar_name):
                with open(tar_name, 'rb') as f:
                    data = f.read()
                try:
                    with open(tmpname, 'wb') as f:
                        f.write(data[:511])
                    rc, out, err = self.tarfilecmd_failure('-t', tmpname)
                    self.assertEqual(out, b'')
                    self.assertEqual(rc, 1)
                finally:
                    os_helper.unlink(tmpname)

    def test_list_command(self):
        for tar_name in testtarnames:
            with support.captured_stdout() as t:
                with tarfile.open(tar_name, 'r') as tf:
                    tf.list(verbose=False)
            expected = t.getvalue().encode('ascii', 'backslashreplace')
            for opt in '-l', '--list':
                out = self.tarfilecmd(opt, tar_name,
                                      PYTHONIOENCODING='ascii')
                self.assertEqual(out, expected)

    def test_list_command_verbose(self):
        for tar_name in testtarnames:
            with support.captured_stdout() as t:
                with tarfile.open(tar_name, 'r') as tf:
                    tf.list(verbose=True)
            expected = t.getvalue().encode('ascii', 'backslashreplace')
            for opt in '-v', '--verbose':
                out = self.tarfilecmd(opt, '-l', tar_name,
                                      PYTHONIOENCODING='ascii')
                self.assertEqual(out, expected)

    def test_list_command_invalid_file(self):
        zipname = support.findfile('zipdir.zip', subdir='archivetestdata')
        rc, out, err = self.tarfilecmd_failure('-l', zipname)
        self.assertIn(b' is not a tar archive.', err)
        self.assertEqual(out, b'')
        self.assertEqual(rc, 1)

    def test_create_command(self):
        files = [support.findfile('tokenize_tests.txt',
                                  subdir='tokenizedata'),
                 support.findfile('tokenize_tests-no-coding-cookie-'
                                  'and-utf8-bom-sig-only.txt',
                                  subdir='tokenizedata')]
        for opt in '-c', '--create':
            try:
                out = self.tarfilecmd(opt, tmpname, *files)
                self.assertEqual(out, b'')
                with tarfile.open(tmpname) as tar:
                    tar.getmembers()
            finally:
                os_helper.unlink(tmpname)

    def test_create_command_verbose(self):
        files = [support.findfile('tokenize_tests.txt',
                                  subdir='tokenizedata'),
                 support.findfile('tokenize_tests-no-coding-cookie-'
                                  'and-utf8-bom-sig-only.txt',
                                  subdir='tokenizedata')]
        for opt in '-v', '--verbose':
            try:
                out = self.tarfilecmd(opt, '-c', tmpname, *files,
                                      PYTHONIOENCODING='utf-8')
                self.assertIn(b' file created.', out)
                with tarfile.open(tmpname) as tar:
                    tar.getmembers()
            finally:
                os_helper.unlink(tmpname)

    def test_create_command_dotless_filename(self):
        files = [support.findfile('tokenize_tests.txt', subdir='tokenizedata')]
        try:
            out = self.tarfilecmd('-c', dotlessname, *files)
            self.assertEqual(out, b'')
            with tarfile.open(dotlessname) as tar:
                tar.getmembers()
        finally:
            os_helper.unlink(dotlessname)

    def test_create_command_dot_started_filename(self):
        tar_name = os.path.join(TEMPDIR, ".testtar")
        files = [support.findfile('tokenize_tests.txt', subdir='tokenizedata')]
        try:
            out = self.tarfilecmd('-c', tar_name, *files)
            self.assertEqual(out, b'')
            with tarfile.open(tar_name) as tar:
                tar.getmembers()
        finally:
            os_helper.unlink(tar_name)

    def test_create_command_compressed(self):
        files = [support.findfile('tokenize_tests.txt',
                                  subdir='tokenizedata'),
                 support.findfile('tokenize_tests-no-coding-cookie-'
                                  'and-utf8-bom-sig-only.txt',
                                  subdir='tokenizedata')]
        for filetype in (GzipTest, Bz2Test, LzmaTest, ZstdTest):
            if not filetype.open:
                continue
            try:
                tar_name = tmpname + '.' + filetype.suffix
                out = self.tarfilecmd('-c', tar_name, *files)
                with filetype.taropen(tar_name) as tar:
                    tar.getmembers()
            finally:
                os_helper.unlink(tar_name)

    def test_extract_command(self):
        self.make_simple_tarfile(tmpname)
        for opt in '-e', '--extract':
            try:
                with os_helper.temp_cwd(tarextdir):
                    out = self.tarfilecmd(opt, tmpname)
                self.assertEqual(out, b'')
            finally:
                os_helper.rmtree(tarextdir)

    def test_extract_command_verbose(self):
        self.make_simple_tarfile(tmpname)
        for opt in '-v', '--verbose':
            try:
                with os_helper.temp_cwd(tarextdir):
                    out = self.tarfilecmd(opt, '-e', tmpname,
                                          PYTHONIOENCODING='utf-8')
                self.assertIn(b' file is extracted.', out)
            finally:
                os_helper.rmtree(tarextdir)

    def test_extract_command_filter(self):
        self.make_evil_tarfile(tmpname)
        # Make an inner directory, so the member named '../evil'
        # is still extracted into `tarextdir`
        destdir = os.path.join(tarextdir, 'dest')
        os.mkdir(tarextdir)
        try:
            with os_helper.temp_cwd(destdir):
                self.tarfilecmd_failure('-e', tmpname,
                                        '-v',
                                        '--filter', 'data')
                out = self.tarfilecmd('-e', tmpname,
                                      '-v',
                                      '--filter', 'fully_trusted',
                                      PYTHONIOENCODING='utf-8')
                self.assertIn(b' file is extracted.', out)
        finally:
            os_helper.rmtree(tarextdir)

    def test_extract_command_different_directory(self):
        self.make_simple_tarfile(tmpname)
        try:
            with os_helper.temp_cwd(tarextdir):
                out = self.tarfilecmd('-e', tmpname, 'spamdir')
            self.assertEqual(out, b'')
        finally:
            os_helper.rmtree(tarextdir)

    def test_extract_command_invalid_file(self):
        zipname = support.findfile('zipdir.zip', subdir='archivetestdata')
        with os_helper.temp_cwd(tarextdir):
            rc, out, err = self.tarfilecmd_failure('-e', zipname)
        self.assertIn(b' is not a tar archive.', err)
        self.assertEqual(out, b'')
        self.assertEqual(rc, 1)


class ContextManagerTest(unittest.TestCase):

    def test_basic(self):
        with tarfile.open(tarname) as tar:
            self.assertFalse(tar.closed, "closed inside runtime context")
        self.assertTrue(tar.closed, "context manager failed")

    def test_closed(self):
        # The __enter__() method is supposed to raise OSError
        # if the TarFile object is already closed.
        tar = tarfile.open(tarname)
        tar.close()
        with self.assertRaises(OSError):
            with tar:
                pass

    def test_exception(self):
        # Test if the OSError exception is passed through properly.
        with self.assertRaises(Exception) as exc:
            with tarfile.open(tarname) as tar:
                raise OSError
        self.assertIsInstance(exc.exception, OSError,
                              "wrong exception raised in context manager")
        self.assertTrue(tar.closed, "context manager failed")

    def test_no_eof(self):
        # __exit__() must not write end-of-archive blocks if an
        # exception was raised.
        try:
            with tarfile.open(tmpname, "w") as tar:
                raise Exception
        except:
            pass
        self.assertEqual(os.path.getsize(tmpname), 0,
                "context manager wrote an end-of-archive block")
        self.assertTrue(tar.closed, "context manager failed")

    def test_eof(self):
        # __exit__() must write end-of-archive blocks, i.e. call
        # TarFile.close() if there was no error.
        with tarfile.open(tmpname, "w"):
            pass
        self.assertNotEqual(os.path.getsize(tmpname), 0,
                "context manager wrote no end-of-archive block")

    def test_fileobj(self):
        # Test that __exit__() did not close the external file
        # object.
        with open(tmpname, "wb") as fobj:
            try:
                with tarfile.open(fileobj=fobj, mode="w") as tar:
                    raise Exception
            except:
                pass
            self.assertFalse(fobj.closed, "external file object was closed")
            self.assertTrue(tar.closed, "context manager failed")


@unittest.skipIf(hasattr(os, "link"), "requires os.link to be missing")
class LinkEmulationTest(ReadTest, unittest.TestCase):

    # Test for issue #8741 regression. On platforms that do not support
    # symbolic or hard links tarfile tries to extract these types of members
    # as the regular files they point to.
    def _test_link_extraction(self, name):
        self.tar.extract(name, TEMPDIR, filter='fully_trusted')
        with open(os.path.join(TEMPDIR, name), "rb") as f:
            data = f.read()
        self.assertEqual(sha256sum(data), sha256_regtype)

    # See issues #1578269, #8879, and #17689 for some history on these skips
    @unittest.skipIf(hasattr(os.path, "islink"),
                     "Skip emulation - has os.path.islink but not os.link")
    def test_hardlink_extraction1(self):
        self._test_link_extraction("ustar/lnktype")

    @unittest.skipIf(hasattr(os.path, "islink"),
                     "Skip emulation - has os.path.islink but not os.link")
    def test_hardlink_extraction2(self):
        self._test_link_extraction("./ustar/linktest2/lnktype")

    @unittest.skipIf(hasattr(os, "symlink"),
                     "Skip emulation if symlink exists")
    def test_symlink_extraction1(self):
        self._test_link_extraction("ustar/symtype")

    @unittest.skipIf(hasattr(os, "symlink"),
                     "Skip emulation if symlink exists")
    def test_symlink_extraction2(self):
        self._test_link_extraction("./ustar/linktest2/symtype")


class Bz2PartialReadTest(Bz2Test, unittest.TestCase):
    # Issue5068: The _BZ2Proxy.read() method loops forever
    # on an empty or partial bzipped file.

    def _test_partial_input(self, mode):
        class MyBytesIO(io.BytesIO):
            hit_eof = False
            def read(self, n):
                if self.hit_eof:
                    raise AssertionError("infinite loop detected in "
                                         "tarfile.open()")
                self.hit_eof = self.tell() == len(self.getvalue())
                return super(MyBytesIO, self).read(n)
            def seek(self, *args):
                self.hit_eof = False
                return super(MyBytesIO, self).seek(*args)

        data = bz2.compress(tarfile.TarInfo("foo").tobuf())
        for x in range(len(data) + 1):
            try:
                tarfile.open(fileobj=MyBytesIO(data[:x]), mode=mode)
            except tarfile.ReadError:
                pass # we have no interest in ReadErrors

    def test_partial_input(self):
        self._test_partial_input("r")

    def test_partial_input_bz2(self):
        self._test_partial_input("r:bz2")


def root_is_uid_gid_0():
    try:
        import pwd, grp
    except ImportError:
        return False
    if pwd.getpwuid(0)[0] != 'root':
        return False
    if grp.getgrgid(0)[0] != 'root':
        return False
    return True


@unittest.skipUnless(hasattr(os, 'chown'), "missing os.chown")
@unittest.skipUnless(hasattr(os, 'geteuid'), "missing os.geteuid")
class NumericOwnerTest(unittest.TestCase):
    # mock the following:
    #  os.chown: so we can test what's being called
    #  os.chmod: so the modes are not actually changed. if they are, we can't
    #             delete the files/directories
    #  os.geteuid: so we can lie and say we're root (uid = 0)

    @staticmethod
    def _make_test_archive(filename_1, dirname_1, filename_2):
        # the file contents to write
        fobj = io.BytesIO(b"content")

        # create a tar file with a file, a directory, and a file within that
        #  directory. Assign various .uid/.gid values to them
        items = [(filename_1, 99, 98, tarfile.REGTYPE, fobj),
                 (dirname_1,  77, 76, tarfile.DIRTYPE, None),
                 (filename_2, 88, 87, tarfile.REGTYPE, fobj),
                 ]
        with tarfile.open(tmpname, 'w') as tarfl:
            for name, uid, gid, typ, contents in items:
                t = tarfile.TarInfo(name)
                t.uid = uid
                t.gid = gid
                t.uname = 'root'
                t.gname = 'root'
                t.type = typ
                tarfl.addfile(t, contents)

        # return the full pathname to the tar file
        return tmpname

    @staticmethod
    @contextmanager
    def _setup_test(mock_geteuid):
        mock_geteuid.return_value = 0  # lie and say we're root
        fname = 'numeric-owner-testfile'
        dirname = 'dir'

        # the names we want stored in the tarfile
        filename_1 = fname
        dirname_1 = dirname
        filename_2 = os.path.join(dirname, fname)

        # create the tarfile with the contents we're after
        tar_filename = NumericOwnerTest._make_test_archive(filename_1,
                                                           dirname_1,
                                                           filename_2)

        # open the tarfile for reading. yield it and the names of the items
        #  we stored into the file
        with tarfile.open(tar_filename) as tarfl:
            yield tarfl, filename_1, dirname_1, filename_2

    @unittest.mock.patch('os.chown')
    @unittest.mock.patch('os.chmod')
    @unittest.mock.patch('os.geteuid')
    def test_extract_with_numeric_owner(self, mock_geteuid, mock_chmod,
                                        mock_chown):
        with self._setup_test(mock_geteuid) as (tarfl, filename_1, _,
                                                filename_2):
            tarfl.extract(filename_1, TEMPDIR, numeric_owner=True,
                          filter='fully_trusted')
            tarfl.extract(filename_2 , TEMPDIR, numeric_owner=True,
                          filter='fully_trusted')

        # convert to filesystem paths
        f_filename_1 = os.path.join(TEMPDIR, filename_1)
        f_filename_2 = os.path.join(TEMPDIR, filename_2)

        mock_chown.assert_has_calls([unittest.mock.call(f_filename_1, 99, 98),
                                     unittest.mock.call(f_filename_2, 88, 87),
                                     ],
                                    any_order=True)

    @unittest.mock.patch('os.chown')
    @unittest.mock.patch('os.chmod')
    @unittest.mock.patch('os.geteuid')
    def test_extractall_with_numeric_owner(self, mock_geteuid, mock_chmod,
                                           mock_chown):
        with self._setup_test(mock_geteuid) as (tarfl, filename_1, dirname_1,
                                                filename_2):
            tarfl.extractall(TEMPDIR, numeric_owner=True,
                             filter='fully_trusted')

        # convert to filesystem paths
        f_filename_1 = os.path.join(TEMPDIR, filename_1)
        f_dirname_1  = os.path.join(TEMPDIR, dirname_1)
        f_filename_2 = os.path.join(TEMPDIR, filename_2)

        mock_chown.assert_has_calls([unittest.mock.call(f_filename_1, 99, 98),
                                     unittest.mock.call(f_dirname_1, 77, 76),
                                     unittest.mock.call(f_filename_2, 88, 87),
                                     ],
                                    any_order=True)

    # this test requires that uid=0 and gid=0 really be named 'root'. that's
    #  because the uname and gname in the test file are 'root', and extract()
    #  will look them up using pwd and grp to find their uid and gid, which we
    #  test here to be 0.
    @unittest.skipUnless(root_is_uid_gid_0(),
                         'uid=0,gid=0 must be named "root"')
    @unittest.mock.patch('os.chown')
    @unittest.mock.patch('os.chmod')
    @unittest.mock.patch('os.geteuid')
    def test_extract_without_numeric_owner(self, mock_geteuid, mock_chmod,
                                           mock_chown):
        with self._setup_test(mock_geteuid) as (tarfl, filename_1, _, _):
            tarfl.extract(filename_1, TEMPDIR, numeric_owner=False,
                          filter='fully_trusted')

        # convert to filesystem paths
        f_filename_1 = os.path.join(TEMPDIR, filename_1)

        mock_chown.assert_called_with(f_filename_1, 0, 0)

    @unittest.mock.patch('os.geteuid')
    def test_keyword_only(self, mock_geteuid):
        with self._setup_test(mock_geteuid) as (tarfl, filename_1, _, _):
            self.assertRaises(TypeError,
                              tarfl.extract, filename_1, TEMPDIR, False, True)


class ReplaceTests(ReadTest, unittest.TestCase):
    def test_replace_name(self):
        member = self.tar.getmember('ustar/regtype')
        replaced = member.replace(name='misc/other')
        self.assertEqual(replaced.name, 'misc/other')
        self.assertEqual(member.name, 'ustar/regtype')
        self.assertEqual(self.tar.getmember('ustar/regtype').name,
                         'ustar/regtype')

    def test_replace_deep(self):
        member = self.tar.getmember('pax/regtype1')
        replaced = member.replace()
        replaced.pax_headers['gname'] = 'not-bar'
        self.assertEqual(member.pax_headers['gname'], 'bar')
        self.assertEqual(
            self.tar.getmember('pax/regtype1').pax_headers['gname'], 'bar')

    def test_replace_shallow(self):
        member = self.tar.getmember('pax/regtype1')
        replaced = member.replace(deep=False)
        replaced.pax_headers['gname'] = 'not-bar'
        self.assertEqual(member.pax_headers['gname'], 'not-bar')
        self.assertEqual(
            self.tar.getmember('pax/regtype1').pax_headers['gname'], 'not-bar')

    def test_replace_all(self):
        member = self.tar.getmember('ustar/regtype')
        for attr_name in ('name', 'mtime', 'mode', 'linkname',
                          'uid', 'gid', 'uname', 'gname'):
            with self.subTest(attr_name=attr_name):
                replaced = member.replace(**{attr_name: None})
                self.assertEqual(getattr(replaced, attr_name), None)
                self.assertNotEqual(getattr(member, attr_name), None)

    def test_replace_internal(self):
        member = self.tar.getmember('ustar/regtype')
        with self.assertRaises(TypeError):
            member.replace(offset=123456789)


class NoneInfoExtractTests(ReadTest):
    # These mainly check that all kinds of members are extracted successfully
    # if some metadata is None.
    # Some of the methods do additional spot checks.

    # We also test that the default filters can deal with None.

    extraction_filter = None

    @classmethod
    def setUpClass(cls):
        tar = tarfile.open(tarname, mode='r', encoding="iso8859-1")
        cls.control_dir = pathlib.Path(TEMPDIR) / "extractall_ctrl"
        tar.errorlevel = 0
        with ExitStack() as cm:
            if cls.extraction_filter is None:
                cm.enter_context(warnings.catch_warnings(
                    action="ignore", category=DeprecationWarning))
            tar.extractall(cls.control_dir, filter=cls.extraction_filter)
        tar.close()
        cls.control_paths = set(
            p.relative_to(cls.control_dir)
            for p in pathlib.Path(cls.control_dir).glob('**/*'))

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.control_dir)

    def check_files_present(self, directory):
        got_paths = set(
            p.relative_to(directory)
            for p in pathlib.Path(directory).glob('**/*'))
        self.assertEqual(self.control_paths, got_paths)

    @contextmanager
    def extract_with_none(self, *attr_names):
        DIR = pathlib.Path(TEMPDIR) / "extractall_none"
        self.tar.errorlevel = 0
        for member in self.tar.getmembers():
            for attr_name in attr_names:
                setattr(member, attr_name, None)
        with os_helper.temp_dir(DIR):
            self.tar.extractall(DIR, filter='fully_trusted')
            self.check_files_present(DIR)
            yield DIR

    def test_extractall_none_mtime(self):
        # mtimes of extracted files should be later than 'now' -- the mtime
        # of a previously created directory.
        now = pathlib.Path(TEMPDIR).stat().st_mtime
        with self.extract_with_none('mtime') as DIR:
            for path in pathlib.Path(DIR).glob('**/*'):
                with self.subTest(path=path):
                    try:
                        mtime = path.stat().st_mtime
                    except OSError:
                        # Some systems can't stat symlinks, ignore those
                        if not path.is_symlink():
                            raise
                    else:
                        self.assertGreaterEqual(path.stat().st_mtime, now)

    def test_extractall_none_mode(self):
        # modes of directories and regular files should match the mode
        # of a "normally" created directory or regular file
        dir_mode = pathlib.Path(TEMPDIR).stat().st_mode
        regular_file = pathlib.Path(TEMPDIR) / 'regular_file'
        regular_file.write_text('')
        regular_file_mode = regular_file.stat().st_mode
        with self.extract_with_none('mode') as DIR:
            for path in pathlib.Path(DIR).glob('**/*'):
                with self.subTest(path=path):
                    if path.is_dir():
                        self.assertEqual(path.stat().st_mode, dir_mode)
                    elif path.is_file():
                        self.assertEqual(path.stat().st_mode,
                                         regular_file_mode)

    def test_extractall_none_uid(self):
        with self.extract_with_none('uid'):
            pass

    def test_extractall_none_gid(self):
        with self.extract_with_none('gid'):
            pass

    def test_extractall_none_uname(self):
        with self.extract_with_none('uname'):
            pass

    def test_extractall_none_gname(self):
        with self.extract_with_none('gname'):
            pass

    def test_extractall_none_ownership(self):
        with self.extract_with_none('uid', 'gid', 'uname', 'gname'):
            pass

class NoneInfoExtractTests_Data(NoneInfoExtractTests, unittest.TestCase):
    extraction_filter = 'data'

class NoneInfoExtractTests_FullyTrusted(NoneInfoExtractTests,
                                        unittest.TestCase):
    extraction_filter = 'fully_trusted'

class NoneInfoExtractTests_Tar(NoneInfoExtractTests, unittest.TestCase):
    extraction_filter = 'tar'

class NoneInfoExtractTests_Default(NoneInfoExtractTests,
                                   unittest.TestCase):
    extraction_filter = None

class NoneInfoTests_Misc(unittest.TestCase):
    def test_add(self):
        # When addfile() encounters None metadata, it raises a ValueError
        bio = io.BytesIO()
        for tarformat in (tarfile.USTAR_FORMAT, tarfile.GNU_FORMAT,
                          tarfile.PAX_FORMAT):
            with self.subTest(tarformat=tarformat):
                tar = tarfile.open(fileobj=bio, mode='w', format=tarformat)
                tarinfo = tar.gettarinfo(tarname)
                try:
                    with open(tarname, 'rb') as f:
                        tar.addfile(tarinfo, f)
                except Exception:
                    if tarformat == tarfile.USTAR_FORMAT:
                        # In the old, limited format, adding might fail for
                        # reasons like the UID being too large
                        pass
                    else:
                        raise
                else:
                    for attr_name in ('mtime', 'mode', 'uid', 'gid',
                                    'uname', 'gname'):
                        with self.subTest(attr_name=attr_name):
                            replaced = tarinfo.replace(**{attr_name: None})
                            with self.assertRaisesRegex(ValueError,
                                                        f"{attr_name}"):
                                with open(tarname, 'rb') as f:
                                    tar.addfile(replaced, f)

    def test_list(self):
        # Change some metadata to None, then compare list() output
        # word-for-word. We want list() to not raise, and to only change
        # printout for the affected piece of metadata.
        # (n.b.: some contents of the test archive are hardcoded.)
        for attr_names in ({'mtime'}, {'mode'}, {'uid'}, {'gid'},
                           {'uname'}, {'gname'},
                           {'uid', 'uname'}, {'gid', 'gname'}):
            with (self.subTest(attr_names=attr_names),
                  tarfile.open(tarname, encoding="iso8859-1") as tar):
                tio_prev = io.TextIOWrapper(io.BytesIO(), 'ascii', newline='\n')
                with support.swap_attr(sys, 'stdout', tio_prev):
                    tar.list()
                for member in tar.getmembers():
                    for attr_name in attr_names:
                        setattr(member, attr_name, None)
                tio_new = io.TextIOWrapper(io.BytesIO(), 'ascii', newline='\n')
                with support.swap_attr(sys, 'stdout', tio_new):
                    tar.list()
                for expected, got in zip(tio_prev.detach().getvalue().split(),
                                         tio_new.detach().getvalue().split()):
                    if attr_names == {'mtime'} and re.match(rb'2003-01-\d\d', expected):
                        self.assertEqual(got, b'????-??-??')
                    elif attr_names == {'mtime'} and re.match(rb'\d\d:\d\d:\d\d', expected):
                        self.assertEqual(got, b'??:??:??')
                    elif attr_names == {'mode'} and re.match(
                            rb'.([r-][w-][x-]){3}', expected):
                        self.assertEqual(got, b'??????????')
                    elif attr_names == {'uname'} and expected.startswith(
                            (b'tarfile/', b'lars/', b'foo/')):
                        exp_user, exp_group = expected.split(b'/')
                        got_user, got_group = got.split(b'/')
                        self.assertEqual(got_group, exp_group)
                        self.assertRegex(got_user, b'[0-9]+')
                    elif attr_names == {'gname'} and expected.endswith(
                            (b'/tarfile', b'/users', b'/bar')):
                        exp_user, exp_group = expected.split(b'/')
                        got_user, got_group = got.split(b'/')
                        self.assertEqual(got_user, exp_user)
                        self.assertRegex(got_group, b'[0-9]+')
                    elif attr_names == {'uid'} and expected.startswith(
                            (b'1000/')):
                        exp_user, exp_group = expected.split(b'/')
                        got_user, got_group = got.split(b'/')
                        self.assertEqual(got_group, exp_group)
                        self.assertEqual(got_user, b'None')
                    elif attr_names == {'gid'} and expected.endswith((b'/100')):
                        exp_user, exp_group = expected.split(b'/')
                        got_user, got_group = got.split(b'/')
                        self.assertEqual(got_user, exp_user)
                        self.assertEqual(got_group, b'None')
                    elif attr_names == {'uid', 'uname'} and expected.startswith(
                            (b'tarfile/', b'lars/', b'foo/', b'1000/')):
                        exp_user, exp_group = expected.split(b'/')
                        got_user, got_group = got.split(b'/')
                        self.assertEqual(got_group, exp_group)
                        self.assertEqual(got_user, b'None')
                    elif attr_names == {'gname', 'gid'} and expected.endswith(
                            (b'/tarfile', b'/users', b'/bar', b'/100')):
                        exp_user, exp_group = expected.split(b'/')
                        got_user, got_group = got.split(b'/')
                        self.assertEqual(got_user, exp_user)
                        self.assertEqual(got_group, b'None')
                    else:
                        # In other cases the output should be the same
                        self.assertEqual(expected, got)

def _filemode_to_int(mode):
    """Inverse of `stat.filemode` (for permission bits)

    Using mode strings rather than numbers makes the later tests more readable.
    """
    str_mode = mode[1:]
    result = (
          {'r': stat.S_IRUSR, '-': 0}[str_mode[0]]
        | {'w': stat.S_IWUSR, '-': 0}[str_mode[1]]
        | {'x': stat.S_IXUSR, '-': 0,
           's': stat.S_IXUSR | stat.S_ISUID,
           'S': stat.S_ISUID}[str_mode[2]]
        | {'r': stat.S_IRGRP, '-': 0}[str_mode[3]]
        | {'w': stat.S_IWGRP, '-': 0}[str_mode[4]]
        | {'x': stat.S_IXGRP, '-': 0,
           's': stat.S_IXGRP | stat.S_ISGID,
           'S': stat.S_ISGID}[str_mode[5]]
        | {'r': stat.S_IROTH, '-': 0}[str_mode[6]]
        | {'w': stat.S_IWOTH, '-': 0}[str_mode[7]]
        | {'x': stat.S_IXOTH, '-': 0,
           't': stat.S_IXOTH | stat.S_ISVTX,
           'T': stat.S_ISVTX}[str_mode[8]]
        )
    # check we did this right
    assert stat.filemode(result)[1:] == mode[1:]

    return result

class ArchiveMaker:
    """Helper to create a tar file with specific contents

    Usage:

        with ArchiveMaker() as t:
            t.add('filename', ...)

        with t.open() as tar:
            ... # `tar` is now a TarFile with 'filename' in it!
    """
    def __init__(self, **kwargs):
        self.bio = io.BytesIO()
        self.tar_kwargs = dict(kwargs)

    def __enter__(self):
        self.tar_w = tarfile.TarFile(mode='w', fileobj=self.bio, **self.tar_kwargs)
        return self

    def __exit__(self, *exc):
        self.tar_w.close()
        self.contents = self.bio.getvalue()
        self.bio = None

    def add(self, name, *, type=None, symlink_to=None, hardlink_to=None,
            mode=None, size=None, **kwargs):
        """Add a member to the test archive. Call within `with`."""
        name = str(name)
        tarinfo = tarfile.TarInfo(name).replace(**kwargs)
        if size is not None:
            tarinfo.size = size
        if mode:
            tarinfo.mode = _filemode_to_int(mode)
        if symlink_to is not None:
            type = tarfile.SYMTYPE
            tarinfo.linkname = str(symlink_to)
        if hardlink_to is not None:
            type = tarfile.LNKTYPE
            tarinfo.linkname = str(hardlink_to)
        if name.endswith('/') and type is None:
            type = tarfile.DIRTYPE
        if type is not None:
            tarinfo.type = type
        if tarinfo.isreg():
            fileobj = io.BytesIO(bytes(tarinfo.size))
        else:
            fileobj = None
        self.tar_w.addfile(tarinfo, fileobj)

    def open(self, **kwargs):
        """Open the resulting archive as TarFile. Call after `with`."""
        bio = io.BytesIO(self.contents)
        return tarfile.open(fileobj=bio, **kwargs)

# Under WASI, `os_helper.can_symlink` is False to make
# `skip_unless_symlink` skip symlink tests. "
# But in the following tests we use can_symlink to *determine* which
# behavior is expected.
# Like other symlink tests, skip these on WASI for now.
if support.is_wasi:
    def symlink_test(f):
        return unittest.skip("WASI: Skip symlink test for now")(f)
else:
    def symlink_test(f):
        return f


class TestExtractionFilters(unittest.TestCase):

    # A temporary directory for the extraction results.
    # All files that "escape" the destination path should still end
    # up in this directory.
    outerdir = pathlib.Path(TEMPDIR) / 'outerdir'

    # The destination for the extraction, within `outerdir`
    destdir = outerdir / 'dest'

    @contextmanager
    def check_context(self, tar, filter):
        """Extracts `tar` to `self.destdir` and allows checking the result

        If an error occurs, it must be checked using `expect_exception`

        Otherwise, all resulting files must be checked using `expect_file`,
        except the destination directory itself and parent directories of
        other files.
        When checking directories, do so before their contents.
        """
        with os_helper.temp_dir(self.outerdir):
            try:
                tar.extractall(self.destdir, filter=filter)
            except Exception as exc:
                self.raised_exception = exc
                self.expected_paths = set()
            else:
                self.raised_exception = None
                self.expected_paths = set(self.outerdir.glob('**/*'))
                self.expected_paths.discard(self.destdir)
            try:
                yield
            finally:
                tar.close()
            if self.raised_exception:
                raise self.raised_exception
            self.assertEqual(self.expected_paths, set())

    def expect_file(self, name, type=None, symlink_to=None, mode=None,
                    size=None):
        """Check a single file. See check_context."""
        if self.raised_exception:
            raise self.raised_exception
        # use normpath() rather than resolve() so we don't follow symlinks
        path = pathlib.Path(os.path.normpath(self.destdir / name))
        self.assertIn(path, self.expected_paths)
        self.expected_paths.remove(path)
        if mode is not None and os_helper.can_chmod() and os.name != 'nt':
            got = stat.filemode(stat.S_IMODE(path.stat().st_mode))
            self.assertEqual(got, mode)
        if type is None and isinstance(name, str) and name.endswith('/'):
            type = tarfile.DIRTYPE
        if symlink_to is not None:
            got = (self.destdir / name).readlink()
            expected = pathlib.Path(symlink_to)
            # The symlink might be the same (textually) as what we expect,
            # but some systems change the link to an equivalent path, so
            # we fall back to samefile().
            if expected != got:
                self.assertTrue(got.samefile(expected))
        elif type == tarfile.REGTYPE or type is None:
            self.assertTrue(path.is_file())
        elif type == tarfile.DIRTYPE:
            self.assertTrue(path.is_dir())
        elif type == tarfile.FIFOTYPE:
            self.assertTrue(path.is_fifo())
        else:
            raise NotImplementedError(type)
        if size is not None:
            self.assertEqual(path.stat().st_size, size)
        for parent in path.parents:
            self.expected_paths.discard(parent)

    def expect_exception(self, exc_type, message_re='.'):
        with self.assertRaisesRegex(exc_type, message_re):
            if self.raised_exception is not None:
                raise self.raised_exception
        self.raised_exception = None

    def test_benign_file(self):
        with ArchiveMaker() as arc:
            arc.add('benign.txt')
        for filter in 'fully_trusted', 'tar', 'data':
            with self.check_context(arc.open(), filter):
                self.expect_file('benign.txt')

    def test_absolute(self):
        # Test handling a member with an absolute path
        # Inspired by 'absolute1' in https://github.com/jwilk/traversal-archives
        with ArchiveMaker() as arc:
            arc.add(self.outerdir / 'escaped.evil')

        with self.check_context(arc.open(), 'fully_trusted'):
            self.expect_file('../escaped.evil')

        for filter in 'tar', 'data':
            with self.check_context(arc.open(), filter):
                if str(self.outerdir).startswith('/'):
                    # We strip leading slashes, as e.g. GNU tar does
                    # (without --absolute-filenames).
                    outerdir_stripped = str(self.outerdir).lstrip('/')
                    self.expect_file(f'{outerdir_stripped}/escaped.evil')
                else:
                    # On this system, absolute paths don't have leading
                    # slashes.
                    # So, there's nothing to strip. We refuse to unpack
                    # to an absolute path, nonetheless.
                    self.expect_exception(
                        tarfile.AbsolutePathError,
                        """['"].*escaped.evil['"] has an absolute path""")

    @symlink_test
    def test_parent_symlink(self):
        # Test interplaying symlinks
        # Inspired by 'dirsymlink2a' in jwilk/traversal-archives
        with ArchiveMaker() as arc:

            # `current` links to `.` which is both:
            #   - the destination directory
            #   - `current` itself
            arc.add('current', symlink_to='.')

            # effectively points to ./../
            arc.add('parent', symlink_to='current/..')

            arc.add('parent/evil')

        if os_helper.can_symlink():
            with self.check_context(arc.open(), 'fully_trusted'):
                if self.raised_exception is not None:
                    # Windows will refuse to create a file that's a symlink to itself
                    # (and tarfile doesn't swallow that exception)
                    self.expect_exception(FileExistsError)
                    # The other cases will fail with this error too.
                    # Skip the rest of this test.
                    return
                else:
                    self.expect_file('current', symlink_to='.')
                    self.expect_file('parent', symlink_to='current/..')
                    self.expect_file('../evil')

            with self.check_context(arc.open(), 'tar'):
                self.expect_exception(
                    tarfile.OutsideDestinationError,
                    """'parent/evil' would be extracted to ['"].*evil['"], """
                    + "which is outside the destination")

            with self.check_context(arc.open(), 'data'):
                self.expect_exception(
                    tarfile.LinkOutsideDestinationError,
                    """'parent' would link to ['"].*outerdir['"], """
                    + "which is outside the destination")

        else:
            # No symlink support. The symlinks are ignored.
            with self.check_context(arc.open(), 'fully_trusted'):
                self.expect_file('parent/evil')
            with self.check_context(arc.open(), 'tar'):
                self.expect_file('parent/evil')
            with self.check_context(arc.open(), 'data'):
                self.expect_file('parent/evil')

    @symlink_test
    def test_parent_symlink2(self):
        # Test interplaying symlinks
        # Inspired by 'dirsymlink2b' in jwilk/traversal-archives

        # Posix and Windows have different pathname resolution:
        # either symlink or a '..' component resolve first.
        # Let's see which we are on.
        if os_helper.can_symlink():
            testpath = os.path.join(TEMPDIR, 'resolution_test')
            os.mkdir(testpath)

            # testpath/current links to `.` which is all of:
            #   - `testpath`
            #   - `testpath/current`
            #   - `testpath/current/current`
            #   - etc.
            os.symlink('.', os.path.join(testpath, 'current'))

            # we'll test where `testpath/current/../file` ends up
            with open(os.path.join(testpath, 'current', '..', 'file'), 'w'):
                pass

            if os.path.exists(os.path.join(testpath, 'file')):
                # Windows collapses 'current\..' to '.' first, leaving
                # 'testpath\file'
                dotdot_resolves_early = True
            elif os.path.exists(os.path.join(testpath, '..', 'file')):
                # Posix resolves 'current' to '.' first, leaving
                # 'testpath/../file'
                dotdot_resolves_early = False
            else:
                raise AssertionError('Could not determine link resolution')

        with ArchiveMaker() as arc:

            # `current` links to `.` which is both the destination directory
            # and `current` itself
            arc.add('current', symlink_to='.')

            # `current/parent` is also available as `./parent`,
            # and effectively points to `./../`
            arc.add('current/parent', symlink_to='..')

            arc.add('parent/evil')

        with self.check_context(arc.open(), 'fully_trusted'):
            if os_helper.can_symlink():
                self.expect_file('current', symlink_to='.')
                self.expect_file('parent', symlink_to='..')
                self.expect_file('../evil')
            else:
                self.expect_file('current/')
                self.expect_file('parent/evil')

        with self.check_context(arc.open(), 'tar'):
            if os_helper.can_symlink():
                # Fail when extracting a file outside destination
                self.expect_exception(
                        tarfile.OutsideDestinationError,
                        "'parent/evil' would be extracted to "
                        + """['"].*evil['"], which is outside """
                        + "the destination")
            else:
                self.expect_file('current/')
                self.expect_file('parent/evil')

        with self.check_context(arc.open(), 'data'):
            if os_helper.can_symlink():
                if dotdot_resolves_early:
                    # Fail when extracting a file outside destination
                    self.expect_exception(
                            tarfile.OutsideDestinationError,
                            "'parent/evil' would be extracted to "
                            + """['"].*evil['"], which is outside """
                            + "the destination")
                else:
                    # Fail as soon as we have a symlink outside the destination
                    self.expect_exception(
                            tarfile.LinkOutsideDestinationError,
                            "'current/parent' would link to "
                            + """['"].*outerdir['"], which is outside """
                            + "the destination")
            else:
                self.expect_file('current/')
                self.expect_file('parent/evil')

    @symlink_test
    def test_absolute_symlink(self):
        # Test symlink to an absolute path
        # Inspired by 'dirsymlink' in jwilk/traversal-archives
        with ArchiveMaker() as arc:
            arc.add('parent', symlink_to=self.outerdir)
            arc.add('parent/evil')

        with self.check_context(arc.open(), 'fully_trusted'):
            if os_helper.can_symlink():
                self.expect_file('parent', symlink_to=self.outerdir)
                self.expect_file('../evil')
            else:
                self.expect_file('parent/evil')

        with self.check_context(arc.open(), 'tar'):
            if os_helper.can_symlink():
                self.expect_exception(
                        tarfile.OutsideDestinationError,
                        "'parent/evil' would be extracted to "
                        + """['"].*evil['"], which is outside """
                        + "the destination")
            else:
                self.expect_file('parent/evil')

        with self.check_context(arc.open(), 'data'):
            self.expect_exception(
                tarfile.AbsoluteLinkError,
                "'parent' is a link to an absolute path")

    def test_absolute_hardlink(self):
        # Test hardlink to an absolute path
        # Inspired by 'dirsymlink' in https://github.com/jwilk/traversal-archives
        with ArchiveMaker() as arc:
            arc.add('parent', hardlink_to=self.outerdir / 'foo')

        with self.check_context(arc.open(), 'fully_trusted'):
            self.expect_exception(KeyError, ".*foo. not found")

        with self.check_context(arc.open(), 'tar'):
            self.expect_exception(KeyError, ".*foo. not found")

        with self.check_context(arc.open(), 'data'):
            self.expect_exception(
                tarfile.AbsoluteLinkError,
                "'parent' is a link to an absolute path")

    @symlink_test
    def test_sly_relative0(self):
        # Inspired by 'relative0' in jwilk/traversal-archives
        with ArchiveMaker() as arc:
            # points to `../../tmp/moo`
            arc.add('../moo', symlink_to='..//tmp/moo')

        try:
            with self.check_context(arc.open(), filter='fully_trusted'):
                if os_helper.can_symlink():
                    if isinstance(self.raised_exception, FileExistsError):
                        # XXX TarFile happens to fail creating a parent
                        # directory.
                        # This might be a bug, but fixing it would hurt
                        # security.
                        # Note that e.g. GNU `tar` rejects '..' components,
                        # so you could argue this is an invalid archive and we
                        # just raise an bad type of exception.
                        self.expect_exception(FileExistsError)
                    else:
                        self.expect_file('../moo', symlink_to='..//tmp/moo')
                else:
                    # The symlink can't be extracted and is ignored
                    pass
        except FileExistsError:
            pass

        for filter in 'tar', 'data':
            with self.check_context(arc.open(), filter):
                self.expect_exception(
                        tarfile.OutsideDestinationError,
                        "'../moo' would be extracted to "
                        + "'.*moo', which is outside "
                        + "the destination")

    @symlink_test
    def test_sly_relative2(self):
        # Inspired by 'relative2' in jwilk/traversal-archives
        with ArchiveMaker() as arc:
            arc.add('tmp/')
            arc.add('tmp/../../moo', symlink_to='tmp/../..//tmp/moo')

        with self.check_context(arc.open(), 'fully_trusted'):
            self.expect_file('tmp', type=tarfile.DIRTYPE)
            if os_helper.can_symlink():
                self.expect_file('../moo', symlink_to='tmp/../../tmp/moo')

        for filter in 'tar', 'data':
            with self.check_context(arc.open(), filter):
                self.expect_exception(
                    tarfile.OutsideDestinationError,
                    "'tmp/../../moo' would be extracted to "
                    + """['"].*moo['"], which is outside the """
                    + "destination")

    @symlink_test
    def test_deep_symlink(self):
        # Test that symlinks and hardlinks inside a directory
        # point to the correct file (`target` of size 3).
        # If links aren't supported we get a copy of the file.
        with ArchiveMaker() as arc:
            arc.add('targetdir/target', size=3)
            # a hardlink's linkname is relative to the archive
            arc.add('linkdir/hardlink', hardlink_to=os.path.join(
                'targetdir', 'target'))
            # a symlink's  linkname is relative to the link's directory
            arc.add('linkdir/symlink', symlink_to=os.path.join(
                '..', 'targetdir', 'target'))

        for filter in 'tar', 'data', 'fully_trusted':
            with self.check_context(arc.open(), filter):
                self.expect_file('targetdir/target', size=3)
                self.expect_file('linkdir/hardlink', size=3)
                if os_helper.can_symlink():
                    self.expect_file('linkdir/symlink', size=3,
                                     symlink_to='../targetdir/target')
                else:
                    self.expect_file('linkdir/symlink', size=3)

    @symlink_test
    def test_chains(self):
        # Test chaining of symlinks/hardlinks.
        # Symlinks are created before the files they point to.
        with ArchiveMaker() as arc:
            arc.add('linkdir/symlink', symlink_to='hardlink')
            arc.add('symlink2', symlink_to=os.path.join(
                'linkdir', 'hardlink2'))
            arc.add('targetdir/target', size=3)
            arc.add('linkdir/hardlink', hardlink_to='targetdir/target')
            arc.add('linkdir/hardlink2', hardlink_to='linkdir/symlink')

        for filter in 'tar', 'data', 'fully_trusted':
            with self.check_context(arc.open(), filter):
                self.expect_file('targetdir/target', size=3)
                self.expect_file('linkdir/hardlink', size=3)
                self.expect_file('linkdir/hardlink2', size=3)
                if os_helper.can_symlink():
                    self.expect_file('linkdir/symlink', size=3,
                                     symlink_to='hardlink')
                    self.expect_file('symlink2', size=3,
                                     symlink_to='linkdir/hardlink2')
                else:
                    self.expect_file('linkdir/symlink', size=3)
                    self.expect_file('symlink2', size=3)

    def test_modes(self):
        # Test how file modes are extracted
        # (Note that the modes are ignored on platforms without working chmod)
        with ArchiveMaker() as arc:
            arc.add('all_bits', mode='?rwsrwsrwt')
            arc.add('perm_bits', mode='?rwxrwxrwx')
            arc.add('exec_group_other', mode='?rw-rwxrwx')
            arc.add('read_group_only', mode='?---r-----')
            arc.add('no_bits', mode='?---------')
            arc.add('dir/', mode='?---rwsrwt')
            arc.add('dir_all_bits/', mode='?rwsrwsrwt')

        # On some systems, setting the uid, gid, and/or sticky bit is a no-ops.
        # Check which bits we can set, so we can compare tarfile machinery to
        # a simple chmod.
        tmp_filename = os.path.join(TEMPDIR, "tmp.file")
        with open(tmp_filename, 'w'):
            pass
        try:
            new_mode = (os.stat(tmp_filename).st_mode
                        | stat.S_ISVTX | stat.S_ISGID | stat.S_ISUID)
            try:
                os.chmod(tmp_filename, new_mode)
            except OSError as exc:
                if exc.errno == getattr(errno, "EFTYPE", 0):
                    # gh-108948: On FreeBSD, regular users cannot set
                    # the sticky bit.
                    self.skipTest("chmod() failed with EFTYPE: "
                                  "regular users cannot set sticky bit")
                else:
                    raise

            got_mode = os.stat(tmp_filename).st_mode
            _t_file = 't' if (got_mode & stat.S_ISVTX) else 'x'
            _suid_file = 's' if (got_mode & stat.S_ISUID) else 'x'
            _sgid_file = 's' if (got_mode & stat.S_ISGID) else 'x'
        finally:
            os.unlink(tmp_filename)

        os.mkdir(tmp_filename)
        new_mode = (os.stat(tmp_filename).st_mode
                    | stat.S_ISVTX | stat.S_ISGID | stat.S_ISUID)
        os.chmod(tmp_filename, new_mode)
        got_mode = os.stat(tmp_filename).st_mode
        _t_dir = 't' if (got_mode & stat.S_ISVTX) else 'x'
        _suid_dir = 's' if (got_mode & stat.S_ISUID) else 'x'
        _sgid_dir = 's' if (got_mode & stat.S_ISGID) else 'x'
        os.rmdir(tmp_filename)

        with self.check_context(arc.open(), 'fully_trusted'):
            self.expect_file('all_bits',
                             mode=f'?rw{_suid_file}rw{_sgid_file}rw{_t_file}')
            self.expect_file('perm_bits', mode='?rwxrwxrwx')
            self.expect_file('exec_group_other', mode='?rw-rwxrwx')
            self.expect_file('read_group_only', mode='?---r-----')
            self.expect_file('no_bits', mode='?---------')
            self.expect_file('dir/', mode=f'?---rw{_sgid_dir}rw{_t_dir}')
            self.expect_file('dir_all_bits/',
                             mode=f'?rw{_suid_dir}rw{_sgid_dir}rw{_t_dir}')

        with self.check_context(arc.open(), 'tar'):
            self.expect_file('all_bits', mode='?rwxr-xr-x')
            self.expect_file('perm_bits', mode='?rwxr-xr-x')
            self.expect_file('exec_group_other', mode='?rw-r-xr-x')
            self.expect_file('read_group_only', mode='?---r-----')
            self.expect_file('no_bits', mode='?---------')
            self.expect_file('dir/', mode='?---r-xr-x')
            self.expect_file('dir_all_bits/', mode='?rwxr-xr-x')

        with self.check_context(arc.open(), 'data'):
            normal_dir_mode = stat.filemode(stat.S_IMODE(
                self.outerdir.stat().st_mode))
            self.expect_file('all_bits', mode='?rwxr-xr-x')
            self.expect_file('perm_bits', mode='?rwxr-xr-x')
            self.expect_file('exec_group_other', mode='?rw-r--r--')
            self.expect_file('read_group_only', mode='?rw-r-----')
            self.expect_file('no_bits', mode='?rw-------')
            self.expect_file('dir/', mode=normal_dir_mode)
            self.expect_file('dir_all_bits/', mode=normal_dir_mode)

    def test_pipe(self):
        # Test handling of a special file
        with ArchiveMaker() as arc:
            arc.add('foo', type=tarfile.FIFOTYPE)

        for filter in 'fully_trusted', 'tar':
            with self.check_context(arc.open(), filter):
                if hasattr(os, 'mkfifo'):
                    self.expect_file('foo', type=tarfile.FIFOTYPE)
                else:
                    # The pipe can't be extracted and is skipped.
                    pass

        with self.check_context(arc.open(), 'data'):
            self.expect_exception(
                tarfile.SpecialFileError,
                "'foo' is a special file")

    def test_special_files(self):
        # Creating device files is tricky. Instead of attempting that let's
        # only check the filter result.
        for special_type in tarfile.FIFOTYPE, tarfile.CHRTYPE, tarfile.BLKTYPE:
            tarinfo = tarfile.TarInfo('foo')
            tarinfo.type = special_type
            trusted = tarfile.fully_trusted_filter(tarinfo, '')
            self.assertIs(trusted, tarinfo)
            tar = tarfile.tar_filter(tarinfo, '')
            self.assertEqual(tar.type, special_type)
            with self.assertRaises(tarfile.SpecialFileError) as cm:
                tarfile.data_filter(tarinfo, '')
            self.assertIsInstance(cm.exception.tarinfo, tarfile.TarInfo)
            self.assertEqual(cm.exception.tarinfo.name, 'foo')

    def test_fully_trusted_filter(self):
        # The 'fully_trusted' filter returns the original TarInfo objects.
        with tarfile.TarFile.open(tarname) as tar:
            for tarinfo in tar.getmembers():
                filtered = tarfile.fully_trusted_filter(tarinfo, '')
                self.assertIs(filtered, tarinfo)

    def test_tar_filter(self):
        # The 'tar' filter returns TarInfo objects with the same name/type.
        # (It can also fail for particularly "evil" input, but we don't have
        # that in the test archive.)
        with tarfile.TarFile.open(tarname) as tar:
            for tarinfo in tar.getmembers():
                try:
                    filtered = tarfile.tar_filter(tarinfo, '')
                except UnicodeEncodeError:
                    continue
                self.assertIs(filtered.name, tarinfo.name)
                self.assertIs(filtered.type, tarinfo.type)

    def test_data_filter(self):
        # The 'data' filter either raises, or returns TarInfo with the same
        # name/type.
        with tarfile.TarFile.open(tarname) as tar:
            for tarinfo in tar.getmembers():
                try:
                    filtered = tarfile.data_filter(tarinfo, '')
                except (tarfile.FilterError, UnicodeEncodeError):
                    continue
                self.assertIs(filtered.name, tarinfo.name)
                self.assertIs(filtered.type, tarinfo.type)

    @unittest.skipIf(sys.platform == 'win32', 'requires native bytes paths')
    def test_filter_unencodable(self):
        # Sanity check using a valid path.
        tarinfo = tarfile.TarInfo(os_helper.TESTFN)
        filtered = tarfile.tar_filter(tarinfo, '')
        self.assertIs(filtered.name, tarinfo.name)
        filtered = tarfile.data_filter(tarinfo, '')
        self.assertIs(filtered.name, tarinfo.name)

        tarinfo = tarfile.TarInfo('test\x00')
        self.assertRaises(ValueError, tarfile.tar_filter, tarinfo, '')
        self.assertRaises(ValueError, tarfile.data_filter, tarinfo, '')
        tarinfo = tarfile.TarInfo('\ud800')
        self.assertRaises(UnicodeEncodeError, tarfile.tar_filter, tarinfo, '')
        self.assertRaises(UnicodeEncodeError, tarfile.data_filter, tarinfo, '')

    @unittest.skipIf(sys.platform == 'win32', 'requires native bytes paths')
    def test_extract_unencodable(self):
        # Create a member with name \xed\xa0\x80 which is UTF-8 encoded
        # lone surrogate \ud800.
        with ArchiveMaker(encoding='ascii', errors='surrogateescape') as arc:
            arc.add('\udced\udca0\udc80')
        with os_helper.temp_cwd() as tmp:
            tar = arc.open(encoding='utf-8', errors='surrogatepass',
                           errorlevel=1)
            self.assertEqual(tar.getnames(), ['\ud800'])
            with self.assertRaises(UnicodeEncodeError):
                tar.extractall()
            self.assertEqual(os.listdir(), [])

            tar = arc.open(encoding='utf-8', errors='surrogatepass',
                           errorlevel=0, debug=1)
            with support.captured_stderr() as stderr:
                tar.extractall()
            self.assertEqual(os.listdir(), [])
            self.assertIn('tarfile: UnicodeEncodeError ', stderr.getvalue())

    def test_change_default_filter_on_instance(self):
        tar = tarfile.TarFile(tarname, 'r')
        def strict_filter(tarinfo, path):
            if tarinfo.name == 'ustar/regtype':
                return tarinfo
            else:
                return None
        tar.extraction_filter = strict_filter
        with self.check_context(tar, None):
            self.expect_file('ustar/regtype')

    def test_change_default_filter_on_class(self):
        def strict_filter(tarinfo, path):
            if tarinfo.name == 'ustar/regtype':
                return tarinfo
            else:
                return None
        tar = tarfile.TarFile(tarname, 'r')
        with support.swap_attr(tarfile.TarFile, 'extraction_filter',
                               staticmethod(strict_filter)):
            with self.check_context(tar, None):
                self.expect_file('ustar/regtype')

    def test_change_default_filter_on_subclass(self):
        class TarSubclass(tarfile.TarFile):
            def extraction_filter(self, tarinfo, path):
                if tarinfo.name == 'ustar/regtype':
                    return tarinfo
                else:
                    return None

        tar = TarSubclass(tarname, 'r')
        with self.check_context(tar, None):
            self.expect_file('ustar/regtype')

    def test_change_default_filter_to_string(self):
        tar = tarfile.TarFile(tarname, 'r')
        tar.extraction_filter = 'data'
        with self.check_context(tar, None):
            self.expect_exception(TypeError)

    def test_custom_filter(self):
        def custom_filter(tarinfo, path):
            self.assertIs(path, self.destdir)
            if tarinfo.name == 'move_this':
                return tarinfo.replace(name='moved')
            if tarinfo.name == 'ignore_this':
                return None
            return tarinfo

        with ArchiveMaker() as arc:
            arc.add('move_this')
            arc.add('ignore_this')
            arc.add('keep')
        with self.check_context(arc.open(), custom_filter):
            self.expect_file('moved')
            self.expect_file('keep')

    def test_bad_filter_name(self):
        with ArchiveMaker() as arc:
            arc.add('foo')
        with self.check_context(arc.open(), 'bad filter name'):
            self.expect_exception(ValueError)

    def test_stateful_filter(self):
        # Stateful filters should be possible.
        # (This doesn't really test tarfile. Rather, it demonstrates
        # that third parties can implement a stateful filter.)
        class StatefulFilter:
            def __enter__(self):
                self.num_files_processed = 0
                return self

            def __call__(self, tarinfo, path):
                try:
                    tarinfo = tarfile.data_filter(tarinfo, path)
                except tarfile.FilterError:
                    return None
                self.num_files_processed += 1
                return tarinfo

            def __exit__(self, *exc_info):
                self.done = True

        with ArchiveMaker() as arc:
            arc.add('good')
            arc.add('bad', symlink_to='/')
            arc.add('good')
        with StatefulFilter() as custom_filter:
            with self.check_context(arc.open(), custom_filter):
                self.expect_file('good')
        self.assertEqual(custom_filter.num_files_processed, 2)
        self.assertEqual(custom_filter.done, True)

    def test_errorlevel(self):
        def extracterror_filter(tarinfo, path):
            raise tarfile.ExtractError('failed with ExtractError')
        def filtererror_filter(tarinfo, path):
            raise tarfile.FilterError('failed with FilterError')
        def oserror_filter(tarinfo, path):
            raise OSError('failed with OSError')
        def tarerror_filter(tarinfo, path):
            raise tarfile.TarError('failed with base TarError')
        def valueerror_filter(tarinfo, path):
            raise ValueError('failed with ValueError')

        with ArchiveMaker() as arc:
            arc.add('file')

        # If errorlevel is 0, errors affected by errorlevel are ignored

        with self.check_context(arc.open(errorlevel=0), extracterror_filter):
            self.expect_file('file')

        with self.check_context(arc.open(errorlevel=0), filtererror_filter):
            self.expect_file('file')

        with self.check_context(arc.open(errorlevel=0), oserror_filter):
            self.expect_file('file')

        with self.check_context(arc.open(errorlevel=0), tarerror_filter):
            self.expect_exception(tarfile.TarError)

        with self.check_context(arc.open(errorlevel=0), valueerror_filter):
            self.expect_exception(ValueError)

        # If 1, all fatal errors are raised

        with self.check_context(arc.open(errorlevel=1), extracterror_filter):
            self.expect_file('file')

        with self.check_context(arc.open(errorlevel=1), filtererror_filter):
            self.expect_exception(tarfile.FilterError)

        with self.check_context(arc.open(errorlevel=1), oserror_filter):
            self.expect_exception(OSError)

        with self.check_context(arc.open(errorlevel=1), tarerror_filter):
            self.expect_exception(tarfile.TarError)

        with self.check_context(arc.open(errorlevel=1), valueerror_filter):
            self.expect_exception(ValueError)

        # If 2, all non-fatal errors are raised as well.

        with self.check_context(arc.open(errorlevel=2), extracterror_filter):
            self.expect_exception(tarfile.ExtractError)

        with self.check_context(arc.open(errorlevel=2), filtererror_filter):
            self.expect_exception(tarfile.FilterError)

        with self.check_context(arc.open(errorlevel=2), oserror_filter):
            self.expect_exception(OSError)

        with self.check_context(arc.open(errorlevel=2), tarerror_filter):
            self.expect_exception(tarfile.TarError)

        with self.check_context(arc.open(errorlevel=2), valueerror_filter):
            self.expect_exception(ValueError)

        # We only handle ExtractionError, FilterError & OSError specially.

        with self.check_context(arc.open(errorlevel='boo!'), filtererror_filter):
            self.expect_exception(TypeError)  # errorlevel is not int


class OverwriteTests(archiver_tests.OverwriteTests, unittest.TestCase):
    testdir = os.path.join(TEMPDIR, "testoverwrite")

    @classmethod
    def setUpClass(cls):
        p = cls.ar_with_file = os.path.join(TEMPDIR, 'tar-with-file.tar')
        cls.addClassCleanup(os_helper.unlink, p)
        with tarfile.open(p, 'w') as tar:
            t = tarfile.TarInfo('test')
            t.size = 10
            tar.addfile(t, io.BytesIO(b'newcontent'))

        p = cls.ar_with_dir = os.path.join(TEMPDIR, 'tar-with-dir.tar')
        cls.addClassCleanup(os_helper.unlink, p)
        with tarfile.open(p, 'w') as tar:
            tar.addfile(tar.gettarinfo(os.curdir, 'test'))

        p = os.path.join(TEMPDIR, 'tar-with-implicit-dir.tar')
        cls.ar_with_implicit_dir = p
        cls.addClassCleanup(os_helper.unlink, p)
        with tarfile.open(p, 'w') as tar:
            t = tarfile.TarInfo('test/file')
            t.size = 10
            tar.addfile(t, io.BytesIO(b'newcontent'))

    def open(self, path):
        return tarfile.open(path, 'r')

    def extractall(self, ar):
        ar.extractall(self.testdir, filter='fully_trusted')


def setUpModule():
    os_helper.unlink(TEMPDIR)
    os.makedirs(TEMPDIR)

    global testtarnames
    testtarnames = [tarname]
    with open(tarname, "rb") as fobj:
        data = fobj.read()

    # Create compressed tarfiles.
    for c in GzipTest, Bz2Test, LzmaTest, ZstdTest:
        if c.open:
            os_helper.unlink(c.tarname)
            testtarnames.append(c.tarname)
            with c.open(c.tarname, "wb") as tar:
                tar.write(data)

def tearDownModule():
    if os.path.exists(TEMPDIR):
        os_helper.rmtree(TEMPDIR)

if __name__ == "__main__":
    unittest.main()
