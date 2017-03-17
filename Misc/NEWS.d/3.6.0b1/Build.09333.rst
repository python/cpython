Issue #27983: Cause lack of llvm-profdata tool when using clang as
required for PGO linking to be a configure time error rather than
make time when --with-optimizations is enabled.  Also improve our
ability to find the llvm-profdata tool on MacOS and some Linuxes.