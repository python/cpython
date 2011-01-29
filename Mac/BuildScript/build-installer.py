#!/usr/bin/python
"""
This script is used to build "official" universal installers on Mac OS X.
It requires at least Mac OS X 10.4, Xcode 2.2 and the 10.4u SDK for
32-bit builds.  64-bit or four-way universal builds require at least
OS X 10.5 and the 10.5 SDK.

Please ensure that this script keeps working with Python 2.5, to avoid
bootstrap issues (/usr/bin/python is Python 2.5 on OSX 10.5).  Sphinx,
which is used to build the documentation, currently requires at least
Python 2.4.

Usage: see USAGE variable in the script.
"""
import platform, os, sys, getopt, textwrap, shutil, urllib2, stat, time, pwd
import grp

INCLUDE_TIMESTAMP = 1
VERBOSE = 1

from plistlib import Plist

import MacOS

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
    raise RuntimeError, "Cannot find variable %s" % variable[:-1]

def getVersion():
    return grepValue(os.path.join(SRCDIR, 'configure'), 'PACKAGE_VERSION')

def getVersionTuple():
    return tuple([int(n) for n in getVersion().split('.')])

def getFullVersion():
    fn = os.path.join(SRCDIR, 'Include', 'patchlevel.h')
    for ln in open(fn):
        if 'PY_VERSION' in ln:
            return ln.split()[-1][1:-1]
    raise RuntimeError, "Cannot find full version??"

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

# Source directory (asume we're in Mac/BuildScript)
SRCDIR = os.path.dirname(
        os.path.dirname(
            os.path.dirname(
                os.path.abspath(__file__
        ))))

# $MACOSX_DEPLOYMENT_TARGET -> minimum OS X level
DEPTARGET = '10.3'

target_cc_map = {
        '10.3': 'gcc-4.0',
        '10.4': 'gcc-4.0',
        '10.5': 'gcc-4.0',
        '10.6': 'gcc-4.2',
}

CC = target_cc_map[DEPTARGET]

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


# Instructions for building libraries that are necessary for building a
# batteries included python.
#   [The recipes are defined here for convenience but instantiated later after
#    command line options have been processed.]
def library_recipes():
    result = []

    if DEPTARGET < '10.5':
        result.extend([
          dict(
              name="Bzip2 1.0.6",
              url="http://bzip.org/1.0.6/bzip2-1.0.6.tar.gz",
              checksum='00b516f4704d4a7cb50a1d97e6e8e15b',
              configure=None,
              install='make install CC=%s PREFIX=%s/usr/local/ CFLAGS="-arch %s -isysroot %s"'%(
                  CC,
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
              install='make install CC=%s prefix=%s/usr/local/ CFLAGS="-arch %s -isysroot %s"'%(
                  CC,
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
                  'http://ftp.gnu.org/pub/gnu/readline/readline-6.1-patches/readline61-001',
                  'http://ftp.gnu.org/pub/gnu/readline/readline-6.1-patches/readline61-002',
              ]
          ),
          dict(
              name="SQLite 3.7.4",
              url="http://www.sqlite.org/sqlite-autoconf-3070400.tar.gz",
              checksum='8f0c690bfb33c3cbbc2471c3d9ba0158',
              configure_env=('CFLAGS="-Os'
                                  ' -DSQLITE_ENABLE_FTS3'
                                  ' -DSQLITE_ENABLE_FTS3_PARENTHESIS'
                                  ' -DSQLITE_ENABLE_RTREE'
                                  ' -DSQLITE_TCL=0'
                                  '"'),
              configure_pre=[
                  '--enable-threadsafe',
                  '--enable-shared=no',
                  '--enable-static=yes',
                  '--disable-readline',
                  '--disable-dependency-tracking',
              ]
          ),
          dict(
              name="NCurses 5.5",
              url="http://ftp.gnu.org/pub/gnu/ncurses/ncurses-5.5.tar.gz",
              checksum='e73c1ac10b4bfc46db43b2ddfd6244ef',
              configure_pre=[
                  "--enable-widec",
                  "--without-cxx",
                  "--without-ada",
                  "--without-progs",
                  "--without-curses-h",
                  "--enable-shared",
                  "--with-shared",
                  "--datadir=/usr/share",
                  "--sysconfdir=/etc",
                  "--sharedstatedir=/usr/com",
                  "--with-terminfo-dirs=/usr/share/terminfo",
                  "--with-default-terminfo-dir=/usr/share/terminfo",
                  "--libdir=/Library/Frameworks/Python.framework/Versions/%s/lib"%(getVersion(),),
                  "--enable-termcap",
              ],
              patches=[
                  "ncurses-5.5.patch",
              ],
              useLDFlags=False,
              install='make && make install DESTDIR=%s && cd %s/usr/local/lib && ln -fs ../../../Library/Frameworks/Python.framework/Versions/%s/lib/lib* .'%(
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  shellQuote(os.path.join(WORKDIR, 'libraries')),
                  getVersion(),
                  ),
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
                that is useable for pydoc and IDLE. If you have installed Xcode
                it will also install a link to the documentation in
                /Developer/Documentation/Python
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

    if DEPTARGET < '10.4':
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
    return open(fn, 'rb').read()

def runCommand(commandline):
    """
    Run a command and raise RuntimeError if it fails. Output is surpressed
    unless the command fails.
    """
    fd = os.popen(commandline, 'r')
    data = fd.read()
    xit = fd.close()
    if xit is not None:
        sys.stdout.write(data)
        raise RuntimeError, "command failed: %s"%(commandline,)

    if VERBOSE:
        sys.stdout.write(data); sys.stdout.flush()

def captureCommand(commandline):
    fd = os.popen(commandline, 'r')
    data = fd.read()
    xit = fd.close()
    if xit is not None:
        sys.stdout.write(data)
        raise RuntimeError, "command failed: %s"%(commandline,)

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
    #       higher than the Apple-supplied system version
    # 2. there is a user-installed framework in /Library/Frameworks with the
    #       same version as the system version.  This allows users to choose
    #       to install a newer patch level.

    for framework in ['Tcl', 'Tk']:
        #fw = dict(lower=framework.lower(),
        #            upper=framework.upper(),
        #            cap=framework.capitalize())
        #fwpth = "Library/Frameworks/%(cap)s.framework/%(lower)sConfig.sh" % fw
        fwpth = 'Library/Frameworks/Tcl.framework/Versions/Current'
        sysfw = os.path.join(SDKPATH, 'System', fwpth)
        libfw = os.path.join('/', fwpth)
        usrfw = os.path.join(os.getenv('HOME'), fwpth)
        #version = "%(upper)s_VERSION" % fw
        if os.readlink(libfw) != os.readlink(sysfw):
            fatal("Version of %s must match %s" % (libfw, sysfw) )
        if os.path.exists(usrfw):
            fatal("Please rename %s to avoid possible dynamic load issues."
                    % usrfw)

    # Remove inherited environment variables which might influence build
    environ_var_prefixes = ['CPATH', 'C_INCLUDE_', 'DYLD_', 'LANG', 'LC_',
                            'LD_', 'LIBRARY_', 'PATH', 'PYTHON']
    for ev in list(os.environ):
        for prefix in environ_var_prefixes:
            if ev.startswith(prefix) :
                print "INFO: deleting environment variable %s=%s" % (
                                                    ev, os.environ[ev])
                del os.environ[ev]

    os.environ['PATH'] = '/bin:/sbin:/usr/bin:/usr/sbin'
    print "Setting default PATH: %s"%(os.environ['PATH'])


def parseOptions(args=None):
    """
    Parse arguments and update global settings.
    """
    global WORKDIR, DEPSRC, SDKPATH, SRCDIR, DEPTARGET
    global UNIVERSALOPTS, UNIVERSALARCHS, ARCHLIST, CC

    if args is None:
        args = sys.argv[1:]

    try:
        options, args = getopt.getopt(args, '?hb',
                [ 'build-dir=', 'third-party=', 'sdk-path=' , 'src-dir=',
                  'dep-target=', 'universal-archs=', 'help' ])
    except getopt.error, msg:
        print msg
        sys.exit(1)

    if args:
        print "Additional arguments"
        sys.exit(1)

    deptarget = None
    for k, v in options:
        if k in ('-h', '-?', '--help'):
            print USAGE
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
                raise NotImplementedError, v

        else:
            raise NotImplementedError, k

    SRCDIR=os.path.abspath(SRCDIR)
    WORKDIR=os.path.abspath(WORKDIR)
    SDKPATH=os.path.abspath(SDKPATH)
    DEPSRC=os.path.abspath(DEPSRC)

    CC=target_cc_map[DEPTARGET]

    print "Settings:"
    print " * Source directory:", SRCDIR
    print " * Build directory: ", WORKDIR
    print " * SDK location:    ", SDKPATH
    print " * Third-party source:", DEPSRC
    print " * Deployment target:", DEPTARGET
    print " * Universal architectures:", ARCHLIST
    print " * C compiler:", CC
    print ""




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
            raise RuntimeError, "Cannot extract %s"%(archiveName,)

        return os.path.join(builddir, retval)

    finally:
        os.chdir(curdir)

KNOWNSIZES = {
    "http://ftp.gnu.org/pub/gnu/readline/readline-5.1.tar.gz": 7952742,
    "http://downloads.sleepycat.com/db-4.4.20.tar.gz": 2030276,
}

def downloadURL(url, fname):
    """
    Download the contents of the url into the file.
    """
    try:
        size = os.path.getsize(fname)
    except OSError:
        pass
    else:
        if KNOWNSIZES.get(url) == size:
            print "Using existing file for", url
            return
    fpIn = urllib2.urlopen(url)
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


    if os.path.exists(sourceArchive):
        print "Using local copy of %s"%(name,)

    else:
        print "Did not find local copy of %s"%(name,)
        print "Downloading %s"%(name,)
        downloadURL(url, sourceArchive)
        print "Archive for %s stored as %s"%(name, sourceArchive)

    print "Extracting archive for %s"%(name,)
    buildDir=os.path.join(WORKDIR, '_bld')
    if not os.path.exists(buildDir):
        os.mkdir(buildDir)

    workDir = extractArchive(buildDir, sourceArchive)
    os.chdir(workDir)
    if 'buildDir' in recipe:
        os.chdir(recipe['buildDir'])


    for fn in recipe.get('patches', ()):
        if fn.startswith('http://'):
            # Download the patch before applying it.
            path = os.path.join(DEPSRC, os.path.basename(fn))
            downloadURL(fn, path)
            fn = path

        fn = os.path.join(curdir, fn)
        runCommand('patch -p%s < %s'%(recipe.get('patchlevel', 1),
            shellQuote(fn),))

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
                "CFLAGS=-arch %s -isysroot %s -I%s/usr/local/include"%(
                        ' -arch '.join(archList),
                        shellQuote(SDKPATH)[1:-1],
                        shellQuote(basedir)[1:-1],),
                "LDFLAGS=-syslibroot,%s -L%s/usr/local/lib -arch %s"%(
                    shellQuote(SDKPATH)[1:-1],
                    shellQuote(basedir)[1:-1],
                    ' -arch '.join(archList)),
            ])
        else:
            configure_args.extend([
                "CFLAGS=-arch %s -isysroot %s -I%s/usr/local/include"%(
                        ' -arch '.join(archList),
                        shellQuote(SDKPATH)[1:-1],
                        shellQuote(basedir)[1:-1],),
            ])

        if 'configure_post' in recipe:
            configure_args = configure_args = list(recipe['configure_post'])

        configure_args.insert(0, configure)
        configure_args = [ shellQuote(a) for a in configure_args ]

        if 'configure_env' in recipe:
            configure_args.insert(0, recipe['configure_env'])

        print "Running configure for %s"%(name,)
        runCommand(' '.join(configure_args) + ' 2>&1')

    print "Running install for %s"%(name,)
    runCommand('{ ' + install + ' ;} 2>&1')

    print "Done %s"%(name,)
    print ""

    os.chdir(curdir)

def buildLibraries():
    """
    Build our dependencies into $WORKDIR/libraries/usr/local
    """
    print ""
    print "Building required libraries"
    print ""
    universal = os.path.join(WORKDIR, 'libraries')
    os.mkdir(universal)
    os.makedirs(os.path.join(universal, 'usr', 'local', 'lib'))
    os.makedirs(os.path.join(universal, 'usr', 'local', 'include'))

    for recipe in library_recipes():
        buildRecipe(recipe, universal, ARCHLIST)



def buildPythonDocs():
    # This stores the documentation as Resources/English.lproj/Documentation
    # inside the framwork. pydoc and IDLE will pick it up there.
    print "Install python documentation"
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
    print "Building a universal python for %s architectures" % UNIVERSALARCHS

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
    print "Running configure..."
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

    print "Running make"
    runCommand("make")

    print "Running make install"
    runCommand("make install DESTDIR=%s"%(
        shellQuote(rootDir)))

    print "Running make frameworkinstallextras"
    runCommand("make frameworkinstallextras DESTDIR=%s"%(
        shellQuote(rootDir)))

    del os.environ['DYLD_LIBRARY_PATH']
    print "Copying required shared libraries"
    if os.path.exists(os.path.join(WORKDIR, 'libraries', 'Library')):
        runCommand("mv %s/* %s"%(
            shellQuote(os.path.join(
                WORKDIR, 'libraries', 'Library', 'Frameworks',
                'Python.framework', 'Versions', getVersion(),
                'lib')),
            shellQuote(os.path.join(WORKDIR, '_root', 'Library', 'Frameworks',
                'Python.framework', 'Versions', getVersion(),
                'lib'))))

    print "Fix file modes"
    frmDir = os.path.join(rootDir, 'Library', 'Frameworks', 'Python.framework')
    gid = grp.getgrnam('admin').gr_gid

    for dirpath, dirnames, filenames in os.walk(frmDir):
        for dn in dirnames:
            os.chmod(os.path.join(dirpath, dn), 0775)
            os.chown(os.path.join(dirpath, dn), -1, gid)


        for fn in filenames:
            if os.path.islink(fn):
                continue

            # "chmod g+w $fn"
            p = os.path.join(dirpath, fn)
            st = os.stat(p)
            os.chmod(p, stat.S_IMODE(st.st_mode) | stat.S_IWGRP)
            os.chown(p, -1, gid)

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
    # the end-users system.
    path =os.path.join(rootDir, 'Library', 'Frameworks', 'Python.framework',
                'Versions', version, 'lib', 'python%s'%(version,),
                'config' + config_suffix, 'Makefile')
    fp = open(path, 'r')
    data = fp.read()
    fp.close()

    data = data.replace('-L%s/libraries/usr/local/lib'%(WORKDIR,), '')
    data = data.replace('-I%s/libraries/usr/local/include'%(WORKDIR,), '')
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
    fp = open(outPath, 'wb')
    fp.write(data)
    fp.close()

def patchScript(inPath, outPath):
    data = fileContents(inPath)
    data = data.replace('@PYVER@', getVersion())
    fp = open(outPath, 'wb')
    fp.write(data)
    fp.close()
    os.chmod(outPath, 0755)



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

        print "- building package %s"%(pkgname,)

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
        major, minor = map(int, getVersion().split('.', 2))
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
    major, minor = map(int, getVersion().split('.', 2))

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
    runCommand("/Developer/Tools/SetFile -a C %s/"%(
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
    os.chmod(folder, 0755)
    setIcon(folder, "../Icons/Python Folder.icns")

    # Create the installer
    buildInstaller()

    # And copy the readme into the directory containing the installer
    patchFile('resources/ReadMe.txt', os.path.join(WORKDIR, 'installer', 'ReadMe.txt'))

    # Ditto for the license file.
    shutil.copy('../../LICENSE', os.path.join(WORKDIR, 'installer', 'License.txt'))

    fp = open(os.path.join(WORKDIR, 'installer', 'Build.txt'), 'w')
    print >> fp, "# BUILD INFO"
    print >> fp, "# Date:", time.ctime()
    print >> fp, "# By:", pwd.getpwuid(os.getuid()).pw_gecos
    fp.close()

    # And copy it to a DMG
    buildDMG()

if __name__ == "__main__":
    main()
