Issue #25285: regrtest now uses subprocesses when the -j1 command line option
is used: each test file runs in a fresh child process. Before, the -j1 option
was ignored.