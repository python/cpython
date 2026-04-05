"""
pyconf.py — module for the CPython Python-based configure system.

Provides the pyconf API used by conf_*.py.
"""

from __future__ import annotations

import fnmatch as _fnmatch
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import types
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterator


# ---------------------------------------------------------------------------
# Argument validation for transpiler-critical functions
# The transpiler precomputes HAVE_* define names from these strings at
# transpile time.  If a caller passes a string that doesn't match the
# expected character set, the precomputed name will be wrong in the
# transpiled shell script.  These checks catch that at Python-run time.
# ---------------------------------------------------------------------------

_RE_FUNC_NAME = re.compile(r"^[a-zA-Z_][a-zA-Z0-9_]*$")
_RE_HEADER_NAME = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9_./+-]*$")
_RE_DECL_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_RE_MEMBER_NAME = re.compile(r"^[a-z_][a-z0-9_ ]*\.[a-z_][a-z0-9_]*$")


def format_yn(value: bool) -> str:
    """Format a boolean as 'yes' or 'no' (for output and Makefile substs)."""
    return "yes" if value else "no"


# ---------------------------------------------------------------------------
# pyconf.vars — shared mutable namespace for cross-part state
# ---------------------------------------------------------------------------


class Vars:
    """Namespace for configure variables.

    Attributes that have not been explicitly set default to
    ``os.environ.get(name, "")``, mirroring autoconf's behaviour where
    unset shell variables expand to the empty string and environment
    variables are picked up automatically.
    """

    _exports: set[str]

    def __init__(self) -> None:
        object.__setattr__(self, "_exports", set())

    def __getattr__(self, name: str) -> str:
        if name.startswith("_"):
            raise AttributeError(name)
        return os.environ.get(name, "")

    def is_set(self, name: str) -> bool:
        """Return True if *name* has been explicitly assigned."""
        return name in self.__dict__

    _SENTINEL = object()

    def export(
        self, name: str, value: str | bool | None | object = _SENTINEL
    ) -> None:
        """Mark a variable for Makefile substitution (AC_SUBST).

        If *value* is given, assigns it to this Vars instance first.
        Otherwise looks up the value from this Vars instance (including
        environment variables via ``__getattr__``).
        The name is recorded in ``self._exports``.
        """
        self._exports.add(name)
        if value is not self._SENTINEL:
            setattr(self, name, value)
        val = self.__dict__.get(name)
        if val is None:
            # Fall back to environment variable (mirrors __getattr__),
            # matching autoconf where AC_SUBST reads shell variables
            # which include inherited environment.
            val = os.environ.get(name, None)
        if val is not None:
            substs[name] = (
                format_yn(val) if isinstance(val, bool) else str(val)
            )
        elif name not in substs:
            substs[name] = ""

    def __repr__(self) -> str:
        items = ", ".join(
            f"{k}={v!r}"
            for k, v in self.__dict__.items()
            if not k.startswith("_")
        )
        return f"Vars({items})"


vars = Vars()

# True between a checking() call and its matching result() call.
# Probe functions (compile_check, link_check, run_check, etc.) use this to
# emit a result line when the caller did a bare checking() without result().
_result_pending: bool = False

# ---------------------------------------------------------------------------
# Platform identity (set by canonical_host())
# ---------------------------------------------------------------------------
cross_compiling: bool | str = False
host: str = ""  # e.g. "x86_64-pc-linux-gnu"
host_cpu: str = ""  # e.g. "x86_64"
build: str = ""  # e.g. "x86_64-pc-linux-gnu"

# Compiler identity (set by find_compiler())
CC: str = ""
CPP: str = ""
GCC: bool = False
ac_cv_cc_name: str = ""  # "gcc", "clang", "emcc", etc.
ac_cv_gcc_compat: bool = False
ac_cv_prog_cc_g: bool = False

# ---------------------------------------------------------------------------
# Build output state
# ---------------------------------------------------------------------------
cache: dict[str, Any] = {}  # probe result cache (ac_cv_* style)
defines: dict[str, tuple] = {}  # pyconfig.h #define entries
substs: dict[str, str] = {}  # Makefile substitution variables
_header_map: dict[str, str] = {}  # HAVE_FOO_H → "foo.h" reverse map
srcdir: str = ""  # source directory path

# ---------------------------------------------------------------------------
# Environment dict (for PKG_CONFIG etc.)
# ---------------------------------------------------------------------------
env: dict[str, str] = {}

# ---------------------------------------------------------------------------
# Command-line behaviour flags (set by init_args())
# ---------------------------------------------------------------------------
quiet: bool = False  # --quiet / -q / --silent
no_create: bool = False  # --no-create / -n
cache_file: str = ""  # --cache-file=FILE / -C
disable_option_checking: bool = False  # --disable-option-checking
_help_requested: bool = False  # deferred --help flag
_dir_args: dict[str, str] = {}  # standard dir args: --prefix, --bindir, etc.
_system_type_args: dict[str, str] = {}  # --host, --build, --target triplets
_srcdir_arg: str = ""  # --srcdir value from argv (empty = not set)
_original_config_args: list[str] = []  # original user-facing args

# ---------------------------------------------------------------------------
# Internal: option + env-var registries (populated by option() / env_var())
# ---------------------------------------------------------------------------
# Each entry: {'flag': str, 'metavar': str, 'help': str, 'default': Any, 'value': str|None}
_options: dict[
    str, dict[str, Any]
] = {}  # keyed by normalized name (no leading --, hyphens→underscores)
_env_vars: dict[str, str] = {}  # keyed by var name, value is description


# ---------------------------------------------------------------------------
# Stub API — all no-ops or minimal implementations
# ---------------------------------------------------------------------------


# Standard autoconf installation directory variables (in display order)
_DIR_VARS: list[tuple[str, str, str]] = [
    # (arg-name,          key,            default)
    ("prefix", "prefix", "/usr/local"),
    ("exec-prefix", "exec_prefix", "${prefix}"),
    ("bindir", "bindir", "${exec_prefix}/bin"),
    ("sbindir", "sbindir", "${exec_prefix}/sbin"),
    ("libexecdir", "libexecdir", "${exec_prefix}/libexec"),
    ("sysconfdir", "sysconfdir", "${prefix}/etc"),
    ("sharedstatedir", "sharedstatedir", "${prefix}/com"),
    ("localstatedir", "localstatedir", "${prefix}/var"),
    ("runstatedir", "runstatedir", "${localstatedir}/run"),
    ("libdir", "libdir", "${exec_prefix}/lib"),
    ("includedir", "includedir", "${prefix}/include"),
    ("oldincludedir", "oldincludedir", "/usr/include"),
    ("datarootdir", "datarootdir", "${prefix}/share"),
    ("datadir", "datadir", "${datarootdir}"),
    ("infodir", "infodir", "${datarootdir}/info"),
    ("localedir", "localedir", "${datarootdir}/locale"),
    ("mandir", "mandir", "${datarootdir}/man"),
    ("docdir", "docdir", "${datarootdir}/doc/${PACKAGE_TARNAME}"),
    ("htmldir", "htmldir", "${docdir}"),
    ("dvidir", "dvidir", "${docdir}"),
    ("pdfdir", "pdfdir", "${docdir}"),
    ("psdir", "psdir", "${docdir}"),
]


def _load_config_site() -> None:
    """Load CONFIG_SITE file, if set.

    CONFIG_SITE is a shell script containing simple VAR=VALUE assignments
    used to pre-seed cache variables for cross-compilation.  Only lines
    matching ``NAME=VALUE`` are processed; comments and blank lines are
    skipped.  Values already present in ``os.environ`` (e.g. from
    command-line ``VAR=VALUE`` arguments) or already set on ``vars`` are
    NOT overridden.

    ``yes``/``no`` values are converted to ``True``/``False`` via
    ``_cache_deserialize`` so that truthiness checks in conf_*.py work
    the same way as autoconf's ``test "x$var" = xyes``.
    """
    config_site = os.environ.get("CONFIG_SITE", "")
    if not config_site or not os.path.isfile(config_site):
        return
    with open(config_site) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)=(.*)", line)
            if not m:
                continue
            name, value = m.group(1), m.group(2)
            # Strip surrounding quotes (single or double)
            if (
                len(value) >= 2
                and value[0] == value[-1]
                and value[0] in ("'", '"')
            ):
                value = value[1:-1]
            # Command-line VAR=VALUE (already in os.environ) takes precedence
            if name not in os.environ and not vars.is_set(name):
                converted = _cache_deserialize(value)
                setattr(vars, name, converted)
                # Also populate the cache dict so that check_func(),
                # check_header(), etc. honour the pre-seeded value and
                # skip the compile/link test (matching autoconf behaviour
                # where CONFIG_SITE sets ac_cv_* shell variables that
                # AC_CHECK_FUNC reads as cached results).
                if name.startswith("ac_cv_"):
                    cache[name] = converted


def init_args() -> None:
    """Parse sys.argv early: handle VAR=VALUE, --help, --version, behaviour flags,
    and standard directory arguments.  Must be called before configure parts run.
    """
    global \
        quiet, \
        no_create, \
        cache_file, \
        disable_option_checking, \
        _help_requested, \
        _srcdir_arg
    global _dir_args, _system_type_args

    global _original_config_args
    # Save user-facing args (exclude --srcdir which is internal to the wrapper)
    _original_config_args = [
        a for a in sys.argv[1:] if not a.startswith("--srcdir")
    ]
    # Register built-in autoconf precious variables in the same order as
    # configure.ac's ac_precious_vars list (PKG_CONFIG before CC, no CXX/CXXFLAGS
    # since configure.ac does not call AC_ARG_VAR for those).  User-level
    # precious variables (MACHDEP, PROFILE_TASK, GDBM_CFLAGS, …) are registered
    # via env_var() calls in the configure_*.py files instead.
    for pvar in [
        "PKG_CONFIG",
        "PKG_CONFIG_PATH",
        "PKG_CONFIG_LIBDIR",
        "CC",
        "CFLAGS",
        "LDFLAGS",
        "LIBS",
        "CPPFLAGS",
        "CPP",
    ]:
        env_var(pvar)

    new_argv: list[str] = [sys.argv[0]]
    i = 1
    while i < len(sys.argv):
        arg = sys.argv[i]
        i += 1

        # VAR=VALUE (no leading '--', NAME is all-caps word chars)
        if re.match(r"^[A-Za-z_][A-Za-z0-9_]*=", arg) and not arg.startswith(
            "-"
        ):
            name, value = arg.split("=", 1)
            os.environ[name] = value
            # Also set on vars so is_set() detects it and bool truthiness
            # works correctly, mirroring what _load_config_site() does.
            # Convert yes/no → True/False so ac_cv_* checks behave the same
            # whether the value came from CONFIG_SITE or the command line.
            converted = _cache_deserialize(value)
            setattr(vars, name, converted)
            if name.startswith("ac_cv_"):
                cache[name] = converted
            continue

        # Early-exit flags
        if arg in ("-h", "--help"):
            _help_requested = True
            continue
        if arg in ("-V", "--version"):
            print("configure (CPython) 3.15")
            sys.exit(0)

        # Behaviour flags
        if arg in ("-q", "--quiet", "--silent"):
            quiet = True
            continue
        if arg in ("-n", "--no-create"):
            no_create = True
            continue
        if arg == "-C":
            cache_file = "config.cache"
            continue
        if arg.startswith("--cache-file="):
            cache_file = arg.split("=", 1)[1]
            continue
        if arg == "--cache-file" and i < len(sys.argv):
            cache_file = sys.argv[i]
            i += 1
            continue
        if arg == "--disable-option-checking":
            disable_option_checking = True
            continue

        # --srcdir=DIR (autoconf compatibility; later args override earlier ones)
        if arg == "--srcdir":
            if i < len(sys.argv):
                _srcdir_arg = sys.argv[i]
                i += 1
            continue
        if arg.startswith("--srcdir="):
            _srcdir_arg = arg.split("=", 1)[1]
            continue

        # Standard directory args
        matched_dir = False
        for argname, key, default in _DIR_VARS:
            if arg == f"--{argname}":
                # Next token is the value
                if i < len(sys.argv):
                    _dir_args[key] = sys.argv[i]
                    i += 1
                matched_dir = True
                break
            if arg.startswith(f"--{argname}="):
                _dir_args[key] = arg.split("=", 1)[1]
                matched_dir = True
                break
        if matched_dir:
            continue

        # System type triplets: --host, --build, --target
        matched_type = False
        for stype in ("host", "build", "target"):
            if arg == f"--{stype}":
                if i < len(sys.argv):
                    _system_type_args[stype] = sys.argv[i]
                    i += 1
                matched_type = True
                break
            if arg.startswith(f"--{stype}="):
                _system_type_args[stype] = arg.split("=", 1)[1]
                matched_type = True
                break
        if matched_type:
            continue

        new_argv.append(arg)

    sys.argv[:] = new_argv

    # Load CONFIG_SITE file if set (autoconf compatibility).
    # CONFIG_SITE is a shell script with simple VAR=VALUE assignments that
    # pre-seed cache variables for cross-compilation.  Command-line VAR=VALUE
    # arguments (already placed into os.environ above) take precedence.
    _load_config_site()

    # Load cache file if requested
    if cache_file:
        if os.path.exists(cache_file):
            print(f"configure: loading cache {cache_file}")
            _load_cache()
        else:
            print(f"configure: creating cache {cache_file}")


def _cache_deserialize(s: str) -> Any:
    """Convert a cache-file string value to a Python object.

    'yes' → True, 'no' → False, numeric strings → int, else str.
    """
    if s == "yes":
        return True
    if s == "no":
        return False
    # Keep numeric strings as strings — callers like check_sizeof() do
    # int(cache[key]) explicitly.
    return s


def _cache_serialize(v: Any) -> str:
    """Convert a Python cache value to a string for the cache file.

    True → 'yes', False → 'no', else str(v).
    """
    if v is True:
        return "yes"
    if v is False:
        return "no"
    return str(v)


def _cache_quote(s: str) -> str:
    """Quote a value for the shell-compatible cache file if needed.

    Values containing spaces, quotes, braces, or other shell meta-characters
    are wrapped in single quotes (with internal single quotes escaped).
    """
    if s and not re.search(r"[^a-zA-Z0-9_./:@+=-]", s):
        return s
    # Shell single-quote: replace ' with '\''
    return "'" + s.replace("'", "'\\''") + "'"


def _load_cache() -> None:
    """Load cached probe results from cache_file into pyconf.cache.

    Handles both the autoconf shell-script format:
        ac_cv_foo=${ac_cv_foo=value}
        test ${ac_cv_foo+y} || ac_cv_foo='value with spaces'
    and the simple key=value format written by older pyconf versions.
    """
    try:
        with open(cache_file) as fh:
            for line in fh:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                # Autoconf format: key=${key=value}
                m = re.match(r"^([a-zA-Z_]\w*)=\$\{\1=(.*)\}$", line)
                if m:
                    k = m.group(1)
                    v = _cache_unquote(m.group(2))
                    cache[k] = _cache_deserialize(v)
                    continue
                # Autoconf alternate: test ${key+y} || key='value'
                m = re.match(
                    r"^test\s+\$\{([a-zA-Z_]\w*)\+y\}\s+\|\|\s+\1=(.*)",
                    line,
                )
                if m:
                    k = m.group(1)
                    v = _cache_unquote(m.group(2))
                    cache[k] = _cache_deserialize(v)
                    continue
                # Simple format: key=value
                if "=" in line:
                    k, v = line.split("=", 1)
                    k = k.strip()
                    v = _cache_unquote(v.strip())
                    cache[k] = _cache_deserialize(v)
    except OSError:
        pass


def _cache_unquote(s: str) -> str:
    """Remove shell single-quotes from a cache value string."""
    if s.startswith("'") and s.endswith("'"):
        # Undo shell single-quoting: 'foo'\''bar' → foo'bar
        s = s[1:-1]
        s = s.replace("'\\''", "'")
    return s


_CACHE_HEADER = """\
# This file is a shell script that caches the results of configure
# tests run on this system so they can be shared between configure
# scripts and configure runs, see configure's option --config-cache.
# It is not useful on other systems.  If it contains results you don't
# want to keep, you may remove or edit it.
#
# config.status only pays attention to the cache file if you give it
# the --recheck option to rerun configure.
#
# 'ac_cv_env_foo' variables (set or unset) will be overridden when
# loading this file, other *unset* 'ac_cv_foo' will be assigned the
# following values.

"""


def _save_cache() -> None:
    """Save pyconf.cache to cache_file in autoconf-compatible format."""
    if not cache_file:
        return
    try:
        with open(cache_file, "w") as fh:
            fh.write(_CACHE_HEADER)
            for k, v in sorted(cache.items()):
                sv = _cache_serialize(v)
                qv = _cache_quote(sv)
                fh.write(f"{k}=${{{k}={qv}}}\n")
    except OSError as e:
        warn(f"cannot write cache file {cache_file}: {e}")


def get_dir_arg(key: str, default: str) -> str:
    """Return a standard directory argument value, or *default* if not supplied."""
    return _dir_args.get(key, default)


def check_help() -> bool:
    """If --help was requested, print help.  Call after all option() calls."""
    if _help_requested:
        _print_help()
        return True
    return False


def _print_help() -> None:
    """Print configure --help output matching autoconf format."""
    lines: list[str] = []
    a = lines.append
    a("Usage: configure [OPTION]... [VAR=VALUE]...")
    a("")
    a("To assign environment variables (e.g., CC, CFLAGS...), specify them as")
    a(
        "VAR=VALUE.  See below for descriptions of some of the useful variables."
    )
    a("")
    a("Defaults for the options are specified in brackets.")
    a("")
    a("Configuration:")
    a("  -h, --help              display this help and exit")
    a("  -V, --version           display version information and exit")
    a("  -q, --quiet, --silent   do not print 'checking ...' messages")
    a("  -n, --no-create         do not create output files")
    a("  --cache-file=FILE       cache test results in FILE [disabled]")
    a("  -C                      alias for --cache-file=config.cache")
    a(
        "  --srcdir=DIR            find the sources in DIR [configure dir or '..']"
    )
    a(
        "  --disable-option-checking  ignore unrecognized --enable/--with options"
    )
    a("")
    a("Installation directories:")
    a(
        "  --prefix=PREFIX         install architecture-independent files in PREFIX"
    )
    a("                          [/usr/local]")
    a(
        "  --exec-prefix=EPREFIX   install architecture-dependent files in EPREFIX"
    )
    a("                          [PREFIX]")
    a("")
    a("By default, 'make install' will install all the files in")
    a("'/usr/local/bin', '/usr/local/lib' etc.  You can specify")
    a("an installation prefix other than '/usr/local' using '--prefix',")
    a("for instance '--prefix=$HOME'.")
    a("")
    a("For better control, use the options below.")
    a("")
    a("Fine tuning of the installation directories:")
    for argname, _key, default in _DIR_VARS[
        2:
    ]:  # skip prefix, exec-prefix (shown above)
        opt = f"  --{argname}=DIR"
        a(f"{opt:<38}[{default}]")
    a("")
    a("System types:")
    a("  --build=BUILD     configure for building on BUILD [guessed]")
    a(
        "  --host=HOST       cross-compile to build programs to run on HOST [BUILD]"
    )
    a("")

    # Optional Features
    features = [
        (k, v)
        for k, v in _options.items()
        if k.startswith("enable_") or k.startswith("disable_")
    ]
    if features:
        a("Optional Features:")
        a(
            "  --disable-FEATURE       do not include FEATURE (same as --enable-FEATURE=no)"
        )
        a("  --enable-FEATURE[=ARG]  include FEATURE [ARG=yes]")
        for key, info in features:
            flag = info["flag"].lstrip("-")
            metavar = info.get("metavar", "")
            help_text = info.get("help", "")
            opt = f"  --{flag}"
            if metavar:
                opt += f"[={metavar}]"
            if help_text:
                if len(opt) <= 38:
                    a(f"{opt:<38}{help_text}")
                else:
                    a(opt)
                    a(f"{'':38}{help_text}")
            else:
                a(opt)
        a("")

    # Optional Packages
    packages = [
        (k, v)
        for k, v in _options.items()
        if k.startswith("with_") or k.startswith("without_")
    ]
    if packages:
        a("Optional Packages:")
        a(
            "  --without-PACKAGE       do not use PACKAGE (same as --with-PACKAGE=no)"
        )
        a("  --with-PACKAGE[=ARG]    use PACKAGE [ARG=yes]")
        for key, info in packages:
            flag = info["flag"].lstrip("-")
            metavar = info.get("metavar", "")
            help_text = info.get("help", "")
            opt = f"  --{flag}"
            if metavar:
                opt += f"[={metavar}]"
            if help_text:
                if len(opt) <= 38:
                    a(f"{opt:<38}{help_text}")
                else:
                    a(opt)
                    a(f"{'':38}{help_text}")
            else:
                a(opt)
        a("")

    # Environment variables
    if _env_vars:
        a("Some influential environment variables:")
        for var, desc in _env_vars.items():
            opt = f"  {var}"
            if desc:
                if len(opt) <= 20:
                    a(f"{opt:<20}{desc}")
                else:
                    a(opt)
                    a(f"{'':20}{desc}")
            else:
                a(opt)
        a("")
    a(
        "Use these variables to override the choices made by 'configure' or to help"
    )
    a("it to find libraries and programs with nonstandard names/locations.")
    print("\n".join(lines))


@dataclass
class OptionSpec:
    """Autoconf-shaped option declaration and parsed value.

    ``_raw`` keeps autoconf's 4-state value model:
    - None (not provided)
    - "yes"
    - "no"
    - arbitrary string

    Use ``process_bool(v)`` or ``process_value(v)`` to resolve the value,
    emit checking/result output, and optionally set a var on *v*.
    """

    kind: str  # "enable" | "with"
    name: str
    display: str
    help: str = ""
    metavar: str = ""
    default: Any = None
    _raw: str | None = None
    var: str | None = None
    false_is: str | None = (
        None  # "no" for enable-style, "yes" for disable-style
    )

    @property
    def given(self) -> bool:
        return self._raw is not None

    def is_yes(self) -> bool:
        return self._raw == "yes"

    def is_no(self) -> bool:
        return self._raw == "no"

    @property
    def value(self) -> Any:
        """Resolved value: raw if given, else declared default."""
        return self._raw if self._raw is not None else self.default

    def value_or(self, default: Any) -> Any:
        return self._raw if self.given else default

    def report(self, checking_text: str, value: Any) -> Any:
        checking(checking_text)
        result(value)
        return value

    def process_bool(self, v: Any = None) -> bool:
        """Resolve option as a boolean, emit checking/result, optionally set var.

        Uses ``false_is`` to determine boolean interpretation:
        - ``false_is="no"``: value == "no" is False, anything else is True
          (enable-style, the most common pattern)
        - ``false_is="yes"``: value == "yes" is False, anything else is True
          (disable-style, e.g. --disable-gil)
        """
        value = self._raw if self._raw is not None else self.default
        if value is None:
            bool_value = False
        else:
            bool_value = value != self.false_is
        checking(f"for {self.display}")
        result("yes" if bool_value else "no")
        if self.var is not None and v is not None:
            setattr(v, self.var, bool_value)
        return bool_value

    def process_value(self, v: Any = None) -> Any:
        """Resolve option as raw value, emit checking/result, optionally set var.

        Returns ``self._raw`` if the user passed the flag, otherwise
        ``self.default``.
        """
        value = self._raw if self._raw is not None else self.default
        checking(f"for {self.display}")
        result(value)
        if self.var is not None and v is not None:
            setattr(v, self.var, value)
        return value


def _normalize_opt_name(name: str) -> str:
    """Normalize option logical names to underscore form."""
    return name.replace("-", "_")


def _option_key(kind: str, name: str) -> str:
    """Build the normalized _options key for arg_enable/arg_with."""
    return f"{kind}_{_normalize_opt_name(name)}"


def _parse_ac_arg(kind: str, name: str) -> str | None:
    """Parse an AC_ARG_ENABLE/AC_ARG_WITH style option from sys.argv.

    Last occurrence wins, matching autoconf CLI behaviour.
    Hyphens and underscores are treated as equivalent for matching names.
    """
    wanted = _normalize_opt_name(name)
    positive = "enable" if kind == "enable" else "with"
    negative = "disable" if kind == "enable" else "without"
    raw: str | None = None

    for token in sys.argv[1:]:
        if not token.startswith("--"):
            continue
        option_token = token[2:]
        if "=" in option_token:
            opt_name, opt_value = option_token.split("=", 1)
        else:
            opt_name, opt_value = option_token, None
        normalized = _normalize_opt_name(opt_name)

        if normalized == f"{positive}_{wanted}":
            raw = "yes" if opt_value is None else opt_value
            continue
        if normalized == f"{negative}_{wanted}":
            raw = "no" if opt_value is None else opt_value
            continue

    return raw


def arg_enable(
    name: str,
    *,
    display: str | None = None,
    help: str = "",
    metavar: str = "",
    default: Any = None,
    var: str | None = None,
    false_is: str | None = None,
) -> OptionSpec:
    """Declare and parse an AC_ARG_ENABLE option."""
    normalized_name = _normalize_opt_name(name)
    key = _option_key("enable", normalized_name)
    shown = display or f"--enable-{normalized_name.replace('_', '-')}"
    raw = _parse_ac_arg("enable", normalized_name)
    _options[key] = {
        "flag": shown,
        "metavar": metavar,
        "help": help,
        "default": default,
        "value": raw,
    }
    return OptionSpec(
        kind="enable",
        name=normalized_name.replace("_", "-"),
        display=shown,
        help=help,
        metavar=metavar,
        default=default,
        _raw=raw,
        var=var,
        false_is=false_is,
    )


def arg_with(
    name: str,
    *,
    display: str | None = None,
    help: str = "",
    metavar: str = "",
    default: Any = None,
    var: str | None = None,
    false_is: str | None = None,
) -> OptionSpec:
    """Declare and parse an AC_ARG_WITH option."""
    normalized_name = _normalize_opt_name(name)
    key = _option_key("with", normalized_name)
    shown = display or f"--with-{normalized_name.replace('_', '-')}"
    raw = _parse_ac_arg("with", normalized_name)
    _options[key] = {
        "flag": shown,
        "metavar": metavar,
        "help": help,
        "default": default,
        "value": raw,
    }
    return OptionSpec(
        kind="with",
        name=normalized_name.replace("_", "-"),
        display=shown,
        help=help,
        metavar=metavar,
        default=default,
        _raw=raw,
        var=var,
        false_is=false_is,
    )


def env_var(name: str, description: str = "") -> None:
    """Declare an environment variable accepted by configure (AC_ARG_VAR).

    Records the variable for help/documentation purposes, seeds its value
    from the process environment if present, and marks it as a precious
    variable so it is recorded in CONFIG_ARGS.
    """
    _env_vars[name] = description
    if name in os.environ:
        env[name] = os.environ[name]
    # Record in _original_config_args so CONFIG_ARGS includes it (like
    # autoconf's precious-variable mechanism).
    present = {
        a.split("=", 1)[0]
        for a in _original_config_args
        if "=" in a and not a.startswith("-")
    }
    if name not in present and name in os.environ:
        _original_config_args.append(f"{name}={os.environ[name]}")


def define(name: str, value: Any = 1, description: str = "") -> None:
    """Write a #define into pyconfig.h (AC_DEFINE).

    *value* is stored as-is (int or str).  String values will be quoted in the
    output; use define_unquoted() to suppress quoting.
    Stores into pyconf.defines[name] = (value, description, quoted=True).
    """
    defines[name] = (value, description, True)


def define_unquoted(name: str, value: Any, description: str = "") -> None:
    """Like define() but value is not quoted in the output (AC_DEFINE_UNQUOTED).

    Stores into pyconf.defines[name] = (value, description, quoted=False).
    """
    defines[name] = (value, description, False)


def define_template(name: str, description: str = "") -> None:
    """Register a pyconfig.h template entry without setting a value.

    Stores into pyconf.defines[name] = (None, description, False).
    """
    defines.setdefault(name, (None, description, False))


def is_defined(name: str) -> bool:
    """Return True if name has been defined (value is not None) in pyconf.defines."""
    entry = defines.get(name)
    return entry is not None and entry[0] is not None


def export(name: str, value: str | None = None) -> None:
    """Mark a variable for Makefile substitution (AC_SUBST).

    If *value* is given, also sets pyconf.substs[name] = value.
    If *value* is None, looks up the value in order:
      1. pyconf.vars (shared namespace)
      2. The caller's local variables (mirrors AC_SUBST reading a shell var)
      3. Falls back to "" as a placeholder.
    """
    if value is not None:
        substs[name] = format_yn(value) if isinstance(value, bool) else value
    else:
        # 1. Try shared vars namespace (only explicitly set attributes)
        val = vars.__dict__.get(name)
        if val is None:
            # 2. Try caller's local variables (AC_SUBST convention)
            import inspect

            frame = inspect.currentframe()
            try:
                caller_locals = (
                    frame.f_back.f_locals if frame and frame.f_back else {}
                )
            finally:
                del frame
            val = caller_locals.get(name)
        if val is not None:
            substs[name] = (
                format_yn(val) if isinstance(val, bool) else str(val)
            )
        elif name not in substs:
            substs[name] = ""


def config_files(files: list[str], chmod_x: bool = False) -> None:
    """List files to generate from templates (AC_CONFIG_FILES).

    *files*: list of output file paths (relative to build dir).
    *chmod_x*: if True, mark generated files as executable.
    Appended to pyconf.substs['_config_files'] for use by output().
    """
    existing = (
        substs.get("_config_files", "").split()
        if substs.get("_config_files")
        else []
    )
    existing.extend(files)
    substs["_config_files"] = " ".join(existing)
    if chmod_x:
        existing_x = (
            substs.get("_config_files_x", "").split()
            if substs.get("_config_files_x")
            else []
        )
        existing_x.extend(files)
        substs["_config_files_x"] = " ".join(existing_x)


def output() -> None:
    """Write pyconfig.h and process config files from accumulated defines/substs.

    Steps:
    1. Re-snapshot all explicitly exported vars with their final values.
    2. Populate module substs from accumulated module state.
    3. Write pyconfig.h from pyconf.defines into the build directory.
    4. For each file listed via config_files(), read <srcdir>/<file>.in,
       substitute all @VAR@ tokens from pyconf.substs, and write the output.
    Files marked chmod_x (via config_files(..., chmod_x=True)) are made executable.
    Skips file writing when --no-create is active.
    Saves probe cache when --cache-file is set.
    """
    _resolve_exports()
    _populate_module_substs()
    if cache_file:
        _save_cache()
    if not no_create:
        _write_pyconfig_h()
        _process_config_files()
        _write_config_status()


def _resolve_exports() -> None:
    """Re-snapshot all explicitly exported vars into substs with final values.

    v.export() registers a variable name and immediately snapshots its value
    into substs.  However, many variables are modified after the initial
    export() call (e.g. LIBS, BASECFLAGS).  This function does a final pass
    so that the last-assigned value wins, matching autoconf's AC_SUBST
    behaviour of capturing the value at output time.
    """
    for name in vars._exports:
        val = vars.__dict__.get(name)
        if val is not None:
            substs[name] = (
                format_yn(val) if isinstance(val, bool) else str(val)
            )
        elif name not in substs:
            substs[name] = ""
    # Finalize CONFIG_ARGS now that all env_var() calls have been made.
    substs["CONFIG_ARGS"] = " ".join(f"'{a}'" for a in _original_config_args)


def _populate_module_substs() -> None:
    """Populate MODULE_{NAME}_TRUE/_STATE/_CFLAGS/_LDFLAGS in substs,
    and build the MODULE_BLOCK string for @MODULE_BLOCK@ in Makefile.pre.in.

    AM_CONDITIONAL sets MODULE_{NAME}_TRUE = "" when the condition is true
    (module enabled+supported) or "#" when false (disabled/missing/n/a).
    These are used as @MODULE_{NAME}_TRUE@ tokens in Modules/Setup.stdlib.in.

    MODULE_BLOCK is a newline-separated block of all MODULE_*_STATE and
    optional MODULE_*_CFLAGS / MODULE_*_LDFLAGS assignments, substituted
    in one shot via @MODULE_BLOCK@ in Makefile.pre.in.
    """
    block_lines: list[str] = []
    for name, info in _stdlib_modules.items():
        key = "MODULE_" + name.upper().replace("-", "_")
        enabled = info.get("enabled", True)
        supported = info.get("supported", True)
        na = info.get("na", False)
        if na:
            state = "n/a"
        elif not enabled:
            state = "disabled"
        elif not supported:
            state = "missing"
        else:
            state = "yes"
        # _TRUE="" means "build this module"; only "yes" state builds
        substs[key + "_TRUE"] = "" if state == "yes" else "#"
        substs[key + "_STATE"] = state
        block_lines.append(f"{key}_STATE={state}")
        if "cflags" in info:
            cflags = str(info["cflags"] or "")
            substs.setdefault(key + "_CFLAGS", cflags)
            if state not in ("disabled", "n/a", "missing"):
                block_lines.append(f"{key}_CFLAGS={cflags}")
        if "ldflags" in info:
            ldflags = str(info["ldflags"] or "")
            substs.setdefault(key + "_LDFLAGS", ldflags)
            if state not in ("disabled", "n/a", "missing"):
                block_lines.append(f"{key}_LDFLAGS={ldflags}")
    substs["MODULE_BLOCK"] = "\n".join(block_lines)


def _write_pyconfig_h(path: str = "pyconfig.h") -> None:
    """Write pyconfig.h by processing pyconfig.h.in (like autoconf's config.status).

    Reads pyconfig.h.in from srcdir.  For each ``#undef NAME`` line, substitutes:
    - ``#define NAME value``  when the symbol was set via define() / define_unquoted()
    - ``/* #undef NAME */``   when it was explicitly undefined (value=None) or not set

    Also appends any pyconf.defines entries that don't appear in pyconfig.h.in
    (defines added by our configure that aren't in the standard template).
    Falls back to the old from-scratch approach if pyconfig.h.in is not found.
    """
    template_path = (
        os.path.join(srcdir, "pyconfig.h.in") if srcdir else "pyconfig.h.in"
    )
    if not os.path.exists(template_path):
        # Fall back: generate from scratch (no template available)
        _write_pyconfig_h_scratch(path)
        return

    def _render(name: str) -> str:
        if name not in defines:
            return f"/* #undef {name} */"
        value, desc, quoted = defines[name]
        if value is None:
            return f"/* #undef {name} */"
        if value == "" or value == 0:
            return f"#define {name} {value}"
        if isinstance(value, int):
            return f"#define {name} {value}"
        if quoted:
            return f'#define {name} "{value}"'
        return f"#define {name} {value}"

    # Match both "#undef NAME" and "# undef NAME" (indented, inside #ifndef blocks)
    undef_re = re.compile(r"^([ \t]*)#[ \t]*undef[ \t]+(\w+)\s*$")
    seen: set[str] = set()
    out_lines: list[str] = [
        "/* pyconfig.h.  Generated from pyconfig.h.in by configure.  */"
    ]
    with open(template_path) as fh:
        for raw in fh:
            line = raw.rstrip("\n")
            m = undef_re.match(line)
            if m:
                indent, name = m.group(1), m.group(2)
                seen.add(name)
                rendered = _render(name)
                # Re-apply indentation (for "# undef" inside #ifndef guards)
                if indent and rendered.startswith("#define "):
                    rendered = indent + "# " + rendered[1:]
                elif indent and rendered.startswith("/* #undef "):
                    rendered = indent + rendered
                out_lines.append(rendered)
            else:
                out_lines.append(line)

    # Do not append extras: autoconf config.status only outputs defines that are
    # in pyconfig.h.in. Defines not in the template are silently dropped.

    with open(path, "w") as fh:
        fh.write("\n".join(out_lines) + "\n")


def _write_pyconfig_h_scratch(path: str = "pyconfig.h") -> None:
    """Fallback: write pyconfig.h from scratch when no template is available."""
    lines = [
        "/* pyconfig.h — generated by pyconf */",
        "",
        "#ifndef Py_PYCONFIG_H",
        "#define Py_PYCONFIG_H",
        "",
    ]
    for name, entry in defines.items():
        value, description, quoted = entry
        if description:
            lines.append(f"/* {description} */")
        if value is None:
            lines.append(f"/* #undef {name} */")
        elif value == "" or value == 0:
            lines.append(f"#define {name} {value}")
        elif isinstance(value, int):
            lines.append(f"#define {name} {value}")
        elif quoted:
            lines.append(f'#define {name} "{value}"')
        else:
            lines.append(f"#define {name} {value}")
        lines.append("")
    lines += ["#endif  /* Py_PYCONFIG_H */", ""]
    with open(path, "w") as fh:
        fh.write("\n".join(lines))


def _subst_vars(text: str) -> str:
    """Replace all @VAR@ tokens in *text* with values from pyconf.substs.

    Replicates autoconf config.status behaviour:
    - When srcdir is "." (in-tree build), VPATH lines whose value reduces to
      only ".", "$(srcdir)", or "${srcdir}" are blanked out (leaving an empty
      line to preserve line numbers), matching autoconf's ac_vpsub logic.
    """
    import re

    def _replace(m: "re.Match[str]") -> str:
        key = m.group(1)
        val = substs.get(key, m.group(0))
        return str(val)

    result = re.sub(r"@([A-Za-z_][A-Za-z0-9_]*)@", _replace, text)
    # Neutralise VPATH when srcdir == "." (autoconf ac_vpsub / config.status):
    # remove sole ".", "$(srcdir)", "${srcdir}" entries; blank the line if empty.
    srcdir = substs.get("srcdir", "")
    if srcdir == ".":
        srcdir_tokens = r"(\.|\$\(srcdir\)|\$\{srcdir\})"

        def _neutralise_vpath(m: "re.Match[str]") -> str:
            val = m.group(3)
            # Strip all srcdir-equivalent colon-separated tokens
            cleaned = re.sub(
                r"(^|:)" + srcdir_tokens + r"(:|$)", ":", val
            ).strip(":")
            return "" if not cleaned.strip() else m.group(0)

        result = re.sub(
            r"^([ \t]*VPATH[ \t]*=)([ \t]*)(.*)$",
            _neutralise_vpath,
            result,
            flags=re.MULTILINE,
        )
    return result


def _process_config_files() -> None:
    """Read each *.in template and write the substituted output file."""
    files_str = substs.get("_config_files", "")
    if not files_str:
        return
    chmod_x_set = set(substs.get("_config_files_x", "").split())
    abs_top_srcdir = substs.get("abs_srcdir", srcdir or ".")
    abs_top_builddir = substs.get("abs_builddir", str(os.path.abspath(".")))
    for outfile in files_str.split():
        infile = (
            os.path.join(srcdir, f"{outfile}.in")
            if srcdir
            else f"{outfile}.in"
        )
        if not os.path.exists(infile):
            warn(f"config file template not found: {infile}")
            continue
        with open(infile) as fh:
            content = fh.read()
        # Compute per-file abs_srcdir/abs_builddir like autoconf does:
        # ac_abs_srcdir = abs_top_srcdir + dir-suffix-of-outfile
        outdir = os.path.dirname(outfile)
        if outdir:
            per_file_abs_srcdir = os.path.join(abs_top_srcdir, outdir)
            per_file_abs_builddir = os.path.join(abs_top_builddir, outdir)
        else:
            per_file_abs_srcdir = abs_top_srcdir
            per_file_abs_builddir = abs_top_builddir
        saved_abs_srcdir = substs.get("abs_srcdir")
        saved_abs_builddir = substs.get("abs_builddir")
        substs["abs_srcdir"] = per_file_abs_srcdir
        substs["abs_builddir"] = per_file_abs_builddir
        content = _subst_vars(content)
        substs["abs_srcdir"] = saved_abs_srcdir or ""
        substs["abs_builddir"] = saved_abs_builddir or ""
        if outdir:
            os.makedirs(outdir, exist_ok=True)
        with open(outfile, "w") as fh:
            fh.write(content)
        if outfile in chmod_x_set:
            import stat

            current = os.stat(outfile).st_mode
            os.chmod(
                outfile, current | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
            )


def _write_config_status() -> None:
    """Write config.status with saved state embedded after ``exit 0``.

    The shell preamble handles ``--recheck`` and delegates file
    regeneration to ``pyconf.py --config-status``.  All SUBST/DEFINES
    data is appended after the ``@CONFIG_STATUS_DATA@`` marker so the
    shell never sees it but Python can read it back.
    """
    config_args = substs.get("CONFIG_ARGS", "")
    sd = srcdir or "."
    pyconf_path = os.path.join(sd, "Tools/configure/pyconf.py")

    script = f"""\
#!/bin/sh
# Generated by configure.  Do not edit.
#
# This script is a minimal replacement for autoconf's config.status.
# It can regenerate output files from their .in templates and re-run
# configure with --recheck.

CONFIG_ARGS="{config_args}"

for arg in "$@"; do
  case "$arg" in
    --recheck)
      echo "running configure with args: $CONFIG_ARGS"
      exec ./configure $CONFIG_ARGS
      ;;
  esac
done

# Regenerate files.  pyconf.py reads saved data from the end of $0.
exec python3 {pyconf_path} --config-status "$0" "$@"

exit 0
"""

    with open("config.status", "w") as fh:
        fh.write(script)

        # Append data section after exit 0.
        fh.write("@CONFIG_STATUS_DATA@\n")
        fh.write("@SUBST@\n")
        for key, val in substs.items():
            lines = val.split("\n") if val else [""]
            fh.write(f"{key}\n{len(lines)}\n")
            for line in lines:
                fh.write(f"{line}\n")
        fh.write("@DEFINES@\n")
        for name, entry in defines.items():
            value, description, quoted = entry
            # Use repr() so ast.literal_eval() restores the exact type
            fh.write(f"{name}\n{repr(value)}\n{description}\n")
            fh.write(f"{repr(quoted)}\n")
        fh.write("@END@\n")

    import stat

    current = os.stat("config.status").st_mode
    os.chmod(
        "config.status", current | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    )


def _load_config_status_data(path: str) -> None:
    """Read saved SUBST/DEFINES data from the end of *path*.

    The file is expected to contain a ``@CONFIG_STATUS_DATA@`` marker
    followed by ``@SUBST@``, key/nlines/value blocks, ``@DEFINES@``,
    key/value/desc/quoted blocks, and ``@END@``.
    """
    import ast

    global substs, defines, srcdir

    with open(path) as fh:
        # Skip to the data marker
        for line in fh:
            if line.rstrip("\n") == "@CONFIG_STATUS_DATA@":
                break
        else:
            raise RuntimeError(f"no @CONFIG_STATUS_DATA@ marker in {path}")

        # Expect @SUBST@
        tag = fh.readline().rstrip("\n")
        assert tag == "@SUBST@", f"expected @SUBST@, got {tag!r}"

        substs = {}
        while True:
            key = fh.readline().rstrip("\n")
            if key == "@DEFINES@":
                break
            nlines = int(fh.readline().rstrip("\n"))
            val_lines = [fh.readline().rstrip("\n") for _ in range(nlines)]
            substs[key] = "\n".join(val_lines)

        defines = {}
        while True:
            name = fh.readline().rstrip("\n")
            if name == "@END@":
                break
            value = ast.literal_eval(fh.readline().rstrip("\n"))
            description = fh.readline().rstrip("\n")
            quoted = ast.literal_eval(fh.readline().rstrip("\n"))
            defines[name] = (value, description, quoted)

    srcdir = substs.get("srcdir", ".")


def _config_status_main(argv: list[str]) -> None:
    """Entry point when config.status invokes pyconf for file regeneration.

    Loads saved state from config.status (embedded after the
    ``@CONFIG_STATUS_DATA@`` marker) and regenerates the requested files.

    Supports:
    - ``./config.status``  — regenerate all config files and pyconfig.h
    - ``./config.status Makefile.pre``  — regenerate only Makefile.pre
    - ``CONFIG_FILES=Makefile.pre CONFIG_HEADERS= ./config.status``
    """
    global substs, defines, srcdir

    # First positional arg is the path to config.status itself
    cs_path = argv[0] if argv else "config.status"
    rest = argv[1:]

    _load_config_status_data(cs_path)

    # Determine which files to process
    file_args = [a for a in rest if not a.startswith("-")]
    env_files = os.environ.get("CONFIG_FILES")
    env_headers = os.environ.get("CONFIG_HEADERS")

    if file_args:
        # Regenerate only the specified files
        requested = set(file_args)
    elif env_files is not None:
        # CONFIG_FILES env var specifies which files to process
        requested = set(env_files.split()) if env_files else set()
    else:
        requested = None  # means "all"

    # Should we regenerate pyconfig.h?
    do_headers = True
    if env_headers is not None:
        # CONFIG_HEADERS= (empty) means skip headers
        do_headers = bool(env_headers)
    if file_args:
        # If specific files were requested, only do headers if pyconfig.h is listed
        assert requested is not None
        do_headers = "pyconfig.h" in requested

    if do_headers:
        _write_pyconfig_h()

    # Regenerate config files
    all_files_str = substs.get("_config_files", "")
    if not all_files_str:
        return
    chmod_x_set = set(substs.get("_config_files_x", "").split())

    for outfile in all_files_str.split():
        if requested is not None and outfile not in requested:
            continue
        infile = (
            os.path.join(srcdir, f"{outfile}.in")
            if srcdir
            else f"{outfile}.in"
        )
        if not os.path.exists(infile):
            warn(f"config file template not found: {infile}")
            continue
        with open(infile) as fh:
            content = fh.read()
        content = _subst_vars(content)
        outdir = os.path.dirname(outfile)
        if outdir:
            os.makedirs(outdir, exist_ok=True)
        with open(outfile, "w") as fh:
            fh.write(content)
        if outfile in chmod_x_set:
            import stat

            current = os.stat(outfile).st_mode
            os.chmod(
                outfile, current | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
            )


def _is_verbose() -> bool:
    """Return True if output should be printed (not quiet and not --help mode)."""
    return not quiet and not _help_requested


def checking(what: str) -> None:
    """Print 'checking <what>...' on stdout without a newline (AC_MSG_CHECKING).

    The matching result() call prints the answer on the same line.
    Suppressed when --quiet / -q is active or --help was requested.
    """
    global _result_pending
    _result_pending = True
    if _is_verbose():
        print(f"checking {what}... ", end="", flush=True)


def result(value: Any) -> None:
    """Print the result of the most recent checking() call (AC_MSG_RESULT).

    Completes the line started by checking().
    Boolean True/False are formatted as 'yes'/'no' to match autoconf behaviour.
    Suppressed when --quiet / -q is active or --help was requested.
    """
    global _result_pending
    _result_pending = False
    if _is_verbose():
        if value is True:
            value = "yes"
        elif value is False:
            value = "no"
        print(str(value))


checking_fn = checking  # alias used by link_check to avoid param shadowing


def cache_check(description: str, cache_var: str) -> Any:
    """AC_CACHE_CHECK — check the cache before running a probe.

    Prints "checking <description>..." and looks up *cache_var* in the
    cache (which may have been pre-seeded from CONFIG_SITE).

    Returns the cached value on a hit (also prints "(cached) <value>")
    or None on a miss.  The caller is responsible for running the actual
    probe on a miss and storing the result with
    ``cache_store(cache_var, value)`` followed by ``result(value)``.

    Typical usage::

        val = pyconf.cache_check("getaddrinfo bug", "ac_cv_buggy_getaddrinfo")
        if val is None:
            val = <run probe>
            pyconf.cache_store("ac_cv_buggy_getaddrinfo", val)
            pyconf.result(val)
    """
    checking(description)
    cached = cache.get(cache_var)
    if cached is not None:
        result(f"(cached) {_format_result(cached)}")
        return cached
    return None


def cache_store(cache_var: str, value: Any) -> None:
    """Store *value* in the cache under *cache_var*."""
    cache[cache_var] = value


def _format_result(value: Any) -> str:
    """Format a cache value for display (True→'yes', False→'no')."""
    if value is True:
        return "yes"
    if value is False:
        return "no"
    return str(value)


def _flush_result(value: Any) -> None:
    """Emit result(value) if a checking() is pending, otherwise do nothing.

    Called by probe functions that follow a bare checking() call.
    """
    if _result_pending:
        result(value)


def _print_cached() -> None:
    """Print '(cached) ' inline if a checking() line is pending.

    Matches autoconf's behaviour of printing '(cached) ' between
    'checking <what>...' and the result value when the answer comes
    from the cache file.
    """
    if _result_pending and _is_verbose():
        print("(cached) ", end="", flush=True)


def warn(msg: str) -> None:
    """Print a configure warning to stderr (AC_MSG_WARN)."""
    print(f"configure: WARNING: {msg}", file=sys.stderr)


def notice(msg: str) -> None:
    """Print an informational notice to stdout with script name prefix (AC_MSG_NOTICE)."""
    if _is_verbose():
        print(f"configure: {msg}")


def error(msg: str) -> None:
    """Print an error message to stderr and exit with code 1 (AC_MSG_ERROR)."""
    print(f"configure: error: {msg}", file=sys.stderr)
    sys.exit(1)


def fatal(msg: str) -> None:
    """Print a fatal error message to stderr and exit with code 1.

    Alias for error() with a slightly different prefix, matching autoconf's
    AC_MSG_FAILURE behaviour.
    """
    print(f"configure: fatal error: {msg}", file=sys.stderr)
    sys.exit(1)


def use_system_extensions() -> None:
    """AC_USE_SYSTEM_EXTENSIONS — enable POSIX/XSI/BSD extensions.

    Defines the standard feature-test macros so that headers expose their
    full APIs.  This mirrors what autoconf's AC_USE_SYSTEM_EXTENSIONS does.
    """
    define_unquoted("_ALL_SOURCE", 1, "Enable extensions on AIX 3, Interix.")
    define_unquoted("_DARWIN_C_SOURCE", 1, "Enable extensions on Mac OS X.")
    define_unquoted(
        "_GNU_SOURCE", 1, "Enable GNU extensions on systems that have them."
    )
    define_unquoted(
        "_HPUX_ALT_XOPEN_SOCKET_API", 1, "Enable extensions on HP-UX."
    )
    define_unquoted("_NETBSD_SOURCE", 1, "Enable extensions on NetBSD.")
    define_unquoted("_OPENBSD_SOURCE", 1, "Enable extensions on OpenBSD.")
    define_unquoted(
        "_POSIX_PTHREAD_SEMANTICS",
        1,
        "Enable threading extensions on Solaris.",
    )
    define_unquoted(
        "__STDC_WANT_IEC_60559_ATTRIBS_EXT__",
        1,
        "Enable IEC 60559 attribs extensions.",
    )
    define_unquoted(
        "__STDC_WANT_IEC_60559_BFP_EXT__",
        1,
        "Enable IEC 60559 BFP extensions.",
    )
    define_unquoted(
        "__STDC_WANT_IEC_60559_DFP_EXT__",
        1,
        "Enable IEC 60559 DFP extensions.",
    )
    define_unquoted(
        "__STDC_WANT_IEC_60559_EXT__", 1, "Enable IEC 60559 extensions."
    )
    define_unquoted(
        "__STDC_WANT_IEC_60559_FUNCS_EXT__",
        1,
        "Enable IEC 60559 funcs extensions.",
    )
    define_unquoted(
        "__STDC_WANT_IEC_60559_TYPES_EXT__",
        1,
        "Enable IEC 60559 types extensions.",
    )
    define_unquoted(
        "__STDC_WANT_LIB_EXT2__", 1, "Enable ISO C 2001 extensions."
    )
    define_unquoted(
        "__STDC_WANT_MATH_SPEC_FUNCS__",
        1,
        "Enable extensions specified in the standard.",
    )
    define_unquoted("_TANDEM_SOURCE", 1, "Enable extensions on HP NonStop.")
    define_unquoted(
        "__EXTENSIONS__", 1, "Enable general extensions on Solaris."
    )


def run(
    cmd: str, *, vars: dict[str, str] | None = None, **kwargs: Any
) -> subprocess.CompletedProcess:
    """Run a shell-like command string, expanding $VAR / ${VAR} references.

    *cmd* is tokenized with shlex.split() (respecting quoting and backslash
    escapes), then each token has shell-variable references expanded using
    *vars* (defaults to os.environ).  The resulting argv list is passed to
    subprocess.run() along with any extra *kwargs*.

    This mirrors how configure.ac shell lines work: whitespace separates
    tokens, quoting protects spaces inside arguments, and $VAR is substituted
    from the environment.  Example::

        pyconf.run("$CPP $CPPFLAGS Misc/platform_triplet.c",
                   vars={"CPP": "gcc -E", "CPPFLAGS": "-I."}, ...)
        # runs: ["gcc", "-E", "-I.", "Misc/platform_triplet.c"]
    """
    lookup = vars if vars is not None else dict(os.environ)
    expanded = re.sub(
        r"\$\{(\w+)\}|\$(\w+)",
        lambda m: lookup.get(m.group(1) or m.group(2), ""),
        cmd,
    )
    argv = shlex.split(expanded)
    return subprocess.run(argv, **kwargs)


def canonical_host() -> None:
    """AC_CANONICAL_HOST equivalent — detect host/build/cross_compiling.

    Uses config.guess and config.sub from the source tree when available,
    matching autoconf's behaviour.  Falls back to Python's platform module.

    Parses --host=TRIPLET and --build=TRIPLET from sys.argv.  When --host
    is given and differs from the build triplet, sets cross_compiling=True.
    """
    global cross_compiling, host, host_cpu, build

    # Locate config.guess/config.sub in the source tree (same directory as
    # configure-old, i.e. the project root).
    aux_dir = srcdir or "."
    config_guess = os.path.join(aux_dir, "config.guess")
    config_sub = os.path.join(aux_dir, "config.sub")

    def _canonicalize(triplet: str) -> str:
        """Canonicalize a triplet through config.sub if available."""
        if os.path.isfile(config_sub) and os.access(config_sub, os.X_OK):
            try:
                return subprocess.check_output(
                    [config_sub, triplet], text=True, stderr=subprocess.DEVNULL
                ).strip()
            except (subprocess.CalledProcessError, OSError):
                pass
        return triplet

    # --host and --build are parsed by init_args() into _system_type_args
    host_arg = _system_type_args.get("host")
    build_arg = _system_type_args.get("build")

    # Determine build triplet
    if build_arg:
        build = _canonicalize(build_arg)
    elif os.path.isfile(config_guess) and os.access(config_guess, os.X_OK):
        try:
            raw = subprocess.check_output(
                [config_guess], text=True, stderr=subprocess.DEVNULL
            ).strip()
            build = _canonicalize(raw)
        except (subprocess.CalledProcessError, OSError):
            build = _fallback_triplet()
    else:
        build = _fallback_triplet()

    # Parse the triplet: cpu-vendor-os (autoconf splits on '-')
    parts = build.split("-", 2)
    build_cpu = parts[0]

    # Determine host triplet and cross-compilation status.
    # Matches autoconf: --host without --build sets cross_compiling="maybe".
    if host_arg:
        host = _canonicalize(host_arg)
        host_parts = host.split("-", 2)
        host_cpu = host_parts[0]
        if build_arg:
            cross_compiling = host != build
        else:
            # autoconf sets "maybe" when --host is given without --build
            cross_compiling = "maybe" if host != build else False
    else:
        # For native builds, host == build
        host = build
        host_cpu = build_cpu
        cross_compiling = False

    # Record build_alias / host_alias in CONFIG_ARGS when --build/--host were
    # given on the command line (matching autoconf's precious-variable behaviour).
    # Insert before the first VAR=VALUE env-var entry so the order matches
    # autoconf: flags first, then aliases, then env vars.
    present = {
        a.split("=", 1)[0]
        for a in _original_config_args
        if "=" in a and not a.startswith("-")
    }
    if build_arg and "build_alias" not in present:
        # Find insertion point: just before the first NAME=VALUE entry.
        insert_pos = next(
            (
                i
                for i, a in enumerate(_original_config_args)
                if "=" in a and not a.startswith("-")
            ),
            len(_original_config_args),
        )
        _original_config_args.insert(insert_pos, f"build_alias={build_arg}")
        insert_pos += 1
        if host_arg and "host_alias" not in present:
            _original_config_args.insert(insert_pos, f"host_alias={host_arg}")
    elif host_arg and "host_alias" not in present:
        insert_pos = next(
            (
                i
                for i, a in enumerate(_original_config_args)
                if "=" in a and not a.startswith("-")
            ),
            len(_original_config_args),
        )
        _original_config_args.insert(insert_pos, f"host_alias={host_arg}")


def _fallback_triplet() -> str:
    """Construct a host triplet from Python's platform module (fallback)."""
    import platform as _platform

    machine = _platform.machine()
    system = _platform.system().lower()
    release = _platform.release()
    # Try to match GNU config.guess output format
    if system == "darwin":
        return f"{machine}-apple-{system}{release}"
    elif system == "linux":
        return f"{machine}-pc-{system}-gnu"
    else:
        return f"{machine}-pc-{system}-gnu"


def _find_working_cc(candidates: list[str]) -> str:
    """Find the first compiler in *candidates* that can compile a trivial program."""
    src = "int main(void) { return 0; }\n"
    for cand in candidates:
        if not shutil.which(cand):
            continue
        try:
            with tempfile.TemporaryDirectory() as tmp:
                srcf = os.path.join(tmp, "conftest.c")
                objf = os.path.join(tmp, "conftest.o")
                with open(srcf, "w") as f:
                    f.write(src)
                r = subprocess.run(
                    f"{cand} -c {shlex.quote(srcf)} -o {shlex.quote(objf)}",
                    shell=True,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
                if r.returncode == 0:
                    return cand
        except OSError:
            continue
    # Fall back to "cc" even if not verified
    return "cc"


def find_compiler(cc: str | None = None, cpp: str | None = None) -> None:
    """AC_PROG_CC equivalent — find and probe the C compiler.

    On macOS, replicates the configure.ac logic that detects llvm-gcc and
    falls back to clang.  Sets pyconf.CC, pyconf.CPP, pyconf.GCC,
    pyconf.ac_cv_cc_name, pyconf.ac_cv_gcc_compat, and pyconf.ac_cv_prog_cc_g.
    """
    global CC, CPP, GCC, ac_cv_cc_name, ac_cv_gcc_compat, ac_cv_prog_cc_g
    env_cc = os.environ.get("CC", "")

    if cc:
        CC = cc
    elif env_cc:
        CC = env_cc
    else:
        # macOS-specific: replicate configure.ac lines 4904-4959
        # Check if gcc is actually llvm-gcc and fall back to clang
        if sys.platform == "darwin":
            CC = _macos_find_cc()
        else:
            # Standard AC_PROG_CC search order: gcc, cc
            # Verify the compiler can actually compile a trivial program,
            # since a compiler may be installed but broken (e.g. FreeBSD
            # gcc package without cc1).
            CC = _find_working_cc(["gcc", "cc"])

    CPP = cpp or os.environ.get("CPP", "") or f"{CC} -E"

    # Probe the actual compiler identity by checking --version output,
    # not just the binary name.  On macOS, /usr/bin/gcc is Apple clang.
    _identify_compiler()
    ac_cv_prog_cc_g = True


def _macos_find_cc() -> str:
    """macOS CC selection, matching configure.ac logic.

    Searches PATH for gcc and clang.  If both found, checks whether gcc
    is actually llvm-gcc (or Apple clang masquerading as gcc) and falls
    back to clang.  Also sets CXX as a side effect when appropriate.
    """
    global CC
    found_gcc = None
    found_clang = None
    for d in os.environ.get("PATH", "").split(os.pathsep):
        if not d:
            continue
        gcc_path = os.path.join(d, "gcc")
        clang_path = os.path.join(d, "clang")
        if (
            found_gcc is None
            and os.path.isfile(gcc_path)
            and os.access(gcc_path, os.X_OK)
        ):
            found_gcc = gcc_path
        if (
            found_clang is None
            and os.path.isfile(clang_path)
            and os.access(clang_path, os.X_OK)
        ):
            found_clang = clang_path
        if found_gcc and found_clang:
            break

    if found_gcc and found_clang:
        # Check if gcc is llvm-gcc
        try:
            ver = subprocess.check_output(
                [found_gcc, "--version"], text=True, stderr=subprocess.DEVNULL
            )
            if "llvm-gcc" in ver:
                notice("Detected llvm-gcc, falling back to clang")
                return found_clang
        except (subprocess.CalledProcessError, OSError):
            pass
        # gcc found and is not llvm-gcc — use it (AC_PROG_CC default)
        return "gcc"
    elif not found_gcc and found_clang:
        notice("No GCC found, use CLANG")
        return found_clang
    elif not found_gcc and not found_clang:
        # Try xcrun
        try:
            found_clang = subprocess.check_output(
                ["/usr/bin/xcrun", "-find", "clang"],
                text=True,
                stderr=subprocess.DEVNULL,
            ).strip()
            if found_clang:
                notice("Using clang from Xcode.app")
                return found_clang
        except (subprocess.CalledProcessError, OSError):
            pass
    # Default: gcc found, clang not — use gcc
    return "gcc"


def _identify_compiler() -> None:
    """Probe the C compiler's true identity via its --version output.

    On macOS, /usr/bin/gcc is Apple Clang, so just checking the binary
    name is not sufficient.  This sets ac_cv_cc_name, ac_cv_gcc_compat,
    and GCC to match what autoconf's AC_PROG_CC + later checks produce.
    """
    global GCC, ac_cv_cc_name, ac_cv_gcc_compat
    # Try to get the compiler's self-identification
    cc_basename = os.path.basename(CC.split()[0]) if CC else ""
    try:
        ver = subprocess.check_output(
            [CC, "--version"], text=True, stderr=subprocess.DEVNULL
        )
    except (subprocess.CalledProcessError, OSError):
        ver = ""

    # Detect emcc (Emscripten) before clang — emcc's --version output
    # contains "clang" but autoconf identifies it as "emcc" via the
    # __EMSCRIPTEN__ preprocessor macro.
    if cc_basename == "emcc" or "emscripten" in ver.lower():
        ac_cv_cc_name = "emcc"
        ac_cv_gcc_compat = True
        GCC = True  # emcc is gcc-compatible
    elif "clang" in ver.lower():
        ac_cv_cc_name = "clang"
        ac_cv_gcc_compat = True
        GCC = True  # clang is gcc-compatible
    elif "clang" in cc_basename:
        ac_cv_cc_name = "clang"
        ac_cv_gcc_compat = True
        GCC = True
    elif (
        "gcc" in cc_basename
        or "GCC" in ver
        or "Free Software Foundation" in ver
    ):
        ac_cv_cc_name = "gcc"
        ac_cv_gcc_compat = True
        GCC = True
    else:
        ac_cv_cc_name = "unknown"
        ac_cv_gcc_compat = False
        GCC = False


def find_install() -> None:
    """AC_PROG_INSTALL — find the install program.

    Sets INSTALL, INSTALL_PROGRAM, INSTALL_SCRIPT, INSTALL_DATA in substs.
    """
    for candidate in ("ginstall", "install"):
        path = shutil.which(candidate)
        if path:
            substs["INSTALL"] = path + " -c"
            break
    else:
        substs["INSTALL"] = "install -c"
    substs.setdefault("INSTALL_PROGRAM", "${INSTALL}")
    substs.setdefault("INSTALL_SCRIPT", "${INSTALL}")
    substs.setdefault("INSTALL_DATA", "${INSTALL} -m 644")


def find_mkdir_p() -> None:
    """AC_PROG_MKDIR_P — find mkdir -p equivalent.

    Matches autoconf logic: only use full path if a coreutils/BusyBox mkdir
    is found (identified by --version output).  Falls back to plain 'mkdir -p'.
    """
    # Search PATH (+ /opt/sfw/bin) for coreutils/BusyBox mkdir or gmkdir
    search_path = os.environ.get("PATH", "") + os.pathsep + "/opt/sfw/bin"
    found = None
    for d in search_path.split(os.pathsep):
        if not d:
            continue
        for prog in ("mkdir", "gmkdir"):
            p = os.path.join(d, prog)
            if not (os.path.isfile(p) and os.access(p, os.X_OK)):
                continue
            try:
                ver = subprocess.check_output(
                    [p, "--version"], text=True, stderr=subprocess.STDOUT
                )
                # autoconf accepts coreutils, BusyBox, or fileutils 4.1
                if (
                    "coreutils)" in ver
                    or "BusyBox " in ver
                    or "fileutils) 4.1" in ver
                ):
                    found = p
                    break
            except (subprocess.CalledProcessError, OSError):
                continue
        if found:
            break
    substs["MKDIR_P"] = f"{found} -p" if found else "mkdir -p"


def check_prog(prog: str, path: str | None = None, default: str = "") -> str:
    """AC_CHECK_PROG / AC_PATH_TOOL — find a single program in PATH (or *path* if given).

    Returns the full path if found, else *default*.
    """
    found = shutil.which(prog, path=path)
    return found if found else default


def check_progs(progs: list[str], default: str = "") -> str:
    """AC_CHECK_PROGS — return the first program found in PATH.

    Returns: first found program name or *default*.
    """
    for p in progs:
        if shutil.which(p):
            return p
    return default


def check_tools(progs: list[str], default: str = "") -> str:
    """Find the first tool (AR, RANLIB, etc.) present in PATH.

    Alias for check_progs(); exists to mirror autoconf's tool-search macros.
    """
    return check_progs(progs, default=default)


def _confdefs_preamble() -> str:
    """Generate #define/#undef lines from current pyconf.defines (like confdefs.h).

    Prepending this to test sources ensures checks see the same definitions
    as the real configure would via autoconf's incrementally-built confdefs.h.
    """
    lines: list[str] = []
    for name, entry in defines.items():
        value, desc, quoted = entry
        if value is None:
            lines.append(f"/* #undef {name} */")
        elif value == "" or value == 0:
            lines.append(f"#define {name} {value}")
        elif isinstance(value, int):
            lines.append(f"#define {name} {value}")
        elif quoted:
            lines.append(f'#define {name} "{value}"')
        else:
            lines.append(f"#define {name} {value}")
    return "\n".join(lines) + "\n" if lines else ""


def _run_cc(src: str, out: str, cflags: str = "", libs: str = "") -> bool:
    """Run CC to compile/link *src* → *out* using shell=True.

    *cflags* goes between CC and the source file.
    *libs* goes after the source file (standard linker ordering).
    Uses the shell to split CC (so CC="ccache gcc" works) and expand quoting.
    Returns True if the compiler exits 0.
    """
    # Use vars.CC if available (may include flags like -pthread added by
    # setup_pthreads), falling back to the module-level CC.
    cc = getattr(vars, "CC", None) or CC
    # shlex.quote protects paths that may contain spaces
    cmd = f"{cc} {cflags} {shlex.quote(src)} -o {shlex.quote(out)} {libs}"
    try:
        r = subprocess.run(
            cmd,
            shell=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return r.returncode == 0
    except OSError:
        return False


def _compile_test(source: str, extra_flags: list[str] | None = None) -> bool:
    """Compile *source* with CC and return True if it succeeds.

    Uses a temporary directory so the working tree is never touched.
    Returns False if CC is not set or compilation fails.
    Includes both CPPFLAGS and CFLAGS, matching autoconf's AC_COMPILE_IFELSE.
    """
    if not CC:
        return False
    vars_cppflags = getattr(vars, "CPPFLAGS", "")
    vars_cflags = getattr(vars, "CFLAGS", "")
    flags = " ".join(shlex.quote(f) for f in (extra_flags or []))
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "conftest.c")
        obj = os.path.join(tmp, "conftest.o")
        with open(src, "w") as f:
            f.write(_confdefs_preamble() + source)
        return _run_cc(
            src,
            obj,
            cflags=f"{vars_cppflags} {vars_cflags} {flags} -c".strip(),
        )


def _link_test(source: str, extra_flags: list[str] | None = None) -> bool:
    """Compile-and-link *source* and return True if it succeeds.

    Automatically includes pyconf.vars.CFLAGS and pyconf.vars.LIBS so that
    probes pick up the same flags that the rest of the build uses.
    Libraries (-l flags) in extra_flags are separated and placed after the
    source file (required by GNU ld); other flags stay before the source.
    """
    if not CC:
        return False
    vars_cflags = getattr(vars, "CFLAGS", "")
    vars_cppflags = getattr(vars, "CPPFLAGS", "")
    vars_ldflags = getattr(vars, "LDFLAGS", "")
    vars_libs = getattr(vars, "LIBS", "")
    # Split extra_flags into compiler flags and linker -l/-L/-Wl, flags
    pre_flags: list[str] = []
    lib_flags: list[str] = []
    for f in extra_flags or []:
        if f.startswith("-l") or f.startswith("-L") or f.startswith("-Wl,"):
            lib_flags.append(f)
        else:
            pre_flags.append(f)
    cflags = f"{vars_cppflags} {vars_cflags} {vars_ldflags} {' '.join(shlex.quote(f) for f in pre_flags)}".strip()
    libs = f"{' '.join(shlex.quote(f) for f in lib_flags)} {vars_libs}".strip()
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "conftest.c")
        exe = os.path.join(tmp, "conftest")
        with open(src, "w") as f:
            f.write(_confdefs_preamble() + source)
        return _run_cc(src, exe, cflags=cflags, libs=libs)


def check_compile_flag(
    flag: str, extra_flags: list[str] | None = None, extra_cflags: str = ""
) -> bool:
    """Return True if CC accepts *flag* (compile-only test).

    Uses a minimal C source file so the test is purely about flag acceptance.
    """
    src = "int main(void) { return 0; }\n"
    # Split flag in case it contains multiple flags separated by spaces
    flag_parts = flag.split()
    all_flags = flag_parts + list(extra_flags or []) + extra_cflags.split()
    return _compile_test(src, extra_flags=all_flags)


def check_linker_flag(flag: str, value: str | None = None) -> str:
    """Return *value* (default: *flag*) if the linker accepts *flag*, else ''.

    If *value* is None, returns *flag* itself on success.
    """
    src = "int main(void) { return 0; }\n"
    if _link_test(src, extra_flags=[flag]):
        return value if value is not None else flag
    return ""


def compile_check(
    *,
    description: str = "",
    preamble: str = "",
    includes: list[str] | None = None,
    body: str = "",
    program: str = "",
    cc: str = "",
    extra_flags: list[str] | None = None,
    extra_cflags: str = "",
    cache_var: str = "",
    then: Any = None,
    otherwise: Any = None,
    else_: Any = None,
) -> Any:
    """AC_COMPILE_IFELSE — test if a C program compiles successfully.

    Builds source from *preamble*/*includes*/*body* (AC_COMPILE_IFELSE style)
    or from *program* directly.  Optionally uses *cc* instead of CC and
    *extra_flags* added to the compiler invocation.

    *then* / *otherwise* (or *else_*) may be callables (called on success/failure)
    or plain values (returned on success/failure).  Returns True/False when neither
    is given; returns the chosen value when one of them is a non-callable.
    """
    inc_lines = "".join(f"#include <{h}>\n" for h in (includes or []))
    if program:
        src = (
            program
            if program.strip().startswith("#") or "main" in program
            else (f"int main(void) {{\n{program}\nreturn 0;\n}}\n")
        )
    else:
        # Ensure body ends with a semicolon so it's valid C inside main()
        body_s = body.rstrip()
        if body_s and not body_s.endswith(";") and not body_s.endswith("}"):
            body_s += ";"
        src = f"{preamble}\n{inc_lines}\nint main(void) {{\n{body_s}\nreturn 0;\n}}\n"

    flags = list(extra_flags or []) + extra_cflags.split()
    if cc:
        # Override CC temporarily for this test
        saved = globals().get("CC", "")
        globals()["CC"] = cc
        ok = _compile_test(src, extra_flags=flags)
        globals()["CC"] = saved
    else:
        ok = _compile_test(src, extra_flags=flags)

    else_val = otherwise if otherwise is not None else else_
    chosen = then if ok else else_val
    if callable(chosen):
        chosen()
        return ok
    if chosen is not None:
        return chosen
    return ok


def link_check(
    prologue: str = "",
    body: str = "",
    *,
    preamble: str = "",
    source: str = "",
    includes: list[str] | None = None,
    extra_cflags: str = "",
    extra_libs: str = "",
    cache_var: str = "",
    checking: str = "",
) -> bool:
    """AC_LINK_IFELSE — test if code compiles and links. Returns bool.

    Two calling conventions are supported:
      link_check(prologue, body, checking="for foo")  — body goes inside main()
      link_check(description, full_source)            — full_source used as-is

    In the second form, if *body* contains 'main' or starts with '#', it is
    treated as a complete source file and *prologue* is treated as *checking*.
    *source* keyword always overrides and is used as-is.
    *extra_cflags* / *extra_libs* are added to the compile/link command.
    """
    # Detect (description, full_source) calling convention:
    # link_check("some description", """#include... int main...""")
    body_is_source = (
        body.lstrip().startswith("#") or "main" in body or "int main" in body
    )
    checking_label = checking
    if body and body_is_source and not source and not preamble:
        # Treat prologue as the checking description, body as full source
        if not checking_label:
            checking_label = prologue
        source = body
        prologue = ""
        body = ""

    # Detect when prologue is itself a complete program (contains int main)
    # link_check("""#include... int main...""") — no body
    prologue_is_source = (
        not body
        and not source
        and not preamble
        and ("int main" in prologue or "return 0;" in prologue)
    )
    if prologue_is_source:
        source = prologue
        prologue = ""

    if checking_label:
        checking_fn(checking_label)
    pre = preamble or prologue
    if source:
        src = source
    else:
        inc_lines = "".join(f"#include <{h}>\n" for h in (includes or []))
        src = (
            f"{pre}\n{inc_lines}\nint main(void) {{\n{body};\nreturn 0;\n}}\n"
        )
    flags = extra_cflags.split() + extra_libs.split()
    ok = _link_test(src, extra_flags=flags if flags else None)
    if checking_label:
        result(ok)
    return ok


def try_link(source: str, extra_flags: list[str] | None = None) -> bool:
    """Compile-and-link *source*; return True on success."""
    return _link_test(source, extra_flags=extra_flags)


def compile_link_check(description: str, compiler: str, source: str) -> str:
    """Compile-and-link *source* using the given *compiler* command.

    Prints "checking <description>..." and the result when description is given,
    matching AC_CACHE_CHECK behaviour in configure.ac.
    Returns 'yes' on success, 'no' on failure.
    """
    if description:
        checking(description)
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "conftest.c")
        exe = os.path.join(tmp, "conftest")
        with open(src, "w") as f:
            f.write(_confdefs_preamble() + source)
        cmd = f"{compiler} {shlex.quote(src)} -o {shlex.quote(exe)}"
        try:
            result_proc = subprocess.run(
                cmd,
                shell=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            ret = "yes" if result_proc.returncode == 0 else "no"
            if description:
                result(ret)
            return ret
        except OSError:
            if description:
                result("no")
            return "no"


def run_check(
    description_or_source: str,
    source: str = "",
    *,
    cross_default: Any = None,
    default: Any = None,
    cross_compiling_default: Any = None,
    cross_compiling_result: Any = None,
    success: Any = None,
    failure: Any = None,
    on_success_return: Any = None,
    on_failure_return: Any = None,
    extra_cflags: str = "",
    extra_libs: str = "",
    cache_var: str = "",
) -> Any:
    """AC_RUN_IFELSE / AC_CACHE_CHECK — compile, link, and run a C program.

    May be called as run_check(source) or run_check(description, source).
    When called as run_check(description, source), prints "checking <description>..."
    and the result, matching AC_CACHE_CHECK behaviour in configure.ac.
    Returns *success* if the program exits 0, *failure* otherwise.
    Returns *cross_default* (or *default* if given) when cross-compiling.
    *extra_cflags* and *extra_libs* are added to the compile/link command.
    """
    desc = description_or_source if source else ""
    code = source if source else description_or_source
    if desc:
        checking(desc)
    # Resolve alias params; default to True/False so callers can use `if result:`
    success = (
        True
        if on_success_return is None and success is None
        else (on_success_return if on_success_return is not None else success)
    )
    failure = (
        False
        if on_failure_return is None and failure is None
        else (on_failure_return if on_failure_return is not None else failure)
    )

    def _first_set(*vals: Any) -> Any:
        for v in vals:
            if v is not None:
                return v
        return False

    xdefault = _first_set(
        default, cross_compiling_result, cross_compiling_default, cross_default
    )
    if cross_compiling:
        if desc:
            result(xdefault)
        return xdefault
    # Include vars.CFLAGS, vars.LIBS etc. to match autoconf's AC_RUN_IFELSE
    # which inherits the accumulated compiler/linker flags (e.g. -pthread).
    vars_cflags = getattr(vars, "CFLAGS", "")
    vars_cppflags = getattr(vars, "CPPFLAGS", "")
    vars_ldflags = getattr(vars, "LDFLAGS", "")
    vars_libs = getattr(vars, "LIBS", "")
    all_cflags = (
        f"{vars_cppflags} {vars_cflags} {vars_ldflags} {extra_cflags}".strip()
    )
    all_libs = f"{extra_libs} {vars_libs}".strip()
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "conftest.c")
        exe = os.path.join(tmp, "conftest")
        with open(src, "w") as f:
            f.write(_confdefs_preamble() + code)
        try:
            if not _run_cc(src, exe, cflags=all_cflags, libs=all_libs):
                if desc:
                    result(failure)
                return failure
            run = subprocess.run(
                [exe], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )
            ret = success if run.returncode == 0 else failure
            if desc:
                result(ret)
            return ret
        except OSError:
            if desc:
                result(failure)
            return failure


def run_check_with_cc_flag(
    description: str, flag: str, source: str, cross_default: bool = False
) -> bool:
    """Compile+run *source* with an extra CC *flag*. Returns bool.

    Prints "checking <description>..." and the result, matching
    AC_CACHE_CHECK behaviour in configure.ac.
    Returns *cross_default* when cross-compiling.
    """
    if description:
        checking(description)
    if cross_compiling:
        if description:
            result(cross_default)
        return cross_default
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "conftest.c")
        exe = os.path.join(tmp, "conftest")
        with open(src, "w") as f:
            f.write(_confdefs_preamble() + source)
        try:
            if not _run_cc(src, exe, cflags=shlex.quote(flag)):
                if description:
                    result(False)
                return False
            run = subprocess.run(
                [exe], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )
            ok = run.returncode == 0
            if description:
                result(ok)
            return ok
        except OSError:
            if description:
                result(False)
            return False


def run_program_output(
    source: str,
    extra_cflags: str = "",
    extra_ldflags: str = "",
    extra_libs: str = "",
) -> str:
    """Compile, link, run *source*, and return its stdout. Returns '' on failure."""
    if cross_compiling:
        return ""
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "conftest.c")
        exe = os.path.join(tmp, "conftest")
        with open(src, "w") as f:
            f.write(_confdefs_preamble() + source)
        cflags = f"{extra_cflags} {extra_ldflags}".strip()
        try:
            if not _run_cc(src, exe, cflags=cflags, libs=extra_libs):
                return ""
            run = subprocess.run([exe], capture_output=True, text=True)
            return run.stdout if run.returncode == 0 else ""
        except OSError:
            return ""


# ---------------------------------------------------------------------------
# Compile-time integer computation (for cross-compilation)
# ---------------------------------------------------------------------------


def _compute_int(expr: str, includes: str = "") -> int | None:
    """Determine the compile-time value of a C integer expression.

    Uses the same technique as autoconf's AC_COMPUTE_INT / _AC_COMPUTE_INT:
    a binary search using compile-time assertions of the form
    ``static int test_array [1 - 2 * !(EXPR)];`` which fail to compile
    when the expression is false (negative-size array).

    Works even when cross-compiling because it only needs to *compile*,
    never to *run* the test program.

    Returns the integer value, or None if it could not be determined.
    """

    def _try(boolean_expr: str) -> bool:
        src = (
            f"{includes}\n"
            "int main(void) {\n"
            f"  static int test_array [1 - 2 * !({boolean_expr})];\n"
            "  test_array[0] = 0;\n"
            "  return test_array[0];\n"
            "}\n"
        )
        return _compile_test(src)

    # Step 1: determine sign
    if _try(f"({expr}) >= 0"):
        # Non-negative: search upward from 0
        ac_lo = 0
        ac_mid = 0
        while True:
            if _try(f"({expr}) <= {ac_mid}"):
                ac_hi = ac_mid
                break
            ac_lo = ac_mid + 1
            if ac_lo <= ac_mid:
                # Overflow
                return None
            ac_mid = 2 * ac_mid + 1
    elif _try(f"({expr}) < 0"):
        # Negative: search downward from -1
        ac_hi = -1
        ac_mid = -1
        while True:
            if _try(f"({expr}) >= {ac_mid}"):
                ac_lo = ac_mid
                break
            ac_hi = ac_mid - 1
            if ac_mid <= ac_hi:
                # Overflow
                return None
            ac_mid = 2 * ac_mid
    else:
        return None

    # Step 2: binary search between lo and hi
    while ac_lo != ac_hi:
        ac_mid = (ac_hi - ac_lo) // 2 + ac_lo
        if _try(f"({expr}) <= {ac_mid}"):
            ac_hi = ac_mid
        else:
            ac_lo = ac_mid + 1

    return ac_lo


# ---------------------------------------------------------------------------
# Header / struct / member probes
# ---------------------------------------------------------------------------


def _header_name_to_define(header: str) -> str:
    """Convert 'sys/types.h' → 'HAVE_SYS_TYPES_H'.

    Dashes are mapped to '_DASH_' to match autoconf's convention where
    'gdbm-ndbm.h' produces HAVE_GDBM_DASH_NDBM_H (distinct from
    'gdbm/ndbm.h' → HAVE_GDBM_NDBM_H).
    """
    return "HAVE_" + header.upper().replace("-", "_DASH_").replace(
        "/", "_"
    ).replace(".", "_")


def _ac_includes_default() -> str:
    """Generate the AC_INCLUDES_DEFAULT preamble.

    This matches autoconf's behavior: conditionally include standard system
    headers that have already been detected.  Many system headers on BSDs
    require sys/types.h or other standard headers to be included first.
    """
    # These are the headers autoconf's AC_INCLUDES_DEFAULT uses.
    _default_headers = [
        ("HAVE_STDIO_H", "stdio.h"),
        ("HAVE_STDLIB_H", "stdlib.h"),
        ("HAVE_STRING_H", "string.h"),
        ("HAVE_INTTYPES_H", "inttypes.h"),
        ("HAVE_STDINT_H", "stdint.h"),
        ("HAVE_STRINGS_H", "strings.h"),
        ("HAVE_SYS_TYPES_H", "sys/types.h"),
        ("HAVE_SYS_STAT_H", "sys/stat.h"),
        ("HAVE_UNISTD_H", "unistd.h"),
    ]
    lines: list[str] = []
    for have_def, hdr in _default_headers:
        entry = defines.get(have_def)
        if entry is not None and entry[0] == 1:
            lines.append(f"#include <{hdr}>")
    return "\n".join(lines) + "\n" if lines else ""


def check_header(
    header: str,
    extra_cflags: str = "",
    extra_includes: str = "",
    autodefine: bool = True,
) -> bool:
    """Check for a single C header. Returns True if found; defines HAVE_<HEADER>.

    *extra_cflags*: extra compiler flags (e.g. -I paths) for this test.
    *extra_includes*: extra #include lines prepended to the test source.
    *autodefine*: if False, do not automatically define HAVE_<HEADER> when found.
      (Use when the caller has a custom action, like AC_CHECK_HEADER with non-empty action.)
    Caches results in pyconf.cache under key 'ac_cv_header_<name>'.

    Like autoconf's AC_CHECK_HEADERS, the default includes (stdio.h,
    stdlib.h, sys/types.h, etc.) are prepended to the test source unless
    *extra_includes* already supplies a custom preamble.
    """
    if not _RE_HEADER_NAME.match(header):
        raise ValueError(
            f"check_header: invalid header name {header!r} "
            f"(must match [a-z0-9][a-z0-9_./+-]*)"
        )
    cache_key = "ac_cv_header_" + header.replace("-", "_dash_").replace(
        "/", "_"
    ).replace(".", "_")
    define_name = _header_name_to_define(header)
    if cache_key in cache and not extra_cflags and not extra_includes:
        _print_cached()
        found = cache[cache_key]
        setattr(vars, cache_key, found)
        _header_map[define_name] = header
        if found and autodefine:
            define(
                define_name,
                1,
                f"Define to 1 if you have the <{header}> header file.",
            )
        return found
    # Use AC_INCLUDES_DEFAULT preamble when no custom extra_includes given.
    preamble = extra_includes if extra_includes else _ac_includes_default()
    src = f"{preamble}\n#include <{header}>\nint main(void) {{ return 0; }}\n"
    flags = extra_cflags.split() if extra_cflags else None
    found = _compile_test(src, extra_flags=flags)
    if not extra_cflags and not extra_includes:
        cache[cache_key] = found
        setattr(vars, cache_key, found)
    # Populate reverse map for conditional_headers lookup
    _header_map[define_name] = header
    if found and autodefine:
        define(
            define_name,
            1,
            f"Define to 1 if you have the <{header}> header file.",
        )
    return found


def check_headers(
    *headers: str | list, prologue: str = "", defines: dict | None = None
) -> bool:
    """AC_CHECK_HEADERS — check multiple headers; define HAVE_<HEADER> for each found.

    May be called as check_headers("a.h", "b.h") or check_headers(["a.h", "b.h"]).
    *prologue*: extra C code placed before the #include under test.
    *defines*: optional dict to record results (header → True/False).
    Returns True if ALL listed headers are found.
    """
    flat: list[str] = []
    for h in headers:
        if isinstance(h, list):
            flat.extend(h)
        else:
            flat.append(h)
    all_found = True
    for h in flat:
        checking(f"for {h}")
        found = check_header(h, extra_includes=prologue)
        result(found)
        all_found = all_found and found
        if defines is not None:
            defines[h] = found
    return all_found


def check_define(header: str, define_name: str) -> bool:
    """Check if macro *define_name* is #defined in *header*. Returns bool."""
    src = (
        f"#include <{header}>\n"
        f"#ifndef {define_name}\n"
        f"#error not defined\n"
        f"#endif\n"
        f"int main(void) {{ return 0; }}\n"
    )
    return _compile_test(src)


def check_member(member: str, includes: list[str] | None = None) -> bool:
    """AC_CHECK_MEMBER — check for a struct.field member. Returns bool.

    *member* is 'struct_name.field', e.g. 'struct stat.st_blksize'.
    Defines HAVE_<STRUCT>_<FIELD> as a side-effect if found.
    """
    if not _RE_MEMBER_NAME.match(member):
        raise ValueError(
            f"check_member: invalid member {member!r} "
            f"(must match [a-z_][a-z0-9_ ]*.field)"
        )
    parts = member.split(".", 1)
    if len(parts) != 2:
        return False
    struct_name, field = parts
    if includes is not None:
        inc_lines = "".join(f"#include <{h}>\n" for h in includes)
    else:
        inc_lines = _ac_includes_default()
    src = (
        f"{inc_lines}\n"
        f"int main(void) {{\n"
        f"  {struct_name} s;\n"
        f"  (void)s.{field};\n"
        f"  return 0;\n"
        f"}}\n"
    )
    found = _compile_test(src)
    define_name = (
        "HAVE_" + struct_name.upper().replace(" ", "_") + "_" + field.upper()
    )
    if found:
        define(define_name, 1, f"Define to 1 if '{member}' exists.")
    return found


def check_members(
    *members: str | list, includes: list[str] | None = None
) -> None:
    """AC_CHECK_MEMBERS — check multiple struct members; define HAVE_<MEMBER> for each.

    May be called as check_members("a.b", "c.d") or check_members(["a.b", "c.d"]).
    """
    flat: list[str] = []
    for m in members:
        if isinstance(m, list):
            flat.extend(m)
        else:
            flat.append(m)
    for m in flat:
        checking(f"for {m}")
        found = check_member(m, includes=includes)
        result(found)


def check_struct_tm() -> None:
    """AC_STRUCT_TM — Check struct tm for tm_zone member; defines HAVE_STRUCT_TM_TM_ZONE.

    AC_STRUCT_TM only checks tm_zone, not tm_gmtoff.
    Also defines HAVE_TM_ZONE if tm_zone is present (autoconf compat).
    """
    if check_member("struct tm.tm_zone", includes=["time.h"]):
        define("HAVE_TM_ZONE", 1, "Define to 1 if your struct tm has tm_zone.")


def check_struct_timezone() -> None:
    """Check for 'struct timezone'; defines HAVE_STRUCT_TIMEZONE if present."""
    src = (
        "#include <sys/time.h>\n"
        "int main(void) { struct timezone tz; (void)tz; return 0; }\n"
    )
    if _compile_test(src):
        define(
            "HAVE_STRUCT_TIMEZONE", 1, "Define to 1 if struct timezone exists."
        )


# ---------------------------------------------------------------------------
# Type / symbol probes
# ---------------------------------------------------------------------------


def _type_to_define(type_: str) -> str:
    """Convert 'unsigned long long' → 'UNSIGNED_LONG_LONG'."""
    return type_.upper().replace(" ", "_").replace("*", "P").replace("/", "_")


def check_sizeof(
    type_: str,
    default: int | None = None,
    headers: list[str] | None = None,
    includes: list[str] | None = None,
) -> int:
    """AC_CHECK_SIZEOF — determine sizeof(*type_*).

    When not cross-compiling, compiles and runs a program that prints the
    size.  When cross-compiling (or when the run fails), uses a compile-time
    binary search matching autoconf's AC_COMPUTE_INT technique.
    Falls back to *default* (or 0) only if both methods fail.
    Defines SIZEOF_<TYPE> as a side-effect.
    """
    cache_key = "ac_cv_sizeof_" + _type_to_define(type_).lower()
    if cache_key in cache:
        _print_cached()
        size = int(cache[cache_key])
        define_name = "SIZEOF_" + _type_to_define(type_)
        define(
            define_name, size, f"The size of `{type_}', as computed by sizeof."
        )
        return size
    extra_includes = "".join(
        f"#include <{h}>\n" for h in (headers or includes or [])
    )
    size = None
    if not cross_compiling:
        src = (
            "#include <stdio.h>\n"
            "#include <stddef.h>\n"
            "#include <stdint.h>\n" + extra_includes + "int main(void) {\n"
            f'  printf("%zu\\n", sizeof({type_}));\n'
            "  return 0;\n"
            "}\n"
        )
        output_str = run_program_output(src)
        if output_str.strip().isdigit():
            size = int(output_str.strip())
    if size is None:
        # Cross-compiling or run failed: compile-time binary search.
        # Use AC_INCLUDES_DEFAULT (which includes stdio.h etc.) so that
        # types like fpos_t are visible, matching autoconf behaviour.
        inc = _ac_includes_default() + extra_includes
        size = _compute_int(f"(long int)(sizeof({type_}))", inc)
    if size is None:
        size = default if default is not None else 0
    cache[cache_key] = str(size)
    define_name = "SIZEOF_" + _type_to_define(type_)
    define(define_name, size, f"The size of `{type_}', as computed by sizeof.")
    return size


def check_alignof(type_: str) -> int:
    """AC_CHECK_ALIGNOF — determine alignment of *type_*.

    When not cross-compiling, runs a program.  When cross-compiling, uses
    the compile-time binary search (offsetof on a struct with a leading
    char member, matching autoconf's AC_CHECK_ALIGNOF).
    Returns alignment as int; defines ALIGNOF_<TYPE> as a side-effect.
    """
    cache_key = "ac_cv_alignof_" + _type_to_define(type_).lower()
    if cache_key in cache:
        _print_cached()
        alignment = int(cache[cache_key])
        define_name = "ALIGNOF_" + _type_to_define(type_)
        define(
            define_name,
            alignment,
            f"The alignment of `{type_}', as computed by _Alignof.",
        )
        return alignment
    alignment = None
    if not cross_compiling:
        src = (
            "#include <stdio.h>\n"
            "#include <stddef.h>\n"
            "int main(void) {\n"
            f'  printf("%zu\\n", _Alignof({type_}));\n'
            "  return 0;\n"
            "}\n"
        )
        output_str = run_program_output(src)
        if output_str.strip().isdigit():
            alignment = int(output_str.strip())
    if alignment is None:
        # Cross-compiling or run failed: compile-time binary search using
        # offsetof(struct{char c; TYPE x;}, x), matching autoconf.
        inc = "#include <stddef.h>\n"
        expr = f"(long int)(offsetof(struct {{ char c; {type_} x; }}, x))"
        alignment = _compute_int(expr, inc)
    if alignment is None:
        alignment = 0
    cache[cache_key] = str(alignment)
    define_name = "ALIGNOF_" + _type_to_define(type_)
    define(
        define_name,
        alignment,
        f"The alignment of `{type_}', as computed by _Alignof.",
    )
    return alignment


def check_type(
    name: str,
    fallback_define: tuple | None = None,
    headers: list[str] | None = None,
    extra_includes: str | list[str] = "",
) -> bool:
    """AC_CHECK_TYPE — check for C type *name*.

    *fallback_define*: (alias, base_type, desc) — if the type is absent, emit
    ``typedef base_type alias;`` by calling define().
    *extra_includes*: extra header string(s) prepended to the test source.
    Returns True if the type exists.
    """
    inc_lines = "".join(f"#include <{h}>\n" for h in (headers or []))
    if isinstance(extra_includes, list):
        extra_inc_str = "".join(f"#include <{h}>\n" for h in extra_includes)
    else:
        extra_inc_str = extra_includes
    src = (
        f"{inc_lines}\n"
        f"{extra_inc_str}\n"
        f"int main(void) {{ {name} x; (void)x; return 0; }}\n"
    )
    found = _compile_test(src)
    if found:
        # AC_CHECK_TYPES defines HAVE_<TYPE> when found
        have_name = "HAVE_" + _type_to_define(name)
        define(
            have_name, 1, f"Define to 1 if the system has the type `{name}'."
        )
    elif fallback_define is not None:
        alias, base_type, desc = fallback_define
        define(alias, base_type, desc)
    return found


def check_c_const() -> None:
    """AC_C_CONST — check for const keyword; defines 'const' to empty if missing."""
    src = "int main(void) {\n  const int x = 0;\n  (void)x;\n  return 0;\n}\n"
    if not _compile_test(src):
        define(
            "const",
            "",
            "Define to empty if 'const' does not conform to ANSI C.",
        )


def check_c_bigendian() -> None:
    """AC_C_BIGENDIAN — detect byte order.

    Matches autoconf's multi-stage detection:
    1. Check sys/param.h BYTE_ORDER macros (compile-only, cross-safe).
    2. Check limits.h _BIG_ENDIAN/_LITTLE_ENDIAN (compile-only, cross-safe).
    3. When cross-compiling: link a program containing marker strings and
       grep the binary for 'BIGenDianSyS' / 'LiTTleEnDian'.
    4. When not cross-compiling: run a test program.

    Defines WORDS_BIGENDIAN if big-endian, AC_APPLE_UNIVERSAL_BUILD if
    universal.
    """
    result_val = "unknown"

    # Stage 1: sys/param.h BYTE_ORDER macros
    if result_val == "unknown":
        src_has_macros = (
            "#include <sys/types.h>\n"
            "#include <sys/param.h>\n"
            "int main(void) {\n"
            "#if ! (defined BYTE_ORDER && defined BIG_ENDIAN \\\n"
            "       && defined LITTLE_ENDIAN && BYTE_ORDER \\\n"
            "       && BIG_ENDIAN && LITTLE_ENDIAN)\n"
            "  bogus endian macros\n"
            "#endif\n"
            "  return 0;\n"
            "}\n"
        )
        if _compile_test(src_has_macros):
            src_is_big = (
                "#include <sys/types.h>\n"
                "#include <sys/param.h>\n"
                "int main(void) {\n"
                "#if BYTE_ORDER != BIG_ENDIAN\n"
                "  not big endian\n"
                "#endif\n"
                "  return 0;\n"
                "}\n"
            )
            result_val = "yes" if _compile_test(src_is_big) else "no"

    # Stage 2: limits.h _LITTLE_ENDIAN / _BIG_ENDIAN (e.g. Solaris)
    if result_val == "unknown":
        src_has_macros = (
            "#include <limits.h>\n"
            "int main(void) {\n"
            "#if ! (defined _LITTLE_ENDIAN || defined _BIG_ENDIAN)\n"
            "  bogus endian macros\n"
            "#endif\n"
            "  return 0;\n"
            "}\n"
        )
        if _compile_test(src_has_macros):
            src_is_big = (
                "#include <limits.h>\n"
                "int main(void) {\n"
                "#ifndef _BIG_ENDIAN\n"
                "  not big endian\n"
                "#endif\n"
                "  return 0;\n"
                "}\n"
            )
            result_val = "yes" if _compile_test(src_is_big) else "no"

    # Stage 3/4: runtime test or object-file grep
    if result_val == "unknown":
        if cross_compiling:
            # Link a program with marker strings, grep the binary.
            marker_src = (
                "unsigned short int ascii_mm[] =\n"
                "  { 0x4249, 0x4765, 0x6E44, 0x6961, 0x6E53, 0x7953, 0 };\n"
                "unsigned short int ascii_ii[] =\n"
                "  { 0x694C, 0x5454, 0x656C, 0x6E45, 0x6944, 0x6E61, 0 };\n"
                "int use_ascii(int i) {\n"
                "  return ascii_mm[i] + ascii_ii[i];\n"
                "}\n"
                "int main(int argc, char **argv) {\n"
                "  char *p = argv[0];\n"
                "  ascii_mm[1] = *p++; ascii_ii[1] = *p++;\n"
                "  return use_ascii(argc);\n"
                "}\n"
            )
            with tempfile.TemporaryDirectory() as tmp:
                src_path = os.path.join(tmp, "conftest.c")
                exe_path = os.path.join(tmp, "conftest")
                with open(src_path, "w") as f:
                    f.write(_confdefs_preamble() + marker_src)
                if _run_cc(src_path, exe_path):
                    try:
                        data = open(exe_path, "rb").read()
                    except OSError:
                        data = b""
                    has_big = b"BIGenDianSyS" in data
                    has_little = b"LiTTleEnDian" in data
                    if has_big and not has_little:
                        result_val = "yes"
                    elif has_little and not has_big:
                        result_val = "no"
        else:
            src = (
                "#include <stdio.h>\n"
                "int main(void) {\n"
                "  unsigned int x = 1;\n"
                "  unsigned char *p = (unsigned char *)&x;\n"
                '  printf("%d\\n", p[0] == 0 ? 1 : 0);\n'
                "  return 0;\n"
                "}\n"
            )
            output_str = run_program_output(src).strip()
            if output_str == "1":
                result_val = "yes"
            elif output_str == "0":
                result_val = "no"

    if result_val == "yes":
        define(
            "WORDS_BIGENDIAN",
            1,
            "Define to 1 if your processor stores words big-endian.",
        )


def ax_c_float_words_bigendian(
    on_big: Any = None, on_little: Any = None, on_unknown: Any = None
) -> None:
    """Check float word ordering; calls on_big/on_little/on_unknown callbacks.

    Uses the same compile-and-grep technique as the autoconf
    AX_C_FLOAT_WORDS_BIGENDIAN macro: the magic constant
    9.090423496703681e+223 encodes as bytes containing 'noonsees' when
    float words are big-endian and 'seesnoon' when little-endian.
    We compile (link) the test program and search the resulting object
    file for these marker strings, which works even when cross-compiling.
    """
    src = (
        "#include <stdlib.h>\n"
        "static double m[] = {9.090423496703681e+223, 0.0};\n"
        "int main(int argc, char *argv[]) {\n"
        "  m[atoi(argv[1])] += atof(argv[2]);\n"
        "  return m[atoi(argv[3])] > 0.0;\n"
        "}\n"
    )
    with tempfile.TemporaryDirectory() as tmp:
        src_path = os.path.join(tmp, "conftest.c")
        exe_path = os.path.join(tmp, "conftest")
        with open(src_path, "w") as f:
            f.write(src)
        if not _run_cc(src_path, exe_path):
            if callable(on_unknown):
                on_unknown()
            return
        # Read all conftest* output files and look for the marker strings.
        # Autoconf uses "grep noonsees conftest*" which globs all outputs;
        # this is important for Emscripten where the compiler may produce
        # conftest.js + conftest.wasm instead of a single conftest binary.
        data = b""
        for name in os.listdir(tmp):
            if not name.startswith("conftest") or name == "conftest.c":
                continue
            path = os.path.join(tmp, name)
            try:
                data += open(path, "rb").read()
            except OSError:
                pass
        if not data:
            if callable(on_unknown):
                on_unknown()
            return
        has_big = b"noonsees" in data
        has_little = b"seesnoon" in data
        if has_big and not has_little and callable(on_big):
            on_big()
        elif has_little and not has_big and callable(on_little):
            on_little()
        elif callable(on_unknown):
            on_unknown()


# ---------------------------------------------------------------------------
# Function / declaration probes
# ---------------------------------------------------------------------------


def check_func(
    func: str,
    headers: list[str] | None = None,
    includes: list[str] | None = None,
    conditional_headers: list[str] | None = None,
    define: str = "",
    body: str = "",
    autodefine: bool = True,
) -> bool:
    """AC_CHECK_FUNC — check for C function *func*. Returns bool.

    *headers* / *includes*: headers to include in the test.
    *conditional_headers*: list of HAVE_* define names; each header is only
      included when the corresponding define is set.
    *define*: override the define name written on success (default: HAVE_<FUNC>).
    *body*: custom function-call expression to use in main() (default: func()).
    Defines HAVE_<FUNC> (or *define*) as a side-effect if found.
    Caches results in pyconf.cache.
    """
    if not _RE_FUNC_NAME.match(func):
        raise ValueError(
            f"check_func: invalid function name {func!r} "
            f"(must match [a-z_][a-z0-9_]*)"
        )
    cache_key = f"ac_cv_func_{func}"
    if cache_key in cache:
        _print_cached()
        found = cache[cache_key]
        setattr(vars, cache_key, found)
        define_name = define if define else "HAVE_" + func.upper()
        if found and autodefine:
            define_unquoted(
                define_name,
                1,
                f"Define to 1 if you have the `{func}' function.",
            )
        return found
    all_headers = list(headers or []) + list(includes or [])
    # Build the set of headers that are covered by conditional_headers, so we
    # don't unconditionally include a header that may not exist on this system.
    cond_header_paths = set()
    for cond_h in conditional_headers or []:
        path = _header_map.get(cond_h, "")
        if path:
            cond_header_paths.add(path)
    # Only unconditionally include headers NOT already in conditional_headers.
    unconditional = [h for h in all_headers if h not in cond_header_paths]
    inc_lines = "".join(f"#include <{h}>\n" for h in unconditional)
    cond_lines = ""
    for cond_h in conditional_headers or []:
        # cond_h is like "HAVE_SYS_MMAN_H" → header "sys/mman.h"
        # Look up the original header path from the defines/header_map dict.
        h = _header_map.get(cond_h, "")
        if not h:
            # Fallback: HAVE_SYS_MMAN_H → sys_mman_h → sys/mman.h
            # Strip HAVE_ prefix and _H suffix, lowercase, replace _ with /
            # for known directory prefixes.
            raw = cond_h.removeprefix("HAVE_").lower()
            if raw.endswith("_h"):
                raw = raw[:-2]
            # Replace first _ with / if the prefix is a known directory
            known_dirs = {
                "sys",
                "linux",
                "netinet",
                "netpacket",
                "net",
                "arpa",
                "machine",
                "asm",
                "bluetooth",
                "readline",
                "editline",
                "openssl",
                "db",
                "gdbm",
                "uuid",
                "pthread",
            }
            parts = raw.split("_", 1)
            if len(parts) == 2 and parts[0] in known_dirs:
                h = parts[0] + "/" + parts[1].replace("_", "/", 0) + ".h"
            else:
                h = raw.replace("_", "/") + ".h"
        cond_lines += f"#ifdef {cond_h}\n#include <{h}>\n#endif\n"
    # Two strategies matching autoconf:
    # - With headers (PY_CHECK_FUNC style): compile-only, take address of func
    #   ("void *x = func") — requires real prototype from headers.
    # - Without headers (AC_CHECK_FUNC style): link test with stub prototype
    #   ("char func(); func()") — works even without a real declaration.
    if body:
        call = body
    elif all_headers or cond_lines:
        call = f"(void)(&{func})"
    else:
        call = f"{func}()"
    # Only emit a conflicting prototype when no real headers are included;
    # with headers, rely on the real declaration to avoid conflicts.
    if all_headers or cond_lines:
        proto = ""
    else:
        proto = (
            f"/* Override any GCC internal prototype to avoid an error. */\n"
            f'#ifdef __cplusplus\nextern "C"\n#endif\n'
            f"char {func}();\n"
        )
    src = (
        f"{inc_lines}{cond_lines}\n"
        f"{proto}"
        f"int main(void) {{ {call}; return 0; }}\n"
    )
    # With headers: compile-only (PY_CHECK_FUNC style, address-of check).
    # Without headers: link test (AC_CHECK_FUNC style, stub + call).
    if all_headers or cond_lines:
        found = _compile_test(src)
    else:
        found = _link_test(src)
    cache[cache_key] = found
    setattr(vars, cache_key, found)
    define_name = define if define else "HAVE_" + func.upper()
    if found and autodefine:
        define_unquoted(
            define_name, 1, f"Define to 1 if you have the `{func}' function."
        )
    return found


def check_funcs(funcs: list[str], headers: list[str] | None = None) -> bool:
    """AC_CHECK_FUNCS — check multiple functions; define HAVE_<FUNC> for each found.

    Returns True if any function was found (matches AC_CHECK_FUNCS action-if-found
    semantics, where the action runs for each found function).
    """
    any_found = False
    for f in funcs:
        checking(f"for {f}")
        found = check_func(f, headers=headers)
        result(found)
        if found:
            any_found = True
    return any_found


def check_decl(
    decl: str,
    includes: list[str] | None = None,
    extra_includes: list[str] | None = None,
    on_found: Any = None,
    on_notfound: Any = None,
    define_name: str | None = None,
) -> bool:
    """AC_CHECK_DECL — check if declaration/macro *decl* is available.

    *includes* / *extra_includes*: headers to include in the test.
    *define_name*: if given, define this macro to 1 when found (0 when not).
    Calls *on_found()* or *on_notfound()* callbacks. Returns bool.
    """
    if not _RE_DECL_NAME.match(decl):
        raise ValueError(
            f"check_decl: invalid declaration name {decl!r} "
            f"(must match [A-Za-z_][A-Za-z0-9_]*)"
        )
    all_includes = list(includes or []) + list(extra_includes or [])
    inc_lines = "".join(f"#include <{h}>\n" for h in all_includes)
    # Use #ifndef guard so macros (e.g. NetBSD's dirfd) are detected too.
    # If decl is a macro, the #ifndef body is skipped and compilation
    # succeeds.  If it's a real declaration, (void)decl compiles.
    src = (
        f"{inc_lines}\n"
        f"int main(void) {{\n"
        f"#ifndef {decl}\n"
        f"  (void){decl};\n"
        f"#endif\n"
        f"  return 0;\n"
        f"}}\n"
    )
    found = _compile_test(src)
    if define_name:
        define(
            define_name,
            1 if found else 0,
            f"Define to 1 if you have the declaration of `{decl}'.",
        )
    if found and callable(on_found):
        on_found()
    elif not found and callable(on_notfound):
        on_notfound()
    return found


def check_decls(
    decls: list[str] | str,
    includes: list[str] | None = None,
    extra_includes: list[str] | None = None,
) -> None:
    """AC_CHECK_DECLS — check multiple declarations; define HAVE_DECL_<DECL> for each.

    *decls* may be a list of names or a single name string.
    """
    names = [decls] if isinstance(decls, str) else decls
    all_includes = list(includes or []) + list(extra_includes or [])
    for d in names:
        checking(f"whether {d} is declared")
        # AC_CHECK_DECLS defines HAVE_DECL_<NAME> as 1 or 0 (always defined)
        found = check_decl(
            d,
            includes=all_includes,
            define_name="HAVE_DECL_" + d.upper(),
        )
        result(found)


# ---------------------------------------------------------------------------
# Library probes
# ---------------------------------------------------------------------------


def check_lib(
    lib: str,
    func: str,
    extra_cflags: str = "",
    extra_libs: str = "",
    libs_also: list[str] | None = None,
) -> bool:
    """AC_CHECK_LIB — check if *lib* provides *func*. Returns bool.

    *extra_cflags* / *extra_libs* are added to the compile/link invocation.
    *libs_also*: additional library names to try alongside *lib*.
    Does NOT automatically add -l<lib> to LDFLAGS; the caller must do that
    if the result is True.
    """
    src = f"char {func}();\nint main(void) {{ return {func}(); }}\n"
    also = " ".join(f"-l{lib_name}" for lib_name in (libs_also or []))
    flags = (
        extra_cflags.split() + [f"-l{lib}"] + extra_libs.split() + also.split()
    )
    found = _link_test(src, extra_flags=flags)
    # Set ac_cv_lib_<lib>_<func> cache variable (autoconf convention)
    cache_key = f"ac_cv_lib_{lib}_{func}"
    cache[cache_key] = found
    setattr(vars, cache_key, found)
    return found


def search_libs(
    func: str, libs: list[str], other_libs: str = "", required: bool = True
) -> str:
    """AC_SEARCH_LIBS — find first library that provides *func*.

    Returns 'none required' if found in default libs, '-l<lib>' if found in
    one of *libs*.  When *required* is True (default) and the function is not
    found anywhere, raises SystemExit.  When *required* is False, returns ""
    (falsy) instead.
    Also adds the found library to vars.LIBS (like AC_SEARCH_LIBS does).
    The return value is always truthy on success.
    """
    # First check if already available without extra -l
    src = f"char {func}();\nint main(void) {{ return {func}(); }}\n"
    extra = other_libs.split() if other_libs else []
    if _link_test(src, extra_flags=extra):
        return "none required"
    for lib in libs:
        if _link_test(src, extra_flags=extra + [f"-l{lib}"]):
            found_flag = f"-l{lib}"
            # Add to vars.LIBS like AC_SEARCH_LIBS does
            cur_libs = getattr(vars, "LIBS", "")
            setattr(vars, "LIBS", f"{found_flag} {cur_libs}".strip())
            return found_flag
    if required:
        error(f"required function '{func}' not found in any of: {libs}")
    return ""


def replace_funcs(funcs: list[str]) -> None:
    """AC_REPLACE_FUNCS — check each func; note which need replacement.

    Records missing functions in pyconf.substs['LIBOBJS'] (space-separated).
    """
    missing = substs.get("LIBOBJS", "").split()
    for func in funcs:
        checking(f"for {func}")
        found = check_func(func)
        result(found)
        if not found:
            # Match autoconf's final LIBOBJS fixup: prepend ${LIBOBJDIR}
            # and insert $U before the extension so Makefile.pre.in can
            # locate the replacement source under Python/.
            missing.append(f"${{LIBOBJDIR}}{func}$U.o")
    substs["LIBOBJS"] = " ".join(missing)


def pkg_check_modules(pkg_var: str, module_spec: str) -> Any:
    """PKG_CHECK_MODULES — query pkg-config for *module_spec*.

    Returns a SimpleNamespace with .cflags and .libs on success, None if
    pkg-config is unavailable or the module is not found.
    Sets <PKG_VAR>_CFLAGS and <PKG_VAR>_LIBS in pyconf.substs.

    Like autoconf's PKG_CHECK_MODULES, if the CFLAGS/LIBS variables are
    already set (e.g. by check_emscripten_port or the user), the pkg-config
    query is skipped and the existing values are used.
    """
    cflags_name = f"{pkg_var}_CFLAGS"
    libs_name = f"{pkg_var}_LIBS"
    # If variables are already set (by user, environment, or emscripten port),
    # honour them without querying pkg-config — matches autoconf behaviour.
    existing_cflags = getattr(vars, cflags_name, "")
    existing_libs = getattr(vars, libs_name, "")
    if existing_cflags or existing_libs:
        ns = types.SimpleNamespace(cflags=existing_cflags, libs=existing_libs)
        return ns
    pkg_config = shutil.which("pkg-config")
    if not pkg_config:
        return None
    try:
        cflags = subprocess.check_output(
            [pkg_config, "--cflags", module_spec],
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
        libs = subprocess.check_output(
            [pkg_config, "--libs", module_spec],
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except subprocess.CalledProcessError:
        return None
    substs[cflags_name] = cflags
    substs[libs_name] = libs
    setattr(vars, cflags_name, cflags)
    setattr(vars, libs_name, libs)
    ns = types.SimpleNamespace(cflags=cflags, libs=libs)
    return ns


def check_emscripten_port(pkg_var: str, emport_args: str) -> None:
    """Check for an Emscripten port (e.g. USE_ZLIB).

    Sets ``{pkg_var}_CFLAGS`` and ``{pkg_var}_LIBS`` to *emport_args*
    when building with emcc and the user hasn't already provided values.
    Matches autoconf's PY_CHECK_EMSCRIPTEN_PORT macro.
    Only active when ac_sys_system is Emscripten; otherwise a no-op.
    """
    if vars.ac_sys_system != "Emscripten":
        return
    flag = (
        f"-s{emport_args}" if not emport_args.startswith("-") else emport_args
    )
    cflags_name = f"{pkg_var}_CFLAGS"
    libs_name = f"{pkg_var}_LIBS"
    # Only set if user hasn't already provided values (Vars.__getattr__
    # returns "" for unset attributes, matching shell empty-variable checks).
    if not getattr(vars, cflags_name) and not getattr(vars, libs_name):
        setattr(vars, cflags_name, flag)
        setattr(vars, libs_name, flag)


# ---------------------------------------------------------------------------
# Stdlib module registration
# ---------------------------------------------------------------------------

# Internal registry: name → dict of module metadata (insertion-ordered)
_stdlib_modules: dict[str, dict[str, Any]] = {}
# Pending na flags from stdlib_module_set_na() for not-yet-registered modules.
# These are applied when stdlib_module() is called, preserving insertion order.
_stdlib_modules_pending_na: set[str] = set()


def stdlib_module(
    name: str, supported: bool = True, enabled: bool = True, **kwargs: Any
) -> None:
    """PY_STDLIB_MOD — register a stdlib extension module with full options.

    kwargs may include: sources, headers, cflags, ldflags, depends, etc.
    Stores metadata in pyconf._stdlib_modules[name].
    Preserves any 'na' flag set by a prior stdlib_module_set_na() call.
    """
    # Check both already-registered entries and the pending-na set
    prev_na = (
        _stdlib_modules.get(name, {}).get("na", False)
        or name in _stdlib_modules_pending_na
    )
    _stdlib_modules_pending_na.discard(name)
    _stdlib_modules[name] = {
        "supported": supported,
        "enabled": enabled,
        **kwargs,
    }
    if prev_na:
        _stdlib_modules[name]["na"] = True
        _stdlib_modules[name]["supported"] = False


_UNSET = object()  # sentinel for "argument not provided"


def stdlib_module_simple(
    name: str, cflags: Any = _UNSET, ldflags: Any = _UNSET
) -> None:
    """PY_STDLIB_MOD_SIMPLE — register an always-available stdlib module.

    Only records cflags/ldflags when they are explicitly provided (matching
    autoconf m4_ifblank behaviour: args not provided → no CFLAGS/LDFLAGS line).
    """
    kwargs: dict[str, Any] = {}
    if cflags is not _UNSET:
        kwargs["cflags"] = cflags
    if ldflags is not _UNSET:
        kwargs["ldflags"] = ldflags
    stdlib_module(name, supported=True, enabled=True, **kwargs)


def stdlib_module_set_na(names: list[str]) -> None:
    """Mark stdlib modules as not available (N/A) on this platform.

    For already-registered modules: sets supported=False and na=True in place
    (preserving insertion order).
    For not-yet-registered modules: stores the flag in _stdlib_modules_pending_na
    so that the insertion order is determined by the later stdlib_module() call,
    matching autoconf's behaviour where module ordering follows PY_STDLIB_MOD.
    """
    for name in names:
        if name in _stdlib_modules:
            _stdlib_modules[name]["supported"] = False
            _stdlib_modules[name]["na"] = True
        else:
            _stdlib_modules_pending_na.add(name)


# ---------------------------------------------------------------------------
# Shell / misc
# ---------------------------------------------------------------------------


def shell(
    code: str, exports: list[str] | None = None
) -> types.SimpleNamespace:
    """Run a shell fragment; capture listed variable names into pyconf.vars.

    *exports*: list of shell variable names whose values should be captured
    into pyconf.vars after the fragment runs.

    Returns a SimpleNamespace with an attribute for each exported name set to
    its captured value (empty string if the variable was unset or the shell
    command failed).
    """
    captured: dict[str, str] = {k: "" for k in (exports or [])}
    if exports:
        # Append export commands so we can read the values back
        capture = "\n".join(f"echo {v}=${{{v}}}" for v in exports)
        full_code = f"{code}\n{capture}"
    else:
        full_code = code
    try:
        result_proc = subprocess.run(
            ["sh", "-c", full_code],
            capture_output=True,
            text=True,
            env={**os.environ, **env},
        )
        if exports and result_proc.returncode == 0:
            for line in result_proc.stdout.splitlines():
                if "=" in line:
                    k, _, val = line.partition("=")
                    k = k.strip()
                    if k in exports:
                        captured[k] = val
                        setattr(vars, k, val)
    except (FileNotFoundError, OSError):
        pass
    return types.SimpleNamespace(**captured)


def sizeof(type_: str) -> int:
    """Return previously computed sizeof result for *type_*.

    Looks up pyconf.cache for the value stored by check_sizeof().
    Returns 0 if not yet computed.
    """
    cache_key = "ac_cv_sizeof_" + _type_to_define(type_).lower()
    v = cache.get(cache_key, "0")
    return int(v) if v.isdigit() else 0


def macro(name: str, args: list | None = None) -> None:
    """Passthrough for unknown/unimplemented macros — records the gap.

    Unknown macro calls are stored in pyconf.substs['_unknown_macros'] for
    later audit rather than silently discarded.
    """
    if args is None:
        args = []
    entry = f"{name}({', '.join(str(a) for a in args)})"
    existing = substs.get("_unknown_macros", "")
    substs["_unknown_macros"] = (existing + "\n" + entry).strip()


# ---------------------------------------------------------------------------
# Transpilable wrapper functions
# ---------------------------------------------------------------------------
# These wrap stdlib calls (re, subprocess, pathlib, shutil, os) behind a
# pyconf API so that conf_*.py files don't need those imports directly.
# The transpiler maps each to a corresponding shell function or built-in.


def is_digit(s: str) -> bool:
    """Return True if *s* is non-empty and all characters are ASCII digits."""
    return s.isdigit()


def fnmatch(string: str, pattern: str) -> bool:
    """Glob-style pattern match (shell: case "$string" in pattern) ...)."""
    return _fnmatch.fnmatch(string, pattern)


def fnmatch_any(string: str, patterns: list[str]) -> bool:
    """Return True if *string* matches any of the glob *patterns*."""
    return any(_fnmatch.fnmatch(string, p) for p in patterns)


def cmd(args: list[str]) -> bool:
    """Run command and return True if it exits successfully."""
    return subprocess.run(args, capture_output=True).returncode == 0


def cmd_output(args: list[str]) -> str:
    """Run command and return stripped stdout.  Raises on failure."""
    return subprocess.check_output(args, text=True).strip()


def cmd_status(args: list[str]) -> tuple[int, str]:
    """Run command and return (exit_status, combined stdout+stderr stripped).

    Stderr is merged into stdout (like shell ``2>&1``) to match the AWK
    runtime (pyconf_cmd_status) and autoconf's typical usage pattern.
    """
    r = subprocess.run(
        args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )
    return r.returncode, r.stdout.strip()


def abspath(path: str) -> str:
    """Resolve to absolute path (shell: $(cd "dir" && pwd))."""
    return str(Path(path).resolve())


def path_exists(path: str) -> bool:
    """Check if path exists (shell: [ -e "$path" ])."""
    return os.path.exists(path)


def path_is_dir(path: str) -> bool:
    """Check if path is a directory (shell: [ -d "$path" ])."""
    return os.path.isdir(path)


def path_is_symlink(path: str) -> bool:
    """Check if path is a symbolic link (shell: [ -L "$path" ])."""
    return os.path.islink(path)


def path_join(parts: list[str]) -> str:
    """Join path parts (shell: "$a/$b")."""
    return os.path.join(*parts)


def path_parent(path: str) -> str:
    """Return parent directory of path (shell: dirname "$path")."""
    return os.path.dirname(path)


def rm_f(path: str) -> None:
    """Remove file if it exists (shell: rm -f "$path")."""
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass


def find_prog(name: str) -> str:
    """Find program on PATH (shell: command -v "$name"). Returns "" if not found."""
    return shutil.which(name) or ""


def sed(s: str, pattern: str, repl: str) -> str:
    """Replace regex *pattern* with *repl* in *s* (shell: echo "$s" | sed)."""
    return re.sub(pattern, repl, s)


def mkdir_p(path: str) -> None:
    """Create directory and parents (shell: mkdir -p "$path")."""
    os.makedirs(path, exist_ok=True)


def write_file(path: str, content: str) -> None:
    """Write *content* to *path*, creating or overwriting."""
    with open(path, "w") as f:
        f.write(content)


def rename_file(src: str, dst: str) -> None:
    """Rename/move file (shell: mv "$src" "$dst")."""
    os.rename(src, dst)


def path_is_file(path: str) -> bool:
    """Check if path is a regular file (shell: [ -f "$path" ])."""
    return os.path.isfile(path)


def basename(path: str) -> str:
    """Return base name of path (shell: basename "$path")."""
    return os.path.basename(path)


def getcwd() -> str:
    """Return current working directory (shell: pwd)."""
    return os.getcwd()


def readlink(path: str) -> str:
    """Read symbolic link target (shell: readlink "$path")."""
    return os.readlink(path)


def is_executable(path: str) -> bool:
    """Check if path is executable (shell: [ -x "$path" ])."""
    return os.access(path, os.X_OK)


def rmdir(path: str) -> None:
    """Remove empty directory (shell: rmdir "$path")."""
    os.rmdir(path)


def relpath(path: str, start: str | None = None) -> str:
    """Return relative path (shell: realpath --relative-to)."""
    if start is not None:
        return os.path.relpath(path, start)
    return os.path.relpath(path)


def glob_files(pattern: str) -> list[str]:
    """Glob for files matching pattern (shell: glob expansion)."""
    import glob as _glob

    return sorted(_glob.glob(pattern))


def platform_machine() -> str:
    """Return platform machine architecture (shell: uname -m)."""
    import platform as _platform

    return _platform.machine()


def platform_system() -> str:
    """Return platform system name (shell: uname -s)."""
    import platform as _platform

    return _platform.system()


def read_file(path: str) -> str:
    """Read entire file as string (shell: cat "$path")."""
    with open(path) as f:
        return f.read()


class _SaveEnvContext:
    """Context manager returned by save_env().

    Captures a snapshot of pyconf.substs and pyconf.vars immediately on
    construction.  Also works as a context manager that restores both on exit.
    Supports dict-style read access (snap["KEY"]) for the substs snapshot.
    """

    def __init__(self) -> None:
        self._snapshot = substs.copy()
        self._vars_snapshot = vars.__dict__.copy()

    def __getitem__(self, key: str) -> Any:
        return self._snapshot[key]

    def __iter__(self) -> Iterator[str]:
        return iter(self._snapshot)

    def __enter__(self) -> "_SaveEnvContext":
        return self

    def __exit__(self, *exc: Any) -> bool:
        substs.clear()
        substs.update(self._snapshot)
        vars.__dict__.clear()
        vars.__dict__.update(self._vars_snapshot)
        return False  # don't suppress exceptions


def save_env() -> _SaveEnvContext:
    """Return a snapshot of pyconf.substs, usable as a context manager.

    Usage::

        with pyconf.save_env():
            substs["CFLAGS"] += " -Ifoo"
            ...  # temporary changes here
        # substs restored on exit

    Or as a plain snapshot::

        snap = pyconf.save_env()
        val = snap["CFLAGS"]
        pyconf.restore_env(snap)
    """
    return _SaveEnvContext()


def restore_env(snapshot: "_SaveEnvContext | dict[str, str]") -> None:
    """Restore pyconf.substs (and pyconf.vars) to a previously saved snapshot.

    Accepts either a _SaveEnvContext (from save_env()) or a plain dict.
    When given a plain dict, only substs is restored.
    """
    if isinstance(snapshot, _SaveEnvContext):
        substs.clear()
        substs.update(snapshot._snapshot)
        vars.__dict__.clear()
        vars.__dict__.update(snapshot._vars_snapshot)
    else:
        substs.clear()
        substs.update(snapshot)


# ---------------------------------------------------------------------------
# Allow pyconf.py to be invoked directly by config.status for file regen
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    if "--config-status" in sys.argv:
        # Strip out our flag and pass the rest
        argv = [a for a in sys.argv[1:] if a != "--config-status"]
        _config_status_main(argv)
    else:
        print("usage: pyconf.py --config-status [FILES...]", file=sys.stderr)
        sys.exit(1)
