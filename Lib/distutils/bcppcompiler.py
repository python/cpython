"""distutils.bcppcompiler

Contains BorlandCCompiler, an implementation of the abstract CCompiler class
for the Borland C++ compiler.
"""

# This implementation by Lyle Johnson, based on the original msvccompiler.py
# module and using the directions originally published by Gordon Williams.

# XXX looks like there's a LOT of overlap between these two classes:
# someone should sit down and factor out the common code as
# WindowsCCompiler!  --GPW

__revision__ = "$Id$"


import sys, os, string
from distutils.errors import \
     DistutilsExecError, DistutilsPlatformError, \
     CompileError, LibError, LinkError
from distutils.ccompiler import \
     CCompiler, gen_preprocess_options, gen_lib_options
from distutils.file_util import write_file


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

                src = os.path.normpath(src)
                obj = os.path.normpath(obj)
                
                output_opt = "-o" + obj
                self.mkpath(os.path.dirname(obj))

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

        # XXX this ignores 'build_temp'!  should follow the lead of
        # msvccompiler.py

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
                ld_args = self.ldflags_shared_debug[:]
            else:
                ld_args = self.ldflags_shared[:]

            # Borland C++ has problems with '/' in paths
            objects = map(os.path.normpath, objects)
            startup_obj = 'c0d32'
            objects.insert(0, startup_obj)

            # either exchange python15.lib in the python libs directory against
            # a Borland-like one, or create one with name bcpp_python15.lib 
            # there and remove the pragmas from config.h  
            #libraries.append ('mypylib')            
            libraries.append ('import32')
            libraries.append ('cw32mt')

            # Create a temporary exports file for use by the linker
            head, tail = os.path.split (output_filename)
            modname, ext = os.path.splitext (tail)
            temp_dir = os.path.dirname(objects[0]) # preserve tree structure
            def_file = os.path.join (temp_dir, '%s.def' % modname)
            contents = ['EXPORTS']
            for sym in (export_symbols or []):
                contents.append('  %s=_%s' % (sym, sym))
            self.execute(write_file, (def_file, contents),
                         "writing %s" % def_file)

            # Start building command line flags and options.

            for l in library_dirs:
                ld_args.append("/L%s" % os.path.normpath(l)) 
                
            ld_args.extend(objects)     # list of object files

            # name of dll file
            ld_args.extend([',',output_filename])
            # no map file and start libraries 
            ld_args.extend([',', ','])

            for lib in libraries:
                # see if we find it and if there is a bcpp specific lib 
                # (bcpp_xxx.lib)
                libfile = self.find_library_file(library_dirs, lib, debug)
                if libfile is None:
                    ld_args.append(lib)
                    # probably a BCPP internal library -- don't warn
                    #    self.warn('library %s not found.' % lib)
                else:
                    # full name which prefers bcpp_xxx.lib over xxx.lib
                    ld_args.append(libfile)
            # def file for export symbols
            ld_args.extend([',',def_file])
            
            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend(extra_postargs)

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
              ("don't know how to set runtime library search path "
               "for Borland C++")

    def library_option (self, lib):
        return self.library_filename (lib)


    def find_library_file (self, dirs, lib, debug=0):
        # List of effective library names to try, in order of preference:
        # bcpp_xxx.lib is better than xxx.lib
        # and xxx_d.lib is better than xxx.lib if debug is set
        #
        # The "bcpp_" prefix is to handle a Python installation for people
        # with multiple compilers (primarily Distutils hackers, I suspect
        # ;-).  The idea is they'd have one static library for each
        # compiler they care about, since (almost?) every Windows compiler
        # seems to have a different format for static libraries.
        if debug:
            dlib = (lib + "_d")
            try_names = ("bcpp_" + dlib, "bcpp_" + lib, dlib, lib)
        else:
            try_names = ("bcpp_" + lib, lib)

        for dir in dirs:
            for name in try_names:
                libfile = os.path.join(dir, self.library_filename(name))
                if os.path.exists(libfile):
                    return libfile
        else:
            # Oops, didn't find it in *any* of 'dirs'
            return None

