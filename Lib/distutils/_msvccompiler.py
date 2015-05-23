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
import subprocess
import re

from distutils.errors import DistutilsExecError, DistutilsPlatformError, \
                             CompileError, LibError, LinkError
from distutils.ccompiler import CCompiler, gen_lib_options
from distutils import log
from distutils.util import get_platform

import winreg
from itertools import count

def _find_vcvarsall():
    with winreg.OpenKeyEx(
        winreg.HKEY_LOCAL_MACHINE,
        r"Software\Microsoft\VisualStudio\SxS\VC7",
        access=winreg.KEY_READ | winreg.KEY_WOW64_32KEY
    ) as key:
        if not key:
            log.debug("Visual C++ is not registered")
            return None

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
            return None

        vcvarsall = os.path.join(best_dir, "vcvarsall.bat")
        if not os.path.isfile(vcvarsall):
            log.debug("%s cannot be found", vcvarsall)
            return None

        return vcvarsall

def _get_vc_env(plat_spec):
    if os.getenv("DISTUTILS_USE_SDK"):
        return {
            key.lower(): value
            for key, value in os.environ.items()
        }

    vcvarsall = _find_vcvarsall()
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

    return {
        key.lower(): value
        for key, _, value in
        (line.partition('=') for line in out.splitlines())
        if key and value
    }

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

        paths = vc_env.get('path', '').split(os.pathsep)
        self.cc = _find_exe("cl.exe", paths)
        self.linker = _find_exe("link.exe", paths)
        self.lib = _find_exe("lib.exe", paths)
        self.rc = _find_exe("rc.exe", paths)   # resource compiler
        self.mc = _find_exe("mc.exe", paths)   # message compiler
        self.mt = _find_exe("mt.exe", paths)   # message compiler

        for dir in vc_env.get('include', '').split(os.pathsep):
            if dir:
                self.add_include_dir(dir)

        for dir in vc_env.get('lib', '').split(os.pathsep):
            if dir:
                self.add_library_dir(dir)

        self.preprocess_options = None
        self.compile_options = [
            '/nologo', '/Ox', '/MD', '/W3', '/GL', '/DNDEBUG'
        ]
        self.compile_options_debug = [
            '/nologo', '/Od', '/MDd', '/Zi', '/W3', '/D_DEBUG'
        ]

        self.ldflags_shared = [
            '/nologo', '/DLL', '/INCREMENTAL:NO'
        ]
        self.ldflags_shared_debug = [
            '/nologo', '/DLL', '/INCREMENTAL:no', '/DEBUG:FULL'
        ]
        self.ldflags_static = [
            '/nologo'
        ]

        self.initialized = True

    # -- Worker methods ------------------------------------------------

    def object_filenames(self,
                         source_filenames,
                         strip_dir=0,
                         output_dir=''):
        ext_map = {ext: self.obj_extension for ext in self.src_extensions}
        ext_map.update((ext, self.res_extension)
                for ext in self._rc_extensions + self._mc_extensions)

        def make_out_path(p):
            base, ext = os.path.splitext(p)
            if strip_dir:
                base = os.path.basename(base)
            else:
                _, base = os.path.splitdrive(base)
                if base.startswith((os.path.sep, os.path.altsep)):
                    base = base[1:]
            try:
                return base + ext_map[ext]
            except LookupError:
                # Better to raise an exception instead of silently continuing
                # and later complain about sources and targets having
                # different lengths
                raise CompileError("Don't know how to compile {}".format(p))

        output_dir = output_dir or ''
        return [
            os.path.join(output_dir, make_out_path(src_name))
            for src_name in source_filenames
        ]


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
            ldflags = (self.ldflags_shared_debug if debug
                       else self.ldflags_shared)
            if target_desc == CCompiler.EXECUTABLE:
                ldflags = ldflags[1:]

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
            build_temp = os.path.dirname(objects[0])
            if export_symbols is not None:
                (dll_name, dll_ext) = os.path.splitext(
                    os.path.basename(output_filename))
                implib_file = os.path.join(
                    build_temp,
                    self.library_filename(dll_name))
                ld_args.append ('/IMPLIB:' + implib_file)

            self.manifest_setup_ldargs(output_filename, build_temp, ld_args)

            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend(extra_postargs)

            self.mkpath(os.path.dirname(output_filename))
            try:
                self.spawn([self.linker] + ld_args)
            except DistutilsExecError as msg:
                raise LinkError(msg)

            # embed the manifest
            # XXX - this is somewhat fragile - if mt.exe fails, distutils
            # will still consider the DLL up-to-date, but it will not have a
            # manifest.  Maybe we should link to a temp file?  OTOH, that
            # implies a build environment error that shouldn't go undetected.
            mfinfo = self.manifest_get_embed_info(target_desc, ld_args)
            if mfinfo is not None:
                mffilename, mfid = mfinfo
                out_arg = '-outputresource:{};{}'.format(output_filename, mfid)
                try:
                    self.spawn([self.mt, '-nologo', '-manifest',
                                mffilename, out_arg])
                except DistutilsExecError as msg:
                    raise LinkError(msg)
        else:
            log.debug("skipping %s (up-to-date)", output_filename)

    def manifest_setup_ldargs(self, output_filename, build_temp, ld_args):
        # If we need a manifest at all, an embedded manifest is recommended.
        # See MSDN article titled
        # "How to: Embed a Manifest Inside a C/C++ Application"
        # (currently at http://msdn2.microsoft.com/en-us/library/ms235591(VS.80).aspx)
        # Ask the linker to generate the manifest in the temp dir, so
        # we can check it, and possibly embed it, later.
        temp_manifest = os.path.join(
                build_temp,
                os.path.basename(output_filename) + ".manifest")
        ld_args.append('/MANIFESTFILE:' + temp_manifest)

    def manifest_get_embed_info(self, target_desc, ld_args):
        # If a manifest should be embedded, return a tuple of
        # (manifest_filename, resource_id).  Returns None if no manifest
        # should be embedded.  See http://bugs.python.org/issue7833 for why
        # we want to avoid any manifest for extension modules if we can)
        for arg in ld_args:
            if arg.startswith("/MANIFESTFILE:"):
                temp_manifest = arg.split(":", 1)[1]
                break
        else:
            # no /MANIFESTFILE so nothing to do.
            return None
        if target_desc == CCompiler.EXECUTABLE:
            # by default, executables always get the manifest with the
            # CRT referenced.
            mfid = 1
        else:
            # Extension modules try and avoid any manifest if possible.
            mfid = 2
            temp_manifest = self._remove_visual_c_ref(temp_manifest)
        if temp_manifest is None:
            return None
        return temp_manifest, mfid

    def _remove_visual_c_ref(self, manifest_file):
        try:
            # Remove references to the Visual C runtime, so they will
            # fall through to the Visual C dependency of Python.exe.
            # This way, when installed for a restricted user (e.g.
            # runtimes are not in WinSxS folder, but in Python's own
            # folder), the runtimes do not need to be in every folder
            # with .pyd's.
            # Returns either the filename of the modified manifest or
            # None if no manifest should be embedded.
            manifest_f = open(manifest_file)
            try:
                manifest_buf = manifest_f.read()
            finally:
                manifest_f.close()
            pattern = re.compile(
                r"""<assemblyIdentity.*?name=("|')Microsoft\."""\
                r"""VC\d{2}\.CRT("|').*?(/>|</assemblyIdentity>)""",
                re.DOTALL)
            manifest_buf = re.sub(pattern, "", manifest_buf)
            pattern = "<dependentAssembly>\s*</dependentAssembly>"
            manifest_buf = re.sub(pattern, "", manifest_buf)
            # Now see if any other assemblies are referenced - if not, we
            # don't want a manifest embedded.
            pattern = re.compile(
                r"""<assemblyIdentity.*?name=(?:"|')(.+?)(?:"|')"""
                r""".*?(?:/>|</assemblyIdentity>)""", re.DOTALL)
            if re.search(pattern, manifest_buf) is None:
                return None

            manifest_f = open(manifest_file, 'w')
            try:
                manifest_f.write(manifest_buf)
                return manifest_file
            finally:
                manifest_f.close()
        except OSError:
            pass

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
