- Issue #26799: Fix python-gdb.py: don't get C types once when the Python code
  is loaded, but get C types on demand. The C types can change if
  python-gdb.py is loaded before the Python executable. Patch written by Thomas
  Ilsche.

- Issue #26271: Fix the Freeze tool to properly use flags passed through
  configure. Patch by Daniel Shaulov.

- Issue #26489: Add dictionary unpacking support to Tools/parser/unparse.py.
  Patch by Guo Ci Teo.

- Issue #26316: Fix variable name typo in Argument Clinic.

