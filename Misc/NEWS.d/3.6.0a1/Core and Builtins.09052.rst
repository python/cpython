Issue #24802: Avoid buffer overreads when int(), float(), compile(), exec()
and eval() are passed bytes-like objects.  These objects are not
necessarily terminated by a null byte, but the functions assumed they were.