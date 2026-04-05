"""conf_macos — Apple platform configuration (macOS, iOS, frameworks).

Handles iOS cross-compilation tools; --enable-universalsdk and
--with-universal-archs; --with-framework-name and --enable-framework
(Darwin and iOS variants); --with-app-store-compliance; macOS/iOS
deployment targets and initial CFLAGS/LDFLAGS; macOS SDKROOT detection
and compiler selection; --enable-framework (WITH_NEXT_FRAMEWORK);
--with-dsymutil; dyld detection; Darwin-specific compiler flags;
universal architecture flags; and MACOSX_DEPLOYMENT_TARGET.
"""

from __future__ import annotations

import os

import pyconf

ENABLE_UNIVERSALSDK = pyconf.arg_enable(
    "universalsdk",
    metavar="SDKDIR",
    help="create a universal binary build; SDKDIR = macOS SDK path (default: no)",
)
WITH_UNIVERSAL_ARCHS = pyconf.arg_with(
    "universal-archs",
    metavar="ARCH",
    help=(
        "kind of macOS universal binary to create; only valid with --enable-universalsdk; "
        'choices: "universal2", "intel-64", "intel-32", "intel", "32-bit", '
        '"64-bit", "3-way", "all"'
    ),
)
WITH_FRAMEWORK_NAME = pyconf.arg_with(
    "framework-name",
    metavar="FRAMEWORK",
    help="name for the Python framework on macOS (only with --enable-framework, default: Python)",
)
ENABLE_FRAMEWORK = pyconf.arg_enable(
    "framework",
    metavar="INSTALLDIR",
    help="create a Python.framework rather than a traditional Unix install (default: no)",
)
WITH_APP_STORE_COMPLIANCE = pyconf.arg_with(
    "app-store-compliance",
    metavar="PATCH-FILE",
    help="enable patches required for app store compliance; optional PATCH-FILE for custom patch",
)
WITH_DSYMUTIL = pyconf.arg_with(
    "dsymutil",
    help="link debug information into final executable with dsymutil in macOS (default is no)",
    false_is="no",
)


# ---------------------------------------------------------------------------
# From conf_platform
# ---------------------------------------------------------------------------


def setup_ios_cross_tools(v):
    """Seed AR/CC/CPP/CXX for iOS cross-compilation."""
    if not v.AR:
        if pyconf.fnmatch(v.host, "aarch64-apple-ios*-simulator"):
            v.AR = "arm64-apple-ios-simulator-ar"
        elif pyconf.fnmatch(v.host, "aarch64-apple-ios*"):
            v.AR = "arm64-apple-ios-ar"
        elif pyconf.fnmatch(v.host, "x86_64-apple-ios*-simulator"):
            v.AR = "x86_64-apple-ios-simulator-ar"

    if not v.CC:
        if pyconf.fnmatch(v.host, "aarch64-apple-ios*-simulator"):
            v.CC = "arm64-apple-ios-simulator-clang"
        elif pyconf.fnmatch(v.host, "aarch64-apple-ios*"):
            v.CC = "arm64-apple-ios-clang"
        elif pyconf.fnmatch(v.host, "x86_64-apple-ios*-simulator"):
            v.CC = "x86_64-apple-ios-simulator-clang"

    if not v.CPP:
        if pyconf.fnmatch(v.host, "aarch64-apple-ios*-simulator"):
            v.CPP = "arm64-apple-ios-simulator-cpp"
        elif pyconf.fnmatch(v.host, "aarch64-apple-ios*"):
            v.CPP = "arm64-apple-ios-cpp"
        elif pyconf.fnmatch(v.host, "x86_64-apple-ios*-simulator"):
            v.CPP = "x86_64-apple-ios-simulator-cpp"

    if not v.CXX:
        if pyconf.fnmatch(v.host, "aarch64-apple-ios*-simulator"):
            v.CXX = "arm64-apple-ios-simulator-clang++"
        elif pyconf.fnmatch(v.host, "aarch64-apple-ios*"):
            v.CXX = "arm64-apple-ios-clang++"
        elif pyconf.fnmatch(v.host, "x86_64-apple-ios*-simulator"):
            v.CXX = "x86_64-apple-ios-simulator-clang++"


def setup_universalsdk(v):
    """Handle --enable-universalsdk and --with-universal-archs."""
    pyconf.checking("for --enable-universalsdk")
    usdk_raw = ENABLE_UNIVERSALSDK.value

    v.UNIVERSALSDK = ""
    enable_universalsdk = None
    if usdk_raw is not None:
        if usdk_raw == "yes":
            # Locate the best usable SDK, see Mac/README for more information
            # Use cmd_status so xcodebuild failure is ignored (matches
            # configure.ac: /usr/bin/xcodebuild ... 2>/dev/null).
            _, val = pyconf.cmd_status(
                ["/usr/bin/xcodebuild", "-version", "-sdk", "macosx", "Path"]
            )
            if ".sdk" not in val:
                val = "/Developer/SDKs/MacOSX10.4u.sdk"
                if not pyconf.path_is_dir(val):
                    val = "/"
        elif usdk_raw == "no":
            val = None
        else:
            val = usdk_raw
        if val is None:
            v.UNIVERSALSDK = ""
            enable_universalsdk = ""
        else:
            v.UNIVERSALSDK = str(val)
            enable_universalsdk = val
            if not pyconf.path_is_dir(v.UNIVERSALSDK):
                pyconf.error(
                    f"error: --enable-universalsdk specifies non-existing SDK: {v.UNIVERSALSDK}"
                )

    if v.UNIVERSALSDK:
        pyconf.result(v.UNIVERSALSDK)
    else:
        pyconf.result("no")
    v.export("UNIVERSALSDK")

    v.enable_universalsdk = enable_universalsdk

    v.export("ARCH_RUN_32BIT", "")

    # For backward compatibility reasons we prefer to select '32-bit' if available,
    # otherwise use 'intel'
    v.UNIVERSAL_ARCHS = "32-bit"
    if pyconf.platform_system() == "Darwin" and v.UNIVERSALSDK:
        dylib = pyconf.path_join([v.UNIVERSALSDK, "usr/lib/libSystem.dylib"])
        _, file_out = pyconf.cmd_status(["/usr/bin/file", "-L", dylib])
        if "ppc" not in file_out:
            v.UNIVERSAL_ARCHS = "intel"

    v.export("LIPO_32BIT_FLAGS", "")
    v.export("LIPO_INTEL64_FLAGS", "")

    # --- --with-universal-archs ---
    pyconf.checking("for --with-universal-archs")
    if WITH_UNIVERSAL_ARCHS.given:
        v.UNIVERSAL_ARCHS = WITH_UNIVERSAL_ARCHS.value_or(v.UNIVERSAL_ARCHS)

    if v.UNIVERSALSDK:
        pyconf.result(v.UNIVERSAL_ARCHS)
    else:
        pyconf.result("no")


def setup_framework_name(v):
    """Handle --with-framework-name option."""
    fw_name = WITH_FRAMEWORK_NAME.value
    if fw_name is not None and fw_name not in ("yes", "no"):
        v.PYTHONFRAMEWORK = fw_name
        v.PYTHONFRAMEWORKDIR = f"{fw_name}.framework"
        v.PYTHONFRAMEWORKIDENTIFIER = f"org.python.{fw_name.lower()}"
    else:
        v.PYTHONFRAMEWORK = "Python"
        v.PYTHONFRAMEWORKDIR = "Python.framework"
        v.PYTHONFRAMEWORKIDENTIFIER = "org.python.python"


def setup_framework(v):
    """Handle --enable-framework and all FRAMEWORK* variables."""
    fw_raw = ENABLE_FRAMEWORK.value
    v.enable_framework = fw_raw

    v.PYTHONFRAMEWORKPREFIX = ""
    v.PYTHONFRAMEWORKINSTALLDIR = ""
    v.PYTHONFRAMEWORKINSTALLNAMEPREFIX = ""
    v.RESSRCDIR = ""
    v.FRAMEWORKINSTALLFIRST = ""
    v.FRAMEWORKINSTALLLAST = ""
    v.FRAMEWORKALTINSTALLFIRST = ""
    v.FRAMEWORKALTINSTALLLAST = ""
    v.FRAMEWORKPYTHONW = ""
    v.FRAMEWORKUNIXTOOLSPREFIX = ""
    v.FRAMEWORKINSTALLAPPSPREFIX = ""
    v.INSTALLTARGETS = "commoninstall bininstall maninstall"

    ac_default_prefix = "/usr/local"

    if fw_raw is not None:
        val = fw_raw
        if fw_raw == "yes":
            if v.ac_sys_system == "Darwin":
                val = "/Library/Frameworks"
            elif v.ac_sys_system == "iOS":
                val = "Platforms/Apple/iOS/Frameworks/$(MULTIARCH)"
            else:
                pyconf.error("error: Unknown platform for framework build")

        if val == "no":
            if v.ac_sys_system == "iOS":
                pyconf.error("error: iOS builds must use --enable-framework")
            v.PYTHONFRAMEWORK = ""
            v.PYTHONFRAMEWORKDIR = "no-framework"
            v.FRAMEWORKUNIXTOOLSPREFIX = (
                v.prefix if v.prefix != "NONE" else ac_default_prefix
            )
            v.enable_framework = ""
        else:
            v.PYTHONFRAMEWORKPREFIX = val
            v.PYTHONFRAMEWORKINSTALLDIR = (
                f"{v.PYTHONFRAMEWORKPREFIX}/{v.PYTHONFRAMEWORKDIR}"
            )

            if v.ac_sys_system == "Darwin":
                _setup_framework_darwin(v, val, ac_default_prefix)
            elif v.ac_sys_system == "iOS":
                _setup_framework_ios(v)
            else:
                pyconf.error("error: Unknown platform for framework build")
    else:
        if v.ac_sys_system == "iOS":
            pyconf.error("error: iOS builds must use --enable-framework")
        v.PYTHONFRAMEWORK = ""
        v.PYTHONFRAMEWORKDIR = "no-framework"
        v.FRAMEWORKUNIXTOOLSPREFIX = (
            v.prefix if v.prefix != "NONE" else ac_default_prefix
        )
        v.enable_framework = ""

    _export_framework_vars(v)


def _setup_framework_darwin(v, val, ac_default_prefix):
    """Configure framework variables for macOS Darwin builds."""
    v.FRAMEWORKINSTALLFIRST = "frameworkinstallversionedstructure"
    v.FRAMEWORKALTINSTALLFIRST = "frameworkinstallversionedstructure "
    v.FRAMEWORKINSTALLLAST = (
        "frameworkinstallmaclib frameworkinstallapps frameworkinstallunixtools"
    )
    v.FRAMEWORKALTINSTALLLAST = "frameworkinstallmaclib frameworkinstallapps frameworkaltinstallunixtools"
    v.FRAMEWORKPYTHONW = "frameworkpythonw"
    v.FRAMEWORKINSTALLAPPSPREFIX = "/Applications"
    v.INSTALLTARGETS = "commoninstall bininstall maninstall"
    v.FRAMEWORKUNIXTOOLSPREFIX = (
        v.prefix if v.prefix != "NONE" else ac_default_prefix
    )

    if val.startswith("/System"):
        v.FRAMEWORKINSTALLAPPSPREFIX = "/Applications"
        if v.prefix == "NONE":
            # See configure.ac for details
            v.FRAMEWORKUNIXTOOLSPREFIX = "/usr"
    elif val.startswith("/Library"):
        v.FRAMEWORKINSTALLAPPSPREFIX = "/Applications"
    elif val.endswith("/Library/Frameworks"):
        mdir = pyconf.path_parent(pyconf.path_parent(val))
        v.FRAMEWORKINSTALLAPPSPREFIX = f"{mdir}/Applications"
        if v.prefix == "NONE":
            # User hasn't specified the --prefix option, but wants to install
            # the framework in a non-default location, ensure that the compatibility
            # links get installed relative to that prefix as well instead of in /usr/local.
            v.FRAMEWORKUNIXTOOLSPREFIX = mdir

    v.prefix = f"{v.PYTHONFRAMEWORKINSTALLDIR}/Versions/{v.VERSION}"
    v.PYTHONFRAMEWORKINSTALLNAMEPREFIX = v.prefix
    v.RESSRCDIR = "Mac/Resources/framework"
    # Add files for Mac specific code to the list of output files
    pyconf.config_files(
        [
            "Mac/Makefile",
            "Mac/PythonLauncher/Makefile",
            "Mac/Resources/framework/Info.plist",
            "Mac/Resources/app/Info.plist",
        ]
    )


def _setup_framework_ios(v):
    """Configure framework variables for iOS builds."""
    v.FRAMEWORKINSTALLFIRST = "frameworkinstallunversionedstructure"
    v.FRAMEWORKALTINSTALLFIRST = "frameworkinstallunversionedstructure "
    v.FRAMEWORKINSTALLLAST = "frameworkinstallmobileheaders"
    v.FRAMEWORKALTINSTALLLAST = "frameworkinstallmobileheaders"
    v.FRAMEWORKPYTHONW = ""
    v.INSTALLTARGETS = "libinstall inclinstall sharedinstall"
    v.prefix = v.PYTHONFRAMEWORKPREFIX
    v.PYTHONFRAMEWORKINSTALLNAMEPREFIX = f"@rpath/{v.PYTHONFRAMEWORKDIR}"
    v.RESSRCDIR = "Platforms/Apple/iOS/Resources"
    pyconf.config_files(["Platforms/Apple/iOS/Resources/Info.plist"])


def _export_framework_vars(v):
    """Export all framework-related variables."""
    v.export("PYTHONFRAMEWORK")
    v.export("PYTHONFRAMEWORKIDENTIFIER")
    v.export("PYTHONFRAMEWORKDIR")
    v.export("PYTHONFRAMEWORKPREFIX")
    v.export("PYTHONFRAMEWORKINSTALLDIR")
    v.export("PYTHONFRAMEWORKINSTALLNAMEPREFIX")
    v.export("RESSRCDIR")
    v.export("FRAMEWORKINSTALLFIRST")
    v.export("FRAMEWORKINSTALLLAST")
    v.export("FRAMEWORKALTINSTALLFIRST")
    v.export("FRAMEWORKALTINSTALLLAST")
    v.export("FRAMEWORKPYTHONW")
    v.export("FRAMEWORKUNIXTOOLSPREFIX")
    v.export("FRAMEWORKINSTALLAPPSPREFIX")
    v.export("INSTALLTARGETS")

    pyconf.define_unquoted(
        "_PYTHONFRAMEWORK", f'"{v.PYTHONFRAMEWORK}"', "framework name"
    )


def setup_app_store_compliance(v):
    """Handle --with-app-store-compliance option."""
    pyconf.checking("for --with-app-store-compliance")
    asc_raw = WITH_APP_STORE_COMPLIANCE.value

    if asc_raw is not None and asc_raw != "no":
        if asc_raw == "yes":
            if v.ac_sys_system in ("Darwin", "iOS"):
                # iOS is able to share the macOS patch
                v.APP_STORE_COMPLIANCE_PATCH = (
                    "Mac/Resources/app-store-compliance.patch"
                )
                pyconf.result("applying default app store compliance patch")
            else:
                pyconf.error(
                    f"error: no default app store compliance patch available for {v.ac_sys_system}"
                )
        else:
            v.APP_STORE_COMPLIANCE_PATCH = asc_raw
            pyconf.result("applying custom app store compliance patch")
    else:
        if v.ac_sys_system == "iOS":
            # Always apply the compliance patch on iOS; we can use the macOS patch
            v.APP_STORE_COMPLIANCE_PATCH = (
                "Mac/Resources/app-store-compliance.patch"
            )
            pyconf.result("applying default app store compliance patch")
        else:
            v.APP_STORE_COMPLIANCE_PATCH = ""
            pyconf.result("not patching for app store compliance")
    v.export("APP_STORE_COMPLIANCE_PATCH")


def setup_deployment_targets_and_flags(v):
    """Set macOS/iOS deployment targets and initialize compiler flags."""
    v.export("CONFIGURE_MACOSX_DEPLOYMENT_TARGET", "")
    v.export("EXPORT_MACOSX_DEPLOYMENT_TARGET", "#")
    v.export("IPHONEOS_DEPLOYMENT_TARGET")
    if not v.is_set("IPHONEOS_DEPLOYMENT_TARGET"):
        v.IPHONEOS_DEPLOYMENT_TARGET = ""

    v.export("CFLAGS", v.CFLAGS)
    v.export("CPPFLAGS", v.CPPFLAGS)
    v.export("LDFLAGS", v.LDFLAGS)
    v.export("LIBS", v.LIBS)
    v.export("BASECFLAGS", "")
    v.export("CFLAGS_NODIST", v.CFLAGS_NODIST)
    v.export("LDFLAGS_NODIST", v.LDFLAGS_NODIST)

    if pyconf.fnmatch(v.host, "wasm64-*-emscripten"):
        v.CFLAGS += " -sMEMORY64=1"
        v.LDFLAGS += " -sMEMORY64=1"

    if v.ac_sys_system == "iOS":
        v.CFLAGS += f" -mios-version-min={v.IPHONEOS_DEPLOYMENT_TARGET}"
        v.LDFLAGS += f" -mios-version-min={v.IPHONEOS_DEPLOYMENT_TARGET}"


def setup_macos_compiler(v):
    """macOS: detect SDKROOT and select compiler (gcc/clang).

    Compiler selection on macOS is more complicated than AC_PROG_CC can handle,
    see Mac/README for more information.
    """
    if v.ac_sys_system != "Darwin":
        return

    HAS_XCRUN = bool(pyconf.find_prog("xcrun"))
    pyconf.checking("macOS SDKROOT")
    if not v.SDKROOT:
        if HAS_XCRUN:
            v.SDKROOT = pyconf.cmd_output(["xcrun", "--show-sdk-path"])
        else:
            v.SDKROOT = "/"
    pyconf.result(v.SDKROOT)

    if not v.CC:
        found_gcc = pyconf.find_prog("gcc")
        found_clang = pyconf.find_prog("clang")
        if found_gcc and found_clang:
            ver = pyconf.cmd_output([found_gcc, "--version"])
            if "llvm-gcc" in ver:
                v.CC = found_clang
                v.CXX = found_clang + "++"
        elif found_clang and not found_gcc:
            v.CC = found_clang
            v.CXX = found_clang + "++"
        elif not found_gcc and not found_clang:
            found_clang = pyconf.cmd_output(
                ["/usr/bin/xcrun", "-find", "clang"]
            )
            if found_clang:
                v.CC = found_clang
                v.CXX = pyconf.cmd_output(
                    ["/usr/bin/xcrun", "-find", "clang++"]
                )


def setup_next_framework(v):
    """Handle --enable-framework option."""
    pyconf.checking("for --enable-framework")
    if v.enable_framework:
        v.BASECFLAGS += " -fno-common -dynamic"
        pyconf.define(
            "WITH_NEXT_FRAMEWORK",
            1,
            "Define if you want to produce an OpenStep/Rhapsody framework "
            "(shared library plus accessory files).",
        )
        pyconf.result("yes")
        if v.enable_shared:
            pyconf.error(
                "Specifying both --enable-shared and --enable-framework is not supported, "
                "use only --enable-framework instead"
            )
    else:
        pyconf.result("no")


def setup_dsymutil(v):
    """Handle --with-dsymutil option."""
    DSYMUTIL_PATH = ""
    dsymutil_enabled = WITH_DSYMUTIL.process_bool()
    if dsymutil_enabled and v.MACHDEP != "darwin":
        pyconf.error("dsymutil debug linking is only available in macOS.")
    DSYMUTIL = "true" if dsymutil_enabled else ""

    if DSYMUTIL:
        DSYMUTIL_PATH = pyconf.find_prog("dsymutil")
        if not DSYMUTIL_PATH:
            pyconf.error("dsymutil command not found on $PATH")

    v.export("DSYMUTIL", DSYMUTIL)
    v.export("DSYMUTIL_PATH", DSYMUTIL_PATH)


def check_dyld(v):
    """Check for dyld on Darwin."""
    pyconf.checking("for dyld")
    if f"{v.ac_sys_system}/{v.ac_sys_release}".startswith("Darwin/"):
        pyconf.define(
            "WITH_DYLD",
            1,
            "Define if you want to use the new-style (Openstep, Rhapsody, MacOS) "
            "dynamic linker (dyld) instead of the old-style (NextStep) dynamic "
            "linker (rld). Dyld is necessary to support frameworks.",
        )
        pyconf.result("always on for Darwin")
    else:
        pyconf.result("no")


def _setup_darwin_flags(v):
    """Darwin-specific compiler flags, universal SDK, and deployment target."""
    # -Wno-long-double, -no-cpp-precomp, and -mno-fused-madd
    # used to be here, but non-Apple gcc doesn't accept them.
    pyconf.checking("which compiler should be used")
    if v.UNIVERSALSDK.endswith("/MacOSX10.4u.sdk"):
        # Build using 10.4 SDK, force usage of gcc when the compiler is gcc,
        # otherwise the user will get very confusing error messages when building on OSX 10.6
        v.CC = "gcc-4.0"
        v.CPP = "cpp-4.0"
    pyconf.result(v.CC)

    # Error on unguarded use of new symbols, which will fail at runtime for
    # users on older versions of macOS
    if pyconf.check_compile_flag(
        "-Wunguarded-availability", extra_flags=["-Werror"]
    ):
        v.CFLAGS_NODIST += " -Werror=unguarded-availability"

    v.LIPO_INTEL64_FLAGS = ""

    if v.enable_universalsdk:
        _setup_universal_archs(v)

    v.export("ARCH_TRIPLES")

    _setup_macos_deployment_target(v)

    pyconf.checking("if specified universal architectures work")
    if not pyconf.link_check("#include <stdio.h>", 'printf("%d", 42);'):
        pyconf.result("no")
        pyconf.error(
            "error: check config.log and use the '--with-universal-archs' option"
        )
    else:
        pyconf.result("yes")


def _setup_universal_archs(v):
    """Set UNIVERSAL_ARCH_FLAGS based on --with-universal-archs value."""
    v.ARCH_TRIPLES = ""
    ua = v.UNIVERSAL_ARCHS
    if ua == "32-bit":
        v.UNIVERSAL_ARCH_FLAGS = "-arch ppc -arch i386"
        v.LIPO_32BIT_FLAGS = ""
        v.ARCH_RUN_32BIT = ""
        v.ARCH_TRIPLES = "ppc-apple-darwin i386-apple-darwin"
    elif ua == "64-bit":
        v.UNIVERSAL_ARCH_FLAGS = "-arch ppc64 -arch x86_64"
        v.LIPO_32BIT_FLAGS = ""
        v.ARCH_RUN_32BIT = "true"
        v.ARCH_TRIPLES = "ppc64-apple-darwin x86_64-apple-darwin"
    elif ua == "all":
        v.UNIVERSAL_ARCH_FLAGS = (
            "-arch i386 -arch ppc -arch ppc64 -arch x86_64"
        )
        v.LIPO_32BIT_FLAGS = "-extract ppc7400 -extract i386"
        v.ARCH_RUN_32BIT = "/usr/bin/arch -i386 -ppc"
        v.ARCH_TRIPLES = "i386-apple-darwin ppc-apple-darwin ppc64-apple-darwin x86_64-apple-darwin"
    elif ua == "universal2":
        v.UNIVERSAL_ARCH_FLAGS = "-arch arm64 -arch x86_64"
        v.LIPO_32BIT_FLAGS = ""
        v.LIPO_INTEL64_FLAGS = "-extract x86_64"
        v.ARCH_RUN_32BIT = "true"
        v.ARCH_TRIPLES = "aarch64-apple-darwin x86_64-apple-darwin"
    elif ua == "intel":
        v.UNIVERSAL_ARCH_FLAGS = "-arch i386 -arch x86_64"
        v.LIPO_32BIT_FLAGS = "-extract i386"
        v.ARCH_RUN_32BIT = "/usr/bin/arch -i386"
        v.ARCH_TRIPLES = "i386-apple-darwin x86_64-apple-darwin"
    elif ua == "intel-32":
        v.UNIVERSAL_ARCH_FLAGS = "-arch i386"
        v.LIPO_32BIT_FLAGS = ""
        v.ARCH_RUN_32BIT = ""
        v.ARCH_TRIPLES = "i386-apple-darwin"
    elif ua == "intel-64":
        v.UNIVERSAL_ARCH_FLAGS = "-arch x86_64"
        v.LIPO_32BIT_FLAGS = ""
        v.ARCH_RUN_32BIT = "true"
        v.ARCH_TRIPLES = "x86_64-apple-darwin"
    elif ua == "3-way":
        v.UNIVERSAL_ARCH_FLAGS = "-arch i386 -arch ppc -arch x86_64"
        v.LIPO_32BIT_FLAGS = "-extract ppc7400 -extract i386"
        v.ARCH_RUN_32BIT = "/usr/bin/arch -i386 -ppc"
        v.ARCH_TRIPLES = (
            "i386-apple-darwin ppc-apple-darwin x86_64-apple-darwin"
        )
    else:
        pyconf.error(
            "error: proper usage is "
            "--with-universal-arch=universal2|32-bit|64-bit|all|intel|3-way"
        )

    if v.UNIVERSALSDK != "/":
        v.CFLAGS = (
            f"{v.UNIVERSAL_ARCH_FLAGS} -isysroot {v.UNIVERSALSDK} {v.CFLAGS}"
        )
        v.LDFLAGS = (
            f"{v.UNIVERSAL_ARCH_FLAGS} -isysroot {v.UNIVERSALSDK} {v.LDFLAGS}"
        )
        v.CPPFLAGS = f"-isysroot {v.UNIVERSALSDK} {v.CPPFLAGS}"
    else:
        v.CFLAGS = f"{v.UNIVERSAL_ARCH_FLAGS} {v.CFLAGS}"
        v.LDFLAGS = f"{v.UNIVERSAL_ARCH_FLAGS} {v.LDFLAGS}"

    v.export("LIPO_32BIT_FLAGS")
    v.export("LIPO_INTEL64_FLAGS")
    v.export("ARCH_RUN_32BIT")


def _setup_macos_deployment_target(v):
    """Determine and set MACOSX_DEPLOYMENT_TARGET.

    Calculate an appropriate deployment target for this build:
    The deployment target value is used explicitly to enable certain
    features (such as builtin libedit support for readline) through the use
    of Apple's Availability Macros and is used as a component of the string
    returned by distutils.get_platform().

    Use the value from:
    1. the MACOSX_DEPLOYMENT_TARGET environment variable if specified
    2. the operating system version of the build machine if >= 10.6
    3. If running on OS X 10.3 through 10.5, use the legacy tests below
       to pick either 10.3, 10.4, or 10.5 as the target.
    4. If we are running on OS X 10.2 or earlier, good luck!
    """
    pyconf.checking("which MACOSX_DEPLOYMENT_TARGET to use")
    sw = pyconf.cmd_output(["sw_vers", "-productVersion"])
    parts = sw.split(".")
    cur_target_major = int(parts[0]) if parts else 0
    cur_target_minor = int(parts[1]) if len(parts) > 1 else 0
    cur_target = f"{cur_target_major}.{cur_target_minor}"

    if cur_target_major == 10 and 3 <= cur_target_minor <= 5:
        # OS X 10.3 through 10.5
        cur_target = "10.3"
        if v.enable_universalsdk:
            if v.UNIVERSAL_ARCHS in (
                "all",
                "3-way",
                "intel",
                "64-bit",
            ):
                # These configurations were first supported in 10.5
                cur_target = "10.5"
        else:
            arch = pyconf.cmd_output(["/usr/bin/arch"])
            if arch == "i386":
                # 10.4 was the first release to support Intel archs
                cur_target = "10.4"

    v.CONFIGURE_MACOSX_DEPLOYMENT_TARGET = (
        v.MACOSX_DEPLOYMENT_TARGET or cur_target
    )

    # Make sure that MACOSX_DEPLOYMENT_TARGET is set in the environment with a
    # value that is the same as what we'll use in the Makefile to ensure that
    # we'll get the same compiler environment during configure and build time.
    v.MACOSX_DEPLOYMENT_TARGET = v.CONFIGURE_MACOSX_DEPLOYMENT_TARGET
    os.environ["MACOSX_DEPLOYMENT_TARGET"] = v.MACOSX_DEPLOYMENT_TARGET
    v.EXPORT_MACOSX_DEPLOYMENT_TARGET = ""
    v.export("MACOSX_DEPLOYMENT_TARGET")
    v.export("CONFIGURE_MACOSX_DEPLOYMENT_TARGET")
    v.export("EXPORT_MACOSX_DEPLOYMENT_TARGET")
    pyconf.result(v.MACOSX_DEPLOYMENT_TARGET)
