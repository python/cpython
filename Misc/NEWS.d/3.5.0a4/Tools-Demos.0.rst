- Issue #18128: pygettext now uses standard +NNNN format in the
  POT-Creation-Date header.

- Issue #23935: Argument Clinic's understanding of format units
  accepting bytes, bytearrays, and buffers is now consistent with
  both the documentation and the implementation.

- Issue #23944: Argument Clinic now wraps long impl prototypes at column 78.

- Issue #20586: Argument Clinic now ensures that functions without docstrings
  have signatures.

- Issue #23492: Argument Clinic now generates argument parsing code with
  PyArg_Parse instead of PyArg_ParseTuple if possible.

- Issue #23500: Argument Clinic is now smarter about generating the "#ifndef"
  (empty) definition of the methoddef macro: it's only generated once, even
  if Argument Clinic processes the same symbol multiple times, and it's emitted
  at the end of all processing rather than immediately after the first use.

