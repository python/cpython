Issue #26754: Some functions (compile() etc) accepted a filename argument
encoded as an iterable of integers. Now only strings and byte-like objects
are accepted.