#!/usr/bin/env python
"""
This script is used to build "official" universal installers on Mac OS X.
It requires at least Mac OS X 10.5, Xcode 3, and the 10.4u SDK for
32-bit builds.  64-bit or four-way universal builds require at least
OS X 10.5 and the 10.5 SDK.

Please ensure that this script keeps working with Python 2.5, to avoid
bootstrap issues (/usr/bin/python is Python 2.5 on OSX 10.5).  Sphinx,
which is used to build the documentation, currently requires at least
Python 2.4.

In addition to what is supplied with OS X 10.5+ and Xcode 3+, the script
requires an installed version of hg and a third-party version of
Tcl/Tk 8.4 (for OS X 10.4 and 10.5 deployment targets) or Tcl/TK 8.5
(for 10.6 or later) installed in /Library/Frameworks.  When installed,
the Python built by this script will attempt to dynamically link first to
Tcl and Tk frameworks in /Library/Frameworks if available otherwise fall
back to the ones in /System/Library/Framework.  For the build, we recommend
installing the most recent ActiveTcl 8.4 or 8.5 version.

32-bit-only installer builds are still possible on OS X 10.4 with Xcode 2.5
and the installation of additional components, such as a newer Python
(2.5 is needed for Python parser updates), hg, and svn (for the documentation
build).

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

from plistlib import Plist

try:
    from plistlib import writePlist
except ImportError:
    # We're run using python2.3
    def writePlist(plist, path):
        plist.write(path)

def shellQuote(value):
    """
    Return the string value in a form that can safely be inserted into
    a shell command.
    """
    return "'%s'"%(value.replace("'", "'\"'\"'"))

def grepValue(fn, variable):
    variable = variable + '='
    for ln in open(fn, 'r'):
        if ln.startswith(variable):
            value = ln[len(variable):].strip()
            return value[1:-1]
    raise RuntimeError("Cannot find variable %s" % variable[:-1])

_cache_getVersion = None

def getVersion():
    global _cache_getVersion
    if _cache_getVersion is None:
        _cache_getVersion = grepValue(
            os.path.join(SRCDIR, 'configure'), 'PACKAGE_VERSION')
    return _cache_getVersion

def getVersionTuple():
    return tuple([int(n) for n in getVersion().split('.')])

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

# The directory we'll use to create the build (will be erased and recreated)
WORKDIR = "/tmp/_py"

# The directory we'll use to store third-party sources. Set this to something
# else if you don't want to re-fetch required libraries every time.
DEPSRC = os.path.join(WORKDIR, 'third-party')
DEPSRC = os.path.expanduser('~/Universal/other-sources')

# Location of the preferred SDK

### There are some issues with the SDK selection below here,
### The resulting binary doesn't work on all platforms that
### it should. Always default to the 10.4u SDK until that
### isue is resolved.
###
##if int(os.uname()[2].split('.')[0]) == 8:
##    # Explicitly use the 10.4u (universal) SDK when
##    # building on 10.4, the system headers are not
##    # useable for a universal build
##    SDKPATH = "/Developer/SDKs/MacOSX10.4u.sdk"
##else:
##    SDKPATH = "/"

SDKPATH = "/Developer/SDKs/MacOSX10.4u.sdk"

universal_opts_map = { '32-bit': ('i386', 'ppc',),
                       '64-bit': ('x86_64', 'ppc64',),
                       'intel':  ('i386', 'x86_64'),
                       '3-way':  ('ppc', 'i386', 'x86_64'),
                       'all':    ('i386', 'ppc', 'x86_64', 'ppc64',) }
default_target_map = {
        '64-bit': '10.5',
        '3-way': '10.5',
        'intel': '10.5',
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
DEPTARGET = '10.3'

target_cc_map = {
        '10.3': ('gcc-4.0', 'g++-4.0'),
        '10.4': ('gcc-4.0', 'g++-4.0'),
        '10.5': ('gcc-4.2', 'g++-4.2'),
        '10.6': ('gcc-4.2', 'g++-4.2'),
        '10.7': ('clang', 'clang++'),
        '10.8': ('clang', 'clang++'),
}

CC, CXX = target_cc_map[DEPTARGET]

PYTHON_3 = getVersionTuple() >= (3, 0)

USAGE = textwrap.dedent("""\
    Usage: build_python [options]

    Options:
    -? or -h:            Show this message
    -b DIR
    --build-dir=DIR:     Create build here (default: %(WORKDIR)r)
    --third-party=DIR:   Store third-party sources here (default: %(DEPSRC)r)
    --sdk-path=DIR:      Location of the SDK (default: %(SDKPATH)r)
    --src-dir=DIR:       Location of the Python sources (default: %(SRCDIR)r)
    --dep-target=10.n    OS X deployment target (default: %(DEPTARGET)r)
    --universal-archs=x  universal architectures (options: %(UNIVERSALOPTS)r, default: %(UNIVERSALARCHS)r)
""")% globals()

# Dict of object file names with shared library names to check after building.
# This is to ensure that we ended up dynamically linking with the shared
# library paths and versions we expected.  For example:
#   EXPECTED_SHARED_LIBS['_tkinter.so'] = [
#                       '/Library/Frameworks/Tcl.framework/Versions/8.5/Tcl',
#                       '/Library/Frameworks/Tk.framework/Versions/8.5/Tk']
EXPECTED_SHARED_LIBS = {}

# Instructions for building libraries that are necessary for building a
# batteries included python.
#   [The recipes are defined here for convenience but instantiated later after
#    command line options have been processed.]
def library_recipes():
    result = []

    LT_10_5 = bool(DEPTARGET < '10.5')

    if getVersionTuple() >= (3, 3):
        result.extend([
          dict(
              name="XZ 5.0.3",
              url="http://tukaani.org/xz/xz-5.0.3.tar.gz",
              checksum='fefe52f9ecd521de2a8ce38c21a27574',
              configure_pre=[
                    '--disable-dependency-tracking',
              ]
              ),
        ])

    result.extend([
          dict(
              name="NCurses 5.9",
              url="http://ftp.gnu.org/pub/gnu/ncurses/ncurses-5.9.tar.gz",
              checksum='8cb9c412e5f2d96bc6f459aa8c6282a1',
              configure_pre=[
                  "--enable-widec",
                  "--without-cxx",
                  "--without-cxx-binding",
                  "--without-ada",
                  "--without-curses-h",
                  "--enable-shared",
                  "--with-shared",
                  "--without-debug",
                  "--without-normal",
                  "--without-tests",
                  "--without-manpages",
                  "--datadir=/usr/share",
                  "--sysconfdir=/etc",
                  "--sharedstatedir=/usr/com",
                  "--with-terminfo-dirs=/usr/share/terminfo",
                  "--with-default-terminfo-dir=/usr/share/terminfo",
                  "--libdir=/Library/Frameworks/Python.framework/Versions/%s/lib"%(getVersion(),),
              ],
              patchscripts=[
                  ("ftp://invisible-island.net/ncurses//5.9/ncurses-5.9-20120616-patch.sh.bz2",
                   "f54bf02a349f96a7c4f0d00922f3a0d4"),
                   ],
              useLDFlags=False,
              install='make && make install DESTDIR=%s && cd %s/usr/local/lib && ln -fs ../../../Library/Frameworks/Python.framework/Versions/%s/lib/lib* .'%(
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  getVersion(),
                  ),
          ),
          dict(
              name="SQLite 3.7.13",
              url="http://www.sqlite.org/sqlite-autoconf-3071300.tar.gz",
              checksum='c97df403e8a3d5b67bb408fcd6aabd8e',
              extra_cflags=('-Os '
                            '-DSQLITE_ENABLE_FTS4 '
                            '-DSQLITE_ENABLE_FTS3_PARENTHESIS '
                            '-DSQLITE_ENABLE_RTREE '
                            '-DSQLITE_TCL=0 '
                 '%s' % ('','-DSQLITE_WITHOUT_ZONEMALLOC ')[LT_10_5]),
              configure_pre=[
                  '--enable-threadsafe',
                  '--enable-shared=no',
                  '--enable-static=yes',
                  '--disable-readline',
                  '--disable-dependency-tracking',
              ]
          ),
        ])

    if DEPTARGET < '10.5':
        result.extend([
          dict(
              name="Bzip2 1.0.6",
              url="http://bzip.org/1.0.6/bzip2-1.0.6.tar.gz",
              checksum='00b516f4704d4a7cb50a1d97e6e8e15b',
              configure=None,
              install='make install CC=%s CXX=%s, PREFIX=%s/usr/local/ CFLAGS="-arch %s -isysroot %s"'%(
                  CC, CXX,
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  ' -arch '.join(ARCHLIST),
                  SDKPATH,
              ),
          ),
          dict(
              name="ZLib 1.2.3",
              url="http://www.gzip.org/zlib/zlib-1.2.3.tar.gz",
              checksum='debc62758716a169df9f62e6ab2bc634',
              configure=None,
              install='make install CC=%s CXX=%s, prefix=%s/usr/local/ CFLAGS="-arch %s -isysroot %s"'%(
                  CC, CXX,
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  ' -arch '.join(ARCHLIST),
                  SDKPATH,
              ),
          ),
          dict(
              # Note that GNU readline is GPL'd software
              name="GNU Readline 6.1.2",
              url="http://ftp.gnu.org/pub/gnu/readline/readline-6.1.tar.gz" ,
              checksum='fc2f7e714fe792db1ce6ddc4c9fb4ef3',
              patchlevel='0',
              patches=[
                  # The readline maintainers don't do actual micro releases, but
                  # just ship a set of patches.
                  ('http://ftp.gnu.org/pub/gnu/readline/readline-6.1-patches/readline61-001',
                   'c642f2e84d820884b0bf9fd176bc6c3f'),
                  ('http://ftp.gnu.org/pub/gnu/readline/readline-6.1-patches/readline61-002',
                   '1a76781a1ea734e831588285db7ec9b1'),
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
                interpreter and the standard library. This also includes Python
                wrappers for lots of Mac OS X API's.
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
                that is useable for pydoc and IDLE.
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
            selected=unselected_for_python3,
        ),
    ]

    if DEPTARGET < '10.4' and not PYTHON_3:
        result.append(
            dict(
                name="PythonSystemFixes",
                long_name="Fix system Python",
                readme="""\
                    This package updates the system python installation on
                    Mac OS X 10.3 to ensure that you can build new python extensions
                    using that copy of python after installing this version.
                    """,
                postflight="../Tools/fixapplepython23.py",
                topdir="/Library/Frameworks/Python.framework",
                source="/empty-dir",
                required=False,
                selected=unselected_for_python3,
            )
        )
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
    except:
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

    if sys.version_info[0:2] < (2, 4):
        fatal("This script must be run with Python 2.4 or later")

    if platform.system() != 'Darwin':
        fatal("This script should be run on a Mac OS X 10.4 (or later) system")

    if int(platform.release().split('.')[0]) < 8:
        fatal("This script should be run on a Mac OS X 10.4 (or later) system")

    if not os.path.exists(SDKPATH):
        fatal("Please install the latest version of Xcode and the %s SDK"%(
            os.path.basename(SDKPATH[:-4])))

    # Because we only support dynamic load of only one major/minor version of
    # Tcl/Tk, ensure:
    # 1. there are no user-installed frameworks of Tcl/Tk with version
    #       higher than the Apple-supplied system version in
    #       SDKROOT/System/Library/Frameworks
    # 2. there is a user-installed framework (usually ActiveTcl) in (or linked
    #       in) SDKROOT/Library/Frameworks with the same version as the system
    #       version. This allows users to choose to install a newer patch level.

    frameworks = {}
    for framework in ['Tcl', 'Tk']:
        fwpth = 'Library/Frameworks/%s.framework/Versions/Current' % framework
        sysfw = os.path.join(SDKPATH, 'System', fwpth)
        libfw = os.path.join(SDKPATH, fwpth)
        usrfw = os.path.join(os.getenv('HOME'), fwpth)
        frameworks[framework] = os.readlink(sysfw)
        if not os.path.exists(libfw):
            fatal("Please install a link to a current %s %s as %s so "
                    "the user can override the system framework."
                    % (framework, frameworks[framework], libfw))
        if os.readlink(libfw) != os.readlink(sysfw):
            fatal("Version of %s must match %s" % (libfw, sysfw) )
        if os.path.exists(usrfw):
            fatal("Please rename %s to avoid possible dynamic load issues."
                    % usrfw)

    if frameworks['Tcl'] != frameworks['Tk']:
        fatal("The Tcl and Tk frameworks are not the same version.")

    # add files to check after build
    EXPECTED_SHARED_LIBS['_tkinter.so'] = [
            "/Library/Frameworks/Tcl.framework/Versions/%s/Tcl"
                % frameworks['Tcl'],
            "/Library/Frameworks/Tk.framework/Versions/%s/Tk"
                % frameworks['Tk'],
            ]

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
    global WORKDIR, DEPSRC, SDKPATH, SRCDIR, DEPTARGET
    global UNIVERSALOPTS, UNIVERSALARCHS, ARCHLIST, CC, CXX

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
            SDKPATH=v

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
                    DEPTARGET = default_target_map.get(v, '10.3')
            else:
                raise NotImplementedError(v)

        else:
            raise NotImplementedError(k)

    SRCDIR=os.path.abspath(SRCDIR)
    WORKDIR=os.path.abspath(WORKDIR)
    SDKPATH=os.path.abspath(SDKPATH)
    DEPSRC=os.path.abspath(DEPSRC)

    CC, CXX=target_cc_map[DEPTARGET]

    print("Settings:")
    print(" * Source directory:", SRCDIR)
    print(" * Build directory: ", WORKDIR)
    print(" * SDK location:    ", SDKPATH)
    print(" * Third-party source:", DEPSRC)
    print(" * Deployment target:", DEPTARGET)
    print(" * Universal architectures:", ARCHLIST)
    print(" * C compiler:", CC)
    print(" * C++ compiler:", CXX)
    print("")




def extractArchive(builddir, archiveName):
    """
    Extract a source archive into 'builddir'. Returns the path of the
    extracted archive.

    XXX: This function assumes that archives contain a toplevel directory
    that is has the same name as the basename of the archive. This is
    save enough for anything we use.
    """
    curdir = os.getcwd()
    try:
        os.chdir(builddir)
        if archiveName.endswith('.tar.gz'):
            retval = os.path.basename(archiveName[:-7])
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
        except:
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
    if os.system(
            'MD5=$(openssl md5 %s) ; test "${MD5##*= }" = "%s"'
                % (shellQuote(fname), checksum) ):
        fatal('MD5 checksum mismatch for file %s' % fname)

def buildRecipe(recipe, basedir, archList):
    """
    Build software using a recipe. This function does the
    'configure;make;make install' dance for C software, with a possibility
    to customize this process, basically a poor-mans DarwinPorts.
    """
    curdir = os.getcwd()

    name = recipe['name']
    url = recipe['url']
    configure = recipe.get('configure', './configure')
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
    if 'buildDir' in recipe:
        os.chdir(recipe['buildDir'])

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
                "CFLAGS=%s-mmacosx-version-min=%s -arch %s -isysroot %s "
                            "-I%s/usr/local/include"%(
                        recipe.get('extra_cflags', ''),
                        DEPTARGET,
                        ' -arch '.join(archList),
                        shellQuote(SDKPATH)[1:-1],
                        shellQuote(basedir)[1:-1],),
                "LDFLAGS=-mmacosx-version-min=%s -syslibroot,%s -L%s/usr/local/lib -arch %s"%(
                    DEPTARGET,
                    shellQuote(SDKPATH)[1:-1],
                    shellQuote(basedir)[1:-1],
                    ' -arch '.join(archList)),
            ])
        else:
            configure_args.extend([
                "CFLAGS=%s-mmacosx-version-min=%s -arch %s -isysroot %s "
                            "-I%s/usr/local/include"%(
                        recipe.get('extra_cflags', ''),
                        DEPTARGET,
                        ' -arch '.join(archList),
                        shellQuote(SDKPATH)[1:-1],
                        shellQuote(basedir)[1:-1],),
            ])

        if 'configure_post' in recipe:
            configure_args = configure_args + list(recipe['configure_post'])

        configure_args.insert(0, configure)
        configure_args = [ shellQuote(a) for a in configure_args ]

        print("Running configure for %s"%(name,))
        runCommand(' '.join(configure_args) + ' 2>&1')

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
    # inside the framwork. pydoc and IDLE will pick it up there.
    print("Install python documentation")
    rootDir = os.path.join(WORKDIR, '_root')
    buildDir = os.path.join('../../Doc')
    docdir = os.path.join(rootDir, 'pydocs')
    curDir = os.getcwd()
    os.chdir(buildDir)
    runCommand('make update')
    runCommand("make html PYTHON='%s'" % os.path.abspath(sys.executable))
    os.chdir(curDir)
    if not os.path.exists(docdir):
        os.mkdir(docdir)
    os.rename(os.path.join(buildDir, 'build', 'html'), docdir)


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

    # Not sure if this is still needed, the original build script
    # claims that parts of the install assume python.exe exists.
    os.symlink('python', os.path.join(buildDir, 'python.exe'))

    # Extract the version from the configure file, needed to calculate
    # several paths.
    version = getVersion()

    # Since the extra libs are not in their installed framework location
    # during the build, augment the library path so that the interpreter
    # will find them during its extension import sanity checks.
    os.environ['DYLD_LIBRARY_PATH'] = os.path.join(WORKDIR,
                                        'libraries', 'usr', 'local', 'lib')
    print("Running configure...")
    runCommand("%s -C --enable-framework --enable-universalsdk=%s "
               "--with-universal-archs=%s "
               "%s "
               "LDFLAGS='-g -L%s/libraries/usr/local/lib' "
               "OPT='-g -O3 -I%s/libraries/usr/local/include' 2>&1"%(
        shellQuote(os.path.join(SRCDIR, 'configure')), shellQuote(SDKPATH),
        UNIVERSALARCHS,
        (' ', '--with-computed-gotos ')[PYTHON_3],
        shellQuote(WORKDIR)[1:-1],
        shellQuote(WORKDIR)[1:-1]))

    print("Running make")
    runCommand("make")

    print("Running make install")
    runCommand("make install DESTDIR=%s"%(
        shellQuote(rootDir)))

    print("Running make frameworkinstallextras")
    runCommand("make frameworkinstallextras DESTDIR=%s"%(
        shellQuote(rootDir)))

    del os.environ['DYLD_LIBRARY_PATH']
    print("Copying required shared libraries")
    if os.path.exists(os.path.join(WORKDIR, 'libraries', 'Library')):
        runCommand("mv %s/* %s"%(
            shellQuote(os.path.join(
                WORKDIR, 'libraries', 'Library', 'Frameworks',
                'Python.framework', 'Versions', getVersion(),
                'lib')),
            shellQuote(os.path.join(WORKDIR, '_root', 'Library', 'Frameworks',
                'Python.framework', 'Versions', getVersion(),
                'lib'))))

    print("Fix file modes")
    frmDir = os.path.join(rootDir, 'Library', 'Frameworks', 'Python.framework')
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
                ABIFLAGS=ln.split()[1]
            if ln.startswith('LDVERSION='):
                LDVERSION=ln.split()[1]
        fp.close()

        LDVERSION = LDVERSION.replace('$(VERSION)', VERSION)
        LDVERSION = LDVERSION.replace('$(ABIFLAGS)', ABIFLAGS)
        config_suffix = '-' + LDVERSION
    else:
        config_suffix = ''      # Python 2.x

    # We added some directories to the search path during the configure
    # phase. Remove those because those directories won't be there on
    # the end-users system. Also remove the directories from _sysconfigdata.py
    # (added in 3.3) if it exists.

    path_to_lib = os.path.join(rootDir, 'Library', 'Frameworks',
                                'Python.framework', 'Versions',
                                version, 'lib', 'python%s'%(version,))
    paths = [os.path.join(path_to_lib, 'config' + config_suffix, 'Makefile'),
             os.path.join(path_to_lib, '_sysconfigdata.py')]
    for path in paths:
        if not os.path.exists(path):
            continue
        fp = open(path, 'r')
        data = fp.read()
        fp.close()

        data = data.replace(' -L%s/libraries/usr/local/lib'%(WORKDIR,), '')
        data = data.replace(' -I%s/libraries/usr/local/include'%(WORKDIR,), '')
        fp = open(path, 'w')
        fp.write(data)
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

    if PYTHON_3:
        # Remove the 'Current' link, that way we don't accidently mess
        # with an already installed version of python 2
        os.unlink(os.path.join(rootDir, 'Library', 'Frameworks',
                            'Python.framework', 'Versions', 'Current'))

def patchFile(inPath, outPath):
    data = fileContents(inPath)
    data = data.replace('$FULL_VERSION', getFullVersion())
    data = data.replace('$VERSION', getVersion())
    data = data.replace('$MACOSX_DEPLOYMENT_TARGET', ''.join((DEPTARGET, ' or later')))
    data = data.replace('$ARCHITECTURES', ", ".join(universal_opts_map[UNIVERSALARCHS]))
    data = data.replace('$INSTALL_SIZE', installSize())

    # This one is not handy as a template variable
    data = data.replace('$PYTHONFRAMEWORKINSTALLDIR', '/Library/Frameworks/Python.framework')
    fp = open(outPath, 'w')
    fp.write(data)
    fp.close()

def patchScript(inPath, outPath):
    data = fileContents(inPath)
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
        pl = Plist(
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
                IFPkgFlagUpdateInstalledLangauges=False,
            )
        writePlist(pl, os.path.join(packageContents, 'Info.plist'))

        pl = Plist(
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

    pl = Plist(
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
    pl = Plist(
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

    shutil.copy("../../LICENSE", os.path.join(rsrcDir, 'License.txt'))


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

    imagepath = os.path.join(outdir,
                    'python-%s-macosx%s'%(getFullVersion(),DEPTARGET))
    if INCLUDE_TIMESTAMP:
        imagepath = imagepath + '-%04d-%02d-%02d'%(time.localtime()[:3])
    imagepath = imagepath + '.dmg'

    os.mkdir(outdir)
    volname='Python %s'%(getFullVersion())
    runCommand("hdiutil create -format UDRW -volname %s -srcfolder %s %s"%(
            shellQuote(volname),
            shellQuote(os.path.join(WORKDIR, 'installer')),
            shellQuote(imagepath + ".tmp.dmg" )))


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
    fn = os.path.join(WORKDIR, "_root", "Applications",
                "Python %s"%(getVersion(),), "Update Shell Profile.command")
    patchScript("scripts/postflight.patch-profile",  fn)

    folder = os.path.join(WORKDIR, "_root", "Applications", "Python %s"%(
        getVersion(),))
    os.chmod(folder, STAT_0o755)
    setIcon(folder, "../Icons/Python Folder.icns")

    # Create the installer
    buildInstaller()

    # And copy the readme into the directory containing the installer
    patchFile('resources/ReadMe.txt', os.path.join(WORKDIR, 'installer', 'ReadMe.txt'))

    # Ditto for the license file.
    shutil.copy('../../LICENSE', os.path.join(WORKDIR, 'installer', 'License.txt'))

    fp = open(os.path.join(WORKDIR, 'installer', 'Build.txt'), 'w')
    fp.write("# BUILD INFO\n")
    fp.write("# Date: %s\n" % time.ctime())
    fp.write("# By: %s\n" % pwd.getpwuid(os.getuid()).pw_gecos)
    fp.close()

    # And copy it to a DMG
    buildDMG()

if __name__ == "__main__":
    main()
