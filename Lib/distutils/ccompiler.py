"""distutils.ccompiler

Contains CCompiler, an abstract base class that defines the interface
for the Distutils compiler abstraction model."""

# created 1999/07/05, Greg Ward

__rcsid__ = "$Id$"

import os
from types import *
from copy import copy
from distutils.errors import *


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
    #     I'm not sure how widely supported this is even by POSIX
    #     compilers, much less on other platforms.  And I'm even less
    #     sure how useful it is; probably for cross-compiling, but I
    #     have no intention of supporting that.
    #   * can't do really freaky things with the library list/library
    #     dirs, e.g. "-Ldir1 -lfoo -Ldir2 -lfoo" to link against
    #     different versions of libfoo.a in different locations.  I
    #     think this is useless without the ability to null out the
    #     library search path anyways.
    #   * don't deal with verbose and dry-run flags -- probably a
    #     CCompiler object should just drag them around the way the
    #     Distribution object does (either that or we have to drag
    #     around a Distribution object, which is what Command objects
    #     do... but might be kind of annoying)
    

    def __init__ (self):

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
                 macros=None,
                 includes=None):
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
           compilation only."""
        pass


    # XXX this is kind of useless without 'link_binary()' or
    # 'link_executable()' or something -- or maybe 'link_static_lib()'
    # should not exist at all, and we just have 'link_binary()'?
    def link_static_lib (self,
                         objects,
                         output_libname,
                         libraries=None,
                         library_dirs=None):
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
           supplied to 'add_library_dir()' and/or 'set_library_dirs()'."""

        pass
    

    # XXX what's better/more consistent/more universally understood
    # terminology: "shared library" or "dynamic library"?

    def link_shared_lib (self,
                         objects,
                         output_libname,
                         libraries=None,
                         library_dirs=None):
        """Link a bunch of stuff together to create a shared library
           file.  Has the same effect as 'link_static_lib()' except
           that the filename inferred from 'output_libname' will most
           likely be different, and the type of file generated will
           almost certainly be different."""
        pass
    
    def link_shared_object (self,
                            objects,
                            output_filename,
                            libraries=None,
                            library_dirs=None):
        """Link a bunch of stuff together to create a shared object
           file.  Much like 'link_shared_lib()', except the output
           filename is explicitly supplied as 'output_filename'."""
        pass

# class CCompiler


def new_compiler (plat=None):
    """Generate a CCompiler instance for platform 'plat' (or the
       current platform, if 'plat' not supplied).  Really instantiates
       some concrete subclass of CCompiler, of course."""

    if plat is None: plat = os.name
    if plat == 'posix':
        from unixccompiler import UnixCCompiler
        return UnixCCompiler ()
    else:
        raise DistutilsPlatformError, \
              "don't know how to compile C/C++ code on platform %s" % plat
