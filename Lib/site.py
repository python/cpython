"""Append module search paths for third-party packages to sys.path.

****************************************************************
* This module is automatically imported during initialization. *
****************************************************************

This will append site-specific paths to the module search path.  On
Unix (including Mac OSX), it starts with sys.prefix and
sys.exec_prefix (if different) and appends
lib/python<version>/site-packages as well as lib/site-python.
On other platforms (such as Windows), it tries each of the
prefixes directly, as well as with lib/site-packages appended.  The
resulting directories, if they exist, are appended to sys.path, and
also inspected for path configuration files.

A path configuration file is a file whose name has the form
<package>.pth; its contents are additional directories (one per line)
to be added to sys.path.  Non-existing directories (or
non-directories) are never added to sys.path; no directory is added to
sys.path more than once.  Blank lines and lines beginning with
'#' are skipped. Lines starting with 'import' are executed.

For example, suppose sys.prefix and sys.exec_prefix are set to
/usr/local and there is a directory /usr/local/lib/python2.5/site-packages
with three subdirectories, foo, bar and spam, and two path
configuration files, foo.pth and bar.pth.  Assume foo.pth contains the
following:

  # foo package configuration
  foo
  bar
  bletch

and bar.pth contains:

  # bar package configuration
  bar

Then the following directories are added to sys.path, in this order:

  /usr/local/lib/python2.5/site-packages/bar
  /usr/local/lib/python2.5/site-packages/foo

Note that bletch is omitted because it doesn't exist; bar precedes foo
because bar.pth comes alphabetically before foo.pth; and spam is
omitted because it is not mentioned in either path configuration file.

After these path manipulations, an attempt is made to import a module
named sitecustomize, which can perform arbitrary additional
site-specific customizations.  If this import fails with an
ImportError exception, it is silently ignored.

"""

import sys
import os
import builtins

# Prefixes for site-packages; add additional prefixes like /usr/local here
PREFIXES = [sys.prefix, sys.exec_prefix]
# Enable per user site-packages directory
# set it to False to disable the feature or True to force the feature
ENABLE_USER_SITE = None
# for distutils.commands.install
USER_SITE = None
USER_BASE = None


def makepath(*paths):
    dir = os.path.abspath(os.path.join(*paths))
    return dir, os.path.normcase(dir)


def abs__file__():
    """Set all module' __file__ attribute to an absolute path"""
    for m in set(sys.modules.values()):
        if hasattr(m, '__loader__'):
            continue   # don't mess with a PEP 302-supplied __file__
        try:
            m.__file__ = os.path.abspath(m.__file__)
        except AttributeError:
            continue


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
        if not dircase in known_paths:
            L.append(dir)
            known_paths.add(dircase)
    sys.path[:] = L
    return known_paths

# XXX This should not be part of site.py, since it is needed even when
# using the -S option for Python.  See http://www.python.org/sf/586680
def addbuilddir():
    """Append ./build/lib.<platform> in case we're running in the build dir
    (especially for Guido :-)"""
    from distutils.util import get_platform
    s = "build/lib.%s-%.3s" % (get_platform(), sys.version)
    if hasattr(sys, 'gettotalrefcount'):
        s += '-pydebug'
    s = os.path.join(os.path.dirname(sys.path[-1]), s)
    sys.path.append(s)


def _init_pathinfo():
    """Return a set containing all existing directory entries from sys.path"""
    d = set()
    for dir in sys.path:
        try:
            if os.path.isdir(dir):
                dir, dircase = makepath(dir)
                d.add(dircase)
        except TypeError:
            continue
    return d


def addpackage(sitedir, name, known_paths):
    """Process a .pth file within the site-packages directory:
       For each line in the file, either combine it with sitedir to a path
       and add that to known_paths, or execute it if it starts with 'import '.
    """
    if known_paths is None:
        _init_pathinfo()
        reset = 1
    else:
        reset = 0
    fullname = os.path.join(sitedir, name)
    try:
        f = open(fullname, "rU")
    except IOError:
        return
    with f:
        for line in f:
            if line.startswith("#"):
                continue
            if line.startswith(("import ", "import\t")):
                exec(line)
                continue
            line = line.rstrip()
            dir, dircase = makepath(sitedir, line)
            if not dircase in known_paths and os.path.exists(dir):
                sys.path.append(dir)
                known_paths.add(dircase)
    if reset:
        known_paths = None
    return known_paths


def addsitedir(sitedir, known_paths=None):
    """Add 'sitedir' argument to sys.path if missing and handle .pth files in
    'sitedir'"""
    if known_paths is None:
        known_paths = _init_pathinfo()
        reset = 1
    else:
        reset = 0
    sitedir, sitedircase = makepath(sitedir)
    if not sitedircase in known_paths:
        sys.path.append(sitedir)        # Add path component
    try:
        names = os.listdir(sitedir)
    except os.error:
        return
    names = [name for name in names if name.endswith(".pth")]
    for name in sorted(names):
        addpackage(sitedir, name, known_paths)
    if reset:
        known_paths = None
    return known_paths


def check_enableusersite():
    """Check if user site directory is safe for inclusion

    The functions tests for the command line flag (including environment var),
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


def addusersitepackages(known_paths):
    """Add a per user site-package to sys.path

    Each user has its own python directory with site-packages in the
    home directory.

    USER_BASE is the root directory for all Python versions

    USER_SITE is the user specific site-packages directory

    USER_SITE/.. can be used for data.
    """
    global USER_BASE, USER_SITE, ENABLE_USER_SITE
    env_base = os.environ.get("PYTHONUSERBASE", None)

    def joinuser(*args):
        return os.path.expanduser(os.path.join(*args))

    #if sys.platform in ('os2emx', 'riscos'):
    #    # Don't know what to put here
    #    USER_BASE = ''
    #    USER_SITE = ''
    if os.name == "nt":
        base = os.environ.get("APPDATA") or "~"
        USER_BASE = env_base if env_base else joinuser(base, "Python")
        USER_SITE = os.path.join(USER_BASE,
                                 "Python" + sys.version[0] + sys.version[2],
                                 "site-packages")
    else:
        USER_BASE = env_base if env_base else joinuser("~", ".local")
        USER_SITE = os.path.join(USER_BASE, "lib",
                                 "python" + sys.version[:3],
                                 "site-packages")

    if ENABLE_USER_SITE and os.path.isdir(USER_SITE):
        addsitedir(USER_SITE, known_paths)
    return known_paths


def addsitepackages(known_paths):
    """Add site-packages (and possibly site-python) to sys.path"""
    sitedirs = []
    seen = []

    for prefix in PREFIXES:
        if not prefix or prefix in seen:
            continue
        seen.append(prefix)

        if sys.platform in ('os2emx', 'riscos'):
            sitedirs.append(os.path.join(prefix, "Lib", "site-packages"))
        elif os.sep == '/':
            sitedirs.append(os.path.join(prefix, "lib",
                                        "python" + sys.version[:3],
                                        "site-packages"))
            sitedirs.append(os.path.join(prefix, "lib", "site-python"))
        else:
            sitedirs.append(prefix)
            sitedirs.append(os.path.join(prefix, "lib", "site-packages"))

        if sys.platform == "darwin":
            # for framework builds *only* we add the standard Apple
            # locations. Currently only per-user, but /Library and
            # /Network/Library could be added too
            if 'Python.framework' in prefix:
                sitedirs.append(
                    os.path.expanduser(
                        os.path.join("~", "Library", "Python",
                                     sys.version[:3], "site-packages")))

    for sitedir in sitedirs:
        if os.path.isdir(sitedir):
            addsitedir(sitedir, known_paths)

    return known_paths


def setBEGINLIBPATH():
    """The OS/2 EMX port has optional extension modules that do double duty
    as DLLs (and must use the .DLL file extension) for other extensions.
    The library search path needs to be amended so these will be found
    during module import.  Use BEGINLIBPATH so that these are at the start
    of the library search path.

    """
    dllpath = os.path.join(sys.prefix, "Lib", "lib-dynload")
    libpath = os.environ['BEGINLIBPATH'].split(';')
    if libpath[-1]:
        libpath.append(dllpath)
    else:
        libpath[-1] = dllpath
    os.environ['BEGINLIBPATH'] = ';'.join(libpath)


def setquit():
    """Define new built-ins 'quit' and 'exit'.
    These are simply strings that display a hint on how to exit.

    """
    if os.sep == ':':
        eof = 'Cmd-Q'
    elif os.sep == '\\':
        eof = 'Ctrl-Z plus Return'
    else:
        eof = 'Ctrl-D (i.e. EOF)'

    class Quitter(object):
        def __init__(self, name):
            self.name = name
        def __repr__(self):
            return 'Use %s() or %s to exit' % (self.name, eof)
        def __call__(self, code=None):
            # Shells like IDLE catch the SystemExit, but listen when their
            # stdin wrapper is closed.
            try:
                fd = -1
                if hasattr(sys.stdin, "fileno"):
                    fd = sys.stdin.fileno()
                if fd != 0:
                    # Don't close stdin if it wraps fd 0
                    sys.stdin.close()
            except:
                pass
            raise SystemExit(code)
    builtins.quit = Quitter('quit')
    builtins.exit = Quitter('exit')


class _Printer(object):
    """interactive prompt objects for printing the license text, a list of
    contributors and the copyright notice."""

    MAXLINES = 23

    def __init__(self, name, data, files=(), dirs=()):
        self.__name = name
        self.__data = data
        self.__files = files
        self.__dirs = dirs
        self.__lines = None

    def __setup(self):
        if self.__lines:
            return
        data = None
        for dir in self.__dirs:
            for filename in self.__files:
                filename = os.path.join(dir, filename)
                try:
                    fp = open(filename, "rU")
                    data = fp.read()
                    fp.close()
                    break
                except IOError:
                    pass
            if data:
                break
        if not data:
            data = self.__data
        self.__lines = data.split('\n')
        self.__linecnt = len(self.__lines)

    def __repr__(self):
        self.__setup()
        if len(self.__lines) <= self.MAXLINES:
            return "\n".join(self.__lines)
        else:
            return "Type %s() to see the full %s text" % ((self.__name,)*2)

    def __call__(self):
        self.__setup()
        prompt = 'Hit Return for more, or q (and Return) to quit: '
        lineno = 0
        while 1:
            try:
                for i in range(lineno, lineno + self.MAXLINES):
                    print(self.__lines[i])
            except IndexError:
                break
            else:
                lineno += self.MAXLINES
                key = None
                while key is None:
                    key = input(prompt)
                    if key not in ('', 'q'):
                        key = None
                if key == 'q':
                    break

def setcopyright():
    """Set 'copyright' and 'credits' in builtins"""
    builtins.copyright = _Printer("copyright", sys.copyright)
    if sys.platform[:4] == 'java':
        builtins.credits = _Printer(
            "credits",
            "Jython is maintained by the Jython developers (www.jython.org).")
    else:
        builtins.credits = _Printer("credits", """\
    Thanks to CWI, CNRI, BeOpen.com, Zope Corporation and a cast of thousands
    for supporting Python development.  See www.python.org for more information.""")
    here = os.path.dirname(os.__file__)
    builtins.license = _Printer(
        "license", "See http://www.python.org/%.3s/license.html" % sys.version,
        ["LICENSE.txt", "LICENSE"],
        [os.path.join(here, os.pardir), here, os.curdir])


class _Helper(object):
    """Define the built-in 'help'.
    This is a wrapper around pydoc.help (with a twist).

    """

    def __repr__(self):
        return "Type help() for interactive help, " \
               "or help(object) for help about object."
    def __call__(self, *args, **kwds):
        import pydoc
        return pydoc.help(*args, **kwds)

def sethelper():
    builtins.help = _Helper()

def aliasmbcs():
    """On Windows, some default encodings are not provided by Python,
    while they are always available as "mbcs" in each locale. Make
    them usable by aliasing to "mbcs" in such a case."""
    if sys.platform == 'win32':
        import locale, codecs
        enc = locale.getdefaultlocale()[1]
        if enc.startswith('cp'):            # "cp***" ?
            try:
                codecs.lookup(enc)
            except LookupError:
                import encodings
                encodings._cache[enc] = encodings._unknown
                encodings.aliases.aliases[enc] = 'mbcs'

def setencoding():
    """Set the string encoding used by the Unicode implementation.  The
    default is 'ascii', but if you're willing to experiment, you can
    change this."""
    encoding = "ascii" # Default value set by _PyUnicode_Init()
    if 0:
        # Enable to support locale aware default string encodings.
        import locale
        loc = locale.getdefaultlocale()
        if loc[1]:
            encoding = loc[1]
    if 0:
        # Enable to switch off string to Unicode coercion and implicit
        # Unicode to string conversion.
        encoding = "undefined"
    if encoding != "ascii":
        # On Non-Unicode builds this will raise an AttributeError...
        sys.setdefaultencoding(encoding) # Needs Python Unicode build !


def execsitecustomize():
    """Run custom site specific code, if available."""
    try:
        import sitecustomize
    except ImportError:
        pass
    except Exception as err:
        if os.environ.get("PYTHONVERBOSE"):
            raise
        sys.stderr.write(
            "Error in sitecustomize; set PYTHONVERBOSE for traceback:\n"
            "%s: %s\n" %
            (err.__class__.__name__, err))


def execusercustomize():
    """Run custom user specific code, if available."""
    try:
        import usercustomize
    except ImportError:
        pass


def main():
    global ENABLE_USER_SITE

    abs__file__()
    known_paths = removeduppaths()
    if (os.name == "posix" and sys.path and
        os.path.basename(sys.path[-1]) == "Modules"):
        addbuilddir()
    if ENABLE_USER_SITE is None:
        ENABLE_USER_SITE = check_enableusersite()
    known_paths = addusersitepackages(known_paths)
    known_paths = addsitepackages(known_paths)
    if sys.platform == 'os2emx':
        setBEGINLIBPATH()
    setquit()
    setcopyright()
    sethelper()
    aliasmbcs()
    setencoding()
    execsitecustomize()
    if ENABLE_USER_SITE:
        execusercustomize()
    # Remove sys.setdefaultencoding() so that users cannot change the
    # encoding after initialization.  The test for presence is needed when
    # this module is run as a script, because this code is executed twice.
    if hasattr(sys, "setdefaultencoding"):
        del sys.setdefaultencoding

main()

def _script():
    help = """\
    %s [--user-base] [--user-site]

    Without arguments print some useful information
    With arguments print the value of USER_BASE and/or USER_SITE separated
    by '%s'.

    Exit codes with --user-base or --user-site:
      0 - user site directory is enabled
      1 - user site diretory is disabled by user
      2 - uses site directory is disabled by super user
          or for security reasons
     >2 - unknown error
    """
    args = sys.argv[1:]
    if not args:
        print("sys.path = [")
        for dir in sys.path:
            print("    %r," % (dir,))
        print("]")
        print("USER_BASE: %r (%s)" % (USER_BASE,
            "exists" if os.path.isdir(USER_BASE) else "doesn't exist"))
        print("USER_SITE: %r (%s)" % (USER_SITE,
            "exists" if os.path.isdir(USER_SITE) else "doesn't exist"))
        print("ENABLE_USER_SITE: %r" %  ENABLE_USER_SITE)
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
