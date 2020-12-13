/* config.h for compiling liblzma (*not* the whole XZ Utils) with MSVC 2013 */

/* Prefix for symbols exported by tuklib_*.c files */
#define TUKLIB_SYMBOL_PREFIX lzma_

/* How many MiB of RAM to assume if the real amount cannot be determined. */
#define ASSUME_RAM 128

/* Define to 1 if crc32 integrity check is enabled. */
#define HAVE_CHECK_CRC32 1

/* Define to 1 if crc64 integrity check is enabled. */
#define HAVE_CHECK_CRC64 1

/* Define to 1 if sha256 integrity check is enabled. */
#define HAVE_CHECK_SHA256 1

/* Define to 1 if any of HAVE_DECODER_foo have been defined. */
#define HAVE_DECODERS 1

/* Define to 1 if arm decoder is enabled. */
#define HAVE_DECODER_ARM 1

/* Define to 1 if armthumb decoder is enabled. */
#define HAVE_DECODER_ARMTHUMB 1

/* Define to 1 if delta decoder is enabled. */
#define HAVE_DECODER_DELTA 1

/* Define to 1 if ia64 decoder is enabled. */
#define HAVE_DECODER_IA64 1

/* Define to 1 if lzma1 decoder is enabled. */
#define HAVE_DECODER_LZMA1 1

/* Define to 1 if lzma2 decoder is enabled. */
#define HAVE_DECODER_LZMA2 1

/* Define to 1 if powerpc decoder is enabled. */
#define HAVE_DECODER_POWERPC 1

/* Define to 1 if sparc decoder is enabled. */
#define HAVE_DECODER_SPARC 1

/* Define to 1 if x86 decoder is enabled. */
#define HAVE_DECODER_X86 1

/* Define to 1 if any of HAVE_ENCODER_foo have been defined. */
#define HAVE_ENCODERS 1

/* Define to 1 if arm encoder is enabled. */
#define HAVE_ENCODER_ARM 1

/* Define to 1 if armthumb encoder is enabled. */
#define HAVE_ENCODER_ARMTHUMB 1

/* Define to 1 if delta encoder is enabled. */
#define HAVE_ENCODER_DELTA 1

/* Define to 1 if ia64 encoder is enabled. */
#define HAVE_ENCODER_IA64 1

/* Define to 1 if lzma1 encoder is enabled. */
#define HAVE_ENCODER_LZMA1 1

/* Define to 1 if lzma2 encoder is enabled. */
#define HAVE_ENCODER_LZMA2 1

/* Define to 1 if powerpc encoder is enabled. */
#define HAVE_ENCODER_POWERPC 1

/* Define to 1 if sparc encoder is enabled. */
#define HAVE_ENCODER_SPARC 1

/* Define to 1 if x86 encoder is enabled. */
#define HAVE_ENCODER_X86 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 to enable bt2 match finder. */
#define HAVE_MF_BT2 1

/* Define to 1 to enable bt3 match finder. */
#define HAVE_MF_BT3 1

/* Define to 1 to enable bt4 match finder. */
#define HAVE_MF_BT4 1

/* Define to 1 to enable hc3 match finder. */
#define HAVE_MF_HC3 1

/* Define to 1 to enable hc4 match finder. */
#define HAVE_MF_HC4 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#define HAVE_VISIBILITY 0

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

#ifdef _M_IX86
/* Define to 1 when using Windows 95 (and thus XP) compatible threads. This
   avoids use of features that were added in Windows Vista.
   This is used for 32-bit x86 builds for compatibility reasons since it
   makes no measurable difference in performance compared to Vista threads. */
#define MYTHREAD_WIN95 1
#else
/* Define to 1 when using Windows Vista compatible threads. This uses features
   that are not available on Windows XP. */
#define MYTHREAD_VISTA 1
#endif

/* Define to 1 to disable debugging code. */
#define NDEBUG 1

/* Define to the full name of this package. */
#define PACKAGE_NAME "XZ Utils"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://tukaani.org/xz/"

/* The size of `size_t', as computed by sizeof. */
#ifdef _WIN64
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_SIZE_T 4
#endif

/* Define to 1 if the system supports fast unaligned access to 16-bit and
   32-bit integers. */
#define TUKLIB_FAST_UNALIGNED_ACCESS 1
