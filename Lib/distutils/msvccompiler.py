"""distutils.msvccompiler

Contains MSVCCompiler, an implementation of the abstract CCompiler class
for the Microsoft Visual Studio."""


# created 1999/08/19, Perry Stoll
# hacked by Robin Becker and Thomas Heller to do a better job of
#   finding DevStudio (through the registry)

__revision__ = "$Id$"

import sys, os, string
from types import *
from distutils.errors import \
     DistutilsExecError, DistutilsPlatformError, \
     CompileError, LibError, LinkError
from distutils.ccompiler import \
     CCompiler, gen_preprocess_options, gen_lib_options
from distutils import log

_can_read_reg = 0
try:
    import _winreg

    _can_read_reg = 1
    hkey_mod = _winreg

    RegOpenKeyEx = _winreg.OpenKeyEx
    RegEnumKey = _winreg.EnumKey
    RegEnumValue = _winreg.EnumValue
    RegError = _winreg.error

except ImportError:
    try:
        import win32api
        import win32con
        _can_read_reg = 1
        hkey_mod = win32con

        RegOpenKeyEx = win32api.RegOpenKeyEx
        RegEnumKey = win32api.RegEnumKey
        RegEnumValue = win32api.RegEnumValue
        RegError = win32api.error

    except ImportError:
        pass

if _can_read_reg:
    HKEY_CLASSES_ROOT = hkey_mod.HKEY_CLASSES_ROOT
    HKEY_LOCAL_MACHINE = hkey_mod.HKEY_LOCAL_MACHINE
    HKEY_CURRENT_USER = hkey_mod.HKEY_CURRENT_USER
    HKEY_USERS = hkey_mod.HKEY_USERS



def get_devstudio_versions ():
    """Get list of devstudio versions from the Windows registry.  Return a
       list of strings containing version numbers; the list will be
       empty if we were unable to access the registry (eg. couldn't import
       a registry-access module) or the appropriate registry keys weren't
       found."""

    if not _can_read_reg:
        return []

    K = 'Software\\Microsoft\\Devstudio'
    L = []
    for base in (HKEY_CLASSES_ROOT,
                 HKEY_LOCAL_MACHINE,
                 HKEY_CURRENT_USER,
                 HKEY_USERS):
        try:
            k = RegOpenKeyEx(base,K)
            i = 0
            while 1:
                try:
                    p = RegEnumKey(k,i)
                    if p[0] in '123456789' and p not in L:
                        L.append(p)
                except RegError:
                    break
                i = i + 1
        except RegError:
            pass
    L.sort()
    L.reverse()
    return L

# get_devstudio_versions ()


def get_msvc_paths (path, version='6.0', platform='x86'):
    """Get a list of devstudio directories (include, lib or path).  Return
       a list of strings; will be empty list if unable to access the
       registry or appropriate registry keys not found."""

    if not _can_read_reg:
        return []

    L = []
    if path=='lib':
        path= 'Library'
    path = string.upper(path + ' Dirs')
    K = ('Software\\Microsoft\\Devstudio\\%s\\' +
         'Build System\\Components\\Platforms\\Win32 (%s)\\Directories') % \
        (version,platform)
    for base in (HKEY_CLASSES_ROOT,
                 HKEY_LOCAL_MACHINE,
                 HKEY_CURRENT_USER,
                 HKEY_USERS):
        try:
            k = RegOpenKeyEx(base,K)
            i = 0
            while 1:
                try:
                    (p,v,t) = RegEnumValue(k,i)
                    if string.upper(p) == path:
                        V = string.split(v,';')
                        for v in V:
                            if hasattr(v, "encode"):
                                try:
                                    v = v.encode("mbcs")
                                except UnicodeError:
                                    pass
                            if v == '' or v in L: continue
                            L.append(v)
                        break
                    i = i + 1
                except RegError:
                    break
        except RegError:
            pass
    return L

# get_msvc_paths()


def find_exe (exe, version_number):
    """Try to find an MSVC executable program 'exe' (from version
       'version_number' of MSVC) in several places: first, one of the MSVC
       program search paths from the registry; next, the directories in the
       PATH environment variable.  If any of those work, return an absolute
       path that is known to exist.  If none of them work, just return the
       original program name, 'exe'."""

    for p in get_msvc_paths ('path', version_number):
        fn = os.path.join (os.path.abspath(p), exe)
        if os.path.isfile(fn):
            return fn

    # didn't find it; try existing path
    for p in string.split (os.environ['Path'],';'):
        fn = os.path.join(os.path.abspath(p),exe)
        if os.path.isfile(fn):
            return fn

    return exe                          # last desperate hope


def set_path_env_var (name, version_number):
    """Set environment variable 'name' to an MSVC path type value obtained
       from 'get_msvc_paths()'.  This is equivalent to a SET command prior
       to execution of spawned commands."""

    p = get_msvc_paths (name, version_number)
    if p:
        os.environ[name] = string.join (p,';')


class MSVCCompiler (CCompiler) :
    """Concrete class that implements an interface to Microsoft Visual C++,
       as defined by the CCompiler abstract class."""

    compiler_type = 'msvc'

    # Just set this so CCompiler's constructor doesn't barf.  We currently
    # don't use the 'set_executables()' bureaucracy provided by CCompiler,
    # as it really isn't necessary for this sort of single-compiler class.
    # Would be nice to have a consistent interface with UnixCCompiler,
    # though, so it's worth thinking about.
    executables = {}

    # Private class data (need to distinguish C from C++ source for compiler)
    _c_extensions = ['.c']
    _cpp_extensions = ['.cc', '.cpp', '.cxx']
    _rc_extensions = ['.rc']
    _mc_extensions = ['.mc']

    # Needed for the filename generation methods provided by the
    # base class, CCompiler.
    src_extensions = (_c_extensions + _cpp_extensions +
                      _rc_extensions + _mc_extensions)
    res_extension = '.res'
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
        versions = get_devstudio_versions ()

        if versions:
            version = versions[0]  # highest version

            self.cc   = find_exe("cl.exe", version)
            self.linker = find_exe("link.exe", version)
            self.lib  = find_exe("lib.exe", version)
            self.rc   = find_exe("rc.exe", version)     # resource compiler
            self.mc   = find_exe("mc.exe", version)     # message compiler
            set_path_env_var ('lib', version)
            set_path_env_var ('include', version)
            path=get_msvc_paths('path', version)
            try:
                for p in string.split(os.environ['path'],';'):
                    path.append(p)
            except KeyError:
                pass
            os.environ['path'] = string.join(path,';')
        else:
            # devstudio not found in the registry
            self.cc = "cl.exe"
            self.linker = "link.exe"
            self.lib = "lib.exe"
            self.rc = "rc.exe"
            self.mc = "mc.exe"

        self.preprocess_options = None
        self.compile_options = [ '/nologo', '/Ox', '/MD', '/W3', '/GX' ]
        self.compile_options_debug = ['/nologo', '/Od', '/MDd', '/W3', '/GX',
                                      '/Z7', '/D_DEBUG']

        self.ldflags_shared = ['/DLL', '/nologo', '/INCREMENTAL:NO']
        self.ldflags_shared_debug = [
            '/DLL', '/nologo', '/INCREMENTAL:no', '/pdb:None', '/DEBUG'
            ]
        self.ldflags_static = [ '/nologo']


    # -- Worker methods ------------------------------------------------

    def object_filenames (self,
                          source_filenames,
                          strip_dir=0,
                          output_dir=''):
        # Copied from ccompiler.py, extended to return .res as 'object'-file
        # for .rc input file
        if output_dir is None: output_dir = ''
        obj_names = []
        for src_name in source_filenames:
            (base, ext) = os.path.splitext (src_name)
            if ext not in self.src_extensions:
                # Better to raise an exception instead of silently continuing
                # and later complain about sources and targets having
                # different lengths
                raise CompileError ("Don't know how to compile %s" % src_name)
            if strip_dir:
                base = os.path.basename (base)
            if ext in self._rc_extensions:
                obj_names.append (os.path.join (output_dir,
                                                base + self.res_extension))
            elif ext in self._mc_extensions:
                obj_names.append (os.path.join (output_dir,
                                                base + self.res_extension))
            else:
                obj_names.append (os.path.join (output_dir,
                                                base + self.obj_extension))
        return obj_names

    # object_filenames ()


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
        compile_opts.append ('/c')
        if debug:
            compile_opts.extend (self.compile_options_debug)
        else:
            compile_opts.extend (self.compile_options)

        for i in range (len (sources)):
            src = sources[i] ; obj = objects[i]
            ext = (os.path.splitext (src))[1]

            if skip_sources[src]:
                log.debug("skipping %s (%s up-to-date)", src, obj)
            else:
                self.mkpath (os.path.dirname (obj))

                if debug:
                    # pass the full pathname to MSVC in debug mode,
                    # this allows the debugger to find the source file
                    # without asking the user to browse for it
                    src = os.path.abspath(src)

                if ext in self._c_extensions:
                    input_opt = "/Tc" + src
                elif ext in self._cpp_extensions:
                    input_opt = "/Tp" + src
                elif ext in self._rc_extensions:
                    # compile .RC to .RES file
                    input_opt = src
                    output_opt = "/fo" + obj
                    try:
                        self.spawn ([self.rc] +
                                    [output_opt] + [input_opt])
                    except DistutilsExecError, msg:
                        raise CompileError, msg
                    continue
                elif ext in self._mc_extensions:

                    # Compile .MC to .RC file to .RES file.
                    #   * '-h dir' specifies the directory for the
                    #     generated include file
                    #   * '-r dir' specifies the target directory of the
                    #     generated RC file and the binary message resource
                    #     it includes
                    #
                    # For now (since there are no options to change this),
                    # we use the source-directory for the include file and
                    # the build directory for the RC file and message
                    # resources. This works at least for win32all.

                    h_dir = os.path.dirname (src)
                    rc_dir = os.path.dirname (obj)
                    try:
                        # first compile .MC to .RC and .H file
                        self.spawn ([self.mc] +
                                    ['-h', h_dir, '-r', rc_dir] + [src])
                        base, _ = os.path.splitext (os.path.basename (src))
                        rc_file = os.path.join (rc_dir, base + '.rc')
                        # then compile .RC to .RES file
                        self.spawn ([self.rc] +
                                    ["/fo" + obj] + [rc_file])

                    except DistutilsExecError, msg:
                        raise CompileError, msg
                    continue
                else:
                    # how to handle this file?
                    raise CompileError (
                        "Don't know how to compile %s to %s" % \
                        (src, obj))

                output_opt = "/Fo" + obj
                try:
                    self.spawn ([self.cc] + compile_opts + pp_opts +
                                [input_opt, output_opt] +
                                extra_postargs)
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
            lib_args = objects + ['/OUT:' + output_filename]
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
            log.debug("skipping %s (up-to-date)", output_filename)

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

        (objects, output_dir) = self._fix_object_args (objects, output_dir)
        (libraries, library_dirs, runtime_library_dirs) = \
            self._fix_lib_args (libraries, library_dirs, runtime_library_dirs)

        if runtime_library_dirs:
            self.warn ("I don't know what to do with 'runtime_library_dirs': "
                       + str (runtime_library_dirs))

        lib_opts = gen_lib_options (self,
                                    library_dirs, runtime_library_dirs,
                                    libraries)
        if output_dir is not None:
            output_filename = os.path.join (output_dir, output_filename)

        if self._need_link (objects, output_filename):

            if target_desc == CCompiler.EXECUTABLE:
                if debug:
                    ldflags = self.ldflags_shared_debug[1:]
                else:
                    ldflags = self.ldflags_shared[1:]
            else:
                if debug:
                    ldflags = self.ldflags_shared_debug
                else:
                    ldflags = self.ldflags_shared

            export_opts = []
            for sym in (export_symbols or []):
                export_opts.append("/EXPORT:" + sym)

            ld_args = (ldflags + lib_opts + export_opts +
                       objects + ['/OUT:' + output_filename])

            # The MSVC linker generates .lib and .exp files, which cannot be
            # suppressed by any linker switches. The .lib files may even be
            # needed! Make sure they are generated in the temporary build
            # directory. Since they have different names for debug and release
            # builds, they can go into the same directory.
            if export_symbols is not None:
                (dll_name, dll_ext) = os.path.splitext(
                    os.path.basename(output_filename))
                implib_file = os.path.join(
                    os.path.dirname(objects[0]),
                    self.library_filename(dll_name))
                ld_args.append ('/IMPLIB:' + implib_file)

            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend(extra_postargs)

            self.mkpath (os.path.dirname (output_filename))
            try:
                self.spawn ([self.linker] + ld_args)
            except DistutilsExecError, msg:
                raise LinkError, msg

        else:
            log.debug("skipping %s (up-to-date)", output_filename)

    # link ()


    # -- Miscellaneous methods -----------------------------------------
    # These are all used by the 'gen_lib_options() function, in
    # ccompiler.py.

    def library_dir_option (self, dir):
        return "/LIBPATH:" + dir

    def runtime_library_dir_option (self, dir):
        raise DistutilsPlatformError, \
              "don't know how to set runtime library search path for MSVC++"

    def library_option (self, lib):
        return self.library_filename (lib)


    def find_library_file (self, dirs, lib, debug=0):
        # Prefer a debugging library if found (and requested), but deal
        # with it if we don't have one.
        if debug:
            try_names = [lib + "_d", lib]
        else:
            try_names = [lib]
        for dir in dirs:
            for name in try_names:
                libfile = os.path.join(dir, self.library_filename (name))
                if os.path.exists(libfile):
                    return libfile
        else:
            # Oops, didn't find it in *any* of 'dirs'
            return None

    # find_library_file ()

# class MSVCCompiler
