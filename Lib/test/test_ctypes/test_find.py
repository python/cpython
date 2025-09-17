import os.path
import sys
import test.support
import unittest
import unittest.mock
from ctypes import CDLL, RTLD_GLOBAL
from ctypes.util import find_library
from test.support import os_helper, thread_unsafe


# On some systems, loading the OpenGL libraries needs the RTLD_GLOBAL mode.
class Test_OpenGL_libs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        lib_gl = lib_glu = lib_gle = None
        if sys.platform == "win32":
            lib_gl = find_library("OpenGL32")
            lib_glu = find_library("Glu32")
        elif sys.platform == "darwin":
            lib_gl = lib_glu = find_library("OpenGL")
        else:
            lib_gl = find_library("GL")
            lib_glu = find_library("GLU")
            lib_gle = find_library("gle")

        # print, for debugging
        if test.support.verbose:
            print("OpenGL libraries:")
            for item in (("GL", lib_gl),
                         ("GLU", lib_glu),
                         ("gle", lib_gle)):
                print("\t", item)

        cls.gl = cls.glu = cls.gle = None
        if lib_gl:
            try:
                cls.gl = CDLL(lib_gl, mode=RTLD_GLOBAL)
            except OSError:
                pass

        if lib_glu:
            try:
                cls.glu = CDLL(lib_glu, RTLD_GLOBAL)
            except OSError:
                pass

        if lib_gle:
            try:
                cls.gle = CDLL(lib_gle)
            except OSError:
                pass

    @classmethod
    def tearDownClass(cls):
        cls.gl = cls.glu = cls.gle = None

    def test_gl(self):
        if self.gl is None:
            self.skipTest('lib_gl not available')
        self.gl.glClearIndex

    def test_glu(self):
        if self.glu is None:
            self.skipTest('lib_glu not available')
        self.glu.gluBeginCurve

    def test_gle(self):
        if self.gle is None:
            self.skipTest('lib_gle not available')
        self.gle.gleGetJoinStyle

    def test_shell_injection(self):
        result = find_library('; echo Hello shell > ' + os_helper.TESTFN)
        self.assertFalse(os.path.lexists(os_helper.TESTFN))
        self.assertIsNone(result)


@unittest.skipUnless(sys.platform.startswith('linux'),
                     'Test only valid for Linux')
class FindLibraryLinux(unittest.TestCase):
    @thread_unsafe('uses setenv')
    def test_find_on_libpath(self):
        import subprocess
        import tempfile

        try:
            p = subprocess.Popen(['gcc', '--version'], stdout=subprocess.PIPE,
                                 stderr=subprocess.DEVNULL)
            out, _ = p.communicate()
        except OSError:
            raise unittest.SkipTest('gcc, needed for test, not available')
        with tempfile.TemporaryDirectory() as d:
            # create an empty temporary file
            srcname = os.path.join(d, 'dummy.c')
            libname = 'py_ctypes_test_dummy'
            dstname = os.path.join(d, 'lib%s.so' % libname)
            with open(srcname, 'wb') as f:
                pass
            self.assertTrue(os.path.exists(srcname))
            # compile the file to a shared library
            cmd = ['gcc', '-o', dstname, '--shared',
                   '-Wl,-soname,lib%s.so' % libname, srcname]
            out = subprocess.check_output(cmd)
            self.assertTrue(os.path.exists(dstname))
            # now check that the .so can't be found (since not in
            # LD_LIBRARY_PATH)
            self.assertIsNone(find_library(libname))
            # now add the location to LD_LIBRARY_PATH
            with os_helper.EnvironmentVarGuard() as env:
                KEY = 'LD_LIBRARY_PATH'
                if KEY not in env:
                    v = d
                else:
                    v = '%s:%s' % (env[KEY], d)
                env.set(KEY, v)
                # now check that the .so can be found (since in
                # LD_LIBRARY_PATH)
                self.assertEqual(find_library(libname), 'lib%s.so' % libname)

    def test_find_library_with_gcc(self):
        with unittest.mock.patch("ctypes.util._findSoname_ldconfig", lambda *args: None):
            self.assertNotEqual(find_library('c'), None)

    def test_find_library_with_ld(self):
        with unittest.mock.patch("ctypes.util._findSoname_ldconfig", lambda *args: None), \
             unittest.mock.patch("ctypes.util._findLib_gcc", lambda *args: None):
            self.assertNotEqual(find_library('c'), None)

    def test_gh114257(self):
        self.assertIsNone(find_library("libc"))


@unittest.skipUnless(sys.platform == 'android', 'Test only valid for Android')
class FindLibraryAndroid(unittest.TestCase):
    def test_find(self):
        for name in [
            "c", "m",  # POSIX
            "z",  # Non-POSIX, but present on Linux
            "log",  # Not present on Linux
        ]:
            with self.subTest(name=name):
                path = find_library(name)
                self.assertIsInstance(path, str)
                self.assertEqual(
                    os.path.dirname(path),
                    "/system/lib64" if "64" in os.uname().machine
                    else "/system/lib")
                self.assertEqual(os.path.basename(path), f"lib{name}.so")
                self.assertTrue(os.path.isfile(path), path)

        for name in ["libc", "nonexistent"]:
            with self.subTest(name=name):
                self.assertIsNone(find_library(name))


@unittest.skipUnless(test.support.is_emscripten,
                     'Test only valid for Emscripten')
class FindLibraryEmscripten(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import tempfile

        # A very simple wasm module
        # In WAT format: (module)
        cls.wasm_module = b'\x00asm\x01\x00\x00\x00\x00\x08\x04name\x02\x01\x00'

        cls.non_wasm_content = b'This is not a WASM file'

        cls.temp_dir = tempfile.mkdtemp()
        cls.libdummy_so_path = os.path.join(cls.temp_dir, 'libdummy.so')
        with open(cls.libdummy_so_path, 'wb') as f:
            f.write(cls.wasm_module)

        cls.libother_wasm_path = os.path.join(cls.temp_dir, 'libother.wasm')
        with open(cls.libother_wasm_path, 'wb') as f:
            f.write(cls.wasm_module)

        cls.libnowasm_so_path = os.path.join(cls.temp_dir, 'libnowasm.so')
        with open(cls.libnowasm_so_path, 'wb') as f:
            f.write(cls.non_wasm_content)

    @classmethod
    def tearDownClass(cls):
        import shutil
        shutil.rmtree(cls.temp_dir)

    def test_find_wasm_file_with_so_extension(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.set('LD_LIBRARY_PATH', self.temp_dir)
            result = find_library('dummy')
            self.assertEqual(result, self.libdummy_so_path)
    def test_find_wasm_file_with_wasm_extension(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.set('LD_LIBRARY_PATH', self.temp_dir)
            result = find_library('other')
            self.assertEqual(result, self.libother_wasm_path)

    def test_ignore_non_wasm_file(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.set('LD_LIBRARY_PATH', self.temp_dir)
            result = find_library('nowasm')
            self.assertIsNone(result)

    def test_find_nothing_without_ld_library_path(self):
        with os_helper.EnvironmentVarGuard() as env:
            if 'LD_LIBRARY_PATH' in env:
                del env['LD_LIBRARY_PATH']
            result = find_library('dummy')
            self.assertIsNone(result)
            result = find_library('other')
            self.assertIsNone(result)

    def test_find_nothing_with_wrong_ld_library_path(self):
        import tempfile
        with tempfile.TemporaryDirectory() as empty_dir:
            with os_helper.EnvironmentVarGuard() as env:
                env.set('LD_LIBRARY_PATH', empty_dir)
                result = find_library('dummy')
                self.assertIsNone(result)
                result = find_library('other')
                self.assertIsNone(result)


if __name__ == "__main__":
    unittest.main()
