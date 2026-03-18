"""conf_compiler — Compiler detection, platform triplet, stack direction, PEP 11 tier.

Decides whether to define _XOPEN_SOURCE and related POSIX macros;
runs AC_PROG_CC (find_compiler), discovers GREP/SED/EGREP,
CC_BASENAME; selects C++ compiler; detects PLATFORM_TRIPLET,
MULTIARCH, SOABI_PLATFORM; determines stack growth direction;
classifies PEP 11 support tier; and checks compiler bugs (glibc
memmove, ipa-pure-const).
"""

from __future__ import annotations

import os

import pyconf
import conf_macos

WITH_STRICT_OVERFLOW = pyconf.arg_with(
    "strict-overflow",
    help="if 'yes', add -fstrict-overflow to CFLAGS, else add -fno-strict-overflow (default is no)",
)
ENABLE_SAFETY = pyconf.arg_enable(
    "safety",
    help="enable usage of the security compiler options with no performance overhead",
    false_is="no",
)
ENABLE_SLOWER_SAFETY = pyconf.arg_enable(
    "slower-safety",
    help="enable usage of the security compiler options with performance overhead",
    false_is="no",
)
WITH_COMPUTED_GOTOS = pyconf.arg_with(
    "computed-gotos",
    help="enable computed gotos in evaluation loop "
    "(enabled by default on supported compilers)",
)


def setup_compiler_detection(v):
    """Run AC_PROG_CC, find GREP/SED/EGREP, system extensions, CC_BASENAME."""
    pyconf.find_compiler(cc=v.CC or None, cpp=v.CPP or None)
    v.export("CC", pyconf.CC)
    v.export("CPP", pyconf.CPP)
    v.export("GCC", pyconf.GCC)
    v.ac_cv_cc_name = pyconf.ac_cv_cc_name
    v.ac_cv_gcc_compat = pyconf.ac_cv_gcc_compat
    v.ac_cv_prog_cc_g = pyconf.ac_cv_prog_cc_g

    v.export("GREP", pyconf.check_prog("grep", default="grep"))
    v.export("SED", pyconf.check_prog("sed", default="sed"))
    v.export("EGREP", pyconf.check_prog("egrep", default="egrep"))

    pyconf.use_system_extensions()

    v.export("CC_BASENAME", pyconf.basename(v.CC.split()[0]) if v.CC else "")

    if v.CC_BASENAME == "mpicc":
        v.ac_cv_cc_name = "mpicc"


def setup_cxx(v):
    """Select C++ compiler."""
    v.export("CXX")
    preset_cxx = v.CXX
    if not v.CXX:
        cxx_map = {
            "gcc": "g++",
            "cc": "c++",
            "clang": "clang++",
            "icc": "icpc",
        }
        cxx_candidate = cxx_map.get(v.ac_cv_cc_name, "")
        if cxx_candidate and pyconf.find_prog(cxx_candidate):
            v.CXX = cxx_candidate
        if not v.CXX:
            for candidate in [
                v.CCC,
                "c++",
                "g++",
                "gcc",
                "CC",
                "cxx",
                "cc++",
                "cl",
            ]:
                if candidate and pyconf.find_prog(candidate):
                    v.CXX = candidate
                    break

    if preset_cxx and preset_cxx != v.CXX:
        pyconf.notice(
            f'\n  By default, distutils will build C++ extension modules with "{v.CXX}".\n'
            f"  If this is not intended, then set CXX on the configure command line.\n"
        )


def setup_stack_direction(v):
    """Determine stack growth direction."""
    v._Py_STACK_GROWS_DOWN = 0 if v.host.startswith("hppa") else 1
    pyconf.define(
        "_Py_STACK_GROWS_DOWN",
        v._Py_STACK_GROWS_DOWN,
        "Define to 1 if the machine stack grows down (default); 0 if it grows up.",
    )
    v.export("_Py_STACK_GROWS_DOWN")


def check_compiler_bugs(v):
    # ---------------------------------------------------------------------------
    # -O2 availability (used by memmove bug and ipa-pure-const checks)
    # ---------------------------------------------------------------------------

    have_o2 = pyconf.compile_check(description="for -O2", extra_cflags="-O2")

    # ---------------------------------------------------------------------------
    # glibc _FORTIFY_SOURCE / memmove bug
    # ---------------------------------------------------------------------------

    memmove_cflags = "-O2 -D_FORTIFY_SOURCE=2" if have_o2 else ""
    memmove_result = pyconf.run_check(
        "for glibc _FORTIFY_SOURCE/memmove bug",
        """
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void foo(void *p, void *q) { memmove(p, q, 19); }
int main(void) {
  char a[32] = "123456789000000000";
  foo(&a[9], a);
  if (strcmp(a, "123456789123456789000000000") != 0)
    return 1;
  foo(a, &a[9]);
  if (strcmp(a, "123456789000000000") != 0)
    return 1;
  return 0;
}
""",
        extra_cflags=memmove_cflags,
        on_success_return=False,
        on_failure_return=True,
        cross_compiling_result="undefined",
    )
    if memmove_result is True:
        pyconf.define(
            "HAVE_GLIBC_MEMMOVE_BUG",
            1,
            "Define if glibc has incorrect _FORTIFY_SOURCE wrappers "
            "for memmove and bcopy.",
        )

    # ---------------------------------------------------------------------------
    # GCC ipa-pure-const inline asm bug
    # ---------------------------------------------------------------------------

    if v.ac_cv_gcc_asm_for_x87 is True:
        if v.ac_cv_cc_name == "gcc":
            ipa_result = pyconf.run_check(
                "for gcc ipa-pure-const bug",
                r"""
__attribute__((noinline)) int
foo(int *p) {
  int r;
  asm ( "movl $6, (%1)\n\t"
        "xorl %0, %0\n\t"
        : "=r" (r) : "r" (p) : "memory"
  );
  return r;
}
int main(void) {
  int p = 8;
  if ((foo(&p) ? : p) != 6)
    return 1;
  return 0;
}
""",
                extra_cflags="-O2",
                on_success_return=False,
                on_failure_return=True,
                cross_compiling_result="undefined",
            )
            if ipa_result is True:
                pyconf.define(
                    "HAVE_IPA_PURE_CONST_BUG",
                    1,
                    "Define if gcc has the ipa-pure-const bug.",
                )


def check_sign_extension_and_getc(v):
    # ---------------------------------------------------------------------------
    # Sign-extension / right-shift
    # ---------------------------------------------------------------------------

    pyconf.checking("whether right shift extends the sign bit")
    ac_cv_rshift_extends_sign = pyconf.run_check(
        "int main(void) { return (((-1)>>3 == -1) ? 0 : 1); }",
        cache_var="ac_cv_rshift_extends_sign",
        cross_compiling_default=True,
    )
    pyconf.result(ac_cv_rshift_extends_sign)
    if not ac_cv_rshift_extends_sign:
        pyconf.define(
            "SIGNED_RIGHT_SHIFT_ZERO_FILLS",
            1,
            "Define if i>>j for signed int i does not extend the sign bit when i < 0",
        )

    # ---------------------------------------------------------------------------
    # getc_unlocked and friends
    # ---------------------------------------------------------------------------

    pyconf.checking("for getc_unlocked() and friends")
    ac_cv_have_getc_unlocked = pyconf.link_check(
        preamble="#include <stdio.h>",
        body=r"""
        FILE *f = fopen("/dev/null", "r");
        flockfile(f);
        getc_unlocked(f);
        funlockfile(f);
    """,
        cache_var="ac_cv_have_getc_unlocked",
    )
    pyconf.result(ac_cv_have_getc_unlocked)
    if ac_cv_have_getc_unlocked:
        pyconf.define(
            "HAVE_GETC_UNLOCKED",
            1,
            "Define this if you have flockfile(), getc_unlocked(), and funlockfile()",
        )


def setup_strict_overflow(v):
    """Handle --with-strict-overflow and -Og support."""
    supports_fstrict_overflow = pyconf.check_compile_flag(
        "-fstrict-overflow -fno-strict-overflow"
    )
    v.export(
        "STRICT_OVERFLOW_CFLAGS",
        "-fstrict-overflow" if supports_fstrict_overflow else "",
    )
    v.export(
        "NO_STRICT_OVERFLOW_CFLAGS",
        "-fno-strict-overflow" if supports_fstrict_overflow else "",
    )

    pyconf.checking("for --with-strict-overflow")
    v.with_strict_overflow = WITH_STRICT_OVERFLOW.is_yes()
    if WITH_STRICT_OVERFLOW.given and not supports_fstrict_overflow:
        pyconf.warn(
            "--with-strict-overflow=yes requires a compiler that supports -fstrict-overflow"
        )
    pyconf.result(WITH_STRICT_OVERFLOW.value_or("no"))


def setup_opt_and_debug_cflags(v):
    """-Og support, OPT, CFLAGS_ALIASING."""
    supports_og = pyconf.check_compile_flag("-Og")

    v.PYDEBUG_CFLAGS = "-Og" if supports_og else "-O0"
    if v.ac_sys_system == "WASI":
        v.PYDEBUG_CFLAGS = "-O3"
    v.export("PYDEBUG_CFLAGS")

    v.export("OPT")
    v.export("CFLAGS_ALIASING")
    opt_env = os.environ.get("OPT")
    v.OPT = opt_env if opt_env is not None else ""
    v.CFLAGS_ALIASING = ""

    if opt_env is None:
        if v.GCC:
            if v.ac_cv_cc_name != "clang":
                v.CFLAGS_ALIASING = "-fno-strict-aliasing"
            if v.ac_cv_prog_cc_g:
                if v.Py_DEBUG is True:
                    v.OPT = f"-g {v.PYDEBUG_CFLAGS} -Wall"
                else:
                    v.OPT = "-g -O3 -Wall"
            else:
                v.OPT = "-O3 -Wall"
            if v.ac_sys_system.startswith("SCO_SV"):
                v.OPT += " -m486 -DSCO5"
        else:
            v.OPT = "-O"


def setup_basecflags_overflow(v):
    """Apply strict-overflow flag to BASECFLAGS."""
    if v.with_strict_overflow:
        v.BASECFLAGS += f" {v.STRICT_OVERFLOW_CFLAGS}"
    else:
        v.BASECFLAGS += f" {v.NO_STRICT_OVERFLOW_CFLAGS}"


def setup_safety_options(v):
    """Handle --enable-safety and --enable-slower-safety."""
    if ENABLE_SAFETY.process_bool():
        for flag in [
            "-fstack-protector-strong",
            "-Wtrampolines",
            "-Wimplicit-fallthrough",
            "-Werror=format-security",
            "-Wbidi-chars=any",
            "-Wall",
        ]:
            if pyconf.check_compile_flag(flag, extra_flags=["-Werror"]):
                v.CFLAGS_NODIST += f" {flag}"
            else:
                pyconf.warn(f"{flag} not supported")

    if ENABLE_SLOWER_SAFETY.process_bool():
        if pyconf.check_compile_flag(
            "-D_FORTIFY_SOURCE=3", extra_flags=["-Werror"]
        ):
            v.CFLAGS_NODIST += " -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3"
        else:
            pyconf.warn("-D_FORTIFY_SOURCE=3 not supported")


def setup_gcc_warnings(v):
    """GCC-compatible compiler warnings, aliasing, visibility, and flag tweaks."""
    if not v.ac_cv_gcc_compat:
        if v.ac_sys_system.startswith(("OpenUNIX", "UnixWare")):
            v.BASECFLAGS += " -K pentium,host,inline,loop_unroll,alloca"
        elif v.ac_sys_system.startswith("SCO_SV"):
            v.BASECFLAGS += " -belf -Ki486 -DSCO5"
        return

    v.CFLAGS_NODIST += " -std=c11"

    if pyconf.check_compile_flag("-Wextra", extra_flags=["-Werror"]):
        v.CFLAGS_NODIST += " -Wextra"

    # Check strict-aliasing behaviour
    cc_with_fna = f"{v.CC} -fno-strict-aliasing"
    cc_with_fsa = f"{v.CC} -fstrict-aliasing"
    fna_supported = pyconf.compile_check(
        cc=cc_with_fna, program="int main(void){return 0;}"
    )
    if fna_supported:
        strict_aliasing_fires = not pyconf.compile_check(
            cc=cc_with_fsa,
            extra_flags=["-Werror", "-Wstrict-aliasing"],
            preamble="void f(int **x) {}",
            body="double *x; f((int **) &x);",
        )
    else:
        strict_aliasing_fires = False
    if strict_aliasing_fires:
        v.BASECFLAGS += " -fno-strict-aliasing"

    if v.ac_cv_cc_name == "icc":
        if pyconf.check_compile_flag(
            "-Wunused-result", extra_flags=["-Werror"]
        ):
            v.BASECFLAGS += " -Wno-unused-result"
            v.CFLAGS_NODIST += " -Wno-unused-result"

    if pyconf.check_compile_flag(
        "-Wno-unused-parameter", extra_flags=["-Werror"]
    ):
        v.CFLAGS_NODIST += " -Wno-unused-parameter"

    if pyconf.check_compile_flag(
        "-Wno-missing-field-initializers", extra_flags=["-Werror"]
    ):
        v.CFLAGS_NODIST += " -Wno-missing-field-initializers"

    if pyconf.check_compile_flag("-Wsign-compare", extra_flags=["-Werror"]):
        v.BASECFLAGS += " -Wsign-compare"

    if pyconf.check_compile_flag(
        "-Wunreachable-code", extra_flags=["-Werror"]
    ):
        cc_ver = pyconf.cmd_output([v.CC, "--version"])
        if v.Py_DEBUG is not True and "Free Software Foundation" not in cc_ver:
            v.BASECFLAGS += " -Wunreachable-code"

    if pyconf.check_compile_flag(
        "-Wstrict-prototypes", extra_flags=["-Werror"]
    ):
        v.CFLAGS_NODIST += " -Wstrict-prototypes"

    if pyconf.check_compile_flag("-Werror=implicit-function-declaration"):
        v.CFLAGS_NODIST += " -Werror=implicit-function-declaration"

    if pyconf.check_compile_flag("-fvisibility=hidden"):
        v.CFLAGS_NODIST += " -fvisibility=hidden"

    if v.host.startswith("alpha"):
        v.BASECFLAGS += " -mieee"

    if v.ac_sys_system.startswith("SCO_SV"):
        v.BASECFLAGS += " -m486 -DSCO5"

    elif v.ac_sys_system.startswith("Darwin"):
        conf_macos._setup_darwin_flags(v)


def check_compiler_characteristics(v):
    # ---------------------------------------------------------------------------
    # Compiler characteristics
    # ---------------------------------------------------------------------------

    pyconf.check_c_const()

    pyconf.checking("for working signed char")
    ac_cv_working_signed_char_c = pyconf.compile_check(
        body="signed char c;", cache_var="ac_cv_working_signed_char_c"
    )
    pyconf.result(ac_cv_working_signed_char_c)
    if not ac_cv_working_signed_char_c:
        pyconf.define(
            "signed", "", "Define to empty if the keyword does not work."
        )

    pyconf.checking("for prototypes")
    ac_cv_function_prototypes = pyconf.compile_check(
        preamble="int foo(int x) { return 0; }",
        body="return foo(10);",
        cache_var="ac_cv_function_prototypes",
    )
    pyconf.result(ac_cv_function_prototypes)
    if ac_cv_function_prototypes:
        pyconf.define(
            "HAVE_PROTOTYPES",
            1,
            "Define if your compiler supports function prototype",
        )

    # socketpair
    pyconf.check_func("socketpair", includes=["sys/types.h", "sys/socket.h"])

    # sockaddr.sa_len
    pyconf.checking("if sockaddr has sa_len member")
    ac_cv_struct_sockaddr_sa_len = pyconf.compile_check(
        preamble="#include <sys/types.h>\n#include <sys/socket.h>",
        body="struct sockaddr x;\nx.sa_len = 0;",
        cache_var="ac_cv_struct_sockaddr_sa_len",
    )
    pyconf.result(ac_cv_struct_sockaddr_sa_len)
    if ac_cv_struct_sockaddr_sa_len:
        pyconf.define(
            "HAVE_SOCKADDR_SA_LEN", 1, "Define if sockaddr has sa_len member"
        )


def check_mbstowcs(v):
    # ---------------------------------------------------------------------------
    # mbstowcs bug check
    # ---------------------------------------------------------------------------

    if pyconf.run_check(
        "for broken mbstowcs",
        """
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    size_t len = -1;
    const char *str = "text";
    len = mbstowcs(NULL, str, 0);
    return (len != 4);
}
""",
        on_success_return=False,
        on_failure_return=True,
        cross_compiling_result=False,
    ):
        pyconf.define(
            "HAVE_BROKEN_MBSTOWCS",
            1,
            'Define if mbstowcs(NULL, "text", 0) does not return the number of '
            "wide chars that would be converted.",
        )


def check_computed_gotos(v):
    # ---------------------------------------------------------------------------
    # --with-computed-gotos
    # ---------------------------------------------------------------------------

    if WITH_COMPUTED_GOTOS.is_yes():
        pyconf.define(
            "USE_COMPUTED_GOTOS",
            1,
            "Define if you want to use computed gotos in ceval.c.",
        )
    elif WITH_COMPUTED_GOTOS.is_no():
        pyconf.define(
            "USE_COMPUTED_GOTOS",
            0,
            "Define if you want to use computed gotos in ceval.c.",
        )

    # Runtime probe: does the C compiler support computed gotos?
    cg_result = pyconf.run_check(
        f"whether {v.CC} supports computed gotos",
        """
int main(int argc, char **argv) {
    static void *targets[1] = { &&LABEL1 };
    goto LABEL2;
LABEL1:
    return 0;
LABEL2:
    goto *targets[0];
    return 1;
}
""",
        cross_compiling_result=True if WITH_COMPUTED_GOTOS.is_yes() else False,
    )
    if cg_result:
        pyconf.define(
            "HAVE_COMPUTED_GOTOS",
            1,
            "Define if the C compiler supports computed gotos.",
        )


def check_stdatomic(v):
    """Check for stdatomic.h and builtin atomic functions."""
    pyconf.checking("for stdatomic.h")
    ac_cv_header_stdatomic_h = pyconf.link_check(
        "#include <stdatomic.h>\n"
        "atomic_int int_var;\n"
        "atomic_uintptr_t uintptr_var;\n"
        "int main() {\n"
        "  atomic_store_explicit(&int_var, 5, memory_order_relaxed);\n"
        "  atomic_store_explicit(&uintptr_var, 0, memory_order_relaxed);\n"
        "  int loaded_value = atomic_load_explicit(&int_var, memory_order_seq_cst);\n"
        "  return 0;\n"
        "}\n"
    )
    pyconf.result(ac_cv_header_stdatomic_h)
    v.ac_cv_header_stdatomic_h = ac_cv_header_stdatomic_h
    if ac_cv_header_stdatomic_h:
        pyconf.define(
            "HAVE_STD_ATOMIC",
            1,
            "Has stdatomic.h with atomic_int and atomic_uintptr_t",
        )

    # __atomic_load_n / __atomic_store_n builtins
    pyconf.checking(
        "for builtin __atomic_load_n and __atomic_store_n functions"
    )
    ac_cv_builtin_atomic = pyconf.link_check(
        "int val;\n"
        "int main() {\n"
        "  __atomic_store_n(&val, 1, __ATOMIC_SEQ_CST);\n"
        "  (void)__atomic_load_n(&val, __ATOMIC_SEQ_CST);\n"
        "  return 0;\n"
        "}\n"
    )
    pyconf.result(ac_cv_builtin_atomic)
    if ac_cv_builtin_atomic:
        pyconf.define(
            "HAVE_BUILTIN_ATOMIC",
            1,
            "Has builtin __atomic_load_n() and __atomic_store_n() functions",
        )

    # Check for __builtin_shufflevector with 128-bit vector support on an
    # architecture where it compiles to worthwhile native SIMD instructions.
    # Used for SIMD-accelerated bytes.hex() in Python/pystrhex.c.
    pyconf.checking("for __builtin_shufflevector")
    ac_cv_efficient_builtin_shufflevector = pyconf.link_check(
        source=(
            "#if !defined(__x86_64__) && !defined(__aarch64__) && \\\n"
            "    !(defined(__arm__) && defined(__ARM_NEON))\n"
            '#  error "128-bit vector SIMD not worthwhile on this architecture"\n'
            "#endif\n"
            "typedef unsigned char v16u8 __attribute__((vector_size(16)));\n"
            "int main(void) {\n"
            "  v16u8 a = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};\n"
            "  v16u8 b = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};\n"
            "  v16u8 c = __builtin_shufflevector(a, b,\n"
            "      0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);\n"
            "  (void)c;\n"
            "  return 0;\n"
            "}\n"
        ),
        cache_var="ac_cv_efficient_builtin_shufflevector",
    )
    pyconf.result(ac_cv_efficient_builtin_shufflevector)
    if ac_cv_efficient_builtin_shufflevector:
        pyconf.define(
            "HAVE_EFFICIENT_BUILTIN_SHUFFLEVECTOR",
            1,
            "Define if compiler supports __builtin_shufflevector with 128-bit "
            "vectors AND the target architecture has native SIMD",
        )


def check_sizes(v):
    """Check sizeof/alignof for fundamental types and pthread types."""
    pyconf.check_sizeof("int", default=4)
    pyconf.check_sizeof("long", default=4)
    pyconf.check_alignof("long")
    pyconf.check_sizeof("long long", default=8)
    pyconf.check_sizeof("void *", default=4)
    pyconf.check_sizeof("short", default=2)
    pyconf.check_sizeof("float", default=4)
    pyconf.check_sizeof("double", default=8)
    pyconf.check_sizeof("fpos_t", default=4)
    pyconf.check_sizeof("size_t", default=4)
    pyconf.check_alignof("size_t")
    pyconf.check_sizeof("pid_t", default=4)
    pyconf.check_sizeof("uintptr_t")
    pyconf.check_alignof("max_align_t")
    # AC_TYPE_LONG_DOUBLE: check that long double exists (sizeof >= sizeof(double))
    # autoconf uses <=, not >: "sizeof(double) <= sizeof(long double)"
    if pyconf.compile_check(
        preamble="",
        body="typedef int test_array[1 - 2 * !(sizeof(double) <= sizeof(long double))];",
        cache_var="ac_cv_type_long_double",
    ):
        pyconf.define(
            "HAVE_LONG_DOUBLE",
            1,
            "Define to 1 if the C compiler supports long double.",
        )
    pyconf.check_sizeof("long double", default=16)
    pyconf.check_sizeof("_Bool", default=1)
    pyconf.check_sizeof("off_t", headers=["sys/types.h"])

    pyconf.checking("whether to enable large file support")
    if pyconf.sizeof("off_t") > pyconf.sizeof("long") and pyconf.sizeof(
        "long long"
    ) >= pyconf.sizeof("off_t"):
        pyconf.define(
            "HAVE_LARGEFILE_SUPPORT",
            1,
            "Defined to enable large file support when an off_t is bigger than a long "
            "and long long is at least as big as an off_t.",
        )
        pyconf.result("yes")
    else:
        pyconf.result("no")

    pyconf.check_sizeof("time_t", headers=["sys/types.h", "time.h"])

    save_CC = v.CC
    if v.ac_cv_kpthread:
        v.CC = f"{v.CC} -Kpthread"
    elif v.ac_cv_kthread:
        v.CC = f"{v.CC} -Kthread"
    elif v.ac_cv_pthread:
        v.CC = f"{v.CC} -pthread"

    ac_cv_have_pthread_t = pyconf.compile_check(
        preamble="#include <pthread.h>",
        body="pthread_t x; x = *(pthread_t*)0;",
    )
    if ac_cv_have_pthread_t:
        pyconf.check_sizeof("pthread_t", headers=["pthread.h"])

    pyconf.check_sizeof("pthread_key_t", headers=["pthread.h"])
    if pyconf.sizeof("pthread_key_t") == pyconf.sizeof(
        "int"
    ) and pyconf.compile_check(
        preamble="#include <pthread.h>",
        body="pthread_key_t k; k * 1;",
    ):
        pyconf.define(
            "PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT",
            1,
            "Define if pthread_key_t is compatible with int.",
        )
    v.CC = save_CC
