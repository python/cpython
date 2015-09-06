# -*- coding: iso-8859-15 -*-

import sys
import os
import shutil
import StringIO
from hashlib import md5
import errno

import unittest
import tarfile

from test import test_support
from test import test_support as support

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

TEMPDIR = os.path.abspath(test_support.TESTFN)
tarname = test_support.findfile("testtar.tar")
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
        data = fobj.read()
        self.assertTrue((len(data), md5sum(data)) == (tarinfo.size, md5_regtype),
                "regular file extraction failed")

    def test_fileobj_readlines(self):
        self.tar.extract("ustar/regtype", TEMPDIR)
        tarinfo = self.tar.getmember("ustar/regtype")
        fobj1 = open(os.path.join(TEMPDIR, "ustar/regtype"), "rU")
        fobj2 = self.tar.extractfile(tarinfo)

        lines1 = fobj1.readlines()
        lines2 = fobj2.readlines()
        self.assertTrue(lines1 == lines2,
                "fileobj.readlines() failed")
        self.assertTrue(len(lines2) == 114,
                "fileobj.readlines() failed")
        self.assertTrue(lines2[83] ==
                "I will gladly admit that Python is not the fastest running scripting language.\n",
                "fileobj.readlines() failed")

    def test_fileobj_iter(self):
        self.tar.extract("ustar/regtype", TEMPDIR)
        tarinfo = self.tar.getmember("ustar/regtype")
        fobj1 = open(os.path.join(TEMPDIR, "ustar/regtype"), "rU")
        fobj2 = self.tar.extractfile(tarinfo)
        lines1 = fobj1.readlines()
        lines2 = [line for line in fobj2]
        self.assertTrue(lines1 == lines2,
                     "fileobj.__iter__() failed")

    def test_fileobj_seek(self):
        self.tar.extract("ustar/regtype", TEMPDIR)
        fobj = open(os.path.join(TEMPDIR, "ustar/regtype"), "rb")
        data = fobj.read()
        fobj.close()

        tarinfo = self.tar.getmember("ustar/regtype")
        fobj = self.tar.extractfile(tarinfo)

        text = fobj.read()
        fobj.seek(0)
        self.assertTrue(0 == fobj.tell(),
                     "seek() to file's start failed")
        fobj.seek(2048, 0)
        self.assertTrue(2048 == fobj.tell(),
                     "seek() to absolute position failed")
        fobj.seek(-1024, 1)
        self.assertTrue(1024 == fobj.tell(),
                     "seek() to negative relative position failed")
        fobj.seek(1024, 1)
        self.assertTrue(2048 == fobj.tell(),
                     "seek() to positive relative position failed")
        s = fobj.read(10)
        self.assertTrue(s == data[2048:2058],
                     "read() after seek failed")
        fobj.seek(0, 2)
        self.assertTrue(tarinfo.size == fobj.tell(),
                     "seek() to file's end failed")
        self.assertTrue(fobj.read() == "",
                     "read() at file's end did not return empty string")
        fobj.seek(-tarinfo.size, 2)
        self.assertTrue(0 == fobj.tell(),
                     "relative seek() to file's start failed")
        fobj.seek(512)
        s1 = fobj.readlines()
        fobj.seek(512)
        s2 = fobj.readlines()
        self.assertTrue(s1 == s2,
                     "readlines() after seek failed")
        fobj.seek(0)
        self.assertTrue(len(fobj.readline()) == fobj.tell(),
                     "tell() after readline() failed")
        fobj.seek(512)
        self.assertTrue(len(fobj.readline()) + 512 == fobj.tell(),
                     "tell() after seek() and readline() failed")
        fobj.seek(0)
        line = fobj.readline()
        self.assertTrue(fobj.read() == data[len(line):],
                     "read() after readline() failed")
        fobj.close()

    # Test if symbolic and hard links are resolved by extractfile().  The
    # test link members each point to a regular member whose data is
    # supposed to be exported.
    def _test_fileobj_link(self, lnktype, regtype):
        a = self.tar.extractfile(lnktype)
        b = self.tar.extractfile(regtype)
        self.assertEqual(a.name, b.name)

    def test_fileobj_link1(self):
        self._test_fileobj_link("ustar/lnktype", "ustar/regtype")

    def test_fileobj_link2(self):
        self._test_fileobj_link("./ustar/linktest2/lnktype", "ustar/linktest1/regtype")

    def test_fileobj_symlink1(self):
        self._test_fileobj_link("ustar/symtype", "ustar/regtype")

    def test_fileobj_symlink2(self):
        self._test_fileobj_link("./ustar/linktest2/symtype", "ustar/linktest1/regtype")

    def test_issue14160(self):
        self._test_fileobj_link("symtype2", "ustar/regtype")


class ListTest(ReadTest, unittest.TestCase):

    # Override setUp to use default encoding (UTF-8)
    def setUp(self):
        self.tar = tarfile.open(self.tarname, mode=self.mode)

    def test_list(self):
        with test_support.captured_stdout() as t:
            self.tar.list(verbose=False)
        out = t.getvalue()
        self.assertIn('ustar/conttype', out)
        self.assertIn('ustar/regtype', out)
        self.assertIn('ustar/lnktype', out)
        self.assertIn('ustar' + ('/12345' * 40) + '67/longname', out)
        self.assertIn('./ustar/linktest2/symtype', out)
        self.assertIn('./ustar/linktest2/lnktype', out)
        # Make sure it puts trailing slash for directory
        self.assertIn('ustar/dirtype/', out)
        self.assertIn('ustar/dirtype-with-size/', out)
        # Make sure it is able to print non-ASCII characters
        self.assertIn('ustar/umlauts-'
                      '\xc4\xd6\xdc\xe4\xf6\xfc\xdf', out)
        self.assertIn('misc/regtype-hpux-signed-chksum-'
                      '\xc4\xd6\xdc\xe4\xf6\xfc\xdf', out)
        self.assertIn('misc/regtype-old-v7-signed-chksum-'
                      '\xc4\xd6\xdc\xe4\xf6\xfc\xdf', out)
        # Make sure it prints files separated by one newline without any
        # 'ls -l'-like accessories if verbose flag is not being used
        # ...
        # ustar/conttype
        # ustar/regtype
        # ...
        self.assertRegexpMatches(out, r'ustar/conttype ?\r?\n'
                                      r'ustar/regtype ?\r?\n')
        # Make sure it does not print the source of link without verbose flag
        self.assertNotIn('link to', out)
        self.assertNotIn('->', out)

    def test_list_verbose(self):
        with test_support.captured_stdout() as t:
            self.tar.list(verbose=True)
        out = t.getvalue()
        # Make sure it prints files separated by one newline with 'ls -l'-like
        # accessories if verbose flag is being used
        # ...
        # ?rw-r--r-- tarfile/tarfile     7011 2003-01-06 07:19:43 ustar/conttype
        # ?rw-r--r-- tarfile/tarfile     7011 2003-01-06 07:19:43 ustar/regtype
        # ...
        self.assertRegexpMatches(out, (r'-rw-r--r-- tarfile/tarfile\s+7011 '
                                       r'\d{4}-\d\d-\d\d\s+\d\d:\d\d:\d\d '
                                       r'ustar/\w+type ?\r?\n') * 2)
        # Make sure it prints the source of link with verbose flag
        self.assertIn('ustar/symtype -> regtype', out)
        self.assertIn('./ustar/linktest2/symtype -> ../linktest1/regtype', out)
        self.assertIn('./ustar/linktest2/lnktype link to '
                      './ustar/linktest1/regtype', out)
        self.assertIn('gnu' + ('/123' * 125) + '/longlink link to gnu' +
                      ('/123' * 125) + '/longname', out)
        self.assertIn('pax' + ('/123' * 125) + '/longlink link to pax' +
                      ('/123' * 125) + '/longname', out)


class GzipListTest(ListTest):
    tarname = gzipname
    mode = "r:gz"
    taropen = tarfile.TarFile.gzopen


class Bz2ListTest(ListTest):
    tarname = bz2name
    mode = "r:bz2"
    taropen = tarfile.TarFile.bz2open


class CommonReadTest(ReadTest):

    def test_empty_tarfile(self):
        # Test for issue6123: Allow opening empty archives.
        # This test checks if tarfile.open() is able to open an empty tar
        # archive successfully. Note that an empty tar archive is not the
        # same as an empty file!
        tarfile.open(tmpname, self.mode.replace("r", "w")).close()
        try:
            tar = tarfile.open(tmpname, self.mode)
            tar.getnames()
        except tarfile.ReadError:
            self.fail("tarfile.open() failed on empty archive")
        self.assertListEqual(tar.getmembers(), [])

    def test_null_tarfile(self):
        # Test for issue6123: Allow opening empty archives.
        # This test guarantees that tarfile.open() does not treat an empty
        # file as an empty tar archive.
        open(tmpname, "wb").close()
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname, self.mode)
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname)

    def test_non_existent_tarfile(self):
        # Test for issue11513: prevent non-existent gzipped tarfiles raising
        # multiple exceptions.
        exctype = OSError if '|' in self.mode else IOError
        with self.assertRaisesRegexp(exctype, "xxx") as ex:
            tarfile.open("xxx", self.mode)
        self.assertEqual(ex.exception.errno, errno.ENOENT)

    def test_ignore_zeros(self):
        # Test TarFile's ignore_zeros option.
        if self.mode.endswith(":gz"):
            _open = gzip.GzipFile
        elif self.mode.endswith(":bz2"):
            _open = bz2.BZ2File
        else:
            _open = open

        for char in ('\0', 'a'):
            # Test if EOFHeaderError ('\0') and InvalidHeaderError ('a')
            # are ignored correctly.
            fobj = _open(tmpname, "wb")
            fobj.write(char * 1024)
            fobj.write(tarfile.TarInfo("foo").tobuf())
            fobj.close()

            tar = tarfile.open(tmpname, mode="r", ignore_zeros=True)
            self.assertListEqual(tar.getnames(), ["foo"],
                    "ignore_zeros=True should have skipped the %r-blocks" % char)
            tar.close()

    def test_premature_end_of_archive(self):
        for size in (512, 600, 1024, 1200):
            with tarfile.open(tmpname, "w:") as tar:
                t = tarfile.TarInfo("foo")
                t.size = 1024
                tar.addfile(t, StringIO.StringIO("a" * 1024))

            with open(tmpname, "r+b") as fobj:
                fobj.truncate(size)

            with tarfile.open(tmpname) as tar:
                with self.assertRaisesRegexp(tarfile.ReadError, "unexpected end of data"):
                    for t in tar:
                        pass

            with tarfile.open(tmpname) as tar:
                t = tar.next()

                with self.assertRaisesRegexp(tarfile.ReadError, "unexpected end of data"):
                    tar.extract(t, TEMPDIR)

                with self.assertRaisesRegexp(tarfile.ReadError, "unexpected end of data"):
                    tar.extractfile(t).read()


class MiscReadTest(CommonReadTest):
    taropen = tarfile.TarFile.taropen

    def test_no_name_argument(self):
        fobj = open(self.tarname, "rb")
        tar = tarfile.open(fileobj=fobj, mode=self.mode)
        self.assertEqual(tar.name, os.path.abspath(fobj.name))

    def test_no_name_attribute(self):
        data = open(self.tarname, "rb").read()
        fobj = StringIO.StringIO(data)
        self.assertRaises(AttributeError, getattr, fobj, "name")
        tar = tarfile.open(fileobj=fobj, mode=self.mode)
        self.assertEqual(tar.name, None)

    def test_empty_name_attribute(self):
        data = open(self.tarname, "rb").read()
        fobj = StringIO.StringIO(data)
        fobj.name = ""
        tar = tarfile.open(fileobj=fobj, mode=self.mode)
        self.assertEqual(tar.name, None)

    def test_illegal_mode_arg(self):
        with open(tmpname, 'wb'):
            pass
        self.addCleanup(os.unlink, tmpname)
        with self.assertRaisesRegexp(ValueError, 'mode must be '):
            tar = self.taropen(tmpname, 'q')
        with self.assertRaisesRegexp(ValueError, 'mode must be '):
            tar = self.taropen(tmpname, 'rw')
        with self.assertRaisesRegexp(ValueError, 'mode must be '):
            tar = self.taropen(tmpname, '')

    def test_fileobj_with_offset(self):
        # Skip the first member and store values from the second member
        # of the testtar.
        tar = tarfile.open(self.tarname, mode=self.mode)
        tar.next()
        t = tar.next()
        name = t.name
        offset = t.offset
        data = tar.extractfile(t).read()
        tar.close()

        # Open the testtar and seek to the offset of the second member.
        if self.mode.endswith(":gz"):
            _open = gzip.GzipFile
        elif self.mode.endswith(":bz2"):
            _open = bz2.BZ2File
        else:
            _open = open
        fobj = _open(self.tarname, "rb")
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

    def test_fail_comp(self):
        # For Gzip and Bz2 Tests: fail with a ReadError on an uncompressed file.
        if self.mode == "r:":
            self.skipTest('needs a gz or bz2 mode')
        self.assertRaises(tarfile.ReadError, tarfile.open, tarname, self.mode)
        fobj = open(tarname, "rb")
        self.assertRaises(tarfile.ReadError, tarfile.open, fileobj=fobj, mode=self.mode)

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
            self.assertTrue(int(tarinfo.mtime) == 07606136617,
                    "wrong mtime for %s" % tarinfo.name)
            if not tarinfo.name.startswith("ustar/"):
                continue
            self.assertTrue(tarinfo.uname == "tarfile",
                    "wrong uname for %s" % tarinfo.name)

    def test_find_members(self):
        self.assertTrue(self.tar.getmembers()[-1].name == "misc/eof",
                "could not find all members")

    def test_extract_hardlink(self):
        # Test hardlink extraction (e.g. bug #857297).
        with tarfile.open(tarname, errorlevel=1, encoding="iso8859-1") as tar:
            tar.extract("ustar/regtype", TEMPDIR)
            self.addCleanup(os.remove, os.path.join(TEMPDIR, "ustar/regtype"))

            tar.extract("ustar/lnktype", TEMPDIR)
            self.addCleanup(os.remove, os.path.join(TEMPDIR, "ustar/lnktype"))
            with open(os.path.join(TEMPDIR, "ustar/lnktype"), "rb") as f:
                data = f.read()
            self.assertEqual(md5sum(data), md5_regtype)

            tar.extract("ustar/symtype", TEMPDIR)
            self.addCleanup(os.remove, os.path.join(TEMPDIR, "ustar/symtype"))
            with open(os.path.join(TEMPDIR, "ustar/symtype"), "rb") as f:
                data = f.read()
            self.assertEqual(md5sum(data), md5_regtype)

    def test_extractall(self):
        # Test if extractall() correctly restores directory permissions
        # and times (see issue1735).
        tar = tarfile.open(tarname, encoding="iso8859-1")
        directories = [t for t in tar if t.isdir()]
        tar.extractall(TEMPDIR, directories)
        for tarinfo in directories:
            path = os.path.join(TEMPDIR, tarinfo.name)
            if sys.platform != "win32":
                # Win32 has no support for fine grained permissions.
                self.assertEqual(tarinfo.mode & 0777, os.stat(path).st_mode & 0777)
            self.assertEqual(tarinfo.mtime, os.path.getmtime(path))
        tar.close()

    def test_init_close_fobj(self):
        # Issue #7341: Close the internal file object in the TarFile
        # constructor in case of an error. For the test we rely on
        # the fact that opening an empty file raises a ReadError.
        empty = os.path.join(TEMPDIR, "empty")
        open(empty, "wb").write("")

        try:
            tar = object.__new__(tarfile.TarFile)
            try:
                tar.__init__(empty)
            except tarfile.ReadError:
                self.assertTrue(tar.fileobj.closed)
            else:
                self.fail("ReadError not raised")
        finally:
            os.remove(empty)

    def test_parallel_iteration(self):
        # Issue #16601: Restarting iteration over tarfile continued
        # from where it left off.
        with tarfile.open(self.tarname) as tar:
            for m1, m2 in zip(tar, tar):
                self.assertEqual(m1.offset, m2.offset)
                self.assertEqual(m1.name, m2.name)


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
            self.assertTrue(v1.read() == v2.read(), "stream extraction failed")

        tar1.close()


class DetectReadTest(unittest.TestCase):

    def _testfunc_file(self, name, mode):
        try:
            tarfile.open(name, mode)
        except tarfile.ReadError:
            self.fail()

    def _testfunc_fileobj(self, name, mode):
        try:
            tarfile.open(name, mode, fileobj=open(name, "rb"))
        except tarfile.ReadError:
            self.fail()

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

    @unittest.skipUnless(bz2, 'requires bz2')
    def test_detect_stream_bz2(self):
        # Originally, tarfile's stream detection looked for the string
        # "BZh91" at the start of the file. This is incorrect because
        # the '9' represents the blocksize (900kB). If the file was
        # compressed using another blocksize autodetection fails.
        with open(tarname, "rb") as fobj:
            data = fobj.read()

        # Compress with blocksize 100kB, the file starts with "BZh11".
        with bz2.BZ2File(tmpname, "wb", compresslevel=1) as fobj:
            fobj.write(data)

        self._testfunc_file(tmpname, "r|*")


class MemberReadTest(ReadTest):

    def _test_member(self, tarinfo, chksum=None, **kwargs):
        if chksum is not None:
            self.assertTrue(md5sum(self.tar.extractfile(tarinfo).read()) == chksum,
                    "wrong md5sum for %s" % tarinfo.name)

        kwargs["mtime"] = 07606136617
        kwargs["uid"] = 1000
        kwargs["gid"] = 100
        if "old-v7" not in tarinfo.name:
            # V7 tar can't handle alphabetic owners.
            kwargs["uname"] = "tarfile"
            kwargs["gname"] = "tarfile"
        for k, v in kwargs.iteritems():
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

    def test_find_umlauts(self):
        tarinfo = self.tar.getmember("ustar/umlauts-ÄÖÜäöüß")
        self._test_member(tarinfo, size=7011, chksum=md5_regtype)

    def test_find_ustar_longname(self):
        name = "ustar/" + "12345/" * 39 + "1234567/longname"
        self.assertIn(name, self.tar.getnames())

    def test_find_regtype_oldv7(self):
        tarinfo = self.tar.getmember("misc/regtype-old-v7")
        self._test_member(tarinfo, size=7011, chksum=md5_regtype)

    def test_find_pax_umlauts(self):
        self.tar = tarfile.open(self.tarname, mode=self.mode, encoding="iso8859-1")
        tarinfo = self.tar.getmember("pax/umlauts-ÄÖÜäöüß")
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
        fobj = StringIO.StringIO(self.tar.fileobj.read(3 * 512))
        self.assertRaises(tarfile.ReadError, tarfile.open, name="foo.tar", fileobj=fobj)

    def test_header_offset(self):
        # Test if the start offset of the TarInfo object includes
        # the preceding extended header.
        longname = self.subdir + "/" + "123/" * 125 + "longname"
        offset = self.tar.getmember(longname).offset
        fobj = open(tarname)
        fobj.seek(offset)
        tarinfo = tarfile.TarInfo.frombuf(fobj.read(512))
        self.assertEqual(tarinfo.type, self.longnametype)


class GNUReadTest(LongnameTest):

    subdir = "gnu"
    longnametype = tarfile.GNUTYPE_LONGNAME

    def test_sparse_file(self):
        tarinfo1 = self.tar.getmember("ustar/sparse")
        fobj1 = self.tar.extractfile(tarinfo1)
        tarinfo2 = self.tar.getmember("gnu/sparse")
        fobj2 = self.tar.extractfile(tarinfo2)
        self.assertTrue(fobj1.read() == fobj2.read(),
                "sparse file extraction failed")


class PaxReadTest(LongnameTest):

    subdir = "pax"
    longnametype = tarfile.XHDTYPE

    def test_pax_global_headers(self):
        tar = tarfile.open(tarname, encoding="iso8859-1")

        tarinfo = tar.getmember("pax/regtype1")
        self.assertEqual(tarinfo.uname, "foo")
        self.assertEqual(tarinfo.gname, "bar")
        self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"), u"ÄÖÜäöüß")

        tarinfo = tar.getmember("pax/regtype2")
        self.assertEqual(tarinfo.uname, "")
        self.assertEqual(tarinfo.gname, "bar")
        self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"), u"ÄÖÜäöüß")

        tarinfo = tar.getmember("pax/regtype3")
        self.assertEqual(tarinfo.uname, "tarfile")
        self.assertEqual(tarinfo.gname, "tarfile")
        self.assertEqual(tarinfo.pax_headers.get("VENDOR.umlauts"), u"ÄÖÜäöüß")

    def test_pax_number_fields(self):
        # All following number fields are read from the pax header.
        tar = tarfile.open(tarname, encoding="iso8859-1")
        tarinfo = tar.getmember("pax/regtype4")
        self.assertEqual(tarinfo.size, 7011)
        self.assertEqual(tarinfo.uid, 123)
        self.assertEqual(tarinfo.gid, 123)
        self.assertEqual(tarinfo.mtime, 1041808783.0)
        self.assertEqual(type(tarinfo.mtime), float)
        self.assertEqual(float(tarinfo.pax_headers["atime"]), 1041808783.0)
        self.assertEqual(float(tarinfo.pax_headers["ctime"]), 1041808783.0)


class WriteTestBase(unittest.TestCase):
    # Put all write tests in here that are supposed to be tested
    # in all possible mode combinations.

    def test_fileobj_no_close(self):
        fobj = StringIO.StringIO()
        tar = tarfile.open(fileobj=fobj, mode=self.mode)
        tar.addfile(tarfile.TarInfo("foo"))
        tar.close()
        self.assertTrue(fobj.closed is False, "external fileobjs must never closed")
        # Issue #20238: Incomplete gzip output with mode="w:gz"
        data = fobj.getvalue()
        del tar
        test_support.gc_collect()
        self.assertFalse(fobj.closed)
        self.assertEqual(data, fobj.getvalue())


class WriteTest(WriteTestBase):

    mode = "w:"

    def test_100_char_name(self):
        # The name field in a tar header stores strings of at most 100 chars.
        # If a string is shorter than 100 chars it has to be padded with '\0',
        # which implies that a string of exactly 100 chars is stored without
        # a trailing '\0'.
        name = "0123456789" * 10
        tar = tarfile.open(tmpname, self.mode)
        t = tarfile.TarInfo(name)
        tar.addfile(t)
        tar.close()

        tar = tarfile.open(tmpname)
        self.assertTrue(tar.getnames()[0] == name,
                "failed to store 100 char filename")
        tar.close()

    def test_tar_size(self):
        # Test for bug #1013882.
        tar = tarfile.open(tmpname, self.mode)
        path = os.path.join(TEMPDIR, "file")
        fobj = open(path, "wb")
        fobj.write("aaa")
        fobj.close()
        tar.add(path)
        tar.close()
        self.assertTrue(os.path.getsize(tmpname) > 0,
                "tarfile is empty")

    # The test_*_size tests test for bug #1167128.
    def test_file_size(self):
        tar = tarfile.open(tmpname, self.mode)

        path = os.path.join(TEMPDIR, "file")
        fobj = open(path, "wb")
        fobj.close()
        tarinfo = tar.gettarinfo(path)
        self.assertEqual(tarinfo.size, 0)

        fobj = open(path, "wb")
        fobj.write("aaa")
        fobj.close()
        tarinfo = tar.gettarinfo(path)
        self.assertEqual(tarinfo.size, 3)

        tar.close()

    def test_directory_size(self):
        path = os.path.join(TEMPDIR, "directory")
        os.mkdir(path)
        try:
            tar = tarfile.open(tmpname, self.mode)
            tarinfo = tar.gettarinfo(path)
            self.assertEqual(tarinfo.size, 0)
        finally:
            os.rmdir(path)

    def test_link_size(self):
        if hasattr(os, "link"):
            link = os.path.join(TEMPDIR, "link")
            target = os.path.join(TEMPDIR, "link_target")
            fobj = open(target, "wb")
            fobj.write("aaa")
            fobj.close()
            os.link(target, link)
            try:
                tar = tarfile.open(tmpname, self.mode)
                # Record the link target in the inodes list.
                tar.gettarinfo(target)
                tarinfo = tar.gettarinfo(link)
                self.assertEqual(tarinfo.size, 0)
            finally:
                os.remove(target)
                os.remove(link)

    def test_symlink_size(self):
        if hasattr(os, "symlink"):
            path = os.path.join(TEMPDIR, "symlink")
            os.symlink("link_target", path)
            try:
                tar = tarfile.open(tmpname, self.mode)
                tarinfo = tar.gettarinfo(path)
                self.assertEqual(tarinfo.size, 0)
            finally:
                os.remove(path)

    def test_add_self(self):
        # Test for #1257255.
        dstname = os.path.abspath(tmpname)

        tar = tarfile.open(tmpname, self.mode)
        self.assertTrue(tar.name == dstname, "archive name must be absolute")

        tar.add(dstname)
        self.assertTrue(tar.getnames() == [], "added the archive to itself")

        cwd = os.getcwd()
        os.chdir(TEMPDIR)
        tar.add(dstname)
        os.chdir(cwd)
        self.assertTrue(tar.getnames() == [], "added the archive to itself")

    def test_exclude(self):
        tempdir = os.path.join(TEMPDIR, "exclude")
        os.mkdir(tempdir)
        try:
            for name in ("foo", "bar", "baz"):
                name = os.path.join(tempdir, name)
                open(name, "wb").close()

            exclude = os.path.isfile

            tar = tarfile.open(tmpname, self.mode, encoding="iso8859-1")
            with test_support.check_warnings(("use the filter argument",
                                              DeprecationWarning)):
                tar.add(tempdir, arcname="empty_dir", exclude=exclude)
            tar.close()

            tar = tarfile.open(tmpname, "r")
            self.assertEqual(len(tar.getmembers()), 1)
            self.assertEqual(tar.getnames()[0], "empty_dir")
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
            tar.add(tempdir, arcname="empty_dir", filter=filter)
            tar.close()

            tar = tarfile.open(tmpname, "r")
            for tarinfo in tar:
                self.assertEqual(tarinfo.uid, 123)
                self.assertEqual(tarinfo.uname, "foo")
            self.assertEqual(len(tar.getmembers()), 3)
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
        tar.add(foo, arcname=path)
        tar.close()

        tar = tarfile.open(tmpname, "r")
        t = tar.next()
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
        with support.change_cwd(TEMPDIR):
            open("foo", "w").close()

            tar = tarfile.open(tmpname, self.mode)
            tar.add(".")
            tar.close()

            tar = tarfile.open(tmpname, "r")
            for t in tar:
                self.assertTrue(t.name == "." or t.name.startswith("./"))
            tar.close()

    @unittest.skipUnless(hasattr(os, 'symlink'), "needs os.symlink")
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
            tar = tarfile.open(temparchive,'w')
            tar.add(source_file, arcname=os.path.basename(source_file))
            tar.add(target_file, arcname=os.path.basename(target_file))
            tar.close()
            # Let's extract it to the location which contains the symlink
            tar = tarfile.open(temparchive,'r')
            # this should not raise OSError: [Errno 17] File exists
            try:
                tar.extractall(path=tempdir)
            except OSError:
                self.fail("extractall failed with symlinked files")
            finally:
                tar.close()
        finally:
            os.unlink(temparchive)
            shutil.rmtree(tempdir)

    @unittest.skipUnless(hasattr(os, 'symlink'), "needs os.symlink")
    def test_extractall_broken_symlinks(self):
        # Test if extractall works properly when tarfile contains broken
        # symlinks
        tempdir = os.path.join(TEMPDIR, "testsymlinks")
        temparchive = os.path.join(TEMPDIR, "testsymlinks.tar")
        os.mkdir(tempdir)
        try:
            source_file = os.path.join(tempdir,'source')
            target_file = os.path.join(tempdir,'symlink')
            with open(source_file,'w') as f:
                f.write('something\n')
            os.symlink(source_file, target_file)
            tar = tarfile.open(temparchive,'w')
            tar.add(target_file, arcname=os.path.basename(target_file))
            tar.close()
            # remove the real file
            os.unlink(source_file)
            # Let's extract it to the location which contains the symlink
            tar = tarfile.open(temparchive,'r')
            # this should not raise OSError: [Errno 17] File exists
            try:
                tar.extractall(path=tempdir)
            except OSError:
                self.fail("extractall failed with broken symlinked files")
            finally:
                tar.close()
        finally:
            os.unlink(temparchive)
            shutil.rmtree(tempdir)

    @unittest.skipUnless(hasattr(os, 'link'), "needs os.link")
    def test_extractall_hardlinks(self):
        # Test if extractall works properly when tarfile contains symlinks
        tempdir = os.path.join(TEMPDIR, "testsymlinks")
        temparchive = os.path.join(TEMPDIR, "testsymlinks.tar")
        os.mkdir(tempdir)
        try:
            source_file = os.path.join(tempdir,'source')
            target_file = os.path.join(tempdir,'symlink')
            with open(source_file,'w') as f:
                f.write('something\n')
            os.link(source_file, target_file)
            tar = tarfile.open(temparchive,'w')
            tar.add(source_file, arcname=os.path.basename(source_file))
            tar.add(target_file, arcname=os.path.basename(target_file))
            tar.close()
            # Let's extract it to the location which contains the symlink
            tar = tarfile.open(temparchive,'r')
            # this should not raise OSError: [Errno 17] File exists
            try:
                tar.extractall(path=tempdir)
            except OSError:
                self.fail("extractall failed with linked files")
            finally:
                tar.close()
        finally:
            os.unlink(temparchive)
            shutil.rmtree(tempdir)

    def test_open_nonwritable_fileobj(self):
        for exctype in IOError, EOFError, RuntimeError:
            class BadFile(StringIO.StringIO):
                first = True
                def write(self, data):
                    if self.first:
                        self.first = False
                        raise exctype

            f = BadFile()
            with self.assertRaises(exctype):
                tar = tarfile.open(tmpname, self.mode, fileobj=f,
                                   format=tarfile.PAX_FORMAT,
                                   pax_headers={'non': 'empty'})
            self.assertFalse(f.closed)

class StreamWriteTest(WriteTestBase):

    mode = "w|"

    def test_stream_padding(self):
        # Test for bug #1543303.
        tar = tarfile.open(tmpname, self.mode)
        tar.close()

        if self.mode.endswith("gz"):
            fobj = gzip.GzipFile(tmpname)
            data = fobj.read()
            fobj.close()
        elif self.mode.endswith("bz2"):
            dec = bz2.BZ2Decompressor()
            data = open(tmpname, "rb").read()
            data = dec.decompress(data)
            self.assertTrue(len(dec.unused_data) == 0,
                    "found trailing data")
        else:
            fobj = open(tmpname, "rb")
            data = fobj.read()
            fobj.close()

        self.assertTrue(data.count("\0") == tarfile.RECORDSIZE,
                         "incorrect zero padding")

    @unittest.skipIf(sys.platform == 'win32', 'not appropriate for Windows')
    @unittest.skipUnless(hasattr(os, 'umask'), 'requires os.umask')
    def test_file_mode(self):
        # Test for issue #8464: Create files with correct
        # permissions.
        if os.path.exists(tmpname):
            os.remove(tmpname)

        original_umask = os.umask(0022)
        try:
            tar = tarfile.open(tmpname, self.mode)
            tar.close()
            mode = os.stat(tmpname).st_mode & 0777
            self.assertEqual(mode, 0644, "wrong file permissions")
        finally:
            os.umask(original_umask)

    def test_issue13639(self):
        try:
            with tarfile.open(unicode(tmpname, sys.getfilesystemencoding()), self.mode):
                pass
        except UnicodeDecodeError:
            self.fail("_Stream failed to write unicode filename")


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
        tar.format = tarfile.GNU_FORMAT
        tar.addfile(tarinfo)

        v1 = self._calc_size(name, link)
        v2 = tar.offset
        self.assertTrue(v1 == v2, "GNU longname/longlink creation failed")

        tar.close()

        tar = tarfile.open(tmpname)
        member = tar.next()
        self.assertIsNotNone(member,
                "unable to read longname member")
        self.assertEqual(tarinfo.name, member.name,
                "unable to read longname member")
        self.assertEqual(tarinfo.linkname, member.linkname,
                "unable to read longname member")

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

        fobj = open(self.foo, "wb")
        fobj.write("foo")
        fobj.close()

        os.link(self.foo, self.bar)

        self.tar = tarfile.open(tmpname, "w")
        self.tar.add(self.foo)

    def tearDown(self):
        self.tar.close()
        os.remove(self.foo)
        os.remove(self.bar)

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
        tar.addfile(tarinfo)
        tar.close()

        tar = tarfile.open(tmpname)
        if link:
            l = tar.getmembers()[0].linkname
            self.assertTrue(link == l, "PAX longlink creation failed")
        else:
            n = tar.getmembers()[0].name
            self.assertTrue(name == n, "PAX longname creation failed")

    def test_pax_global_header(self):
        pax_headers = {
                u"foo": u"bar",
                u"uid": u"0",
                u"mtime": u"1.23",
                u"test": u"äöü",
                u"äöü": u"test"}

        tar = tarfile.open(tmpname, "w", format=tarfile.PAX_FORMAT,
                pax_headers=pax_headers)
        tar.addfile(tarfile.TarInfo("test"))
        tar.close()

        # Test if the global header was written correctly.
        tar = tarfile.open(tmpname, encoding="iso8859-1")
        self.assertEqual(tar.pax_headers, pax_headers)
        self.assertEqual(tar.getmembers()[0].pax_headers, pax_headers)

        # Test if all the fields are unicode.
        for key, val in tar.pax_headers.iteritems():
            self.assertTrue(type(key) is unicode)
            self.assertTrue(type(val) is unicode)
            if key in tarfile.PAX_NUMBER_FIELDS:
                try:
                    tarfile.PAX_NUMBER_FIELDS[key](val)
                except (TypeError, ValueError):
                    self.fail("unable to convert pax header field")

    def test_pax_extended_header(self):
        # The fields from the pax header have priority over the
        # TarInfo.
        pax_headers = {u"path": u"foo", u"uid": u"123"}

        tar = tarfile.open(tmpname, "w", format=tarfile.PAX_FORMAT, encoding="iso8859-1")
        t = tarfile.TarInfo()
        t.name = u"äöü"     # non-ASCII
        t.uid = 8**8        # too large
        t.pax_headers = pax_headers
        tar.addfile(t)
        tar.close()

        tar = tarfile.open(tmpname, encoding="iso8859-1")
        t = tar.getmembers()[0]
        self.assertEqual(t.pax_headers, pax_headers)
        self.assertEqual(t.name, "foo")
        self.assertEqual(t.uid, 123)


class UstarUnicodeTest(unittest.TestCase):
    # All *UnicodeTests FIXME

    format = tarfile.USTAR_FORMAT

    def test_iso8859_1_filename(self):
        self._test_unicode_filename("iso8859-1")

    def test_utf7_filename(self):
        self._test_unicode_filename("utf7")

    def test_utf8_filename(self):
        self._test_unicode_filename("utf8")

    def _test_unicode_filename(self, encoding):
        tar = tarfile.open(tmpname, "w", format=self.format, encoding=encoding, errors="strict")
        name = u"äöü"
        tar.addfile(tarfile.TarInfo(name))
        tar.close()

        tar = tarfile.open(tmpname, encoding=encoding)
        self.assertTrue(type(tar.getnames()[0]) is not unicode)
        self.assertEqual(tar.getmembers()[0].name, name.encode(encoding))
        tar.close()

    def test_unicode_filename_error(self):
        tar = tarfile.open(tmpname, "w", format=self.format, encoding="ascii", errors="strict")
        tarinfo = tarfile.TarInfo()

        tarinfo.name = "äöü"
        if self.format == tarfile.PAX_FORMAT:
            self.assertRaises(UnicodeError, tar.addfile, tarinfo)
        else:
            tar.addfile(tarinfo)

        tarinfo.name = u"äöü"
        self.assertRaises(UnicodeError, tar.addfile, tarinfo)

        tarinfo.name = "foo"
        tarinfo.uname = u"äöü"
        self.assertRaises(UnicodeError, tar.addfile, tarinfo)

    def test_unicode_argument(self):
        tar = tarfile.open(tarname, "r", encoding="iso8859-1", errors="strict")
        for t in tar:
            self.assertTrue(type(t.name) is str)
            self.assertTrue(type(t.linkname) is str)
            self.assertTrue(type(t.uname) is str)
            self.assertTrue(type(t.gname) is str)
        tar.close()

    def test_uname_unicode(self):
        for name in (u"äöü", "äöü"):
            t = tarfile.TarInfo("foo")
            t.uname = name
            t.gname = name

            fobj = StringIO.StringIO()
            tar = tarfile.open("foo.tar", mode="w", fileobj=fobj, format=self.format, encoding="iso8859-1")
            tar.addfile(t)
            tar.close()
            fobj.seek(0)

            tar = tarfile.open("foo.tar", fileobj=fobj, encoding="iso8859-1")
            t = tar.getmember("foo")
            self.assertEqual(t.uname, "äöü")
            self.assertEqual(t.gname, "äöü")


class GNUUnicodeTest(UstarUnicodeTest):

    format = tarfile.GNU_FORMAT


class PaxUnicodeTest(UstarUnicodeTest):

    format = tarfile.PAX_FORMAT

    def _create_unicode_name(self, name):
        tar = tarfile.open(tmpname, "w", format=self.format)
        t = tarfile.TarInfo()
        t.pax_headers["path"] = name
        tar.addfile(t)
        tar.close()

    def test_error_handlers(self):
        # Test if the unicode error handlers work correctly for characters
        # that cannot be expressed in a given encoding.
        self._create_unicode_name(u"äöü")

        for handler, name in (("utf-8", u"äöü".encode("utf8")),
                    ("replace", "???"), ("ignore", "")):
            tar = tarfile.open(tmpname, format=self.format, encoding="ascii",
                    errors=handler)
            self.assertEqual(tar.getnames()[0], name)

        self.assertRaises(UnicodeError, tarfile.open, tmpname,
                encoding="ascii", errors="strict")

    def test_error_handler_utf8(self):
        # Create a pathname that has one component representable using
        # iso8859-1 and the other only in iso8859-15.
        self._create_unicode_name(u"äöü/€")

        tar = tarfile.open(tmpname, format=self.format, encoding="iso8859-1",
                errors="utf-8")
        self.assertEqual(tar.getnames()[0], "äöü/" + u"€".encode("utf8"))


class AppendTest(unittest.TestCase):
    # Test append mode (cp. patch #1652681).

    def setUp(self):
        self.tarname = tmpname
        if os.path.exists(self.tarname):
            os.remove(self.tarname)

    def _add_testfile(self, fileobj=None):
        tar = tarfile.open(self.tarname, "a", fileobj=fileobj)
        tar.addfile(tarfile.TarInfo("bar"))
        tar.close()

    def _create_testtar(self, mode="w:"):
        src = tarfile.open(tarname, encoding="iso8859-1")
        t = src.getmember("ustar/regtype")
        t.name = "foo"
        f = src.extractfile(t)
        tar = tarfile.open(self.tarname, mode)
        tar.addfile(t, f)
        tar.close()

    def _test(self, names=["bar"], fileobj=None):
        tar = tarfile.open(self.tarname, fileobj=fileobj)
        self.assertEqual(tar.getnames(), names)

    def test_non_existing(self):
        self._add_testfile()
        self._test()

    def test_empty(self):
        tarfile.open(self.tarname, "w:").close()
        self._add_testfile()
        self._test()

    def test_empty_fileobj(self):
        fobj = StringIO.StringIO("\0" * 1024)
        self._add_testfile(fobj)
        fobj.seek(0)
        self._test(fileobj=fobj)

    def test_fileobj(self):
        self._create_testtar()
        data = open(self.tarname).read()
        fobj = StringIO.StringIO(data)
        self._add_testfile(fobj)
        fobj.seek(0)
        self._test(names=["foo", "bar"], fileobj=fobj)

    def test_existing(self):
        self._create_testtar()
        self._add_testfile()
        self._test(names=["foo", "bar"])

    @unittest.skipUnless(gzip, 'requires gzip')
    def test_append_gz(self):
        self._create_testtar("w:gz")
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname, "a")

    @unittest.skipUnless(bz2, 'requires bz2')
    def test_append_bz2(self):
        self._create_testtar("w:bz2")
        self.assertRaises(tarfile.ReadError, tarfile.open, tmpname, "a")

    # Append mode is supposed to fail if the tarfile to append to
    # does not end with a zero block.
    def _test_error(self, data):
        open(self.tarname, "wb").write(data)
        self.assertRaises(tarfile.ReadError, self._add_testfile)

    def test_null(self):
        self._test_error("")

    def test_incomplete(self):
        self._test_error("\0" * 13)

    def test_premature_eof(self):
        data = tarfile.TarInfo("foo").tobuf()
        self._test_error(data)

    def test_trailing_garbage(self):
        data = tarfile.TarInfo("foo").tobuf()
        self._test_error(data + "\0" * 13)

    def test_invalid(self):
        self._test_error("a" * 512)


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
        tarinfo.uid = 010000000
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.USTAR_FORMAT)

    def test_gnu_limits(self):
        tarinfo = tarfile.TarInfo("123/" * 126 + "longname")
        tarinfo.tobuf(tarfile.GNU_FORMAT)

        tarinfo = tarfile.TarInfo("longlink")
        tarinfo.linkname = "123/" * 126 + "longname"
        tarinfo.tobuf(tarfile.GNU_FORMAT)

        # uid >= 256 ** 7
        tarinfo = tarfile.TarInfo("name")
        tarinfo.uid = 04000000000000000000L
        self.assertRaises(ValueError, tarinfo.tobuf, tarfile.GNU_FORMAT)

    def test_pax_limits(self):
        tarinfo = tarfile.TarInfo("123/" * 126 + "longname")
        tarinfo.tobuf(tarfile.PAX_FORMAT)

        tarinfo = tarfile.TarInfo("longlink")
        tarinfo.linkname = "123/" * 126 + "longname"
        tarinfo.tobuf(tarfile.PAX_FORMAT)

        tarinfo = tarfile.TarInfo("name")
        tarinfo.uid = 04000000000000000000L
        tarinfo.tobuf(tarfile.PAX_FORMAT)


class MiscTest(unittest.TestCase):

    def test_read_number_fields(self):
        # Issue 24514: Test if empty number fields are converted to zero.
        self.assertEqual(tarfile.nti("\0"), 0)
        self.assertEqual(tarfile.nti("       \0"), 0)


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
        fobj = open(tmpname, "wb")
        try:
            with tarfile.open(fileobj=fobj, mode="w") as tar:
                raise Exception
        except:
            pass
        self.assertFalse(fobj.closed, "external file object was closed")
        self.assertTrue(tar.closed, "context manager failed")
        fobj.close()


class LinkEmulationTest(ReadTest):

    # Test for issue #8741 regression. On platforms that do not support
    # symbolic or hard links tarfile tries to extract these types of members as
    # the regular files they point to.
    def _test_link_extraction(self, name):
        self.tar.extract(name, TEMPDIR)
        data = open(os.path.join(TEMPDIR, name), "rb").read()
        self.assertEqual(md5sum(data), md5_regtype)

    def test_hardlink_extraction1(self):
        self._test_link_extraction("ustar/lnktype")

    def test_hardlink_extraction2(self):
        self._test_link_extraction("./ustar/linktest2/lnktype")

    def test_symlink_extraction1(self):
        self._test_link_extraction("ustar/symtype")

    def test_symlink_extraction2(self):
        self._test_link_extraction("./ustar/linktest2/symtype")


class GzipMiscReadTest(MiscReadTest):
    tarname = gzipname
    mode = "r:gz"
    taropen = tarfile.TarFile.gzopen
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
    taropen = tarfile.TarFile.bz2open
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
        class MyStringIO(StringIO.StringIO):
            hit_eof = False
            def read(self, n):
                if self.hit_eof:
                    raise AssertionError("infinite loop detected in tarfile.open()")
                self.hit_eof = self.pos == self.len
                return StringIO.StringIO.read(self, n)
            def seek(self, *args):
                self.hit_eof = False
                return StringIO.StringIO.seek(self, *args)

        data = bz2.compress(tarfile.TarInfo("foo").tobuf())
        for x in range(len(data) + 1):
            try:
                tarfile.open(fileobj=MyStringIO(data[:x]), mode=mode)
            except tarfile.ReadError:
                pass # we have no interest in ReadErrors

    def test_partial_input(self):
        self._test_partial_input("r")

    def test_partial_input_bz2(self):
        self._test_partial_input("r:bz2")


def test_main():
    os.makedirs(TEMPDIR)

    tests = [
        UstarReadTest,
        MiscReadTest,
        StreamReadTest,
        DetectReadTest,
        MemberReadTest,
        GNUReadTest,
        PaxReadTest,
        ListTest,
        WriteTest,
        StreamWriteTest,
        GNUWriteTest,
        PaxWriteTest,
        UstarUnicodeTest,
        GNUUnicodeTest,
        PaxUnicodeTest,
        AppendTest,
        LimitsTest,
        MiscTest,
        ContextManagerTest,
    ]

    if hasattr(os, "link"):
        tests.append(HardlinkTest)
    else:
        tests.append(LinkEmulationTest)

    fobj = open(tarname, "rb")
    data = fobj.read()
    fobj.close()

    if gzip:
        # Create testtar.tar.gz and add gzip-specific tests.
        tar = gzip.open(gzipname, "wb")
        tar.write(data)
        tar.close()

        tests += [
            GzipMiscReadTest,
            GzipUstarReadTest,
            GzipStreamReadTest,
            GzipListTest,
            GzipWriteTest,
            GzipStreamWriteTest,
        ]

    if bz2:
        # Create testtar.tar.bz2 and add bz2-specific tests.
        tar = bz2.BZ2File(bz2name, "wb")
        tar.write(data)
        tar.close()

        tests += [
            Bz2MiscReadTest,
            Bz2UstarReadTest,
            Bz2StreamReadTest,
            Bz2ListTest,
            Bz2WriteTest,
            Bz2StreamWriteTest,
            Bz2PartialReadTest,
        ]

    try:
        test_support.run_unittest(*tests)
    finally:
        if os.path.exists(TEMPDIR):
            shutil.rmtree(TEMPDIR)

if __name__ == "__main__":
    test_main()
