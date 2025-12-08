#!/usr/bin/env python
"""
This script is used to build "official" universal installers on macOS.

NEW for 3.10 and backports:
- support universal2 variant with arm64 and x86_64 archs
- enable clang optimizations when building on 10.15+

NEW for 3.9.0 and backports:
- 2.7 end-of-life issues:
    - Python 3 installs now update the Current version link
      in /Library/Frameworks/Python.framework/Versions
- fully support running under Python 3 as well as 2.7
- support building on newer macOS systems with SIP
- fully support building on macOS 10.9+
- support 10.6+ on best effort
- support bypassing docs build by supplying a prebuilt
    docs html tarball in the third-party source library,
    in the format and filename conventional of those
    downloadable from python.org:
        python-3.x.y-docs-html.tar.bz2

NEW for 3.7.0:
- support Intel 64-bit-only () and 32-bit-only installer builds
- build and use internal Tcl/Tk 8.6 for 10.6+ builds
- deprecate use of explicit SDK (--sdk-path=) since all but the oldest
  versions of Xcode support implicit setting of an SDK via environment
  variables (SDKROOT and friends, see the xcrun man page for more info).
  The SDK stuff was primarily needed for building universal installers
  for 10.4; so as of 3.7.0, building installers for 10.4 is no longer
  supported with build-installer.
- use generic "gcc" as compiler (CC env var) rather than "gcc-4.2"

TODO:
- test building with SDKROOT and DEVELOPER_DIR xcrun env variables

Usage: see USAGE variable in the script.
"""
import platform, os, sys, getopt, textwrap, shutil, stat, time, pwd, grp
try:
    import urllib2 as urllib_request
except ImportError:
    import urllib.request as urllib_request

STAT_0o755 = ( stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR
             | stat.S_IRGRP |                stat.S_IXGRP
             | stat.S_IROTH |                stat.S_IXOTH )

STAT_0o775 = ( stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR
             | stat.S_IRGRP | stat.S_IWGRP | stat.S_IXGRP
             | stat.S_IROTH |                stat.S_IXOTH )

INCLUDE_TIMESTAMP = 1
VERBOSE = 1

RUNNING_ON_PYTHON2 = sys.version_info.major == 2

if RUNNING_ON_PYTHON2:
    from plistlib import writePlist
else:
    from plistlib import dump
    def writePlist(path, plist):
        with open(plist, 'wb') as fp:
            dump(path, fp)

def shellQuote(value):
    """
    Return the string value in a form that can safely be inserted into
    a shell command.
    """
    return "'%s'"%(value.replace("'", "'\"'\"'"))

def grepValue(fn, variable):
    """
    Return the unquoted value of a variable from a file..
    QUOTED_VALUE='quotes'    -> str('quotes')
    UNQUOTED_VALUE=noquotes  -> str('noquotes')
    """
    variable = variable + '='
    for ln in open(fn, 'r'):
        if ln.startswith(variable):
            value = ln[len(variable):].strip()
            return value.strip("\"'")
    raise RuntimeError("Cannot find variable %s" % variable[:-1])

_cache_getVersion = None

def getVersion():
    global _cache_getVersion
    if _cache_getVersion is None:
        _cache_getVersion = grepValue(
            os.path.join(SRCDIR, 'configure'), 'PACKAGE_VERSION')
    return _cache_getVersion

def getVersionMajorMinor():
    return tuple([int(n) for n in getVersion().split('.', 2)])

_cache_getFullVersion = None

def getFullVersion():
    global _cache_getFullVersion
    if _cache_getFullVersion is not None:
        return _cache_getFullVersion
    fn = os.path.join(SRCDIR, 'Include', 'patchlevel.h')
    for ln in open(fn):
        if 'PY_VERSION' in ln:
            _cache_getFullVersion = ln.split()[-1][1:-1]
            return _cache_getFullVersion
    raise RuntimeError("Cannot find full version??")

FW_PREFIX = ["Library", "Frameworks", "Python.framework"]
FW_VERSION_PREFIX = "--undefined--" # initialized in parseOptions
FW_SSL_DIRECTORY = "--undefined--" # initialized in parseOptions

# The directory we'll use to create the build (will be erased and recreated)
WORKDIR = "/tmp/_py"

# The directory we'll use to store third-party sources. Set this to something
# else if you don't want to re-fetch required libraries every time.
DEPSRC = os.path.join(WORKDIR, 'third-party')
DEPSRC = os.path.expanduser('~/Universal/other-sources')

universal_opts_map = { 'universal2': ('arm64', 'x86_64'),
                       '32-bit': ('i386', 'ppc',),
                       '64-bit': ('x86_64', 'ppc64',),
                       'intel':  ('i386', 'x86_64'),
                       'intel-32':  ('i386',),
                       'intel-64':  ('x86_64',),
                       '3-way':  ('ppc', 'i386', 'x86_64'),
                       'all':    ('i386', 'ppc', 'x86_64', 'ppc64',) }
default_target_map = {
        'universal2': '10.9',
        '64-bit': '10.5',
        '3-way': '10.5',
        'intel': '10.5',
        'intel-32': '10.4',
        'intel-64': '10.5',
        'all': '10.5',
}

UNIVERSALOPTS = tuple(universal_opts_map.keys())

UNIVERSALARCHS = '32-bit'

ARCHLIST = universal_opts_map[UNIVERSALARCHS]

# Source directory (assume we're in Mac/BuildScript)
SRCDIR = os.path.dirname(
        os.path.dirname(
            os.path.dirname(
                os.path.abspath(__file__
        ))))

# $MACOSX_DEPLOYMENT_TARGET -> minimum OS X level
DEPTARGET = '10.5'

def getDeptargetTuple():
    return tuple([int(n) for n in DEPTARGET.split('.')[0:2]])

def getBuildTuple():
    return tuple([int(n) for n in platform.mac_ver()[0].split('.')[0:2]])

def getTargetCompilers():
    target_cc_map = {
        '10.4': ('gcc-4.0', 'g++-4.0'),
        '10.5': ('gcc', 'g++'),
        '10.6': ('gcc', 'g++'),
        '10.7': ('gcc', 'g++'),
        '10.8': ('gcc', 'g++'),
    }
    return target_cc_map.get(DEPTARGET, ('clang', 'clang++') )

CC, CXX = getTargetCompilers()

PYTHON_3 = getVersionMajorMinor() >= (3, 0)

USAGE = textwrap.dedent("""\
    Usage: build_python [options]

    Options:
    -? or -h:            Show this message
    -b DIR
    --build-dir=DIR:     Create build here (default: %(WORKDIR)r)
    --third-party=DIR:   Store third-party sources here (default: %(DEPSRC)r)
    --sdk-path=DIR:      Location of the SDK (deprecated, use SDKROOT env variable)
    --src-dir=DIR:       Location of the Python sources (default: %(SRCDIR)r)
    --dep-target=10.n    macOS deployment target (default: %(DEPTARGET)r)
    --universal-archs=x  universal architectures (options: %(UNIVERSALOPTS)r, default: %(UNIVERSALARCHS)r)
""")% globals()

# Dict of object file names with shared library names to check after building.
# This is to ensure that we ended up dynamically linking with the shared
# library paths and versions we expected.  For example:
#   EXPECTED_SHARED_LIBS['_tkinter.so'] = [
#                       '/Library/Frameworks/Tcl.framework/Versions/8.5/Tcl',
#                       '/Library/Frameworks/Tk.framework/Versions/8.5/Tk']
EXPECTED_SHARED_LIBS = {}

# Are we building and linking with our own copy of Tcl/TK?
#   For now, do so if deployment target is 10.6+.
def internalTk():
    return getDeptargetTuple() >= (10, 6)

# Do we use 8.6.8 when building our own copy
# of Tcl/Tk or a modern version.
#   We use the old version when building on
#   old versions of macOS due to build issues.
def useOldTk():
    return getBuildTuple() < (10, 15)


def tweak_tcl_build(basedir, archList):
    with open("Makefile", "r") as fp:
        contents = fp.readlines()

    # For reasons I don't understand the tcl configure script
    # decides that some stdlib symbols aren't present, before
    # deciding that strtod is broken.
    new_contents = []
    for line in contents:
        if line.startswith("COMPAT_OBJS"):
            # note: the space before strtod.o is intentional,
            # the detection of a broken strtod results in
            # "fixstrod.o" on this line.
            for nm in ("strstr.o", "strtoul.o", " strtod.o"):
                line = line.replace(nm, "")
        new_contents.append(line)

    with open("Makefile", "w") as fp:
        fp.writelines(new_contents)

# List of names of third party software built with this installer.
# The names will be inserted into the rtf version of the License.
THIRD_PARTY_LIBS = []

# Instructions for building libraries that are necessary for building a
# batteries included python.
#   [The recipes are defined here for convenience but instantiated later after
#    command line options have been processed.]
def library_recipes():
    result = []

    # Since Apple removed the header files for the deprecated system
    # OpenSSL as of the Xcode 7 release (for OS X 10.10+), we do not
    # have much choice but to build our own copy here, too.

    result.extend([
          dict(
              name="OpenSSL 3.5.4",
              url="https://github.com/openssl/openssl/releases/download/openssl-3.5.4/openssl-3.5.4.tar.gz",
              checksum="967311f84955316969bdb1d8d4b983718ef42338639c621ec4c34fddef355e99",
              buildrecipe=build_universal_openssl,
              configure=None,
              install=None,
          ),
    ])

    if internalTk():
        if useOldTk():
            tcl_tk_ver='8.6.8'
            tcl_checksum='81656d3367af032e0ae6157eff134f89'

            tk_checksum='5e0faecba458ee1386078fb228d008ba'
            tk_patches = ['backport_gh71383_fix.patch', 'tk868_on_10_8_10_9.patch', 'backport_gh110950_fix.patch']

        else:
            tcl_tk_ver='9.0.2'
            tcl_checksum='e074c6a8d9ba2cddf914ba97b6677a552d7a52a3ca102924389a05ccb249b520'

            tk_checksum='76fb852b2f167592fe8b41aa6549ce4e486dbf3b259a269646600e3894517c76'
            tk_patches = []


        base_url = "https://prdownloads.sourceforge.net/tcl/{what}{version}-src.tar.gz"
        result.extend([
          dict(
              name="Tcl %s"%(tcl_tk_ver,),
              url=base_url.format(what="tcl", version=tcl_tk_ver),
              checksum=tcl_checksum,
              buildDir="unix",
              configure_pre=[
                    '--enable-shared',
                    '--enable-threads',
                    '--libdir=/Library/Frameworks/Python.framework/Versions/%s/lib'%(getVersion(),),
              ],
              useLDFlags=False,
              buildrecipe=tweak_tcl_build,
              install='make TCL_LIBRARY=%(TCL_LIBRARY)s && make install TCL_LIBRARY=%(TCL_LIBRARY)s DESTDIR=%(DESTDIR)s'%{
                  "DESTDIR": shellQuote(os.path.join(WORKDIR, 'libraries')),
                  "TCL_LIBRARY": shellQuote('/Library/Frameworks/Python.framework/Versions/%s/lib/tcl8.6'%(getVersion())),
                  },
              ),
          dict(
              name="Tk %s"%(tcl_tk_ver,),
              url=base_url.format(what="tk", version=tcl_tk_ver),
              checksum=tk_checksum,
              patches=tk_patches,
              buildDir="unix",
              configure_pre=[
                    '--enable-aqua',
                    '--enable-shared',
                    '--enable-threads',
                    '--libdir=/Library/Frameworks/Python.framework/Versions/%s/lib'%(getVersion(),),
              ],
              useLDFlags=False,
              install='make TCL_LIBRARY=%(TCL_LIBRARY)s TK_LIBRARY=%(TK_LIBRARY)s && make install TCL_LIBRARY=%(TCL_LIBRARY)s TK_LIBRARY=%(TK_LIBRARY)s DESTDIR=%(DESTDIR)s'%{
                  "DESTDIR": shellQuote(os.path.join(WORKDIR, 'libraries')),
                  "TCL_LIBRARY": shellQuote('/Library/Frameworks/Python.framework/Versions/%s/lib/tcl8.6'%(getVersion())),
                  "TK_LIBRARY": shellQuote('/Library/Frameworks/Python.framework/Versions/%s/lib/tk8.6'%(getVersion())),
                  },
                ),
        ])

    if PYTHON_3:
        result.extend([
          dict(
              name="XZ 5.2.3",
              url="http://tukaani.org/xz/xz-5.2.3.tar.gz",
              checksum='ef68674fb47a8b8e741b34e429d86e9d',
              configure_pre=[
                    '--disable-dependency-tracking',
              ]
              ),
        ])

    result.extend([
          dict(
              name="NCurses 6.5",
              url="https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz",
              checksum="136d91bc269a9a5785e5f9e980bc76ab57428f604ce3e5a5a90cebc767971cc6",
              configure_pre=[
                  "--datadir=/usr/share",
                  "--disable-lib-suffixes",
                  "--disable-db-install",
                  "--disable-mixed-case",
                  "--enable-overwrite",
                  "--enable-widec",
                  f"--libdir=/Library/Frameworks/Python.framework/Versions/{getVersion()}/lib",
                  "--sharedstatedir=/usr/com",
                  "--sysconfdir=/etc",
                  "--with-default-terminfo-dir=/usr/share/terminfo",
                  "--with-shared",
                  "--with-terminfo-dirs=/usr/share/terminfo",
                  "--without-ada",
                  "--without-cxx",
                  "--without-cxx-binding",
                  "--without-cxx-shared",
                  "--without-debug",
                  "--without-manpages",
                  "--without-normal",
                  "--without-progs",
                  "--without-tests",
              ],
              useLDFlags=False,
              install='make && make install DESTDIR=%s && cd %s/usr/local/lib && ln -fs ../../../Library/Frameworks/Python.framework/Versions/%s/lib/lib* .'%(
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  getVersion(),
                  ),
          ),
          dict(
              name="SQLite 3.50.4",
              url="https://www.sqlite.org/2025/sqlite-autoconf-3500400.tar.gz",
              checksum="a3db587a1b92ee5ddac2f66b3edb41b26f9c867275782d46c3a088977d6a5b18",
              extra_cflags=('-Os '
                            '-DSQLITE_ENABLE_FTS5 '
                            '-DSQLITE_ENABLE_FTS4 '
                            '-DSQLITE_ENABLE_FTS3_PARENTHESIS '
                            '-DSQLITE_ENABLE_RTREE '
                            '-DSQLITE_OMIT_AUTOINIT '
                            '-DSQLITE_TCL=0 '
                            ),
              configure_pre=[
                  '--enable-threadsafe',
                  '--disable-readline',
                  '--disable-dependency-tracking',
              ],
              install=f"make && ranlib libsqlite3.a && make install DESTDIR={shellQuote(os.path.join(WORKDIR, 'libraries'))}",
          ),
          dict(
              name="libmpdec 4.0.1",
              url="https://www.bytereef.org/software/mpdecimal/releases/mpdecimal-4.0.1.tar.gz",
              checksum="96d33abb4bb0070c7be0fed4246cd38416188325f820468214471938545b1ac8",
              configure_pre=[
                  "--disable-cxx",
                  "MACHINE=universal",
              ]
          ),
        ])

    if not PYTHON_3:
        result.extend([
          dict(
              name="Sleepycat DB 4.7.25",
              url="http://download.oracle.com/berkeley-db/db-4.7.25.tar.gz",
              checksum='ec2b87e833779681a0c3a814aa71359e',
              buildDir="build_unix",
              configure="../dist/configure",
              configure_pre=[
                  '--includedir=/usr/local/include/db4',
              ]
          ),
        ])

    return result

def compilerCanOptimize():
    """
    Return True iff the default Xcode version can use PGO and LTO
    """
    # bpo-42235: The version check is pretty conservative, can be
    # adjusted after testing
    mac_ver = tuple(map(int, platform.mac_ver()[0].split('.')))
    return mac_ver >= (10, 15)

# Instructions for building packages inside the .mpkg.
def pkg_recipes():
    unselected_for_python3 = ('selected', 'unselected')[PYTHON_3]
    result = [
        dict(
            name="PythonFramework",
            long_name="Python Framework",
            source="/Library/Frameworks/Python.framework",
            readme="""\
                This package installs Python.framework, that is the python
                interpreter and the standard library.
            """,
            postflight="scripts/postflight.framework",
            selected='selected',
        ),
        dict(
            name="PythonApplications",
            long_name="GUI Applications",
            source="/Applications/Python %(VER)s",
            readme="""\
                This package installs IDLE (an interactive Python IDE),
                Python Launcher and Build Applet (create application bundles
                from python scripts).

                It also installs a number of examples and demos.
                """,
            required=False,
            selected='selected',
        ),
        dict(
            name="PythonUnixTools",
            long_name="UNIX command-line tools",
            source="/usr/local/bin",
            readme="""\
                This package installs the unix tools in /usr/local/bin for
                compatibility with older releases of Python. This package
                is not necessary to use Python.
                """,
            required=False,
            selected='selected',
        ),
        dict(
            name="PythonDocumentation",
            long_name="Python Documentation",
            topdir="/Library/Frameworks/Python.framework/Versions/%(VER)s/Resources/English.lproj/Documentation",
            source="/pydocs",
            readme="""\
                This package installs the python documentation at a location
                that is usable for pydoc and IDLE.
                """,
            postflight="scripts/postflight.documentation",
            required=False,
            selected='selected',
        ),
        dict(
            name="PythonProfileChanges",
            long_name="Shell profile updater",
            readme="""\
                This packages updates your shell profile to make sure that
                the Python tools are found by your shell in preference of
                the system provided Python tools.

                If you don't install this package you'll have to add
                "/Library/Frameworks/Python.framework/Versions/%(VER)s/bin"
                to your PATH by hand.
                """,
            postflight="scripts/postflight.patch-profile",
            topdir="/Library/Frameworks/Python.framework",
            source="/empty-dir",
            required=False,
            selected='selected',
        ),
        dict(
            name="PythonInstallPip",
            long_name="Install or upgrade pip",
            readme="""\
                This package installs (or upgrades from an earlier version)
                pip, a tool for installing and managing Python packages.
                """,
            postflight="scripts/postflight.ensurepip",
            topdir="/Library/Frameworks/Python.framework",
            source="/empty-dir",
            required=False,
            selected='selected',
        ),
    ]

    return result

def fatal(msg):
    """
    A fatal error, bail out.
    """
    sys.stderr.write('FATAL: ')
    sys.stderr.write(msg)
    sys.stderr.write('\n')
    sys.exit(1)

def fileContents(fn):
    """
    Return the contents of the named file
    """
    return open(fn, 'r').read()

def runCommand(commandline):
    """
    Run a command and raise RuntimeError if it fails. Output is suppressed
    unless the command fails.
    """
    fd = os.popen(commandline, 'r')
    data = fd.read()
    xit = fd.close()
    if xit is not None:
        sys.stdout.write(data)
        raise RuntimeError("command failed: %s"%(commandline,))

    if VERBOSE:
        sys.stdout.write(data); sys.stdout.flush()

def captureCommand(commandline):
    fd = os.popen(commandline, 'r')
    data = fd.read()
    xit = fd.close()
    if xit is not None:
        sys.stdout.write(data)
        raise RuntimeError("command failed: %s"%(commandline,))

    return data

def getTclTkVersion(configfile, versionline):
    """
    search Tcl or Tk configuration file for version line
    """
    try:
        f = open(configfile, "r")
    except OSError:
        fatal("Framework configuration file not found: %s" % configfile)

    for l in f:
        if l.startswith(versionline):
            f.close()
            return l

    fatal("Version variable %s not found in framework configuration file: %s"
            % (versionline, configfile))

def checkEnvironment():
    """
    Check that we're running on a supported system.
    """

    if sys.version_info[0:2] < (2, 7):
        fatal("This script must be run with Python 2.7 (or later)")

    if platform.system() != 'Darwin':
        fatal("This script should be run on a macOS 10.5 (or later) system")

    if int(platform.release().split('.')[0]) < 8:
        fatal("This script should be run on a macOS 10.5 (or later) system")

    # Because we only support dynamic load of only one major/minor version of
    # Tcl/Tk, if we are not using building and using our own private copy of
    # Tcl/Tk, ensure:
    # 1. there is a user-installed framework (usually ActiveTcl) in (or linked
    #       in) SDKROOT/Library/Frameworks.  As of Python 3.7.0, we no longer
    #       enforce that the version of the user-installed framework also
    #       exists in the system-supplied Tcl/Tk frameworks.  Time to support
    #       Tcl/Tk 8.6 even if Apple does not.
    if not internalTk():
        frameworks = {}
        for framework in ['Tcl', 'Tk']:
            fwpth = 'Library/Frameworks/%s.framework/Versions/Current' % framework
            libfw = os.path.join('/', fwpth)
            usrfw = os.path.join(os.getenv('HOME'), fwpth)
            frameworks[framework] = os.readlink(libfw)
            if not os.path.exists(libfw):
                fatal("Please install a link to a current %s %s as %s so "
                        "the user can override the system framework."
                        % (framework, frameworks[framework], libfw))
            if os.path.exists(usrfw):
                fatal("Please rename %s to avoid possible dynamic load issues."
                        % usrfw)

        if frameworks['Tcl'] != frameworks['Tk']:
            fatal("The Tcl and Tk frameworks are not the same version.")

        print(" -- Building with external Tcl/Tk %s frameworks"
                    % frameworks['Tk'])

        # add files to check after build
        EXPECTED_SHARED_LIBS['_tkinter.so'] = [
                "/Library/Frameworks/Tcl.framework/Versions/%s/Tcl"
                    % frameworks['Tcl'],
                "/Library/Frameworks/Tk.framework/Versions/%s/Tk"
                    % frameworks['Tk'],
                ]
    else:
        print(" -- Building private copy of Tcl/Tk")
    print("")

    # Remove inherited environment variables which might influence build
    environ_var_prefixes = ['CPATH', 'C_INCLUDE_', 'DYLD_', 'LANG', 'LC_',
                            'LD_', 'LIBRARY_', 'PATH', 'PYTHON']
    for ev in list(os.environ):
        for prefix in environ_var_prefixes:
            if ev.startswith(prefix) :
                print("INFO: deleting environment variable %s=%s" % (
                                                    ev, os.environ[ev]))
                del os.environ[ev]

    base_path = '/bin:/sbin:/usr/bin:/usr/sbin'
    if 'SDK_TOOLS_BIN' in os.environ:
        base_path = os.environ['SDK_TOOLS_BIN'] + ':' + base_path
    # Xcode 2.5 on OS X 10.4 does not include SetFile in its usr/bin;
    # add its fixed location here if it exists
    OLD_DEVELOPER_TOOLS = '/Developer/Tools'
    if os.path.isdir(OLD_DEVELOPER_TOOLS):
        base_path = base_path + ':' + OLD_DEVELOPER_TOOLS
    os.environ['PATH'] = base_path
    print("Setting default PATH: %s"%(os.environ['PATH']))

def parseOptions(args=None):
    """
    Parse arguments and update global settings.
    """
    global WORKDIR, DEPSRC, SRCDIR, DEPTARGET
    global UNIVERSALOPTS, UNIVERSALARCHS, ARCHLIST, CC, CXX
    global FW_VERSION_PREFIX
    global FW_SSL_DIRECTORY

    if args is None:
        args = sys.argv[1:]

    try:
        options, args = getopt.getopt(args, '?hb',
                [ 'build-dir=', 'third-party=', 'sdk-path=' , 'src-dir=',
                  'dep-target=', 'universal-archs=', 'help' ])
    except getopt.GetoptError:
        print(sys.exc_info()[1])
        sys.exit(1)

    if args:
        print("Additional arguments")
        sys.exit(1)

    deptarget = None
    for k, v in options:
        if k in ('-h', '-?', '--help'):
            print(USAGE)
            sys.exit(0)

        elif k in ('-d', '--build-dir'):
            WORKDIR=v

        elif k in ('--third-party',):
            DEPSRC=v

        elif k in ('--sdk-path',):
            print(" WARNING: --sdk-path is no longer supported")

        elif k in ('--src-dir',):
            SRCDIR=v

        elif k in ('--dep-target', ):
            DEPTARGET=v
            deptarget=v

        elif k in ('--universal-archs', ):
            if v in UNIVERSALOPTS:
                UNIVERSALARCHS = v
                ARCHLIST = universal_opts_map[UNIVERSALARCHS]
                if deptarget is None:
                    # Select alternate default deployment
                    # target
                    DEPTARGET = default_target_map.get(v, '10.5')
            else:
                raise NotImplementedError(v)

        else:
            raise NotImplementedError(k)

    SRCDIR=os.path.abspath(SRCDIR)
    WORKDIR=os.path.abspath(WORKDIR)
    DEPSRC=os.path.abspath(DEPSRC)

    CC, CXX = getTargetCompilers()

    FW_VERSION_PREFIX = FW_PREFIX[:] + ["Versions", getVersion()]
    FW_SSL_DIRECTORY = FW_VERSION_PREFIX[:] + ["etc", "openssl"]

    print("-- Settings:")
    print("   * Source directory:    %s" % SRCDIR)
    print("   * Build directory:     %s" % WORKDIR)
    print("   * Third-party source:  %s" % DEPSRC)
    print("   * Deployment target:   %s" % DEPTARGET)
    print("   * Universal archs:     %s" % str(ARCHLIST))
    print("   * C compiler:          %s" % CC)
    print("   * C++ compiler:        %s" % CXX)
    print("")
    print(" -- Building a Python %s framework at patch level %s"
                % (getVersion(), getFullVersion()))
    print("")

def extractArchive(builddir, archiveName):
    """
    Extract a source archive into 'builddir'. Returns the path of the
    extracted archive.

    XXX: This function assumes that archives contain a toplevel directory
    that is has the same name as the basename of the archive. This is
    safe enough for almost anything we use.  Unfortunately, it does not
    work for current Tcl and Tk source releases where the basename of
    the archive ends with "-src" but the uncompressed directory does not.
    For now, just special case Tcl and Tk tar.gz downloads.
    """
    curdir = os.getcwd()
    try:
        os.chdir(builddir)
        if archiveName.endswith('.tar.gz'):
            retval = os.path.basename(archiveName[:-7])
            if ((retval.startswith('tcl') or retval.startswith('tk'))
                    and retval.endswith('-src')):
                retval = retval[:-4]
                # Strip rcxx suffix from Tcl/Tk release candidates
                retval_rc = retval.find('rc')
                if retval_rc > 0:
                    retval = retval[:retval_rc]
            if os.path.exists(retval):
                shutil.rmtree(retval)
            fp = os.popen("tar zxf %s 2>&1"%(shellQuote(archiveName),), 'r')

        elif archiveName.endswith('.tar.bz2'):
            retval = os.path.basename(archiveName[:-8])
            if os.path.exists(retval):
                shutil.rmtree(retval)
            fp = os.popen("tar jxf %s 2>&1"%(shellQuote(archiveName),), 'r')

        elif archiveName.endswith('.tar'):
            retval = os.path.basename(archiveName[:-4])
            if os.path.exists(retval):
                shutil.rmtree(retval)
            fp = os.popen("tar xf %s 2>&1"%(shellQuote(archiveName),), 'r')

        elif archiveName.endswith('.zip'):
            retval = os.path.basename(archiveName[:-4])
            if os.path.exists(retval):
                shutil.rmtree(retval)
            fp = os.popen("unzip %s 2>&1"%(shellQuote(archiveName),), 'r')

        data = fp.read()
        xit = fp.close()
        if xit is not None:
            sys.stdout.write(data)
            raise RuntimeError("Cannot extract %s"%(archiveName,))

        return os.path.join(builddir, retval)

    finally:
        os.chdir(curdir)

def downloadURL(url, fname):
    """
    Download the contents of the url into the file.
    """
    fpIn = urllib_request.urlopen(url)
    fpOut = open(fname, 'wb')
    block = fpIn.read(10240)
    try:
        while block:
            fpOut.write(block)
            block = fpIn.read(10240)
        fpIn.close()
        fpOut.close()
    except:
        try:
            os.unlink(fname)
        except OSError:
            pass

def verifyThirdPartyFile(url, checksum, fname):
    """
    Download file from url to filename fname if it does not already exist.
    Abort if file contents does not match supplied md5 checksum.
    """
    name = os.path.basename(fname)
    if os.path.exists(fname):
        print("Using local copy of %s"%(name,))
    else:
        print("Did not find local copy of %s"%(name,))
        print("Downloading %s"%(name,))
        downloadURL(url, fname)
        print("Archive for %s stored as %s"%(name, fname))
    if len(checksum) == 32:
        algo = 'md5'
    elif len(checksum) == 64:
        algo = 'sha256'
    else:
        raise ValueError(checksum)
    if os.system(
            'CHECKSUM=$(openssl %s %s) ; test "${CHECKSUM##*= }" = "%s"'
                % (algo, shellQuote(fname), checksum) ):
        fatal('%s checksum mismatch for file %s' % (algo, fname))

def build_universal_openssl(basedir, archList):
    """
    Special case build recipe for universal build of openssl.

    The upstream OpenSSL build system does not directly support
    OS X universal builds.  We need to build each architecture
    separately then lipo them together into fat libraries.
    """

    # OpenSSL fails to build with Xcode 2.5 (on OS X 10.4).
    # If we are building on a 10.4.x or earlier system,
    # unilaterally disable assembly code building to avoid the problem.
    no_asm = int(platform.release().split(".")[0]) < 9

    def build_openssl_arch(archbase, arch):
        "Build one architecture of openssl"
        arch_opts = {
            "i386": ["darwin-i386-cc"],
            "x86_64": ["darwin64-x86_64-cc", "enable-ec_nistp_64_gcc_128"],
            "arm64": ["darwin64-arm64-cc"],
            "ppc": ["darwin-ppc-cc"],
            "ppc64": ["darwin64-ppc-cc"],
        }

        # Somewhere between OpenSSL 1.1.0j and 1.1.1c, changes cause the
        # "enable-ec_nistp_64_gcc_128" option to get compile errors when
        # building on our 10.6 gcc-4.2 environment.  There have been other
        # reports of projects running into this when using older compilers.
        # So, for now, do not try to use "enable-ec_nistp_64_gcc_128" when
        # building for 10.6.
        if getDeptargetTuple() == (10, 6):
            arch_opts['x86_64'].remove('enable-ec_nistp_64_gcc_128')

        configure_opts = [
            "no-idea",
            "no-mdc2",
            "no-rc5",
            "no-zlib",
            "no-ssl3",
            # "enable-unit-test",
            "shared",
            "--prefix=%s"%os.path.join("/", *FW_VERSION_PREFIX),
            "--openssldir=%s"%os.path.join("/", *FW_SSL_DIRECTORY),
        ]
        if no_asm:
            configure_opts.append("no-asm")
        runCommand(" ".join(["perl", "Configure"]
                        + arch_opts[arch] + configure_opts))
        runCommand("make depend")
        runCommand("make all")
        runCommand("make install_sw DESTDIR=%s"%shellQuote(archbase))
        # runCommand("make test")
        return

    srcdir = os.getcwd()
    universalbase = os.path.join(srcdir, "..",
                        os.path.basename(srcdir) + "-universal")
    os.mkdir(universalbase)
    archbasefws = []
    for arch in archList:
        # fresh copy of the source tree
        archsrc = os.path.join(universalbase, arch, "src")
        shutil.copytree(srcdir, archsrc, symlinks=True)
        # install base for this arch
        archbase = os.path.join(universalbase, arch, "root")
        os.mkdir(archbase)
        # Python framework base within install_prefix:
        # the build will install into this framework..
        # This is to ensure that the resulting shared libs have
        # the desired real install paths built into them.
        archbasefw = os.path.join(archbase, *FW_VERSION_PREFIX)

        # build one architecture
        os.chdir(archsrc)
        build_openssl_arch(archbase, arch)
        os.chdir(srcdir)
        archbasefws.append(archbasefw)

    # copy arch-independent files from last build into the basedir framework
    basefw = os.path.join(basedir, *FW_VERSION_PREFIX)
    shutil.copytree(
            os.path.join(archbasefw, "include", "openssl"),
            os.path.join(basefw, "include", "openssl")
            )

    shlib_version_number = grepValue(os.path.join(archsrc, "Makefile"),
            "SHLIB_VERSION_NUMBER")
    #   e.g. -> "1.0.0"
    libcrypto = "libcrypto.dylib"
    libcrypto_versioned = libcrypto.replace(".", "."+shlib_version_number+".")
    #   e.g. -> "libcrypto.1.0.0.dylib"
    libssl = "libssl.dylib"
    libssl_versioned = libssl.replace(".", "."+shlib_version_number+".")
    #   e.g. -> "libssl.1.0.0.dylib"

    try:
        os.mkdir(os.path.join(basefw, "lib"))
    except OSError:
        pass

    # merge the individual arch-dependent shared libs into a fat shared lib
    archbasefws.insert(0, basefw)
    for (lib_unversioned, lib_versioned) in [
                (libcrypto, libcrypto_versioned),
                (libssl, libssl_versioned)
            ]:
        runCommand("lipo -create -output " +
                    " ".join(shellQuote(
                            os.path.join(fw, "lib", lib_versioned))
                                    for fw in archbasefws))
        # and create an unversioned symlink of it
        os.symlink(lib_versioned, os.path.join(basefw, "lib", lib_unversioned))

    # Create links in the temp include and lib dirs that will be injected
    # into the Python build so that setup.py can find them while building
    # and the versioned links so that the setup.py post-build import test
    # does not fail.
    relative_path = os.path.join("..", "..", "..", *FW_VERSION_PREFIX)
    for fn in [
            ["include", "openssl"],
            ["lib", libcrypto],
            ["lib", libssl],
            ["lib", libcrypto_versioned],
            ["lib", libssl_versioned],
        ]:
        os.symlink(
            os.path.join(relative_path, *fn),
            os.path.join(basedir, "usr", "local", *fn)
        )

    return

def buildRecipe(recipe, basedir, archList):
    """
    Build software using a recipe. This function does the
    'configure;make;make install' dance for C software, with a possibility
    to customize this process, basically a poor-mans DarwinPorts.
    """
    curdir = os.getcwd()

    name = recipe['name']
    THIRD_PARTY_LIBS.append(name)
    url = recipe['url']
    configure = recipe.get('configure', './configure')
    buildrecipe = recipe.get('buildrecipe', None)
    install = recipe.get('install', 'make && make install DESTDIR=%s'%(
        shellQuote(basedir)))

    archiveName = os.path.split(url)[-1]
    sourceArchive = os.path.join(DEPSRC, archiveName)

    if not os.path.exists(DEPSRC):
        os.mkdir(DEPSRC)

    verifyThirdPartyFile(url, recipe['checksum'], sourceArchive)
    print("Extracting archive for %s"%(name,))
    buildDir=os.path.join(WORKDIR, '_bld')
    if not os.path.exists(buildDir):
        os.mkdir(buildDir)

    workDir = extractArchive(buildDir, sourceArchive)
    os.chdir(workDir)

    for patch in recipe.get('patches', ()):
        if isinstance(patch, tuple):
            url, checksum = patch
            fn = os.path.join(DEPSRC, os.path.basename(url))
            verifyThirdPartyFile(url, checksum, fn)
        else:
            # patch is a file in the source directory
            fn = os.path.join(curdir, patch)
        runCommand('patch -p%s < %s'%(recipe.get('patchlevel', 1),
            shellQuote(fn),))

    for patchscript in recipe.get('patchscripts', ()):
        if isinstance(patchscript, tuple):
            url, checksum = patchscript
            fn = os.path.join(DEPSRC, os.path.basename(url))
            verifyThirdPartyFile(url, checksum, fn)
        else:
            # patch is a file in the source directory
            fn = os.path.join(curdir, patchscript)
        if fn.endswith('.bz2'):
            runCommand('bunzip2 -fk %s' % shellQuote(fn))
            fn = fn[:-4]
        runCommand('sh %s' % shellQuote(fn))
        os.unlink(fn)

    if 'buildDir' in recipe:
        os.chdir(recipe['buildDir'])

    if configure is not None:
        configure_args = [
            "--prefix=/usr/local",
            "--enable-static",
            "--disable-shared",
            #"CPP=gcc -arch %s -E"%(' -arch '.join(archList,),),
        ]

        if 'configure_pre' in recipe:
            args = list(recipe['configure_pre'])
            if '--disable-static' in args:
                configure_args.remove('--enable-static')
            if '--enable-shared' in args:
                configure_args.remove('--disable-shared')
            configure_args.extend(args)

        if recipe.get('useLDFlags', 1):
            configure_args.extend([
                "CFLAGS=%s-mmacosx-version-min=%s -arch %s "
                            "-I%s/usr/local/include"%(
                        recipe.get('extra_cflags', ''),
                        DEPTARGET,
                        ' -arch '.join(archList),
                        shellQuote(basedir)[1:-1],),
                "LDFLAGS=-mmacosx-version-min=%s -L%s/usr/local/lib -arch %s"%(
                    DEPTARGET,
                    shellQuote(basedir)[1:-1],
                    ' -arch '.join(archList)),
            ])
        else:
            configure_args.extend([
                "CFLAGS=%s-mmacosx-version-min=%s -arch %s "
                            "-I%s/usr/local/include"%(
                        recipe.get('extra_cflags', ''),
                        DEPTARGET,
                        ' -arch '.join(archList),
                        shellQuote(basedir)[1:-1],),
            ])

        if 'configure_post' in recipe:
            configure_args = configure_args + list(recipe['configure_post'])

        configure_args.insert(0, configure)
        configure_args = [ shellQuote(a) for a in configure_args ]

        print("Running configure for %s"%(name,))
        runCommand(' '.join(configure_args) + ' 2>&1')

    if buildrecipe is not None:
        # call special-case build recipe, e.g. for openssl
        buildrecipe(basedir, archList)

    if install is not None:
        print("Running install for %s"%(name,))
        runCommand('{ ' + install + ' ;} 2>&1')

    print("Done %s"%(name,))
    print("")

    os.chdir(curdir)

def buildLibraries():
    """
    Build our dependencies into $WORKDIR/libraries/usr/local
    """
    print("")
    print("Building required libraries")
    print("")
    universal = os.path.join(WORKDIR, 'libraries')
    os.mkdir(universal)
    os.makedirs(os.path.join(universal, 'usr', 'local', 'lib'))
    os.makedirs(os.path.join(universal, 'usr', 'local', 'include'))

    for recipe in library_recipes():
        buildRecipe(recipe, universal, ARCHLIST)



def buildPythonDocs():
    # This stores the documentation as Resources/English.lproj/Documentation
    # inside the framework. pydoc and IDLE will pick it up there.
    print("Install python documentation")
    rootDir = os.path.join(WORKDIR, '_root')
    buildDir = os.path.join('../../Doc')
    docdir = os.path.join(rootDir, 'pydocs')
    curDir = os.getcwd()
    os.chdir(buildDir)
    runCommand('make clean')

    # Search third-party source directory for a pre-built version of the docs.
    #   Use the naming convention of the docs.python.org html downloads:
    #       python-3.9.0b1-docs-html.tar.bz2
    doctarfiles = [ f for f in os.listdir(DEPSRC)
        if f.startswith('python-'+getFullVersion())
        if f.endswith('-docs-html.tar.bz2') ]
    if doctarfiles:
        doctarfile = doctarfiles[0]
        if not os.path.exists('build'):
            os.mkdir('build')
        # if build directory existed, it was emptied by make clean, above
        os.chdir('build')
        # Extract the first archive found for this version into build
        runCommand('tar xjf %s'%shellQuote(os.path.join(DEPSRC, doctarfile)))
        # see if tar extracted a directory ending in -docs-html
        archivefiles = [ f for f in os.listdir('.')
            if f.endswith('-docs-html')
            if os.path.isdir(f) ]
        if archivefiles:
            archivefile = archivefiles[0]
            # make it our 'Docs/build/html' directory
            print(' -- using pre-built python documentation from %s'%archivefile)
            os.rename(archivefile, 'html')
        os.chdir(buildDir)

    htmlDir = os.path.join('build', 'html')
    if not os.path.exists(htmlDir):
        # Create virtual environment for docs builds with blurb and sphinx
        runCommand('make venv')
        runCommand('make html PYTHON=venv/bin/python')
    os.rename(htmlDir, docdir)
    os.chdir(curDir)


def buildPython():
    print("Building a universal python for %s architectures" % UNIVERSALARCHS)

    buildDir = os.path.join(WORKDIR, '_bld', 'python')
    rootDir = os.path.join(WORKDIR, '_root')

    if os.path.exists(buildDir):
        shutil.rmtree(buildDir)
    if os.path.exists(rootDir):
        shutil.rmtree(rootDir)
    os.makedirs(buildDir)
    os.makedirs(rootDir)
    os.makedirs(os.path.join(rootDir, 'empty-dir'))
    curdir = os.getcwd()
    os.chdir(buildDir)

    # Extract the version from the configure file, needed to calculate
    # several paths.
    version = getVersion()

    # Since the extra libs are not in their installed framework location
    # during the build, augment the library path so that the interpreter
    # will find them during its extension import sanity checks.

    print("Running configure...")
    print(" NOTE: --with-mimalloc=no pending resolution of weak linking issues")
    runCommand("%s -C --enable-framework --enable-universalsdk=/ "
               "--with-mimalloc=no "
               "--with-system-libmpdec "
               "--with-universal-archs=%s "
               "%s "
               "%s "
               "%s "
               "%s "
               "%s "
               "%s "
               "LDFLAGS='-g -L%s/libraries/usr/local/lib' "
               "CFLAGS='-g -I%s/libraries/usr/local/include' 2>&1"%(
        shellQuote(os.path.join(SRCDIR, 'configure')),
        UNIVERSALARCHS,
        (' ', '--with-computed-gotos ')[PYTHON_3],
        (' ', '--without-ensurepip ')[PYTHON_3],
        (' ', "--with-openssl='%s/libraries/usr/local'"%(
                            shellQuote(WORKDIR)[1:-1],))[PYTHON_3],
        (' ', "--enable-optimizations --with-lto")[compilerCanOptimize()],
        (' ', "TCLTK_CFLAGS='-I%s/libraries/usr/local/include'"%(
                            shellQuote(WORKDIR)[1:-1],))[internalTk()],
        (' ', "TCLTK_LIBS='-L%s/libraries/usr/local/lib -ltcl8.6 -ltk8.6'"%(
                            shellQuote(WORKDIR)[1:-1],))[internalTk()],
        shellQuote(WORKDIR)[1:-1],
        shellQuote(WORKDIR)[1:-1]))

    # As of macOS 10.11 with SYSTEM INTEGRITY PROTECTION, DYLD_*
    # environment variables are no longer automatically inherited
    # by child processes from their parents. We used to just set
    # DYLD_LIBRARY_PATH, pointing to the third-party libs,
    # in build-installer.py's process environment and it was
    # passed through the make utility into the environment of
    # setup.py. Instead, we now append DYLD_LIBRARY_PATH to
    # the existing RUNSHARED configuration value when we call
    # make for extension module builds.

    runshared_for_make = "".join([
            " RUNSHARED=",
            "'",
            grepValue("Makefile", "RUNSHARED"),
            ' DYLD_LIBRARY_PATH=',
            os.path.join(WORKDIR, 'libraries', 'usr', 'local', 'lib'),
            "'" ])

    # Look for environment value BUILDINSTALLER_BUILDPYTHON_MAKE_EXTRAS
    # and, if defined, append its value to the make command.  This allows
    # us to pass in version control tags, like GITTAG, to a build from a
    # tarball rather than from a vcs checkout, thus eliminating the need
    # to have a working copy of the vcs program on the build machine.
    #
    # A typical use might be:
    #      export BUILDINSTALLER_BUILDPYTHON_MAKE_EXTRAS=" \
    #                         GITVERSION='echo 123456789a' \
    #                         GITTAG='echo v3.6.0' \
    #                         GITBRANCH='echo 3.6'"

    make_extras = os.getenv("BUILDINSTALLER_BUILDPYTHON_MAKE_EXTRAS")
    if make_extras:
        make_cmd = "make " + make_extras + runshared_for_make
    else:
        make_cmd = "make" + runshared_for_make
    print("Running " + make_cmd)
    runCommand(make_cmd)

    make_cmd = "make install DESTDIR=%s %s"%(
        shellQuote(rootDir),
        runshared_for_make)
    print("Running " + make_cmd)
    runCommand(make_cmd)

    make_cmd = "make frameworkinstallextras DESTDIR=%s %s"%(
        shellQuote(rootDir),
        runshared_for_make)
    print("Running " + make_cmd)
    runCommand(make_cmd)

    print("Copying required shared libraries")
    if os.path.exists(os.path.join(WORKDIR, 'libraries', 'Library')):
        build_lib_dir = os.path.join(
                WORKDIR, 'libraries', 'Library', 'Frameworks',
                'Python.framework', 'Versions', getVersion(), 'lib')
        fw_lib_dir = os.path.join(
                WORKDIR, '_root', 'Library', 'Frameworks',
                'Python.framework', 'Versions', getVersion(), 'lib')
        if internalTk():
            # move Tcl and Tk pkgconfig files
            runCommand("mv %s/pkgconfig/* %s/pkgconfig"%(
                        shellQuote(build_lib_dir),
                        shellQuote(fw_lib_dir) ))
            runCommand("rm -r %s/pkgconfig"%(
                        shellQuote(build_lib_dir), ))
        runCommand("mv %s/* %s"%(
                    shellQuote(build_lib_dir),
                    shellQuote(fw_lib_dir) ))

    frmDir = os.path.join(rootDir, 'Library', 'Frameworks', 'Python.framework')
    frmDirVersioned = os.path.join(frmDir, 'Versions', version)
    path_to_lib = os.path.join(frmDirVersioned, 'lib', 'python%s'%(version,))
    # create directory for OpenSSL certificates
    sslDir = os.path.join(frmDirVersioned, 'etc', 'openssl')
    os.makedirs(sslDir)

    print("Fix file modes")
    gid = grp.getgrnam('admin').gr_gid

    shared_lib_error = False
    for dirpath, dirnames, filenames in os.walk(frmDir):
        for dn in dirnames:
            os.chmod(os.path.join(dirpath, dn), STAT_0o775)
            os.chown(os.path.join(dirpath, dn), -1, gid)

        for fn in filenames:
            if os.path.islink(fn):
                continue

            # "chmod g+w $fn"
            p = os.path.join(dirpath, fn)
            st = os.stat(p)
            os.chmod(p, stat.S_IMODE(st.st_mode) | stat.S_IWGRP)
            os.chown(p, -1, gid)

            if fn in EXPECTED_SHARED_LIBS:
                # check to see that this file was linked with the
                # expected library path and version
                data = captureCommand("otool -L %s" % shellQuote(p))
                for sl in EXPECTED_SHARED_LIBS[fn]:
                    if ("\t%s " % sl) not in data:
                        print("Expected shared lib %s was not linked with %s"
                                % (sl, p))
                        shared_lib_error = True

    if shared_lib_error:
        fatal("Unexpected shared library errors.")

    if PYTHON_3:
        LDVERSION=None
        VERSION=None
        ABIFLAGS=None

        fp = open(os.path.join(buildDir, 'Makefile'), 'r')
        for ln in fp:
            if ln.startswith('VERSION='):
                VERSION=ln.split()[1]
            if ln.startswith('ABIFLAGS='):
                ABIFLAGS=ln.split()
                ABIFLAGS=ABIFLAGS[1] if len(ABIFLAGS) > 1 else ''
            if ln.startswith('LDVERSION='):
                LDVERSION=ln.split()[1]
        fp.close()

        LDVERSION = LDVERSION.replace('$(VERSION)', VERSION)
        LDVERSION = LDVERSION.replace('$(ABIFLAGS)', ABIFLAGS)
        config_suffix = '-' + LDVERSION
        if getVersionMajorMinor() >= (3, 6):
            config_suffix = config_suffix + '-darwin'
    else:
        config_suffix = ''      # Python 2.x

    # We added some directories to the search path during the configure
    # phase. Remove those because those directories won't be there on
    # the end-users system. Also remove the directories from _sysconfigdata.py
    # (added in 3.3) if it exists.

    include_path = '-I%s/libraries/usr/local/include' % (WORKDIR,)
    lib_path = '-L%s/libraries/usr/local/lib' % (WORKDIR,)

    # fix Makefile
    path = os.path.join(path_to_lib, 'config' + config_suffix, 'Makefile')
    fp = open(path, 'r')
    data = fp.read()
    fp.close()

    for p in (include_path, lib_path):
        data = data.replace(" " + p, '')
        data = data.replace(p + " ", '')

    fp = open(path, 'w')
    fp.write(data)
    fp.close()

    # fix _sysconfigdata
    #
    # TODO: make this more robust!  test_sysconfig_module of
    # distutils.tests.test_sysconfig.SysconfigTestCase tests that
    # the output from get_config_var in both sysconfig and
    # distutils.sysconfig is exactly the same for both CFLAGS and
    # LDFLAGS.  The fixing up is now complicated by the pretty
    # printing in _sysconfigdata.py.  Also, we are using the
    # pprint from the Python running the installer build which
    # may not cosmetically format the same as the pprint in the Python
    # being built (and which is used to originally generate
    # _sysconfigdata.py).

    import pprint
    if getVersionMajorMinor() >= (3, 6):
        # XXX this is extra-fragile
        path = os.path.join(path_to_lib,
            '_sysconfigdata_%s_darwin_darwin.py' % (ABIFLAGS,))
    else:
        path = os.path.join(path_to_lib, '_sysconfigdata.py')
    fp = open(path, 'r')
    data = fp.read()
    fp.close()
    # create build_time_vars dict
    if RUNNING_ON_PYTHON2:
        exec(data)
    else:
        g_dict = {}
        l_dict = {}
        exec(data, g_dict, l_dict)
        build_time_vars = l_dict['build_time_vars']
    vars = {}
    for k, v in build_time_vars.items():
        if isinstance(v, str):
            for p in (include_path, lib_path):
                v = v.replace(' ' + p, '')
                v = v.replace(p + ' ', '')
        vars[k] = v

    fp = open(path, 'w')
    # duplicated from sysconfig._generate_posix_vars()
    fp.write('# system configuration generated and used by'
                ' the sysconfig module\n')
    fp.write('build_time_vars = ')
    pprint.pprint(vars, stream=fp)
    fp.close()

    # Add symlinks in /usr/local/bin, using relative links
    usr_local_bin = os.path.join(rootDir, 'usr', 'local', 'bin')
    to_framework = os.path.join('..', '..', '..', 'Library', 'Frameworks',
            'Python.framework', 'Versions', version, 'bin')
    if os.path.exists(usr_local_bin):
        shutil.rmtree(usr_local_bin)
    os.makedirs(usr_local_bin)
    for fn in os.listdir(
                os.path.join(frmDir, 'Versions', version, 'bin')):
        os.symlink(os.path.join(to_framework, fn),
                   os.path.join(usr_local_bin, fn))

    os.chdir(curdir)

def patchFile(inPath, outPath):
    data = fileContents(inPath)
    data = data.replace('$FULL_VERSION', getFullVersion())
    data = data.replace('$VERSION', getVersion())
    data = data.replace('$MACOSX_DEPLOYMENT_TARGET', ''.join((DEPTARGET, ' or later')))
    data = data.replace('$ARCHITECTURES', ", ".join(universal_opts_map[UNIVERSALARCHS]))
    data = data.replace('$INSTALL_SIZE', installSize())
    data = data.replace('$THIRD_PARTY_LIBS', "\\\n".join(THIRD_PARTY_LIBS))

    # This one is not handy as a template variable
    data = data.replace('$PYTHONFRAMEWORKINSTALLDIR', '/Library/Frameworks/Python.framework')
    fp = open(outPath, 'w')
    fp.write(data)
    fp.close()

def patchScript(inPath, outPath):
    major, minor = getVersionMajorMinor()
    data = fileContents(inPath)
    data = data.replace('@PYMAJOR@', str(major))
    data = data.replace('@PYVER@', getVersion())
    fp = open(outPath, 'w')
    fp.write(data)
    fp.close()
    os.chmod(outPath, STAT_0o755)



def packageFromRecipe(targetDir, recipe):
    curdir = os.getcwd()
    try:
        # The major version (such as 2.5) is included in the package name
        # because having two version of python installed at the same time is
        # common.
        pkgname = '%s-%s'%(recipe['name'], getVersion())
        srcdir  = recipe.get('source')
        pkgroot = recipe.get('topdir', srcdir)
        postflight = recipe.get('postflight')
        readme = textwrap.dedent(recipe['readme'])
        isRequired = recipe.get('required', True)

        print("- building package %s"%(pkgname,))

        # Substitute some variables
        textvars = dict(
            VER=getVersion(),
            FULLVER=getFullVersion(),
        )
        readme = readme % textvars

        if pkgroot is not None:
            pkgroot = pkgroot % textvars
        else:
            pkgroot = '/'

        if srcdir is not None:
            srcdir = os.path.join(WORKDIR, '_root', srcdir[1:])
            srcdir = srcdir % textvars

        if postflight is not None:
            postflight = os.path.abspath(postflight)

        packageContents = os.path.join(targetDir, pkgname + '.pkg', 'Contents')
        os.makedirs(packageContents)

        if srcdir is not None:
            os.chdir(srcdir)
            runCommand("pax -wf %s . 2>&1"%(shellQuote(os.path.join(packageContents, 'Archive.pax')),))
            runCommand("gzip -9 %s 2>&1"%(shellQuote(os.path.join(packageContents, 'Archive.pax')),))
            runCommand("mkbom . %s 2>&1"%(shellQuote(os.path.join(packageContents, 'Archive.bom')),))

        fn = os.path.join(packageContents, 'PkgInfo')
        fp = open(fn, 'w')
        fp.write('pmkrpkg1')
        fp.close()

        rsrcDir = os.path.join(packageContents, "Resources")
        os.mkdir(rsrcDir)
        fp = open(os.path.join(rsrcDir, 'ReadMe.txt'), 'w')
        fp.write(readme)
        fp.close()

        if postflight is not None:
            patchScript(postflight, os.path.join(rsrcDir, 'postflight'))

        vers = getFullVersion()
        major, minor = getVersionMajorMinor()
        pl = dict(
                CFBundleGetInfoString="Python.%s %s"%(pkgname, vers,),
                CFBundleIdentifier='org.python.Python.%s'%(pkgname,),
                CFBundleName='Python.%s'%(pkgname,),
                CFBundleShortVersionString=vers,
                IFMajorVersion=major,
                IFMinorVersion=minor,
                IFPkgFormatVersion=0.10000000149011612,
                IFPkgFlagAllowBackRev=False,
                IFPkgFlagAuthorizationAction="RootAuthorization",
                IFPkgFlagDefaultLocation=pkgroot,
                IFPkgFlagFollowLinks=True,
                IFPkgFlagInstallFat=True,
                IFPkgFlagIsRequired=isRequired,
                IFPkgFlagOverwritePermissions=False,
                IFPkgFlagRelocatable=False,
                IFPkgFlagRestartAction="NoRestart",
                IFPkgFlagRootVolumeOnly=True,
                IFPkgFlagUpdateInstalledLanguages=False,
            )
        writePlist(pl, os.path.join(packageContents, 'Info.plist'))

        pl = dict(
                    IFPkgDescriptionDescription=readme,
                    IFPkgDescriptionTitle=recipe.get('long_name', "Python.%s"%(pkgname,)),
                    IFPkgDescriptionVersion=vers,
                )
        writePlist(pl, os.path.join(packageContents, 'Resources', 'Description.plist'))

    finally:
        os.chdir(curdir)


def makeMpkgPlist(path):

    vers = getFullVersion()
    major, minor = getVersionMajorMinor()

    pl = dict(
            CFBundleGetInfoString="Python %s"%(vers,),
            CFBundleIdentifier='org.python.Python',
            CFBundleName='Python',
            CFBundleShortVersionString=vers,
            IFMajorVersion=major,
            IFMinorVersion=minor,
            IFPkgFlagComponentDirectory="Contents/Packages",
            IFPkgFlagPackageList=[
                dict(
                    IFPkgFlagPackageLocation='%s-%s.pkg'%(item['name'], getVersion()),
                    IFPkgFlagPackageSelection=item.get('selected', 'selected'),
                )
                for item in pkg_recipes()
            ],
            IFPkgFormatVersion=0.10000000149011612,
            IFPkgFlagBackgroundScaling="proportional",
            IFPkgFlagBackgroundAlignment="left",
            IFPkgFlagAuthorizationAction="RootAuthorization",
        )

    writePlist(pl, path)


def buildInstaller():

    # Zap all compiled files
    for dirpath, _, filenames in os.walk(os.path.join(WORKDIR, '_root')):
        for fn in filenames:
            if fn.endswith('.pyc') or fn.endswith('.pyo'):
                os.unlink(os.path.join(dirpath, fn))

    outdir = os.path.join(WORKDIR, 'installer')
    if os.path.exists(outdir):
        shutil.rmtree(outdir)
    os.mkdir(outdir)

    pkgroot = os.path.join(outdir, 'Python.mpkg', 'Contents')
    pkgcontents = os.path.join(pkgroot, 'Packages')
    os.makedirs(pkgcontents)
    for recipe in pkg_recipes():
        packageFromRecipe(pkgcontents, recipe)

    rsrcDir = os.path.join(pkgroot, 'Resources')

    fn = os.path.join(pkgroot, 'PkgInfo')
    fp = open(fn, 'w')
    fp.write('pmkrpkg1')
    fp.close()

    os.mkdir(rsrcDir)

    makeMpkgPlist(os.path.join(pkgroot, 'Info.plist'))
    pl = dict(
                IFPkgDescriptionTitle="Python",
                IFPkgDescriptionVersion=getVersion(),
            )

    writePlist(pl, os.path.join(pkgroot, 'Resources', 'Description.plist'))
    for fn in os.listdir('resources'):
        if fn == '.svn': continue
        if fn.endswith('.jpg'):
            shutil.copy(os.path.join('resources', fn), os.path.join(rsrcDir, fn))
        else:
            patchFile(os.path.join('resources', fn), os.path.join(rsrcDir, fn))


def installSize(clear=False, _saved=[]):
    if clear:
        del _saved[:]
    if not _saved:
        data = captureCommand("du -ks %s"%(
                    shellQuote(os.path.join(WORKDIR, '_root'))))
        _saved.append("%d"%((0.5 + (int(data.split()[0]) / 1024.0)),))
    return _saved[0]


def buildDMG():
    """
    Create DMG containing the rootDir.
    """
    outdir = os.path.join(WORKDIR, 'diskimage')
    if os.path.exists(outdir):
        shutil.rmtree(outdir)

    # We used to use the deployment target as the last characters of the
    # installer file name. With the introduction of weaklinked installer
    # variants, we may have two variants with the same file name, i.e.
    # both ending in '10.9'.  To avoid this, we now use the major/minor
    # version numbers of the macOS version we are building on.
    # Also, as of macOS 11, operating system version numbering has
    # changed from three components to two, i.e.
    #   10.14.1, 10.14.2, ...
    #   10.15.1, 10.15.2, ...
    #   11.1, 11.2, ...
    #   12.1, 12.2, ...
    # (A further twist is that, when running on macOS 11, binaries built
    # on older systems may be shown an operating system version of 10.16
    # instead of 11.  We should not run into that situation here.)
    # Also we should use "macos" instead of "macosx" going forward.
    #
    # To maintain compatibility for legacy variants, the file name for
    # builds on macOS 10.15 and earlier remains:
    #   python-3.x.y-macosx10.z.{dmg->pkg}
    #   e.g. python-3.9.4-macosx10.9.{dmg->pkg}
    # and for builds on macOS 11+:
    #   python-3.x.y-macosz.{dmg->pkg}
    #   e.g. python-3.9.4-macos11.{dmg->pkg}

    build_tuple = getBuildTuple()
    if build_tuple[0] < 11:
        os_name = 'macosx'
        build_system_version = '%s.%s' % build_tuple
    else:
        os_name = 'macos'
        build_system_version = str(build_tuple[0])
    imagepath = os.path.join(outdir,
                    'python-%s-%s%s'%(getFullVersion(),os_name,build_system_version))
    if INCLUDE_TIMESTAMP:
        imagepath = imagepath + '-%04d-%02d-%02d'%(time.localtime()[:3])
    imagepath = imagepath + '.dmg'

    os.mkdir(outdir)

    # Try to mitigate race condition in certain versions of macOS, e.g. 10.9,
    # when hdiutil create fails with  "Resource busy".  For now, just retry
    # the create a few times and hope that it eventually works.

    volname='Python %s'%(getFullVersion())
    cmd = ("hdiutil create -format UDRW -volname %s -srcfolder %s -size 100m %s"%(
            shellQuote(volname),
            shellQuote(os.path.join(WORKDIR, 'installer')),
            shellQuote(imagepath + ".tmp.dmg" )))
    for i in range(5):
        fd = os.popen(cmd, 'r')
        data = fd.read()
        xit = fd.close()
        if not xit:
            break
        sys.stdout.write(data)
        print(" -- retrying hdiutil create")
        time.sleep(5)
    else:
        raise RuntimeError("command failed: %s"%(cmd,))

    if not os.path.exists(os.path.join(WORKDIR, "mnt")):
        os.mkdir(os.path.join(WORKDIR, "mnt"))
    runCommand("hdiutil attach %s -mountroot %s"%(
        shellQuote(imagepath + ".tmp.dmg"), shellQuote(os.path.join(WORKDIR, "mnt"))))

    # Custom icon for the DMG, shown when the DMG is mounted.
    shutil.copy("../Icons/Disk Image.icns",
            os.path.join(WORKDIR, "mnt", volname, ".VolumeIcon.icns"))
    runCommand("SetFile -a C %s/"%(
            shellQuote(os.path.join(WORKDIR, "mnt", volname)),))

    runCommand("hdiutil detach %s"%(shellQuote(os.path.join(WORKDIR, "mnt", volname))))

    setIcon(imagepath + ".tmp.dmg", "../Icons/Disk Image.icns")
    runCommand("hdiutil convert %s -format UDZO -o %s"%(
            shellQuote(imagepath + ".tmp.dmg"), shellQuote(imagepath)))
    setIcon(imagepath, "../Icons/Disk Image.icns")

    os.unlink(imagepath + ".tmp.dmg")

    return imagepath


def setIcon(filePath, icnsPath):
    """
    Set the custom icon for the specified file or directory.
    """

    dirPath = os.path.normpath(os.path.dirname(__file__))
    toolPath = os.path.join(dirPath, "seticon.app/Contents/MacOS/seticon")
    if not os.path.exists(toolPath) or os.stat(toolPath).st_mtime < os.stat(dirPath + '/seticon.m').st_mtime:
        # NOTE: The tool is created inside an .app bundle, otherwise it won't work due
        # to connections to the window server.
        appPath = os.path.join(dirPath, "seticon.app/Contents/MacOS")
        if not os.path.exists(appPath):
            os.makedirs(appPath)
        runCommand("cc -o %s %s/seticon.m -framework Cocoa"%(
            shellQuote(toolPath), shellQuote(dirPath)))

    runCommand("%s %s %s"%(shellQuote(os.path.abspath(toolPath)), shellQuote(icnsPath),
        shellQuote(filePath)))

def main():
    # First parse options and check if we can perform our work
    parseOptions()
    checkEnvironment()

    os.environ['MACOSX_DEPLOYMENT_TARGET'] = DEPTARGET
    os.environ['CC'] = CC
    os.environ['CXX'] = CXX

    if os.path.exists(WORKDIR):
        shutil.rmtree(WORKDIR)
    os.mkdir(WORKDIR)

    os.environ['LC_ALL'] = 'C'

    # Then build third-party libraries such as sleepycat DB4.
    buildLibraries()

    # Now build python itself
    buildPython()

    # And then build the documentation
    # Remove the Deployment Target from the shell
    # environment, it's no longer needed and
    # an unexpected build target can cause problems
    # when Sphinx and its dependencies need to
    # be (re-)installed.
    del os.environ['MACOSX_DEPLOYMENT_TARGET']
    buildPythonDocs()


    # Prepare the applications folder
    folder = os.path.join(WORKDIR, "_root", "Applications", "Python %s"%(
        getVersion(),))
    fn = os.path.join(folder, "License.rtf")
    patchFile("resources/License.rtf",  fn)
    fn = os.path.join(folder, "ReadMe.rtf")
    patchFile("resources/ReadMe.rtf",  fn)
    fn = os.path.join(folder, "Update Shell Profile.command")
    patchScript("resources/update_shell_profile.command",  fn)
    fn = os.path.join(folder, "Install Certificates.command")
    patchScript("resources/install_certificates.command",  fn)
    os.chmod(folder, STAT_0o755)
    setIcon(folder, "../Icons/Python Folder.icns")

    # Create the installer
    buildInstaller()

    # And copy the readme into the directory containing the installer
    patchFile('resources/ReadMe.rtf',
                os.path.join(WORKDIR, 'installer', 'ReadMe.rtf'))

    # Ditto for the license file.
    patchFile('resources/License.rtf',
                os.path.join(WORKDIR, 'installer', 'License.rtf'))

    fp = open(os.path.join(WORKDIR, 'installer', 'Build.txt'), 'w')
    fp.write("# BUILD INFO\n")
    fp.write("# Date: %s\n" % time.ctime())
    fp.write("# By: %s\n" % pwd.getpwuid(os.getuid()).pw_gecos)
    fp.close()

    # And copy it to a DMG
    buildDMG()

if __name__ == "__main__":
    main()
