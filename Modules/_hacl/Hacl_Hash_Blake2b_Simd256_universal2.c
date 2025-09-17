// This file isn't part of a standard HACL source tree.
//
// It is required for compatibility with universal2 macOS builds. The code in
// Hacl_Hash_Blake2b_Simd256.c *will* compile on macOS x86_64, but *won't*
// compile on ARM64. However, because universal2 builds are compiled in a
// single pass, autoconf detects that the required compiler features *are*
// available, and tries to compile this file, which then fails because of the
// lack of support on ARM64.
//
// To compensate for this, autoconf will include *this* file instead of
// Hacl_Hash_Blake2b_Simd256.c when compiling for universal. This allows the
// underlying source code of HACL to remain unmodified.
#if !(defined(__APPLE__) && defined(__arm64__))
#include "Hacl_Hash_Blake2b_Simd256.c"
#endif
