"""distutils.command.build_ext

Implements the Distutils 'build_ext' command, for building extension
modules (currently limited to C extensions, should accomodate C++
extensions ASAP)."""

# created 1999/08/09, Greg Ward

__rcsid__ = "$Id$"

import sys, os, string, re
from types import *
from distutils.core import Command
from distutils.errors import *


# An extension name is just a dot-separated list of Python NAMEs (ie.
# the same as a fully-qualified module name).
extension_name_re = re.compile \
    (r'^[a-zA-Z_][a-zA-Z_0-9]*(\.[a-zA-Z_][a-zA-Z_0-9]*)*$')


class BuildExt (Command):
    
    description = "build C/C++ extensions (compile/link to build directory)"

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

    options = [('build-dir=', 'd',
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
        self.extensions = None
        self.build_dir = None
        self.package = None

        self.include_dirs = None
        self.define = None
        self.undef = None
        self.libs = None
        self.library_dirs = None
        self.rpath = None
        self.link_objects = None


    def set_final_options (self):
        from distutils import sysconfig

        self.set_undefined_options ('build', ('build_platlib', 'build_dir'))

        if self.package is None:
            self.package = self.distribution.ext_package

        self.extensions = self.distribution.ext_modules
        

        # Make sure Python's include directories (for Python.h, config.h,
        # etc.) are in the include search path.  We have to roll our own
        # "exec include dir", because the Makefile parsed by sysconfig
        # doesn't have it (sigh).
        py_include = sysconfig.INCLUDEPY # prefix + "include" + "python" + ver
        exec_py_include = os.path.join (sysconfig.exec_prefix, 'include',
                                        'python' + sys.version[0:3])
        if self.include_dirs is None:
            self.include_dirs = self.distribution.include_dirs or []
        if type (self.include_dirs) is StringType:
            self.include_dirs = string.split (self.include_dirs,
                                              os.pathsep)

        self.include_dirs.insert (0, py_include)
        if exec_py_include != py_include:
            self.include_dirs.insert (0, exec_py_include)

        # XXX how the heck are 'self.define' and 'self.undef' supposed to
        # be set?

    # set_final_options ()
    

    def run (self):

        from distutils.ccompiler import new_compiler

        # 'self.extensions', as supplied by setup.py, is a list of 2-tuples.
        # Each tuple is simple:
        #    (ext_name, build_info)
        # build_info is a dictionary containing everything specific to
        # building this extension.  (Info pertaining to all extensions
        # should be handled by general distutils options passed from
        # setup.py down to right here, but that's not taken care of yet.)

        if not self.extensions:
            return

        # First, sanity-check the 'self.extensions' list
        self.check_extensions_list (self.extensions)

        # Setup the CCompiler object that we'll use to do all the
        # compiling and linking
        self.compiler = new_compiler (plat=os.environ.get ('PLAT'),
                                      verbose=self.verbose,
                                      dry_run=self.dry_run,
                                      force=self.force)
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
        self.build_extensions (self.extensions)

            

    def check_extensions_list (self, extensions):
        """Ensure that the list of extensions (presumably provided as a
           command option 'extensions') is valid, i.e. it is a list of
           2-tuples, where the tuples are (extension_name, build_info_dict).
           Raise DistutilsValueError if the structure is invalid anywhere;
           just returns otherwise."""

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
                      "must be a dictionary (build info)"

        # end sanity-check for

    # check_extensions_list ()


    def get_source_files (self):

        filenames = []

        # Wouldn't it be neat if we knew the names of header files too...
        for (extension_name, build_info) in self.extensions:
            sources = build_info.get ('sources')
            if type (sources) in (ListType, TupleType):
                filenames.extend (sources)

        return filenames


    def build_extensions (self, extensions):

        for (extension_name, build_info) in extensions:
            sources = build_info.get ('sources')
            if sources is None or type (sources) not in (ListType, TupleType):
                raise DistutilsValueError, \
                      ("in 'ext_modules' option (extension '%s'), " +
                       "'sources' must be present and must be " +
                       "a list of source filenames") % extension_name
            sources = list (sources)

            self.announce ("building '%s' extension" % extension_name)

            # First step: compile the source code to object files.  This
            # drops the object files in the current directory, regardless
            # of where the source is (may be a bad thing, but that's how a
            # Makefile.pre.in-based system does it, so at least there's a
            # precedent!)
            macros = build_info.get ('macros')
            include_dirs = build_info.get ('include_dirs')
            self.compiler.compile (sources,
                                   macros=macros,
                                   include_dirs=include_dirs)

            # Now link the object files together into a "shared object" --
            # of course, first we have to figure out all the other things
            # that go into the mix.
            objects = self.compiler.object_filenames (sources)
            extra_objects = build_info.get ('extra_objects')
            if extra_objects:
                objects.extend (extra_objects)
            libraries = build_info.get ('libraries')
            library_dirs = build_info.get ('library_dirs')
            extra_args = build_info.get ('extra_link_args') or []
            if self.compiler.compiler_type == 'msvc':
                extra_args.append ('/export:init%s' % extension_name)

            ext_filename = self.extension_filename \
                           (extension_name, self.package)
            ext_filename = os.path.join (self.build_dir, ext_filename)
            dest_dir = os.path.dirname (ext_filename)
            self.mkpath (dest_dir)
            self.compiler.link_shared_object (objects, ext_filename, 
                                              libraries=libraries,
                                              library_dirs=library_dirs,
                                              extra_postargs=extra_args)

    # build_extensions ()


    def extension_filename (self, ext_name, package=None):
        from distutils import sysconfig
        if package:
            ext_name = package + '.' + ext_name
        ext_path = string.split (ext_name, '.')
        return apply (os.path.join, ext_path) + sysconfig.SO

# class BuildExt
