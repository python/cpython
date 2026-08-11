"""Append module search paths for third-party packages to sys.path.

****************************************************************
* This module is automatically imported during initialization. *
****************************************************************

This will append site-specific paths to the module search path.  On
Unix (including Mac OSX), it starts with sys.prefix and
sys.exec_prefix (if different) and appends
lib/python<version>/site-packages.
On other platforms (such as Windows), it tries each of the
prefixes directly, as well as with lib/site-packages appended.  The
resulting directories, if they exist, are appended to sys.path, and
also inspected for path configuration files.

If a file named "pyvenv.cfg" exists one directory above sys.executable,
sys.prefix and sys.exec_prefix are set to that directory and
it is also checked for site-packages (sys.base_prefix and
sys.base_exec_prefix will always be the "real" prefixes of the Python
installation). If "pyvenv.cfg" (a bootstrap configuration file) contains
the key "include-system-site-packages" set to "true" (case-insensitive),
the system-level prefixes will still also be searched for site-packages;
otherwise they won't.

Two kinds of configuration files are processed in each site-packages
directory:

- <name>.pth files extend sys.path with additional directories (one per
  line).  Lines starting with "import" are deprecated (see PEP 829).

- <name>.start files specify startup entry points using the pkg.mod:callable
  syntax.  These are resolved via pkgutil.resolve_name() and called with no
  arguments.

When called from main(), all .pth path extensions are applied before any
.start entry points are executed, ensuring that paths are available before
startup code runs.

See the documentation for the site module for full details:
https://docs.python.org/3/library/site.html
"""

import sys
import os
import builtins
import _sitebuiltins
import _io as io
import stat
import errno

lazy import locale
lazy import pkgutil
lazy import traceback
lazy import warnings

# Prefixes for site-packages; add additional prefixes like /usr/local here
PREFIXES = [sys.prefix, sys.exec_prefix]
# Enable per user site-packages directory
# set it to False to disable the feature or True to force the feature
ENABLE_USER_SITE = None

# for distutils.commands.install
# These values are initialized by the getuserbase() and getusersitepackages()
# functions, through the main() function when Python starts.
USER_SITE = None
USER_BASE = None


def _trace(message, exc=None):
    if sys.flags.verbose:
        _print_error(message, exc)


def _print_error(message, exc=None):
    """Print an error message to stderr, optionally with a formatted traceback."""
    print(message, file=sys.stderr)
    if exc is not None:
        for record in traceback.format_exception(exc):
            for line in record.splitlines():
                print('  ' + line, file=sys.stderr)


def _warn(*args, **kwargs):
    warnings.warn(*args, **kwargs)


def _warn_future_us(message, remove):
    # Don't call warnings._deprecated() directly because we're lazily importing warnings and don't
    # want to have to trigger an eager import if it's not necessary.  Startup time matters a lot
    # here and warnings isn't cheap!  This inlines the check from
    # warnings._py_warnings._deprecated().
    _version = sys.version_info
    if (_version[:2] > remove) or (_version[:2] == remove and _version[3] != "alpha"):
        warnings._deprecated(message, remove=remove)


def makepath(*paths):
    dir = os.path.join(*paths)
    try:
        dir = os.path.abspath(dir)
    except OSError:
        pass
    return dir, os.path.normcase(dir)


def abs_paths():
    """Set __file__ to an absolute path."""
    for m in set(sys.modules.values()):
        loader_module = None
        try:
            loader_module = m.__loader__.__module__
        except AttributeError:
            try:
                loader_module = m.__spec__.loader.__module__
            except AttributeError:
                pass
        if loader_module not in {'_frozen_importlib', '_frozen_importlib_external'}:
            continue   # don't mess with a PEP 302-supplied __file__
        try:
            m.__file__ = os.path.abspath(m.__file__)
        except (AttributeError, OSError, TypeError):
            pass


def removeduppaths():
    """ Remove duplicate entries from sys.path along with making them
    absolute"""
    # This ensures that the initial path provided by the interpreter contains
    # only absolute pathnames, even if we're running from the build directory.
    L = []
    known_paths = set()
    for dir in sys.path:
        # Filter out duplicate paths (on case-insensitive file systems also
        # if they only differ in case); turn relative paths into absolute
        # paths.
        dir, dircase = makepath(dir)
        if dircase not in known_paths:
            L.append(dir)
            known_paths.add(dircase)
    sys.path[:] = L
    return known_paths


def _init_pathinfo():
    """Return a set containing all existing file system items from sys.path."""
    d = set()
    for item in sys.path:
        try:
            if os.path.exists(item):
                _, itemcase = makepath(item)
                d.add(itemcase)
        except TypeError:
            continue
    return d


# PEP 829 implementation notes.
#
# Startup information (.pth and .start file information) can be processed in
# implicit or explicit batches.  Implicit batches are self-contained
# site.addsitedir() calls: they create a per-call StartupState, populate it
# from the site directory's .pth and .start files, run process() on it, and
# then throw the state away.
#
# main() needs different semantics: it accumulates state across multiple
# StartupState.addsitedir() calls (user-site plus all global site-packages) so
# that every sys.path extension is visible *before* any startup code (.start
# entry points and .pth import lines) runs.  Callers can opt into the same
# behavior by creating a StartupState directly and calling its addsitedir(),
# addusersitepackages(), and addsitepackages() methods, then invoking
# process() once at the end of the batch.
#
# Here's the CRITICAL reentrancy invariant: recursive site.addsitedir() calls
# reached from a .start entry point or an exec'd .pth import line must not
# mutate the StartupState currently being processed.  Reentrant calls reach
# the module-level site.addsitedir() shim, which always builds a fresh
# per-call state.


def _read_pthstart_file(sitedir, name, suffix):
    """Parse a .start or .pth file and return (lines, filename).

    On success, ``lines`` is a (possibly empty) list of the file's lines.
    On failure (file missing, hidden, unreadable, or .start with bad
    encoding), ``lines`` is ``None`` so callers can distinguish a
    successfully-read empty file from one that could not be read.
    """
    filename = os.path.join(sitedir, name)
    _trace(f"Reading startup configuration file: {filename}")

    try:
        st = os.lstat(filename)
    except OSError as exc:
        _trace(f"Cannot stat {filename!r}", exc)
        return None, filename

    if ((getattr(st, 'st_flags', 0) & stat.UF_HIDDEN) or
        (getattr(st, 'st_file_attributes', 0) & stat.FILE_ATTRIBUTE_HIDDEN)):
        _trace(f"Skipping hidden {suffix} file: {filename!r}")
        return None, filename

    _trace(f"Processing {suffix} file: {filename!r}")
    try:
        with io.open_code(filename) as f:
            raw_content = f.read()
    except OSError as exc:
        _trace(f"Cannot read {filename!r}", exc)
        return None, filename

    try:
        # Accept BOM markers in .start and .pth files as we do in source files
        # (Windows PowerShell 5.1 makes it hard to emit UTF-8 files without a BOM).
        content = raw_content.decode("utf-8-sig")
    except UnicodeDecodeError:
        _trace(f"Cannot read {filename!r} as UTF-8.")
        # For .pth files only, and then only until Python 3.20, fall back to
        # locale encoding for backward compatibility.
        _warn_future_us(
            ".pth files decoded to locale encoding as a fallback",
            remove=(3, 20)
        )
        if suffix == ".pth":
            content = raw_content.decode(locale.getencoding())
            _trace(f"Using fallback encoding {locale.getencoding()!r}")
        else:
            return None, filename

    return content.splitlines(), filename


class StartupState:
    """Per-batch accumulator for .pth and .start file processing.

    A StartupState collects sys.path extensions, deprecated .pth import lines,
    and .start entry points read from one or more site-packages directories.
    Calling process() applies them in PEP 829 order: paths are added to
    sys.path first, then import lines from .pth files (skipping any with a
    matching .start), then entry points from .start files.

    State lives entirely on the instance; there is no module-level pending
    state.  This is what makes the module reentrancy-safe: a site.addsitedir()
    call reached recursively from an exec'd import line or a .start entry
    point operates on a different StartupState than the one being processed by
    the outer call.

    The internal data is intentionally private.  The lower-level write
    methods (_record_sitedir(), _read_pth_file(), _read_start_file()) are
    private to the site module; the public surface is addsitedir(),
    addusersitepackages(), addsitepackages(), and process().
    """
    __slots__ = (
        '_known_paths',
        '_processed_sitedirs',
        '_path_entries',
        '_importexecs',
        '_entrypoints',
    )

    def __init__(self, known_paths=None):
        """Create an independent startup state.

        *known_paths* is a set of case-normalized paths already present
        on sys.path, used to avoid duplicate path entries.  When None
        (the default), it is initialized from the current sys.path.

        A caller-supplied set is stored by reference and mutated in place
        as new paths are recorded; pass a fresh set per StartupState if
        isolation across instances is required.
        """
        self._known_paths = (
            _init_pathinfo()
            if known_paths is None
            else known_paths)
        self._processed_sitedirs = set()
        # The sys.path append ledger.  This is a list of 2-tuples of the form
        # (pthfile, path) where `pthfile` is the .pth file which is extending
        # the path, and `path` is the directory to add to sys.path.  Note that
        # to preserve the interleaving semantics (i.e. .pth file paths are
        # added after the sitedir in which the .pth file is found), `path`
        # could be a sitedir, in which case `pthfile` will always be None.
        self._path_entries = []
        # Both dicts map "<full path to .pth or .start file>" -> list
        # of items collected from that file.  Mapping by filename lets us
        # cross-reference a .pth and its matching .start (PEP 829 import
        # suppression rule) and lets _print_error report the source file
        # when an entry fails.
        self._importexecs = {}
        self._entrypoints = {}

    def addsitedir(self, sitedir):
        """Add a site directory and accumulate its .pth and .start startup data.

        Read the .pth and .start files in *sitedir* and record their
        sys.path extensions, deprecated .pth import lines, and .start entry
        points on this state.  The recorded data is not applied until
        process() is called.

        Typically used to batch multiple site directories before a single
        process() call, so that every sys.path extension is visible before
        any startup code runs.  Reentrant calls reached from a .start entry
        point or an exec'd .pth import line must not mutate the state
        currently being processed; for those cases, use site.addsitedir()
        instead, which always creates a fresh per-call state.
        """
        self._addsitedir(sitedir, process_known_sitedirs=True)

    def addusersitepackages(self):
        """Add the per-user site-packages directory, if enabled.

        The user site directory is added only when user site-packages are
        enabled and the directory exists.  Its startup data is accumulated
        for later processing by process().
        """
        _trace("Processing user site-packages")
        user_site = getusersitepackages()
        if ENABLE_USER_SITE and os.path.isdir(user_site):
            self.addsitedir(user_site)

    def addsitepackages(self, prefixes=None):
        """Add global site-packages directories, if they exist.

        Site-packages directories are computed from *prefixes*, or from the
        global PREFIXES when *prefixes* is None.  Each directory's startup
        data is accumulated for later processing by process().
        """
        _trace("Processing global site-packages")
        for sitedir in getsitepackages(prefixes):
            if os.path.isdir(sitedir):
                self.addsitedir(sitedir)

    def _addsitedir(self, sitedir, *, process_known_sitedirs):
        """Internal addsitedir() implementation with full dedup control.

        The public addsitedir() always uses process_known_sitedirs=True
        (gh-149819 semantics).  The module-level legacy known_paths shim
        uses process_known_sitedirs=False to preserve 3.14 idempotency
        (gh-75723).
        """
        sitedir = self._record_sitedir(
            sitedir, process_known_sitedirs=process_known_sitedirs)
        if sitedir is None:
            return
        try:
            names = os.listdir(sitedir)
        except OSError:
            return

        # The following phases are defined by PEP 829.
        # Phases 1-3: Read .pth files, accumulating paths and import lines.
        pth_names = sorted(
            name for name in names
            if name.endswith(".pth") and not name.startswith(".")
        )
        for name in pth_names:
            self._read_pth_file(sitedir, name)

        # Phases 6-7: Discover .start files and accumulate their entry points.
        # Import lines from .pth files with a matching .start file are
        # discarded at flush time by _exec_imports().
        start_names = sorted(
            name for name in names
            if name.endswith(".start") and not name.startswith(".")
        )
        for name in start_names:
            self._read_start_file(sitedir, name)

    def _record_sitedir(self, sitedir, *, process_known_sitedirs=True):
        sitedir, sitedircase = makepath(sitedir)
        # Have we already processed this sitedir?
        if sitedircase in self._processed_sitedirs:
            return None
        # In legacy known_paths mode, a known sitedir means its startup files
        # were already processed by an earlier addsitedir() call, so skip it
        # to preserve idempotency (gh-75723).  In explicit StartupState mode,
        # known_paths only tracks sys.path entries; a sitedir may already be
        # on sys.path (for example from $PYTHONPATH, gh-149819) but still need
        # its .pth and .start files processed once.  The separate
        # _processed_sitedirs set is what lets explicit batches distinguish
        # "already on sys.path" from "startup files already read".
        if not process_known_sitedirs and sitedircase in self._known_paths:
            return None
        # Record that we've processed this sitedir.
        self._processed_sitedirs.add(sitedircase)
        if sitedircase not in self._known_paths:
            self._known_paths.add(sitedircase)
            # Add the sitedir to the sys.path extension ledger.  There is no
            # .pth file to record.
            self._path_entries.append((None, sitedir))
        return sitedir

    def _read_pth_file(self, sitedir, name):
        """Parse a .pth file, accumulating sys.path extensions and import lines.

        Errors on individual lines do not abort processing of the rest of
        the file (PEP 829).  Per-batch deduplication is done against
        self._known_paths: any path already in it is skipped, and newly
        accepted paths are added to it so that subsequent .pth files in
        the same batch don't add them more than once.
        """
        lines, filename = _read_pthstart_file(sitedir, name, ".pth")
        if lines is None:
            return

        for n, line in enumerate(lines, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            # In Python 3.18 and 3.19, `import` lines are silently
            # ignored.  In Python 3.20 and beyond, issue a warning when
            # `import` lines in .pth files are detected.
            if line.startswith(("import ", "import\t")):
                _warn_future_us(
                    "import lines in .pth files are silently ignored",
                    remove=(3, 18),
                )
                _warn_future_us(
                    "import lines in .pth files are noisily ignored",
                    remove=(3, 20),
                )
                self._importexecs.setdefault(filename, []).append(line)
                continue

            try:
                dir_, dircase = makepath(sitedir, line)
            except Exception as exc:
                _trace(f"Error in {filename!r}, line {n:d}: {line!r}", exc)
                continue

            # PEP 829 dedup: skip paths already seen in this batch.
            if dircase in self._known_paths:
                _trace(
                    f"In {filename!r}, line {n:d}: "
                    f"skipping duplicate sys.path entry: {dir_}"
                )
            else:
                # Add this directory to the sys.path extension ledger, while
                # also recording the .pth file it was found in.
                self._path_entries.append((filename, dir_))
                self._known_paths.add(dircase)

    def _read_start_file(self, sitedir, name):
        """Parse a .start file for a list of entry point strings."""
        lines, filename = _read_pthstart_file(sitedir, name, ".start")
        if lines is None:
            return

        # PEP 829: the *presence* of a matching .start file disables `import`
        # line processing in the matched .pth file, regardless of whether this
        # .start file contains any entry points.  Register the filename as a
        # key now so an empty (or comment-only) .start file still suppresses.
        entrypoints = self._entrypoints.setdefault(filename, [])

        for n, line in enumerate(lines, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            # Syntax validation is deferred to entry point execution
            # time, where pkgutil.resolve_name(strict=True) enforces the
            # pkg.mod:callable form.
            entrypoints.append(line)

    def process(self):
        """Apply accumulated state in PEP 829 order.

        Phase order matters: all .pth path extensions are applied to
        sys.path *before* any import line or .start entry point runs, so
        that an entry point may live in a module reachable only via a
        .pth-extended path.
        """
        self._extend_syspath()
        self._exec_imports()
        self._execute_start_entrypoints()

    def _extend_syspath(self):
        # Duplicate path-extension specifications have already been filtered
        # out upstream across .pth files within this batch (via known_paths),
        # and ledger entries are already abspath/normpath'd.  .pth-derived
        # entries (filename is not None) are existence-checked and skipped
        # with an error if missing.  Sitedir entries (filename is None) are
        # appended unconditionally: legacy addsitedir() added the sitedir to
        # sys.path before attempting to list it, so an unreadable or
        # non-existent sitedir still landed on sys.path.  Deferring the
        # append to here preserves that contract.
        for filename, dir_ in self._path_entries:
            # As a backstop, known_paths may not have been seeded from sys.path
            # (callers can pass an empty set), and multiple StartupState
            # instances against the same sys.path don't share state, so always
            # do a final anti-duplication check.
            if dir_ in sys.path:
                continue
            if filename is None or os.path.exists(dir_):
                if filename is not None:
                    _trace(f"Extending sys.path with {dir_} from {filename}")
                sys.path.append(dir_)
            else:
                _print_error(
                    f"In {filename}: {dir_} does not exist; "
                    f"skipping sys.path append"
                )

    def _exec_imports(self):
        # For each `import` line we've seen in a .pth file, exec() it in
        # order, unless the .pth has a matching .start file in this same
        # batch.  In that case, PEP 829 says the import lines are
        # suppressed in favor of the .start's entry points.
        for filename, imports in self._importexecs.items():
            # Inject 'sitedir' local variable in the current frame for
            # compatibility with Python 3.14. Especially, "-nspkg.pth" files
            # generated by setuptools use: sys._getframe(1).f_locals['sitedir'].
            sitedir = os.path.dirname(filename)

            # Given "/path/to/foo.pth", check whether "/path/to/foo.start" was
            # registered in this same batch.
            name, dot, pth = filename.rpartition(".")
            assert dot == "." and pth == "pth", (
                f"Bad startup filename: {filename}"
            )
            if f"{name}.start" in self._entrypoints:
                _trace(
                    f"import lines in {filename} are suppressed "
                    f"due to matching {name}.start file."
                )
                continue

            _trace(
                f"import lines in {filename} are deprecated, "
                f"use entry points in a {name}.start file instead."
            )
            for line in imports:
                try:
                    _trace(f"Exec'ing from {filename}: {line}")
                    exec(line)
                except Exception as exc:
                    _print_error(
                        f"Error in import line from {filename}: {line}",
                        exc,
                    )

    def _execute_start_entrypoints(self):
        # Resolve each entry point string to a callable via
        # pkgutil.resolve_name(strict=True), which both validates the
        # required pkg.mod:callable form and performs the import in one
        # step, then call it with no arguments.
        for filename, entrypoints in self._entrypoints.items():
            for entrypoint in entrypoints:
                try:
                    _trace(
                        f"Executing entry point: {entrypoint} from {filename}"
                    )
                    callable_ = pkgutil.resolve_name(entrypoint, strict=True)
                except ValueError as exc:
                    _print_error(
                        f"Invalid entry point syntax in {filename}: "
                        f"{entrypoint!r}",
                        exc,
                    )
                except Exception as exc:
                    _print_error(
                        f"Error resolving entry point {entrypoint} "
                        f"from {filename}",
                        exc,
                    )
                else:
                    try:
                        callable_()
                    except Exception as exc:
                        _print_error(
                            f"Error in entry point {entrypoint} from {filename}",
                            exc,
                        )


def addpackage(sitedir, name, known_paths):
    """Process a .pth file within the site-packages directory."""
    if known_paths is None:
        known_paths = _init_pathinfo()
        reset = True
    else:
        reset = False

    state = StartupState(known_paths)
    state._read_pth_file(sitedir, name)
    state.process()

    return None if reset else known_paths


def addsitedir(sitedir, known_paths=None):
    """Add a site directory and process its startup files.

    For batched processing across multiple site directories, build a
    StartupState explicitly and call StartupState.addsitedir() on it; that
    defers .pth/.start processing until a single StartupState.process() call.
    """
    _trace(f"Adding directory: {sitedir!r}")
    if known_paths is None:
        state = StartupState(_init_pathinfo())
        state.addsitedir(sitedir)
    else:
        # Preserve gh-75723 idempotency for legacy known_paths mode: a
        # sitedir already present in known_paths is skipped, not reprocessed.
        state = StartupState(known_paths)
        state._addsitedir(sitedir, process_known_sitedirs=False)
    state.process()
    return known_paths


def check_enableusersite():
    """Check if user site directory is safe for inclusion

    The function tests for the command line flag (including environment var),
    process uid/gid equal to effective uid/gid.

    None: Disabled for security reasons
    False: Disabled by user (command line option)
    True: Safe and enabled
    """
    if sys.flags.no_user_site:
        return False

    if hasattr(os, "getuid") and hasattr(os, "geteuid"):
        # check process uid == effective uid
        if os.geteuid() != os.getuid():
            return None
    if hasattr(os, "getgid") and hasattr(os, "getegid"):
        # check process gid == effective gid
        if os.getegid() != os.getgid():
            return None

    return True


# NOTE: sysconfig and it's dependencies are relatively large but site module
# needs very limited part of them.
# To speedup startup time, we have copy of them.
#
# See https://bugs.python.org/issue29585

# Copy of sysconfig._get_implementation()
def _get_implementation():
    return 'Python'

# Copy of sysconfig._getuserbase()
def _getuserbase():
    env_base = os.environ.get("PYTHONUSERBASE", None)
    if env_base:
        return env_base

    # Emscripten, iOS, tvOS, VxWorks, WASI, and watchOS have no home directories
    if sys.platform in {"emscripten", "ios", "tvos", "vxworks", "wasi", "watchos"}:
        return None

    def joinuser(*args):
        return os.path.expanduser(os.path.join(*args))

    if os.name == "nt":
        base = os.environ.get("APPDATA") or "~"
        return joinuser(base, _get_implementation())

    if sys.platform == "darwin" and sys._framework:
        return joinuser("~", "Library", sys._framework,
                        "%d.%d" % sys.version_info[:2])

    return joinuser("~", ".local")


# Same to sysconfig.get_path('purelib', os.name+'_user')
def _get_path(userbase):
    version = sys.version_info
    if hasattr(sys, 'abiflags') and 't' in sys.abiflags:
        abi_thread = 't'
    else:
        abi_thread = ''

    implementation = _get_implementation()
    implementation_lower = implementation.lower()
    if os.name == 'nt':
        ver_nodot = sys.winver.replace('.', '')
        return f'{userbase}\\{implementation}{ver_nodot}\\site-packages'

    if sys.platform == 'darwin' and sys._framework:
        return f'{userbase}/lib/{implementation_lower}/site-packages'

    return f'{userbase}/lib/{implementation_lower}{version[0]}.{version[1]}{abi_thread}/site-packages'


def getuserbase():
    """Returns the `user base` directory path.

    The `user base` directory can be used to store data. If the global
    variable ``USER_BASE`` is not initialized yet, this function will also set
    it.
    """
    global USER_BASE
    if USER_BASE is None:
        USER_BASE = _getuserbase()
    return USER_BASE


def getusersitepackages():
    """Returns the user-specific site-packages directory path.

    If the global variable ``USER_SITE`` is not initialized yet, this
    function will also set it.
    """
    global USER_SITE, ENABLE_USER_SITE
    userbase = getuserbase() # this will also set USER_BASE

    if USER_SITE is None:
        if userbase is None:
            ENABLE_USER_SITE = False # disable user site and return None
        else:
            USER_SITE = _get_path(userbase)

    return USER_SITE


def addusersitepackages(known_paths):
    """Add the per-user site-packages directory, if enabled.

    The user site directory is added only when user site-packages are enabled
    and the directory exists.  Return *known_paths*, updated with any paths
    added by addsitedir().
    """
    state = StartupState(known_paths)
    state.addusersitepackages()
    state.process()
    return known_paths


def getsitepackages(prefixes=None):
    """Returns a list containing all global site-packages directories.

    For each directory present in ``prefixes`` (or the global ``PREFIXES``),
    this function will find its `site-packages` subdirectory depending on the
    system environment, and will return a list of full paths.
    """
    sitepackages = []
    seen = set()

    if prefixes is None:
        prefixes = PREFIXES

    for prefix in prefixes:
        if not prefix or prefix in seen:
            continue
        seen.add(prefix)

        implementation = _get_implementation().lower()
        ver = sys.version_info
        if hasattr(sys, 'abiflags') and 't' in sys.abiflags:
            abi_thread = 't'
        else:
            abi_thread = ''
        if os.sep == '/':
            libdirs = [sys.platlibdir]
            if sys.platlibdir != "lib":
                libdirs.append("lib")

            for libdir in libdirs:
                path = os.path.join(prefix, libdir,
                                    f"{implementation}{ver[0]}.{ver[1]}{abi_thread}",
                                    "site-packages")
                sitepackages.append(path)
        else:
            sitepackages.append(prefix)
            sitepackages.append(os.path.join(prefix, "Lib", "site-packages"))
    return sitepackages


def addsitepackages(known_paths, prefixes=None):
    """Add global site-packages directories, if they exist.

    Site-packages directories are computed from *prefixes*, or from the global
    prefixes when *prefixes* is None.  Return *known_paths*, updated with any
    paths added by addsitedir().
    """
    state = StartupState(known_paths)
    state.addsitepackages(prefixes)
    state.process()
    return known_paths


def setquit():
    """Define new builtins 'quit' and 'exit'.

    These are objects which make the interpreter exit when called.
    The repr of each object contains a hint at how it works.

    """
    if os.sep == '\\':
        eof = 'Ctrl-Z plus Return'
    else:
        eof = 'Ctrl-D (i.e. EOF)'

    builtins.quit = _sitebuiltins.Quitter('quit', eof)
    builtins.exit = _sitebuiltins.Quitter('exit', eof)


def setcopyright():
    """Set 'copyright' and 'credits' in builtins"""
    builtins.copyright = _sitebuiltins._Printer("copyright", sys.copyright)
    builtins.credits = _sitebuiltins._Printer("credits", """\
Thanks to CWI, CNRI, BeOpen, Zope Corporation, the Python Software
Foundation, and a cast of thousands for supporting Python
development.  See www.python.org for more information.""")
    files, dirs = [], []
    # Not all modules are required to have a __file__ attribute.  See
    # PEP 420 for more details.
    here = getattr(sys, '_stdlib_dir', None)
    if not here and hasattr(os, '__file__'):
        here = os.path.dirname(os.__file__)
    if here:
        files.extend(["LICENSE.txt", "LICENSE"])
        dirs.extend([os.path.join(here, os.pardir), here, os.curdir])
    builtins.license = _sitebuiltins._Printer(
        "license",
        "See https://www.python.org/psf/license/",
        files, dirs)


def sethelper():
    builtins.help = _sitebuiltins._Helper()


def gethistoryfile():
    """Check if the PYTHON_HISTORY environment variable is set and define
    it as the .python_history file.  If PYTHON_HISTORY is not set, use the
    default .python_history file.
    """
    if not sys.flags.ignore_environment:
        history = os.environ.get("PYTHON_HISTORY")
        if history:
            return history
    return os.path.join(os.path.expanduser('~'),
        '.python_history')


def enablerlcompleter():
    """Enable default readline configuration on interactive prompts, by
    registering a sys.__interactivehook__.
    """
    sys.__interactivehook__ = register_readline


def register_readline():
    """Configure readline completion on interactive prompts.

    If the readline module can be imported, the hook will set the Tab key
    as completion key and register ~/.python_history as history file.
    This can be overridden in the sitecustomize or usercustomize module,
    or in a PYTHONSTARTUP file.
    """
    if not sys.flags.ignore_environment:
        PYTHON_BASIC_REPL = os.getenv("PYTHON_BASIC_REPL")
    else:
        PYTHON_BASIC_REPL = False

    import atexit

    try:
        try:
            import readline
        except ImportError:
            readline = None
        else:
            import rlcompleter  # noqa: F401
    except ImportError:
        return

    try:
        if PYTHON_BASIC_REPL:
            CAN_USE_PYREPL = False
        else:
            original_path = sys.path
            sys.path = [p for p in original_path if p != '']
            try:
                import _pyrepl.readline
                if os.name == "nt":
                    import _pyrepl.windows_console
                    console_errors = (_pyrepl.windows_console._error,)
                else:
                    import _pyrepl.unix_console
                    console_errors = _pyrepl.unix_console._error
                from _pyrepl.main import CAN_USE_PYREPL
            except ModuleNotFoundError:
                CAN_USE_PYREPL = False
            finally:
                sys.path = original_path
    except ImportError:
        return

    if readline is not None:
        # Reading the initialization (config) file may not be enough to set a
        # completion key, so we set one first and then read the file.
        if readline.backend == 'editline':
            readline.parse_and_bind('bind ^I rl_complete')
        else:
            readline.parse_and_bind('tab: complete')

        try:
            readline.read_init_file()
        except OSError:
            # An OSError here could have many causes, but the most likely one
            # is that there's no .inputrc file (or .editrc file in the case of
            # Mac OS X + libedit) in the expected location.  In that case, we
            # want to ignore the exception.
            pass

    if readline is None or readline.get_current_history_length() == 0:
        # If no history was loaded, default to .python_history,
        # or PYTHON_HISTORY.
        # The guard is necessary to avoid doubling history size at
        # each interpreter exit when readline was already configured
        # through a PYTHONSTARTUP hook, see:
        # http://bugs.python.org/issue5845#msg198636
        history = gethistoryfile()

        if CAN_USE_PYREPL:
            readline_module = _pyrepl.readline
            exceptions = (OSError, *console_errors)
        else:
            if readline is None:
                return
            readline_module = readline
            exceptions = OSError

        try:
            readline_module.read_history_file(history)
        except exceptions:
            pass

        def write_history():
            try:
                readline_module.write_history_file(history)
            except FileNotFoundError, PermissionError:
                # home directory does not exist or is not writable
                # https://bugs.python.org/issue19891
                pass
            except OSError:
                if errno.EROFS:
                    pass  # gh-128066: read-only file system
                else:
                    raise

        atexit.register(write_history)


def venv(known_paths):
    """Process pyvenv.cfg and add the venv site-packages, if applicable."""
    state = StartupState(known_paths)
    _venv(state)
    state.process()
    return known_paths


def _venv(state):
    """State-driven implementation of venv(); used by main() for batching."""
    global PREFIXES, ENABLE_USER_SITE

    env = os.environ
    if sys.platform == 'darwin' and '__PYVENV_LAUNCHER__' in env:
        executable = sys._base_executable = os.environ['__PYVENV_LAUNCHER__']
    else:
        executable = sys.executable
    exe_dir = os.path.dirname(os.path.abspath(executable))
    site_prefix = os.path.dirname(exe_dir)
    sys._home = None
    conf_basename = 'pyvenv.cfg'
    candidate_conf = next(
        (
            conffile for conffile in (
                os.path.join(exe_dir, conf_basename),
                os.path.join(site_prefix, conf_basename)
            )
            if os.path.isfile(conffile)
        ),
        None
    )

    if candidate_conf:
        virtual_conf = candidate_conf
        system_site = "true"
        version, version_info = None, None
        # Issue 25185: Use UTF-8, as that's what the venv module uses when
        # writing the file.
        with open(virtual_conf, encoding='utf-8') as f:
            for line in f:
                if '=' in line:
                    key, _, value = line.partition('=')
                    key = key.strip().lower()
                    value = value.strip()
                    if key == 'include-system-site-packages':
                        system_site = value.lower()
                    elif key == 'home':
                        sys._home = value
                    elif key == 'version':
                        version = value
                    elif key == 'version_info':
                        version_info = value

        for field_name, field_value in [
            ('version',version), ('version_info',version_info)
        ]:
            if field_value is not None:
                try:
                    major, minor = map(int, field_value.split(".")[:2])
                except (ValueError, AttributeError):
                    _warn(
                        f"Malformed {field_name} string in pyvenv.cfg: {field_value!r}",
                        RuntimeWarning,
                    )
                else:
                    if (
                        major == sys.version_info.major
                        and minor != sys.version_info.minor
                    ):
                        _warn(
                            f"This virtual environment was created for Python {major}.{minor}, "
                            f"but the current interpreter is Python "
                            f"{sys.version_info.major}.{sys.version_info.minor}. "
                            "Consider running `python -m venv --upgrade` to update the environment.",
                            RuntimeWarning,
                        )
                        break

        if sys.prefix != site_prefix:
            _warn(
                f'Unexpected value in sys.prefix, expected {site_prefix}, got {sys.prefix}',
                RuntimeWarning)
        if sys.exec_prefix != site_prefix:
            _warn(
                f'Unexpected value in sys.exec_prefix, expected {site_prefix}, got {sys.exec_prefix}',
                RuntimeWarning)

        # Doing this here ensures venv takes precedence over user-site.
        state.addsitepackages([sys.prefix])

        if system_site == "true":
            PREFIXES += [sys.base_prefix, sys.base_exec_prefix]
        else:
            ENABLE_USER_SITE = False


def execsitecustomize():
    """Run custom site specific code, if available."""
    try:
        try:
            import sitecustomize  # noqa: F401
        except ImportError as exc:
            if exc.name == 'sitecustomize':
                pass
            else:
                raise
    except Exception as err:
        if sys.flags.verbose:
            sys.excepthook(*sys.exc_info())
        else:
            sys.stderr.write(
                "Error in sitecustomize; set PYTHONVERBOSE for traceback:\n"
                "%s: %s\n" %
                (err.__class__.__name__, err))


def execusercustomize():
    """Run custom user specific code, if available."""
    try:
        try:
            import usercustomize  # noqa: F401
        except ImportError as exc:
            if exc.name == 'usercustomize':
                pass
            else:
                raise
    except Exception as err:
        if sys.flags.verbose:
            sys.excepthook(*sys.exc_info())
        else:
            sys.stderr.write(
                "Error in usercustomize; set PYTHONVERBOSE for traceback:\n"
                "%s: %s\n" %
                (err.__class__.__name__, err))


def main():
    """Add standard site-specific directories to the module search path.

    This function is called automatically when this module is imported,
    unless the python interpreter was started with the -S flag.
    """
    global ENABLE_USER_SITE

    orig_path = sys.path[:]
    removeduppaths()
    if orig_path != sys.path:
        # removeduppaths() might make sys.path absolute.
        # Fix __file__ of already imported modules too.
        abs_paths()

    state = StartupState(set())
    _venv(state)

    if ENABLE_USER_SITE is None:
        ENABLE_USER_SITE = check_enableusersite()

    state.addusersitepackages()
    state.addsitepackages()
    # PEP 829: flush accumulated data from all .pth and .start files.
    # Paths are extended first, then deprecated import lines are exec'd,
    # and finally .start entry points are executed — ensuring sys.path is
    # fully populated before any startup code runs.
    state.process()
    setquit()
    setcopyright()
    sethelper()
    if not sys.flags.isolated:
        enablerlcompleter()
    execsitecustomize()
    if ENABLE_USER_SITE:
        execusercustomize()

# Prevent extending of sys.path when python was started with -S and
# site is imported later.
if not sys.flags.no_site:
    main()

def _script():
    help = """\
    %s [--user-base] [--user-site]

    Without arguments print some useful information
    With arguments print the value of USER_BASE and/or USER_SITE separated
    by '%s'.

    Exit codes with --user-base or --user-site:
      0 - user site directory is enabled
      1 - user site directory is disabled by user
      2 - user site directory is disabled by super user
          or for security reasons
     >2 - unknown error
    """
    args = sys.argv[1:]
    if not args:
        user_base = getuserbase()
        user_site = getusersitepackages()
        print("sys.path = [")
        for dir in sys.path:
            print("    %r," % (dir,))
        print("]")
        def exists(path):
            if path is not None and os.path.isdir(path):
                return "exists"
            else:
                return "doesn't exist"
        print(f"USER_BASE: {user_base!r} ({exists(user_base)})")
        print(f"USER_SITE: {user_site!r} ({exists(user_site)})")
        print(f"ENABLE_USER_SITE: {ENABLE_USER_SITE!r}")
        sys.exit(0)

    buffer = []
    if '--user-base' in args:
        buffer.append(USER_BASE)
    if '--user-site' in args:
        buffer.append(USER_SITE)

    if buffer:
        print(os.pathsep.join(buffer))
        if ENABLE_USER_SITE:
            sys.exit(0)
        elif ENABLE_USER_SITE is False:
            sys.exit(1)
        elif ENABLE_USER_SITE is None:
            sys.exit(2)
        else:
            sys.exit(3)
    else:
        import textwrap
        print(textwrap.dedent(help % (sys.argv[0], os.pathsep)))
        sys.exit(10)

if __name__ == '__main__':
    _script()
