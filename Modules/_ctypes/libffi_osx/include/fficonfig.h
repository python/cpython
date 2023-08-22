/*	Manually created fficonfig.h for Darwin on PowerPC or Intel

	This file is manually generated to do away with the need for autoconf and
	therefore make it easier to cross-compile and build fat binaries.

	NOTE: This file was added by PyObjC.
*/

#ifndef MACOSX
#error "This file is only supported on Mac OS X"
#endif

#if defined(__i386__)
#	define	BYTEORDER 1234
#	undef	HOST_WORDS_BIG_ENDIAN
#	undef	WORDS_BIGENDIAN
#	define	SIZEOF_DOUBLE 8
#	define	HAVE_LONG_DOUBLE 1
#	define	SIZEOF_LONG_DOUBLE 16

#elif defined(__x86_64__)
#	define	BYTEORDER 1234
#	undef	HOST_WORDS_BIG_ENDIAN
#	undef	WORDS_BIGENDIAN
#	define	SIZEOF_DOUBLE 8
#	define	HAVE_LONG_DOUBLE 1
#	define	SIZEOF_LONG_DOUBLE 16

#elif defined(__ppc__)
#	define	BYTEORDER 4321
#	define	HOST_WORDS_BIG_ENDIAN 1
#	define	WORDS_BIGENDIAN 1
#	define	SIZEOF_DOUBLE 8
#	if __GNUC__ >= 4
#		define	HAVE_LONG_DOUBLE 1
#		define	SIZEOF_LONG_DOUBLE 16
#	else
#		undef	HAVE_LONG_DOUBLE
#		define	SIZEOF_LONG_DOUBLE 8
#	endif

#elif defined(__ppc64__)
#	define	BYTEORDER 4321
#	define	HOST_WORDS_BIG_ENDIAN 1
#	define	WORDS_BIGENDIAN 1
#	define	SIZEOF_DOUBLE 8
#	define	HAVE_LONG_DOUBLE 1
#	define	SIZEOF_LONG_DOUBLE 16

#else
#error "Unknown CPU type"
#endif

/*	Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
	systems. This function is required for `alloca.c' support on those systems.	*/
#undef CRAY_STACKSEG_END

/*	Define to 1 if using `alloca.c'. */
/*	#undef C_ALLOCA */

/*	Define to the flags needed for the .section .eh_frame directive. */
#define EH_FRAME_FLAGS "aw"

/*	Define this if you want extra debugging. */
/*	#undef FFI_DEBUG */

/*	Define this is you do not want support for the raw API. */
#define FFI_NO_RAW_API 1

/*	Define this if you do not want support for aggregate types. */
/*	#undef FFI_NO_STRUCTS */

/*	Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/*	Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).	*/
#define HAVE_ALLOCA_H 1

/*	Define if your assembler supports .register. */
/*	#undef HAVE_AS_REGISTER_PSEUDO_OP */

/*	Define if your assembler and linker support unaligned PC relative relocs.	*/
/*	#undef HAVE_AS_SPARC_UA_PCREL */

/*	Define to 1 if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/*	Define if mmap with MAP_ANON(YMOUS) works. */
#define HAVE_MMAP_ANON 1

/*	Define if mmap of /dev/zero works. */
/*	#undef HAVE_MMAP_DEV_ZERO */

/*	Define if read-only mmap of a plain file works. */
#define HAVE_MMAP_FILE 1

/*	Define if .eh_frame sections should be read-only. */
/*	#undef HAVE_RO_EH_FRAME */

/*	Define to 1 if your C compiler doesn't accept -c and -o together. */
/*	#undef NO_MINUS_C_MINUS_O */

/*	Name of package */
#define PACKAGE "libffi"

/*	Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://gcc.gnu.org/bugs.html"

/*	Define to the full name of this package. */
#define PACKAGE_NAME "libffi"

/*	Define to the full name and version of this package. */
#define PACKAGE_STRING "libffi 2.1"

/*	Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libffi"

/*	Define to the version of this package. */
#define PACKAGE_VERSION "2.1"

/*	If using the C implementation of alloca, define if you know the
	direction of stack growth for your system; otherwise it will be
	automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown	*/
/*	#undef STACK_DIRECTION */

/*	Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/*	Define this if you are using Purify and want to suppress spurious messages.	*/
/*	#undef USING_PURIFY	*/

/*	Version number of package */
#define VERSION "2.1-pyobjc"

#ifdef HAVE_HIDDEN_VISIBILITY_ATTRIBUTE
#	ifdef LIBFFI_ASM
#		define FFI_HIDDEN(name) .hidden name
#	else
#		define FFI_HIDDEN __attribute__((visibility ("hidden")))
#	endif
#else
#	ifdef LIBFFI_ASM
#		define FFI_HIDDEN(name)
#	else
#		define FFI_HIDDEN
#	endif
#endif