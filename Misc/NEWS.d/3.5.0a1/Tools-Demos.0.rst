- Issue #22314: pydoc now works when the LINES environment variable is set.

- Issue #22615: Argument Clinic now supports the "type" argument for the
  int converter.  This permits using the int converter with enums and
  typedefs.

- Issue #20076: The makelocalealias.py script no longer ignores UTF-8 mapping.

- Issue #20079: The makelocalealias.py script now can parse the SUPPORTED file
  from glibc sources and supports command line options for source paths.

- Issue #22201: Command-line interface of the zipfile module now correctly
  extracts ZIP files with directory entries.  Patch by Ryan Wilson.

- Issue #22120: For functions using an unsigned integer return converter,
  Argument Clinic now generates a cast to that type for the comparison
  to -1 in the generated code.  (This suppresses a compilation warning.)

- Issue #18974: Tools/scripts/diff.py now uses argparse instead of optparse.

- Issue #21906: Make Tools/scripts/md5sum.py work in Python 3.
  Patch by Zachary Ware.

- Issue #21629: Fix Argument Clinic's "--converters" feature.

- Add support for ``yield from`` to 2to3.

- Add support for the PEP 465 matrix multiplication operator to 2to3.

- Issue #16047: Fix module exception list and __file__ handling in freeze.
  Patch by Meador Inge.

- Issue #11824: Consider ABI tags in freeze. Patch by Meador Inge.

- Issue #20535: PYTHONWARNING no longer affects the run_tests.py script.
  Patch by Arfrever Frehtes Taifersar Arahesis.

