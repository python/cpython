import _ctypes
import os
import platform
import sys
import test.support
import unittest
from ctypes import CDLL, c_int
from ctypes.util import find_library


FOO_C = r"""
#include <unistd.h>

/* This is a 'GNU indirect function' (IFUNC) that will be called by
   dlsym() to resolve the symbol "foo" to an address. Typically, such
   a function would return the address of an actual function, but it
   can also just return NULL.  For some background on IFUNCs, see
   https://willnewton.name/uncategorized/using-gnu-indirect-functions.

   Adapted from Michael Kerrisk's answer: https://stackoverflow.com/a/53590014.
*/

asm (".type foo STT_GNU_IFUNC");

void *foo(void)
{
    write($DESCRIPTOR, "OK", 2);
    return NULL;
}
"""


@unittest.skipUnless(sys.platform.startswith('linux'),
                     'test requires GNU IFUNC support')
class TestNullDlsym(unittest.TestCase):
    """GH-126554: Ensure that we catch NULL dlsym return values

    In rare cases, such as when using GNU IFUNCs, dlsym(),
    the C function that ctypes' CDLL uses to get the address
    of symbols, can return NULL.

    The objective way of telling if an error during symbol
    lookup happened is to call glibc's dlerror() and check
    for a non-NULL return value.

    However, there can be cases where dlsym() returns NULL
    and dlerror() is also NULL, meaning that glibc did not
    encounter any error.

    In the case of ctypes, we subjectively treat that as
    an error, and throw a relevant exception.

    This test case ensures that we correctly enforce
    this 'dlsym returned NULL -> throw Error' rule.
    """

    def test_null_dlsym(self):
        import subprocess
        import tempfile

        try:
            retcode = subprocess.call(["gcc", "--version"],
                                      stdout=subprocess.DEVNULL,
                                      stderr=subprocess.DEVNULL)
        except OSError:
            self.skipTest("gcc is missing")
        if retcode != 0:
            self.skipTest("gcc --version failed")

        pipe_r, pipe_w = os.pipe()
        self.addCleanup(os.close, pipe_r)
        self.addCleanup(os.close, pipe_w)

        with tempfile.TemporaryDirectory() as d:
            # Create a C file with a GNU Indirect Function (FOO_C)
            # and compile it into a shared library.
            srcname = os.path.join(d, 'foo.c')
            dstname = os.path.join(d, 'libfoo.so')
            with open(srcname, 'w') as f:
                f.write(FOO_C.replace('$DESCRIPTOR', str(pipe_w)))
            args = ['gcc', '-fPIC', '-shared', '-o', dstname, srcname]
            p = subprocess.run(args, capture_output=True)

            if p.returncode != 0:
                # IFUNC is not supported on all architectures.
                if platform.machine() == 'x86_64':
                    # It should be supported here. Something else went wrong.
                    p.check_returncode()
                else:
                    # IFUNC might not be supported on this machine.
                    self.skipTest(f"could not compile indirect function: {p}")

            # Case #1: Test 'PyCFuncPtr_FromDll' from Modules/_ctypes/_ctypes.c
            L = CDLL(dstname)
            with self.assertRaisesRegex(AttributeError, "function 'foo' not found"):
                # Try accessing the 'foo' symbol.
                # It should resolve via dlsym() to NULL,
                # and since we subjectively treat NULL
                # addresses as errors, we should get
                # an error.
                L.foo

            # Assert that the IFUNC was called
            self.assertEqual(os.read(pipe_r, 2), b'OK')

            # Case #2: Test 'CDataType_in_dll_impl' from Modules/_ctypes/_ctypes.c
            with self.assertRaisesRegex(ValueError, "symbol 'foo' not found"):
                c_int.in_dll(L, "foo")

            # Assert that the IFUNC was called
            self.assertEqual(os.read(pipe_r, 2), b'OK')

            # Case #3: Test 'py_dl_sym' from Modules/_ctypes/callproc.c
            dlopen = test.support.get_attribute(_ctypes, 'dlopen')
            dlsym = test.support.get_attribute(_ctypes, 'dlsym')
            L = dlopen(dstname)
            with self.assertRaisesRegex(OSError, "symbol 'foo' not found"):
                dlsym(L, "foo")

            # Assert that the IFUNC was called
            self.assertEqual(os.read(pipe_r, 2), b'OK')


@unittest.skipUnless(os.name != 'nt', 'test requires dlerror() calls')
class TestLocalization(unittest.TestCase):

    @staticmethod
    def configure_locales(func):
        return test.support.run_with_locale(
            'LC_ALL',
            'fr_FR.iso88591', 'ja_JP.sjis', 'zh_CN.gbk',
            'fr_FR.utf8', 'en_US.utf8',
            '',
        )(func)

    @classmethod
    def setUpClass(cls):
        cls.libc_filename = find_library("c")
        if cls.libc_filename is None:
            raise unittest.SkipTest('cannot find libc')

    @configure_locales
    def test_localized_error_from_dll(self):
        dll = CDLL(self.libc_filename)
        with self.assertRaises(AttributeError):
            dll.this_name_does_not_exist

    @configure_locales
    def test_localized_error_in_dll(self):
        dll = CDLL(self.libc_filename)
        with self.assertRaises(ValueError):
            c_int.in_dll(dll, 'this_name_does_not_exist')

    @unittest.skipUnless(hasattr(_ctypes, 'dlopen'),
                         'test requires _ctypes.dlopen()')
    @configure_locales
    def test_localized_error_dlopen(self):
        missing_filename = b'missing\xff.so'
        # Depending whether the locale, we may encode '\xff' differently
        # but we are only interested in avoiding a UnicodeDecodeError
        # when reporting the dlerror() error message which contains
        # the localized filename.
        filename_pattern = r'missing.*?\.so'
        with self.assertRaisesRegex(OSError, filename_pattern):
            _ctypes.dlopen(missing_filename, 2)

    @unittest.skipUnless(hasattr(_ctypes, 'dlopen'),
                         'test requires _ctypes.dlopen()')
    @unittest.skipUnless(hasattr(_ctypes, 'dlsym'),
                         'test requires _ctypes.dlsym()')
    @configure_locales
    def test_localized_error_dlsym(self):
        dll = _ctypes.dlopen(self.libc_filename)
        with self.assertRaises(OSError):
            _ctypes.dlsym(dll, 'this_name_does_not_exist')


if __name__ == "__main__":
    unittest.main()
