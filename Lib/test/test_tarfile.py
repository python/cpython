import sys
import os
import shutil
import tempfile

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
membercount = 10

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
            lines1 = file(os.path.join(dirname(), filename), "rU").readlines()
            lines2 = self.tar.extractfile(filename).readlines()
            self.assert_(lines1 == lines2,
                         "_FileObject.readline() does not work correctly")

    def test_seek(self):
        """Test seek() method of _FileObject, incl. random reading.
        """
        if self.sep != "|":
            filename = "0-REGTYPE"
            self.tar.extract(filename, dirname())
            data = file(os.path.join(dirname(), filename), "rb").read()

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
            fobj.close()

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

        stream.close()

class WriteTest(BaseTest):
    mode = 'w'

    def setUp(self):
        mode = self.mode + self.sep + self.comp
        self.src = tarfile.open(tarname(self.comp), 'r')
        self.dst = tarfile.open(tmpname(), mode)

    def tearDown(self):
        self.src.close()
        self.dst.close()

    def test_posix(self):
        self.dst.posix = 1
        self._test()

    def test_nonposix(self):
        self.dst.posix = 0
        self._test()

    def _test(self):
        for tarinfo in self.src:
            if not tarinfo.isreg():
                continue
            f = self.src.extractfile(tarinfo)
            if self.dst.posix and len(tarinfo.name) > tarfile.LENGTH_NAME:
                self.assertRaises(ValueError, self.dst.addfile,
                                 tarinfo, f)
            else:
                self.dst.addfile(tarinfo, f)

class WriteStreamTest(WriteTest):
    sep = '|'

# Gzip TestCases
class ReadTestGzip(ReadTest):
    comp = "gz"
class ReadStreamTestGzip(ReadStreamTest):
    comp = "gz"
class WriteTestGzip(WriteTest):
    comp = "gz"
class WriteStreamTestGzip(WriteStreamTest):
    comp = "gz"

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

# If importing gzip failed, discard the Gzip TestCases.
if not gzip:
    del ReadTestGzip
    del ReadStreamTestGzip
    del WriteTestGzip
    del WriteStreamTestGzip

def test_main():
    if gzip:
        # create testtar.tar.gz
        gzip.open(tarname("gz"), "wb").write(file(tarname(), "rb").read())
    if bz2:
        # create testtar.tar.bz2
        bz2.BZ2File(tarname("bz2"), "wb").write(file(tarname(), "rb").read())

    tests = [
        ReadTest,
        ReadStreamTest,
        WriteTest,
        WriteStreamTest
    ]

    if gzip:
        tests.extend([
            ReadTestGzip, ReadStreamTestGzip,
            WriteTestGzip, WriteStreamTestGzip
        ])

    if bz2:
        tests.extend([
            ReadTestBzip2, ReadStreamTestBzip2,
            WriteTestBzip2, WriteStreamTestBzip2
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
