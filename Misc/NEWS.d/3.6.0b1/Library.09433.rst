Issue #28485: Always raise ValueError for negative
compileall.compile_dir(workers=...) parameter, even when multithreading is
unavailable.