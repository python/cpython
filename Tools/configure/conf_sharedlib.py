"""conf_sharedlib — Shared-lib linking, perf trampoline.

Determines shared library extension (SHLIB_SUFFIX); computes LDSHARED,
LDCXXSHARED, BLDSHARED for each platform; sets CCSHARED (-fPIC etc.);
determines LINKFORSHARED for the main executable; exports shared-lib
variables (CFLAGSFORSHARED, SHLIBS); and configures perf trampoline
support.
"""

import pyconf


def setup_shared_lib_suffix(v):
    """Determine the shared library file extension."""
    pyconf.checking("the extension of shared libraries")
    if not v.SHLIB_SUFFIX:
        if v.ac_sys_system.startswith(("hp", "HP")):
            v.SHLIB_SUFFIX = (
                ".so" if pyconf.platform_machine() == "ia64" else ".sl"
            )
        elif v.ac_sys_system.startswith("CYGWIN"):
            v.SHLIB_SUFFIX = ".dll"
        else:
            v.SHLIB_SUFFIX = ".so"
    pyconf.result(v.SHLIB_SUFFIX)


def setup_ldshared(v):
    """Determine LDSHARED, LDCXXSHARED, and BLDSHARED flags."""
    pyconf.checking("LDSHARED")
    if not v.LDSHARED:
        sr = f"{v.ac_sys_system}/{v.ac_sys_release}"
        if sr.startswith("AIX"):
            v.BLDSHARED = "Modules/ld_so_aix $(CC) -bI:Modules/python.exp"
            v.LDSHARED = "$(LIBPL)/ld_so_aix $(CC) -bI:$(LIBPL)/python.exp"
        elif sr.startswith("SunOS/5"):
            if v.ac_cv_gcc_compat:
                v.LDSHARED = "$(CC) -shared"
                v.LDCXXSHARED = "$(CXX) -shared"
            else:
                v.LDSHARED = "$(CC) -G"
                v.LDCXXSHARED = "$(CXX) -G"
        elif v.ac_sys_system.startswith(("hp", "HP")):
            if v.ac_cv_gcc_compat:
                v.LDSHARED = "$(CC) -shared"
                v.LDCXXSHARED = "$(CXX) -shared"
            else:
                v.LDSHARED = "$(CC) -b"
                v.LDCXXSHARED = "$(CXX) -b"
        elif sr.startswith("Darwin/1.3"):
            v.LDSHARED = "$(CC) -bundle"
            v.LDCXXSHARED = "$(CXX) -bundle"
            if v.enable_framework:
                v.BLDSHARED = f"{v.LDSHARED} $(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
                v.LDSHARED += " $(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
                v.LDCXXSHARED += " $(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
            else:
                v.LDSHARED += " -undefined suppress"
                v.LDCXXSHARED += " -undefined suppress"
        elif sr.startswith(("Darwin/1.4", "Darwin/5.", "Darwin/6.")):
            v.LDSHARED = "$(CC) -bundle"
            v.LDCXXSHARED = "$(CXX) -bundle"
            if v.enable_framework:
                v.BLDSHARED = f"{v.LDSHARED} $(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
                v.LDSHARED += " $(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
                v.LDCXXSHARED += " $(PYTHONFRAMEWORKPREFIX)/$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
            else:
                v.BLDSHARED = f"{v.LDSHARED} -bundle_loader $(BUILDPYTHON)"
                v.LDSHARED += (
                    " -bundle_loader $(BINDIR)/python$(VERSION)$(EXE)"
                )
                v.LDCXXSHARED += (
                    " -bundle_loader $(BINDIR)/python$(VERSION)$(EXE)"
                )
        elif sr.startswith("Darwin/"):
            dt = v.MACOSX_DEPLOYMENT_TARGET or ""
            dt_parts = dt.split(".")
            dt_major = int(dt_parts[0]) if dt_parts else 0
            dt_minor = int(dt_parts[1]) if len(dt_parts) > 1 else 0
            if dt_major == 10 and dt_minor <= 2:
                pyconf.error(
                    f"MACOSX_DEPLOYMENT_TARGET too old ({dt}), "
                    "only 10.3 or later is supported"
                )
            v.LDSHARED = "$(CC) -bundle -undefined dynamic_lookup"
            v.LDCXXSHARED = "$(CXX) -bundle -undefined dynamic_lookup"
            v.BLDSHARED = v.LDSHARED
        elif sr.startswith("iOS/"):
            v.LDSHARED = "$(CC) -dynamiclib -F . -framework $(PYTHONFRAMEWORK)"
            v.LDCXXSHARED = (
                "$(CXX) -dynamiclib -F . -framework $(PYTHONFRAMEWORK)"
            )
            v.BLDSHARED = v.LDSHARED
        elif v.ac_sys_system in ("Emscripten", "WASI"):
            v.LDSHARED = "$(CC) -shared"
            v.LDCXXSHARED = "$(CXX) -shared"
        elif v.ac_sys_system.startswith(
            ("Linux", "GNU", "QNX", "VxWorks", "Haiku")
        ):
            v.LDSHARED = "$(CC) -shared"
            v.LDCXXSHARED = "$(CXX) -shared"
        elif v.ac_sys_system.startswith("FreeBSD"):
            has_elf = pyconf.shell(
                f'echo "" | {v.CC} -dM -E - | grep __ELF__',
                exports=["_elf_out"],
            )
            if has_elf._elf_out:
                v.LDSHARED = "$(CC) -shared"
                v.LDCXXSHARED = "$(CXX) -shared"
            else:
                v.LDSHARED = "ld -Bshareable"
        elif v.ac_sys_system.startswith("OpenBSD"):
            has_elf = pyconf.shell(
                f'echo "" | {v.CC} -dM -E - | grep __ELF__',
                exports=["_elf_out"],
            )
            if has_elf._elf_out:
                v.LDSHARED = "$(CC) -shared $(CCSHARED)"
                v.LDCXXSHARED = "$(CXX) -shared $(CCSHARED)"
            else:
                rel = v.ac_sys_release
                if pyconf.fnmatch_any(
                    rel, ["0.*", "1.*", "2.[0-7]", "2.[0-7].*"]
                ):
                    v.LDSHARED = f"ld -Bshareable {v.LDFLAGS}"
                else:
                    v.LDSHARED = "$(CC) -shared $(CCSHARED)"
                    v.LDCXXSHARED = "$(CXX) -shared $(CCSHARED)"
        elif v.ac_sys_system.startswith(("NetBSD", "DragonFly")):
            v.LDSHARED = "$(CC) -shared"
            v.LDCXXSHARED = "$(CXX) -shared"
        elif v.ac_sys_system.startswith(("OpenUNIX", "UnixWare")):
            if v.ac_cv_gcc_compat:
                v.LDSHARED = "$(CC) -shared"
                v.LDCXXSHARED = "$(CXX) -shared"
            else:
                v.LDSHARED = "$(CC) -G"
                v.LDCXXSHARED = "$(CXX) -G"
        elif v.ac_sys_system.startswith("SCO_SV"):
            v.LDSHARED = "$(CC) -Wl,-G,-Bexport"
            v.LDCXXSHARED = "$(CXX) -Wl,-G,-Bexport"
        elif v.ac_sys_system.startswith("CYGWIN"):
            v.LDSHARED = "gcc -shared -Wl,--enable-auto-image-base"
            v.LDCXXSHARED = "g++ -shared -Wl,--enable-auto-image-base"
        else:
            v.LDSHARED = "ld"

    if v.enable_wasm_dynamic_linking and v.ac_sys_system == "Emscripten":
        v.BLDSHARED = "$(CC) -shared -sSIDE_MODULE=1"

    pyconf.result(v.LDSHARED)
    v.LDCXXSHARED = v.LDCXXSHARED or v.LDSHARED

    pyconf.checking("BLDSHARED flags")
    v.BLDSHARED = v.BLDSHARED or v.LDSHARED
    pyconf.result(v.BLDSHARED)


def setup_ccshared(v):
    """Determine CCSHARED flag for position-independent code."""
    pyconf.checking("CCSHARED")
    if not v.CCSHARED:
        if v.ac_sys_system.startswith("SunOS"):
            if v.ac_cv_gcc_compat:
                v.CCSHARED = "-fPIC"
            elif pyconf.cmd_output(["uname", "-p"]) == "sparc":
                v.CCSHARED = "-xcode=pic32"
            else:
                v.CCSHARED = "-Kpic"
        elif v.ac_sys_system.startswith(("hp", "HP")):
            v.CCSHARED = "-fPIC" if v.ac_cv_gcc_compat else "+z"
        elif v.ac_sys_system.startswith(("Linux", "GNU")):
            v.CCSHARED = "-fPIC"
        elif v.ac_sys_system in ("Emscripten", "WASI"):
            if v.enable_wasm_dynamic_linking:
                v.CCSHARED = "-fPIC"
        elif v.ac_sys_system.startswith(
            ("FreeBSD", "NetBSD", "OpenBSD", "DragonFly")
        ):
            v.CCSHARED = "-fPIC"
        elif v.ac_sys_system.startswith("Haiku"):
            v.CCSHARED = "-fPIC"
        elif v.ac_sys_system.startswith(("OpenUNIX", "UnixWare")):
            v.CCSHARED = "-fPIC" if v.ac_cv_gcc_compat else "-KPIC"
        elif v.ac_sys_system.startswith("SCO_SV"):
            v.CCSHARED = "-fPIC" if v.ac_cv_gcc_compat else "-Kpic -belf"
        elif v.ac_sys_system.startswith("VxWorks"):
            v.CCSHARED = (
                "-fpic -D__SO_PICABILINUX__  -ftls-model=global-dynamic"
            )
    pyconf.result(v.CCSHARED)


def setup_linkforshared(v):
    """Determine LINKFORSHARED flags for the main executable."""
    pyconf.checking("LINKFORSHARED")
    if not v.LINKFORSHARED:
        sr = f"{v.ac_sys_system}/{v.ac_sys_release}"
        if sr.startswith("AIX"):
            v.LINKFORSHARED = "-Wl,-bE:Modules/python.exp -lld"
        elif v.ac_sys_system.startswith(("hp", "HP")):
            v.LINKFORSHARED = "-Wl,-E -Wl,+s"
        elif v.ac_sys_system.startswith("Linux-android"):
            v.LINKFORSHARED = "-pie -Xlinker -export-dynamic"
        elif v.ac_sys_system.startswith(("Linux", "GNU")):
            v.LINKFORSHARED = "-Xlinker -export-dynamic"
        elif v.ac_sys_system.startswith(("Darwin", "iOS")):
            v.LINKFORSHARED = (
                f"{v.extra_undefs} -framework CoreFoundation".strip()
            )
            stack_size = "4000000" if v.with_ubsan else "1000000"
            pyconf.define_unquoted(
                "THREAD_STACK_SIZE",
                f"0x{stack_size}",
                "Custom thread stack size depending on chosen sanitizer runtimes.",
            )
            if v.ac_sys_system == "Darwin":
                v.LINKFORSHARED = (
                    f"-Wl,-stack_size,{stack_size} {v.LINKFORSHARED}"
                )
                if v.enable_framework:
                    v.LINKFORSHARED += " $(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
            elif v.ac_sys_system == "iOS":
                v.LINKFORSHARED = (
                    f"-Wl,-stack_size,{stack_size} {v.LINKFORSHARED} "
                    "$(PYTHONFRAMEWORKDIR)/$(PYTHONFRAMEWORK)"
                )
        elif v.ac_sys_system.startswith(("OpenUNIX", "UnixWare")):
            v.LINKFORSHARED = "-Wl,-Bexport"
        elif v.ac_sys_system.startswith("SCO_SV"):
            v.LINKFORSHARED = "-Wl,-Bexport"
        elif v.ac_sys_system.startswith("ReliantUNIX"):
            v.LINKFORSHARED = "-W1 -Blargedynsym"
        elif v.ac_sys_system.startswith(
            ("FreeBSD", "NetBSD", "OpenBSD", "DragonFly")
        ):
            has_elf = pyconf.shell(
                f'_elf_out=$(echo "" | {v.CC} -dM -E - | grep __ELF__)',
                exports=["_elf_out"],
            )
            if has_elf._elf_out:
                v.LINKFORSHARED = "-Wl,--export-dynamic"
        elif sr.startswith("SunOS/5"):
            if v.ac_cv_gcc_compat:
                _, help_out = pyconf.cmd_status([v.CC, "-Xlinker", "--help"])
                if "export-dynamic" in help_out:
                    v.LINKFORSHARED = "-Xlinker --export-dynamic"
        elif v.ac_sys_system.startswith("CYGWIN"):
            if not v.enable_shared:
                v.LINKFORSHARED = "-Wl,--out-implib=$(LDLIBRARY)"
        elif v.ac_sys_system.startswith("QNX"):
            v.LINKFORSHARED = "-Wl,-E -N 2048K"
        elif v.ac_sys_system.startswith("VxWorks"):
            v.LINKFORSHARED = "-Wl,-export-dynamic"
    pyconf.result(v.LINKFORSHARED)


def setup_shared_lib_exports(v):
    """Export shared library variables and set CFLAGSFORSHARED/SHLIBS."""
    v.export("SHLIB_SUFFIX")
    v.export("LDSHARED")
    v.export("LDCXXSHARED")
    v.export("BLDSHARED")
    v.export("CCSHARED")
    v.export("LINKFORSHARED")

    pyconf.checking("CFLAGSFORSHARED")
    if v.LIBRARY != v.LDLIBRARY:
        if v.ac_sys_system.startswith("CYGWIN"):
            v.CFLAGSFORSHARED = ""
        else:
            v.CFLAGSFORSHARED = "$(CCSHARED)"
    else:
        v.CFLAGSFORSHARED = ""
    if v.enable_wasm_dynamic_linking:
        v.CFLAGSFORSHARED = "$(CCSHARED)"
    pyconf.result(v.CFLAGSFORSHARED)
    v.export("CFLAGSFORSHARED")

    pyconf.checking("SHLIBS")
    v.SHLIBS = "$(LIBS)"
    pyconf.result(v.SHLIBS)
    v.export("SHLIBS")


def setup_dynload(v):
    """Select dynamic loading module and set MACHDEP_OBJS."""
    v.export("DLINCLDIR", ".")

    pyconf.check_func("dlopen")
    ac_cv_func_dlopen = pyconf.cache.get("ac_cv_func_dlopen", False)
    # Also update v in case Part 5 didn't set it
    if not v.is_set("ac_cv_func_dlopen"):
        v.ac_cv_func_dlopen = ac_cv_func_dlopen

    pyconf.checking("DYNLOADFILE")
    v.DYNLOADFILE = ""
    if not v.DYNLOADFILE:
        sys_rel = f"{v.ac_sys_system}/{v.ac_sys_release}"
        if pyconf.fnmatch(sys_rel, "[hH][pP]*"):
            v.DYNLOADFILE = "dynload_hpux.o"
        elif v.ac_cv_func_dlopen:
            v.DYNLOADFILE = "dynload_shlib.o"
        else:
            v.DYNLOADFILE = "dynload_stub.o"
    pyconf.result(v.DYNLOADFILE)
    v.export("DYNLOADFILE")

    if v.DYNLOADFILE != "dynload_stub.o":
        pyconf.define(
            "HAVE_DYNAMIC_LOADING",
            1,
            "Defined when any dynamic module loading is enabled.",
        )

    # MACHDEP_OBJS
    extra_machdep_objs = v.extra_machdep_objs
    pyconf.checking("MACHDEP_OBJS")
    v.MACHDEP_OBJS = ""
    if not v.MACHDEP_OBJS:
        v.MACHDEP_OBJS = extra_machdep_objs
    else:
        v.MACHDEP_OBJS = f"{v.MACHDEP_OBJS} {extra_machdep_objs}"
    pyconf.result(v.MACHDEP_OBJS if v.MACHDEP_OBJS else "none")
    v.export("MACHDEP_OBJS")
