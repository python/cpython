- Issue #18982: Add tests for CLI of the calendar module.

- Issue #19548: Added some additional checks to test_codecs to ensure that
  statements in the updated documentation remain accurate. Patch by Martin
  Panter.

- Issue #22838: All test_re tests now work with unittest test discovery.

- Issue #22173: Update lib2to3 tests to use unittest test discovery.

- Issue #16000: Convert test_curses to use unittest.

- Issue #21456: Skip two tests in test_urllib2net.py if _ssl module not
  present. Patch by Remi Pointel.

- Issue #20746: Fix test_pdb to run in refleak mode (-R).  Patch by Xavier
  de Gaye.

- Issue #22060: test_ctypes has been somewhat cleaned up and simplified; it
  now uses unittest test discovery to find its tests.

- Issue #22104: regrtest.py no longer holds a reference to the suite of tests
  loaded from test modules that don't define test_main().

- Issue #22111: Assorted cleanups in test_imaplib.  Patch by Milan Oberkirch.

- Issue #22002: Added ``load_package_tests`` function to test.support and used
  it to implement/augment test discovery in test_asyncio, test_email,
  test_importlib, test_json, and test_tools.

- Issue #21976: Fix test_ssl to accept LibreSSL version strings.  Thanks
  to William Orr.

- Issue #21918: Converted test_tools from a module to a package containing
  separate test files for each tested script.

- Issue #9554: Use modern unittest features in test_argparse. Initial patch by
  Denver Coneybeare and Radu Voicilas.

- Issue #20155: Changed HTTP method names in failing tests in test_httpservers
  so that packet filtering software (specifically Windows Base Filtering Engine)
  does not interfere with the transaction semantics expected by the tests.

- Issue #19493: Refactored the ctypes test package to skip tests explicitly
  rather than silently.

- Issue #18492: All resources are now allowed when tests are not run by
  regrtest.py.

- Issue #21634: Fix pystone micro-benchmark: use floor division instead of true
  division to benchmark integers instead of floating point numbers. Set pystone
  version to 1.2. Patch written by Lennart Regebro.

- Issue #21605: Added tests for Tkinter images.

- Issue #21493: Added test for ntpath.expanduser().  Original patch by
  Claudiu Popa.

- Issue #19925: Added tests for the spwd module. Original patch by Vajrasky Kok.

- Issue #21522: Added Tkinter tests for Listbox.itemconfigure(),
  PanedWindow.paneconfigure(), and Menu.entryconfigure().

- Issue #17756: Fix test_code test when run from the installed location.

- Issue #17752: Fix distutils tests when run from the installed location.

- Issue #18604: Consolidated checks for GUI availability.  All platforms now
  at least check whether Tk can be instantiated when the GUI resource is
  requested.

- Issue #21275: Fix a socket test on KFreeBSD.

- Issue #21223: Pass test_site/test_startup_imports when some of the extensions
  are built as builtins.

- Issue #20635: Added tests for Tk geometry managers.

- Add test case for freeze.

- Issue #20743: Fix a reference leak in test_tcl.

- Issue #21097: Move test_namespace_pkgs into test_importlib.

- Issue #21503: Use test_both() consistently in test_importlib.

- Issue #20939: Avoid various network test failures due to new
  redirect of http://www.python.org/ to https://www.python.org:
  use http://www.example.com instead.

- Issue #20668: asyncio tests no longer rely on tests.txt file.
  (Patch by Vajrasky Kok)

- Issue #21093: Prevent failures of ctypes test_macholib on OS X if a
  copy of libz exists in $HOME/lib or /usr/local/lib.

- Issue #22770: Prevent some Tk segfaults on OS X when running gui tests.

- Issue #23211: Workaround test_logging failure on some OS X 10.6 systems.

- Issue #23345: Prevent test_ssl failures with large OpenSSL patch level
  values (like 0.9.8zc).

