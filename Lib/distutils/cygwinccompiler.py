"""distutils.cygwinccompiler

Provides the CygwinCCompiler class, a subclass of UnixCCompiler that
handles the Cygwin port of the GNU C compiler to Windows.  It also contains
the Mingw32CCompiler class which handles the mingw32 port of GCC (same as
cygwin in no-cygwin mode).
"""

# created 2000/05/05, Rene Liebscher

__revision__ = "$Id$"

import os,sys,string
from distutils import sysconfig
from distutils.unixccompiler import UnixCCompiler

# Because these compilers aren't configured in Python's config.h file by
# default we should at least warn the user if he is using a unmodified
# version.

def check_config_h():
    """Checks if the GCC compiler is mentioned in config.h.  If it is not,
    compiling probably doesn't work, so print a warning to stderr.
    """

    # XXX the result of the check should be returned!

    from distutils import sysconfig
    import string,sys
    try:
        # It would probably better to read single lines to search.
        # But we do this only once, and it is fast enough 
        f=open(sysconfig.get_config_h_filename())
        s=f.read()
        f.close()
        try:
            # is somewhere a #ifdef __GNUC__ or something similar
            string.index(s,"__GNUC__") 
        except ValueError:
            sys.stderr.write ("warning: "+
                "Python's config.h doesn't seem to support your compiler.\n")
    except IOError:
        # if we can't read this file, we cannot say it is wrong
        # the compiler will complain later about this file as missing
        pass


# This is called when the module is imported, so we make this check only once
# XXX why not make it only when the compiler is needed?
check_config_h()


class CygwinCCompiler (UnixCCompiler):

    compiler_type = 'cygwin'
   
    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):

        UnixCCompiler.__init__ (self, verbose, dry_run, force)

        # Hard-code GCC because that's what this is all about.
        # XXX optimization, warnings etc. should be customizable.
        self.set_executables(compiler='gcc -O -Wall',
                             compiler_so='gcc -O -Wall',
                             linker_exe='gcc',
                             linker_so='dllwrap --target=i386-cygwin32')

        # cygwin and mingw32 need different sets of libraries 
        self.dll_libraries=[
               # cygwin shouldn't need msvcrt, 
               # but without the dll's will crash
               # ( gcc version 2.91.57 )
               # perhaps something about initialization
               # mingw32 needs it in all cases
                            "msvcrt"
                            ]
        
    # __init__ ()

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
        
        if libraries == None:
            libraries = []
        
        # Additional libraries: the python library is always needed on
        # Windows we need the python version without the dot, eg. '15'

        pythonlib = ("python%d%d" %
                     (sys.hexversion >> 24, (sys.hexversion >> 16) & 0xff))
        libraries.append(pythonlib)
        libraries.extend(self.dll_libraries)

        # name of extension

        # XXX WRONG WRONG WRONG
        # this is NOT the place to make guesses about Python namespaces;
        # that MUST be done in build_ext.py

        if not debug:
            ext_name = os.path.basename(output_filename)[:-len(".pyd")]
        else:
            ext_name = os.path.basename(output_filename)[:-len("_d.pyd")]

        def_file = os.path.join(build_temp, ext_name + ".def")
        #exp_file = os.path.join(build_temp, ext_name + ".exp")
        #lib_file = os.path.join(build_temp, 'lib' + ext_name + ".a")
        
        # Make .def file
        # (It would probably better to check if we really need this, 
        # but for this we had to insert some unchanged parts of 
        # UnixCCompiler, and this is not what we want.) 
        f = open(def_file,"w")
        f.write("EXPORTS\n") # intro
        if export_symbols == None: 
            # export a function "init" + ext_name
            f.write("init" + ext_name + "\n")    
        else:
            # if there are more symbols to export write them into f
            for sym in export_symbols:
                f.write(sym+"\n")                    
        f.close()
        
        if extra_preargs == None:
                extra_preargs = []
        
        extra_preargs = extra_preargs + [
                        #"--verbose",
                        #"--output-exp",exp_file,
                        #"--output-lib",lib_file,
                        "--def",def_file
                        ]
        
        # who wants symbols and a many times larger output file
        # should explicitely switch the debug mode on 
        # otherwise we let dllwrap strip the output file
        # (On my machine unstripped_file = stripped_file + 254KB
        #   10KB < stripped_file < ??100KB ) 
        if not debug: 
            extra_preargs = extra_preargs + ["-s"] 
        
        UnixCCompiler.link_shared_object(self,
                            objects,
                            output_filename,
                            output_dir,
                            libraries,
                            library_dirs,
                            runtime_library_dirs,
                            None, # export_symbols, we do this with our def-file
                            debug,
                            extra_preargs,
                            extra_postargs,
                            build_temp)
        
    # link_shared_object ()

# class CygwinCCompiler


# the same as cygwin plus some additional parameters
class Mingw32CCompiler (CygwinCCompiler):

    compiler_type = 'mingw32'

    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):

        CygwinCCompiler.__init__ (self, verbose, dry_run, force)

        self.set_executables(compiler='gcc -mno-cygwin -O -Wall',
                             compiler_so='gcc -mno-cygwin -O -Wall',
                             linker_exe='gcc -mno-cygwin',
                             linker_so='dllwrap' 
                                    + ' --target=i386-mingw32'
                                    + ' --entry _DllMain@12')
        # mingw32 doesn't really need 'target' and cygwin too (it seems, 
        # it is enough to specify a different entry point)                

        # no additional libraries need 
        # (only msvcrt, which is already added by CygwinCCompiler)

    # __init__ ()
                
# class Mingw32CCompiler
