import os.path
import sys
import unittest
from ctypes import CDLL

FOO_C = r"""
#include <stdio.h>

/* This is a 'GNU indirect function' (IFUNC) that will be called by
   dlsym() to resolve the symbol "foo" to an address. Typically, such
   a function would return the address of an actual function, but it
   can also just return NULL.  For some background on IFUNCs, see
   https://willnewton.name/uncategorized/using-gnu-indirect-functions/

   Adapted from Michael Kerrisk's answer: https://stackoverflow.com/a/53590014
*/

asm (".type foo, @gnu_indirect_function");

void *foo(void)
{
    fprintf(stderr, "foo IFUNC called\n");
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

        try:
            p = subprocess.Popen(['gcc', '--version'], stdout=subprocess.PIPE,
                                 stderr=subprocess.DEVNULL)
            out, _ = p.communicate()
        except OSError:
            raise unittest.SkipTest('gcc, needed for test, not available')
        with tempfile.TemporaryDirectory() as d:
            # Create a source file foo.c, that uses
            # a GNU Indirect Function. See FOO_C.
            srcname = os.path.join(d, 'foo.c')
            libname = 'py_ctypes_test_null_dlsym'
            dstname = os.path.join(d, 'lib%s.so' % libname)
            with open(srcname, 'w') as f:
                f.write(FOO_C)
            self.assertTrue(os.path.exists(srcname))
            # Compile the file to a shared library
            cmd = ['gcc', '-fPIC', '-shared', '-o', dstname, srcname]
            out = subprocess.check_output(cmd)
            self.assertTrue(os.path.exists(dstname))
            # Load the shared library
            L = CDLL(dstname)

            with self.assertRaises(AttributeError) as cm:
                # Try accessing the 'foo' symbol.
                # It should resolve via dlsym() to NULL,
                # and since we subjectively treat NULL
                # addresses as errors, we should get
                # an error.
                L.foo

            pred = "function 'foo' not found" in str(cm.exception)
            self.assertTrue(pred)
            # self.assertEqual(str(cm.exception),
            #                  "function 'foo' not found")

if __name__ == "__main__":
    unittest.main()
