"""distutils.ccompiler

Contains CCompiler, an abstract base class that defines the interface
for the Distutils compiler abstraction model."""

# created 1999/07/05, Greg Ward

__rcsid__ = "$Id$"

import sys, os
from types import *
from copy import copy
from distutils.errors import *
from distutils.spawn import spawn
from distutils.util import move_file


class CCompiler:
    """Abstract base class to define the interface that must be implemented
       by real compiler abstraction classes.  Might have some use as a
       place for shared code, but it's not yet clear what code can be
       shared between compiler abstraction models for different platforms.

       The basic idea behind a compiler abstraction class is that each
       instance can be used for all the compile/link steps in building
       a single project.  Thus, attributes common to all of those compile
       and link steps -- include directories, macros to define, libraries
       to link against, etc. -- are attributes of the compiler instance.
       To allow for variability in how individual files are treated,
       most (all?) of those attributes may be varied on a per-compilation
       or per-link basis."""

    # 'compiler_type' is a class attribute that identifies this class.  It
    # keeps code that wants to know what kind of compiler it's dealing with
    # from having to import all possible compiler classes just to do an
    # 'isinstance'.  In concrete CCompiler subclasses, 'compiler_type'
    # should really, really be one of the keys of the 'compiler_class'
    # dictionary (see below -- used by the 'new_compiler()' factory
    # function) -- authors of new compiler interface classes are
    # responsible for updating 'compiler_class'!
    compiler_type = None

    # XXX things not handled by this compiler abstraction model:
    #   * client can't provide additional options for a compiler,
    #     e.g. warning, optimization, debugging flags.  Perhaps this
    #     should be the domain of concrete compiler abstraction classes
    #     (UnixCCompiler, MSVCCompiler, etc.) -- or perhaps the base
    #     class should have methods for the common ones.
    #   * can't put output files (object files, libraries, whatever)
    #     into a separate directory from their inputs.  Should this be
    #     handled by an 'output_dir' attribute of the whole object, or a
    #     parameter to the compile/link_* methods, or both?
    #   * can't completely override the include or library searchg
    #     path, ie. no "cc -I -Idir1 -Idir2" or "cc -L -Ldir1 -Ldir2".
    #     I'm not sure how widely supported this is even by Unix
    #     compilers, much less on other platforms.  And I'm even less
    #     sure how useful it is; maybe for cross-compiling, but
    #     support for that is a ways off.  (And anyways, cross
    #     compilers probably have a dedicated binary with the
    #     right paths compiled in.  I hope.)
    #   * can't do really freaky things with the library list/library
    #     dirs, e.g. "-Ldir1 -lfoo -Ldir2 -lfoo" to link against
    #     different versions of libfoo.a in different locations.  I
    #     think this is useless without the ability to null out the
    #     library search path anyways.
    

    def __init__ (self,
                  verbose=0,
                  dry_run=0):

        self.verbose = verbose
        self.dry_run = dry_run

        # 'output_dir': a common output directory for object, library,
        # shared object, and shared library files
        self.output_dir = None

        # 'macros': a list of macro definitions (or undefinitions).  A
        # macro definition is a 2-tuple (name, value), where the value is
        # either a string or None (no explicit value).  A macro
        # undefinition is a 1-tuple (name,).
        self.macros = []

        # 'include_dirs': a list of directories to search for include files
        self.include_dirs = []

        # 'libraries': a list of libraries to include in any link
        # (library names, not filenames: eg. "foo" not "libfoo.a")
        self.libraries = []

        # 'library_dirs': a list of directories to search for libraries
        self.library_dirs = []

        # 'runtime_library_dirs': a list of directories to search for
        # shared libraries/objects at runtime
        self.runtime_library_dirs = []

        # 'objects': a list of object files (or similar, such as explicitly
        # named library files) to include on any link
        self.objects = []

    # __init__ ()


    def _find_macro (self, name):
        i = 0
        for defn in self.macros:
            if defn[0] == name:
                return i
            i = i + 1

        return None


    def _check_macro_definitions (self, definitions):
        """Ensures that every element of 'definitions' is a valid macro
           definition, ie. either (name,value) 2-tuple or a (name,)
           tuple.  Do nothing if all definitions are OK, raise 
           TypeError otherwise."""

        for defn in definitions:
            if not (type (defn) is TupleType and
                    (len (defn) == 1 or
                     (len (defn) == 2 and
                      (type (defn[1]) is StringType or defn[1] is None))) and
                    type (defn[0]) is StringType):
                raise TypeError, \
                      ("invalid macro definition '%s': " % defn) + \
                      "must be tuple (string,), (string, string), or " + \
                      "(string, None)"


    # -- Bookkeeping methods -------------------------------------------

    def define_macro (self, name, value=None):
        """Define a preprocessor macro for all compilations driven by
           this compiler object.  The optional parameter 'value' should be
           a string; if it is not supplied, then the macro will be defined
           without an explicit value and the exact outcome depends on the
           compiler used (XXX true? does ANSI say anything about this?)"""

        # Delete from the list of macro definitions/undefinitions if
        # already there (so that this one will take precedence).
        i = self._find_macro (name)
        if i is not None:
            del self.macros[i]

        defn = (name, value)
        self.macros.append (defn)


    def undefine_macro (self, name):
        """Undefine a preprocessor macro for all compilations driven by
           this compiler object.  If the same macro is defined by
           'define_macro()' and undefined by 'undefine_macro()' the last
           call takes precedence (including multiple redefinitions or
           undefinitions).  If the macro is redefined/undefined on a
           per-compilation basis (ie. in the call to 'compile()'), then
           that takes precedence."""

        # Delete from the list of macro definitions/undefinitions if
        # already there (so that this one will take precedence).
        i = self._find_macro (name)
        if i is not None:
            del self.macros[i]

        undefn = (name,)
        self.macros.append (undefn)


    def add_include_dir (self, dir):
        """Add 'dir' to the list of directories that will be searched
           for header files.  The compiler is instructed to search
           directories in the order in which they are supplied by
           successive calls to 'add_include_dir()'."""
        self.include_dirs.append (dir)

    def set_include_dirs (self, dirs):
        """Set the list of directories that will be searched to 'dirs'
           (a list of strings).  Overrides any preceding calls to
           'add_include_dir()'; subsequence calls to 'add_include_dir()'
           add to the list passed to 'set_include_dirs()'.  This does
           not affect any list of standard include directories that
           the compiler may search by default."""
        self.include_dirs = copy (dirs)


    def add_library (self, libname):
        """Add 'libname' to the list of libraries that will be included
           in all links driven by this compiler object.  Note that
           'libname' should *not* be the name of a file containing a
           library, but the name of the library itself: the actual filename
           will be inferred by the linker, the compiler, or the compiler
           abstraction class (depending on the platform).

           The linker will be instructed to link against libraries in the
           order they were supplied to 'add_library()' and/or
           'set_libraries()'.  It is perfectly valid to duplicate library
           names; the linker will be instructed to link against libraries
           as many times as they are mentioned."""
        self.libraries.append (libname)

    def set_libraries (self, libnames):
        """Set the list of libraries to be included in all links driven
           by this compiler object to 'libnames' (a list of strings).
           This does not affect any standard system libraries that the
           linker may include by default."""

        self.libraries = copy (libnames)


    def add_library_dir (self, dir):
        """Add 'dir' to the list of directories that will be searched for
           libraries specified to 'add_library()' and 'set_libraries()'.
           The linker will be instructed to search for libraries in the
           order they are supplied to 'add_library_dir()' and/or
           'set_library_dirs()'."""
        self.library_dirs.append (dir)

    def set_library_dirs (self, dirs):
        """Set the list of library search directories to 'dirs' (a list
           of strings).  This does not affect any standard library
           search path that the linker may search by default."""
        self.library_dirs = copy (dirs)


    def add_runtime_library_dir (self, dir):
        """Add 'dir' to the list of directories that will be searched for
           shared libraries at runtime."""
        self.runtime_library_dirs.append (dir)

    def set_runtime_library_dirs (self, dirs):
        """Set the list of directories to search for shared libraries
           at runtime to 'dirs' (a list of strings).  This does not affect
           any standard search path that the runtime linker may search by
           default."""
        self.runtime_library_dirs = copy (dirs)


    def add_link_object (self, object):
        """Add 'object' to the list of object files (or analogues, such
           as explictly named library files or the output of "resource
           compilers") to be included in every link driven by this
           compiler object."""
        self.objects.append (object)

    def set_link_objects (self, objects):
        """Set the list of object files (or analogues) to be included
           in every link to 'objects'.  This does not affect any
           standard object files that the linker may include by default
           (such as system libraries)."""
        self.objects = copy (objects)


    # -- Worker methods ------------------------------------------------
    # (must be implemented by subclasses)

    def compile (self,
                 sources,
                 output_dir=None,
                 macros=None,
                 includes=None,
                 extra_preargs=None,
                 extra_postargs=None):
        """Compile one or more C/C++ source files.  'sources' must be
           a list of strings, each one the name of a C/C++ source
           file.  Return a list of the object filenames generated
           (one for each source filename in 'sources').

           'macros', if given, must be a list of macro definitions.  A
           macro definition is either a (name, value) 2-tuple or a (name,)
           1-tuple.  The former defines a macro; if the value is None, the
           macro is defined without an explicit value.  The 1-tuple case
           undefines a macro.  Later definitions/redefinitions/
           undefinitions take precedence.

           'includes', if given, must be a list of strings, the directories
           to add to the default include file search path for this
           compilation only.

           'extra_preargs' and 'extra_postargs' are optional lists of extra
           command-line arguments that will be, respectively, prepended or
           appended to the generated command line immediately before
           execution.  These will most likely be peculiar to the particular
           platform and compiler being worked with, but are a necessary
           escape hatch for those occasions when the abstract compiler
           framework doesn't cut the mustard."""
           
        pass


    # XXX this is kind of useless without 'link_binary()' or
    # 'link_executable()' or something -- or maybe 'link_static_lib()'
    # should not exist at all, and we just have 'link_binary()'?
    def link_static_lib (self,
                         objects,
                         output_libname,
                         output_dir=None,
                         libraries=None,
                         library_dirs=None,
                         extra_preargs=None,
                         extra_postargs=None):
        """Link a bunch of stuff together to create a static library
           file.  The "bunch of stuff" consists of the list of object
           files supplied as 'objects', the extra object files supplied
           to 'add_link_object()' and/or 'set_link_objects()', the
           libraries supplied to 'add_library()' and/or
           'set_libraries()', and the libraries supplied as 'libraries'
           (if any).

           'output_libname' should be a library name, not a filename;
           the filename will be inferred from the library name.

           'library_dirs', if supplied, should be a list of additional
           directories to search on top of the system default and those
           supplied to 'add_library_dir()' and/or 'set_library_dirs()'.

           'extra_preargs' and 'extra_postargs' are as for 'compile()'
           (except of course that they supply command-line arguments
           for the particular linker being used)."""

        pass
    

    def link_shared_lib (self,
                         objects,
                         output_libname,
                         output_dir=None,
                         libraries=None,
                         library_dirs=None,
                         extra_preargs=None,
                         extra_postargs=None):
        """Link a bunch of stuff together to create a shared library
           file.  Has the same effect as 'link_static_lib()' except
           that the filename inferred from 'output_libname' will most
           likely be different, and the type of file generated will
           almost certainly be different."""
        pass
    

    def link_shared_object (self,
                            objects,
                            output_filename,
                            output_dir=None,
                            libraries=None,
                            library_dirs=None,
                            extra_preargs=None,
                            extra_postargs=None):
        """Link a bunch of stuff together to create a shared object
           file.  Much like 'link_shared_lib()', except the output filename
           is explicitly supplied as 'output_filename'.  If 'output_dir' is
           supplied, 'output_filename' is relative to it
           (i.e. 'output_filename' can provide directory components if
           needed)."""
        pass


    # -- Filename mangling methods -------------------------------------

    # General principle for the filename-mangling methods: by default,
    # don't include a directory component, no matter what the caller
    # supplies.  Eg. for UnixCCompiler, a source file of "foo/bar/baz.c"
    # becomes "baz.o" or "baz.so", etc.  (That way, it's easiest for the
    # caller to decide where it wants to put/find the output file.)  The
    # 'output_dir' parameter overrides this, of course -- the directory
    # component of the input filenames is replaced by 'output_dir'.

    def object_filenames (self, source_filenames, output_dir=None):
        """Return the list of object filenames corresponding to each
           specified source filename."""
        pass

    def shared_object_filename (self, source_filename):
        """Return the shared object filename corresponding to a
           specified source filename (assuming the same directory)."""
        pass    

    def library_filename (self, libname):
        """Return the static library filename corresponding to the
           specified library name."""
        
        pass

    def shared_library_filename (self, libname):
        """Return the shared library filename corresponding to the
           specified library name."""
        pass

    # XXX ugh -- these should go!
    def object_name (self, inname):
        """Given a name with no extension, return the name + object extension"""
        return inname + self._obj_ext

    def shared_library_name (self, inname):
        """Given a name with no extension, return the name + shared object extension"""
        return inname + self._shared_lib_ext

    # -- Utility methods -----------------------------------------------

    def announce (self, msg, level=1):
        if self.verbose >= level:
            print msg

    def spawn (self, cmd):
        spawn (cmd, verbose=self.verbose, dry_run=self.dry_run)

    def move_file (self, src, dst):
        return move_file (src, dst, verbose=self.verbose, dry_run=self.dry_run)


# class CCompiler


# Map a platform ('posix', 'nt') to the default compiler type for
# that platform.
default_compiler = { 'posix': 'unix',
                     'nt': 'msvc',
                   }

# Map compiler types to (module_name, class_name) pairs -- ie. where to
# find the code that implements an interface to this compiler.  (The module
# is assumed to be in the 'distutils' package.)
compiler_class = { 'unix': ('unixccompiler', 'UnixCCompiler'),
                   'msvc': ('msvccompiler', 'MSVCCompiler'),
                 }


def new_compiler (plat=None,
                  compiler=None,
                  verbose=0,
                  dry_run=0):

    """Generate an instance of some CCompiler subclass for the supplied
       platform/compiler combination.  'plat' defaults to 'os.name'
       (eg. 'posix', 'nt'), and 'compiler' defaults to the default
       compiler for that platform.  Currently only 'posix' and 'nt'
       are supported, and the default compilers are "traditional Unix
       interface" (UnixCCompiler class) and Visual C++ (MSVCCompiler
       class).  Note that it's perfectly possible to ask for a Unix
       compiler object under Windows, and a Microsoft compiler object
       under Unix -- if you supply a value for 'compiler', 'plat'
       is ignored."""

    if plat is None:
        plat = os.name

    try:
        if compiler is None:
            compiler = default_compiler[plat]
        
        (module_name, class_name) = compiler_class[compiler]
    except KeyError:
        msg = "don't know how to compile C/C++ code on platform '%s'" % plat
        if compiler is not None:
            msg = msg + " with '%s' compiler" % compiler
        raise DistutilsPlatformError, msg
              
    try:
        module_name = "distutils." + module_name
        __import__ (module_name)
        module = sys.modules[module_name]
        klass = vars(module)[class_name]
    except ImportError:
        raise DistutilsModuleError, \
              "can't compile C/C++ code: unable to load module '%s'" % \
              module_name
    except KeyError:
        raise DistutilsModuleError, \
              ("can't compile C/C++ code: unable to find class '%s' " +
               "in module '%s'") % (class_name, module_name)

    return klass (verbose, dry_run)


def gen_preprocess_options (macros, includes):
    """Generate C pre-processor options (-D, -U, -I) as used by at
       least two types of compilers: the typical Unix compiler and Visual
       C++.  'macros' is the usual thing, a list of 1- or 2-tuples, where
       (name,) means undefine (-U) macro 'name', and (name,value) means
       define (-D) macro 'name' to 'value'.  'includes' is just a list of
       directory names to be added to the header file search path (-I).
       Returns a list of command-line options suitable for either
       Unix compilers or Visual C++."""
    
    # XXX it would be nice (mainly aesthetic, and so we don't generate
    # stupid-looking command lines) to go over 'macros' and eliminate
    # redundant definitions/undefinitions (ie. ensure that only the
    # latest mention of a particular macro winds up on the command
    # line).  I don't think it's essential, though, since most (all?)
    # Unix C compilers only pay attention to the latest -D or -U
    # mention of a macro on their command line.  Similar situation for
    # 'includes'.  I'm punting on both for now.  Anyways, weeding out
    # redundancies like this should probably be the province of
    # CCompiler, since the data structures used are inherited from it
    # and therefore common to all CCompiler classes.

    pp_opts = []
    for macro in macros:

        if not (type (macro) is TupleType and
                1 <= len (macro) <= 2):
            raise TypeError, \
                  ("bad macro definition '%s': " +
                   "each element of 'macros' list must be a 1- or 2-tuple") % \
                  macro

        if len (macro) == 1:        # undefine this macro
            pp_opts.append ("-U%s" % macro[0])
        elif len (macro) == 2:
            if macro[1] is None:    # define with no explicit value
                pp_opts.append ("-D%s" % macro[0])
            else:
                # XXX *don't* need to be clever about quoting the
                # macro value here, because we're going to avoid the
                # shell at all costs when we spawn the command!
                pp_opts.append ("-D%s=%s" % macro)

    for dir in includes:
        pp_opts.append ("-I%s" % dir)

    return pp_opts

# gen_preprocess_options ()


def gen_lib_options (library_dirs, libraries, dir_format, lib_format):
    """Generate linker options for searching library directories and
       linking with specific libraries.  'libraries' and 'library_dirs'
       are, respectively, lists of library names (not filenames!) and
       search directories.  'lib_format' is a format string with exactly
       one "%s", into which will be plugged each library name in turn;
       'dir_format' is similar, but directory names will be plugged into
       it.  Returns a list of command-line options suitable for use with
       some compiler (depending on the two format strings passed in)."""

    lib_opts = []

    for dir in library_dirs:
        lib_opts.append (dir_format % dir)

    # XXX it's important that we *not* remove redundant library mentions!
    # sometimes you really do have to say "-lfoo -lbar -lfoo" in order to
    # resolve all symbols.  I just hope we never have to say "-lfoo obj.o
    # -lbar" to get things to work -- that's certainly a possibility, but a
    # pretty nasty way to arrange your C code.

    for lib in libraries:
        lib_opts.append (lib_format % lib)

    return lib_opts

# _gen_lib_options ()
