Issue #14285: When executing a package with the "python -m package" option,
and package initialization fails, a proper traceback is now reported.  The
"runpy" module now lets exceptions from package initialization pass back to
the caller, rather than raising ImportError.