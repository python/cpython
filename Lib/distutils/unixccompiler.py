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
from distutils import sysconfig
from distutils.ccompiler import CCompiler, gen_preprocess_options, gen_lib_options

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

    # XXX perhaps there should really be *three* kinds of include
    # directories: those built in to the preprocessor, those from Python's
    # Makefiles, and those supplied to {add,set}_include_dirs().  Currently
    # we make no distinction between the latter two at this point; it's all
    # up to the client class to select the include directories to use above
    # and beyond the compiler's defaults.  That is, both the Python include
    # directories and any module- or package-specific include directories
    # are specified via {add,set}_include_dirs(), and there's no way to
    # distinguish them.  This might be a bug.

    compiler_type = 'unix'

    # Needed for the filename generation methods provided by the
    # base class, CCompiler.
    src_extensions = [".c",".C",".cc",".cxx",".cpp"]
    obj_extension = ".o"
    static_lib_extension = ".a"
    shared_lib_extension = sysconfig.SO
    static_lib_format = shared_lib_format = "lib%s%s"

    # Command to create a static library: seems to be pretty consistent
    # across the major Unices.  Might have to move down into the
    # constructor if we need platform-specific guesswork.
    archiver = sysconfig.AR
    archiver_options = "-cr"
    ranlib = sysconfig.RANLIB


    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):

        CCompiler.__init__ (self, verbose, dry_run, force)

        self.preprocess_options = None
        self.compile_options = None

        # Munge CC and OPT together in case there are flags stuck in CC.
        # Note that using these variables from sysconfig immediately makes
        # this module specific to building Python extensions and
        # inappropriate as a general-purpose C compiler front-end.  So sue
        # me.  Note also that we use OPT rather than CFLAGS, because CFLAGS
        # is the flags used to compile Python itself -- not only are there
        # -I options in there, they are the *wrong* -I options.  We'll
        # leave selection of include directories up to the class using
        # UnixCCompiler!

        (self.cc, self.ccflags) = \
            _split_command (sysconfig.CC + ' ' + sysconfig.OPT)
        self.ccflags_shared = string.split (sysconfig.CCSHARED)

        (self.ld_shared, self.ldflags_shared) = \
            _split_command (sysconfig.LDSHARED)

        self.ld_exec = self.cc

    # __init__ ()


    def compile (self,
                 sources,
                 output_dir=None,
                 macros=None,
                 include_dirs=None,
                 debug=0,
                 extra_preargs=None,
                 extra_postargs=None):

        (output_dir, macros, include_dirs) = \
            self._fix_compile_args (output_dir, macros, include_dirs)
        (objects, skip_sources) = self._prep_compile (sources, output_dir)

        # Figure out the options for the compiler command line.
        pp_opts = gen_preprocess_options (macros, include_dirs)
        cc_args = ['-c'] + pp_opts + self.ccflags + self.ccflags_shared
        if debug:
            cc_args[:0] = ['-g']
        if extra_preargs:
            cc_args[:0] = extra_preargs
        if extra_postargs is None:
            extra_postargs = []

        # Compile all source files that weren't eliminated by
        # '_prep_compile()'.        
        for i in range (len (sources)):
            src = sources[i] ; obj = objects[i]
            if skip_sources[src]:
                self.announce ("skipping %s (%s up-to-date)" % (src, obj))
            else:
                self.mkpath (os.path.dirname (obj))
                self.spawn ([self.cc] + cc_args + [src, '-o', obj] + extra_postargs)

        # Return *all* object filenames, not just the ones we just built.
        return objects

    # compile ()
    

    def create_static_lib (self,
                           objects,
                           output_libname,
                           output_dir=None,
                           debug=0):

        (objects, output_dir) = self._fix_object_args (objects, output_dir)

        output_filename = \
            self.library_filename (output_libname, output_dir=output_dir)

        if self._need_link (objects, output_filename):
            self.mkpath (os.path.dirname (output_filename))
            self.spawn ([self.archiver,
                         self.archiver_options,
                         output_filename] +
                        objects + self.objects)

            # Not many Unices required ranlib anymore -- SunOS 4.x is,
            # I think the only major Unix that does.  Probably should
            # have some platform intelligence here to skip ranlib if
            # it's not needed.
            self.spawn ([self.ranlib, output_filename])
        else:
            self.announce ("skipping %s (up-to-date)" % output_filename)

    # create_static_lib ()


    def link_shared_lib (self,
                         objects,
                         output_libname,
                         output_dir=None,
                         libraries=None,
                         library_dirs=None,
                         runtime_library_dirs=None,
                         debug=0,
                         extra_preargs=None,
                         extra_postargs=None):
        self.link_shared_object (
            objects,
            self.shared_library_filename (output_libname),
            output_dir,
            libraries,
            library_dirs,
            runtime_library_dirs,
            debug,
            extra_preargs,
            extra_postargs)
        

    def link_shared_object (self,
                            objects,
                            output_filename,
                            output_dir=None,
                            libraries=None,
                            library_dirs=None,
                            runtime_library_dirs=None,
                            debug=0,
                            extra_preargs=None,
                            extra_postargs=None):

        (objects, output_dir) = self._fix_object_args (objects, output_dir)
        (libraries, library_dirs, runtime_library_dirs) = \
            self._fix_lib_args (libraries, library_dirs, runtime_library_dirs)

        lib_opts = gen_lib_options (self,
                                    library_dirs, runtime_library_dirs,
                                    libraries)
        if type (output_dir) not in (StringType, NoneType):
            raise TypeError, "'output_dir' must be a string or None"
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        if self._need_link (objects, output_filename):
            ld_args = (self.ldflags_shared + objects + self.objects + 
                       lib_opts + ['-o', output_filename])
            if debug:
                ld_args[:0] = ['-g']
            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend (extra_postargs)
            self.mkpath (os.path.dirname (output_filename))
            self.spawn ([self.ld_shared] + ld_args)
        else:
            self.announce ("skipping %s (up-to-date)" % output_filename)

    # link_shared_object ()


    def link_executable (self,
                         objects,
                         output_progname,
                         output_dir=None,
                         libraries=None,
                         library_dirs=None,
                         runtime_library_dirs=None,
                         debug=0,
                         extra_preargs=None,
                         extra_postargs=None):
    
        (objects, output_dir) = self._fix_object_args (objects, output_dir)
        (libraries, library_dirs, runtime_library_dirs) = \
            self._fix_lib_args (libraries, library_dirs, runtime_library_dirs)

        lib_opts = gen_lib_options (self,
                                    library_dirs, runtime_library_dirs,
                                    libraries)
        output_filename = output_progname # Unix-ism!
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        if self._need_link (objects, output_filename):
            ld_args = objects + self.objects + lib_opts + ['-o', output_filename]
            if debug:
                ld_args[:0] = ['-g']
            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend (extra_postargs)
            self.mkpath (os.path.dirname (output_filename))
            self.spawn ([self.ld_exec] + ld_args)
        else:
            self.announce ("skipping %s (up-to-date)" % output_filename)

    # link_executable ()


    # -- Miscellaneous methods -----------------------------------------
    # These are all used by the 'gen_lib_options() function, in
    # ccompiler.py.
    
    def library_dir_option (self, dir):
        return "-L" + dir

    def runtime_library_dir_option (self, dir):
        return "-R" + dir

    def library_option (self, lib):
        return "-l" + lib


    def find_library_file (self, dirs, lib):

        for dir in dirs:
            shared = os.path.join (dir, self.shared_library_filename (lib))
            static = os.path.join (dir, self.library_filename (lib))

            # We're second-guessing the linker here, with not much hard
            # data to go on: GCC seems to prefer the shared library, so I'm
            # assuming that *all* Unix C compilers do.  And of course I'm
            # ignoring even GCC's "-static" option.  So sue me.
            if os.path.exists (shared):
                return shared
            elif os.path.exists (static):
                return static

        else:
            # Oops, didn't find it in *any* of 'dirs'
            return None

    # find_library_file ()

# class UnixCCompiler


def _split_command (cmd):
    """Split a command string up into the progam to run (a string) and
       the list of arguments; return them as (cmd, arglist)."""
    args = string.split (cmd)
    return (args[0], args[1:])
