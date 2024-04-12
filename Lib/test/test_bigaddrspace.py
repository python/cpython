"""
These tests are meant to exercise that requests to create objects bigger
than what the address space allows are properly met with an OverflowError
(rather than crash weirdly).

Primarily, this means 32-bit builds with at least 2 GiB of available memory.
You need to pass the -M option to regrtest (e.g. "-M 2.1G") for tests to
be enabled.
"""

from test import support
from test.support import bigaddrspacetest, MAX_Py_ssize_t

import contextlib
import operator
import os
import sys
import unittest


@contextlib.contextmanager
def ignore_stderr():
    fd = 2
    old_stderr = os.dup(fd)
    try:
        # Redirect stderr to /dev/null
        with open(os.devnull, 'wb') as null:
            os.dup2(null.fileno(), fd)
            yield
    finally:
        os.dup2(old_stderr, fd)
        os.close(old_stderr)


class BytesTest(unittest.TestCase):

    @bigaddrspacetest
    def test_concat(self):
        # Allocate a bytestring that's near the maximum size allowed by
        # the address space, and then try to build a new, larger one through
        # concatenation.
        try:
            x = b"x" * (MAX_Py_ssize_t - 128)
            self.assertRaises(OverflowError, operator.add, x, b"x" * 128)
        finally:
            x = None

    @bigaddrspacetest
    def test_optimized_concat(self):
        try:
            x = b"x" * (MAX_Py_ssize_t - 128)

            with self.assertRaises(OverflowError) as cm:
                # this statement used a fast path in ceval.c
                x = x + b"x" * 128

            with self.assertRaises(OverflowError) as cm:
                # this statement used a fast path in ceval.c
                x +=  b"x" * 128
        finally:
            x = None

    @bigaddrspacetest
    def test_repeat(self):
        try:
            x = b"x" * (MAX_Py_ssize_t - 128)
            self.assertRaises(OverflowError, operator.mul, x, 128)
        finally:
            x = None

    @unittest.skipUnless(sys.maxsize >= 0x7FFFFFFF_FFFFFFFF,
                         'need 64-bit size')
    def test_large_alloc(self):
        debug_bytes = 0
        if support.check_impl_detail(cpython=True) and support.Py_DEBUG:
            try:
                from _testcapi import SIZEOF_SIZE_T
            except ImportError:
                if sys.maxsize > 0xffff_ffff:
                    SIZEOF_SIZE_T = 8
                else:
                    SIZEOF_SIZE_T = 4

            # The debug hook on memory allocator adds 3 size_t per memory block
            # See PYMEM_DEBUG_EXTRA_BYTES in Objects/obmalloc.c.
            debug_bytes = SIZEOF_SIZE_T * 3

            try:
                from _testinternalcapi import pymem_getallocatorsname
                if not pymem_getallocatorsname().endswith('_debug'):
                    # PYTHONMALLOC env var is used and disables the debug hook
                    debug_bytes = 0
            except (ImportError, RuntimeError):
                pass

        def allocate(size):
            length = size - sys.getsizeof(b'') - debug_bytes
            # allocate 'size' bytes
            return b'x' * length

        # 64-bit size which cannot be allocated on any reasonable hardware
        # (in 2024) and must fail immediately with MemoryError.
        for size in (
            # gh-114331: test_decimal.test_maxcontext_exact_arith()
            0x0BAFC246_72035E58,
            # gh-117755: test_io.test_constructor()
            0x7FFFFFFF_FFFFFFFF,
        ):
            with self.subTest(size=size):
                with self.assertRaises(MemoryError):
                    # ignore "mimalloc: error: unable to allocate memory"
                    with ignore_stderr():
                        allocate(size)


class StrTest(unittest.TestCase):

    unicodesize = 4

    @bigaddrspacetest
    def test_concat(self):
        try:
            # Create a string that would fill almost the address space
            x = "x" * int(MAX_Py_ssize_t // (1.1 * self.unicodesize))
            # Unicode objects trigger MemoryError in case an operation that's
            # going to cause a size overflow is executed
            self.assertRaises(MemoryError, operator.add, x, x)
        finally:
            x = None

    @bigaddrspacetest
    def test_optimized_concat(self):
        try:
            x = "x" * int(MAX_Py_ssize_t // (1.1 * self.unicodesize))

            with self.assertRaises(MemoryError) as cm:
                # this statement uses a fast path in ceval.c
                x = x + x

            with self.assertRaises(MemoryError) as cm:
                # this statement uses a fast path in ceval.c
                x +=  x
        finally:
            x = None

    @bigaddrspacetest
    def test_repeat(self):
        try:
            x = "x" * int(MAX_Py_ssize_t // (1.1 * self.unicodesize))
            self.assertRaises(MemoryError, operator.mul, x, 2)
        finally:
            x = None


if __name__ == '__main__':
    if len(sys.argv) > 1:
        support.set_memlimit(sys.argv[1])
    unittest.main()
