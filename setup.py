# Autodetecting setup.py script for building the Python extensions
#
# To be fixed:
#   Implement --disable-modules setting
#

__version__ = "$Revision$"

import sys, os, getopt
from distutils import sysconfig
from distutils import text_file
from distutils.errors import *
from distutils.core import Extension, setup
from distutils.command.build_ext import build_ext

# This global variable is used to hold the list of modules to be disabled.
disabled_module_list = []

def find_file(filename, std_dirs, paths):
    """Searches for the directory where a given file is located,
    and returns a possibly-empty list of additional directories, or None
    if the file couldn't be found at all.

    'filename' is the name of a file, such as readline.h or libcrypto.a.
    'std_dirs' is the list of standard system directories; if the
        file is found in one of them, no additional directives are needed.
    'paths' is a list of additional locations to check; if the file is
        found in one of them, the resulting list will contain the directory.
    """

    # Check the standard locations
    for dir in std_dirs:
        f = os.path.join(dir, filename)
        if os.path.exists(f): return []

    # Check the additional directories
    for dir in paths:
        f = os.path.join(dir, filename)
        if os.path.exists(f):
            return [dir]

    # Not found anywhere
    return None

def find_library_file(compiler, libname, std_dirs, paths):
    filename = compiler.library_filename(libname, lib_type='shared')
    result = find_file(filename, std_dirs, paths)
    if result is not None: return result

    filename = compiler.library_filename(libname, lib_type='static')
    result = find_file(filename, std_dirs, paths)
    return result

def module_enabled(extlist, modname):
    """Returns whether the module 'modname' is present in the list
    of extensions 'extlist'."""
    extlist = [ext for ext in extlist if ext.name == modname]
    return len(extlist)

class PyBuildExt(build_ext):

    def build_extensions(self):

        # Detect which modules should be compiled
        self.detect_modules()

        # Remove modules that are present on the disabled list
        self.extensions = [ext for ext in self.extensions
                           if ext.name not in disabled_module_list]

        # Fix up the autodetected modules, prefixing all the source files
        # with Modules/ and adding Python's include directory to the path.
        (srcdir,) = sysconfig.get_config_vars('srcdir')

        # Figure out the location of the source code for extension modules
        moddir = os.path.join(os.getcwd(), srcdir, 'Modules')
        moddir = os.path.normpath(moddir)
        srcdir, tail = os.path.split(moddir)
        srcdir = os.path.normpath(srcdir)
        moddir = os.path.normpath(moddir)

        # Fix up the paths for scripts, too
        self.distribution.scripts = [os.path.join(srcdir, filename)
                                     for filename in self.distribution.scripts]

        for ext in self.extensions[:]:
            ext.sources = [ os.path.join(moddir, filename)
                            for filename in ext.sources ]
            ext.include_dirs.append( '.' ) # to get config.h
            ext.include_dirs.append( os.path.join(srcdir, './Include') )

            # If a module has already been built statically,
            # don't build it here
            if ext.name in sys.builtin_module_names:
                self.extensions.remove(ext)

        # Parse Modules/Setup to figure out which modules are turned
        # on in the file. 
        input = text_file.TextFile('Modules/Setup', join_lines=1)
        remove_modules = []
        while 1:
            line = input.readline()
            if not line: break
            line = line.split()
            remove_modules.append( line[0] )
        input.close()
        
        for ext in self.extensions[:]:
            if ext.name in remove_modules:
                self.extensions.remove(ext)
        
        # When you run "make CC=altcc" or something similar, you really want
        # those environment variables passed into the setup.py phase.  Here's
        # a small set of useful ones.
        compiler = os.environ.get('CC')
        linker_so = os.environ.get('LDSHARED')
        args = {}
        # unfortunately, distutils doesn't let us provide separate C and C++
        # compilers
        if compiler is not None:
            (ccshared,) = sysconfig.get_config_vars('CCSHARED')
            args['compiler_so'] = compiler + ' ' + ccshared
        if linker_so is not None:
            args['linker_so'] = linker_so + ' -shared'
        self.compiler.set_executables(**args)

        build_ext.build_extensions(self)

    def build_extension(self, ext):

        try:
            build_ext.build_extension(self, ext)
        except (CCompilerError, DistutilsError), why:
            self.announce('WARNING: building of extension "%s" failed: %s' %
                          (ext.name, sys.exc_info()[1]))

    def get_platform (self):
        # Get value of sys.platform
        platform = sys.platform
        if platform[:6] =='cygwin':
            platform = 'cygwin'
        elif platform[:4] =='beos':
            platform = 'beos'

        return platform

    def detect_modules(self):
        # Ensure that /usr/local is always used
        if '/usr/local/lib' not in self.compiler.library_dirs:
            self.compiler.library_dirs.insert(0, '/usr/local/lib')
        if '/usr/local/include' not in self.compiler.include_dirs:
            self.compiler.include_dirs.insert(0, '/usr/local/include' )

        # lib_dirs and inc_dirs are used to search for files;
        # if a file is found in one of those directories, it can
        # be assumed that no additional -I,-L directives are needed.
        lib_dirs = self.compiler.library_dirs + ['/lib', '/usr/lib']
        inc_dirs = self.compiler.include_dirs + ['/usr/include'] 
        exts = []

        platform = self.get_platform()
        
        # Check for MacOS X, which doesn't need libm.a at all
        math_libs = ['m']
        if platform in ['Darwin1.2', 'beos']:
            math_libs = []

        # XXX Omitted modules: gl, pure, dl, SGI-specific modules

        #
        # The following modules are all pretty straightforward, and compile
        # on pretty much any POSIXish platform.
        #

        # Some modules that are normally always on:
        exts.append( Extension('regex', ['regexmodule.c', 'regexpr.c']) )
        exts.append( Extension('pcre', ['pcremodule.c', 'pypcre.c']) )

        exts.append( Extension('_weakref', ['_weakref.c']) )
        exts.append( Extension('_symtable', ['symtablemodule.c']) )
        exts.append( Extension('xreadlines', ['xreadlinesmodule.c']) )

        # array objects
        exts.append( Extension('array', ['arraymodule.c']) )
        # complex math library functions
        exts.append( Extension('cmath', ['cmathmodule.c'],
                               libraries=math_libs) )

        # math library functions, e.g. sin()
        exts.append( Extension('math',  ['mathmodule.c'],
                               libraries=math_libs) )
        # fast string operations implemented in C
        exts.append( Extension('strop', ['stropmodule.c']) )
        # time operations and variables
        exts.append( Extension('time', ['timemodule.c'],
                               libraries=math_libs) )
        # operator.add() and similar goodies
        exts.append( Extension('operator', ['operator.c']) )
        # access to the builtin codecs and codec registry
        exts.append( Extension('_codecs', ['_codecsmodule.c']) )
        # Python C API test module
        exts.append( Extension('_testcapi', ['_testcapimodule.c']) )
        # static Unicode character database
        exts.append( Extension('unicodedata', ['unicodedata.c']) )
        # access to ISO C locale support
        exts.append( Extension('_locale', ['_localemodule.c']) )

        # Modules with some UNIX dependencies -- on by default:
        # (If you have a really backward UNIX, select and socket may not be
        # supported...)

        # fcntl(2) and ioctl(2)
        exts.append( Extension('fcntl', ['fcntlmodule.c']) )
        # pwd(3)
        exts.append( Extension('pwd', ['pwdmodule.c']) )
        # grp(3)
        exts.append( Extension('grp', ['grpmodule.c']) )
        # posix (UNIX) errno values
        exts.append( Extension('errno', ['errnomodule.c']) )
        # select(2); not on ancient System V
        exts.append( Extension('select', ['selectmodule.c']) )

        # The md5 module implements the RSA Data Security, Inc. MD5
        # Message-Digest Algorithm, described in RFC 1321.  The necessary files
        # md5c.c and md5.h are included here.
        exts.append( Extension('md5', ['md5module.c', 'md5c.c']) )

        # The sha module implements the SHA checksum algorithm.
        # (NIST's Secure Hash Algorithm.)
        exts.append( Extension('sha', ['shamodule.c']) )

        # Tommy Burnette's 'new' module (creates new empty objects of certain
        # kinds):
        exts.append( Extension('new', ['newmodule.c']) )

        # Helper module for various ascii-encoders
        exts.append( Extension('binascii', ['binascii.c']) )

        # Fred Drake's interface to the Python parser
        exts.append( Extension('parser', ['parsermodule.c']) )

        # Digital Creations' cStringIO and cPickle
        exts.append( Extension('cStringIO', ['cStringIO.c']) )
        exts.append( Extension('cPickle', ['cPickle.c']) )

        # Memory-mapped files (also works on Win32).
        exts.append( Extension('mmap', ['mmapmodule.c']) )

        # Lance Ellinghaus's modules:
        # enigma-inspired encryption
        exts.append( Extension('rotor', ['rotormodule.c']) )
        # syslog daemon interface
        exts.append( Extension('syslog', ['syslogmodule.c']) )

        # George Neville-Neil's timing module:
        exts.append( Extension('timing', ['timingmodule.c']) )

        #
        # Here ends the simple stuff.  From here on, modules need certain
        # libraries, are platform-specific, or present other surprises.
        #

        # Multimedia modules
        # These don't work for 64-bit platforms!!!
        # These represent audio samples or images as strings:

        # Disabled on 64-bit platforms
        if sys.maxint != 9223372036854775807L:
            # Operations on audio samples
            exts.append( Extension('audioop', ['audioop.c']) )
            # Operations on images
            exts.append( Extension('imageop', ['imageop.c']) )
            # Read SGI RGB image files (but coded portably)
            exts.append( Extension('rgbimg', ['rgbimgmodule.c']) )

        # readline
        if self.compiler.find_library_file(lib_dirs, 'readline'):
            readline_libs = ['readline']
            if self.compiler.find_library_file(lib_dirs +
                                               ['/usr/lib/termcap'],
                                               'termcap'):
                readline_libs.append('termcap')
            exts.append( Extension('readline', ['readline.c'],
                                   library_dirs=['/usr/lib/termcap'],
                                   libraries=readline_libs) )

        # The crypt module is now disabled by default because it breaks builds
        # on many systems (where -lcrypt is needed), e.g. Linux (I believe).

        if self.compiler.find_library_file(lib_dirs, 'crypt'):
            libs = ['crypt']
        else:
            libs = []
        exts.append( Extension('crypt', ['cryptmodule.c'], libraries=libs) )

        # socket(2)
        # Detect SSL support for the socket module
        ssl_incs = find_file('openssl/ssl.h', inc_dirs,
                             ['/usr/local/ssl/include',
                              '/usr/contrib/ssl/include/'
                             ]
                             )
        ssl_libs = find_library_file(self.compiler, 'ssl',lib_dirs,
                                     ['/usr/local/ssl/lib',
                                      '/usr/contrib/ssl/lib/'
                                     ] )

        if (ssl_incs is not None and
            ssl_libs is not None):
            exts.append( Extension('_socket', ['socketmodule.c'],
                                   include_dirs = ssl_incs,
                                   library_dirs = ssl_libs,
                                   libraries = ['ssl', 'crypto'],
                                   define_macros = [('USE_SSL',1)] ) )
        else:
            exts.append( Extension('_socket', ['socketmodule.c']) )

        # Modules that provide persistent dictionary-like semantics.  You will
        # probably want to arrange for at least one of them to be available on
        # your machine, though none are defined by default because of library
        # dependencies.  The Python module anydbm.py provides an
        # implementation independent wrapper for these; dumbdbm.py provides
        # similar functionality (but slower of course) implemented in Python.

        # The standard Unix dbm module:
        if platform not in ['cygwin']:
            if (self.compiler.find_library_file(lib_dirs, 'ndbm')):
                exts.append( Extension('dbm', ['dbmmodule.c'],
                                       libraries = ['ndbm'] ) )
            else:
                exts.append( Extension('dbm', ['dbmmodule.c']) )

        # Anthony Baxter's gdbm module.  GNU dbm(3) will require -lgdbm:
        if (self.compiler.find_library_file(lib_dirs, 'gdbm')):
            exts.append( Extension('gdbm', ['gdbmmodule.c'],
                                   libraries = ['gdbm'] ) )

        # Berkeley DB interface.
        #
        # This requires the Berkeley DB code, see
        # ftp://ftp.cs.berkeley.edu/pub/4bsd/db.1.85.tar.gz
        #
        # Edit the variables DB and DBPORT to point to the db top directory
        # and the subdirectory of PORT where you built it.
        #
        # (See http://electricrain.com/greg/python/bsddb3/ for an interface to
        # BSD DB 3.x.)

        dblib = []
        if self.compiler.find_library_file(lib_dirs, 'db'):
            dblib = ['db']
        
        db185_incs = find_file('db_185.h', inc_dirs,
                               ['/usr/include/db3', '/usr/include/db2'])
        db_inc = find_file('db.h', inc_dirs, ['/usr/include/db1'])
        if db185_incs is not None:
            exts.append( Extension('bsddb', ['bsddbmodule.c'],
                                   include_dirs = db185_incs,
                                   define_macros=[('HAVE_DB_185_H',1)],
                                   libraries = dblib ) )
        elif db_inc is not None:
            exts.append( Extension('bsddb', ['bsddbmodule.c'],
                                   include_dirs = db_inc,
                                   libraries = dblib) )

        # The mpz module interfaces to the GNU Multiple Precision library.
        # You need to ftp the GNU MP library.
        # This was originally written and tested against GMP 1.2 and 1.3.2.
        # It has been modified by Rob Hooft to work with 2.0.2 as well, but I
        # haven't tested it recently.   For a more complete module,
        # refer to pympz.sourceforge.net.

        # A compatible MP library unencombered by the GPL also exists.  It was
        # posted to comp.sources.misc in volume 40 and is widely available from
        # FTP archive sites. One URL for it is:
        # ftp://gatekeeper.dec.com/.b/usenet/comp.sources.misc/volume40/fgmp/part01.Z

        if (self.compiler.find_library_file(lib_dirs, 'gmp')):
            exts.append( Extension('mpz', ['mpzmodule.c'],
                                   libraries = ['gmp'] ) )


        # Unix-only modules
        if platform not in ['mac', 'win32']:
            # Steen Lumholt's termios module
            exts.append( Extension('termios', ['termios.c']) )
            # Jeremy Hylton's rlimit interface
            if platform not in ['cygwin']:
                exts.append( Extension('resource', ['resource.c']) )

            # Generic dynamic loading module
            #exts.append( Extension('dl', ['dlmodule.c']) )
            
            # Sun yellow pages. Some systems have the functions in libc.
            if platform not in ['cygwin']:
                if (self.compiler.find_library_file(lib_dirs, 'nsl')):
                    libs = ['nsl']
                else:
                    libs = []
                exts.append( Extension('nis', ['nismodule.c'],
                                       libraries = libs) )

        # Curses support, requring the System V version of curses, often
        # provided by the ncurses library.
        if platform == 'sunos4':
            inc_dirs += ['/usr/5include']
            lib_dirs += ['/usr/5lib']

        if (self.compiler.find_library_file(lib_dirs, 'ncurses')):
            curses_libs = ['ncurses']
            exts.append( Extension('_curses', ['_cursesmodule.c'],
                                   libraries = curses_libs) )
        elif (self.compiler.find_library_file(lib_dirs, 'curses')):
            if (self.compiler.find_library_file(lib_dirs, 'terminfo')):
                curses_libs = ['curses', 'terminfo']
            else:
                curses_libs = ['curses', 'termcap']

            exts.append( Extension('_curses', ['_cursesmodule.c'],
                                   libraries = curses_libs) )

        # If the curses module is enabled, check for the panel module
        if (os.path.exists('Modules/_curses_panel.c') and
            module_enabled(exts, '_curses') and
            self.compiler.find_library_file(lib_dirs, 'panel')):
            exts.append( Extension('_curses_panel', ['_curses_panel.c'],
                                   libraries = ['panel'] + curses_libs) )



        # Lee Busby's SIGFPE modules.
        # The library to link fpectl with is platform specific.
        # Choose *one* of the options below for fpectl:

        if platform == 'irix5':
            # For SGI IRIX (tested on 5.3):
            exts.append( Extension('fpectl', ['fpectlmodule.c'],
                                   libraries=['fpe']) )
        elif 0: # XXX how to detect SunPro?
            # For Solaris with SunPro compiler (tested on Solaris 2.5 with SunPro C 4.2):
            # (Without the compiler you don't have -lsunmath.)
            #fpectl fpectlmodule.c -R/opt/SUNWspro/lib -lsunmath -lm
            pass
        else:
            # For other systems: see instructions in fpectlmodule.c.
            #fpectl fpectlmodule.c ...
            exts.append( Extension('fpectl', ['fpectlmodule.c']) )


        # Andrew Kuchling's zlib module.
        # This require zlib 1.1.3 (or later).
        # See http://www.cdrom.com/pub/infozip/zlib/
        zlib_inc = find_file('zlib.h', [], inc_dirs)
        if zlib_inc is not None:
            zlib_h = zlib_inc[0] + '/zlib.h'
            version = '"0.0.0"'
            version_req = '"1.1.3"'
            fp = open(zlib_h)
            while 1:
                line = fp.readline()
                if not line:
                    break
                if line.find('#define ZLIB_VERSION', 0) == 0:
                    version = line.split()[2]
                    break
            if version >= version_req:
                if (self.compiler.find_library_file(lib_dirs, 'z')):
                    exts.append( Extension('zlib', ['zlibmodule.c'],
                                           libraries = ['z']) )

        # Interface to the Expat XML parser
        #
        # Expat is written by James Clark and must be downloaded separately
        # (see below).  The pyexpat module was written by Paul Prescod after a
        # prototype by Jack Jansen.
        #
        # The Expat dist includes Windows .lib and .dll files.  Home page is
        # at http://www.jclark.com/xml/expat.html, the current production
        # release is always ftp://ftp.jclark.com/pub/xml/expat.zip.
        #
        # EXPAT_DIR, below, should point to the expat/ directory created by
        # unpacking the Expat source distribution.
        #
        # Note: the expat build process doesn't yet build a libexpat.a; you
        # can do this manually while we try convince the author to add it.  To
        # do so, cd to EXPAT_DIR, run "make" if you have not done so, then
        # run:
        #
        #    ar cr libexpat.a xmltok/*.o xmlparse/*.o
        #
        expat_defs = []
        expat_incs = find_file('expat.h', inc_dirs, [])
        if expat_incs is not None:
            # expat.h was found
            expat_defs = [('HAVE_EXPAT_H', 1)]
        else:
            expat_incs = find_file('xmlparse.h', inc_dirs, [])

        if (expat_incs is not None and
            self.compiler.find_library_file(lib_dirs, 'expat')):
            exts.append( Extension('pyexpat', ['pyexpat.c'],
                                   define_macros = expat_defs,
                                   libraries = ['expat']) )

        # Platform-specific libraries
        if platform == 'linux2':
            # Linux-specific modules
            exts.append( Extension('linuxaudiodev', ['linuxaudiodev.c']) )

        if platform == 'sunos5':
            # SunOS specific modules
            exts.append( Extension('sunaudiodev', ['sunaudiodev.c']) )

        self.extensions.extend(exts)

        # Call the method for detecting whether _tkinter can be compiled
        self.detect_tkinter(inc_dirs, lib_dirs)


    def detect_tkinter(self, inc_dirs, lib_dirs):
        # The _tkinter module.
        
        # Assume we haven't found any of the libraries or include files
        tcllib = tklib = tcl_includes = tk_includes = None
        for version in ['8.4', '8.3', '8.2', '8.1', '8.0']:
             tklib = self.compiler.find_library_file(lib_dirs,
                                                     'tk' + version )
             tcllib = self.compiler.find_library_file(lib_dirs,
                                                      'tcl' + version )
             if tklib and tcllib:
                # Exit the loop when we've found the Tcl/Tk libraries
                break

        # Now check for the header files
        if tklib and tcllib:
            # Check for the include files on Debian, where
            # they're put in /usr/include/{tcl,tk}X.Y
            debian_tcl_include = [ '/usr/include/tcl' + version ]
            debian_tk_include =  [ '/usr/include/tk'  + version ] + debian_tcl_include
            tcl_includes = find_file('tcl.h', inc_dirs, debian_tcl_include)
            tk_includes = find_file('tk.h', inc_dirs, debian_tk_include)

        if (tcllib is None or tklib is None and
            tcl_includes is None or tk_includes is None):
            # Something's missing, so give up
            return

        # OK... everything seems to be present for Tcl/Tk.

        include_dirs = [] ; libs = [] ; defs = [] ; added_lib_dirs = []
        for dir in tcl_includes + tk_includes:
            if dir not in include_dirs:
                include_dirs.append(dir)

        # Check for various platform-specific directories
        platform = self.get_platform()
        if platform == 'sunos5':
            include_dirs.append('/usr/openwin/include')
            added_lib_dirs.append('/usr/openwin/lib')
        elif os.path.exists('/usr/X11R6/include'):
            include_dirs.append('/usr/X11R6/include')
            added_lib_dirs.append('/usr/X11R6/lib')
        elif os.path.exists('/usr/X11R5/include'):
            include_dirs.append('/usr/X11R5/include')
            added_lib_dirs.append('/usr/X11R5/lib')
        else:
            # Assume default location for X11
            include_dirs.append('/usr/X11/include')
            added_lib_dirs.append('/usr/X11/lib')

        # Check for BLT extension
        if self.compiler.find_library_file(lib_dirs + added_lib_dirs, 'BLT8.0'):
            defs.append( ('WITH_BLT', 1) )
            libs.append('BLT8.0')

        # Add the Tcl/Tk libraries
        libs.append('tk'+version)
        libs.append('tcl'+version)

        if platform in ['aix3', 'aix4']:
            libs.append('ld')

        # Finally, link with the X11 libraries
        libs.append('X11')

        ext = Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                        define_macros=[('WITH_APPINIT', 1)] + defs,
                        include_dirs = include_dirs,
                        libraries = libs,
                        library_dirs = added_lib_dirs,
                        )
        self.extensions.append(ext)

        # XXX handle these, but how to detect?
        # *** Uncomment and edit for PIL (TkImaging) extension only:
        #       -DWITH_PIL -I../Extensions/Imaging/libImaging  tkImaging.c \
        # *** Uncomment and edit for TOGL extension only:
        #       -DWITH_TOGL togl.c \
        # *** Uncomment these for TOGL extension only:
        #       -lGL -lGLU -lXext -lXmu \

def main():
    setup(name = 'Python standard library',
          version = '%d.%d' % sys.version_info[:2],
          cmdclass = {'build_ext':PyBuildExt},
          # The struct module is defined here, because build_ext won't be
          # called unless there's at least one extension module defined.
          ext_modules=[Extension('struct', ['structmodule.c'])],

          # Scripts to install
          scripts = ['Tools/scripts/pydoc']
        )

# --install-platlib
if __name__ == '__main__':
    sysconfig.set_python_build()
    main()
