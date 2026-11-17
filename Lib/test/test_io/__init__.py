"""Tests for the io module and its implementations (_io and _pyio)

Tests are split across multiple files to increase
parallelism and focus on specific implementation pieces.

Tests for code that uses io live with that code, for example:

* test_capi.test_file - the PyFile_* C API
* test_builtin, test_print - open(), input() and print()
* test_cmd_line, test_subprocess - stdio buffering and newlines at startup
  and through pipes
* test_utf8_mode - UTF-8 mode encodings, including with _pyio
* test_os, test_eintr - fd-level I/O and EINTR retries

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
