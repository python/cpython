/*
 * PowerPC AltiVec bool keyword fix for HACL* BLAKE2 SIMD128.
 *
 * When GCC compiles with -maltivec, it makes "bool" a keyword meaning
 * "__vector __bool int" (a 128-bit vector type). This conflicts with
 * C99/C11 stdbool.h where bool means _Bool (a scalar type).
 *
 * The -Dbool=_Bool workaround does NOT work because altivec.h re-enables
 * the keyword after the macro is defined. Instead, this header includes
 * altivec.h first, then undefines the bool/true/false keywords and
 * restores the C99 scalar definitions. Vector boolean types remain
 * accessible via the explicit __vector __bool syntax.
 *
 * This header is force-included (-include) before HACL SIMD128 sources
 * via LIBHACL_SIMD128_FLAGS in configure.ac.
 */
#ifndef PPC_ALTIVEC_BOOL_FIX_H
#define PPC_ALTIVEC_BOOL_FIX_H

#if defined(__ALTIVEC__) || defined(__VSX__)
#include <altivec.h>

#undef bool
#undef true
#undef false

#define bool _Bool
#define true 1
#define false 0
#endif /* __ALTIVEC__ || __VSX__ */

#endif /* PPC_ALTIVEC_BOOL_FIX_H */
