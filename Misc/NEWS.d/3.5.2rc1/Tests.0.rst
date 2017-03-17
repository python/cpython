- Issue #21916: Added tests for the turtle module.  Patch by ingrid,
  Gregory Loyse and Jelle Zijlstra.

- Issue #26523: The multiprocessing thread pool (multiprocessing.dummy.Pool)
  was untested.

- Issue #26015: Added new tests for pickling iterators of mutable sequences.

- Issue #26325: Added test.support.check_no_resource_warning() to check that
  no ResourceWarning is emitted.

- Issue #25940: Changed test_ssl to use self-signed.pythontest.net.  This
  avoids relying on svn.python.org, which recently changed root certificate.

- Issue #25616: Tests for OrderedDict are extracted from test_collections
  into separate file test_ordered_dict.

- Issue #26583: Skip test_timestamp_overflow in test_import if bytecode
  files cannot be written.

