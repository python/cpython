Issue #27512: Fix a segfault when os.fspath() called an __fspath__() method
that raised an exception. Patch by Xiang Zhang.