"""distutils.command.build_ext

Implements the Distutils 'build_ext' command, for building extension
modules (currently limited to C extensions, should accomodate C++
extensions ASAP)."""

# created 1999/08/09, Greg Ward

__rcsid__ = "$Id$"

import sys, os, string, re
from types import *
from distutils.core import Command
from distutils.ccompiler import new_compiler
from distutils.sysconfig import INCLUDEPY, SO, exec_prefix
from distutils.errors import *


# This is the same as a Python NAME, since we must accept any
# valid module name for the extension name.
extension_name_re = re.compile (r'^[a-zA-Z_][a-zA-Z_0-9]*$')


class BuildExt (Command):
    
    # XXX thoughts on how to deal with complex command-line options like
    # these, i.e. how to make it so fancy_getopt can suck them off the
    # command line and make it look like setup.py defined the appropriate
    # lists of tuples of what-have-you.
    #   - each command needs a callback to process its command-line options
    #   - Command.__init__() needs access to its share of the whole
    #     command line (must ultimately come from
    #     Distribution.parse_command_line())
    #   - it then calls the current command class' option-parsing
    #     callback to deal with weird options like -D, which have to
    #     parse the option text and churn out some custom data
    #     structure
    #   - that data structure (in this case, a list of 2-tuples)
    #     will then be present in the command object by the time
    #     we get to set_final_options() (i.e. the constructor
    #     takes care of both command-line and client options
    #     in between set_default_options() and set_final_options())

    options = [('dir=', 'd',
                "directory for compiled extension modules"),
               ('include-dirs=', 'I',
                "list of directories to search for header files"),
               ('define=', 'D',
                "C preprocessor macros to define"),
               ('undef=', 'U',
                "C preprocessor macros to undefine"),
               ('libs=', 'l',
                "external C libraries to link with"),
               ('library-dirs=', 'L',
                "directories to search for external C libraries"),
               ('rpath=', 'R',
                "directories to search for shared C libraries at runtime"),
               ('link-objects=', 'O',
                "extra explicit link objects to include in the link"),
              ]


    def set_default_options (self):
        self.dir = None
        self.include_dirs = None
        self.define = None
        self.undef = None
        self.libs = None
        self.library_dirs = None
        self.rpath = None
        self.link_objects = None

    def set_final_options (self):
        self.set_undefined_options ('build', ('platdir', 'dir'))

        # Make sure Python's include directories (for Python.h, config.h,
        # etc.) are in the include search path.  We have to roll our own
        # "exec include dir", because the Makefile parsed by sysconfig
        # doesn't have it (sigh).
        py_include = INCLUDEPY         # prefix + "include" + "python" + ver
        exec_py_include = os.path.join (exec_prefix, 'include',
                                        'python' + sys.version[0:3])
        if self.include_dirs is None:
            self.include_dirs = []
        self.include_dirs.insert (0, py_include)
        if exec_py_include != py_include:
            self.include_dirs.insert (0, exec_py_include)


    def run (self):

        self.set_final_options ()
        (extensions, package) = \
            self.distribution.get_options ('ext_modules', 'package')

        # 'extensions', as supplied by setup.py, is a list of 2-tuples.
        # Each tuple is simple:
        #    (ext_name, build_info)
        # build_info is a dictionary containing everything specific to
        # building this extension.  (Info pertaining to all extensions
        # should be handled by general distutils options passed from
        # setup.py down to right here, but that's not taken care of yet.)


        # First, sanity-check the 'extensions' list
        self.check_extensions_list (extensions)

        # Setup the CCompiler object that we'll use to do all the
        # compiling and linking
        self.compiler = new_compiler (verbose=self.distribution.verbose,
                                      dry_run=self.distribution.dry_run)
        if self.include_dirs is not None:
            self.compiler.set_include_dirs (self.include_dirs)
        if self.define is not None:
            # 'define' option is a list of (name,value) tuples
            for (name,value) in self.define:
                self.compiler.define_macro (name, value)
        if self.undef is not None:
            for macro in self.undef:
                self.compiler.undefine_macro (macro)
        if self.libs is not None:
            self.compiler.set_libraries (self.libs)
        if self.library_dirs is not None:
            self.compiler.set_library_dirs (self.library_dirs)
        if self.rpath is not None:
            self.compiler.set_runtime_library_dirs (self.rpath)
        if self.link_objects is not None:
            self.compiler.set_link_objects (self.link_objects)
                                      
        # Now the real loop over extensions
        self.build_extensions (extensions)

            

    def check_extensions_list (self, extensions):

        if type (extensions) is not ListType:
            raise DistutilsValueError, \
                  "'ext_modules' option must be a list of tuples"
        
        for ext in extensions:
            if type (ext) is not TupleType and len (ext) != 2:
                raise DistutilsValueError, \
                      "each element of 'ext_modules' option must be a 2-tuple"

            if not (type (ext[0]) is StringType and
                    extension_name_re.match (ext[0])):
                raise DistutilsValueError, \
                      "first element of each tuple in 'ext_modules' " + \
                      "must be the extension name (a string)"

            if type (ext[1]) is not DictionaryType:
                raise DistutilsValueError, \
                      "second element of each tuple in 'ext_modules' " + \
                      "must be a dictionary"

        # end sanity-check for

    # check_extensions_list ()


    def build_extensions (self, extensions):

        for (extension_name, build_info) in extensions:
            sources = build_info.get ('sources')
            if sources is None or type (sources) is not ListType:
                raise DistutilsValueError, \
                      "in ext_modules option, 'sources' must be present " + \
                      "and must be a list of source filenames"
            
            macros = build_info.get ('macros')
            include_dirs = build_info.get ('include_dirs')
            self.compiler.compile (sources, macros, include_dirs)

            objects = self.compiler.object_filenames (sources)
            extra_objects = build_info.get ('extra_objects')
            if extra_objects:
                objects.extend (extra_objects)
            libraries = build_info.get ('libraries')
            library_dirs = build_info.get ('library_dirs')

            ext_filename = self.extension_filename (extension_name)
            self.compiler.link_shared_object (objects, ext_filename,
                                              libraries, library_dirs)

    # build_extensions ()


    def extension_filename (self, ext_name):
        return ext_name + SO

# class BuildExt
