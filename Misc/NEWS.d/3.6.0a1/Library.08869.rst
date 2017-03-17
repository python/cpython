Issue #25626: Change three zlib functions to accept sizes that fit in
Py_ssize_t, but internally cap those sizes to UINT_MAX.  This resolves a
regression in 3.5 where GzipFile.read() failed to read chunks larger than 2
or 4 GiB.  The change affects the zlib.Decompress.decompress() max_length
parameter, the zlib.decompress() bufsize parameter, and the
zlib.Decompress.flush() length parameter.