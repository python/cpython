"""distutils.cygwinccompiler

Contains the CygwinCCompiler class, a subclass of UnixCCompiler that handles
the Gnu Win32 C compiler.
It also contains the Mingw32CCompiler class which handles the mingw32 compiler
(same as cygwin in no-cygwin mode.)

"""

# created 2000/05/05, Rene Liebscher

__revision__ = "$Id$"

import os,sys,string,tempfile
from distutils import sysconfig
from distutils.unixccompiler import UnixCCompiler

# Because these compilers aren't configured in Python's config.h file by default
# we should at least warn the user if he used this unmodified version. 
def check_if_config_h_is_gcc_ready():
        """ checks, if the gcc-compiler is mentioned in config.h 
            if it is not, compiling probably doesn't work """ 
        from distutils import sysconfig
        import string,sys
        try:
                # It would probably better to read single lines to search.
                # But we do this only once, and it is fast enough 
                f=open(sysconfig.get_config_h_filename())
                s=f.read()
                f.close()
                try:
                        string.index(s,"__GNUC__") # is somewhere a #ifdef __GNUC__ or something similar
                except:
                        sys.stderr.write ("warning: Python's config.h doesn't seem to support your compiler.\n")
        except: # unspecific error => ignore
                pass


# This is called when the module is imported, so we make this check only once
check_if_config_h_is_gcc_ready()


# XXX Things not currently handled:
#   * see UnixCCompiler

class CygwinCCompiler (UnixCCompiler):

    compiler_type = 'cygwin'
   
    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):

        UnixCCompiler.__init__ (self, verbose, dry_run, force)

	# our compiler uses other names
	self.cc='gcc'
	self.ld_shared='dllwrap'
	self.ldflags_shared=[]

        # some variables to manage the differences between cygwin and mingw32
        self.dllwrap_options=["--target=i386-cygwin32"]
        # specification of entry point is not necessary
        
        self.dll_additional_libraries=[
               # cygwin shouldn't need msvcrt, but without the dll's will crash
               # perhaps something about initialization (Python uses it, too)
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
                            extra_postargs=None):
        
        if libraries==None:
                libraries=[]
        
        python_library=["python"+str(sys.hexversion>>24)+str((sys.hexversion>>16)&0xff)]
        libraries=libraries+python_library+self.dll_additional_libraries
        
        # if you don't need the def-file afterwards, it is
        # better to use for it tempfile.mktemp() as its name
        # (unix-style compilers don't like backslashes in filenames)
        win_dll_def_file=string.replace(tempfile.mktemp(),"\\","/")
        #win_dll_def_file=output_filename[:-len(self.shared_lib_extension)]+".def"
        #win_dll_exp_file=output_filename[:-len(self.shared_lib_extension)]+".exp"
        #win_dll_lib_file=output_filename[:-len(self.shared_lib_extension)]+".a"
        
        # Make .def file
        # (It would probably better to check if we really need this, but for this we had to
        # insert some unchanged parts of UnixCCompiler, and this is not what I want.) 
        f=open(win_dll_def_file,"w")
        f.write("EXPORTS\n") # intro
        # always export a function "init"+module_name
        if not debug:
                f.write("init"+os.path.basename(output_filename)[:-len(self.shared_lib_extension)]+"\n")
        else: # in debug mode outfile_name is something like XXXXX_d.pyd
                f.write("init"+os.path.basename(output_filename)[:-(len(self.shared_lib_extension)+2)]+"\n")
        # if there are more symbols to export
        # insert code here to write them in f
        if export_symbols!=None: 
            for sym in export_symbols:
                f.write(sym+"\n")                
        f.close()
        
        if extra_preargs==None:
                extra_preargs=[]
        
        extra_preargs=extra_preargs+[
                        #"--verbose",
                        #"--output-exp",win_dll_exp_file,
                        #"--output-lib",win_dll_lib_file,
                        "--def",win_dll_def_file
                        ]+ self.dllwrap_options
        
        # who wants symbols and a many times greater output file
        # should explicitely switch the debug mode on 
        # otherwise we let dllwrap strip the outputfile
        # (On my machine unstripped_file=stripped_file+254KB
        #   10KB < stripped_file < ??100KB ) 
        if not debug: 
                extra_preargs=extra_preargs+["-s"] 
	
	try:        
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
                            extra_postargs)
        finally: 
    	    # we don't need the def-file anymore
    	    os.remove(win_dll_def_file) 
        
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

        self.ccflags = self.ccflags + ["-mno-cygwin"]
        self.dllwrap_options=[
                                # mingw32 doesn't really need 'target' 
                                # and cygwin too (it seems, it is enough
                                # to specify a different entry point)                
                                #"--target=i386-mingw32",
                                "--entry","_DllMain@12"
                                ]
        # no additional libraries need 
        # (only msvcrt, which is already added by CygwinCCompiler)

    # __init__ ()
                
# class Mingw32CCompiler
