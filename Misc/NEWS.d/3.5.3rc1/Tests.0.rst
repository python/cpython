- Issue #28950: Disallow -j0 to be combined with -T/-l/-M in regrtest
  command line arguments.

- Issue #28666: Now test.support.rmtree is able to remove unwritable or
  unreadable directories.

- Issue #23839: Various caches now are cleared before running every test file.

- Issue #28409: regrtest: fix the parser of command line arguments.

- Issue #27787: Call gc.collect() before checking each test for "dangling
  threads", since the dangling threads are weak references.

- Issue #27369: In test_pyexpat, avoid testing an error message detail that
  changed in Expat 2.2.0.

