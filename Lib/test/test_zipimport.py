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
import linecache
import doctest
import inspect
import io
from traceback import extract_tb, extract_stack, print_tb
raise_src = 'def do_raise(): raise TypeError\n'

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
            mtime = int(-0x100000000 + int(mtime))
    pyc = imp.get_magic() + struct.pack("<i", int(mtime)) + data
    return pyc

def module_path_to_dotted_name(path):
    return path.replace(os.sep, '.')

NOW = time.time()
test_pyc = make_pyc(test_co, NOW)


if __debug__:
    pyc_ext = ".pyc"
else:
    pyc_ext = ".pyo"


TESTMOD = "ziptestmodule"
TESTPACK = "ziptestpackage"
TESTPACK2 = "ziptestpackage2"
TEMP_ZIP = os.path.abspath("junk95142.zip")

class UncompressedZipImportTestCase(ImportHooksBaseTestCase):

    compression = ZIP_STORED

    def setUp(self):
        # We're reusing the zip archive path, so we must clear the
        # cached directory info and linecache
        linecache.clearcache()
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

            call = kw.get('call')
            if call is not None:
                call(mod)

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
        badmagic_pyc = bytearray(test_pyc)
        badmagic_pyc[0] ^= 0x04  # flip an arbitrary bit
        files = {TESTMOD + ".py": (NOW, test_src),
                 TESTMOD + pyc_ext: (NOW, badmagic_pyc)}
        self.doTest(".py", files, TESTMOD)

    def testBadMagic2(self):
        # make pyc magic word invalid, causing an ImportError
        badmagic_pyc = bytearray(test_pyc)
        badmagic_pyc[0] ^= 0x04  # flip an arbitrary bit
        files = {TESTMOD + pyc_ext: (NOW, badmagic_pyc)}
        try:
            self.doTest(".py", files, TESTMOD)
        except ImportError:
            pass
        else:
            self.fail("expected ImportError; import from bad pyc")

    def testBadMTime(self):
        badtime_pyc = bytearray(test_pyc)
        badtime_pyc[7] ^= 0x02  # flip the second bit -- not the first as that one
                                # isn't stored in the .py's mtime in the zip archive.
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
            mod = __import__(module_path_to_dotted_name(mod_name))
            self.assertEquals(zi.get_source(TESTPACK), None)
            self.assertEquals(zi.get_source(mod_name), None)

            # test prefix and archivepath members
            zi2 = zipimport.zipimporter(TEMP_ZIP + os.sep + TESTPACK)
            self.assertEquals(zi2.archive, TEMP_ZIP)
            self.assertEquals(zi2.prefix, TESTPACK + os.sep)
        finally:
            z.close()
            os.remove(TEMP_ZIP)

    def testGetData(self):
        z = ZipFile(TEMP_ZIP, "w")
        z.compression = self.compression
        try:
            name = "testdata.dat"
            data = bytes(x for x in range(256))
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
        if __loader__.get_data("some.data") != b"some data":
            raise AssertionError("bad data")\n"""
        pyc = make_pyc(compile(src, "<???>", "exec"), NOW)
        files = {TESTMOD + pyc_ext: (NOW, pyc),
                 "some.data": (NOW, "some data")}
        self.doTest(pyc_ext, files, TESTMOD)

    def testImport_WithStuff(self):
        # try importing from a zipfile which contains additional
        # stuff at the beginning of the file
        files = {TESTMOD + ".py": (NOW, test_src)}
        self.doTest(".py", files, TESTMOD,
                    stuff=b"Some Stuff"*31)

    def assertModuleSource(self, module):
        self.assertEqual(inspect.getsource(module), test_src)

    def testGetSource(self):
        files = {TESTMOD + ".py": (NOW, test_src)}
        self.doTest(".py", files, TESTMOD, call=self.assertModuleSource)

    def testGetCompiledSource(self):
        pyc = make_pyc(compile(test_src, "<???>", "exec"), NOW)
        files = {TESTMOD + ".py": (NOW, test_src),
                 TESTMOD + pyc_ext: (NOW, pyc)}
        self.doTest(pyc_ext, files, TESTMOD, call=self.assertModuleSource)

    def runDoctest(self, callback):
        files = {TESTMOD + ".py": (NOW, test_src),
                 "xyz.txt": (NOW, ">>> log.append(True)\n")}
        self.doTest(".py", files, TESTMOD, call=callback)

    def doDoctestFile(self, module):
        log = []
        old_master, doctest.master = doctest.master, None
        try:
            doctest.testfile(
                'xyz.txt', package=module, module_relative=True,
                globs=locals()
            )
        finally:
            doctest.master = old_master
        self.assertEqual(log,[True])

    def testDoctestFile(self):
        self.runDoctest(self.doDoctestFile)

    def doDoctestSuite(self, module):
        log = []
        doctest.DocFileTest(
            'xyz.txt', package=module, module_relative=True,
            globs=locals()
        ).run()
        self.assertEqual(log,[True])

    def testDoctestSuite(self):
        self.runDoctest(self.doDoctestSuite)


    def doTraceback(self, module):
        try:
            module.do_raise()
        except:
            tb = sys.exc_info()[2].tb_next

            f,lno,n,line = extract_tb(tb, 1)[0]
            self.assertEqual(line, raise_src.strip())

            f,lno,n,line = extract_stack(tb.tb_frame, 1)[0]
            self.assertEqual(line, raise_src.strip())

            s = io.StringIO()
            print_tb(tb, 1, s)
            self.failUnless(s.getvalue().endswith(raise_src))
        else:
            raise AssertionError("This ought to be impossible")

    def testTraceback(self):
        files = {TESTMOD + ".py": (NOW, raise_src)}
        self.doTest(None, files, TESTMOD, call=self.doTraceback)


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
        try:
            os.close(fd)
            self.assertZipFailure(TESTMOD)
        finally:
            # If we leave "the read-only bit" set on Windows, nothing can
            # delete TESTMOD, and later tests suffer bogus failures.
            os.chmod(TESTMOD, 0o666)
            test_support.unlink(TESTMOD)

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
