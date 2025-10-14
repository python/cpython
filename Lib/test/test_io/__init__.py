"""Tests for the io module and its implementations (_io and _pyio)

Tests are split across multiple files to increase
parallelism and focus on specific implementation pieces.

* test_io
  * test_bufferedio - tests file buffering
  * test_memoryio - tests BytesIO and StringIO
  * test_fileio - tests FileIO
  * test_file - tests the file interface
  * test_general - tests everything else in the io module
  * test_univnewlines - tests universal newline support
  * test_largefile - tests operations on a file greater than 2**32 bytes
    (only enabled with -ulargefile)
* test_free_threading/test_io - tests thread safety of io objects

.. attention::
   When writing tests for io, it's important to test both the C and Python
   implementations. This is usually done by writing a base test that refers to
   the type it is testing as an attribute. Then it provides custom subclasses to
   test both implementations. This directory contains lots of examples.
"""

import os
from test.support import load_package_tests

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
