- Issue #28808: PyUnicode_CompareWithASCIIString() now never raises exceptions.

- Issue #26754: PyUnicode_FSDecoder() accepted a filename argument encoded as
  an iterable of integers. Now only strings and bytes-like objects are accepted.

