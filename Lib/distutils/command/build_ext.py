"""distutils.command.build_ext

Implements the Distutils 'build_ext' command, for building extension
modules (currently limited to C extensions, should accomodate C++
extensions ASAP)."""

# created 1999/08/09, Greg Ward

__revision__ = "$Id$"

import sys, os, string, re
from types import *
from distutils.core import Command
from distutils.errors import *
from distutils.dep_util import newer_group

# An extension name is just a dot-separated list of Python NAMEs (ie.
# the same as a fully-qualified module name).
extension_name_re = re.compile \
    (r'^[a-zA-Z_][a-zA-Z_0-9]*(\.[a-zA-Z_][a-zA-Z_0-9]*)*$')


class build_ext (Command):
    
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
    #     we get to finalize_options() (i.e. the constructor
    #     takes care of both command-line and client options
    #     in between initialize_options() and finalize_options())

    user_options = [
        ('build-lib=', 'b',
         "directory for compiled extension modules"),
        ('build-temp=', 't',
         "directory for temporary files (build by-products)"),
        ('inplace', 'i',
         "ignore build-lib and put compiled extensions into the source" +
         "directory alongside your pure Python modules"),
        ('include-dirs=', 'I',
         "list of directories to search for header files"),
        ('define=', 'D',
         "C preprocessor macros to define"),
        ('undef=', 'U',
         "C preprocessor macros to undefine"),
        ('libraries=', 'l',
         "external C libraries to link with"),
        ('library-dirs=', 'L',
         "directories to search for external C libraries"),
        ('rpath=', 'R',
         "directories to search for shared C libraries at runtime"),
        ('link-objects=', 'O',
         "extra explicit link objects to include in the link"),
        ('debug', 'g',
         "compile/link with debugging information"),
        ('force', 'f',
         "forcibly build everything (ignore file timestamps)"),
        ('compiler=', 'c',
         "specify the compiler type"),
        ]


    def initialize_options (self):
        self.extensions = None
        self.build_lib = None
        self.build_temp = None
        self.inplace = 0
        self.package = None

        self.include_dirs = None
        self.define = None
        self.undef = None
        self.libraries = None
        self.library_dirs = None
        self.rpath = None
        self.link_objects = None
        self.debug = None
        self.force = None
        self.compiler = None


    def finalize_options (self):
        from distutils import sysconfig

        self.set_undefined_options ('build',
                                    ('build_lib', 'build_lib'),
                                    ('build_temp', 'build_temp'),
                                    ('compiler', 'compiler'),
                                    ('debug', 'debug'),
                                    ('force', 'force'))

        if self.package is None:
            self.package = self.distribution.ext_package

        self.extensions = self.distribution.ext_modules
        

        # Make sure Python's include directories (for Python.h, config.h,
        # etc.) are in the include search path.
        py_include = sysconfig.get_python_inc()
        plat_py_include = sysconfig.get_python_inc(plat_specific=1)
        if self.include_dirs is None:
            self.include_dirs = self.distribution.include_dirs or []
        if type (self.include_dirs) is StringType:
            self.include_dirs = string.split (self.include_dirs,
                                              os.pathsep)

        # Put the Python "system" include dir at the end, so that
        # any local include dirs take precedence.
        self.include_dirs.append (py_include)
        if plat_py_include != py_include:
            self.include_dirs.append (plat_py_include)

        if type (self.libraries) is StringType:
            self.libraries = [self.libraries]

        # Life is easier if we're not forever checking for None, so
        # simplify these options to empty lists if unset
        if self.libraries is None:
            self.libraries = []
        if self.library_dirs is None:
            self.library_dirs = []
        if self.rpath is None:
            self.rpath = []

        # for extensions under windows use different directories
        # for Release and Debug builds.
        # also Python's library directory must be appended to library_dirs
        if os.name == 'nt':
            self.library_dirs.append (os.path.join(sys.exec_prefix, 'libs'))
            if self.debug:
                self.build_temp = os.path.join (self.build_temp, "Debug")
            else:
                self.build_temp = os.path.join (self.build_temp, "Release")
    # finalize_options ()
    

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

        # If we were asked to build any C/C++ libraries, make sure that the
        # directory where we put them is in the library search path for
        # linking extensions.
        if self.distribution.has_c_libraries():
            build_clib = self.find_peer ('build_clib')
            self.libraries.extend (build_clib.get_library_names() or [])
            self.library_dirs.append (build_clib.build_clib)

        # Setup the CCompiler object that we'll use to do all the
        # compiling and linking
        self.compiler = new_compiler (compiler=self.compiler,
                                      verbose=self.verbose,
                                      dry_run=self.dry_run,
                                      force=self.force)

        # And make sure that any compile/link-related options (which might
        # come from the command-line or from the setup script) are set in
        # that CCompiler object -- that way, they automatically apply to
        # all compiling and linking done here.
        if self.include_dirs is not None:
            self.compiler.set_include_dirs (self.include_dirs)
        if self.define is not None:
            # 'define' option is a list of (name,value) tuples
            for (name,value) in self.define:
                self.compiler.define_macro (name, value)
        if self.undef is not None:
            for macro in self.undef:
                self.compiler.undefine_macro (macro)
        if self.libraries is not None:
            self.compiler.set_libraries (self.libraries)
        if self.library_dirs is not None:
            self.compiler.set_library_dirs (self.library_dirs)
        if self.rpath is not None:
            self.compiler.set_runtime_library_dirs (self.rpath)
        if self.link_objects is not None:
            self.compiler.set_link_objects (self.link_objects)

        # Now actually compile and link everything.
        self.build_extensions ()

    # run ()


    def check_extensions_list (self, extensions):
        """Ensure that the list of extensions (presumably provided as a
           command option 'extensions') is valid, i.e. it is a list of
           2-tuples, where the tuples are (extension_name, build_info_dict).
           Raise DistutilsSetupError if the structure is invalid anywhere;
           just returns otherwise."""

        if type (extensions) is not ListType:
            raise DistutilsSetupError, \
                  "'ext_modules' option must be a list of tuples"
        
        for ext in extensions:
            if type (ext) is not TupleType and len (ext) != 2:
                raise DistutilsSetupError, \
                      "each element of 'ext_modules' option must be a 2-tuple"

            if not (type (ext[0]) is StringType and
                    extension_name_re.match (ext[0])):
                raise DistutilsSetupError, \
                      "first element of each tuple in 'ext_modules' " + \
                      "must be the extension name (a string)"

            if type (ext[1]) is not DictionaryType:
                raise DistutilsSetupError, \
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


    def get_outputs (self):

        # Sanity check the 'extensions' list -- can't assume this is being
        # done in the same run as a 'build_extensions()' call (in fact, we
        # can probably assume that it *isn't*!).
        self.check_extensions_list (self.extensions)

        # And build the list of output (built) filenames.  Note that this
        # ignores the 'inplace' flag, and assumes everything goes in the
        # "build" tree.
        outputs = []
        for (extension_name, build_info) in self.extensions:
            fullname = self.get_ext_fullname (extension_name)
            outputs.append (os.path.join (self.build_lib,
                                          self.get_ext_filename(fullname)))
        return outputs

    # get_outputs ()


    def build_extensions (self):

        # First, sanity-check the 'extensions' list
        self.check_extensions_list (self.extensions)

        for (extension_name, build_info) in self.extensions:
            sources = build_info.get ('sources')
            if sources is None or type (sources) not in (ListType, TupleType):
                raise DistutilsSetupError, \
                      ("in 'ext_modules' option (extension '%s'), " +
                       "'sources' must be present and must be " +
                       "a list of source filenames") % extension_name
            sources = list (sources)

            fullname = self.get_ext_fullname (extension_name)
            if self.inplace:
                # ignore build-lib -- put the compiled extension into
                # the source tree along with pure Python modules

                modpath = string.split (fullname, '.')
                package = string.join (modpath[0:-1], '.')
                base = modpath[-1]

                build_py = self.find_peer ('build_py')
                package_dir = build_py.get_package_dir (package)
                ext_filename = os.path.join (package_dir,
                                             self.get_ext_filename(base))
            else:
                ext_filename = os.path.join (self.build_lib,
                                             self.get_ext_filename(fullname))

	    if not newer_group(sources, ext_filename, 'newer'):
	    	self.announce ("skipping '%s' extension (up-to-date)" %
                               extension_name)
		continue # 'for' loop over all extensions
	    else:
        	self.announce ("building '%s' extension" % extension_name)

            # First step: compile the source code to object files.  This
            # drops the object files in the current directory, regardless
            # of where the source is (may be a bad thing, but that's how a
            # Makefile.pre.in-based system does it, so at least there's a
            # precedent!)
            macros = build_info.get ('macros')
            include_dirs = build_info.get ('include_dirs')
            extra_args = build_info.get ('extra_compile_args')
            # honor CFLAGS enviroment variable
            # XXX do we *really* need this? or is it just a hack until
            # the user can put compiler flags in setup.cfg?
            if os.environ.has_key('CFLAGS'):
                if not extra_args:
                    extra_args = []
                extra_args = string.split(os.environ['CFLAGS']) + extra_args
                
            objects = self.compiler.compile (sources,
                                             output_dir=self.build_temp,
                                             macros=macros,
                                             include_dirs=include_dirs,
                                             debug=self.debug,
                                             extra_postargs=extra_args)

            # Now link the object files together into a "shared object" --
            # of course, first we have to figure out all the other things
            # that go into the mix.
            extra_objects = build_info.get ('extra_objects')
            if extra_objects:
                objects.extend (extra_objects)
            libraries = build_info.get ('libraries')
            library_dirs = build_info.get ('library_dirs')
            rpath = build_info.get ('rpath')
            extra_args = build_info.get ('extra_link_args') or []

            # XXX this is a kludge!  Knowledge of specific compilers or
            # platforms really doesn't belong here; in an ideal world, the
            # CCompiler interface would provide access to everything in a
            # compiler/linker system needs to build Python extensions, and
            # we would just do everything nicely and cleanly through that
            # interface.  However, this is a not an ideal world and the
            # CCompiler interface doesn't handle absolutely everything.
            # Thus, kludges like this slip in occasionally.  (This is no
            # excuse for committing more platform- and compiler-specific
            # kludges; they are to be avoided if possible!)
            if self.compiler.compiler_type == 'msvc':
                def_file = build_info.get ('def_file')
                if def_file is None:
                    source_dir = os.path.dirname (sources[0])
                    ext_base = (string.split (extension_name, '.'))[-1]
                    def_file = os.path.join (source_dir, "%s.def" % ext_base)
                    if not os.path.exists (def_file):
                        def_file = None

                if def_file is not None:
                    extra_args.append ('/DEF:' + def_file)
                else:
                    modname = string.split (extension_name, '.')[-1]
                    extra_args.append('/export:init%s'%modname)

                # The MSVC linker generates unneeded .lib and .exp files,
                # which cannot be suppressed by any linker switches.  So
                # make sure they are generated in the temporary build
                # directory.
                implib_file = os.path.join (
                    self.build_temp,
                    self.get_ext_libname (extension_name))
                extra_args.append ('/IMPLIB:' + implib_file)
                self.mkpath (os.path.dirname (implib_file))
            # if MSVC

            self.compiler.link_shared_object (objects, ext_filename, 
                                              libraries=libraries,
                                              library_dirs=library_dirs,
                                              runtime_library_dirs=rpath,
                                              extra_postargs=extra_args,
                                              debug=self.debug)

    # build_extensions ()


    def get_ext_fullname (self, ext_name):
        if self.package is None:
            return ext_name
        else:
            return self.package + '.' + ext_name

    def get_ext_filename (self, ext_name):
        from distutils import sysconfig
        ext_path = string.split (ext_name, '.')
        # extensions in debug_mode are named 'module_d.pyd' under windows
        if os.name == 'nt' and self.debug:
            return apply (os.path.join, ext_path) + '_d' + sysconfig.SO
        return apply (os.path.join, ext_path) + sysconfig.SO

    def get_ext_libname (self, ext_name):
        # create a filename for the (unneeded) lib-file.
        # extensions in debug_mode are named 'module_d.pyd' under windows
        ext_path = string.split (ext_name, '.')
        if os.name == 'nt' and self.debug:
            return apply (os.path.join, ext_path) + '_d.lib'
        return apply (os.path.join, ext_path) + '.lib'

# class build_ext
