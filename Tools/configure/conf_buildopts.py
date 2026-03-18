"""conf_buildopts — GIL, debug, assertions, optimizations, profile task.

Handles --disable-gil (free-threaded build), --with-pydebug (Py_DEBUG),
--with-trace-refs, --enable-pystats, --with-assertions, and
--enable-optimizations (PGO, -fno-semantic-interposition).  Also sets
the PROFILE_TASK used for PGO training runs and DEF_MAKE_ALL_RULE.
"""

from __future__ import annotations

import pyconf

DISABLE_GIL = pyconf.arg_enable(
    "gil",
    display="--disable-gil",
    help="enable support for running without the GIL (default is no)",
    var="disable_gil",
    false_is="yes",
)
WITH_PYDEBUG = pyconf.arg_with(
    "pydebug",
    help="build with Py_DEBUG defined (default is no)",
    var="Py_DEBUG",
    false_is="no",
)
WITH_TRACE_REFS = pyconf.arg_with(
    "trace-refs",
    help="enable tracing references for debugging purpose (default is no)",
    default="no",
    var="with_trace_refs",
)
ENABLE_PYSTATS = pyconf.arg_enable(
    "pystats",
    help="enable internal statistics gathering (default is no)",
    default="no",
    var="enable_pystats",
)
WITH_ASSERTIONS = pyconf.arg_with(
    "assertions",
    help="build with C assertions enabled (default is no)",
    false_is="no",
)
ENABLE_OPTIMIZATIONS = pyconf.arg_enable(
    "optimizations",
    help="enable expensive, stable optimizations (PGO, etc.) (default is no)",
    var="Py_OPT",
    false_is="no",
)
ENABLE_EXPERIMENTAL_JIT = pyconf.arg_enable(
    "experimental-jit",
    metavar="no|yes|yes-off|interpreter",
    default=False,
    help="build the experimental just-in-time compiler (default is no)",
)
WITH_ADDRESS_SANITIZER = pyconf.arg_with(
    "address-sanitizer",
    help="enable AddressSanitizer memory error detector, 'asan' (default is no)",
    default="no",
    false_is="no",
)
WITH_MEMORY_SANITIZER = pyconf.arg_with(
    "memory-sanitizer",
    help="enable MemorySanitizer allocation error detector, 'msan' (default is no)",
    default="no",
    false_is="no",
)
WITH_UNDEFINED_BEHAVIOR_SANITIZER = pyconf.arg_with(
    "undefined-behavior-sanitizer",
    help="enable UndefinedBehaviorSanitizer undefined behaviour detector, 'ubsan' (default is no)",
    default="no",
    false_is="no",
)
WITH_THREAD_SANITIZER = pyconf.arg_with(
    "thread-sanitizer",
    help="enable ThreadSanitizer data race detector, 'tsan' (default is no)",
    default="no",
    false_is="no",
)
WITH_TAIL_CALL_INTERP = pyconf.arg_with(
    "tail-call-interp",
    help="enable tail-calling interpreter in evaluation loop and rest of CPython",
)
WITH_REMOTE_DEBUG = pyconf.arg_with(
    "remote-debug",
    help="enable remote debugging support (default is yes)",
)
WITH_ENSUREPIP = pyconf.arg_with(
    "ensurepip",
    metavar="install|upgrade|no",
    help='"install" or "upgrade" using bundled pip (default is upgrade)',
)
WITH_MIMALLOC = pyconf.arg_with(
    "mimalloc",
    help="build with mimalloc memory allocator (default yes if stdatomic.h available)",
)
WITH_PYMALLOC = pyconf.arg_with(
    "pymalloc", help="enable specialized mallocs (default is yes)"
)
WITH_PYMALLOC_HUGEPAGES = pyconf.arg_with(
    "pymalloc-hugepages",
    help="enable huge page support for pymalloc arenas (default is no)",
    default="no",
)
WITH_VALGRIND = pyconf.arg_with(
    "valgrind", help="enable Valgrind support (default is no)", default="no"
)
WITH_DTRACE = pyconf.arg_with(
    "dtrace", help="enable DTrace support (default is no)", default="no"
)
WITH_DOC_STRINGS = pyconf.arg_with(
    "doc-strings",
    help="enable documentation strings (default is yes)",
    default="yes",
)


def setup_disable_gil(v):
    """Handle --disable-gil option."""
    if DISABLE_GIL.process_bool(v):
        pyconf.define(
            "Py_GIL_DISABLED", 1, "Define if you want to disable the GIL"
        )
        v.ABIFLAGS += "t"
        v.export("ABI_THREAD", "t")


def setup_pydebug(v):
    """Handle --with-pydebug option."""
    if WITH_PYDEBUG.process_bool(v):
        pyconf.define(
            "Py_DEBUG",
            1,
            "Define if you want to build an interpreter with many run-time checks.",
        )
        v.ABIFLAGS += "d"


def setup_trace_refs(v):
    """Handle --with-trace-refs option."""
    WITH_TRACE_REFS.process_value(v)

    if WITH_TRACE_REFS.is_yes():
        pyconf.define(
            "Py_TRACE_REFS",
            1,
            "Define if you want to enable tracing references for debugging purpose",
        )

    if v.disable_gil and WITH_TRACE_REFS.is_yes():
        pyconf.error("--disable-gil cannot be used with --with-trace-refs")


def setup_pystats(v):
    """Handle --enable-pystats option."""
    ENABLE_PYSTATS.process_value(v)

    if ENABLE_PYSTATS.is_yes():
        pyconf.define(
            "Py_STATS",
            1,
            "Define if you want to enable internal statistics gathering.",
        )


def setup_assertions(v):
    """Handle --with-assertions option."""
    assertions_bool = WITH_ASSERTIONS.process_bool()
    v.assertions = "true" if assertions_bool else "false"

    if not assertions_bool and v.Py_DEBUG is True:
        v.assertions = "true"
        pyconf.checking("for --with-assertions")
        pyconf.result("implied by --with-pydebug")


def setup_optimizations(v):
    """Handle --enable-optimizations and DEF_MAKE_ALL_RULE."""
    v.export("DEF_MAKE_ALL_RULE")
    v.export("DEF_MAKE_RULE")

    ENABLE_OPTIMIZATIONS.process_bool(v)

    v.REQUIRE_PGO = False
    DEF_MAKE_ALL_RULE = ""
    DEF_MAKE_RULE = ""

    if v.Py_OPT is True:
        if "-O0" in v.CFLAGS:
            pyconf.warn(
                "CFLAGS contains -O0 which may conflict with --enable-optimizations. "
                "Consider removing -O0 from CFLAGS for optimal performance."
            )
        DEF_MAKE_ALL_RULE = "profile-opt"
        v.REQUIRE_PGO = True
        DEF_MAKE_RULE = "build_all"
        if v.ac_cv_gcc_compat:
            if pyconf.check_compile_flag(
                "-fno-semantic-interposition", extra_flags=["-Werror"]
            ):
                v.CFLAGS_NODIST += " -fno-semantic-interposition"
                v.LDFLAGS_NODIST += " -fno-semantic-interposition"
    elif v.ac_sys_system == "Emscripten":
        DEF_MAKE_ALL_RULE = "build_emscripten"
        DEF_MAKE_RULE = "all"
    elif v.ac_sys_system == "WASI":
        DEF_MAKE_ALL_RULE = "build_wasm"
        DEF_MAKE_RULE = "all"
    else:
        DEF_MAKE_ALL_RULE = "build_all"
        DEF_MAKE_RULE = "all"

    v.DEF_MAKE_ALL_RULE = DEF_MAKE_ALL_RULE
    v.DEF_MAKE_RULE = DEF_MAKE_RULE
    v.export("REQUIRE_PGO")


def setup_experimental_jit(v):
    """Handle --enable-experimental-jit, compiler-specific flags, NDEBUG, arch flags."""
    pyconf.checking("for --enable-experimental-jit")
    jit = ENABLE_EXPERIMENTAL_JIT.value
    if jit is None or jit is False or jit == "no":
        jit_flags = ""
        tier2_flags = ""
    elif jit == "yes":
        jit_flags = "-D_Py_JIT"
        tier2_flags = "-D_Py_TIER2=1"
    elif jit == "yes-off":
        jit_flags = "-D_Py_JIT"
        tier2_flags = "-D_Py_TIER2=3"
    elif jit == "interpreter":
        jit_flags = ""
        tier2_flags = "-D_Py_TIER2=4"
    elif jit == "interpreter-off":
        jit_flags = ""
        tier2_flags = "-D_Py_TIER2=6"
    else:
        pyconf.error(
            f"invalid argument: --enable-experimental-jit={jit}; "
            "expected no|yes|yes-off|interpreter"
        )

    if tier2_flags:
        v.CFLAGS_NODIST += f" {tier2_flags}"
    if jit_flags:
        v.CFLAGS_NODIST += f" {jit_flags}"
        REGEN_JIT_COMMAND = (
            "$(PYTHON_FOR_REGEN) $(srcdir)/Tools/jit/build.py "
            f"{v.ARCH_TRIPLES or v.host} --output-dir . --pyconfig-dir . "
            f'--cflags="{v.CFLAGS_JIT}" --llvm-version="{v.LLVM_VERSION}"'
        )
        if v.Py_DEBUG is True:
            REGEN_JIT_COMMAND += " --debug"
    else:
        REGEN_JIT_COMMAND = ""
    v.export("REGEN_JIT_COMMAND", REGEN_JIT_COMMAND)
    pyconf.result(f"{tier2_flags} {jit_flags}")

    if v.disable_gil and jit is not False:
        pyconf.warn(
            "--enable-experimental-jit does not work correctly with --disable-gil."
        )

    if v.ac_cv_cc_name == "mpicc":
        pass
    elif v.ac_cv_cc_name == "icc":
        v.CFLAGS_NODIST += " -fp-model strict"
    elif v.ac_cv_cc_name == "xlc":
        v.CFLAGS_NODIST += " -qalias=noansi -qmaxmem=-1"

    if v.assertions != "true":
        v.OPT = f"-DNDEBUG {v.OPT}"

    if v.ac_arch_flags:
        v.BASECFLAGS += f" {v.ac_arch_flags}"


def check_jit_stencils(v):
    # ---------------------------------------------------------------------------
    # JIT stencils header selection
    # ---------------------------------------------------------------------------

    v.JIT_STENCILS_H = ""
    if ENABLE_EXPERIMENTAL_JIT.given and not ENABLE_EXPERIMENTAL_JIT.is_no():
        host = v.host
        if host.startswith("aarch64-apple-darwin"):
            v.JIT_STENCILS_H = "jit_stencils-aarch64-apple-darwin.h"
        elif host.startswith("x86_64-apple-darwin"):
            v.JIT_STENCILS_H = "jit_stencils-x86_64-apple-darwin.h"
        elif host.startswith("aarch64-pc-windows-msvc"):
            v.JIT_STENCILS_H = "jit_stencils-aarch64-pc-windows-msvc.h"
        elif host.startswith("i686-pc-windows-msvc"):
            v.JIT_STENCILS_H = "jit_stencils-i686-pc-windows-msvc.h"
        elif host.startswith("x86_64-pc-windows-msvc"):
            v.JIT_STENCILS_H = "jit_stencils-x86_64-pc-windows-msvc.h"
        elif pyconf.fnmatch(host, "aarch64-*-linux-gnu*"):
            v.JIT_STENCILS_H = "jit_stencils-aarch64-unknown-linux-gnu.h"
        elif pyconf.fnmatch(host, "x86_64-*-linux-gnu*"):
            v.JIT_STENCILS_H = "jit_stencils-x86_64-unknown-linux-gnu.h"

    v.export("JIT_STENCILS_H")


def setup_sanitizers(v):
    """Handle --with-address/memory/undefined-behavior/thread-sanitizer options."""
    if WITH_ADDRESS_SANITIZER.process_bool(v):
        v.BASECFLAGS = (
            f"-fsanitize=address -fno-omit-frame-pointer {v.BASECFLAGS}"
        )
        v.LDFLAGS = f"-fsanitize=address {v.LDFLAGS}"

    if WITH_MEMORY_SANITIZER.process_bool(v):
        if not pyconf.check_compile_flag("-fsanitize=memory"):
            pyconf.error(
                "The selected compiler doesn't support memory sanitizer"
            )
        v.BASECFLAGS = (
            f"-fsanitize=memory -fsanitize-memory-track-origins=2 "
            f"-fno-omit-frame-pointer {v.BASECFLAGS}"
        )
        v.LDFLAGS = (
            f"-fsanitize=memory -fsanitize-memory-track-origins=2 {v.LDFLAGS}"
        )

    if WITH_UNDEFINED_BEHAVIOR_SANITIZER.process_bool(v):
        v.BASECFLAGS = f"-fsanitize=undefined {v.BASECFLAGS}"
        v.LDFLAGS = f"-fsanitize=undefined {v.LDFLAGS}"
        v.with_ubsan = True
    else:
        v.with_ubsan = False

    if WITH_THREAD_SANITIZER.process_bool(v):
        v.BASECFLAGS = f"-fsanitize=thread {v.BASECFLAGS}"
        v.LDFLAGS = f"-fsanitize=thread {v.LDFLAGS}"


def check_tail_call_interp(v):
    # ---------------------------------------------------------------------------
    # --with-tail-call-interp
    # ---------------------------------------------------------------------------

    if WITH_TAIL_CALL_INTERP.is_yes():
        pyconf.define(
            "_Py_TAIL_CALL_INTERP",
            1,
            "Define if you want to use tail-calling interpreters in CPython.",
        )
    elif WITH_TAIL_CALL_INTERP.is_no():
        pyconf.define(
            "_Py_TAIL_CALL_INTERP",
            0,
            "Define if you want to use tail-calling interpreters in CPython.",
        )


def check_remote_debug(v):
    # ---------------------------------------------------------------------------
    # --with-remote-debug
    # ---------------------------------------------------------------------------

    with_rd = WITH_REMOTE_DEBUG.value_or("yes")
    if with_rd == "yes":
        pyconf.define(
            "Py_REMOTE_DEBUG",
            1,
            "Define if you want to enable remote debugging support.",
        )


def check_ensurepip(v):
    # ---------------------------------------------------------------------------
    # --with-ensurepip
    # ---------------------------------------------------------------------------

    default_ensurepip = (
        False
        if v.ac_sys_system in ("Emscripten", "WASI", "iOS")
        else "upgrade"
    )
    ensurepip_raw = WITH_ENSUREPIP.value
    with_ensurepip = (
        ensurepip_raw if ensurepip_raw is not None else default_ensurepip
    )

    if with_ensurepip in ("yes", "upgrade"):
        v.ENSUREPIP = "upgrade"
    elif with_ensurepip == "install":
        v.ENSUREPIP = "install"
    elif with_ensurepip is False or with_ensurepip == "no":
        v.ENSUREPIP = False
    else:
        pyconf.error("--with-ensurepip=upgrade|install|no")

    v.export("ENSUREPIP")


def setup_mimalloc(v):
    """Handle --with-mimalloc option."""
    pyconf.checking("for --with-mimalloc")
    with_mimalloc = WITH_MIMALLOC.value
    if with_mimalloc is None:
        with_mimalloc = "yes" if v.ac_cv_header_stdatomic_h else "no"

    v.with_mimalloc = with_mimalloc

    if with_mimalloc != "no":
        if not v.ac_cv_header_stdatomic_h:
            pyconf.fatal(
                "mimalloc requires stdatomic.h, use --without-mimalloc to disable mimalloc."
            )
        with_mimalloc = "yes"
        v.with_mimalloc = with_mimalloc
        pyconf.define(
            "WITH_MIMALLOC",
            1,
            "Define if you want to compile in mimalloc memory allocator.",
        )
        v.export("MIMALLOC_HEADERS", r"$(MIMALLOC_HEADERS)")
    elif v.disable_gil is True:
        pyconf.fatal(
            "--disable-gil requires mimalloc memory allocator (--with-mimalloc)."
        )

    pyconf.result(with_mimalloc)
    v.export("INSTALL_MIMALLOC", with_mimalloc)
    v.export("MIMALLOC_HEADERS", "")


def setup_pymalloc(v):
    """Handle --with-pymalloc and --with-pymalloc-hugepages options."""
    pyconf.checking("for --with-pymalloc")
    if not v.is_set("with_pymalloc") or not v.with_pymalloc:
        with_pymalloc = WITH_PYMALLOC.value
        if with_pymalloc is None:
            if v.ac_sys_system in ("Emscripten", "WASI"):
                with_pymalloc = "no"
            else:
                with_pymalloc = "yes"
        v.with_pymalloc = with_pymalloc
    else:
        with_pymalloc = v.with_pymalloc

    if with_pymalloc != "no":
        pyconf.define(
            "WITH_PYMALLOC",
            1,
            "Define if you want to compile in Python-specific mallocs",
        )
    pyconf.result(with_pymalloc)

    # --with-pymalloc-hugepages
    pyconf.checking("for --with-pymalloc-hugepages")
    with_pymalloc_hugepages = WITH_PYMALLOC_HUGEPAGES.value_or("no")
    if with_pymalloc_hugepages == "yes":
        if pyconf.compile_check(
            preamble="#include <sys/mman.h>",
            body="int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;\n"
            "(void)flags;",
        ):
            pyconf.define(
                "PYMALLOC_USE_HUGEPAGES",
                1,
                "Define to use huge pages for pymalloc arenas",
            )
        else:
            pyconf.warn(
                "--with-pymalloc-hugepages requested but MAP_HUGETLB not found"
            )
            with_pymalloc_hugepages = "no"
    pyconf.result(with_pymalloc_hugepages)


def setup_valgrind(v):
    """Handle --with-valgrind option."""
    with_valgrind = WITH_VALGRIND.process_value(None)
    if with_valgrind != "no":
        if pyconf.check_header("valgrind/valgrind.h"):
            pyconf.define(
                "WITH_VALGRIND",
                1,
                "Define if you want pymalloc to be disabled when running under valgrind",
            )
        else:
            pyconf.fatal(
                "Valgrind support requested but headers not available"
            )
        v.OPT = "-DDYNAMIC_ANNOTATIONS_ENABLED=1 " + v.OPT


def setup_dtrace(v):
    """Handle --with-dtrace option."""
    with_dtrace = WITH_DTRACE.process_value(None)

    v.export("DTRACE", "")
    v.export("DFLAGS", "")
    v.export("DTRACE_HEADERS", "")
    v.export("DTRACE_OBJS", "")

    if with_dtrace == "yes":
        dtrace = pyconf.find_prog("dtrace")
        if not dtrace:
            pyconf.fatal("dtrace command not found on $PATH")
        v.DTRACE = dtrace
        pyconf.define(
            "WITH_DTRACE", 1, "Define if you want to compile in DTrace support"
        )
        v.DTRACE_HEADERS = "Include/pydtrace_probes.h"

        # Check if DTrace probes require linking (ELF generation via -G flag)
        pyconf.checking("whether DTrace probes require linking")
        ac_cv_dtrace_link = False
        host = v.host
        if "netbsd" in host.lower():
            dtrace_test_flags = ["-x", "nolibs", "-h"]
        else:
            dtrace_test_flags = ["-G"]
        tmp = "/tmp/conftest.d"
        pyconf.write_file(tmp, "BEGIN{}\n")
        dtrace_cmd: list[str] = (
            [str(v.DTRACE)]
            + v.DFLAGS.split()
            + dtrace_test_flags
            + ["-s", tmp, "-o", "/tmp/conftest.o"]
        )
        if pyconf.cmd(dtrace_cmd):
            ac_cv_dtrace_link = True
        pyconf.rm_f(tmp)
        pyconf.result(ac_cv_dtrace_link)

        if ac_cv_dtrace_link:
            v.DTRACE_OBJS = "Python/pydtrace.o"

        # NetBSD-specific DTrace flags
        if "netbsd" in host.lower():
            v.DFLAGS += " -x nolibs"


def setup_perf_trampoline(v):
    """Check and configure perf trampoline support."""
    pyconf.checking("perf trampoline")
    if v.PLATFORM_TRIPLET in ("x86_64-linux-gnu", "aarch64-linux-gnu"):
        perf_trampoline = True
    elif v.PLATFORM_TRIPLET == "darwin":
        target = v.MACOSX_DEPLOYMENT_TARGET
        if pyconf.fnmatch_any(target, ["10.[0-9]", "10.1[0-1]"]):
            perf_trampoline = False
        else:
            perf_trampoline = True
    else:
        perf_trampoline = False
    pyconf.result(perf_trampoline)

    if perf_trampoline:
        pyconf.define(
            "PY_HAVE_PERF_TRAMPOLINE",
            1,
            "Define to 1 if you have the perf trampoline.",
        )
        PERF_TRAMPOLINE_OBJ = "Python/asm_trampoline.o"
        if v.Py_DEBUG is True:
            v.BASECFLAGS += (
                " -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer"
            )
    else:
        PERF_TRAMPOLINE_OBJ = ""
    v.export("PERF_TRAMPOLINE_OBJ", PERF_TRAMPOLINE_OBJ)


def setup_doc_strings():
    """Handle --with-doc-strings option."""
    with_doc_strings = WITH_DOC_STRINGS.process_value(None)
    if with_doc_strings != "no":
        pyconf.define(
            "WITH_DOC_STRINGS",
            1,
            "Define if you want documentation strings in extension modules",
        )
