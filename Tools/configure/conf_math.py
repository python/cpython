"""conf_math — Math, floating-point, semaphores, wchar.

Handles --with-libm and --with-libc; checks GCC inline assembler
and floating-point features (x64 asm, x87 control word, mc68881 fpcr,
double rounding, float word endianness); checks C99 math functions;
probes POSIX named and unnamed semaphores; handles --enable-big-digits
for Python long integers; and checks wchar_t size and signedness
(including Solaris workarounds).
"""

from __future__ import annotations

import pyconf

WITH_LIBM = pyconf.arg_with(
    "libm",
    help="override libm math library to STRING (default is system-dependent)",
)
WITH_LIBC = pyconf.arg_with(
    "libc",
    help="override libc C library to STRING (default is system-dependent)",
)
ENABLE_BIG_DIGITS = pyconf.arg_enable(
    "big-digits",
    help="use big digits (30 or 15 bits) for Python longs (default is 30)",
)


def check_math_library(v):
    # ---------------------------------------------------------------------------
    # Math library: --with-libm and --with-libc
    # ---------------------------------------------------------------------------

    v.export("LIBM")  # registered for Makefile substitution
    # Linux requires this for correct floating-point operations
    # (Note: AC_CHECK_FUNC with custom action, no HAVE_ define)

    if v.ac_sys_system != "Darwin":
        v.LIBM = "-lm"
    else:
        v.LIBM = ""

    pyconf.checking("for --with-libm=STRING")
    with_libm = WITH_LIBM.value
    if with_libm is not None:
        if with_libm == "no":
            v.LIBM = ""
            pyconf.result("force LIBM empty")
        elif with_libm != "yes":
            v.LIBM = with_libm
            pyconf.result(f'set LIBM="{with_libm}"')
        else:
            pyconf.error("proper usage is --with-libm=STRING")
    else:
        pyconf.result(f'default LIBM="{v.LIBM}"')

    v.export("LIBC")

    pyconf.checking("for --with-libc=STRING")
    with_libc = WITH_LIBC.value
    if with_libc is not None:
        if with_libc == "no":
            v.LIBC = ""
            pyconf.result("force LIBC empty")
        elif with_libc != "yes":
            v.LIBC = with_libc
            pyconf.result(f'set LIBC="{with_libc}"')
        else:
            pyconf.error("proper usage is --with-libc=STRING")
    else:
        pyconf.result(f'default LIBC="{v.LIBC}"')

    # Linux: __fpu_control (AC_CHECK_FUNC with custom action → no HAVE_ define)
    if not pyconf.check_func("__fpu_control", autodefine=False):
        pyconf.check_lib("ieee", "__fpu_control")


def _define_float_big():
    pyconf.define(
        "DOUBLE_IS_BIG_ENDIAN_IEEE754",
        1,
        "Define if C doubles are 64-bit IEEE 754 binary format, "
        "stored with the most significant byte first",
    )


def _define_float_little():
    pyconf.define(
        "DOUBLE_IS_LITTLE_ENDIAN_IEEE754",
        1,
        "Define if C doubles are 64-bit IEEE 754 binary format, "
        "stored with the least significant byte first",
    )


def _define_float_unknown():
    # Fallback for platforms where float endianness detection fails.
    # Modern ARM (including iOS) uses little-endian IEEE 754 doubles.
    if pyconf.fnmatch_any(pyconf.host_cpu, ["arm*", "aarch64"]):
        _define_float_little()
    else:
        pyconf.error(
            "Unknown float word ordering. You need to manually "
            "preset ax_cv_c_float_words_bigendian=no (or yes) "
            "according to your system."
        )


def check_gcc_asm_and_floating_point(v):
    # ---------------------------------------------------------------------------
    # GCC inline assembler checks
    # ---------------------------------------------------------------------------

    # Check for x64 gcc inline assembler
    pyconf.checking("for x64 gcc inline assembler")
    ac_cv_gcc_asm_for_x64 = pyconf.link_check(
        body='__asm__ __volatile__ ("movq %rcx, %rax");',
        cache_var="ac_cv_gcc_asm_for_x64",
    )
    pyconf.result(ac_cv_gcc_asm_for_x64)
    if ac_cv_gcc_asm_for_x64:
        pyconf.define(
            "HAVE_GCC_ASM_FOR_X64",
            1,
            "Define if we can use x64 gcc inline assembler",
        )

    # ---------------------------------------------------------------------------
    # Floating-point properties
    # ---------------------------------------------------------------------------

    # Check for various properties of floating point
    pyconf.ax_c_float_words_bigendian(
        on_big=_define_float_big,
        on_little=_define_float_little,
        on_unknown=_define_float_unknown,
    )

    # The short float repr introduced in Python 3.1 requires the
    # correctly-rounded string <-> double conversion functions from
    # Python/dtoa.c, which in turn require that the FPU uses 53-bit
    # rounding; this is a problem on x86, where the x87 FPU has a default
    # rounding precision of 64 bits. For gcc/x86, we can fix this by
    # using inline assembler to get and set the x87 FPU control word.
    #
    # This inline assembler syntax may also work for suncc and icc,
    # so we try it on all platforms.
    #
    # x87 control word (gcc inline asm)
    pyconf.checking(
        "whether we can use gcc inline assembler to get and set x87 control word"
    )
    ac_cv_gcc_asm_for_x87 = pyconf.link_check(
        body="""
      unsigned short cw;
      __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));
      __asm__ __volatile__ ("fldcw %0" : : "m" (cw));
    """,
        cache_var="ac_cv_gcc_asm_for_x87",
    )
    pyconf.result(ac_cv_gcc_asm_for_x87)
    if ac_cv_gcc_asm_for_x87:
        pyconf.define(
            "HAVE_GCC_ASM_FOR_X87",
            1,
            "Define if we can use gcc inline assembler to get and set x87 control word",
        )

    # mc68881 fpcr
    pyconf.checking(
        "whether we can use gcc inline assembler to get and set mc68881 fpcr"
    )
    ac_cv_gcc_asm_for_mc68881 = pyconf.link_check(
        body="""
      unsigned int fpcr;
      __asm__ __volatile__ ("fmove.l %%fpcr,%0" : "=dm" (fpcr));
      __asm__ __volatile__ ("fmove.l %0,%%fpcr" : : "dm" (fpcr));
    """,
        cache_var="ac_cv_gcc_asm_for_mc68881",
    )
    pyconf.result(ac_cv_gcc_asm_for_mc68881)
    if ac_cv_gcc_asm_for_mc68881:
        pyconf.define(
            "HAVE_GCC_ASM_FOR_MC68881",
            1,
            "Define if we can use gcc inline assembler to get and set mc68881 fpcr",
        )

    # Detect whether system arithmetic is subject to x87-style double
    # rounding issues. The result of this test has little meaning on non
    # IEEE 754 platforms. On IEEE 754, test should return 1 if rounding
    # mode is round-to-nearest and double rounding issues are present, and
    # 0 otherwise. See https://github.com/python/cpython/issues/47186 for more info.
    pyconf.checking("for x87-style double rounding")
    saved_cc = v.CC
    # $BASECFLAGS may affect the result
    v.CC = f"{v.CC} {v.BASECFLAGS}".strip()
    ac_cv_x87_double_rounding = pyconf.run_check(
        r"""
    #include <stdlib.h>
    #include <math.h>
    int main(void) {
        volatile double x, y, z;
        x = 0.99999999999999989; /* 1-2**-53 */
        y = 1./x;
        if (y != 1.)
            exit(0);
        x = 1e16;
        y = 2.99999;
        z = x + y;
        if (z != 1e16+4.)
            exit(0);
        exit(1);
    }
    """,
        cache_var="ac_cv_x87_double_rounding",
        on_success_return=False,
        on_failure_return=True,
        cross_compiling_default=False,
    )
    v.CC = saved_cc
    pyconf.result(ac_cv_x87_double_rounding)
    if ac_cv_x87_double_rounding:
        pyconf.define(
            "X87_DOUBLE_ROUNDING",
            1,
            "Define if arithmetic is subject to x87-style double rounding issue",
        )


def check_c99_math(v):
    # ---------------------------------------------------------------------------
    # Check for mathematical functions (required)
    # ---------------------------------------------------------------------------

    libs_save = v.LIBS
    v.LIBS = f"{v.LIBS} {v.LIBM}".strip()
    for mfunc in (
        "acosh",
        "asinh",
        "atanh",
        "erf",
        "erfc",
        "expm1",
        "log1p",
        "log2",
    ):
        if not pyconf.check_func(mfunc):
            pyconf.error("Python requires C99 compatible libm")
    v.LIBS = libs_save


def check_big_digits(v):
    # ---------------------------------------------------------------------------
    # Python long digit size: --enable-big-digits
    # ---------------------------------------------------------------------------

    pyconf.checking("digit size for Python's longs")
    big_digits_raw = ENABLE_BIG_DIGITS.value
    if big_digits_raw is not None:
        if big_digits_raw == "yes":
            enable_big_digits = "30"
        elif big_digits_raw == "no":
            enable_big_digits = "15"
        elif big_digits_raw in ("15", "30"):
            enable_big_digits = big_digits_raw
        else:
            pyconf.error(
                f"bad value {big_digits_raw} for --enable-big-digits; "
                "value should be 15 or 30"
            )
            enable_big_digits = (
                big_digits_raw  # unreachable but satisfies linter
            )
        pyconf.result(enable_big_digits)
        pyconf.define_unquoted(
            "PYLONG_BITS_IN_DIGIT",
            enable_big_digits,
            "Define as the preferred size in bits of long digits",
        )
    else:
        pyconf.result("no value specified")


def check_wchar(v):
    # ---------------------------------------------------------------------------
    # wchar.h and wchar_t properties
    # ---------------------------------------------------------------------------

    # Check for wchar.h
    if pyconf.check_header("wchar.h"):
        pyconf.define(
            "HAVE_WCHAR_H",
            1,
            "Define if the compiler provides a wchar.h header file.",
        )
        v.wchar_h = True
    else:
        v.wchar_h = False

    if v.wchar_h:
        # Determine wchar_t size
        pyconf.check_sizeof("wchar_t", default=4, includes=["wchar.h"])

        # Check whether wchar_t is signed or not
        pyconf.checking("whether wchar_t is signed")
        ac_cv_wchar_t_signed = pyconf.run_check(
            r"""
    #include <wchar.h>
    int main()
    {
        return ((((wchar_t) -1) < ((wchar_t) 0)) ? 0 : 1);
    }
    """,
            cache_var="ac_cv_wchar_t_signed",
            cross_compiling_default=True,
        )
        pyconf.result(ac_cv_wchar_t_signed)

        pyconf.checking("whether wchar_t is usable")
        # wchar_t is only usable if it maps to an unsigned type
        wchar_size = int(v.ac_cv_sizeof_wchar_t or "0")
        wchar_unsigned = not ac_cv_wchar_t_signed
        if wchar_size >= 2 and wchar_unsigned:
            pyconf.define(
                "HAVE_USABLE_WCHAR_T",
                1,
                "Define if you have a useable wchar_t type defined in wchar.h; "
                "useable means wchar_t must be an unsigned type with at least "
                "16 bits. (see Include/unicodeobject.h).",
            )
            pyconf.result("yes")
        else:
            pyconf.result("no")

    # Oracle Solaris: wchar_t is non-Unicode in non-Unicode locales
    # bpo-43667: In Oracle Solaris, the internal form of wchar_t in
    # non-Unicode locales is not Unicode and hence cannot be used directly.
    # https://docs.oracle.com/cd/E37838_01/html/E61053/gmwke.html
    if v.ac_sys_system == "SunOS":
        if pyconf.path_is_file("/etc/os-release"):
            os_release_content = pyconf.read_file("/etc/os-release")
            os_name = ""
            for line in os_release_content.splitlines():
                if line.startswith("NAME="):
                    os_name = line.split("=", 1)[1].strip().strip('"')
                    break
            if os_name == "Oracle Solaris":
                pyconf.define(
                    "HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION",
                    1,
                    "Define if the internal form of wchar_t in non-Unicode locales "
                    "is not Unicode.",
                )


WITH_SYSTEM_LIBMPDEC = pyconf.arg_with(
    "system-libmpdec",
    help="build _decimal using installed mpdecimal library (default is yes)",
    default="yes",
    var="with_system_libmpdec",
)
WITH_DECIMAL_CONTEXTVAR = pyconf.arg_with(
    "decimal-contextvar",
    help="build _decimal using coroutine-local rather than thread-local context (default is yes)",
    default="yes",
)


def _use_bundled_libmpdec(v):
    v.LIBMPDEC_CFLAGS = r"-I$(srcdir)/Modules/_decimal/libmpdec"
    v.LIBMPDEC_LIBS = r"-lm $(LIBMPDEC_A)"
    v.LIBMPDEC_INTERNAL = r"$(LIBMPDEC_HEADERS) $(LIBMPDEC_A)"
    v.have_mpdec = True


def detect_libmpdec(v):
    """Detect libmpdec and configure decimal module."""
    v.have_mpdec = False
    v.LIBMPDEC_CFLAGS = ""
    v.LIBMPDEC_LIBS = ""
    v.LIBMPDEC_INTERNAL = ""

    with_system_libmpdec = WITH_SYSTEM_LIBMPDEC.process_value(v)

    if with_system_libmpdec == "no":
        _use_bundled_libmpdec(v)
    else:
        # Try pkg-config first
        pkg = pyconf.find_prog("pkg-config")
        found_pkg = False
        if pkg:
            if pyconf.cmd([pkg, "--atleast-version=2.5.0", "libmpdec"]):
                status, output = pyconf.cmd_status(
                    [pkg, "--cflags", "--libs", "libmpdec"]
                )
                if status == 0:
                    parts = output.split()
                    cflags_parts = []
                    libs_parts = []
                    for p in parts:
                        if p.startswith("-I") or p.startswith("-D"):
                            cflags_parts.append(p)
                        else:
                            libs_parts.append(p)
                    v.LIBMPDEC_CFLAGS = " ".join(cflags_parts)
                    v.LIBMPDEC_LIBS = " ".join(libs_parts)
                    found_pkg = True
        if not found_pkg:
            v.LIBMPDEC_CFLAGS = ""
            v.LIBMPDEC_LIBS = "-lmpdec -lm"
            v.LIBMPDEC_INTERNAL = ""

        # Verify linkage (mpd_version symbol)
        if pyconf.link_check(
            preamble="""\
#include <mpdecimal.h>
#if MPD_VERSION_HEX < 0x02050000
#  error "mpdecimal 2.5.0 or higher required"
#endif
""",
            body="const char *x = mpd_version();",
            extra_cflags=v.LIBMPDEC_CFLAGS,
            extra_libs=v.LIBMPDEC_LIBS,
        ):
            v.have_mpdec = True
        else:
            v.have_mpdec = False

    # Disable forced inlining in debug builds, see GH-94847
    with_pydebug = v.Py_DEBUG
    if with_pydebug:
        v.LIBMPDEC_CFLAGS += " -DTEST_COVERAGE"

    # Check whether _decimal should use a coroutine-local or thread-local context
    # --with-decimal-contextvar
    with_decimal_contextvar = WITH_DECIMAL_CONTEXTVAR.process_value(None)
    if with_decimal_contextvar != "no":
        pyconf.define(
            "WITH_DECIMAL_CONTEXTVAR",
            1,
            "Define if you want build the _decimal module using a "
            "coroutine-local rather than a thread-local context",
        )

    if with_system_libmpdec == "no":
        # Determine bundled libmpdec machine flavor
        pyconf.checking("decimal libmpdec machine")
        if v.ac_sys_system.startswith("Darwin"):
            # universal here means: build libmpdec with the same arch options
            # the python interpreter was built with
            libmpdec_machine = "universal"
        else:
            ac_cv_sizeof_size_t = int(v.ac_cv_sizeof_size_t or 8)
            ac_cv_gcc_asm_for_x64 = v.ac_cv_gcc_asm_for_x64
            ac_cv_type___uint128_t = v.ac_cv_type___uint128_t
            ac_cv_gcc_asm_for_x87 = v.ac_cv_gcc_asm_for_x87
            libmpdec_system = (
                "sunos" if v.ac_sys_system.startswith("SunOS") else "other"
            )
            libmpdec_machine = "unknown"
            if ac_cv_sizeof_size_t == 8:
                if ac_cv_gcc_asm_for_x64:
                    libmpdec_machine = "x64"
                elif ac_cv_type___uint128_t:
                    libmpdec_machine = "uint128"
                else:
                    libmpdec_machine = "ansi64"
            elif ac_cv_sizeof_size_t == 4:
                if ac_cv_gcc_asm_for_x87 and libmpdec_system != "sunos":
                    cc_name = v.ac_cv_cc_name
                    if "gcc" in cc_name or "clang" in cc_name:
                        libmpdec_machine = "ppro"
                    else:
                        libmpdec_machine = "ansi32"
                else:
                    libmpdec_machine = "ansi32"
        pyconf.result(libmpdec_machine)

        mflags = {
            "x64": " -DCONFIG_64=1 -DASM=1",
            "uint128": " -DCONFIG_64=1 -DANSI=1 -DHAVE_UINT128_T=1",
            "ansi64": " -DCONFIG_64=1 -DANSI=1",
            "ppro": " -DCONFIG_32=1 -DANSI=1 -DASM=1 -Wno-unknown-pragmas",
            "ansi32": " -DCONFIG_32=1 -DANSI=1",
            "ansi-legacy": " -DCONFIG_32=1 -DANSI=1 -DLEGACY_COMPILER=1",
            "universal": " -DUNIVERSAL=1",
        }
        if libmpdec_machine in mflags:
            v.LIBMPDEC_CFLAGS += mflags[libmpdec_machine]
        elif libmpdec_machine == "unknown":
            pyconf.fatal("_decimal: unsupported architecture")

        if v.have_ipa_pure_const_bug:
            # Some versions of gcc miscompile inline asm:
            # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=46491
            # https://gcc.gnu.org/ml/gcc/2010-11/msg00366.html
            v.LIBMPDEC_CFLAGS += " -fno-ipa-pure-const"

        if v.have_glibc_memmove_bug:
            # _FORTIFY_SOURCE wrappers for memmove and bcopy are incorrect:
            # https://sourceware.org/ml/libc-alpha/2010-12/msg00009.html
            v.LIBMPDEC_CFLAGS += " -U_FORTIFY_SOURCE"

    v.export("LIBMPDEC_CFLAGS")
    v.export("LIBMPDEC_LIBS")
    v.export("LIBMPDEC_INTERNAL")
