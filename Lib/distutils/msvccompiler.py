"""distutils.ccompiler

Contains MSVCCompiler, an implementation of the abstract CCompiler class
for the Microsoft Visual Studio """


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

    def __init__ (self,
                  verbose=0,
                  dry_run=0):

        CCompiler.__init__ (self, verbose, dry_run)

        # XXX This is a nasty dependency to add on something otherwise
        # pretty clean.  move it to build_ext under an nt specific part.
        # shared libraries need to link against python15.lib
        self.add_library ( "python" + sys.version[0] + sys.version[2] )
        self.add_library_dir( os.path.join( sys.exec_prefix, 'libs' ) )
        
        self.cc   = "cl.exe"
        self.link = "link.exe"
        self.preprocess_options = None
        self.compile_options = [ '/nologo' ]

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

        if macros is None:
            macros = []
        if includes is None:
            includes = []

        objectFiles = []

        base_pp_opts = gen_preprocess_options (self.macros + macros,
                                               self.include_dirs + includes)

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

            pp_opts = base_pp_opts + [ outputOpt, inputOpt ]

            self.spawn( [ self.cc ] + self.compile_options + pp_opts)
            objectFiles.append( objFile )

        return objectFiles


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
        
        if libraries is None:
            libraries = []
        if library_dirs is None:
            library_dirs = []
        if build_info is None:
            build_info = {}
        
        lib_opts = gen_lib_options (self.libraries + libraries,
                                    self.library_dirs + library_dirs,
                                    "%s.lib", "/LIBPATH:%s")

        if build_info.has_key('def_file') :
            lib_opts.append('/DEF:' + build_info['def_file'] )
                            
        ld_args = self.ldflags_static + lib_opts + \
                  objects + ['/OUT:' + output_filename]

        self.spawn ( [ self.link ] + ld_args )
    

    def link_shared_lib (self,
                         objects,
                         output_libname,
                         libraries=None,
                         library_dirs=None,
                         build_info=None):
        """Link a bunch of stuff together to create a shared library
           file.  Has the same effect as 'link_static_lib()' except
           that the filename inferred from 'output_libname' will most
           likely be different, and the type of file generated will
           almost certainly be different."""
        # XXX should we sanity check the library name? (eg. no
        # slashes)
        self.link_shared_object (objects, self.shared_library_name(output_libname),
                                 build_info=build_info )
    
    def link_shared_object (self,
                            objects,
                            output_filename,
                            libraries=None,
                            library_dirs=None,
                            build_info=None):
        """Link a bunch of stuff together to create a shared object
           file.  Much like 'link_shared_lib()', except the output
           filename is explicitly supplied as 'output_filename'."""
        if libraries is None:
            libraries = []
        if library_dirs is None:
            library_dirs = []
        if build_info is None:
            build_info = {}
        
        lib_opts = gen_lib_options (self.libraries + libraries,
                                    self.library_dirs + library_dirs,
                                    "%s.lib", "/LIBPATH:%s")

        if build_info.has_key('def_file') :
            lib_opts.append('/DEF:' + build_info['def_file'] )
                            
        ld_args = self.ldflags_shared + lib_opts + \
                  objects + ['/OUT:' + output_filename]

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
        return "lib%s%s" %( libname, self._static_lib_ext )

    def shared_library_filename (self, libname):
        """Return the shared library filename corresponding to the
           specified library name."""
        return "lib%s%s" %( libname, self._shared_lib_ext )

# class MSVCCompiler
