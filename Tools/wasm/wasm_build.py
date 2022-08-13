#!/usr/bin/env python3
"""Build script for Python on WebAssembly platforms.

  $ ./Tools/wasm/wasm_builder.py emscripten-browser compile
  $ ./Tools/wasm/wasm_builder.py emscripten-node-dl test
  $ ./Tools/wasm/wasm_builder.py wasi test

Primary build targets are "emscripten-node-dl" (NodeJS, dynamic linking),
"emscripten-browser", and "wasi".

Emscripten builds require a recent Emscripten SDK. The tools looks for an
activated EMSDK environment (". /path/to/emsdk_env.sh"). System packages
(Debian, Homebrew) are not supported.

WASI builds require WASI SDK and wasmtime. The tool looks for 'WASI_SDK_PATH'
and falls back to /opt/wasi-sdk.
"""
import argparse
import enum
import dataclasses
import os
import pathlib
import shlex
import shutil
import subprocess
import sysconfig

# for Python 3.8
from typing import Any, Dict, Callable, Iterable, List, Optional, Union

SRCDIR = pathlib.Path(__file__).parent.parent.parent.absolute()
WASMTOOLS = SRCDIR / "Tools" / "wasm"
BUILDDIR = SRCDIR / "builddir"
CONFIGURE = SRCDIR / "configure"
SETUP_LOCAL = SRCDIR / "Modules" / "Setup.local"

HAS_CCACHE = shutil.which("ccache") is not None

# path to WASI-SDK root
WASI_SDK_PATH = pathlib.Path(os.environ.get("WASI_SDK_PATH", "/opt/wasi-sdk"))

# path to Emscripten SDK config file.
# auto-detect's EMSDK in /opt/emsdk without ". emsdk_env.sh".
EM_CONFIG = pathlib.Path(os.environ.setdefault("EM_CONFIG", "/opt/emsdk/.emscripten"))
# 3.1.16 has broken utime()
EMSDK_MIN_VERSION = (3, 1, 17)
_MISSING = pathlib.PurePath("MISSING")

# WASM_WEBSERVER = WASMTOOLS / "wasmwebserver.py"

CLEAN_SRCDIR = f"""
Builds require a clean source directory. Please use a clean checkout or
run "make clean -C '{SRCDIR}'".
"""

INSTALL_EMSDK = """
wasm32-emscripten builds need Emscripten SDK. Please follow instructions at
https://emscripten.org/docs/getting_started/downloads.html how to install
Emscripten and how to activate the SDK with ". /path/to/emsdk/emsdk_env.sh".

    git clone https://github.com/emscripten-core/emsdk.git /path/to/emsdk
    cd /path/to/emsdk
    ./emsdk install latest
    ./emsdk activate latest
    source /path/to/emsdk_env.sh
"""

INSTALL_WASI_SDK = """
wasm32-wasi builds need WASI SDK. Please fetch the latest SDK from
https://github.com/WebAssembly/wasi-sdk/releases and install it to
"/opt/wasi-sdk". Alternatively you can install the SDK in a different location
and point the environment variable WASI_SDK_PATH to the root directory
of the SDK. The SDK is available for Linux x86_64, macOS x86_64, and MinGW.
"""

INSTALL_WASMTIME = """
wasm32-wasi tests require wasmtime on PATH. Please follow instructions at
https://wasmtime.dev/ to install wasmtime.
"""


def get_emscripten_root(emconfig: pathlib.Path = EM_CONFIG) -> pathlib.PurePath:
    """Parse EM_CONFIG file and lookup EMSCRIPTEN_ROOT

    The ".emscripten" config file is a Python snippet that uses "EM_CONFIG"
    environment variable. EMSCRIPTEN_ROOT is the "upstream/emscripten"
    subdirectory with tools like "emconfigure".
    """
    if not emconfig.exists():
        return _MISSING
    with open(emconfig, encoding="utf-8") as f:
        code = f.read()
    # EM_CONFIG file is a Python snippet
    local: Dict[str, Any] = {}
    exec(code, globals(), local)
    return pathlib.Path(local["EMSCRIPTEN_ROOT"])


EMSCRIPTEN_ROOT = get_emscripten_root()


class ConditionError(ValueError):
    def __init__(self, info: str, text: str):
        self.info = info
        self.text = text

    def __str__(self):
        return f"{type(self).__name__}: '{self.info}'\n{self.text}"


class MissingDependency(ConditionError):
    pass


class DirtySourceDirectory(ConditionError):
    pass


@dataclasses.dataclass
class Platform:
    """Platform-specific settings

    - CONFIG_SITE override
    - configure wrapper (e.g. emconfigure)
    - make wrapper (e.g. emmake)
    - additional environment variables
    - check function to verify SDK
    """

    name: str
    pythonexe: str
    config_site: Optional[pathlib.PurePath]
    configure_wrapper: Optional[pathlib.PurePath]
    make_wrapper: Optional[pathlib.PurePath]
    environ: dict
    check: Callable[[], None]

    def getenv(self, profile: "BuildProfile") -> dict:
        return self.environ.copy()


def _check_clean_src():
    candidates = [
        SRCDIR / "Programs" / "python.o",
        SRCDIR / "Python" / "frozen_modules" / "importlib._bootstrap.h",
    ]
    for candidate in candidates:
        if candidate.exists():
            raise DirtySourceDirectory(os.fspath(candidate), CLEAN_SRCDIR)


NATIVE = Platform(
    "native",
    # macOS has python.exe
    pythonexe=sysconfig.get_config_var("BUILDPYTHON") or "python",
    config_site=None,
    configure_wrapper=None,
    make_wrapper=None,
    environ={},
    check=_check_clean_src,
)


def _check_emscripten():
    if EMSCRIPTEN_ROOT is _MISSING:
        raise MissingDependency("Emscripten SDK EM_CONFIG", INSTALL_EMSDK)
    # sanity check
    emconfigure = EMSCRIPTEN.configure_wrapper
    if not emconfigure.exists():
        raise MissingDependency(os.fspath(emconfigure), INSTALL_EMSDK)
    # version check
    version_txt = EMSCRIPTEN_ROOT / "emscripten-version.txt"
    if not version_txt.exists():
        raise MissingDependency(os.fspath(version_txt), INSTALL_EMSDK)
    with open(version_txt) as f:
        version = f.read().strip().strip('"')
    version_tuple = tuple(int(v) for v in version.split("."))
    if version_tuple < EMSDK_MIN_VERSION:
        raise MissingDependency(
            os.fspath(version_txt),
            f"Emscripten SDK {version} in '{EMSCRIPTEN_ROOT}' is older than "
            "minimum required version "
            f"{'.'.join(str(v) for v in EMSDK_MIN_VERSION)}.",
        )
    _check_clean_src()


EMSCRIPTEN = Platform(
    "emscripten",
    pythonexe="python.js",
    config_site=WASMTOOLS / "config.site-wasm32-emscripten",
    configure_wrapper=EMSCRIPTEN_ROOT / "emconfigure",
    make_wrapper=EMSCRIPTEN_ROOT / "emmake",
    environ={"EM_COMPILER_WRAPPER": "ccache"} if HAS_CCACHE else {},
    check=_check_emscripten,
)


def _check_wasi():
    wasm_ld = WASI_SDK_PATH / "bin" / "wasm-ld"
    if not wasm_ld.exists():
        raise MissingDependency(os.fspath(wasm_ld), INSTALL_WASI_SDK)
    wasmtime = shutil.which("wasmtime")
    if wasmtime is None:
        raise MissingDependency("wasmtime", INSTALL_WASMTIME)
    _check_clean_src()


WASI = Platform(
    "wasi",
    pythonexe="python.wasm",
    config_site=WASMTOOLS / "config.site-wasm32-wasi",
    configure_wrapper=WASMTOOLS / "wasi-env",
    make_wrapper=None,
    environ={
        "WASI_SDK_PATH": WASI_SDK_PATH,
        # workaround for https://github.com/python/cpython/issues/95952
        "HOSTRUNNER": (
            "wasmtime run "
            "--env PYTHONPATH=/{relbuilddir}/build/lib.wasi-wasm32-$(VERSION):/Lib "
            "--mapdir /::{srcdir} --"
        ),
    },
    check=_check_wasi,
)


class Host(enum.Enum):
    """Target host triplet"""

    wasm32_emscripten = "wasm32-unknown-emscripten"
    wasm64_emscripten = "wasm64-unknown-emscripten"
    wasm32_wasi = "wasm32-unknown-wasi"
    wasm64_wasi = "wasm64-unknown-wasi"
    # current platform
    build = sysconfig.get_config_var("BUILD_GNU_TYPE")

    @property
    def platform(self) -> Platform:
        if self.is_emscripten:
            return EMSCRIPTEN
        elif self.is_wasi:
            return WASI
        else:
            return NATIVE

    @property
    def is_emscripten(self) -> bool:
        cls = type(self)
        return self in {cls.wasm32_emscripten, cls.wasm64_emscripten}

    @property
    def is_wasi(self) -> bool:
        cls = type(self)
        return self in {cls.wasm32_wasi, cls.wasm64_wasi}


class EmscriptenTarget(enum.Enum):
    """Emscripten-specific targets (--with-emscripten-target)"""

    browser = "browser"
    browser_debug = "browser-debug"
    node = "node"
    node_debug = "node-debug"

    @property
    def can_execute(self) -> bool:
        cls = type(self)
        return self not in {cls.browser, cls.browser_debug}


@dataclasses.dataclass
class BuildProfile:
    name: str
    host: Host
    target: Union[EmscriptenTarget, None] = None
    dynamic_linking: Union[bool, None] = None
    pthreads: Union[bool, None] = None
    testopts: str = "-j2"

    @property
    def can_execute(self) -> bool:
        """Can target run pythoninfo and tests?

        Disabled for browser, enabled for all other targets
        """
        return self.target is None or self.target.can_execute

    @property
    def builddir(self) -> pathlib.Path:
        """Path to build directory"""
        return BUILDDIR / self.name

    @property
    def python_cmd(self) -> pathlib.Path:
        """Path to python executable"""
        return self.builddir / self.host.platform.pythonexe

    @property
    def makefile(self) -> pathlib.Path:
        """Path to Makefile"""
        return self.builddir / "Makefile"

    @property
    def configure_cmd(self) -> List[str]:
        """Generate configure command"""
        # use relative path, so WASI tests can find lib prefix.
        # pathlib.Path.relative_to() does not work here.
        configure = os.path.relpath(CONFIGURE, self.builddir)
        cmd = [configure, "-C"]
        platform = self.host.platform
        if platform.configure_wrapper:
            cmd.insert(0, os.fspath(platform.configure_wrapper))

        cmd.append(f"--host={self.host.value}")
        cmd.append(f"--build={Host.build.value}")

        if self.target is not None:
            assert self.host.is_emscripten
            cmd.append(f"--with-emscripten-target={self.target.value}")

        if self.dynamic_linking is not None:
            assert self.host.is_emscripten
            opt = "enable" if self.dynamic_linking else "disable"
            cmd.append(f"--{opt}-wasm-dynamic-linking")

        if self.pthreads is not None:
            assert self.host.is_emscripten
            opt = "enable" if self.pthreads else "disable"
            cmd.append(f"--{opt}-wasm-pthreads")

        if self.host != Host.build:
            cmd.append(f"--with-build-python={BUILD.python_cmd}")

        if platform.config_site is not None:
            cmd.append(f"CONFIG_SITE={platform.config_site}")

        return cmd

    @property
    def make_cmd(self) -> List[str]:
        """Generate make command"""
        cmd = ["make"]
        platform = self.host.platform
        if platform.make_wrapper:
            cmd.insert(0, os.fspath(platform.make_wrapper))
        return cmd

    def getenv(self) -> dict:
        """Generate environ dict for platform"""
        env = os.environ.copy()
        env.setdefault("MAKEFLAGS", f"-j{os.cpu_count()}")
        platenv = self.host.platform.getenv(self)
        for key, value in platenv.items():
            if isinstance(value, str):
                value = value.format(
                    relbuilddir=self.builddir.relative_to(SRCDIR),
                    srcdir=SRCDIR,
                )
            env[key] = value
        return env

    def _run_cmd(self, cmd: Iterable[str], args: Iterable[str]):
        cmd = list(cmd)
        cmd.extend(args)
        return subprocess.check_call(
            cmd,
            cwd=os.fspath(self.builddir),
            env=self.getenv(),
        )

    def _check_execute(self):
        if not self.can_execute:
            raise ValueError(f"Cannot execute on {self.target}")

    def run_build(self, force_configure: bool = False):
        """Run configure (if necessary) and make"""
        if force_configure or not self.makefile.exists():
            self.run_configure()
        self.run_make()

    def run_configure(self, *args):
        """Run configure script to generate Makefile"""
        os.makedirs(self.builddir, exist_ok=True)
        return self._run_cmd(self.configure_cmd, args)

    def run_make(self, *args):
        """Run make (defaults to build all)"""
        return self._run_cmd(self.make_cmd, args)

    def run_pythoninfo(self):
        """Run 'make pythoninfo'"""
        self._check_execute()
        return self.run_make("pythoninfo")

    def run_test(self):
        """Run buildbottests"""
        self._check_execute()
        return self.run_make("buildbottest", f"TESTOPTS={self.testopts}")

    def run_py(self, *args):
        """Run Python with hostrunner"""
        self._check_execute()
        self.run_make(
            "--eval", f"run: all; $(HOSTRUNNER) ./$(PYTHON) {shlex.join(args)}", "run"
        )

    def clean(self, all: bool = False):
        """Clean build directory"""
        if all:
            if self.builddir.exists():
                shutil.rmtree(self.builddir)
        elif self.makefile.exists():
            self.run_make("clean")


# native build (build Python)
BUILD = BuildProfile(
    "build",
    host=Host.build,
)

_profiles = [
    BUILD,
    # wasm32-emscripten
    BuildProfile(
        "emscripten-browser",
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.browser,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-browser-debug",
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.browser_debug,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-node-dl",
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-node-dl-debug",
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node_debug,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-node-pthreads",
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node,
        pthreads=True,
    ),
    BuildProfile(
        "emscripten-node-pthreads-debug",
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node_debug,
        pthreads=True,
    ),
    # wasm64-emscripten (currently not working)
    BuildProfile(
        "wasm64-emscripten-node-debug",
        host=Host.wasm64_emscripten,
        target=EmscriptenTarget.node_debug,
        # MEMORY64 is not compatible with dynamic linking
        dynamic_linking=False,
        pthreads=False,
    ),
    # wasm32-wasi
    BuildProfile(
        "wasi",
        host=Host.wasm32_wasi,
        # skip sysconfig test_srcdir
        testopts="-i '*.test_srcdir' -j2",
    ),
    # no SDK available yet
    # BuildProfile(
    #    "wasm64-wasi",
    #    host=Host.wasm64_wasi,
    # ),
]

PROFILES = {p.name: p for p in _profiles}

parser = argparse.ArgumentParser(
    "wasm_build.py",
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter,
)
parser.add_argument(
    "--clean", "-c", help="Clean build directories first", action="store_true"
)

platforms = list(PROFILES) + ["cleanall"]
parser.add_argument(
    "platform",
    metavar="PLATFORM",
    help=f"Build platform: {', '.join(platforms)}",
    choices=platforms,
)

ops = ["compile", "pythoninfo", "test", "repl", "clean", "cleanall"]
parser.add_argument(
    "op",
    metavar="OP",
    help=f"operation: {', '.join(ops)}",
    choices=ops,
    default="compile",
    nargs="?",
)


def main():
    args = parser.parse_args()
    if args.platform == "cleanall":
        for builder in PROFILES.values():
            builder.clean(all=True)
        parser.exit(0)

    builder = PROFILES[args.platform]
    try:
        builder.host.platform.check()
    except ConditionError as e:
        parser.error(str(e))

    # hack for WASI
    if builder.host.is_wasi and not SETUP_LOCAL.exists():
        SETUP_LOCAL.touch()

    if args.op in {"compile", "pythoninfo", "repl", "test"}:
        # all targets need a build Python
        if builder is not BUILD:
            if args.clean:
                BUILD.clean(all=False)
                BUILD.run_build()
            elif not BUILD.python_cmd.exists():
                BUILD.run_build()

        if args.clean:
            builder.clean(all=False)

        if args.op == "compile":
            builder.run_build(force_configure=True)
        else:
            if not builder.makefile.exists():
                builder.run_configure()
            if args.op == "pythoninfo":
                builder.run_pythoninfo()
            elif args.op == "repl":
                builder.run_py()
            elif args.op == "test":
                builder.run_test()
    elif args.op == "clean":
        builder.clean(all=False)
    elif args.op == "cleanall":
        builder.clean(all=True)
    else:
        raise ValueError(args.op)

    print(builder.builddir)
    parser.exit(0)


if __name__ == "__main__":
    main()
