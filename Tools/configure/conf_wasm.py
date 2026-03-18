"""conf_wasm — WebAssembly flags and options.

Handles --enable-wasm-dynamic-linking and --enable-wasm-pthreads options;
checks for unsupported systems; configures Emscripten-specific flags
(LINKFORSHARED, threading); and configures WASI-specific flags and defines.
"""

from __future__ import annotations

import pyconf

ENABLE_WASM_DYNAMIC_LINKING = pyconf.arg_enable(
    "wasm-dynamic-linking",
    help="Enable dynamic linking support for WebAssembly (default is no); "
    "WASI requires an external dynamic loader to handle imports",
)
ENABLE_WASM_PTHREADS = pyconf.arg_enable(
    "wasm-pthreads",
    help="Enable pthread emulation for WebAssembly (default is no)",
)


def setup_wasm_options(v):
    """Check for unsupported systems and handle WASM options."""
    sys_rel = f"{v.ac_sys_system}/{v.ac_sys_release}"
    if sys_rel.startswith(("atheos", "Linux/1")):
        pyconf.error(
            f"This system ({sys_rel}) is no longer supported. "
            "See README for details."
        )

    pyconf.checking("for --enable-wasm-dynamic-linking")
    if ENABLE_WASM_DYNAMIC_LINKING.given:
        if v.ac_sys_system not in ("Emscripten", "WASI"):
            pyconf.error(
                "--enable-wasm-dynamic-linking only applies to Emscripten and WASI"
            )
        v.enable_wasm_dynamic_linking = ENABLE_WASM_DYNAMIC_LINKING.value_or(
            False
        )
    else:
        v.enable_wasm_dynamic_linking = False
    pyconf.result(
        v.enable_wasm_dynamic_linking
        if v.enable_wasm_dynamic_linking is not False
        else "missing"
    )

    pyconf.checking("for --enable-wasm-pthreads")
    if ENABLE_WASM_PTHREADS.given:
        if v.ac_sys_system not in ("Emscripten", "WASI"):
            pyconf.error(
                "--enable-wasm-pthreads only applies to Emscripten and WASI"
            )
        v.enable_wasm_pthreads = ENABLE_WASM_PTHREADS.value_or(False)
    else:
        v.enable_wasm_pthreads = "missing"
    pyconf.result(v.enable_wasm_pthreads)


def setup_wasm_flags(v):
    """Set WASM-specific CFLAGS, LDFLAGS, and LINKFORSHARED."""
    v.LINKFORSHARED = ""

    if v.ac_sys_system == "Emscripten":
        _setup_emscripten_flags(v)
    elif v.ac_sys_system == "WASI":
        _setup_wasi_flags(v)

    # Force/suppress ac_cv_func_dlopen for WASM dynamic linking
    if v.enable_wasm_dynamic_linking is True:
        v.ac_cv_func_dlopen = True
    elif v.enable_wasm_dynamic_linking is False:
        v.ac_cv_func_dlopen = False

    # Ensure variables exist even on non-WASM builds
    if not v.is_set("ARCH_TRIPLES"):
        v.ARCH_TRIPLES = ""
    if not v.is_set("wasm_debug"):
        v.wasm_debug = False
    if not v.is_set("ac_cv_func_dlopen"):
        pass  # left unset; Part 5 will probe it

    v.export("BASECFLAGS")
    v.export("CFLAGS_NODIST")
    v.export("LDFLAGS_NODIST")
    v.export("LDFLAGS_NOLTO")
    v.export("EXE_LDFLAGS")
    v.export("LINKFORSHARED")
    v.export("WASM_ASSETS_DIR")
    v.export("WASM_STDLIB")

    v.export("UNIVERSAL_ARCH_FLAGS", "")


def _setup_emscripten_flags(v):
    """Set Emscripten-specific flags."""
    v.wasm_debug = v.Py_DEBUG is True

    v.LINKFORSHARED += " -sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=20971520"
    v.LDFLAGS_NODIST += " -sWASM_BIGINT"
    v.LINKFORSHARED += (
        " -sFORCE_FILESYSTEM -lidbfs.js -lnodefs.js -lproxyfs.js -lworkerfs.js"
    )
    v.LINKFORSHARED += (
        " -sEXPORTED_RUNTIME_METHODS=FS,callMain,ENV,HEAPU32,TTY"
    )
    v.LINKFORSHARED += (
        " -sEXPORTED_FUNCTIONS=_main,_Py_Version,__PyRuntime,"
        "_PyGILState_GetThisThreadState,__Py_DumpTraceback,"
        "__PyEM_EMSCRIPTEN_TRAMPOLINE_OFFSET"
    )
    v.LINKFORSHARED += " -sSTACK_SIZE=5MB"
    v.LINKFORSHARED += " -sTEXTDECODER=2"

    if v.enable_wasm_dynamic_linking:
        v.LINKFORSHARED += " -sMAIN_MODULE"

    if v.enable_wasm_pthreads:
        v.CFLAGS_NODIST += " -pthread"
        v.LDFLAGS_NODIST += " -sUSE_PTHREADS"
        v.LINKFORSHARED += " -sPROXY_TO_PTHREAD"

    v.LDFLAGS_NODIST += " -sEXIT_RUNTIME"
    WASM_LINKFORSHARED_DEBUG = "-gseparate-dwarf --emit-symbol-map"

    if v.wasm_debug:
        v.LDFLAGS_NODIST += " -sASSERTIONS"
        v.LINKFORSHARED += f" {WASM_LINKFORSHARED_DEBUG}"
    else:
        v.LINKFORSHARED += " -O2 -g0"


def _setup_wasi_flags(v):
    """Set WASI-specific flags and defines."""
    pyconf.define(
        "_WASI_EMULATED_SIGNAL",
        1,
        "Define to 1 if you want to emulate signals on WASI",
    )
    pyconf.define(
        "_WASI_EMULATED_GETPID",
        1,
        "Define to 1 if you want to emulate getpid() on WASI",
    )
    pyconf.define(
        "_WASI_EMULATED_PROCESS_CLOCKS",
        1,
        "Define to 1 if you want to emulate process clocks on WASI",
    )
    v.LIBS += " -lwasi-emulated-signal -lwasi-emulated-getpid -lwasi-emulated-process-clocks"

    if v.enable_wasm_pthreads:
        v.CFLAGS += " -target wasm32-wasi-threads -pthread"
        v.CFLAGS_NODIST += " -target wasm32-wasi-threads -pthread"
        v.LDFLAGS_NODIST += (
            " -target wasm32-wasi-threads -pthread"
            " -Wl,--import-memory"
            " -Wl,--export-memory"
            " -Wl,--max-memory=10485760"
        )

    v.LDFLAGS_NODIST += " -z stack-size=16777216 -Wl,--stack-first -Wl,--initial-memory=41943040"


def setup_platform_objects(v):
    """Set PLATFORM_HEADERS and PLATFORM_OBJS for Emscripten."""
    v.PLATFORM_HEADERS = ""
    v.PLATFORM_OBJS = ""

    if v.ac_sys_system == "Emscripten":
        v.PLATFORM_OBJS += (
            " Python/emscripten_signal.o"
            " Python/emscripten_trampoline.o"
            " Python/emscripten_trampoline_wasm.o"
            " Python/emscripten_syscalls.o"
        )
        v.PLATFORM_HEADERS += (
            " $(srcdir)/Include/internal/pycore_emscripten_signal.h"
            " $(srcdir)/Include/internal/pycore_emscripten_trampoline.h"
        )

    v.export("PLATFORM_HEADERS")
    v.export("PLATFORM_OBJS")
