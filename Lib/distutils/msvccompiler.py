"""distutils.ccompiler

Contains MSVCCompiler, an implementation of the abstract CCompiler class
for the Microsoft Visual Studio."""


# created 1999/08/19, Perry Stoll
# 
__rcsid__ = "$Id$"

import os
import sys
from distutils.errors import *
from distutils.ccompiler import \
     CCompiler, gen_preprocess_options, gen_lib_options


class MSVCCompiler (CCompiler) :
    """Concrete class that implements an interface to Microsoft Visual C++,
       as defined by the CCompiler abstract class."""

    compiler_type = 'msvc'

    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):

        CCompiler.__init__ (self, verbose, dry_run, force)

        # XXX This is a nasty dependency to add on something otherwise
        # pretty clean.  move it to build_ext under an nt specific part.
        # shared libraries need to link against python15.lib
        self.add_library ( "python" + sys.version[0] + sys.version[2] )
        self.add_library_dir( os.path.join( sys.exec_prefix, 'libs' ) )
        
        self.cc   = "cl.exe"
        self.link = "link.exe"
        self.preprocess_options = None
        self.compile_options = [ '/nologo', '/Ox', '/MD', '/GD' ]

        self.ldflags_shared = ['/DLL', '/nologo']
        self.ldflags_static = [ '/nologo']


    # -- Worker methods ------------------------------------------------
    # (must be implemented by subclasses)

    _c_extensions = [ '.c' ]
    _cpp_extensions = [ '.cc', '.cpp' ]

    _obj_ext = '.obj'
    _exe_ext = '.exe'
    _shared_lib_ext = '.dll'
    _static_lib_ext = '.lib'

    # XXX the 'output_dir' parameter is ignored by the methods in this
    # class!  I just put it in to be consistent with CCompiler and
    # UnixCCompiler, but someone who actually knows Visual C++ will
    # have to make it work...
    
    def compile (self,
                 sources,
                 output_dir=None,
                 macros=None,
                 include_dirs=None,
                 extra_preargs=None,
                 extra_postargs=None):

        if macros is None:
            macros = []
        if include_dirs is None:
            include_dirs = []

        objectFiles = []

        base_pp_opts = \
            gen_preprocess_options (self.macros + macros,
                                    self.include_dirs + include_dirs)

        base_pp_opts.append('/c')
        
        for srcFile in sources:
            base,ext = os.path.splitext(srcFile)
            objFile = base + ".obj"

            if ext in self._c_extensions:
                fileOpt = "/Tc"
            elif ext in self._cpp_extensions:
                fileOpt = "/Tp"

            inputOpt  = fileOpt + srcFile
            outputOpt = "/Fo"   + objFile

            cc_args = self.compile_options + \
                      base_pp_opts + \
                      [outputOpt, inputOpt]
            if extra_preargs:
                cc_args[:0] = extra_preargs
            if extra_postargs:
                cc_args.extend (extra_postargs)

            self.spawn ([self.cc] + cc_args)
            objectFiles.append( objFile )
        return objectFiles


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

        if libraries is None:
            libraries = []
        if library_dirs is None:
            library_dirs = []
        
        lib_opts = gen_lib_options (self.libraries + libraries,
                                    self.library_dirs + library_dirs,
                                    "%s.lib", "/LIBPATH:%s")

        ld_args = self.ldflags_static + lib_opts + \
                  objects + ['/OUT:' + output_filename]
        if extra_preargs:
            ld_args[:0] = extra_preargs
        if extra_postargs:
            ld_args.extend (extra_postargs)

        self.spawn ( [ self.link ] + ld_args )
    

    def link_shared_lib (self,
                         objects,
                         output_libname,
                         output_dir=None,
                         libraries=None,
                         library_dirs=None,
                         extra_preargs=None,
                         extra_postargs=None):

        # XXX should we sanity check the library name? (eg. no
        # slashes)
        self.link_shared_object (objects,
                                 self.shared_library_name(output_libname))
    
    def link_shared_object (self,
                            objects,
                            output_filename,
                            output_dir=None,
                            libraries=None,
                            library_dirs=None,
                            extra_preargs=None,
                            extra_postargs=None):
        """Link a bunch of stuff together to create a shared object
           file.  Much like 'link_shared_lib()', except the output
           filename is explicitly supplied as 'output_filename'."""
        if libraries is None:
            libraries = []
        if library_dirs is None:
            library_dirs = []
        
        lib_opts = gen_lib_options (self,
                                    self.library_dirs + library_dirs,
                                    self.libraries + libraries)

        ld_args = self.ldflags_shared + lib_opts + \
                  objects + ['/OUT:' + output_filename]
        if extra_preargs:
            ld_args[:0] = extra_preargs
        if extra_postargs:
            ld_args.extend (extra_postargs)

        self.spawn ( [ self.link ] + ld_args )


    # -- Filename mangling methods -------------------------------------

    def _change_extensions( self, filenames, newExtension ):
        object_filenames = []

        for srcFile in filenames:
            base,ext = os.path.splitext( srcFile )
            # XXX should we strip off any existing path?
            object_filenames.append( base + newExtension )

        return object_filenames

    def object_filenames (self, source_filenames):
        """Return the list of object filenames corresponding to each
           specified source filename."""
        return self._change_extensions( source_filenames, self._obj_ext )

    def shared_object_filename (self, source_filename):
        """Return the shared object filename corresponding to a
           specified source filename."""
        return self._change_extensions( source_filenames, self._shared_lib_ext )

    def library_filename (self, libname):
        """Return the static library filename corresponding to the
           specified library name."""
        return "%s%s" %( libname, self._static_lib_ext )

    def shared_library_filename (self, libname):
        """Return the shared library filename corresponding to the
           specified library name."""
        return "%s%s" %( libname, self._shared_lib_ext )


    def library_dir_option (self, dir):
        return "/LIBPATH:" + dir

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

    # find_library_file ()

# class MSVCCompiler
