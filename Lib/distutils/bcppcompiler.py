"""distutils.bcppcompiler

Contains BorlandCCompiler, an implementation of the abstract CCompiler class
for the Borland C++ compiler.
"""

# This implementation by Lyle Johnson, based on the original msvccompiler.py
# module and using the directions originally published by Gordon Williams.

# XXX looks like there's a LOT of overlap between these two classes:
# someone should sit down and factor out the common code as
# WindowsCCompiler!  --GPW

# XXX Lyle reports that this doesn't quite work yet:
# """...but this is what I've got so far. The compile step works fine but
# when it runs the link step I get an "out of memory" failure.  Since
# spawn() echoes the command it's trying to spawn, I can type the link line
# verbatim at the DOS prompt and it links the Windows DLL correctly -- so
# the syntax is correct. There's just some weird interaction going on when
# it tries to "spawn" the link process from within the setup.py script. I'm
# not really sure how to debug this one right off-hand; obviously there's
# nothing wrong with the "spawn()" function since it's working properly for
# the compile stage."""

__revision__ = "$Id$"


import sys, os, string
from distutils.errors import \
     DistutilsExecError, DistutilsPlatformError, \
     CompileError, LibError, LinkError
from distutils.ccompiler import \
     CCompiler, gen_preprocess_options, gen_lib_options


class BCPPCompiler(CCompiler) :
    """Concrete class that implements an interface to the Borland C/C++
    compiler, as defined by the CCompiler abstract class.
    """

    compiler_type = 'bcpp'

    # Just set this so CCompiler's constructor doesn't barf.  We currently
    # don't use the 'set_executables()' bureaucracy provided by CCompiler,
    # as it really isn't necessary for this sort of single-compiler class.
    # Would be nice to have a consistent interface with UnixCCompiler,
    # though, so it's worth thinking about.
    executables = {}

    # Private class data (need to distinguish C from C++ source for compiler)
    _c_extensions = ['.c']
    _cpp_extensions = ['.cc', '.cpp', '.cxx']

    # Needed for the filename generation methods provided by the
    # base class, CCompiler.
    src_extensions = _c_extensions + _cpp_extensions
    obj_extension = '.obj'
    static_lib_extension = '.lib'
    shared_lib_extension = '.dll'
    static_lib_format = shared_lib_format = '%s%s'
    exe_extension = '.exe'


    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):

        CCompiler.__init__ (self, verbose, dry_run, force)

        # These executables are assumed to all be in the path.
        # Borland doesn't seem to use any special registry settings to
        # indicate their installation locations.

        self.cc = "bcc32.exe"
        self.link = "ilink32.exe"
        self.lib = "tlib.exe"

        self.preprocess_options = None
        self.compile_options = ['/tWM', '/O2', '/q']
        self.compile_options_debug = ['/tWM', '/Od', '/q']

        self.ldflags_shared = ['/Tpd', '/Gn', '/q', '/x']
        self.ldflags_shared_debug = ['/Tpd', '/Gn', '/q', '/x']
        self.ldflags_static = []


    # -- Worker methods ------------------------------------------------

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

        if extra_postargs is None:
            extra_postargs = []

        pp_opts = gen_preprocess_options (macros, include_dirs)
        compile_opts = extra_preargs or []
        compile_opts.append ('-c')
        if debug:
            compile_opts.extend (self.compile_options_debug)
        else:
            compile_opts.extend (self.compile_options)
        
        for i in range (len (sources)):
            src = sources[i] ; obj = objects[i]
            ext = (os.path.splitext (src))[1]

            if skip_sources[src]:
                self.announce ("skipping %s (%s up-to-date)" % (src, obj))
            else:
                if ext in self._c_extensions:
                    input_opt = ""
                elif ext in self._cpp_extensions:
                    input_opt = "-P"

                output_opt = "-o" + obj

                self.mkpath (os.path.dirname (obj))

                # Compiler command line syntax is: "bcc32 [options] file(s)".
                # Note that the source file names must appear at the end of
                # the command line.

                try:
                    self.spawn ([self.cc] + compile_opts + pp_opts +
                                [input_opt, output_opt] +
                                extra_postargs + [src])
                except DistutilsExecError, msg:
                    raise CompileError, msg

        return objects

    # compile ()


    def create_static_lib (self,
                           objects,
                           output_libname,
                           output_dir=None,
                           debug=0,
                           extra_preargs=None,
                           extra_postargs=None):

        (objects, output_dir) = self._fix_object_args (objects, output_dir)
        output_filename = \
            self.library_filename (output_libname, output_dir=output_dir)

        if self._need_link (objects, output_filename):
            lib_args = [output_filename, '/u'] + objects
            if debug:
                pass                    # XXX what goes here?
            if extra_preargs:
                lib_args[:0] = extra_preargs
            if extra_postargs:
                lib_args.extend (extra_postargs)
            try:
               self.spawn ([self.lib] + lib_args)
            except DistutilsExecError, msg:
               raise LibError, msg
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
                         export_symbols=None,
                         debug=0,
                         extra_preargs=None,
                         extra_postargs=None,
                         build_temp=None):

        self.link_shared_object (objects,
                                 self.shared_library_name(output_libname),
                                 output_dir=output_dir,
                                 libraries=libraries,
                                 library_dirs=library_dirs,
                                 runtime_library_dirs=runtime_library_dirs,
                                 export_symbols=export_symbols,
                                 debug=debug,
                                 extra_preargs=extra_preargs,
                                 extra_postargs=extra_postargs,
                                 build_temp=build_temp)
                    
    
    def link_shared_object (self,
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

        (objects, output_dir) = self._fix_object_args (objects, output_dir)
        (libraries, library_dirs, runtime_library_dirs) = \
            self._fix_lib_args (libraries, library_dirs, runtime_library_dirs)

        if runtime_library_dirs:
            self.warn ("I don't know what to do with 'runtime_library_dirs': "
                       + str (runtime_library_dirs))
        
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        if self._need_link (objects, output_filename):

            if debug:
                ldflags = self.ldflags_shared_debug
            else:
                ldflags = self.ldflags_shared

            startup_obj = 'c0d32'

            libraries.append ('mypylib')
            libraries.append ('import32')
            libraries.append ('cw32mt')

            # Create a temporary exports file for use by the linker
            head, tail = os.path.split (output_filename)
            modname, ext = os.path.splitext (tail)
            def_file = os.path.join (build_temp, '%s.def' % modname)
            f = open (def_file, 'w')
            f.write ('EXPORTS\n')
            for sym in (export_symbols or []):
                f.write ('  %s=_%s\n' % (sym, sym))

            ld_args = ldflags + [startup_obj] + objects + \
                [',%s,,' % output_filename] + \
                libraries + [',' + def_file]
            
            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend (extra_postargs)

            self.mkpath (os.path.dirname (output_filename))
            try:
                self.spawn ([self.link] + ld_args)
            except DistutilsExecError, msg:
                raise LinkError, msg

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

        if runtime_library_dirs:
            self.warn ("I don't know what to do with 'runtime_library_dirs': "
                       + str (runtime_library_dirs))
        
        lib_opts = gen_lib_options (self,
                                    library_dirs, runtime_library_dirs,
                                    libraries)
        output_filename = output_progname + self.exe_extension
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        if self._need_link (objects, output_filename):

            if debug:
                ldflags = self.ldflags_shared_debug[1:]
            else:
                ldflags = self.ldflags_shared[1:]

            ld_args = ldflags + lib_opts + \
                      objects + ['/OUT:' + output_filename]

            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend (extra_postargs)

            self.mkpath (os.path.dirname (output_filename))
            try:
                self.spawn ([self.link] + ld_args)
            except DistutilsExecError, msg:
                raise LinkError, msg
        else:
            self.announce ("skipping %s (up-to-date)" % output_filename)   
    

    # -- Miscellaneous methods -----------------------------------------
    # These are all used by the 'gen_lib_options() function, in
    # ccompiler.py.

    def library_dir_option (self, dir):
        return "-L" + dir

    def runtime_library_dir_option (self, dir):
        raise DistutilsPlatformError, \
              "don't know how to set runtime library search path for MSVC++"

    def library_option (self, lib):
        return self.library_filename (lib)


    def find_library_file (self, dirs, lib):

        for dir in dirs:
            libfile = os.path.join (dir, self.library_filename (lib))
            if os.path.exists (libfile):
                return libfile

        else:
            # Oops, didn't find it in *any* of 'dirs'
            return None

