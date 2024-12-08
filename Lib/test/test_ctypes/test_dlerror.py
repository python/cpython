import os
import sys
import unittest
import platform

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
                     'Test only valid for Linux')
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

        # To avoid ImportErrors on Windows, where _ctypes does not have
        # dlopen and dlsym,
        # import here, i.e., inside the test function.
        # The skipUnless('linux') decorator ensures that we're on linux
        # if we're executing these statements.
        from ctypes import CDLL, c_int
        from _ctypes import dlopen, dlsym

        retcode = subprocess.call(["gcc", "--version"],
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL)
        if retcode != 0:
            self.skipTest("gcc is missing")

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
            L = dlopen(dstname)
            with self.assertRaisesRegex(OSError, "symbol 'foo' not found"):
                dlsym(L, "foo")

            # Assert that the IFUNC was called
            self.assertEqual(os.read(pipe_r, 2), b'OK')


if __name__ == "__main__":
    unittest.main()
