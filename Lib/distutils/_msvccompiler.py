"""distutils._msvccompiler

Contains MSVCCompiler, an implementation of the abstract CCompiler class
for Microsoft Visual Studio 2015.

The module is compatible with VS 2015 and later. You can find legacy support
for older versions in distutils.msvc9compiler and distutils.msvccompiler.
"""

# Written by Perry Stoll
# hacked by Robin Becker and Thomas Heller to do a better job of
#   finding DevStudio (through the registry)
# ported to VS 2005 and VS 2008 by Christian Heimes
# ported to VS 2015 by Steve Dower

import os
import shutil
import stat
import subprocess

from distutils.errors import DistutilsExecError, DistutilsPlatformError, \
                             CompileError, LibError, LinkError
from distutils.ccompiler import CCompiler, gen_lib_options
from distutils import log
from distutils.util import get_platform

import winreg
from itertools import count

def _find_vcvarsall(plat_spec):
    with winreg.OpenKeyEx(
        winreg.HKEY_LOCAL_MACHINE,
        r"Software\Microsoft\VisualStudio\SxS\VC7",
        access=winreg.KEY_READ | winreg.KEY_WOW64_32KEY
    ) as key:
        if not key:
            log.debug("Visual C++ is not registered")
            return None, None

        best_version = 0
        best_dir = None
        for i in count():
            try:
                v, vc_dir, vt = winreg.EnumValue(key, i)
            except OSError:
                break
            if v and vt == winreg.REG_SZ and os.path.isdir(vc_dir):
                try:
                    version = int(float(v))
                except (ValueError, TypeError):
                    continue
                if version >= 14 and version > best_version:
                    best_version, best_dir = version, vc_dir
        if not best_version:
            log.debug("No suitable Visual C++ version found")
            return None, None

        vcvarsall = os.path.join(best_dir, "vcvarsall.bat")
        if not os.path.isfile(vcvarsall):
            log.debug("%s cannot be found", vcvarsall)
            return None, None

        vcruntime = None
        vcruntime_spec = _VCVARS_PLAT_TO_VCRUNTIME_REDIST.get(plat_spec)
        if vcruntime_spec:
            vcruntime = os.path.join(best_dir,
                vcruntime_spec.format(best_version))
            if not os.path.isfile(vcruntime):
                log.debug("%s cannot be found", vcruntime)
                vcruntime = None

        return vcvarsall, vcruntime

def _get_vc_env(plat_spec):
    if os.getenv("DISTUTILS_USE_SDK"):
        return {
            key.lower(): value
            for key, value in os.environ.items()
        }

    vcvarsall, vcruntime = _find_vcvarsall(plat_spec)
    if not vcvarsall:
        raise DistutilsPlatformError("Unable to find vcvarsall.bat")

    try:
        out = subprocess.check_output(
            '"{}" {} && set'.format(vcvarsall, plat_spec),
            shell=True,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
        )
    except subprocess.CalledProcessError as exc:
        log.error(exc.output)
        raise DistutilsPlatformError("Error executing {}"
                .format(exc.cmd))

    env = {
        key.lower(): value
        for key, _, value in
        (line.partition('=') for line in out.splitlines())
        if key and value
    }
    
    if vcruntime:
        env['py_vcruntime_redist'] = vcruntime
    return env

def _find_exe(exe, paths=None):
    """Return path to an MSVC executable program.

    Tries to find the program in several places: first, one of the
    MSVC program search paths from the registry; next, the directories
    in the PATH environment variable.  If any of those work, return an
    absolute path that is known to exist.  If none of them work, just
    return the original program name, 'exe'.
    """
    if not paths:
        paths = os.getenv('path').split(os.pathsep)
    for p in paths:
        fn = os.path.join(os.path.abspath(p), exe)
        if os.path.isfile(fn):
            return fn
    return exe

# A map keyed by get_platform() return values to values accepted by
# 'vcvarsall.bat'.  Note a cross-compile may combine these (eg, 'x86_amd64' is
# the param to cross-compile on x86 targetting amd64.)
PLAT_TO_VCVARS = {
    'win32' : 'x86',
    'win-amd64' : 'amd64',
}

# A map keyed by get_platform() return values to the file under
# the VC install directory containing the vcruntime redistributable.
_VCVARS_PLAT_TO_VCRUNTIME_REDIST = {
    'x86' : 'redist\\x86\\Microsoft.VC{0}0.CRT\\vcruntime{0}0.dll',
    'amd64' : 'redist\\x64\\Microsoft.VC{0}0.CRT\\vcruntime{0}0.dll',
    'x86_amd64' : 'redist\\x64\\Microsoft.VC{0}0.CRT\\vcruntime{0}0.dll',
}

# A set containing the DLLs that are guaranteed to be available for
# all micro versions of this Python version. Known extension
# dependencies that are not in this set will be copied to the output
# path.
_BUNDLED_DLLS = frozenset(['vcruntime140.dll'])

class MSVCCompiler(CCompiler) :
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


    def __init__(self, verbose=0, dry_run=0, force=0):
        CCompiler.__init__ (self, verbose, dry_run, force)
        # target platform (.plat_name is consistent with 'bdist')
        self.plat_name = None
        self.initialized = False

    def initialize(self, plat_name=None):
        # multi-init means we would need to check platform same each time...
        assert not self.initialized, "don't init multiple times"
        if plat_name is None:
            plat_name = get_platform()
        # sanity check for platforms to prevent obscure errors later.
        if plat_name not in PLAT_TO_VCVARS:
            raise DistutilsPlatformError("--plat-name must be one of {}"
                                         .format(tuple(PLAT_TO_VCVARS)))

        # On x86, 'vcvarsall.bat amd64' creates an env that doesn't work;
        # to cross compile, you use 'x86_amd64'.
        # On AMD64, 'vcvarsall.bat amd64' is a native build env; to cross
        # compile use 'x86' (ie, it runs the x86 compiler directly)
        if plat_name == get_platform() or plat_name == 'win32':
            # native build or cross-compile to win32
            plat_spec = PLAT_TO_VCVARS[plat_name]
        else:
            # cross compile from win32 -> some 64bit
            plat_spec = '{}_{}'.format(
                PLAT_TO_VCVARS[get_platform()],
                PLAT_TO_VCVARS[plat_name]
            )

        vc_env = _get_vc_env(plat_spec)
        if not vc_env:
            raise DistutilsPlatformError("Unable to find a compatible "
                "Visual Studio installation.")

        self._paths = vc_env.get('path', '')
        paths = self._paths.split(os.pathsep)
        self.cc = _find_exe("cl.exe", paths)
        self.linker = _find_exe("link.exe", paths)
        self.lib = _find_exe("lib.exe", paths)
        self.rc = _find_exe("rc.exe", paths)   # resource compiler
        self.mc = _find_exe("mc.exe", paths)   # message compiler
        self.mt = _find_exe("mt.exe", paths)   # message compiler
        self._vcruntime_redist = vc_env.get('py_vcruntime_redist', '')

        for dir in vc_env.get('include', '').split(os.pathsep):
            if dir:
                self.add_include_dir(dir)

        for dir in vc_env.get('lib', '').split(os.pathsep):
            if dir:
                self.add_library_dir(dir)

        self.preprocess_options = None
        # If vcruntime_redist is available, link against it dynamically. Otherwise,
        # use /MT[d] to build statically, then switch from libucrt[d].lib to ucrt[d].lib
        # later to dynamically link to ucrtbase but not vcruntime.
        self.compile_options = [
            '/nologo', '/Ox', '/W3', '/GL', '/DNDEBUG'
        ]
        self.compile_options.append('/MD' if self._vcruntime_redist else '/MT')
        
        self.compile_options_debug = [
            '/nologo', '/Od', '/MDd', '/Zi', '/W3', '/D_DEBUG'
        ]

        ldflags = [
            '/nologo', '/INCREMENTAL:NO', '/LTCG'
        ]
        if not self._vcruntime_redist:
            ldflags.extend(('/nodefaultlib:libucrt.lib', 'ucrt.lib'))

        ldflags_debug = [
            '/nologo', '/INCREMENTAL:NO', '/LTCG', '/DEBUG:FULL'
        ]

        self.ldflags_exe = [*ldflags, '/MANIFEST:EMBED,ID=1']
        self.ldflags_exe_debug = [*ldflags_debug, '/MANIFEST:EMBED,ID=1']
        self.ldflags_shared = [*ldflags, '/DLL', '/MANIFEST:EMBED,ID=2', '/MANIFESTUAC:NO']
        self.ldflags_shared_debug = [*ldflags_debug, '/DLL', '/MANIFEST:EMBED,ID=2', '/MANIFESTUAC:NO']
        self.ldflags_static = [*ldflags]
        self.ldflags_static_debug = [*ldflags_debug]

        self._ldflags = {
            (CCompiler.EXECUTABLE, None): self.ldflags_exe,
            (CCompiler.EXECUTABLE, False): self.ldflags_exe,
            (CCompiler.EXECUTABLE, True): self.ldflags_exe_debug,
            (CCompiler.SHARED_OBJECT, None): self.ldflags_shared,
            (CCompiler.SHARED_OBJECT, False): self.ldflags_shared,
            (CCompiler.SHARED_OBJECT, True): self.ldflags_shared_debug,
            (CCompiler.SHARED_LIBRARY, None): self.ldflags_static,
            (CCompiler.SHARED_LIBRARY, False): self.ldflags_static,
            (CCompiler.SHARED_LIBRARY, True): self.ldflags_static_debug,
        }

        self.initialized = True

    # -- Worker methods ------------------------------------------------

    def object_filenames(self,
                         source_filenames,
                         strip_dir=0,
                         output_dir=''):
        ext_map = {
            **{ext: self.obj_extension for ext in self.src_extensions},
            **{ext: self.res_extension for ext in self._rc_extensions + self._mc_extensions},
        }

        output_dir = output_dir or ''

        def make_out_path(p):
            base, ext = os.path.splitext(p)
            if strip_dir:
                base = os.path.basename(base)
            else:
                _, base = os.path.splitdrive(base)
                if base.startswith((os.path.sep, os.path.altsep)):
                    base = base[1:]
            try:
                # XXX: This may produce absurdly long paths. We should check
                # the length of the result and trim base until we fit within
                # 260 characters.
                return os.path.join(output_dir, base + ext_map[ext])
            except LookupError:
                # Better to raise an exception instead of silently continuing
                # and later complain about sources and targets having
                # different lengths
                raise CompileError("Don't know how to compile {}".format(p))

        return list(map(make_out_path, source_filenames))


    def compile(self, sources,
                output_dir=None, macros=None, include_dirs=None, debug=0,
                extra_preargs=None, extra_postargs=None, depends=None):

        if not self.initialized:
            self.initialize()
        compile_info = self._setup_compile(output_dir, macros, include_dirs,
                                           sources, depends, extra_postargs)
        macros, objects, extra_postargs, pp_opts, build = compile_info

        compile_opts = extra_preargs or []
        compile_opts.append('/c')
        if debug:
            compile_opts.extend(self.compile_options_debug)
        else:
            compile_opts.extend(self.compile_options)


        add_cpp_opts = False

        for obj in objects:
            try:
                src, ext = build[obj]
            except KeyError:
                continue
            if debug:
                # pass the full pathname to MSVC in debug mode,
                # this allows the debugger to find the source file
                # without asking the user to browse for it
                src = os.path.abspath(src)

            if ext in self._c_extensions:
                input_opt = "/Tc" + src
            elif ext in self._cpp_extensions:
                input_opt = "/Tp" + src
                add_cpp_opts = True
            elif ext in self._rc_extensions:
                # compile .RC to .RES file
                input_opt = src
                output_opt = "/fo" + obj
                try:
                    self.spawn([self.rc] + pp_opts + [output_opt, input_opt])
                except DistutilsExecError as msg:
                    raise CompileError(msg)
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
                h_dir = os.path.dirname(src)
                rc_dir = os.path.dirname(obj)
                try:
                    # first compile .MC to .RC and .H file
                    self.spawn([self.mc, '-h', h_dir, '-r', rc_dir, src])
                    base, _ = os.path.splitext(os.path.basename (src))
                    rc_file = os.path.join(rc_dir, base + '.rc')
                    # then compile .RC to .RES file
                    self.spawn([self.rc, "/fo" + obj, rc_file])

                except DistutilsExecError as msg:
                    raise CompileError(msg)
                continue
            else:
                # how to handle this file?
                raise CompileError("Don't know how to compile {} to {}"
                                   .format(src, obj))

            args = [self.cc] + compile_opts + pp_opts
            if add_cpp_opts:
                args.append('/EHsc')
            args.append(input_opt)
            args.append("/Fo" + obj)
            args.extend(extra_postargs)

            try:
                self.spawn(args)
            except DistutilsExecError as msg:
                raise CompileError(msg)

        return objects


    def create_static_lib(self,
                          objects,
                          output_libname,
                          output_dir=None,
                          debug=0,
                          target_lang=None):

        if not self.initialized:
            self.initialize()
        objects, output_dir = self._fix_object_args(objects, output_dir)
        output_filename = self.library_filename(output_libname,
                                                output_dir=output_dir)

        if self._need_link(objects, output_filename):
            lib_args = objects + ['/OUT:' + output_filename]
            if debug:
                pass # XXX what goes here?
            try:
                log.debug('Executing "%s" %s', self.lib, ' '.join(lib_args))
                self.spawn([self.lib] + lib_args)
            except DistutilsExecError as msg:
                raise LibError(msg)
        else:
            log.debug("skipping %s (up-to-date)", output_filename)


    def link(self,
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
             build_temp=None,
             target_lang=None):

        if not self.initialized:
            self.initialize()
        objects, output_dir = self._fix_object_args(objects, output_dir)
        fixed_args = self._fix_lib_args(libraries, library_dirs,
                                        runtime_library_dirs)
        libraries, library_dirs, runtime_library_dirs = fixed_args

        if runtime_library_dirs:
            self.warn("I don't know what to do with 'runtime_library_dirs': "
                       + str(runtime_library_dirs))

        lib_opts = gen_lib_options(self,
                                   library_dirs, runtime_library_dirs,
                                   libraries)
        if output_dir is not None:
            output_filename = os.path.join(output_dir, output_filename)

        if self._need_link(objects, output_filename):
            ldflags = self._ldflags[target_desc, debug]

            export_opts = ["/EXPORT:" + sym for sym in (export_symbols or [])]

            ld_args = (ldflags + lib_opts + export_opts +
                       objects + ['/OUT:' + output_filename])

            # The MSVC linker generates .lib and .exp files, which cannot be
            # suppressed by any linker switches. The .lib files may even be
            # needed! Make sure they are generated in the temporary build
            # directory. Since they have different names for debug and release
            # builds, they can go into the same directory.
            build_temp = os.path.dirname(objects[0])
            if export_symbols is not None:
                (dll_name, dll_ext) = os.path.splitext(
                    os.path.basename(output_filename))
                implib_file = os.path.join(
                    build_temp,
                    self.library_filename(dll_name))
                ld_args.append ('/IMPLIB:' + implib_file)

            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend(extra_postargs)

            output_dir = os.path.dirname(os.path.abspath(output_filename))
            self.mkpath(output_dir)
            try:
                log.debug('Executing "%s" %s', self.linker, ' '.join(ld_args))
                self.spawn([self.linker] + ld_args)
                self._copy_vcruntime(output_dir)
            except DistutilsExecError as msg:
                raise LinkError(msg)
        else:
            log.debug("skipping %s (up-to-date)", output_filename)

    def _copy_vcruntime(self, output_dir):
        vcruntime = self._vcruntime_redist
        if not vcruntime or not os.path.isfile(vcruntime):
            return

        if os.path.basename(vcruntime).lower() in _BUNDLED_DLLS:
            return

        log.debug('Copying "%s"', vcruntime)
        vcruntime = shutil.copy(vcruntime, output_dir)
        os.chmod(vcruntime, stat.S_IWRITE)

    def spawn(self, cmd):
        old_path = os.getenv('path')
        try:
            os.environ['path'] = self._paths
            return super().spawn(cmd)
        finally:
            os.environ['path'] = old_path

    # -- Miscellaneous methods -----------------------------------------
    # These are all used by the 'gen_lib_options() function, in
    # ccompiler.py.

    def library_dir_option(self, dir):
        return "/LIBPATH:" + dir

    def runtime_library_dir_option(self, dir):
        raise DistutilsPlatformError(
              "don't know how to set runtime library search path for MSVC")

    def library_option(self, lib):
        return self.library_filename(lib)

    def find_library_file(self, dirs, lib, debug=0):
        # Prefer a debugging library if found (and requested), but deal
        # with it if we don't have one.
        if debug:
            try_names = [lib + "_d", lib]
        else:
            try_names = [lib]
        for dir in dirs:
            for name in try_names:
                libfile = os.path.join(dir, self.library_filename(name))
                if os.path.isfile(libfile):
                    return libfile
        else:
            # Oops, didn't find it in *any* of 'dirs'
            return None
