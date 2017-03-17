Issue #26754: PyUnicode_FSDecoder() accepted a filename argument encoded as
an iterable of integers. Now only strings and byte-like objects are accepted.