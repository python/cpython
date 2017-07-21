Fuzz Tests for CPython
======================

These fuzz tests are designed to be included in Google's `oss-fuzz`_ project.

Adding a new fuzz test
----------------------

In a file named ``$test_name.inc``, add a function with the same name::

    int $test_name (const char* data, size_t size) {
        ...
        return 0;
    }


This function will be called from ``LLVMFuzzerTestOneInput`` for fuzz testing
when run in oss-fuzz.

What makes a good fuzz test
---------------------------

Libraries written in C that might handle untrusted data are worthwhile. See the
existing examples for reference, and refer to the `oss-fuzz`_ docs.

.. _oss-fuzz: https://github.com/google/oss-fuzz
