"""Append module search paths for third-party packages to sys.path.

****************************************************************
* This module is automatically imported during initialization. *
****************************************************************

In earlier versions of Python (up to 1.5a3), scripts or modules that
needed to use site-specific modules would place ``import site''
somewhere near the top of their code.  Because of the automatic
import, this is no longer necessary (but code that does it still
works).

This will append site-specific paths to to the module search path.  On
Unix, it starts with sys.prefix and sys.exec_prefix (if different) and
appends lib/python<version>/site-packages as well as lib/site-python.
On other platforms (mainly Mac and Windows), it uses just sys.prefix
\(and sys.exec_prefix, if different, but this is unlikely).  The
resulting directories, if they exist, are appended to sys.path, and
also inspected for path configuration files.

A path configuration file is a file whose name has the form
<package>.pth; its contents are additional directories (one per line)
to be added to sys.path.  Non-existing directories (or
non-directories) are never added to sys.path; no directory is added to
sys.path more than once.  Blank lines and lines beginning with
\code{#} are skipped. Lines starting with \code{import} are executed.

For example, suppose sys.prefix and sys.exec_prefix are set to
/usr/local and there is a directory /usr/local/lib/python1.5/site-packages
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

  /usr/local/lib/python1.5/site-packages/bar
  /usr/local/lib/python1.5/site-packages/foo

Note that bletch is omitted because it doesn't exist; bar precedes foo
because bar.pth comes alphabetically before foo.pth; and spam is
omitted because it is not mentioned in either path configuration file.

After these path manipulations, an attempt is made to import a module
named sitecustomize, which can perform arbitrary additional
site-specific customizations.  If this import fails with an
ImportError exception, it is silently ignored.

"""

import sys, os

if os.sep==".":
    endsep = "/"
else:
    endsep = "."


def makepath(*paths):
    dir = os.path.abspath(os.path.join(*paths))
    return dir, os.path.normcase(dir)

for m in sys.modules.values():
    if hasattr(m, "__file__") and m.__file__:
        m.__file__ = os.path.abspath(m.__file__)
del m

# This ensures that the initial path provided by the interpreter contains
# only absolute pathnames, even if we're running from the build directory.
L = []
dirs_in_sys_path = {}
for dir in sys.path:
    dir, dircase = makepath(dir)
    if not dirs_in_sys_path.has_key(dircase):
        L.append(dir)
        dirs_in_sys_path[dircase] = 1
sys.path[:] = L
del dir, L

# Append ./build/lib.<platform> in case we're running in the build dir
# (especially for Guido :-)
if os.name == "posix" and os.path.basename(sys.path[-1]) == "Modules":
    from distutils.util import get_platform
    s = "build/lib.%s-%.3s" % (get_platform(), sys.version)
    s = os.path.join(os.path.dirname(sys.path[-1]), s)
    sys.path.append(s)
    del get_platform, s

def addsitedir(sitedir):
    sitedir, sitedircase = makepath(sitedir)
    if not dirs_in_sys_path.has_key(sitedircase):
        sys.path.append(sitedir)        # Add path component
    try:
        names = os.listdir(sitedir)
    except os.error:
        return
    names.sort()
    for name in names:
        if name[-4:] == endsep + "pth":
            addpackage(sitedir, name)

def addpackage(sitedir, name):
    fullname = os.path.join(sitedir, name)
    try:
        f = open(fullname)
    except IOError:
        return
    while 1:
        dir = f.readline()
        if not dir:
            break
        if dir[0] == '#':
            continue
        if dir.startswith("import"):
            exec dir
            continue
        if dir[-1] == '\n':
            dir = dir[:-1]
        dir, dircase = makepath(sitedir, dir)
        if not dirs_in_sys_path.has_key(dircase) and os.path.exists(dir):
            sys.path.append(dir)
            dirs_in_sys_path[dircase] = 1

prefixes = [sys.prefix]
if sys.exec_prefix != sys.prefix:
    prefixes.append(sys.exec_prefix)
for prefix in prefixes:
    if prefix:
        if os.sep == '/':
            sitedirs = [os.path.join(prefix,
                                     "lib",
                                     "python" + sys.version[:3],
                                     "site-packages"),
                        os.path.join(prefix, "lib", "site-python")]
        elif os.sep == ':':
            sitedirs = [os.path.join(prefix, "lib", "site-packages")]
        else:
            sitedirs = [prefix]
        for sitedir in sitedirs:
            if os.path.isdir(sitedir):
                addsitedir(sitedir)

del dirs_in_sys_path

# Define new built-ins 'quit' and 'exit'.
# These are simply strings that display a hint on how to exit.
if os.sep == ':':
    exit = 'Use Cmd-Q to quit.'
elif os.sep == '\\':
    exit = 'Use Ctrl-Z plus Return to exit.'
else:
    exit = 'Use Ctrl-D (i.e. EOF) to exit.'
import __builtin__
__builtin__.quit = __builtin__.exit = exit
del exit

# interactive prompt objects for printing the license text, a list of
# contributors and the copyright notice.
class _Printer:
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
            for file in self.__files:
                file = os.path.join(dir, file)
                try:
                    fp = open(file)
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
                    print self.__lines[i]
            except IndexError:
                break
            else:
                lineno += self.MAXLINES
                key = None
                while key is None:
                    key = raw_input(prompt)
                    if key not in ('', 'q'):
                        key = None
                if key == 'q':
                    break

__builtin__.copyright = _Printer("copyright", sys.copyright)
if sys.platform[:4] == 'java':
    __builtin__.credits = _Printer(
        "credits",
        "Jython is maintained by the Jython developers (www.jython.org).")
else:
    __builtin__.credits = _Printer("credits", """\
Thanks to CWI, CNRI, BeOpen.com, Digital Creations and a cast of thousands
for supporting Python development.  See www.python.org for more information.""")
here = os.path.dirname(os.__file__)
__builtin__.license = _Printer(
    "license", "See http://www.pythonlabs.com/products/python2.0/license.html",
    ["LICENSE.txt", "LICENSE"],
    [os.path.join(here, os.pardir), here, os.curdir])


# Set the string encoding used by the Unicode implementation.  The
# default is 'ascii', but if you're willing to experiment, you can
# change this.

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
    sys.setdefaultencoding(encoding)

#
# Run custom site specific code, if available.
#
try:
    import sitecustomize
except ImportError:
    pass

#
# Remove sys.setdefaultencoding() so that users cannot change the
# encoding after initialization.  The test for presence is needed when
# this module is run as a script, because this code is executed twice.
#
if hasattr(sys, "setdefaultencoding"):
    del sys.setdefaultencoding

def _test():
    print "sys.path = ["
    for dir in sys.path:
        print "    %s," % `dir`
    print "]"

if __name__ == '__main__':
    _test()
