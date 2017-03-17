Issue #26983: float() now always return an instance of exact float.
The deprecation warning is emitted if __float__ returns an instance of
a strict subclass of float.  In a future versions of Python this can
be an error.