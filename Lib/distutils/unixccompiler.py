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
from sysconfig import \
     CC, CCSHARED, CFLAGS, OPT, LDSHARED, LDFLAGS, RANLIB, AR, SO
from ccompiler import CCompiler, gen_preprocess_options, gen_lib_options
from util import move_file, newer_pairwise, newer_group

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

    _obj_ext = '.o'
    _exe_ext = ''
    _shared_lib_ext = SO
    _static_lib_ext = '.a'

    # Command to create a static library: seems to be pretty consistent
    # across the major Unices.  Might have to move down into the
    # constructor if we need platform-specific guesswork.
    archiver = "ar"
    archiver_options = "-cr"


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
            _split_command (CC + ' ' + OPT)
        self.ccflags_shared = string.split (CCSHARED)

        (self.ld_shared, self.ldflags_shared) = \
            _split_command (LDSHARED)

        self.ld_exec = self.cc


    def compile (self,
                 sources,
                 output_dir=None,
                 keep_dir=0,
                 macros=None,
                 include_dirs=None,
                 debug=0,
                 extra_preargs=None,
                 extra_postargs=None):

        if type (output_dir) not in (StringType, NoneType):
            raise TypeError, "'output_dir' must be a string or None"
        if output_dir is None:
            output_dir = self.output_dir
        if macros is None:
            macros = []
        if include_dirs is None:
            include_dirs = []

        if type (macros) is not ListType:
            raise TypeError, \
                  "'macros' (if supplied) must be a list of tuples"
        if type (include_dirs) not in (ListType, TupleType):
            raise TypeError, \
                  "'include_dirs' (if supplied) must be a list of strings"
        include_dirs = list (include_dirs)

        pp_opts = gen_preprocess_options (self.macros + macros,
                                          self.include_dirs + include_dirs)

        # So we can mangle 'sources' without hurting the caller's data
        orig_sources = sources
        sources = copy (sources)

        # Get the list of expected output (object) files and drop files we
        # don't have to recompile.  (Simplistic check -- we just compare the
        # source and object file, no deep dependency checking involving
        # header files.  Hmmm.)
        objects = self.object_filenames (sources,
                                         output_dir=output_dir,
                                         keep_dir=keep_dir)
        all_objects = copy (objects)    # preserve full list to return

        if not self.force:
            skipped = newer_pairwise (sources, objects)
            for skipped_pair in skipped:
                self.announce ("skipping %s (%s up-to-date)" % skipped_pair)

        # Build list of (source,object) tuples for convenience
        srcobj = []
        for i in range (len (sources)):
            srcobj.append ((sources[i], objects[i]))

        # Compile all source files that weren't eliminated by
        # 'newer_pairwise()'.
        # XXX use of ccflags_shared means we're blithely assuming
        # that we're compiling for inclusion in a shared object!
        # (will have to fix this when I add the ability to build a
        # new Python)
        cc_args = ['-c'] + pp_opts + self.ccflags + self.ccflags_shared
        if debug:
            cc_args[:0] = ['-g']
        if extra_preargs:
            cc_args[:0] = extra_preargs
        if extra_postargs is None:
            extra_postargs = []

        if output_dir is not None:
            self.mkpath (output_dir)
        for (source,object) in srcobj:
            self.spawn ([self.cc] + cc_args +
                        [source, '-o', object] +
                        extra_postargs)

        # Have to re-fetch list of object filenames, because we want to
        # return *all* of them, including those that weren't recompiled on
        # this call!
        return all_objects


    def _fix_link_args (self, output_dir, libraries, library_dirs):
        """Fixes up the arguments supplied to the 'link_*' methods:
           if output_dir is None, use self.output_dir; ensure that
           libraries and library_dirs are both lists (could be None or
           tuples on input -- both are converted to lists).  Return
           a tuple of the three input arguments."""

        if output_dir is None:
            output_dir = self.output_dir
        if libraries is None:
            libraries = []
        if library_dirs is None:
            library_dirs = []
        
        if type (libraries) not in (ListType, TupleType):
            raise TypeError, \
                  "'libraries' (if supplied) must be a list of strings"
        if type (library_dirs) not in (ListType, TupleType):
            raise TypeError, \
                  "'library_dirs' (if supplied) must be a list of strings"
        libraries = list (libraries)
        library_dirs = list (library_dirs)

        return (output_dir, libraries, library_dirs)


    def link_static_lib (self,
                         objects,
                         output_libname,
                         output_dir=None,
                         debug=0):

        if type (objects) not in (ListType, TupleType):
            raise TypeError, \
                  "'objects' must be a list or tuple of strings"
        objects = list (objects)
            
        if type (output_dir) not in (StringType, NoneType):
            raise TypeError, "'output_dir' must be a string or None"
        if output_dir is None:
            output_dir = self.output_dir

        output_filename = self.library_filename (output_libname)
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        # Check timestamps: if any of the object files are newer than
        # the library file, *or* if "force" is true, then we'll
        # recreate the library.
        if not self.force:
            if self.dry_run:
                newer = newer_group (objects, output_filename, missing='newer')
            else:
                newer = newer_group (objects, output_filename)

        if self.force or newer:
            self.mkpath (os.path.dirname (output_filename))
            self.spawn ([self.archiver,
                         self.archiver_options,
                         output_filename] +
                        objects)
        else:
            self.announce ("skipping %s (up-to-date)" % output_filename)

    # link_static_lib ()


    def link_shared_lib (self,
                         objects,
                         output_libname,
                         output_dir=None,
                         libraries=None,
                         library_dirs=None,
                         debug=0,
                         extra_preargs=None,
                         extra_postargs=None):
        # XXX should we sanity check the library name? (eg. no
        # slashes)
        self.link_shared_object (
            objects,
            "lib%s%s" % (output_libname, self._shared_lib_ext),
            output_dir,
            libraries,
            library_dirs,
            debug,
            extra_preargs,
            extra_postargs)
        

    def link_shared_object (self,
                            objects,
                            output_filename,
                            output_dir=None,
                            libraries=None,
                            library_dirs=None,
                            debug=0,
                            extra_preargs=None,
                            extra_postargs=None):

        (output_dir, libraries, library_dirs) = \
            self._fix_link_args (output_dir, libraries, library_dirs)

        lib_opts = gen_lib_options (self,
                                    self.library_dirs + library_dirs,
                                    self.libraries + libraries)
        if type (output_dir) not in (StringType, NoneType):
            raise TypeError, "'output_dir' must be a string or None"
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        # If any of the input object files are newer than the output shared
        # object, relink.  Again, this is a simplistic dependency check:
        # doesn't look at any of the libraries we might be linking with.

        if not self.force:
            if self.dry_run:
                newer = newer_group (objects, output_filename, missing='newer')
            else:
                newer = newer_group (objects, output_filename)

        if self.force or newer:
            ld_args = self.ldflags_shared + objects + \
                      lib_opts + ['-o', output_filename]
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
                         debug=0,
                         extra_preargs=None,
                         extra_postargs=None):
    
        (output_dir, libraries, library_dirs) = \
            self._fix_link_args (output_dir, libraries, library_dirs)

        lib_opts = gen_lib_options (self,
                                    self.library_dirs + library_dirs,
                                    self.libraries + libraries)
        output_filename = output_progname # Unix-ism!
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        # Same ol' simplistic-but-still-useful dependency check.
        if not self.force:
            if self.dry_run:
                newer = newer_group (objects, output_filename, missing='newer')
            else:
                newer = newer_group (objects, output_filename)

        if self.force or newer:
            ld_args = objects + lib_opts + ['-o', output_filename]
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


    # -- Filename-mangling (etc.) methods ------------------------------

    def object_filenames (self, source_filenames,
                          keep_dir=0, output_dir=None):
        outnames = []
        for inname in source_filenames:
            outname = re.sub (r'\.(c|C|cc|cxx|cpp)$', self._obj_ext, inname)
            if not keep_dir:
                outname = os.path.basename (outname)
            if output_dir is not None:
                outname = os.path.join (output_dir, outname)
            outnames.append (outname)
        return outnames

    def shared_object_filename (self, source_filename,
                                keep_dir=0, output_dir=None):
        outname = re.sub (r'\.(c|C|cc|cxx|cpp)$', self._shared_lib_ext)
        if not keep_dir:
            outname = os.path.basename (outname)
        if output_dir is not None:
            outname = os.path.join (output_dir, outname)
        return outname


    def library_filename (self, libname):
        (dirname, basename) = os.path.split (libname)
        return os.path.join (dirname,
                             "lib%s%s" % (basename, self._static_lib_ext))

    def shared_library_filename (self, libname):
        (dirname, basename) = os.path.split (libname)
        return os.path.join (dirname,
                             "lib%s%s" % (basename, self._shared_lib_ext))


    def library_dir_option (self, dir):
        return "-L" + dir

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
