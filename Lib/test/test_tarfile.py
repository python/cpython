import sys
import os
import io
import shutil
import io
from hashlib import md5
import errno

import unittest
import tarfile

from test import support

# Check for our compression modules.
try:
    import gzip
    gzip.GzipFile
except (ImportError, AttributeError):
    gzip = None
try:
    import bz2
except ImportError:
    bz2 = None

def md5sum(data):
    return md5(data).hexdigest()

TEMPDIR = os.path.abspath(support.TESTFN) + "-tardir"
tarname = support.findfile("testtar.tar")
gzipname = os.path.join(TEMPDIR, "testtar.tar.gz")
bz2name = os.path.join(TEMPDIR, "testtar.tar.bz2")
tmpname = os.path.join(TEMPDIR, "tmp.tar")

md5_regtype = "65f477c818ad9e15f7feab0c6d37742f"
md5_sparse = "a54fbc4ca4f4399a90e1b27164012fc6"


class ReadTest(unittest.TestCase):

    tarname = tarname
    mode = "r:"

    def setUp(self):
        self.tar = tarfile.open(self.tarname, mode=self.mode, encoding="iso8859-1")

    def tearDown(self):
        self.tar.close()


class UstarReadTest(ReadTest):

    def test_fileobj_regular_file(self):
        tarinfo = self.tar.getmember("ustar/regtype")
        fobj = self.tar.extractfile(tarinfo)
        try:
            data = fobj.read()
            self.assertTrue((len(data), md5sum(data)) == (tarinfo.size, md5_regtype),
                    "regular file extraction failed")
        finally:
            fobj.close()

    def test_fileobj_readlines(self):
        self.tar.extract("ustar/regtype", TEMPDIR)
        tarinfo = self.tar.getmember("ustar/regtype")
        with open(os.path.join(TEMPDIR, "ustar/regtype"), "r") as fobj1:
            lines1 = fobj1.readlines()

        fobj = self.tar.extractfile(tarinfo)
        try:
            fobj2 = io.TextIOWrapper(fobj)
            lines2 = fobj2.readlines()
            self.assertTrue(lines1 == lines2,
                    "fileobj.readlines() failed")
            self.assertTrue(len(lines2) == 114,
                    "fileobj.readlines() failed")
            self.assertTrue(lines2[83] ==
                    "I will gladly admit that Python is not the fastest running scripting language.\n",
                    "fileobj.readlines() failed")
        finally:
            fobj.close()

    def test_fileobj_iter(self):
        self.tar.extract("ustar/regtype", TEMPDIR)
        tarinfo = self.tar.getmember("ustar/regtype")
        with open(os.path.join(TEMPDIR, "ustar/regtype"), "rU") as fobj1:
            lines1 = fobj1.readlines()
        fobj2 = self.tar.extractfile(tarinfo)
        try:
            lines2 = list(io.TextIOWrapper(fobj2))
            self.assertTrue(lines1 == lines2,
                         "fileobj.__iter__() failed")
        finally:
            fobj2.close()

    def test_fileobj_seek(self):
        self.tar.extract("ustar/regtype", TEMPDIR)
        with open(os.path.join(TEMPDIR, "ustar/regtype"), "rb") as fobj:
            data = fobj.read()

        tarinfo = self.tar.getmember("ustar/regtype")
        fobj = self.tar.extractfile(tarinfo)

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
        self.assertTrue(s == data[2048:2058],
                     "read() after seek failed")
        fobj.seek(0, 2)
        self.assertEqual(tarinfo.size, fobj.tell(),
                     "seek() to file's end failed")
        self.assertTrue(fobj.read() == b"",
                     "read() at file's end did not return empty string")
        fobj.seek(-tarinfo.size, 2)
        self.assertEqual(0, fobj.tell(),
                     "relative seek() to file's end failed")
        fobj.seek(512)
        s1 = fobj.readlines()
        fobj.seek(512)
        s2 = fobj.readlines()
        self.assertTrue(s1 == s2,
                     "readlines() after seek failed")
        fobj.seek(0)
        self.assertEqual(len(fobj.readline()), fobj.tell(),
                     "tell() after readline() failed")
        fobj.seek(512)
        self.assertTrue(len(fobj.readline()) + 512 == fobj.tell(),
                     "tell() after seek() and readline() failed")
        fobj.seek(0)
        line = fobj.readline()
        self.assertEqual(fobj.read(), data[len(line):],
                     "read() after readline() failed")
        fobj.close()

    # Test if symbolic and hard links are resolved by extractfile().  The
    # test link members each point to a regular member whose data is
    # supposed to be exported.
    def _test_fileobj_link(self, lnktype, regtype):
        a = self.tar.extractfile(lnktype)
        b = self.tar.extractfile(regtype)
        try:
            self.assertEqual(a.name, b.name)
        finally:
            a.close()
            b.close()

    def test_fileobj_link1(self):
        self._test_fileobj_link("ustar/lnktype", "ustar/regtype")

    def test_fileobj_link2(self):
        self._test_fileobj_link("./ustar/linktest2/lnktype", "ustar/linktest1/regtype")

    def test_fileobj_symlink1(self):
        self._test_fileobj_link("ustar/symtype", "ustar/regtype")

    def test_fileobj_symlink2(self):
        self._test_fileobj_link("./ustar/linktest2/symtype", "ustar/linktest1/regtype")


class CommonReadTest(ReadTest):

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
        if self.mode.endswith(":gz"):
            _open = gzip.GzipFile
        elif self.mode.endswith(":bz2"):
            _open = bz2.BZ2File
        else:
            _open = open

        for char in (b'\0', b'a'):
            # Test if EOFHeaderError ('\0') and InvalidHeaderError ('a')
            # are ignored correctly.
            with _open(tmpname, "wb") as fobj:
                fobj.write(char * 1024)
                fobj.write(tarfile.TarInfo("foo").tobuf())

            tar = tarfile.open(tmpname, mode="r", ignore_zeros=True)
            try:
                self.assertListEqual(tar.getnames(), ["foo"],
                    "ignore_zeros=True should have skipped the %r-blocks" % char)
            finally:
                tar.close()


class MiscReadTest(CommonReadTest):

    def test_no_name_argument(self):
        with open(self.tarname, "rb") as fobj:
            tar = tarfile.open(fileobj=fobj, mode=self.mode)
            self.assertEqual(tar.name, os.path.abspath(fobj.name))

    def test_no_name_attribute(self):
        with open(self.tarname, "rb") as fobj:
            data = fobj.read()
        fobj = io.BytesIO(data)
        self.assertRaises(AttributeError, getattr, fobj, "name")
        tar = tarfile.open(fileobj=fobj, mode=self.mode)
        self.assertEqual(tar.name, None)

    def test_empty_name_attribute(self):
        with open(self.tarname, "rb") as fobj:
            data = fobj.read()
        fobj = io.BytesIO(data)
        fobj.name = ""
        with tarfile.open(fileobj=fobj, mode=self.mode) as tar:
            self.assertEqual(tar.name, None)

    def test_fileobj_with_offset(self):
        # Skip the first member and store values from the second member
        # of the testtar.
        tar = tarfile.open(self.tarname, mode=self.mode)
        try:
            tar.next()
            t = tar.next()
            name = t.name
            offset = t.offset
            f = tar.extractfile(t)
            data = f.read()
            f.close()
        finally:
            tar.close()

        # Open the testtar and seek to the offset of the second member.
        if self.mode.endswith(":gz"):
            _open = gzip.GzipFile
        elif self.mode.endswith(":bz2"):
            _open = bz2.BZ2File
        else:
            _open = open
        fobj = _open(self.tarname, "rb")
        try:
            fobj.seek(offset)

            # Test if the tarfile starts with the second member.
            tar = tar.open(self.tarname, mode="r:", fileobj=fobj)
            t = tar.next()
            self.assertEqual(t.name, name)
            # Read to the end of fileobj and test if seeking back to the
            # beginning works.
            tar.getmembers()
            self.assertEqual(tar.extractfile(t).read(), data,
                    "seek back did not work")
            tar.close()
        finally:
            fobj.close()

    def test_fail_comp(self):
        # For Gzip and Bz2 Tests: fail with a ReadError on an uncompressed file.
        if self.mode == "r:":
            return
        self.assertRaises(tarfile.ReadError, tarfile.open, tarname, self.mode)
        with open(tarname, "rb") as fobj:
            self.assertRaises(tarfile.ReadError, tarfile.open,
                              fileobj=fobj, mode=self.mode)

    def test_v7_dirtype(self):
        # Test old style dirtype member (bug #1336623):
        # Old V7 tars create directory members using an AREGTYPE
        # header with a "/" appended to the filename field.
        tarinfo = self.tar.getmember("misc/dirtype-old-v7")
        self.assertTrue(tarinfo.type == tarfile.DIRTYPE,
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
            self.assertTrue(int(tarinfo.mtime) == 0o7606136617,
                    "wrong mtime for %s" % tarinfo.name)
            if not tarinfo.name.startswith("ustar/"):
                continue
            self.assertTrue(tarinfo.uname == "tarfile",
                    "wrong uname for %s" % tarinfo.name)

    def test_find_members(self):
        self.assertTrue(self.tar.getmembers()[-1].name == "misc/eof",
                "could not find all members")

    @unittest.skipUnless(hasattr(os, "link"),
                         "Missing hardlink implementation")
    @support.skip_unless_symlink
    def test_extract_hardlink(self):
        # Test hardlink extraction (e.g. bug #857297).
        tar = tarfile.open(tarname, errorlevel=1, encoding="iso8859-1")

        try:
            tar.extract("ustar/regtype", TEMPDIR)
            try:
                tar.extract("ustar/lnktype", TEMPDIR)
            except EnvironmentError as e:
                if e.errno == errno.ENOENT:
                    self.fail("hardlink not extracted properly")

            with open(os.path.join(TEMPDIR, "ustar/lnktype"), "rb") as f:
                data = f.read()
            self.assertEqual(md5sum(data), md5_regtype)

            try:
                tar.extract("ustar/symtype", TEMPDIR)
            except EnvironmentError as e:
                if e.errno == errno.ENOENT:
                    self.fail("symlink not extracted properly")

            with open(os.path.join(TEMPDIR, "ustar/symtype"), "rb") as f:
                data = f.read()
            self.assertEqual(md5sum(data), md5_regtype)
        finally:
            tar.close()

    def test_extractall(self):
        # Test if extractall() correctly restores directory permissions
        # and times (see issue1735).
        tar = tarfile.open(tarname, encoding="iso8859-1")
        DIR = os.path.join(TEMPDIR, "extractall")
        os.mkdir(DIR)
        try:
            directories = [t for t in tar if t.isdir()]
            tar.extractall(DIR, directories)
            for tarinfo in directories:
                path = os.path.join(DIR, tarinfo.name)
                if sys.platform != "win32":
                    # Win32 has no support for fine grained permissions.
                    self.assertEqual(tarinfo.mode & 0o777, os.stat(path).st_mode & 0o777)
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
            shutil.rmtree(DIR)

    def test_extract_directory(self):
        dirtype = "ustar/dirtype"
        DIR = os.path.join(TEMPDIR, "extractdir")
        os.mkdir(DIR)
        try:
            with tarfile.open(tarname, encoding="iso8859-1") as tar:
                tarinfo = tar.getmember(dirtype)
                tar.extract(tarinfo, path=DIR)
                extracted = os.path.join(DIR, dirtype)
                self.assertEqual(os.path.getmtime(extracted), tarinfo.mtime)
                if sys.platform != "win32":
                    self.assertEqual(os.stat(extracted).st_mode & 0o777, 0o755)
        finally:
            shutil.rmtree(DIR)

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
            support.unlink(empty)


class StreamReadTest(CommonReadTest):

    mode="r|"

    def test_fileobj_regular_file(self):
        tarinfo = self.tar.next() # get "regtype" (can't use getmember)
        fobj = self.tar.extractfile(tarinfo)
        data = fobj.read()
        self.assertTrue((len(data), md5sum(data)) == (tarinfo.size, md5_regtype),
                "regular file extraction failed")

    def test_provoke_stream_error(self):
        tarinfos = self.tar.getmembers()
        f = self.tar.extractfile(tarinfos[0]) # read the first member
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
                self.assertTrue(t2 is not None, "stream.next() failed.")

                if t2.islnk() or t2.issym():
                    self.assertRaises(tarfile.StreamError, tar2.extractfile, t2)
                    continue

                v1 = tar1.extractfile(t1)
                v2 = tar2.extractfile(t2)
                if v1 is None:
                    continue
                self.assertTrue(v2 is not None, "stream.extractfile() failed")
                self.assertEqual(v1.read(), v2.read(), "stream extraction failed")
        finally:
            tar1.close()


class DetectReadTest(unittest.TestCase):

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
        testfunc(tarname, "r")
        testfunc(tarname, "r:")
        testfunc(tarname, "r:*")
        testfunc(tarname, "r|")
        testfunc(tarname, "r|*")

        if gzip:
            self.assertRaises(tarfile.ReadError, tarfile.open, tarname, mode="r:gz")
            self.assertRaises(tarfile.ReadError, tarfile.open, tarname, mode="r|gz")
            self.assertRaises(tarfile.ReadError, tarfile.open, gzipname, mode="r:")
            self.assertRaises(tarfile.ReadError, tarfile.open, gzipname, mode="r|")

            testfunc(gzipname, "r")
            testfunc(gzipname, "r:*")
            testfunc(gzipname, "r:gz")
            testfunc(gzipname, "r|*")
            testfunc(gzipname, "r|gz")

        if bz2:
            self.assertRaises(tarfile.ReadError, tarfile.open, tarname, mode="r:bz2")
            self.assertRaises(tarfile.ReadError, tarfile.open, tarname, mode="r|bz2")
            self.assertRaises(tarfile.ReadError, tarfile.open, bz2name, mode="r:")
            self.assertRaises(tarfile.ReadError, tarfile.open, bz2name, mode="r|")

            testfunc(bz2name, "r")
            testfunc(bz2name, "r:*")
            testfunc(bz2name, "r:bz2")
            testfunc(bz2name, "r|*")
            testfunc(bz2name, "r|bz2")

    def test_detect_file(self):
        self._test_modes(self._testfunc_file)

    def test_detect_fileobj(self):
        self._test_modes(self._testfunc_fileobj)


class MemberReadTest(ReadTest):

    def _test_member(self, tarinfo, chksum=None, **kwargs):
        if chksum is not None:
            self.assertTrue(md5sum(self.tar.extractfile(tarinfo).read()) == chksum,
                    "wrong md5sum for %s" % tarinfo.name)

        kwargs["mtime"] = 0o7606136617
        kwargs["uid"] = 1000
        kwargs["gid"] = 100
        if "old-v7" not in tarinfo.name:
            # V7 tar can't handle alphabetic owners.
            kwargs["uname"] = "tarfile"
            kwargs["gname"] = "tarfile"
        for k, v in kwargs.items():
            self.assertTrue(getattr(tarinfo, k) == v,
                    "wrong value in %s field of %s" % (k, tarinfo.name))

    def test_find_regtype(self):
        tarinfo = self.tar.getmember("ustar/regtype")
        self._test_member(tarinfo, size=7011, chksum=md5_regtype)

    def test_find_conttype(self):
        tarinfo = self.tar.getmember("ustar/conttype")
        self._test_member(tarinfo, size=7011, chksum=md5_regtype)

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
        self._test_member(tarinfo, size=86016, chksum=md5_sparse)

    def test_find_gnusparse(self):
        tarinfo = self.tar.getmember("gnu/sparse")
        self._test_member(tarinfo, size=86016, chksum=md5_sparse)

    def test_find_gnusparse_00(self):
        tarinfo = self.tar.getmember("gnu/sparse-0.0")
        self._test_member(tarinfo, size=86016, chksum=md5_sparse)

    def test_find_gnusparse_01(self):
        tarinfo = self.tar.getmember("gnu/sparse-0.1")
        self._test_member(tarinfo, size=86016, chksum=md5_sparse)

    def test_find_gnusparse_10(self):
        tarinfo = self.tar.getmember("gnu/sparse-1.0")
        self._test_member(tarinfo, size=86016, chksum=md5_sparse)

    def test_find_umlauts(self):
        tarinfo = self.tar.getmember("ustar/umlauts-\xc4\xd6\xdc\xe4\xf6\xfc\xdf")
        self._test_member(tarinfo, size=7011, chksum=md5_regtype)

    def test_find_ustar_longname(self):
        name = "ustar/" + "12345/" * 39 + "1234567/longname"
        self.assertIn(name, self.tar.getnames())

    def test_find_regtype_oldv7(self):
        tarinfo = self.tar.getmember("misc/regtype-old-v7")
        self._test_member(tarinfo, size=7011, chksum=md5_regtype)

    def test_find_pax_umlauts(self):
        self.tar.close()
        self.tar = tarfile.open(self.tarname, mode=self.mode, encoding="iso8859-1")
        tarinfo = self.tar.getmember("pax/umlauts-\xc4\xd6\xdc\xe4\xf6\xfc\xdf")
        self._test_member(tarinfo, size=7011, chksum=md5_regtype)


class LongnameTest(ReadTest):

    def test_read_longname(self):
        # Test reading of longname (bug #1471427).
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        try:
            tarinfo = self.tar.getmember(longname)
        except KeyError:
            self.fail("longname not found")
        self.assertTrue(tarinfo.type != tarfile.DIRTYPE, "read longname as dirtype")

    def test_read_longlink(self):
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        longlink = self.subdir + "/" + "123/" * 125 + "longlink"
        try:
            tarinfo = self.tar.getmember(longlink)
        except KeyError:
            self.fail("longlink not found")
        self.assertTrue(tarinfo.linkname == longname, "linkname wrong")

    def test_truncated_longname(self):
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        tarinfo = self.tar.getmember(longname)
        offset = tarinfo.offset
        self.tar.fileobj.seek(offset)
        fobj = io.BytesIO(self.tar.fileobj.read(3 * 512))
        self.assertRaises(tarfile.ReadError, tarfile.open, name="foo.tar", fileobj=fobj)

    def test_header_offset(self):
        # Test if the start offset of the TarInfo object includes
        # the preceding extended header.
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        offset = self.tar.getmember(longname).offset
        with open(tarname, "rb") as fobj:
            fobj.seek(offset)
            tarinfo = tarfile.TarInfo.frombuf(fobj.read(512), "iso8859-1", "strict")
            self.assertEqual(tarinfo.type, self.longnametype)


class GNUReadTest(LongnameTest):

    subdir = "gnu"
    longnametype = tarfile.GNUTYPE_LONGNAME

    # Since 3.2 tarfile is supposed to accurately restore sparse members and
    # produce files with holes. This is what we actually want to test here.
    # Unfortunately, not all platforms/filesystems support sparse files, and
    # even on platforms that do it is non-trivial to make reliable assertions
    # about holes in files. Therefore, we first do one basic test which works
    # an all platforms, and after that a test that will work only on
    # platforms/filesystems that prove to support sparse files.
    def _test_sparse_file(self, name):
        self.tar.extract(name, TEMPDIR)
        filename = os.path.join(TEMPDIR, name)
        with open(filename, "rb") as fobj:
            data = fobj.read()
        self.assertEqual(md5sum(data), md5_sparse,
                "wrong md5sum for %s" % name)

        if self._fs_supports_holes():
            s = os.stat(filename)
            self.assertTrue(s.st_blocks * 512 < s.st_size)

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
        # store holes in files.
        if sys.platform == "linux2":
            # Linux evidentially has 512 byte st_blocks units.
            name = os.path.join(TEMPDIR, "sparse-test")
            with open(name, "wb") as fobj:
                fobj.seek(4096)
                fobj.truncate()
            s = os.stat(name)
            os.remove(name)
            return s.st_blocks == 0
        else:
            return False


class PaxReadTest(LongnameTest):

    subdir = "pax"
    longnametype = tarfile.XHDTYPE

    def test_pax_global_headers(self):
        tar = tarfile.open(tarname, encoding="iso8859-1")
        try:
            tarinfo = tar.getmember("pax/regtype1")
            self.assertEqual(tarinfo.uname, "foo")
            self.assertEqual(tarinfo.gname, "bar")
            self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"), "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")

            tarinfo = tar.getmember("pax/regtype2")
            self.assertEqual(tarinfo.uname, "")
            self.assertEqual(tarinfo.gname, "bar")
            self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"), "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")

            tarinfo = tar.getmember("pax/regtype3")
            self.assertEqual(tarinfo.uname, "tarfile")
            self.assertEqual(tarinfo.gname, "tarfile")
            self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"), "\xc4\xd6\xdc\xe4\xf6\xfc\xdf")
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


class WriteTestBase(unittest.TestCase):
    # Put all write tests in here that are supposed to be tested
    # in all possible mode combinations.

    def test_fileobj_no_close(self):
        fobj = io.BytesIO()
        tar = tarfile.open(fileobj=fobj, mode=self.mode)
        tar.addfile(tarfile.TarInfo("foo"))
        tar.close()
        self.assertTrue(fobj.closed is False, "external fileobjs must never closed")


class WriteTest(WriteTestBase):

    mode = "w:"

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
            self.assertTrue(tar.getnames()[0] == name,
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
        self.assertTrue(os.path.getsize(tmpname) > 0,
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
            os.rmdir(path)

    def test_link_size(self):
        if hasattr(os, "link"):
            link = os.path.join(TEMPDIR, "link")
            target = os.path.join(TEMPDIR, "link_target")
            with open(target, "wb") as fobj:
                fobj.write(b"aaa")
            os.link(target, link)
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
                os.remove(target)
                os.remove(link)

    @support.skip_unless_symlink
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
            os.remove(path)

    def test_add_self(self):
        # Test for #1257255.
        dstname = os.path.abspath(tmpname)
        tar = tarfile.open(tmpname, self.mode)
        try:
            self.assertTrue(tar.name == dstname, "archive name must be absolute")
            tar.add(dstname)
            self.assertTrue(tar.getnames() == [], "added the archive to itself")

            cwd = os.getcwd()
            os.chdir(TEMPDIR)
            tar.add(dstname)
            os.chdir(cwd)
            self.assertTrue(tar.getnames() == [], "added the archive to itself")
        finally:
            tar.close()

    def test_exclude(self):
        tempdir = os.path.join(TEMPDIR, "exclude")
        os.mkdir(tempdir)
        try:
            for name in ("foo", "bar", "baz"):
                name = os.path.join(tempdir, name)
                open(name, "wb").close()

            exclude = os.path.isfile

            tar = tarfile.open(tmpname, self.mode, encoding="iso8859-1")
            try:
                with support.check_warnings(("use the filter argument",
                                             DeprecationWarning)):
                    tar.add(tempdir, arcname="empty_dir", exclude=exclude)
            finally:
                tar.close()

            tar = tarfile.open(tmpname, "r")
            try:
                self.assertEqual(len(tar.getmembers()), 1)
                self.assertEqual(tar.getnames()[0], "empty_dir")
            finally:
                tar.close()
        finally:
            shutil.rmtree(tempdir)

    def test_filter(self):
        tempdir = os.path.join(TEMPDIR, "filter")
        os.mkdir(tempdir)
        try:
            for name in ("foo", "bar", "baz"):
                name = os.path.join(tempdir, name)
                open(name, "wb").close()

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

            tar = tarfile.open(tmpname, "r")
            try:
                for tarinfo in tar:
                    self.assertEqual(tarinfo.uid, 123)
                    self.assertEqual(tarinfo.uname, "foo")
                self.assertEqual(len(tar.getmembers()), 3)
            finally:
                tar.close()
        finally:
            shutil.rmtree(tempdir)

    # Guarantee that stored pathnames are not modified. Don't
    # remove ./ or ../ or double slashes. Still make absolute
    # pathnames relative.
    # For details see bug #6054.
    def _test_pathname(self, path, cmp_path=None, dir=False):
        # Create a tarfile with an empty member named path
        # and compare the stored name with the original.
        foo = os.path.join(TEMPDIR, "foo")
        if not dir:
            open(foo, "w").close()
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
            os.remove(foo)
        else:
            os.rmdir(foo)

        self.assertEqual(t.name, cmp_path or path.replace(os.sep, "/"))

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
        cwd = os.getcwd()
        os.chdir(TEMPDIR)
        try:
            tar = tarfile.open(tmpname, self.mode)
            try:
                tar.add(".")
            finally:
                tar.close()

            tar = tarfile.open(tmpname, "r")
            try:
                for t in tar:
                    self.assertTrue(t.name == "." or t.name.startswith("./"))
            finally:
                tar.close()
        finally:
            os.chdir(cwd)


class StreamWriteTest(WriteTestBase):

    mode = "w|"

    def test_stream_padding(self):
        # Test for bug #1543303.
        tar = tarfile.open(tmpname, self.mode)
        tar.close()

        if self.mode.endswith("gz"):
            with gzip.GzipFile(tmpname) as fobj:
                data = fobj.read()
        elif self.mode.endswith("bz2"):
            dec = bz2.BZ2Decompressor()
            with open(tmpname, "rb") as fobj:
                data = fobj.read()
            data = dec.decompress(data)
            self.assertTrue(len(dec.unused_data) == 0,
                    "found trailing data")
        else:
            with open(tmpname, "rb") as fobj:
                data = fobj.read()

        self.assertTrue(data.count(b"\0") == tarfile.RECORDSIZE,
                         "incorrect zero padding")

    def test_file_mode(self):
        # Test for issue #8464: Create files with correct
        # permissions.
        if sys.platform == "win32" or not hasattr(os, "umask"):
            return

        if os.path.exists(tmpname):
            os.remove(tmpname)

        original_umask = os.umask(0o022)
        try:
            tar = tarfile.open(tmpname, self.mode)
            tar.close()
            mode = os.stat(tmpname).st_mode & 0o777
            self.assertEqual(mode, 0o644, "wrong file permissions")
        finally:
            os.umask(original_umask)


class GNUWriteTest(unittest.TestCase):
    # This testcase checks for correct creation of GNU Longname
    # and Longlink extended headers (cp. bug #812325).

    def _length(self, s):
        blocks, remainder = divmod(len(s) + 1, 512)
        if remainder:
            blocks += 1
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
            self.assertTrue(v1 == v2, "GNU longname/longlink creation failed")
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


class HardlinkTest(unittest.TestCase):
    # Test the creation of LNKTYPE (hardlink) members in an archive.

    def setUp(self):
        self.foo = os.path.join(TEMPDIR, "foo")
        self.bar = os.path.join(TEMPDIR, "bar")

        with open(self.foo, "wb") as fobj:
            fobj.write(b"foo")

        os.link(self.foo, self.bar)

        self.tar = tarfile.open(tmpname, "w")
        self.tar.add(self.foo)

    def tearDown(self):
        self.tar.close()
        support.unlink(self.foo)
        support.unlink(self.bar)

    def test_add_twice(self):
        # The same name will be added as a REGTYPE every
        # time regardless of st_nlink.
        tarinfo = self.tar.gettarinfo(self.foo)
        self.assertTrue(tarinfo.type == tarfile.REGTYPE,
                "add file as regular failed")

    def test_add_hardlink(self):
        tarinfo = self.tar.gettarinfo(self.bar)
        self.assertTrue(tarinfo.type == tarfile.LNKTYPE,
                "add file as hardlink failed")

    def test_dereference_hardlink(self):
        self.tar.dereference = True
        tarinfo = self.tar.gettarinfo(self.bar)
        self.assertTrue(tarinfo.type == tarfile.REGTYPE,
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
                self.assertTrue(link == l, "PAX longlink creation failed")
            else:
                n = tar.getmembers()[0].name
                self.assertTrue(name == n, "PAX longname creation failed")
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
                self.assertTrue(type(key) is not bytes)
                self.assertTrue(type(val) is not bytes)
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

        tar = tarfile.open(tmpname, "w", format=tarfile.PAX_FORMAT, encoding="iso8859-1")
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


class UstarUnicodeTest(unittest.TestCase):

    format = tarfile.USTAR_FORMAT

    def test_iso8859_1_filename(self):
        self._test_unicode_filename("iso8859-1")

    def test_utf7_filename(self):
        self._test_unicode_filename("utf7")

    def test_utf8_filename(self):
        self._test_unicode_filename("utf8")

    def _test_unicode_filename(self, encoding):
        tar = tarfile.open(tmpname, "w", format=self.format, encoding=encoding, errors="strict")
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
        if self.format == tarfile.PAX_FORMAT:
            # PAX_FORMAT ignores encoding in write mode.
            return

        tar = tarfile.open(tmpname, "w", format=self.format, encoding="ascii", errors="strict")
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
        tar = tarfile.open(tarname, "r", encoding="iso8859-1", errors="strict")
        try:
            for t in tar:
                self.assertTrue(type(t.name) is str)
                self.assertTrue(type(t.linkname) is str)
                self.assertTrue(type(t.uname) is str)
                self.assertTrue(type(t.gname) is str)
        finally:
            tar.close()

    def test_uname_unicode(self):
        t = tarfile.TarInfo("foo")
        t.uname = "\xe4\xf6\xfc"
        t.gname = "\xe4\xf6\xfc"

        tar = tarfile.open(tmpname, mode="w", format=self.format, encoding="iso8859-1")
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


class GNUUnicodeTest(UstarUnicodeTest):

    format = tarfile.GNU_FORMAT

    def test_bad_pax_header(self):
        # Test for issue #8633. GNU tar <= 1.23 creates raw binary fields
        # without a hdrcharset=BINARY header.
        for encoding, name in (("utf8", "pax/bad-pax-\udce4\udcf6\udcfc"),
                ("iso8859-1", "pax/bad-pax-\xe4\xf6\xfc"),):
            with tarfile.open(tarname, encoding=encoding, errors="surrogateescape") as tar:
                try:
                    t = tar.getmember(name)
                except KeyError:
                    self.fail("unable to read bad GNU tar pax header")


class PAXUnicodeTest(UstarUnicodeTest):

    format = tarfile.PAX_FORMAT

    def test_binary_header(self):
        # Test a POSIX.1-2008 compatible header with a hdrcharset=BINARY field.
        for encoding, name in (("utf8", "pax/hdrcharset-\udce4\udcf6\udcfc"),
                ("iso8859-1", "pax/hdrcharset-\xe4\xf6\xfc"),):
            with tarfile.open(tarname, encoding=encoding, errors="surrogateescape") as tar:
                try:
                    t = tar.getmember(name)
                except KeyError:
                    self.fail("unable to read POSIX.1-2008 binary header")


class AppendTest(unittest.TestCase):
    # Test append mode (cp. patch #1652681).

    def setUp(self):
        self.tarname = tmpname
        if os.path.exists(self.tarname):
            os.remove(self.tarname)

    def _add_testfile(self, fileobj=None):
        with tarfile.open(self.tarname, "a", fileobj=fileobj) as tar:
            tar.addfile(tarfile.TarInfo("bar"))

    def _create_testtar(self, mode="w:"):
        with tarfile.open(tarname, encoding="iso8859-1") as src:
            t = src.getmember("ustar/regtype")
            t.name = "foo"
            f = src.extractfile(t)
            try:
                with tarfile.open(self.tarname, mode) as tar:
                    tar.addfile(t, f)
            finally:
                f.close()

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

    def test_append_gz(self):
        if gzip is None:
            return
        self._create_testtar("w:gz")
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname, "a")

    def test_append_bz2(self):
        if bz2 is None:
            return
        self._create_testtar("w:bz2")
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname, "a")

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
        self.assertEqual(tarfile.stn("foo", 8, "ascii", "strict"), b"foo\0\0\0\0\0")
        self.assertEqual(tarfile.stn("foobar", 3, "ascii", "strict"), b"foo")
        self.assertEqual(tarfile.nts(b"foo\0\0\0\0\0", "ascii", "strict"), "foo")
        self.assertEqual(tarfile.nts(b"foo\0bar\0", "ascii", "strict"), "foo")

    def test_number_fields(self):
        self.assertEqual(tarfile.itn(1), b"0000001\x00")
        self.assertEqual(tarfile.itn(0xffffffff), b"\x80\x00\x00\x00\xff\xff\xff\xff")


class ContextManagerTest(unittest.TestCase):

    def test_basic(self):
        with tarfile.open(tarname) as tar:
            self.assertFalse(tar.closed, "closed inside runtime context")
        self.assertTrue(tar.closed, "context manager failed")

    def test_closed(self):
        # The __enter__() method is supposed to raise IOError
        # if the TarFile object is already closed.
        tar = tarfile.open(tarname)
        tar.close()
        with self.assertRaises(IOError):
            with tar:
                pass

    def test_exception(self):
        # Test if the IOError exception is passed through properly.
        with self.assertRaises(Exception) as exc:
            with tarfile.open(tarname) as tar:
                raise IOError
        self.assertIsInstance(exc.exception, IOError,
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


class LinkEmulationTest(ReadTest):

    # Test for issue #8741 regression. On platforms that do not support
    # symbolic or hard links tarfile tries to extract these types of members as
    # the regular files they point to.
    def _test_link_extraction(self, name):
        self.tar.extract(name, TEMPDIR)
        data = open(os.path.join(TEMPDIR, name), "rb").read()
        self.assertEqual(md5sum(data), md5_regtype)

    # When 8879 gets fixed, this will need to change. Currently on Windows
    # we have os.path.islink but no os.link, so these tests fail without the
    # following skip until link is completed.
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


class GzipMiscReadTest(MiscReadTest):
    tarname = gzipname
    mode = "r:gz"
class GzipUstarReadTest(UstarReadTest):
    tarname = gzipname
    mode = "r:gz"
class GzipStreamReadTest(StreamReadTest):
    tarname = gzipname
    mode = "r|gz"
class GzipWriteTest(WriteTest):
    mode = "w:gz"
class GzipStreamWriteTest(StreamWriteTest):
    mode = "w|gz"


class Bz2MiscReadTest(MiscReadTest):
    tarname = bz2name
    mode = "r:bz2"
class Bz2UstarReadTest(UstarReadTest):
    tarname = bz2name
    mode = "r:bz2"
class Bz2StreamReadTest(StreamReadTest):
    tarname = bz2name
    mode = "r|bz2"
class Bz2WriteTest(WriteTest):
    mode = "w:bz2"
class Bz2StreamWriteTest(StreamWriteTest):
    mode = "w|bz2"

class Bz2PartialReadTest(unittest.TestCase):
    # Issue5068: The _BZ2Proxy.read() method loops forever
    # on an empty or partial bzipped file.

    def _test_partial_input(self, mode):
        class MyBytesIO(io.BytesIO):
            hit_eof = False
            def read(self, n):
                if self.hit_eof:
                    raise AssertionError("infinite loop detected in tarfile.open()")
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


def test_main():
    support.unlink(TEMPDIR)
    os.makedirs(TEMPDIR)

    tests = [
        UstarReadTest,
        MiscReadTest,
        StreamReadTest,
        DetectReadTest,
        MemberReadTest,
        GNUReadTest,
        PaxReadTest,
        WriteTest,
        StreamWriteTest,
        GNUWriteTest,
        PaxWriteTest,
        UstarUnicodeTest,
        GNUUnicodeTest,
        PAXUnicodeTest,
        AppendTest,
        LimitsTest,
        MiscTest,
        ContextManagerTest,
    ]

    if hasattr(os, "link"):
        tests.append(HardlinkTest)
    else:
        tests.append(LinkEmulationTest)

    with open(tarname, "rb") as fobj:
        data = fobj.read()

    if gzip:
        # Create testtar.tar.gz and add gzip-specific tests.
        support.unlink(gzipname)
        with gzip.open(gzipname, "wb") as tar:
            tar.write(data)

        tests += [
            GzipMiscReadTest,
            GzipUstarReadTest,
            GzipStreamReadTest,
            GzipWriteTest,
            GzipStreamWriteTest,
        ]

    if bz2:
        # Create testtar.tar.bz2 and add bz2-specific tests.
        support.unlink(bz2name)
        tar = bz2.BZ2File(bz2name, "wb")
        try:
            tar.write(data)
        finally:
            tar.close()

        tests += [
            Bz2MiscReadTest,
            Bz2UstarReadTest,
            Bz2StreamReadTest,
            Bz2WriteTest,
            Bz2StreamWriteTest,
            Bz2PartialReadTest,
        ]

    try:
        support.run_unittest(*tests)
    finally:
        if os.path.exists(TEMPDIR):
            shutil.rmtree(TEMPDIR)

if __name__ == "__main__":
    test_main()
