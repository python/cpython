import sys
import os
import marshal
import imp
import struct
import time
import unittest

from test import support
from test.test_importhooks import ImportHooksBaseTestCase, test_src, test_co

from zipfile import ZipFile, ZipInfo, ZIP_STORED, ZIP_DEFLATED

import zipimport
import linecache
import doctest
import inspect
import io
from traceback import extract_tb, extract_stack, print_tb
raise_src = 'def do_raise(): raise TypeError\n'

def make_pyc(co, mtime, size):
    data = marshal.dumps(co)
    if type(mtime) is type(0.0):
        # Mac mtimes need a bit of special casing
        if mtime < 0x7fffffff:
            mtime = int(mtime)
        else:
            mtime = int(-0x100000000 + int(mtime))
    pyc = imp.get_magic() + struct.pack("<ii", int(mtime), size & 0xFFFFFFFF) + data
    return pyc

def module_path_to_dotted_name(path):
    return path.replace(os.sep, '.')

NOW = time.time()
test_pyc = make_pyc(test_co, NOW, len(test_src))


TESTMOD = "ziptestmodule"
TESTPACK = "ziptestpackage"
TESTPACK2 = "ziptestpackage2"
TEMP_ZIP = os.path.abspath("junk95142.zip")

pyc_file = imp.cache_from_source(TESTMOD + '.py')
pyc_ext = ('.pyc' if __debug__ else '.pyo')


def _write_zip_package(zipname, files,
                       data_to_prepend=b"", compression=ZIP_STORED):
    z = ZipFile(zipname, "w")
    try:
        for name, (mtime, data) in files.items():
            zinfo = ZipInfo(name, time.localtime(mtime))
            zinfo.compress_type = compression
            z.writestr(zinfo, data)
    finally:
        z.close()

    if data_to_prepend:
        # Prepend data to the start of the zipfile
        with open(zipname, "rb") as f:
            zip_data = f.read()

        with open(zipname, "wb") as f:
            f.write(data_to_prepend)
            f.write(zip_data)


class UncompressedZipImportTestCase(ImportHooksBaseTestCase):

    compression = ZIP_STORED

    def setUp(self):
        # We're reusing the zip archive path, so we must clear the
        # cached directory info and linecache
        linecache.clearcache()
        zipimport._zip_directory_cache.clear()
        ImportHooksBaseTestCase.setUp(self)

    def doTest(self, expected_ext, files, *modules, **kw):
        _write_zip_package(TEMP_ZIP, files, data_to_prepend=kw.get("stuff"),
                           compression=self.compression)
        try:
            sys.path.insert(0, TEMP_ZIP)

            mod = __import__(".".join(modules), globals(), locals(),
                             ["__dummy__"])

            call = kw.get('call')
            if call is not None:
                call(mod)

            if expected_ext:
                file = mod.get_file()
                self.assertEqual(file, os.path.join(TEMP_ZIP,
                                 *modules) + expected_ext)
        finally:
            while TEMP_ZIP in sys.path:
                sys.path.remove(TEMP_ZIP)
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
            self.skipTest('zlib is a builtin module')
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
        # flip the second bit -- not the first as that one isn't stored in the
        # .py's mtime in the zip archive.
        badtime_pyc[7] ^= 0x02
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
            self.assertEqual(zi.archive, TEMP_ZIP)
            self.assertEqual(zi.is_package(TESTPACK), True)
            mod = zi.load_module(TESTPACK)
            self.assertEqual(zi.get_filename(TESTPACK), mod.__file__)

            existing_pack_path = __import__(TESTPACK).__path__[0]
            expected_path_path = os.path.join(TEMP_ZIP, TESTPACK)
            self.assertEqual(existing_pack_path, expected_path_path)

            self.assertEqual(zi.is_package(packdir + '__init__'), False)
            self.assertEqual(zi.is_package(packdir + TESTPACK2), True)
            self.assertEqual(zi.is_package(packdir2 + TESTMOD), False)

            mod_path = packdir2 + TESTMOD
            mod_name = module_path_to_dotted_name(mod_path)
            __import__(mod_name)
            mod = sys.modules[mod_name]
            self.assertEqual(zi.get_source(TESTPACK), None)
            self.assertEqual(zi.get_source(mod_path), None)
            self.assertEqual(zi.get_filename(mod_path), mod.__file__)
            # To pass in the module name instead of the path, we must use the
            # right importer
            loader = mod.__loader__
            self.assertEqual(loader.get_source(mod_name), None)
            self.assertEqual(loader.get_filename(mod_name), mod.__file__)

            # test prefix and archivepath members
            zi2 = zipimport.zipimporter(TEMP_ZIP + os.sep + TESTPACK)
            self.assertEqual(zi2.archive, TEMP_ZIP)
            self.assertEqual(zi2.prefix, TESTPACK + os.sep)
        finally:
            z.close()
            os.remove(TEMP_ZIP)

    def testZipImporterMethodsInSubDirectory(self):
        packdir = TESTPACK + os.sep
        packdir2 = packdir + TESTPACK2 + os.sep
        files = {packdir2 + "__init__" + pyc_ext: (NOW, test_pyc),
                 packdir2 + TESTMOD + pyc_ext: (NOW, test_pyc)}

        z = ZipFile(TEMP_ZIP, "w")
        try:
            for name, (mtime, data) in files.items():
                zinfo = ZipInfo(name, time.localtime(mtime))
                zinfo.compress_type = self.compression
                z.writestr(zinfo, data)
            z.close()

            zi = zipimport.zipimporter(TEMP_ZIP + os.sep + packdir)
            self.assertEqual(zi.archive, TEMP_ZIP)
            self.assertEqual(zi.prefix, packdir)
            self.assertEqual(zi.is_package(TESTPACK2), True)
            mod = zi.load_module(TESTPACK2)
            self.assertEqual(zi.get_filename(TESTPACK2), mod.__file__)

            self.assertEqual(
                zi.is_package(TESTPACK2 + os.sep + '__init__'), False)
            self.assertEqual(
                zi.is_package(TESTPACK2 + os.sep + TESTMOD), False)

            mod_path = TESTPACK2 + os.sep + TESTMOD
            mod_name = module_path_to_dotted_name(mod_path)
            __import__(mod_name)
            mod = sys.modules[mod_name]
            self.assertEqual(zi.get_source(TESTPACK2), None)
            self.assertEqual(zi.get_source(mod_path), None)
            self.assertEqual(zi.get_filename(mod_path), mod.__file__)
            # To pass in the module name instead of the path, we must use the
            # right importer
            loader = mod.__loader__
            self.assertEqual(loader.get_source(mod_name), None)
            self.assertEqual(loader.get_filename(mod_name), mod.__file__)
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
            self.assertEqual(data, zi.get_data(name))
            self.assertIn('zipimporter object', repr(zi))
        finally:
            z.close()
            os.remove(TEMP_ZIP)

    def testImporterAttr(self):
        src = """if 1:  # indent hack
        def get_file():
            return __file__
        if __loader__.get_data("some.data") != b"some data":
            raise AssertionError("bad data")\n"""
        pyc = make_pyc(compile(src, "<???>", "exec"), NOW, len(src))
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
        pyc = make_pyc(compile(test_src, "<???>", "exec"), NOW, len(test_src))
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
            self.assertTrue(s.getvalue().endswith(raise_src))
        else:
            raise AssertionError("This ought to be impossible")

    def testTraceback(self):
        files = {TESTMOD + ".py": (NOW, raise_src)}
        self.doTest(None, files, TESTMOD, call=self.doTraceback)

    @unittest.skipIf(support.TESTFN_UNENCODABLE is None,
                     "need an unencodable filename")
    def testUnencodable(self):
        filename = support.TESTFN_UNENCODABLE + ".zip"
        z = ZipFile(filename, "w")
        zinfo = ZipInfo(TESTMOD + ".py", time.localtime(NOW))
        zinfo.compress_type = self.compression
        z.writestr(zinfo, test_src)
        z.close()
        try:
            zipimport.zipimporter(filename)
        finally:
            os.remove(filename)


@support.requires_zlib
class CompressedZipImportTestCase(UncompressedZipImportTestCase):
    compression = ZIP_DEFLATED


class ZipFileModifiedAfterImportTestCase(ImportHooksBaseTestCase):
    def setUp(self):
        zipimport._zip_directory_cache.clear()
        zipimport._zip_stat_cache.clear()
        ImportHooksBaseTestCase.setUp(self)

    def tearDown(self):
        ImportHooksBaseTestCase.tearDown(self)
        if os.path.exists(TEMP_ZIP):
            os.remove(TEMP_ZIP)

    def testZipFileChangesAfterFirstImport(self):
        """Alter the zip file after caching its index and try an import."""
        packdir = TESTPACK + os.sep
        files = {packdir + "__init__" + pyc_ext: (NOW, test_pyc),
                 packdir + TESTMOD + ".py": (NOW, "test_value = 38\n"),
                 "ziptest_a.py": (NOW, "test_value = 23\n"),
                 "ziptest_b.py": (NOW, "test_value = 42\n"),
                 "ziptest_c.py": (NOW, "test_value = 1337\n")}
        zipfile_path = TEMP_ZIP
        _write_zip_package(zipfile_path, files)
        self.assertTrue(os.path.exists(zipfile_path))
        sys.path.insert(0, zipfile_path)

        # Import something out of the zipfile and confirm it is correct.
        testmod = __import__(TESTPACK + "." + TESTMOD,
                             globals(), locals(), ["__dummy__"])
        self.assertEqual(testmod.test_value, 38)
        # Import something else out of the zipfile and confirm it is correct.
        ziptest_b = __import__("ziptest_b", globals(), locals(), ["test_value"])
        self.assertEqual(ziptest_b.test_value, 42)

        # Truncate and fill the zip file with non-zip garbage.
        with open(zipfile_path, "rb") as orig_zip_file:
            orig_zip_file_contents = orig_zip_file.read()
        with open(zipfile_path, "wb") as byebye_valid_zip_file:
            byebye_valid_zip_file.write(b"Tear down this wall!\n"*1987)
        # Now that the zipfile has been replaced, import something else from it
        # which should fail as the file contents are now garbage.
        with self.assertRaises(ImportError):
            ziptest_a = __import__("ziptest_a", globals(), locals(),
                                   ["test_value"])
            self.assertEqual(ziptest_a.test_value, 23)

        # Now lets make it a valid zipfile that has some garbage at the start.
        # This alters all of the offsets within the file
        with open(zipfile_path, "wb") as new_zip_file:
            new_zip_file.write(b"X"*1991)  # The year Python was created.
            new_zip_file.write(orig_zip_file_contents)

        # Now that the zip file has been "restored" to a valid but different
        # zipfile the zipimporter should *successfully* re-read the new zip
        # file's end of file central index and be able to import from it again.
        ziptest_c = __import__("ziptest_c", globals(), locals(), ["test_value"])
        self.assertEqual(ziptest_c.test_value, 1337)


class BadFileZipImportTestCase(unittest.TestCase):
    def assertZipFailure(self, filename):
        with self.assertRaises(zipimport.ZipImportError):
            zipimport.zipimporter(filename)

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
        support.unlink(TESTMOD)
        support.create_empty_file(TESTMOD)
        self.assertZipFailure(TESTMOD)

    def testFileUnreadable(self):
        support.unlink(TESTMOD)
        fd = os.open(TESTMOD, os.O_CREAT, 000)
        try:
            os.close(fd)
            self.assertZipFailure(TESTMOD)
        finally:
            # If we leave "the read-only bit" set on Windows, nothing can
            # delete TESTMOD, and later tests suffer bogus failures.
            os.chmod(TESTMOD, 0o666)
            support.unlink(TESTMOD)

    def testNotZipFile(self):
        support.unlink(TESTMOD)
        fp = open(TESTMOD, 'w+')
        fp.write('a' * 22)
        fp.close()
        self.assertZipFailure(TESTMOD)

    # XXX: disabled until this works on Big-endian machines
    def _testBogusZipFile(self):
        support.unlink(TESTMOD)
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


def test_main():
    try:
        support.run_unittest(
              UncompressedZipImportTestCase,
              CompressedZipImportTestCase,
              BadFileZipImportTestCase,
              ZipFileModifiedAfterImportTestCase,
            )
    finally:
        support.unlink(TESTMOD)

if __name__ == "__main__":
    test_main()
