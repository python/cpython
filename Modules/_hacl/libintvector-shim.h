/* Some older compilers do not let you include, e.g., <immintrin.h> unless you also use e.g. -mavx.
 * This poses a problem for files like Hacl_Streaming_HMAC.c, which *do* need a vec128 type defined
 * in scope (so that their state type can be defined), but which *must not* be compiled with -mavx
 * (otherwise it would result in illegal instruction errors on machines without -mavx).
 *
 * Rather than add another layer of hacks with `[@ CAbstractStruct ]` and another internal
 * abstraction barrier between Hacl_Streaming_HMAC and optimized files, we simply define the two
 * relevant types to be an incomplete struct (we could also define them to be void). Two
 * consequences:
 * - these types cannot be constructed, which enforces the abstraction barrier -- you can define the
 *   Hacl_Streaming_HMAC state type but only if it uses vec128 behind a pointer, which is exactly
 *   the intent, and
 * - the absence of actual operations over these types once again enforces that a client module
 *   which relies on this header only does type definitions, nothing more, and leaves the actual
 *   operations to the relevant module (e.g. Hacl_Hash_Blake2b_256) -- that one will include
 *   libintvector.h
 *
 * See https://github.com/python/cpython/issues/130213 for a detailed description of the issue
 * including actual problematic compilers.
 *
 * Currently, only Hacl_Streaming_HMAC is crafted carefully enough to do this.
 */

typedef struct __vec128 Lib_IntVector_Intrinsics_vec128;
typedef struct __vec256 Lib_IntVector_Intrinsics_vec256;

/* If a module includes this header, it almost certainly has #ifdef HACL_CAN_COMPILE_XXX all over
 * the place, so bring that into scope too via config.h */
#if defined(__has_include)
#if __has_include("config.h")
#include "config.h"
#endif
#endif

#define HACL_INTRINSICS_SHIMMED
