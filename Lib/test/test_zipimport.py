import sys
import os
import marshal
import imp
import struct
import time
import unittest

import zlib # implied prerequisite
from zipfile import ZipFile, ZipInfo, ZIP_STORED, ZIP_DEFLATED
from test import test_support
from test.test_importhooks import ImportHooksBaseTestCase, test_src, test_co

import zipimport


# so we only run testAFakeZlib once if this test is run repeatedly
# which happens when we look for ref leaks
test_imported = False


def make_pyc(co, mtime):
    data = marshal.dumps(co)
    if type(mtime) is type(0.0):
        # Mac mtimes need a bit of special casing
        if mtime < 0x7fffffff:
            mtime = int(mtime)
        else:
            mtime = int(-0x100000000L + long(mtime))
    pyc = imp.get_magic() + struct.pack("<i", int(mtime)) + data
    return pyc

NOW = time.time()
test_pyc = make_pyc(test_co, NOW)


if __debug__:
    pyc_ext = ".pyc"
else:
    pyc_ext = ".pyo"


TESTMOD = "ziptestmodule"
TESTPACK = "ziptestpackage"
TESTPACK2 = "ziptestpackage2"
TEMP_ZIP = os.path.abspath("junk95142" + os.extsep + "zip")

class UncompressedZipImportTestCase(ImportHooksBaseTestCase):

    compression = ZIP_STORED

    def setUp(self):
        # We're reusing the zip archive path, so we must clear the
        # cached directory info.
        zipimport._zip_directory_cache.clear()
        ImportHooksBaseTestCase.setUp(self)

    def doTest(self, expected_ext, files, *modules, **kw):
        z = ZipFile(TEMP_ZIP, "w")
        try:
            for name, (mtime, data) in files.items():
                zinfo = ZipInfo(name, time.localtime(mtime))
                zinfo.compress_type = self.compression
                z.writestr(zinfo, data)
            z.close()

            stuff = kw.get("stuff", None)
            if stuff is not None:
                # Prepend 'stuff' to the start of the zipfile
                f = open(TEMP_ZIP, "rb")
                data = f.read()
                f.close()

                f = open(TEMP_ZIP, "wb")
                f.write(stuff)
                f.write(data)
                f.close()

            sys.path.insert(0, TEMP_ZIP)

            mod = __import__(".".join(modules), globals(), locals(),
                             ["__dummy__"])
            if expected_ext:
                file = mod.get_file()
                self.assertEquals(file, os.path.join(TEMP_ZIP,
                                  *modules) + expected_ext)
        finally:
            z.close()
            os.remove(TEMP_ZIP)

    def testAFakeZlib(self):
        #
        # This could cause a stack overflow before: importing zlib.py
        # from a compressed archive would cause zlib to be imported
        # which would find zlib.py in the archive, which would... etc.
        #
        # This test *must* be executed first: it must be the first one
        # to trigger zipimport to import zlib (zipimport caches the
        # zlib.decompress function object, after which the problem being
        # tested here wouldn't be a problem anymore...
        # (Hence the 'A' in the test method name: to make it the first
        # item in a list sorted by name, like unittest.makeSuite() does.)
        #
        # This test fails on platforms on which the zlib module is
        # statically linked, but the problem it tests for can't
        # occur in that case (builtin modules are always found first),
        # so we'll simply skip it then. Bug #765456.
        #
        if "zlib" in sys.builtin_module_names:
            return
        if "zlib" in sys.modules:
            del sys.modules["zlib"]
        files = {"zlib.py": (NOW, test_src)}
        try:
            self.doTest(".py", files, "zlib")
        except ImportError:
            if self.compression != ZIP_DEFLATED:
                self.fail("expected test to not raise ImportError")
        else:
            if self.compression != ZIP_STORED:
                self.fail("expected test to raise ImportError")

    def testPy(self):
        files = {TESTMOD + ".py": (NOW, test_src)}
        self.doTest(".py", files, TESTMOD)

    def testPyc(self):
        files = {TESTMOD + pyc_ext: (NOW, test_pyc)}
        self.doTest(pyc_ext, files, TESTMOD)

    def testBoth(self):
        files = {TESTMOD + ".py": (NOW, test_src),
                 TESTMOD + pyc_ext: (NOW, test_pyc)}
        self.doTest(pyc_ext, files, TESTMOD)

    def testEmptyPy(self):
        files = {TESTMOD + ".py": (NOW, "")}
        self.doTest(None, files, TESTMOD)

    def testBadMagic(self):
        # make pyc magic word invalid, forcing loading from .py
        m0 = ord(test_pyc[0])
        m0 ^= 0x04  # flip an arbitrary bit
        badmagic_pyc = chr(m0) + test_pyc[1:]
        files = {TESTMOD + ".py": (NOW, test_src),
                 TESTMOD + pyc_ext: (NOW, badmagic_pyc)}
        self.doTest(".py", files, TESTMOD)

    def testBadMagic2(self):
        # make pyc magic word invalid, causing an ImportError
        m0 = ord(test_pyc[0])
        m0 ^= 0x04  # flip an arbitrary bit
        badmagic_pyc = chr(m0) + test_pyc[1:]
        files = {TESTMOD + pyc_ext: (NOW, badmagic_pyc)}
        try:
            self.doTest(".py", files, TESTMOD)
        except ImportError:
            pass
        else:
            self.fail("expected ImportError; import from bad pyc")

    def testBadMTime(self):
        t3 = ord(test_pyc[7])
        t3 ^= 0x02  # flip the second bit -- not the first as that one
                    # isn't stored in the .py's mtime in the zip archive.
        badtime_pyc = test_pyc[:7] + chr(t3) + test_pyc[8:]
        files = {TESTMOD + ".py": (NOW, test_src),
                 TESTMOD + pyc_ext: (NOW, badtime_pyc)}
        self.doTest(".py", files, TESTMOD)

    def testPackage(self):
        packdir = TESTPACK + os.sep
        files = {packdir + "__init__" + pyc_ext: (NOW, test_pyc),
                 packdir + TESTMOD + pyc_ext: (NOW, test_pyc)}
        self.doTest(pyc_ext, files, TESTPACK, TESTMOD)

    def testDeepPackage(self):
        packdir = TESTPACK + os.sep
        packdir2 = packdir + TESTPACK2 + os.sep
        files = {packdir + "__init__" + pyc_ext: (NOW, test_pyc),
                 packdir2 + "__init__" + pyc_ext: (NOW, test_pyc),
                 packdir2 + TESTMOD + pyc_ext: (NOW, test_pyc)}
        self.doTest(pyc_ext, files, TESTPACK, TESTPACK2, TESTMOD)

    def testZipImporterMethods(self):
        packdir = TESTPACK + os.sep
        packdir2 = packdir + TESTPACK2 + os.sep
        files = {packdir + "__init__" + pyc_ext: (NOW, test_pyc),
                 packdir2 + "__init__" + pyc_ext: (NOW, test_pyc),
                 packdir2 + TESTMOD + pyc_ext: (NOW, test_pyc)}

        z = ZipFile(TEMP_ZIP, "w")
        try:
            for name, (mtime, data) in files.items():
                zinfo = ZipInfo(name, time.localtime(mtime))
                zinfo.compress_type = self.compression
                z.writestr(zinfo, data)
            z.close()

            zi = zipimport.zipimporter(TEMP_ZIP)
            self.assertEquals(zi.is_package(TESTPACK), True)
            zi.load_module(TESTPACK)
            
            self.assertEquals(zi.is_package(packdir + '__init__'), False)
            self.assertEquals(zi.is_package(packdir + TESTPACK2), True)
            self.assertEquals(zi.is_package(packdir2 + TESTMOD), False)

            mod_name = packdir2 + TESTMOD
            mod = __import__(mod_name.replace('/', '.'))
            self.assertEquals(zi.get_source(TESTPACK), None)
            self.assertEquals(zi.get_source(mod_name), None)
        finally:
            z.close()
            os.remove(TEMP_ZIP)

    def testGetData(self):
        z = ZipFile(TEMP_ZIP, "w")
        z.compression = self.compression
        try:
            name = "testdata.dat"
            data = "".join([chr(x) for x in range(256)]) * 500
            z.writestr(name, data)
            z.close()
            zi = zipimport.zipimporter(TEMP_ZIP)
            self.assertEquals(data, zi.get_data(name))
            self.assert_('zipimporter object' in repr(zi))
        finally:
            z.close()
            os.remove(TEMP_ZIP)

    def testImporterAttr(self):
        src = """if 1:  # indent hack
        def get_file():
            return __file__
        if __loader__.get_data("some.data") != "some data":
            raise AssertionError, "bad data"\n"""
        pyc = make_pyc(compile(src, "<???>", "exec"), NOW)
        files = {TESTMOD + pyc_ext: (NOW, pyc),
                 "some.data": (NOW, "some data")}
        self.doTest(pyc_ext, files, TESTMOD)

    def testImport_WithStuff(self):
        # try importing from a zipfile which contains additional
        # stuff at the beginning of the file
        files = {TESTMOD + ".py": (NOW, test_src)}
        self.doTest(".py", files, TESTMOD,
                    stuff="Some Stuff"*31)

class CompressedZipImportTestCase(UncompressedZipImportTestCase):
    compression = ZIP_DEFLATED


class BadFileZipImportTestCase(unittest.TestCase):
    def assertZipFailure(self, filename):
        self.assertRaises(zipimport.ZipImportError,
                          zipimport.zipimporter, filename)

    def testNoFile(self):
        self.assertZipFailure('AdfjdkFJKDFJjdklfjs')

    def testEmptyFilename(self):
        self.assertZipFailure('')

    def testBadArgs(self):
        self.assertRaises(TypeError, zipimport.zipimporter, None)
        self.assertRaises(TypeError, zipimport.zipimporter, TESTMOD, kwd=None)

    def testFilenameTooLong(self):
        self.assertZipFailure('A' * 33000)

    def testEmptyFile(self):
        test_support.unlink(TESTMOD)
        open(TESTMOD, 'w+').close()
        self.assertZipFailure(TESTMOD)

    def testFileUnreadable(self):
        test_support.unlink(TESTMOD)
        fd = os.open(TESTMOD, os.O_CREAT, 000)
        os.close(fd)
        self.assertZipFailure(TESTMOD)

    def testNotZipFile(self):
        test_support.unlink(TESTMOD)
        fp = open(TESTMOD, 'w+')
        fp.write('a' * 22)
        fp.close()
        self.assertZipFailure(TESTMOD)

    # XXX: disabled until this works on Big-endian machines
    def _testBogusZipFile(self):
        test_support.unlink(TESTMOD)
        fp = open(TESTMOD, 'w+')
        fp.write(struct.pack('=I', 0x06054B50))
        fp.write('a' * 18)
        fp.close()
        z = zipimport.zipimporter(TESTMOD)

        try:
            self.assertRaises(TypeError, z.find_module, None)
            self.assertRaises(TypeError, z.load_module, None)
            self.assertRaises(TypeError, z.is_package, None)
            self.assertRaises(TypeError, z.get_code, None)
            self.assertRaises(TypeError, z.get_data, None)
            self.assertRaises(TypeError, z.get_source, None)

            error = zipimport.ZipImportError
            self.assertEqual(z.find_module('abc'), None)

            self.assertRaises(error, z.load_module, 'abc')
            self.assertRaises(error, z.get_code, 'abc')
            self.assertRaises(IOError, z.get_data, 'abc')
            self.assertRaises(error, z.get_source, 'abc')
            self.assertRaises(error, z.is_package, 'abc')
        finally:
            zipimport._zip_directory_cache.clear()


def cleanup():
    # this is necessary if test is run repeated (like when finding leaks)
    global test_imported
    if test_imported:
        zipimport._zip_directory_cache.clear()
        if hasattr(UncompressedZipImportTestCase, 'testAFakeZlib'):
            delattr(UncompressedZipImportTestCase, 'testAFakeZlib')
        if hasattr(CompressedZipImportTestCase, 'testAFakeZlib'):
            delattr(CompressedZipImportTestCase, 'testAFakeZlib')
    test_imported = True

def test_main():
    cleanup()
    try:
        test_support.run_unittest(
              UncompressedZipImportTestCase,
              CompressedZipImportTestCase,
              BadFileZipImportTestCase,
            )
    finally:
        test_support.unlink(TESTMOD)

if __name__ == "__main__":
    test_main()
