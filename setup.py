# To be fixed:
#   Implement --disable-modules setting
#   Don't install to site-packages, but to lib-dynload
            
import sys, os, string, getopt
from distutils import sysconfig
from distutils.core import Extension, setup
from distutils.command.build_ext import build_ext

# This global variable is used to hold the list of modules to be disabled.
disabled_module_list = []

def find_file(path, filename):
    for p in path:
        fullpath = p + os.sep + filename
        if os.path.exists(fullpath):
            return fullpath
    return None

def module_enabled(extlist, modname):
    """Returns whether the module 'modname' is present in the list
    of extensions 'extlist'."""
    extlist = [ext for ext in extlist if ext.name == modname]
    return len(extlist)
               
class PyBuildExt(build_ext):
    
    def build_extensions(self):
        from distutils.ccompiler import new_compiler
        from distutils.sysconfig import customize_compiler

        # Detect which modules should be compiled
        self.detect_modules()

        # Remove modules that are present on the disabled list
        self.extensions = [ext for ext in self.extensions
                           if ext.name not in disabled_module_list]
        
        # Fix up the autodetected modules, prefixing all the source files
        # with Modules/ and adding Python's include directory to the path.
        (srcdir,) = sysconfig.get_config_vars('srcdir')

        # 
        moddir = os.path.join(os.getcwd(), 'Modules', srcdir)
        moddir = os.path.normpath(moddir)
        srcdir, tail = os.path.split(moddir)
        srcdir = os.path.normpath(srcdir)
        moddir = os.path.normpath(moddir)

        for ext in self.extensions:
            ext.sources = [ os.path.join(moddir, filename)
                            for filename in ext.sources ]
            ext.include_dirs.append( '.' ) # to get config.h
            ext.include_dirs.append( os.path.join(srcdir, './Include') )
            
        # If the signal module is being compiled, remove the sigcheck.o and
        # intrcheck.o object files from the archive.
        if module_enabled(self.extensions, 'signal'):
            print 'removing sigcheck.o intrcheck.o'
            ar, library = sysconfig.get_config_vars('AR', 'LIBRARY')
            cmd = '%s d Modules/%s sigcheck.o intrcheck.o' % (ar, library)
#            os.system(cmd)
            
        build_ext.build_extensions(self)

    def detect_modules(self):
        # XXX this always gets an empty list -- hardwiring to
        # a fixed list
        lib_dirs = self.compiler.library_dirs[:]
        lib_dirs += ['/lib', '/usr/lib', '/usr/local/lib']
#        std_lib_dirs = ['/lib', '/usr/lib']
        exts = []

        # XXX Omitted modules: gl, pure, dl, SGI-specific modules

        #
        # The following modules are all pretty straightforward, and compile
        # on pretty much any POSIXish platform.
        #
        
        # Some modules that are normally always on:
        exts.append( Extension('regex', ['regexmodule.c', 'regexpr.c']) )
        exts.append( Extension('pcre', ['pcremodule.c', 'pypcre.c']) )
        exts.append( Extension('signal', ['signalmodule.c']) )
        
        # XXX uncomment this with 2.0CVS
        #exts.append( Extension('xreadlines', ['xreadlines.c']) )

        # array objects
        exts.append( Extension('array', ['arraymodule.c']) )
        # complex math library functions
        exts.append( Extension('cmath', ['cmathmodule.c'], libraries=['m']) )
        # math library functions, e.g. sin()
        exts.append( Extension('math',  ['mathmodule.c'], libraries=['m']) )
        # fast string operations implemented in C
        exts.append( Extension('strop', ['stropmodule.c']) )
        # time operations and variables
        exts.append( Extension('time', ['timemodule.c'], libraries=['m']) )
        # operator.add() and similar goodies
        exts.append( Extension('operator', ['operator.c']) )
        # access to the builtin codecs and codec registry
        exts.append( Extension('_codecs', ['_codecsmodule.c']) )
        # static Unicode character database
        exts.append( Extension('unicodedata', ['unicodedata.c', 'unicodedatabase.c']) )
        # Unicode Character Name expansion hash table
        exts.append( Extension('ucnhash', ['ucnhash.c']) )
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

        # Tommy Burnette's 'new' module (creates new empty objects of certain kinds):
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
        # Here ends the simple stuff.  From here on, modules need certain libraries,
        # are platform-specific, or present other surprises.
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
        if (self.compiler.find_library_file(self.compiler.library_dirs, 'readline')):
            exts.append( Extension('readline', ['readline.c'],
                                   libraries=['readline', 'termcap']) )

        # The crypt module is now disabled by default because it breaks builds
        # on many systems (where -lcrypt is needed), e.g. Linux (I believe).

        if self.compiler.find_library_file(lib_dirs, 'crypt'):
            libs = ['crypt']
        else:
            libs = []
        exts.append( Extension('crypt', ['cryptmodule.c'], libraries=libs) )

        # socket(2)
        # Detect SSL support for the socket module
        if (self.compiler.find_library_file(lib_dirs, 'ssl') and
            self.compiler.find_library_file(lib_dirs, 'crypto') ):
            exts.append( Extension('_socket', ['socketmodule.c'],
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

        # Note: If a db.h file is found by configure, bsddb will be enabled
        # automatically via Setup.config.in.  It only needs to be enabled here
        # if it is not automatically enabled there; check the generated
        # Setup.config before enabling it here.

        if (self.compiler.find_library_file(lib_dirs, 'db') and
            find_file(['/usr/include', '/usr/local/include'] + self.include_dirs,
                      'db_185.h') ):
            exts.append( Extension('bsddb', ['bsddbmodule.c'],
                                   libraries = ['db'] ) )

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

        # Anthony Baxter's gdbm module.  GNU dbm(3) will require -lgdbm:
        if (self.compiler.find_library_file(lib_dirs, 'gmp')):
            exts.append( Extension('mpz', ['mpzmodule.c'],
                                   libraries = ['gmp'] ) )


        # Unix-only modules
        if sys.platform not in ['mac', 'win32']:
            # Steen Lumholt's termios module
            exts.append( Extension('termios', ['termios.c']) )
            # Jeremy Hylton's rlimit interface
            exts.append( Extension('resource', ['resource.c']) )

            if (self.compiler.find_library_file(lib_dirs, 'nsl')):
                exts.append( Extension('nis', ['nismodule.c'],
                                       libraries = ['nsl']) )

        # Curses support, requring the System V version of curses, often
        # provided by the ncurses library. 
        if sys.platform == 'sunos4':
            include_dirs += ['/usr/5include']
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

        if sys.platform == 'irix5':
            # For SGI IRIX (tested on 5.3):
            exts.append( Extension('fpectl', ['fpectlmodule.c'],
                                   libraries=['fpe']) )
        elif 0: # XXX
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
        if (self.compiler.find_library_file(lib_dirs, 'z')):
            exts.append( Extension('zlib', ['zlibmodule.c'],
                                   libraries = ['z']) )

        # Interface to the Expat XML parser
        #
        # Expat is written by James Clark and must be downloaded separately
        # (see below).  The pyexpat module was written by Paul Prescod after a
        # prototype by Jack Jansen.
        #
        # The Expat dist includes Windows .lib and .dll files.  Home page is at
        # http://www.jclark.com/xml/expat.html, the current production release is
        # always ftp://ftp.jclark.com/pub/xml/expat.zip.
        #
        # EXPAT_DIR, below, should point to the expat/ directory created by
        # unpacking the Expat source distribution.
        #
        # Note: the expat build process doesn't yet build a libexpat.a; you can
        # do this manually while we try convince the author to add it.  To do so,
        # cd to EXPAT_DIR, run "make" if you have not done so, then run:
        #
        #    ar cr libexpat.a xmltok/*.o xmlparse/*.o
        #
        if (self.compiler.find_library_file(lib_dirs, 'expat')):
            exts.append( Extension('pyexpat', ['pyexpat.c'],
                                   libraries = ['expat']) )

        # Platform-specific libraries
        plat = sys.platform
        if plat == 'linux2':
            # Linux-specific modules
            exts.append( Extension('linuxaudiodev', ['linuxaudiodev.c']) )

        if plat == 'sunos5':
            # SunOS specific modules 
            exts.append( Extension('sunaudiodev', ['sunaudiodev.c']) )

        # The _tkinter module.
        #
        # The command for _tkinter is long and site specific.  Please
        # uncomment and/or edit those parts as indicated.  If you don't have a
        # specific extension (e.g. Tix or BLT), leave the corresponding line
        # commented out.  (Leave the trailing backslashes in!  If you
        # experience strange errors, you may want to join all uncommented
        # lines and remove the backslashes -- the backslash interpretation is
        # done by the shell's "read" command and it may not be implemented on
        # every system.

        # XXX need to add the old 7.x/4.x unsynced version numbers here
        for tcl_version, tk_version in [('8.4', '8.4'),
                                        ('8.3', '8.3'),
                                        ('8.2', '8.2'),
                                        ('8.1', '8.1'),
                                        ('8.0', '8.0')]:
            tklib = self.compiler.find_library_file(lib_dirs,
                                                    'tk' + tk_version )
            tcllib = self.compiler.find_library_file(lib_dirs,
                                                     'tcl' + tcl_version )
            if tklib and tcllib:
                # Exit the loop when we've found the Tcl/Tk libraries
                break
            
        if (tcllib and tklib):
            include_dirs = [] ; libs = [] ; defs = [] ; added_lib_dirs = []

            # Determine the prefix where Tcl/Tk is installed by
            # chopping off the 'lib' suffix.
            prefix = os.path.dirname(tcllib)
            L = string.split(prefix, os.sep)
            if L[-1] == 'lib': del L[-1]
            prefix = string.join(L, os.sep)

            if prefix + os.sep + 'lib' not in lib_dirs:
                added_lib_dirs.append( prefix + os.sep + 'lib')

                # Check for the include files on Debian, where
                # they're put in /usr/include/{tcl,tk}X.Y
                # XXX currently untested
                debian_tcl_include = ( prefix + os.sep + 'include/tcl' +
                                       tcl_version )
                debian_tk_include = ( prefix + os.sep + 'include/tk' +
                                       tk_version )
                if os.path.exists(debian_tcl_include):
                    include_dirs = [debian_tcl_include, debian_tk_include]
                else:
                    # Fallback for non-Debian systems
                    include_dirs = [prefix + os.sep + 'include']

            # Check for various platform-specific directories
            if sys.platform == 'sunos5':
                include_dirs.append('/usr/openwin/include')
                added_lib_dirs.append('/usr/openwin/lib')
            elif os.path.exists('/usr/X11R6/include'):
                include_dirs.append('/usr/X11R6/include')
                added_lib_dirs.append('/usr/X11R6/lib')
            elif os.path.exists('/usr/X11R5/include'):
                include_dirs.append('/usr/X11R5/include')
                added_lib_dirs.append('/usr/X11R5/lib')
            else:
                # Assume default form
                include_dirs.append('/usr/X11/include')
                added_lib_dirs.append('/usr/X11/lib')

            if self.compiler.find_library_file(lib_dirs + added_lib_dirs, 'tix4.1.8.0'):
                defs.append( ('WITH_TIX', 1) )
                libs.append('tix4.1.8.0')

            if self.compiler.find_library_file(lib_dirs + added_lib_dirs, 'BLT8.0'):
                defs.append( ('WITH_BLT', 1) )
                libs.append('BLT8.0')

            if sys.platform in ['aix3', 'aix4']:
                libs.append('ld')

            # X11 libraries to link with:
            libs.append('X11')

            tklib, ext = os.path.splitext(tklib)
            tcllib, ext = os.path.splitext(tcllib)
            tklib = os.path.basename(tklib)
            tcllib = os.path.basename(tcllib)
            libs.append( tklib[3:] )    # Chop off 'lib' prefix
            libs.append( tcllib[3:] )
            
            exts.append( Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                                   define_macros=[('WITH_APPINIT', 1)] + defs,
                                   include_dirs = include_dirs,
                                   libraries = libs,
                                   library_dirs = added_lib_dirs,
                                   )
                         )
        # XXX handle these
        # *** Uncomment and edit for PIL (TkImaging) extension only:
        #	-DWITH_PIL -I../Extensions/Imaging/libImaging  tkImaging.c \
        # *** Uncomment and edit for TOGL extension only:
        #	-DWITH_TOGL togl.c \
        # *** Uncomment these for TOGL extension only:
        #	-lGL -lGLU -lXext -lXmu \

        self.extensions = exts

def main():
    setup(name = 'Python standard library',
          version = '2.0',
          cmdclass = {'build_ext':PyBuildExt},
          # The struct module is defined here, because build_ext won't be
          # called unless there's at least one extension module defined.
          ext_modules=[Extension('struct', ['structmodule.c'])]
        )
          
# --install-platlib
if __name__ == '__main__':
    sysconfig.set_python_build()
    main()

