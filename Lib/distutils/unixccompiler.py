"""distutils.unixccompiler

Contains the UnixCCompiler class, a subclass of CCompiler that handles
the "typical" Unix-style command-line C compiler:
  * macros defined with -Dname[=value]
  * macros undefined with -Uname
  * include search directories specified with -Idir
  * libraries specified with -lllib
  * library search directories specified with -Ldir
  * compile handled by 'cc' (or similar) executable with -c option:
    compiles .c to .o
  * link static library handled by 'ar' command (possibly with 'ranlib')
  * link shared library handled by 'cc -shared'
"""

# created 1999/07/05, Greg Ward

__revision__ = "$Id$"

import string, re, os
from types import *
from copy import copy
from distutils.dep_util import newer
from distutils.ccompiler import \
     CCompiler, gen_preprocess_options, gen_lib_options
from distutils.errors import \
     DistutilsExecError, CompileError, LibError, LinkError

# XXX Things not currently handled:
#   * optimization/debug/warning flags; we just use whatever's in Python's
#     Makefile and live with it.  Is this adequate?  If not, we might
#     have to have a bunch of subclasses GNUCCompiler, SGICCompiler,
#     SunCCompiler, and I suspect down that road lies madness.
#   * even if we don't know a warning flag from an optimization flag,
#     we need some way for outsiders to feed preprocessor/compiler/linker
#     flags in to us -- eg. a sysadmin might want to mandate certain flags
#     via a site config file, or a user might want to set something for
#     compiling this module distribution only via the setup.py command
#     line, whatever.  As long as these options come from something on the
#     current system, they can be as system-dependent as they like, and we
#     should just happily stuff them into the preprocessor/compiler/linker
#     options and carry on.


class UnixCCompiler (CCompiler):

    compiler_type = 'unix'

    # These are used by CCompiler in two places: the constructor sets
    # instance attributes 'preprocessor', 'compiler', etc. from them, and
    # 'set_executable()' allows any of these to be set.  The defaults here
    # are pretty generic; they will probably have to be set by an outsider
    # (eg. using information discovered by the sysconfig about building
    # Python extensions).
    executables = {'preprocessor' : None,
                   'compiler'     : ["cc"],
                   'compiler_so'  : ["cc"],
                   'linker_so'    : ["cc", "-shared"],
                   'linker_exe'   : ["cc"],
                   'archiver'     : ["ar", "-cr"],
                   'ranlib'       : None,
                  }

    # Needed for the filename generation methods provided by the base
    # class, CCompiler.  NB. whoever instantiates/uses a particular
    # UnixCCompiler instance should set 'shared_lib_ext' -- we set a
    # reasonable common default here, but it's not necessarily used on all
    # Unices!

    src_extensions = [".c",".C",".cc",".cxx",".cpp",".m"]
    obj_extension = ".o"
    static_lib_extension = ".a"
    shared_lib_extension = ".so"
    static_lib_format = shared_lib_format = "lib%s%s"



    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):
        CCompiler.__init__ (self, verbose, dry_run, force)


    def preprocess (self,
                    source,
                    output_file=None,
                    macros=None,
                    include_dirs=None,
                    extra_preargs=None,
                    extra_postargs=None):

        (_, macros, include_dirs) = \
            self._fix_compile_args(None, macros, include_dirs)
        pp_opts = gen_preprocess_options(macros, include_dirs)
        pp_args = self.preprocessor + pp_opts
        if output_file:
            pp_args.extend(['-o', output_file])
        if extra_preargs:
            pp_args[:0] = extra_preargs
        if extra_postargs:
            pp_args.extend(extra_postargs)

        # We need to preprocess: either we're being forced to, or we're
        # generating output to stdout, or there's a target output file and 
        # the source file is newer than the target (or the target doesn't
        # exist).
        if self.force or output_file is None or newer(source, output_file):
            if output_file:
                self.mkpath(os.path.dirname(output_file))
            try:
                self.spawn(pp_args)
            except DistutilsExecError, msg:
                raise CompileError, msg


    def compile (self,
                 sources,
                 output_dir=None,
                 macros=None,
                 include_dirs=None,
                 debug=0,
                 extra_preargs=None,
                 extra_postargs=None):

        (output_dir, macros, include_dirs) = \
            self._fix_compile_args(output_dir, macros, include_dirs)
        (objects, skip_sources) = self._prep_compile(sources, output_dir)

        # Figure out the options for the compiler command line.
        pp_opts = gen_preprocess_options(macros, include_dirs)
        cc_args = pp_opts + ['-c']
        if debug:
            cc_args[:0] = ['-g']
        if extra_preargs:
            cc_args[:0] = extra_preargs
        if extra_postargs is None:
            extra_postargs = []

        # Compile all source files that weren't eliminated by
        # '_prep_compile()'.        
        for i in range(len(sources)):
            src = sources[i] ; obj = objects[i]
            if skip_sources[src]:
                self.announce("skipping %s (%s up-to-date)" % (src, obj))
            else:
                self.mkpath(os.path.dirname(obj))
                try:
                    self.spawn(self.compiler_so + cc_args +
                               [src, '-o', obj] +
                               extra_postargs)
                except DistutilsExecError, msg:
                    raise CompileError, msg

        # Return *all* object filenames, not just the ones we just built.
        return objects

    # compile ()
    

    def create_static_lib (self,
                           objects,
                           output_libname,
                           output_dir=None,
                           debug=0):

        (objects, output_dir) = self._fix_object_args(objects, output_dir)

        output_filename = \
            self.library_filename(output_libname, output_dir=output_dir)

        if self._need_link(objects, output_filename):
            self.mkpath(os.path.dirname(output_filename))
            self.spawn(self.archiver +
                       [output_filename] +
                       objects + self.objects)

            # Not many Unices required ranlib anymore -- SunOS 4.x is, I
            # think the only major Unix that does.  Maybe we need some
            # platform intelligence here to skip ranlib if it's not
            # needed -- or maybe Python's configure script took care of
            # it for us, hence the check for leading colon.
            if self.ranlib:
                try:
                    self.spawn(self.ranlib + [output_filename])
                except DistutilsExecError, msg:
                    raise LibError, msg
        else:
            self.announce("skipping %s (up-to-date)" % output_filename)

    # create_static_lib ()


    def link (self,
              target_desc,    
              objects,
              output_filename,
              output_dir=None,
              libraries=None,
              library_dirs=None,
              runtime_library_dirs=None,
              export_symbols=None,
              debug=0,
              extra_preargs=None,
              extra_postargs=None,
              build_temp=None):

        (objects, output_dir) = self._fix_object_args(objects, output_dir)
        (libraries, library_dirs, runtime_library_dirs) = \
            self._fix_lib_args(libraries, library_dirs, runtime_library_dirs)

        lib_opts = gen_lib_options(self,
                                   library_dirs, runtime_library_dirs,
                                   libraries)
        if type(output_dir) not in (StringType, NoneType):
            raise TypeError, "'output_dir' must be a string or None"
        if output_dir is not None:
            output_filename = os.path.join(output_dir, output_filename)

        if self._need_link(objects, output_filename):
            ld_args = (objects + self.objects + 
                       lib_opts + ['-o', output_filename])
            if debug:
                ld_args[:0] = ['-g']
            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend(extra_postargs)
            self.mkpath(os.path.dirname(output_filename))
            try:
                if target_desc == CCompiler.EXECUTABLE:    
                    self.spawn(self.linker_exe + ld_args)
                else:
                    self.spawn(self.linker_so + ld_args)
            except DistutilsExecError, msg:
                raise LinkError, msg
        else:
            self.announce("skipping %s (up-to-date)" % output_filename)

    # link ()


    # -- Miscellaneous methods -----------------------------------------
    # These are all used by the 'gen_lib_options() function, in
    # ccompiler.py.
    
    def library_dir_option (self, dir):
        return "-L" + dir

    def runtime_library_dir_option (self, dir):
        return "-R" + dir

    def library_option (self, lib):
        return "-l" + lib


    def find_library_file (self, dirs, lib, debug=0):

        for dir in dirs:
            shared = os.path.join(
                dir, self.library_filename(lib, lib_type='shared'))
            static = os.path.join(
                dir, self.library_filename(lib, lib_type='static'))

            # We're second-guessing the linker here, with not much hard
            # data to go on: GCC seems to prefer the shared library, so I'm
            # assuming that *all* Unix C compilers do.  And of course I'm
            # ignoring even GCC's "-static" option.  So sue me.
            if os.path.exists(shared):
                return shared
            elif os.path.exists(static):
                return static

        else:
            # Oops, didn't find it in *any* of 'dirs'
            return None

    # find_library_file ()

# class UnixCCompiler
