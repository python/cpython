"""conf_optimization — LLVM, LTO, PGO, BOLT toolchain integration.

Locates the LLVM bin directory for clang-based builds; handles
--with-lto (full/thin/default) including llvm-ar discovery and
platform-specific LTO flags for clang, gcc, and emcc; configures
PGO instrumentation/use flags and llvm-profdata; and handles
--enable-bolt (llvm-bolt, merge-fdata, BOLT_*_FLAGS).
"""

import os

import pyconf

WITH_LTO = pyconf.arg_with(
    "lto",
    metavar="full|thin|no|yes",
    help="enable Link-Time-Optimization in any build (default is no)",
)
ENABLE_BOLT = pyconf.arg_enable(
    "bolt",
    help="enable usage of the llvm-bolt post-link optimizer (default is no)",
    false_is="no",
)


def setup_profile_task(v):
    """Handle PROFILE_TASK environment variable."""
    pyconf.env_var("PROFILE_TASK", "Python args for PGO generation task")
    pyconf.checking("PROFILE_TASK")
    v.PROFILE_TASK = v.PROFILE_TASK or "-m test --pgo --timeout=$(TESTTIMEOUT)"
    pyconf.result(v.PROFILE_TASK)
    v.export("PROFILE_TASK")


def locate_llvm_bin(v):
    """Locate LLVM bin directory for clang-based builds."""
    llvm_bin_dir = ""
    llvm_path = os.environ.get("PATH", "")
    if v.ac_cv_cc_name == "clang":
        clang_bin = pyconf.find_prog("clang")
        if clang_bin and pyconf.path_is_symlink(clang_bin):
            clang_dir = pyconf.path_parent(clang_bin)
            clang_target = pyconf.readlink(clang_bin)
            llvm_bin_dir = pyconf.path_join(
                [clang_dir, pyconf.path_parent(clang_target)]
            )
            llvm_path = llvm_path + os.pathsep + llvm_bin_dir
    v.llvm_path = llvm_path


def setup_lto(v):
    """Handle --with-lto option."""
    pyconf.checking("for --with-lto")
    v.Py_LTO = "false"
    v.Py_LTO_POLICY = "default"
    v.LTOFLAGS = ""
    v.LTOCFLAGS = ""
    v.LDFLAGS_NOLTO = ""

    lto_val = WITH_LTO.value
    if lto_val is not None:
        if lto_val == "full":
            v.Py_LTO = "true"
            v.Py_LTO_POLICY = "full"
            pyconf.result("yes")
        elif lto_val == "thin":
            v.Py_LTO = "true"
            v.Py_LTO_POLICY = "thin"
            pyconf.result("yes")
        elif lto_val == "yes":
            v.Py_LTO = "true"
            v.Py_LTO_POLICY = "default"
            pyconf.result("yes")
        elif lto_val == "no":
            v.Py_LTO = "false"
            pyconf.result("no")
        else:
            pyconf.error(f"error: unknown lto option: '{lto_val}'")
    else:
        pyconf.result("no")

    if v.Py_LTO == "true":
        _setup_lto_flags(v)

    v.export("LTOFLAGS")
    v.export("LTOCFLAGS")


def _setup_lto_flags(v):
    """Set LTO compiler/linker flags based on compiler type."""
    if v.ac_cv_cc_name == "clang":
        _setup_lto_clang(v)
    elif v.ac_cv_cc_name == "emcc":
        if v.Py_LTO_POLICY != "default":
            pyconf.error("error: emcc supports only default lto.")
        v.LTOFLAGS = "-flto"
        v.LTOCFLAGS = "-flto"
    elif v.ac_cv_cc_name == "gcc":
        if v.Py_LTO_POLICY == "thin":
            pyconf.error(
                "error: thin lto is not supported under gcc compiler."
            )
        v.LDFLAGS_NOLTO = "-fno-lto"
        if v.ac_sys_system.startswith("Darwin"):
            v.LTOFLAGS = (
                '-flto -Wl,-export_dynamic -Wl,-object_path_lto,"$@".lto'
            )
            v.LTOCFLAGS = "-flto"
        else:
            v.LTOFLAGS = "-flto -fuse-linker-plugin -ffat-lto-objects"

    if v.ac_cv_prog_cc_g:
        v.LTOFLAGS += " -g"

    v.CFLAGS_NODIST += f" {v.LTOCFLAGS or v.LTOFLAGS}"
    v.LDFLAGS_NODIST += f" {v.LTOFLAGS}"


def _setup_lto_clang(v):
    """Set LTO flags for clang compiler."""
    v.LDFLAGS_NOLTO = "-fno-lto"
    if pyconf.check_compile_flag("-flto=thin"):
        v.LDFLAGS_NOLTO = "-flto=thin"
    else:
        v.LDFLAGS_NOLTO = "-flto"

    LLVM_AR = pyconf.check_prog("llvm-ar", path=v.llvm_path, default="")
    v.export("LLVM_AR")
    LLVM_AR_FOUND = (
        "found" if (LLVM_AR and pyconf.is_executable(LLVM_AR)) else "not-found"
    )
    v.export("LLVM_AR_FOUND")

    if v.ac_sys_system.startswith("Darwin") and LLVM_AR_FOUND == "not-found":
        xcrun_status, xcrun_out = pyconf.cmd_status(
            ["/usr/bin/xcrun", "-find", "ar"]
        )
        if xcrun_status == 0 and xcrun_out:
            LLVM_AR = "/usr/bin/xcrun ar"
            LLVM_AR_FOUND = "found"
            pyconf.notice(f"llvm-ar found via xcrun: {LLVM_AR}")

    if LLVM_AR_FOUND == "not-found":
        pyconf.error(
            "error: llvm-ar is required for a --with-lto build with clang "
            "but could not be found."
        )

    v.AR = LLVM_AR

    if v.ac_sys_system.startswith("Darwin"):
        if v.Py_LTO_POLICY == "default":
            if pyconf.check_compile_flag("-flto=thin"):
                v.LTOFLAGS = '-flto=thin -Wl,-export_dynamic -Wl,-object_path_lto,"$@".lto'
                v.LTOCFLAGS = "-flto=thin"
            else:
                v.LTOFLAGS = (
                    '-flto -Wl,-export_dynamic -Wl,-object_path_lto,"$@".lto'
                )
                v.LTOCFLAGS = "-flto"
        else:
            v.LTOFLAGS = (
                f"-flto={v.Py_LTO_POLICY} -Wl,-export_dynamic"
                f' -Wl,-object_path_lto,"$@".lto'
            )
            v.LTOCFLAGS = f"-flto={v.Py_LTO_POLICY}"
    else:
        if v.Py_LTO_POLICY == "default":
            if pyconf.check_compile_flag("-flto=thin"):
                v.LTOFLAGS = "-flto=thin"
            else:
                v.LTOFLAGS = "-flto"
        else:
            v.LTOFLAGS = f"-flto={v.Py_LTO_POLICY}"


def setup_pgo_flags(v):
    """Set PGO profiling flags based on compiler type."""
    v.export("PGO_PROF_GEN_FLAG")
    v.export("PGO_PROF_USE_FLAG")
    v.export("LLVM_PROF_MERGER")
    v.export("LLVM_PROF_FILE")
    v.export("LLVM_PROF_ERR")
    v.export("LLVM_PROFDATA")

    v.LLVM_PROFDATA = pyconf.check_prog(
        "llvm-profdata", path=v.llvm_path, default=""
    )
    v.export("LLVM_PROFDATA")
    LLVM_PROF_FOUND = (
        "found"
        if (v.LLVM_PROFDATA and pyconf.is_executable(v.LLVM_PROFDATA))
        else "not-found"
    )
    v.export("LLVM_PROF_FOUND")

    if v.ac_sys_system.startswith("Darwin") and LLVM_PROF_FOUND == "not-found":
        xcrun_status, xcrun_out = pyconf.cmd_status(
            ["/usr/bin/xcrun", "-find", "llvm-profdata"]
        )
        if xcrun_status == 0 and xcrun_out:
            v.LLVM_PROFDATA = "/usr/bin/xcrun llvm-profdata"
            LLVM_PROF_FOUND = "found"
            pyconf.notice(f"llvm-profdata found via xcrun: {v.LLVM_PROFDATA}")

    v.LLVM_PROF_ERR = False
    v.PGO_PROF_GEN_FLAG = ""
    v.PGO_PROF_USE_FLAG = ""
    v.LLVM_PROF_MERGER = ""
    v.LLVM_PROF_FILE = ""

    if v.ac_cv_cc_name == "clang":
        v.PGO_PROF_GEN_FLAG = "-fprofile-instr-generate"
        v.PGO_PROF_USE_FLAG = (
            '-fprofile-instr-use="$(shell pwd)/code.profclangd"'
        )
        v.LLVM_PROF_MERGER = (
            f"{v.LLVM_PROFDATA} merge"
            ' -output="$(shell pwd)/code.profclangd"'
            ' "$(shell pwd)"/*.profclangr'
        )
        v.LLVM_PROF_FILE = (
            'LLVM_PROFILE_FILE="$(shell pwd)/code-%p.profclangr"'
        )
        if LLVM_PROF_FOUND == "not-found":
            v.LLVM_PROF_ERR = True
            if v.REQUIRE_PGO:
                pyconf.error(
                    "error: llvm-profdata is required for a --enable-optimizations "
                    "build but could not be found."
                )
    elif v.ac_cv_cc_name == "gcc":
        if pyconf.check_compile_flag("-fprofile-update=atomic"):
            v.PGO_PROF_GEN_FLAG = "-fprofile-generate -fprofile-update=atomic"
        else:
            v.PGO_PROF_GEN_FLAG = "-fprofile-generate"
        v.PGO_PROF_USE_FLAG = "-fprofile-use -fprofile-correction"
        v.LLVM_PROF_MERGER = "true"
        v.LLVM_PROF_FILE = ""
    elif v.ac_cv_cc_name == "icc":
        v.PGO_PROF_GEN_FLAG = "-prof-gen"
        v.PGO_PROF_USE_FLAG = "-prof-use"
        v.LLVM_PROF_MERGER = "true"
        v.LLVM_PROF_FILE = ""


def setup_bolt(v):
    """Handle --enable-bolt option."""
    bolt_enabled = ENABLE_BOLT.process_bool()
    v.Py_BOLT = "true" if bolt_enabled else "false"

    v.export("PREBOLT_RULE")
    v.PREBOLT_RULE = ""
    v.LLVM_BOLT = ""
    v.MERGE_FDATA = ""
    v.export("LLVM_BOLT")
    v.export("MERGE_FDATA")

    if v.Py_BOLT == "true":
        _setup_bolt_tools(v)

    _setup_bolt_flags(v)


def _setup_bolt_tools(v):
    """Find and configure BOLT tools when --enable-bolt is active."""
    v.PREBOLT_RULE = v.DEF_MAKE_ALL_RULE
    v.DEF_MAKE_ALL_RULE = "bolt-opt"
    v.DEF_MAKE_RULE = "build_all"

    if pyconf.check_compile_flag("-fno-reorder-blocks-and-partition"):
        v.CFLAGS_NODIST += " -fno-reorder-blocks-and-partition"

    v.LDFLAGS_NODIST += " -Wl,--emit-relocs"
    v.CFLAGS_NODIST += " -fno-pie"
    v.LINKCC += " -fno-pie -no-pie"

    v.LLVM_BOLT = pyconf.check_prog("llvm-bolt", path=v.llvm_path, default="")
    if v.LLVM_BOLT and pyconf.is_executable(v.LLVM_BOLT):
        pyconf.result("Found llvm-bolt")
    else:
        pyconf.error(
            "error: llvm-bolt is required for a --enable-bolt build but could not be found."
        )

    v.MERGE_FDATA = pyconf.check_prog(
        "merge-fdata", path=v.llvm_path, default=""
    )
    if v.MERGE_FDATA and pyconf.is_executable(v.MERGE_FDATA):
        pyconf.result("Found merge-fdata")
    else:
        pyconf.error(
            "error: merge-fdata is required for a --enable-bolt build but could not be found."
        )


def _setup_bolt_flags(v):
    """Set BOLT_BINARIES and BOLT flag variables."""
    v.export("BOLT_BINARIES")
    v.BOLT_BINARIES = "$(BUILDPYTHON)"
    if v.enable_shared:
        v.BOLT_BINARIES += " $(INSTSONAME)"

    pyconf.env_var(
        "BOLT_COMMON_FLAGS",
        "Common arguments to llvm-bolt when instrumenting and applying",
    )
    pyconf.checking("BOLT_COMMON_FLAGS")
    if not v.BOLT_COMMON_FLAGS:
        v.BOLT_COMMON_FLAGS = (
            "-update-debug-sections "
            "-skip-funcs=_PyEval_EvalFrameDefault,"
            "sre_ucs1_match/1,sre_ucs2_match/1,sre_ucs4_match/1,"
            "sre_ucs1_match.lto_priv.0/1,sre_ucs2_match.lto_priv.0/1,sre_ucs4_match.lto_priv.0/1"
        )

    pyconf.env_var(
        "BOLT_INSTRUMENT_FLAGS",
        "Arguments to llvm-bolt when instrumenting binaries",
    )
    pyconf.checking("BOLT_INSTRUMENT_FLAGS")
    if not v.BOLT_INSTRUMENT_FLAGS:
        v.BOLT_INSTRUMENT_FLAGS = v.BOLT_COMMON_FLAGS
    pyconf.result(v.BOLT_INSTRUMENT_FLAGS)

    pyconf.env_var(
        "BOLT_APPLY_FLAGS",
        "Arguments to llvm-bolt when creating a BOLT optimized binary",
    )
    pyconf.checking("BOLT_APPLY_FLAGS")
    if not v.BOLT_APPLY_FLAGS:
        v.BOLT_APPLY_FLAGS = (
            f"{v.BOLT_COMMON_FLAGS} "
            "-reorder-blocks=ext-tsp "
            "-reorder-functions=cdsort "
            "-split-functions "
            "-icf=1 "
            "-inline-all "
            "-split-eh "
            "-reorder-functions-use-hot-size "
            "-peepholes=none "
            "-jump-tables=aggressive "
            "-inline-ap "
            "-indirect-call-promotion=all "
            "-dyno-stats "
            "-use-gnu-stack "
            "-frame-opt=hot"
        )
    pyconf.result(v.BOLT_APPLY_FLAGS)
    v.export("BOLT_COMMON_FLAGS")
    v.export("BOLT_INSTRUMENT_FLAGS")
    v.export("BOLT_APPLY_FLAGS")
