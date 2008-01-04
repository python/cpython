import sys
import os
import shutil
import tempfile
import StringIO

import unittest
import tarfile

from test import test_support

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

def path(path):
    return test_support.findfile(path)

testtar = path("testtar.tar")
tempdir = os.path.join(tempfile.gettempdir(), "testtar" + os.extsep + "dir")
tempname = test_support.TESTFN
membercount = 12

def tarname(comp=""):
    if not comp:
        return testtar
    return os.path.join(tempdir, "%s%s%s" % (testtar, os.extsep, comp))

def dirname():
    if not os.path.exists(tempdir):
        os.mkdir(tempdir)
    return tempdir

def tmpname():
    return tempname


class BaseTest(unittest.TestCase):
    comp = ''
    mode = 'r'
    sep = ':'

    def setUp(self):
        mode = self.mode + self.sep + self.comp
        self.tar = tarfile.open(tarname(self.comp), mode)

    def tearDown(self):
        self.tar.close()

class ReadTest(BaseTest):

    def test(self):
        """Test member extraction.
        """
        members = 0
        for tarinfo in self.tar:
            members += 1
            if not tarinfo.isreg():
                continue
            f = self.tar.extractfile(tarinfo)
            self.assert_(len(f.read()) == tarinfo.size,
                         "size read does not match expected size")
            f.close()

        self.assert_(members == membercount,
                     "could not find all members")

    def test_sparse(self):
        """Test sparse member extraction.
        """
        if self.sep != "|":
            f1 = self.tar.extractfile("S-SPARSE")
            f2 = self.tar.extractfile("S-SPARSE-WITH-NULLS")
            self.assert_(f1.read() == f2.read(),
                         "_FileObject failed on sparse file member")

    def test_readlines(self):
        """Test readlines() method of _FileObject.
        """
        if self.sep != "|":
            filename = "0-REGTYPE-TEXT"
            self.tar.extract(filename, dirname())
            f = open(os.path.join(dirname(), filename), "rU")
            lines1 = f.readlines()
            f.close()
            lines2 = self.tar.extractfile(filename).readlines()
            self.assert_(lines1 == lines2,
                         "_FileObject.readline() does not work correctly")

    def test_iter(self):
        # Test iteration over ExFileObject.
        if self.sep != "|":
            filename = "0-REGTYPE-TEXT"
            self.tar.extract(filename, dirname())
            f = open(os.path.join(dirname(), filename), "rU")
            lines1 = f.readlines()
            f.close()
            lines2 = [line for line in self.tar.extractfile(filename)]
            self.assert_(lines1 == lines2,
                         "ExFileObject iteration does not work correctly")

    def test_seek(self):
        """Test seek() method of _FileObject, incl. random reading.
        """
        if self.sep != "|":
            filename = "0-REGTYPE-TEXT"
            self.tar.extract(filename, dirname())
            f = open(os.path.join(dirname(), filename), "rb")
            data = f.read()
            f.close()

            tarinfo = self.tar.getmember(filename)
            fobj = self.tar.extractfile(tarinfo)

            text = fobj.read()
            fobj.seek(0)
            self.assert_(0 == fobj.tell(),
                         "seek() to file's start failed")
            fobj.seek(2048, 0)
            self.assert_(2048 == fobj.tell(),
                         "seek() to absolute position failed")
            fobj.seek(-1024, 1)
            self.assert_(1024 == fobj.tell(),
                         "seek() to negative relative position failed")
            fobj.seek(1024, 1)
            self.assert_(2048 == fobj.tell(),
                         "seek() to positive relative position failed")
            s = fobj.read(10)
            self.assert_(s == data[2048:2058],
                         "read() after seek failed")
            fobj.seek(0, 2)
            self.assert_(tarinfo.size == fobj.tell(),
                         "seek() to file's end failed")
            self.assert_(fobj.read() == "",
                         "read() at file's end did not return empty string")
            fobj.seek(-tarinfo.size, 2)
            self.assert_(0 == fobj.tell(),
                         "relative seek() to file's start failed")
            fobj.seek(512)
            s1 = fobj.readlines()
            fobj.seek(512)
            s2 = fobj.readlines()
            self.assert_(s1 == s2,
                         "readlines() after seek failed")
            fobj.seek(0)
            self.assert_(len(fobj.readline()) == fobj.tell(),
                         "tell() after readline() failed")
            fobj.seek(512)
            self.assert_(len(fobj.readline()) + 512 == fobj.tell(),
                         "tell() after seek() and readline() failed")
            fobj.seek(0)
            line = fobj.readline()
            self.assert_(fobj.read() == data[len(line):],
                         "read() after readline() failed")
            fobj.close()

    def test_old_dirtype(self):
        """Test old style dirtype member (bug #1336623).
        """
        # Old tars create directory members using a REGTYPE
        # header with a "/" appended to the filename field.

        # Create an old tar style directory entry.
        filename = tmpname()
        tarinfo = tarfile.TarInfo("directory/")
        tarinfo.type = tarfile.REGTYPE

        fobj = open(filename, "w")
        fobj.write(tarinfo.tobuf())
        fobj.close()

        try:
            # Test if it is still a directory entry when
            # read back.
            tar = tarfile.open(filename)
            tarinfo = tar.getmembers()[0]
            tar.close()

            self.assert_(tarinfo.type == tarfile.DIRTYPE)
            self.assert_(tarinfo.name.endswith("/"))
        finally:
            try:
                os.unlink(filename)
            except:
                pass

    def test_dirtype(self):
        for tarinfo in self.tar:
            if tarinfo.isdir():
                self.assert_(tarinfo.name.endswith("/"))
                self.assert_(not tarinfo.name[:-1].endswith("/"))

    def test_extractall(self):
        # Test if extractall() correctly restores directory permissions
        # and times (see issue1735).
        if sys.platform == "win32":
            # Win32 has no support for utime() on directories or
            # fine grained permissions.
            return

        fobj = StringIO.StringIO()
        tar = tarfile.open(fileobj=fobj, mode="w:")
        for name in ("foo", "foo/bar"):
            tarinfo = tarfile.TarInfo(name)
            tarinfo.type = tarfile.DIRTYPE
            tarinfo.mtime = 07606136617
            tarinfo.mode = 0755
            tar.addfile(tarinfo)
        tar.close()
        fobj.seek(0)

        TEMPDIR = os.path.join(dirname(), "extract-test")
        tar = tarfile.open(fileobj=fobj)
        tar.extractall(TEMPDIR)
        for tarinfo in tar.getmembers():
            path = os.path.join(TEMPDIR, tarinfo.name)
            self.assertEqual(tarinfo.mode, os.stat(path).st_mode & 0777)
            self.assertEqual(tarinfo.mtime, os.path.getmtime(path))
        tar.close()


class ReadStreamTest(ReadTest):
    sep = "|"

    def test(self):
        """Test member extraction, and for StreamError when
           seeking backwards.
        """
        ReadTest.test(self)
        tarinfo = self.tar.getmembers()[0]
        f = self.tar.extractfile(tarinfo)
        self.assertRaises(tarfile.StreamError, f.read)

    def test_stream(self):
        """Compare the normal tar and the stream tar.
        """
        stream = self.tar
        tar = tarfile.open(tarname(), 'r')

        while 1:
            t1 = tar.next()
            t2 = stream.next()
            if t1 is None:
                break
            self.assert_(t2 is not None, "stream.next() failed.")

            if t2.islnk() or t2.issym():
                self.assertRaises(tarfile.StreamError, stream.extractfile, t2)
                continue
            v1 = tar.extractfile(t1)
            v2 = stream.extractfile(t2)
            if v1 is None:
                continue
            self.assert_(v2 is not None, "stream.extractfile() failed")
            self.assert_(v1.read() == v2.read(), "stream extraction failed")

        tar.close()
        stream.close()

class ReadDetectTest(ReadTest):

    def setUp(self):
        self.tar = tarfile.open(tarname(self.comp), self.mode)

class ReadDetectFileobjTest(ReadTest):

    def setUp(self):
        name = tarname(self.comp)
        self.tar = tarfile.open(name, mode=self.mode,
                                fileobj=open(name, "rb"))

class ReadAsteriskTest(ReadTest):

    def setUp(self):
        mode = self.mode + self.sep + "*"
        self.tar = tarfile.open(tarname(self.comp), mode)

class ReadStreamAsteriskTest(ReadStreamTest):

    def setUp(self):
        mode = self.mode + self.sep + "*"
        self.tar = tarfile.open(tarname(self.comp), mode)

class ReadFileobjTest(BaseTest):

    def test_fileobj_with_offset(self):
        # Skip the first member and store values from the second member
        # of the testtar.
        self.tar.next()
        t = self.tar.next()
        name = t.name
        offset = t.offset
        data = self.tar.extractfile(t).read()
        self.tar.close()

        # Open the testtar and seek to the offset of the second member.
        if self.comp == "gz":
            _open = gzip.GzipFile
        elif self.comp == "bz2":
            _open = bz2.BZ2File
        else:
            _open = open
        fobj = _open(tarname(self.comp), "rb")
        fobj.seek(offset)

        # Test if the tarfile starts with the second member.
        self.tar = tarfile.open(tarname(self.comp), "r:", fileobj=fobj)
        t = self.tar.next()
        self.assertEqual(t.name, name)
        # Read to the end of fileobj and test if seeking back to the
        # beginning works.
        self.tar.getmembers()
        self.assertEqual(self.tar.extractfile(t).read(), data,
                "seek back did not work")

class WriteTest(BaseTest):
    mode = 'w'

    def setUp(self):
        mode = self.mode + self.sep + self.comp
        self.src = tarfile.open(tarname(self.comp), 'r')
        self.dstname = tmpname()
        self.dst = tarfile.open(self.dstname, mode)

    def tearDown(self):
        self.src.close()
        self.dst.close()

    def test_posix(self):
        self.dst.posix = 1
        self._test()

    def test_nonposix(self):
        self.dst.posix = 0
        self._test()

    def test_small(self):
        self.dst.add(os.path.join(os.path.dirname(__file__),"cfgparser.1"))
        self.dst.close()
        self.assertNotEqual(os.stat(self.dstname).st_size, 0)

    def _test(self):
        for tarinfo in self.src:
            if not tarinfo.isreg():
                continue
            f = self.src.extractfile(tarinfo)
            if self.dst.posix and len(tarinfo.name) > tarfile.LENGTH_NAME and "/" not in tarinfo.name:
                self.assertRaises(ValueError, self.dst.addfile,
                                 tarinfo, f)
            else:
                self.dst.addfile(tarinfo, f)

    def test_add_self(self):
        dstname = os.path.abspath(self.dstname)

        self.assertEqual(self.dst.name, dstname, "archive name must be absolute")

        self.dst.add(dstname)
        self.assertEqual(self.dst.getnames(), [], "added the archive to itself")

        cwd = os.getcwd()
        os.chdir(dirname())
        self.dst.add(dstname)
        os.chdir(cwd)
        self.assertEqual(self.dst.getnames(), [], "added the archive to itself")


class Write100Test(BaseTest):
    # The name field in a tar header stores strings of at most 100 chars.
    # If a string is shorter than 100 chars it has to be padded with '\0',
    # which implies that a string of exactly 100 chars is stored without
    # a trailing '\0'.

    def setUp(self):
        self.name = "01234567890123456789012345678901234567890123456789"
        self.name += "01234567890123456789012345678901234567890123456789"

        self.tar = tarfile.open(tmpname(), "w")
        t = tarfile.TarInfo(self.name)
        self.tar.addfile(t)
        self.tar.close()

        self.tar = tarfile.open(tmpname())

    def tearDown(self):
        self.tar.close()

    def test(self):
        self.assertEqual(self.tar.getnames()[0], self.name,
                "failed to store 100 char filename")


class WriteSize0Test(BaseTest):
    mode = 'w'

    def setUp(self):
        self.tmpdir = dirname()
        self.dstname = tmpname()
        self.dst = tarfile.open(self.dstname, "w")

    def tearDown(self):
        self.dst.close()

    def test_file(self):
        path = os.path.join(self.tmpdir, "file")
        f = open(path, "w")
        f.close()
        tarinfo = self.dst.gettarinfo(path)
        self.assertEqual(tarinfo.size, 0)
        f = open(path, "w")
        f.write("aaa")
        f.close()
        tarinfo = self.dst.gettarinfo(path)
        self.assertEqual(tarinfo.size, 3)

    def test_directory(self):
        path = os.path.join(self.tmpdir, "directory")
        if os.path.exists(path):
            # This shouldn't be necessary, but is <wink> if a previous
            # run was killed in mid-stream.
            shutil.rmtree(path)
        os.mkdir(path)
        tarinfo = self.dst.gettarinfo(path)
        self.assertEqual(tarinfo.size, 0)

    def test_symlink(self):
        if hasattr(os, "symlink"):
            path = os.path.join(self.tmpdir, "symlink")
            os.symlink("link_target", path)
            tarinfo = self.dst.gettarinfo(path)
            self.assertEqual(tarinfo.size, 0)


class WriteStreamTest(WriteTest):
    sep = '|'

    def test_padding(self):
        self.dst.close()

        if self.comp == "gz":
            f = gzip.GzipFile(self.dstname)
            s = f.read()
            f.close()
        elif self.comp == "bz2":
            f = bz2.BZ2Decompressor()
            s = file(self.dstname).read()
            s = f.decompress(s)
            self.assertEqual(len(f.unused_data), 0, "trailing data")
        else:
            f = file(self.dstname)
            s = f.read()
            f.close()

        self.assertEqual(s.count("\0"), tarfile.RECORDSIZE,
                         "incorrect zero padding")


class WriteGNULongTest(unittest.TestCase):
    """This testcase checks for correct creation of GNU Longname
       and Longlink extensions.

       It creates a tarfile and adds empty members with either
       long names, long linknames or both and compares the size
       of the tarfile with the expected size.

       It checks for SF bug #812325 in TarFile._create_gnulong().

       While I was writing this testcase, I noticed a second bug
       in the same method:
       Long{names,links} weren't null-terminated which lead to
       bad tarfiles when their length was a multiple of 512. This
       is tested as well.
    """

    def _length(self, s):
        blocks, remainder = divmod(len(s) + 1, 512)
        if remainder:
            blocks += 1
        return blocks * 512

    def _calc_size(self, name, link=None):
        # initial tar header
        count = 512

        if len(name) > tarfile.LENGTH_NAME:
            # gnu longname extended header + longname
            count += 512
            count += self._length(name)

        if link is not None and len(link) > tarfile.LENGTH_LINK:
            # gnu longlink extended header + longlink
            count += 512
            count += self._length(link)

        return count

    def _test(self, name, link=None):
        tarinfo = tarfile.TarInfo(name)
        if link:
            tarinfo.linkname = link
            tarinfo.type = tarfile.LNKTYPE

        tar = tarfile.open(tmpname(), "w")
        tar.posix = False
        tar.addfile(tarinfo)

        v1 = self._calc_size(name, link)
        v2 = tar.offset
        self.assertEqual(v1, v2, "GNU longname/longlink creation failed")

        tar.close()

        tar = tarfile.open(tmpname())
        member = tar.next()
        self.failIf(member is None, "unable to read longname member")
        self.assert_(tarinfo.name == member.name and \
                     tarinfo.linkname == member.linkname, \
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

class ReadGNULongTest(unittest.TestCase):

    def setUp(self):
        self.tar = tarfile.open(tarname())

    def tearDown(self):
        self.tar.close()

    def test_1471427(self):
        """Test reading of longname (bug #1471427).
        """
        name = "test/" * 20 + "0-REGTYPE"
        try:
            tarinfo = self.tar.getmember(name)
        except KeyError:
            tarinfo = None
        self.assert_(tarinfo is not None, "longname not found")
        self.assert_(tarinfo.type != tarfile.DIRTYPE, "read longname as dirtype")

    def test_read_name(self):
        name = ("0-LONGNAME-" * 10)[:101]
        try:
            tarinfo = self.tar.getmember(name)
        except KeyError:
            tarinfo = None
        self.assert_(tarinfo is not None, "longname not found")

    def test_read_link(self):
        link = ("1-LONGLINK-" * 10)[:101]
        name = ("0-LONGNAME-" * 10)[:101]
        try:
            tarinfo = self.tar.getmember(link)
        except KeyError:
            tarinfo = None
        self.assert_(tarinfo is not None, "longlink not found")
        self.assert_(tarinfo.linkname == name, "linkname wrong")

    def test_truncated_longname(self):
        f = open(tarname())
        fobj = StringIO.StringIO(f.read(1024))
        f.close()
        tar = tarfile.open(name="foo.tar", fileobj=fobj)
        self.assert_(len(tar.getmembers()) == 0, "")
        tar.close()


class ExtractHardlinkTest(BaseTest):

    def test_hardlink(self):
        """Test hardlink extraction (bug #857297)
        """
        # Prevent errors from being caught
        self.tar.errorlevel = 1

        self.tar.extract("0-REGTYPE", dirname())
        try:
            # Extract 1-LNKTYPE which is a hardlink to 0-REGTYPE
            self.tar.extract("1-LNKTYPE", dirname())
        except EnvironmentError, e:
            import errno
            if e.errno == errno.ENOENT:
                self.fail("hardlink not extracted properly")

class CreateHardlinkTest(BaseTest):
    """Test the creation of LNKTYPE (hardlink) members in an archive.
       In this respect tarfile.py mimics the behaviour of GNU tar: If
       a file has a st_nlink > 1, it will be added a REGTYPE member
       only the first time.
    """

    def setUp(self):
        self.tar = tarfile.open(tmpname(), "w")

        self.foo = os.path.join(dirname(), "foo")
        self.bar = os.path.join(dirname(), "bar")

        if os.path.exists(self.foo):
            os.remove(self.foo)
        if os.path.exists(self.bar):
            os.remove(self.bar)

        f = open(self.foo, "w")
        f.write("foo")
        f.close()
        self.tar.add(self.foo)

    def test_add_twice(self):
        # If st_nlink == 1 then the same file will be added as
        # REGTYPE every time.
        tarinfo = self.tar.gettarinfo(self.foo)
        self.assertEqual(tarinfo.type, tarfile.REGTYPE,
                "add file as regular failed")

    def test_add_hardlink(self):
        # If st_nlink > 1 then the same file will be added as
        # LNKTYPE.
        os.link(self.foo, self.bar)
        tarinfo = self.tar.gettarinfo(self.foo)
        self.assertEqual(tarinfo.type, tarfile.LNKTYPE,
                "add file as hardlink failed")

        tarinfo = self.tar.gettarinfo(self.bar)
        self.assertEqual(tarinfo.type, tarfile.LNKTYPE,
                "add file as hardlink failed")

    def test_dereference_hardlink(self):
        self.tar.dereference = True
        os.link(self.foo, self.bar)
        tarinfo = self.tar.gettarinfo(self.bar)
        self.assertEqual(tarinfo.type, tarfile.REGTYPE,
                "dereferencing hardlink failed")


# Gzip TestCases
class ReadTestGzip(ReadTest):
    comp = "gz"
class ReadStreamTestGzip(ReadStreamTest):
    comp = "gz"
class WriteTestGzip(WriteTest):
    comp = "gz"
class WriteStreamTestGzip(WriteStreamTest):
    comp = "gz"
class ReadDetectTestGzip(ReadDetectTest):
    comp = "gz"
class ReadDetectFileobjTestGzip(ReadDetectFileobjTest):
    comp = "gz"
class ReadAsteriskTestGzip(ReadAsteriskTest):
    comp = "gz"
class ReadStreamAsteriskTestGzip(ReadStreamAsteriskTest):
    comp = "gz"
class ReadFileobjTestGzip(ReadFileobjTest):
    comp = "gz"

# Filemode test cases

class FileModeTest(unittest.TestCase):
    def test_modes(self):
        self.assertEqual(tarfile.filemode(0755), '-rwxr-xr-x')
        self.assertEqual(tarfile.filemode(07111), '---s--s--t')

class OpenFileobjTest(BaseTest):

    def test_opener(self):
        # Test for SF bug #1496501.
        fobj = StringIO.StringIO("foo\n")
        try:
            tarfile.open("", mode="r", fileobj=fobj)
        except tarfile.ReadError:
            self.assertEqual(fobj.tell(), 0, "fileobj's position has moved")

    def test_no_name_argument(self):
        fobj = open(testtar, "rb")
        tar = tarfile.open(fileobj=fobj, mode="r")
        self.assertEqual(tar.name, os.path.abspath(fobj.name))

    def test_no_name_attribute(self):
        data = open(testtar, "rb").read()
        fobj = StringIO.StringIO(data)
        self.assertRaises(AttributeError, getattr, fobj, "name")
        tar = tarfile.open(fileobj=fobj, mode="r")
        self.assertEqual(tar.name, None)

    def test_empty_name_attribute(self):
        data = open(testtar, "rb").read()
        fobj = StringIO.StringIO(data)
        fobj.name = ""
        tar = tarfile.open(fileobj=fobj, mode="r")
        self.assertEqual(tar.name, None)


if bz2:
    # Bzip2 TestCases
    class ReadTestBzip2(ReadTestGzip):
        comp = "bz2"
    class ReadStreamTestBzip2(ReadStreamTestGzip):
        comp = "bz2"
    class WriteTestBzip2(WriteTest):
        comp = "bz2"
    class WriteStreamTestBzip2(WriteStreamTestGzip):
        comp = "bz2"
    class ReadDetectTestBzip2(ReadDetectTest):
        comp = "bz2"
    class ReadDetectFileobjTestBzip2(ReadDetectFileobjTest):
        comp = "bz2"
    class ReadAsteriskTestBzip2(ReadAsteriskTest):
        comp = "bz2"
    class ReadStreamAsteriskTestBzip2(ReadStreamAsteriskTest):
        comp = "bz2"
    class ReadFileobjTestBzip2(ReadFileobjTest):
        comp = "bz2"

# If importing gzip failed, discard the Gzip TestCases.
if not gzip:
    del ReadTestGzip
    del ReadStreamTestGzip
    del WriteTestGzip
    del WriteStreamTestGzip

def test_main():
    # Create archive.
    f = open(tarname(), "rb")
    fguts = f.read()
    f.close()
    if gzip:
        # create testtar.tar.gz
        tar = gzip.open(tarname("gz"), "wb")
        tar.write(fguts)
        tar.close()
    if bz2:
        # create testtar.tar.bz2
        tar = bz2.BZ2File(tarname("bz2"), "wb")
        tar.write(fguts)
        tar.close()

    tests = [
        FileModeTest,
        OpenFileobjTest,
        ReadTest,
        ReadStreamTest,
        ReadDetectTest,
        ReadDetectFileobjTest,
        ReadAsteriskTest,
        ReadStreamAsteriskTest,
        ReadFileobjTest,
        WriteTest,
        Write100Test,
        WriteSize0Test,
        WriteStreamTest,
        WriteGNULongTest,
        ReadGNULongTest,
    ]

    if hasattr(os, "link"):
        tests.append(ExtractHardlinkTest)
        tests.append(CreateHardlinkTest)

    if gzip:
        tests.extend([
            ReadTestGzip, ReadStreamTestGzip,
            WriteTestGzip, WriteStreamTestGzip,
            ReadDetectTestGzip, ReadDetectFileobjTestGzip,
            ReadAsteriskTestGzip, ReadStreamAsteriskTestGzip,
            ReadFileobjTestGzip
        ])

    if bz2:
        tests.extend([
            ReadTestBzip2, ReadStreamTestBzip2,
            WriteTestBzip2, WriteStreamTestBzip2,
            ReadDetectTestBzip2, ReadDetectFileobjTestBzip2,
            ReadAsteriskTestBzip2, ReadStreamAsteriskTestBzip2,
            ReadFileobjTestBzip2
        ])
    try:
        test_support.run_unittest(*tests)
    finally:
        if gzip:
            os.remove(tarname("gz"))
        if bz2:
            os.remove(tarname("bz2"))
        if os.path.exists(dirname()):
            shutil.rmtree(dirname())
        if os.path.exists(tmpname()):
            os.remove(tmpname())

if __name__ == "__main__":
    test_main()
