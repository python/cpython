// This file isn't part of a standard HACL source tree.
//
// It is required for compatibility with universal2 macOS builds. The code in
// Hacl_Hash_Blake2s_Simd128.c will compile on macOS ARM64, but performance
// isn't great, so it's disabled. However, because universal2 builds are
// compiled in a single pass, autoconf detects that the required compiler
// features *are* available, and tries to include this file.
//
// To compensate for this, autoconf will include *this* file instead of
// Hacl_Hash_Blake2s_Simd128.c when compiling for universal. This allows the
// underlying source code of HACL to remain unmodified.
#if !(defined(__APPLE__) && defined(__arm64__))
#include "Hacl_Hash_Blake2s_Simd128.c"
#endif
