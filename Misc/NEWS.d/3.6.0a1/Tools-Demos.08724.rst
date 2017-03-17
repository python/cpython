Issue #26799: Fix python-gdb.py: don't get C types once when the Python code
is loaded, but get C types on demand. The C types can change if
python-gdb.py is loaded before the Python executable. Patch written by Thomas
Ilsche.