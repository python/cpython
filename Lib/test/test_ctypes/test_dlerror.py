import os
import sys
import unittest
from ctypes import CDLL

FOO_C = r"""
#include <unistd.h>

/* This is a 'GNU indirect function' (IFUNC) that will be called by
   dlsym() to resolve the symbol "foo" to an address. Typically, such
   a function would return the address of an actual function, but it
   can also just return NULL.  For some background on IFUNCs, see
   https://willnewton.name/uncategorized/using-gnu-indirect-functions.

   Adapted from Michael Kerrisk's answer: https://stackoverflow.com/a/53590014.
*/

asm (".type foo, @gnu_indirect_function");

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

        # Recall: using shell=True is equivalent to: /bin/sh -c 'args[0]' args[1] ...,
        # so we need to pass a list with a single item, namely the command to run.
        retcode = subprocess.call(["gcc --version"],
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL,
                                  shell=True)
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
            self.assertEqual(p.returncode, 0, p)
            # Load the shared library
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


if __name__ == "__main__":
    unittest.main()
