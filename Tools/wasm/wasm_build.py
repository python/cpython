#!/usr/bin/env python3
"""Build script for Python on WebAssembly platforms.

  $ ./Tools/wasm/wasm_builder.py emscripten-browser build repl
  $ ./Tools/wasm/wasm_builder.py emscripten-node-dl build test
  $ ./Tools/wasm/wasm_builder.py wasi build test

Primary build targets are "emscripten-node-dl" (NodeJS, dynamic linking),
"emscripten-browser", and "wasi".

Emscripten builds require a recent Emscripten SDK. The tools looks for an
activated EMSDK environment (". /path/to/emsdk_env.sh"). System packages
(Debian, Homebrew) are not supported.

WASI builds require WASI SDK and wasmtime. The tool looks for 'WASI_SDK_PATH'
and falls back to /opt/wasi-sdk.

The 'build' Python interpreter must be rebuilt every time Python's byte code
changes.

  ./Tools/wasm/wasm_builder.py --clean build build

"""
import argparse
import enum
import dataclasses
import logging
import os
import pathlib
import re
import shlex
import shutil
import socket
import subprocess
import sys
import sysconfig
import tempfile
import time
import warnings
import webbrowser

# for Python 3.8
from typing import Any, Callable, Dict, Iterable, List, Optional, Tuple, Union

logger = logging.getLogger("wasm_build")

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
EMSDK_MIN_VERSION = (3, 1, 19)
EMSDK_BROKEN_VERSION = {
    (3, 1, 14): "https://github.com/emscripten-core/emscripten/issues/17338",
    (3, 1, 16): "https://github.com/emscripten-core/emscripten/issues/17393",
    (3, 1, 20): "https://github.com/emscripten-core/emscripten/issues/17720",
}
_MISSING = pathlib.PurePath("MISSING")

WASM_WEBSERVER = WASMTOOLS / "wasm_webserver.py"

CLEAN_SRCDIR = f"""
Builds require a clean source directory. Please use a clean checkout or
run "make clean -C '{SRCDIR}'".
"""

INSTALL_NATIVE = """
Builds require a C compiler (gcc, clang), make, pkg-config, and development
headers for dependencies like zlib.

Debian/Ubuntu: sudo apt install build-essential git curl pkg-config zlib1g-dev
Fedora/CentOS: sudo dnf install gcc make git-core curl pkgconfig zlib-devel
"""

INSTALL_EMSDK = """
wasm32-emscripten builds need Emscripten SDK. Please follow instructions at
https://emscripten.org/docs/getting_started/downloads.html how to install
Emscripten and how to activate the SDK with "emsdk_env.sh".

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


def parse_emconfig(
    emconfig: pathlib.Path = EM_CONFIG,
) -> Tuple[pathlib.PurePath, pathlib.PurePath]:
    """Parse EM_CONFIG file and lookup EMSCRIPTEN_ROOT and NODE_JS.

    The ".emscripten" config file is a Python snippet that uses "EM_CONFIG"
    environment variable. EMSCRIPTEN_ROOT is the "upstream/emscripten"
    subdirectory with tools like "emconfigure".
    """
    if not emconfig.exists():
        return _MISSING, _MISSING
    with open(emconfig, encoding="utf-8") as f:
        code = f.read()
    # EM_CONFIG file is a Python snippet
    local: Dict[str, Any] = {}
    exec(code, globals(), local)
    emscripten_root = pathlib.Path(local["EMSCRIPTEN_ROOT"])
    node_js = pathlib.Path(local["NODE_JS"])
    return emscripten_root, node_js


EMSCRIPTEN_ROOT, NODE_JS = parse_emconfig()


def read_python_version(configure: pathlib.Path = CONFIGURE) -> str:
    """Read PACKAGE_VERSION from configure script

    configure and configure.ac are the canonical source for major and
    minor version number.
    """
    version_re = re.compile(r"^PACKAGE_VERSION='(\d\.\d+)'")
    with configure.open(encoding="utf-8") as f:
        for line in f:
            mo = version_re.match(line)
            if mo:
                return mo.group(1)
    raise ValueError(f"PACKAGE_VERSION not found in {configure}")


PYTHON_VERSION = read_python_version()


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
    # Used for build_emports().
    ports: Optional[pathlib.PurePath]
    cc: Optional[pathlib.PurePath]

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


def _check_native():
    if not any(shutil.which(cc) for cc in ["cc", "gcc", "clang"]):
        raise MissingDependency("cc", INSTALL_NATIVE)
    if not shutil.which("make"):
        raise MissingDependency("make", INSTALL_NATIVE)
    if sys.platform == "linux":
        # skip pkg-config check on macOS
        if not shutil.which("pkg-config"):
            raise MissingDependency("pkg-config", INSTALL_NATIVE)
        # zlib is needed to create zip files
        for devel in ["zlib"]:
            try:
                subprocess.check_call(["pkg-config", "--exists", devel])
            except subprocess.CalledProcessError:
                raise MissingDependency(devel, INSTALL_NATIVE) from None
    _check_clean_src()


NATIVE = Platform(
    "native",
    # macOS has python.exe
    pythonexe=sysconfig.get_config_var("BUILDPYTHON") or "python",
    config_site=None,
    configure_wrapper=None,
    ports=None,
    cc=None,
    make_wrapper=None,
    environ={},
    check=_check_native,
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
    if version.endswith("-git"):
        # git / upstream / tot-upstream installation
        version = version[:-4]
    version_tuple = tuple(int(v) for v in version.split("."))
    if version_tuple < EMSDK_MIN_VERSION:
        raise ConditionError(
            os.fspath(version_txt),
            f"Emscripten SDK {version} in '{EMSCRIPTEN_ROOT}' is older than "
            "minimum required version "
            f"{'.'.join(str(v) for v in EMSDK_MIN_VERSION)}.",
        )
    broken = EMSDK_BROKEN_VERSION.get(version_tuple)
    if broken is not None:
        raise ConditionError(
            os.fspath(version_txt),
            (
                f"Emscripten SDK {version} in '{EMSCRIPTEN_ROOT}' has known "
                f"bugs, see {broken}."
            ),
        )
    if os.environ.get("PKG_CONFIG_PATH"):
        warnings.warn(
            "PKG_CONFIG_PATH is set and not empty. emconfigure overrides "
            "this environment variable. Use EM_PKG_CONFIG_PATH instead."
        )
    _check_clean_src()


EMSCRIPTEN = Platform(
    "emscripten",
    pythonexe="python.js",
    config_site=WASMTOOLS / "config.site-wasm32-emscripten",
    configure_wrapper=EMSCRIPTEN_ROOT / "emconfigure",
    ports=EMSCRIPTEN_ROOT / "embuilder",
    cc=EMSCRIPTEN_ROOT / "emcc",
    make_wrapper=EMSCRIPTEN_ROOT / "emmake",
    environ={
        # workaround for https://github.com/emscripten-core/emscripten/issues/17635
        "TZ": "UTC",
        "EM_COMPILER_WRAPPER": "ccache" if HAS_CCACHE else None,
        "PATH": [EMSCRIPTEN_ROOT, os.environ["PATH"]],
    },
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
    ports=None,
    cc=WASI_SDK_PATH / "bin" / "clang",
    make_wrapper=None,
    environ={
        "WASI_SDK_PATH": WASI_SDK_PATH,
        # workaround for https://github.com/python/cpython/issues/95952
        "HOSTRUNNER": (
            "wasmtime run "
            "--wasm max-wasm-stack=8388608 "
            "--wasi preview2 "
            "--dir {srcdir}::/ "
            "--env PYTHONPATH=/{relbuilddir}/build/lib.wasi-wasm32-{version}:/Lib"
        ),
        "PATH": [WASI_SDK_PATH / "bin", os.environ["PATH"]],
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

    def get_extra_paths(self) -> Iterable[pathlib.PurePath]:
        """Host-specific os.environ["PATH"] entries.

        Emscripten's Node version 14.x works well for wasm32-emscripten.
        wasm64-emscripten requires more recent v8 version, e.g. node 16.x.
        Attempt to use system's node command.
        """
        cls = type(self)
        if self == cls.wasm32_emscripten:
            return [NODE_JS.parent]
        elif self == cls.wasm64_emscripten:
            # TODO: look for recent node
            return []
        else:
            return []

    @property
    def emport_args(self) -> List[str]:
        """Host-specific port args (Emscripten)."""
        cls = type(self)
        if self is cls.wasm64_emscripten:
            return ["-sMEMORY64=1"]
        elif self is cls.wasm32_emscripten:
            return ["-sMEMORY64=0"]
        else:
            return []

    @property
    def embuilder_args(self) -> List[str]:
        """Host-specific embuilder args (Emscripten)."""
        cls = type(self)
        if self is cls.wasm64_emscripten:
            return ["--wasm64"]
        else:
            return []


class EmscriptenTarget(enum.Enum):
    """Emscripten-specific targets (--with-emscripten-target)"""

    browser = "browser"
    browser_debug = "browser-debug"
    node = "node"
    node_debug = "node-debug"

    @property
    def is_browser(self):
        cls = type(self)
        return self in {cls.browser, cls.browser_debug}

    @property
    def emport_args(self) -> List[str]:
        """Target-specific port args."""
        cls = type(self)
        if self in {cls.browser_debug, cls.node_debug}:
            # some libs come in debug and non-debug builds
            return ["-O0"]
        else:
            return ["-O2"]


class SupportLevel(enum.Enum):
    supported = "tier 3, supported"
    working = "working, unsupported"
    experimental = "experimental, may be broken"
    broken = "broken / unavailable"

    def __bool__(self):
        cls = type(self)
        return self in {cls.supported, cls.working}


@dataclasses.dataclass
class BuildProfile:
    name: str
    support_level: SupportLevel
    host: Host
    target: Union[EmscriptenTarget, None] = None
    dynamic_linking: Union[bool, None] = None
    pthreads: Union[bool, None] = None
    default_testopts: str = "-j2"

    @property
    def is_browser(self) -> bool:
        """Is this a browser build?"""
        return self.target is not None and self.target.is_browser

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
            if value is None:
                env.pop(key, None)
            elif key == "PATH":
                # list of path items, prefix with extra paths
                new_path: List[pathlib.PurePath] = []
                new_path.extend(self.host.get_extra_paths())
                new_path.extend(value)
                env[key] = os.pathsep.join(os.fspath(p) for p in new_path)
            elif isinstance(value, str):
                env[key] = value.format(
                    relbuilddir=self.builddir.relative_to(SRCDIR),
                    srcdir=SRCDIR,
                    version=PYTHON_VERSION,
                )
            else:
                env[key] = value
        return env

    def _run_cmd(
        self,
        cmd: Iterable[str],
        args: Iterable[str] = (),
        cwd: Optional[pathlib.Path] = None,
    ):
        cmd = list(cmd)
        cmd.extend(args)
        if cwd is None:
            cwd = self.builddir
        logger.info('Running "%s" in "%s"', shlex.join(cmd), cwd)
        return subprocess.check_call(
            cmd,
            cwd=os.fspath(cwd),
            env=self.getenv(),
        )

    def _check_execute(self):
        if self.is_browser:
            raise ValueError(f"Cannot execute on {self.target}")

    def run_build(self, *args):
        """Run configure (if necessary) and make"""
        if not self.makefile.exists():
            logger.info("Makefile not found, running configure")
            self.run_configure(*args)
        self.run_make("all", *args)

    def run_configure(self, *args):
        """Run configure script to generate Makefile"""
        os.makedirs(self.builddir, exist_ok=True)
        return self._run_cmd(self.configure_cmd, args)

    def run_make(self, *args):
        """Run make (defaults to build all)"""
        return self._run_cmd(self.make_cmd, args)

    def run_pythoninfo(self, *args):
        """Run 'make pythoninfo'"""
        self._check_execute()
        return self.run_make("pythoninfo", *args)

    def run_test(self, target: str, testopts: Optional[str] = None):
        """Run buildbottests"""
        self._check_execute()
        if testopts is None:
            testopts = self.default_testopts
        return self.run_make(target, f"TESTOPTS={testopts}")

    def run_py(self, *args):
        """Run Python with hostrunner"""
        self._check_execute()
        self.run_make(
            "--eval", f"run: all; $(HOSTRUNNER) ./$(PYTHON) {shlex.join(args)}", "run"
        )

    def run_browser(self, bind="127.0.0.1", port=8000):
        """Run WASM webserver and open build in browser"""
        relbuilddir = self.builddir.relative_to(SRCDIR)
        url = f"http://{bind}:{port}/{relbuilddir}/python.html"
        args = [
            sys.executable,
            os.fspath(WASM_WEBSERVER),
            "--bind",
            bind,
            "--port",
            str(port),
        ]
        srv = subprocess.Popen(args, cwd=SRCDIR)
        # wait for server
        end = time.monotonic() + 3.0
        while time.monotonic() < end and srv.returncode is None:
            try:
                with socket.create_connection((bind, port), timeout=0.1) as _:
                    pass
            except OSError:
                time.sleep(0.01)
            else:
                break

        webbrowser.open(url)

        try:
            srv.wait()
        except KeyboardInterrupt:
            pass

    def clean(self, all: bool = False):
        """Clean build directory"""
        if all:
            if self.builddir.exists():
                shutil.rmtree(self.builddir)
        elif self.makefile.exists():
            self.run_make("clean")

    def build_emports(self, force: bool = False):
        """Pre-build emscripten ports."""
        platform = self.host.platform
        if platform.ports is None or platform.cc is None:
            raise ValueError("Need ports and CC command")

        embuilder_cmd = [os.fspath(platform.ports)]
        embuilder_cmd.extend(self.host.embuilder_args)
        if force:
            embuilder_cmd.append("--force")

        ports_cmd = [os.fspath(platform.cc)]
        ports_cmd.extend(self.host.emport_args)
        if self.target:
            ports_cmd.extend(self.target.emport_args)

        if self.dynamic_linking:
            # Trigger PIC build.
            ports_cmd.append("-sMAIN_MODULE")
            embuilder_cmd.append("--pic")

        if self.pthreads:
            # Trigger multi-threaded build.
            ports_cmd.append("-sUSE_PTHREADS")

        # Pre-build libbz2, libsqlite3, libz, and some system libs.
        ports_cmd.extend(["-sUSE_ZLIB", "-sUSE_BZIP2", "-sUSE_SQLITE3"])
        # Multi-threaded sqlite3 has different suffix
        embuilder_cmd.extend(
            ["build", "bzip2", "sqlite3-mt" if self.pthreads else "sqlite3", "zlib"]
        )

        self._run_cmd(embuilder_cmd, cwd=SRCDIR)

        with tempfile.TemporaryDirectory(suffix="-py-emport") as tmpdir:
            tmppath = pathlib.Path(tmpdir)
            main_c = tmppath / "main.c"
            main_js = tmppath / "main.js"
            with main_c.open("w") as f:
                f.write("int main(void) { return 0; }\n")
            args = [
                os.fspath(main_c),
                "-o",
                os.fspath(main_js),
            ]
            self._run_cmd(ports_cmd, args, cwd=tmppath)


# native build (build Python)
BUILD = BuildProfile(
    "build",
    support_level=SupportLevel.working,
    host=Host.build,
)

_profiles = [
    BUILD,
    # wasm32-emscripten
    BuildProfile(
        "emscripten-browser",
        support_level=SupportLevel.supported,
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.browser,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-browser-debug",
        support_level=SupportLevel.working,
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.browser_debug,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-node-dl",
        support_level=SupportLevel.supported,
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-node-dl-debug",
        support_level=SupportLevel.working,
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node_debug,
        dynamic_linking=True,
    ),
    BuildProfile(
        "emscripten-node-pthreads",
        support_level=SupportLevel.supported,
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node,
        pthreads=True,
    ),
    BuildProfile(
        "emscripten-node-pthreads-debug",
        support_level=SupportLevel.working,
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node_debug,
        pthreads=True,
    ),
    # Emscripten build with both pthreads and dynamic linking is crashing.
    BuildProfile(
        "emscripten-node-dl-pthreads-debug",
        support_level=SupportLevel.broken,
        host=Host.wasm32_emscripten,
        target=EmscriptenTarget.node_debug,
        dynamic_linking=True,
        pthreads=True,
    ),
    # wasm64-emscripten (requires Emscripten >= 3.1.21)
    BuildProfile(
        "wasm64-emscripten-node-debug",
        support_level=SupportLevel.experimental,
        host=Host.wasm64_emscripten,
        target=EmscriptenTarget.node_debug,
        # MEMORY64 is not compatible with dynamic linking
        dynamic_linking=False,
        pthreads=False,
    ),
    # wasm32-wasi
    BuildProfile(
        "wasi",
        support_level=SupportLevel.supported,
        host=Host.wasm32_wasi,
    ),
    # wasm32-wasi-threads
    BuildProfile(
        "wasi-threads",
        support_level=SupportLevel.experimental,
        host=Host.wasm32_wasi,
        pthreads=True,
    ),
    # no SDK available yet
    # BuildProfile(
    #    "wasm64-wasi",
    #    support_level=SupportLevel.broken,
    #    host=Host.wasm64_wasi,
    # ),
]

PROFILES = {p.name: p for p in _profiles}

parser = argparse.ArgumentParser(
    "wasm_build.py",
    description=__doc__,
    formatter_class=argparse.RawTextHelpFormatter,
)

parser.add_argument(
    "--clean",
    "-c",
    help="Clean build directories first",
    action="store_true",
)

parser.add_argument(
    "--verbose",
    "-v",
    help="Verbose logging",
    action="store_true",
)

parser.add_argument(
    "--silent",
    help="Run configure and make in silent mode",
    action="store_true",
)

parser.add_argument(
    "--testopts",
    help=(
        "Additional test options for 'test' and 'hostrunnertest', e.g. "
        "--testopts='-v test_os'."
    ),
    default=None,
)

# Don't list broken and experimental variants in help
platforms_choices = list(p.name for p in _profiles) + ["cleanall"]
platforms_help = list(p.name for p in _profiles if p.support_level) + ["cleanall"]
parser.add_argument(
    "platform",
    metavar="PLATFORM",
    help=f"Build platform: {', '.join(platforms_help)}",
    choices=platforms_choices,
)

ops = dict(
    build="auto build (build 'build' Python, emports, configure, compile)",
    configure="run ./configure",
    compile="run 'make all'",
    pythoninfo="run 'make pythoninfo'",
    test="run 'make buildbottest TESTOPTS=...' (supports parallel tests)",
    hostrunnertest="run 'make hostrunnertest TESTOPTS=...'",
    repl="start interactive REPL / webserver + browser session",
    clean="run 'make clean'",
    cleanall="remove all build directories",
    emports="build Emscripten port with embuilder (only Emscripten)",
)
ops_help = "\n".join(f"{op:16s} {help}" for op, help in ops.items())
parser.add_argument(
    "ops",
    metavar="OP",
    help=f"operation (default: build)\n\n{ops_help}",
    choices=tuple(ops),
    default="build",
    nargs="*",
)


def main():
    args = parser.parse_args()
    logging.basicConfig(
        level=logging.INFO if args.verbose else logging.ERROR,
        format="%(message)s",
    )

    if args.platform == "cleanall":
        for builder in PROFILES.values():
            builder.clean(all=True)
        parser.exit(0)

    # additional configure and make args
    cm_args = ("--silent",) if args.silent else ()

    # nargs=* with default quirk
    if args.ops == "build":
        args.ops = ["build"]

    builder = PROFILES[args.platform]
    try:
        builder.host.platform.check()
    except ConditionError as e:
        parser.error(str(e))

    if args.clean:
        builder.clean(all=False)

    # hack for WASI
    if builder.host.is_wasi and not SETUP_LOCAL.exists():
        SETUP_LOCAL.touch()

    # auto-build
    if "build" in args.ops:
        # check and create build Python
        if builder is not BUILD:
            logger.info("Auto-building 'build' Python.")
            try:
                BUILD.host.platform.check()
            except ConditionError as e:
                parser.error(str(e))
            if args.clean:
                BUILD.clean(all=False)
            BUILD.run_build(*cm_args)
        # build Emscripten ports with embuilder
        if builder.host.is_emscripten and "emports" not in args.ops:
            builder.build_emports()

    for op in args.ops:
        logger.info("\n*** %s %s", args.platform, op)
        if op == "build":
            builder.run_build(*cm_args)
        elif op == "configure":
            builder.run_configure(*cm_args)
        elif op == "compile":
            builder.run_make("all", *cm_args)
        elif op == "pythoninfo":
            builder.run_pythoninfo(*cm_args)
        elif op == "repl":
            if builder.is_browser:
                builder.run_browser()
            else:
                builder.run_py()
        elif op == "test":
            builder.run_test("buildbottest", testopts=args.testopts)
        elif op == "hostrunnertest":
            builder.run_test("hostrunnertest", testopts=args.testopts)
        elif op == "clean":
            builder.clean(all=False)
        elif op == "cleanall":
            builder.clean(all=True)
        elif op == "emports":
            builder.build_emports(force=args.clean)
        else:
            raise ValueError(op)

    print(builder.builddir)
    parser.exit(0)


if __name__ == "__main__":
    main()
