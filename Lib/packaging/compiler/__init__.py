"""Compiler abstraction model used by packaging.

An abstract base class is defined in the ccompiler submodule, and
concrete implementations suitable for various platforms are defined in
the other submodules.  The extension module is also placed in this
package.

In general, code should not instantiate compiler classes directly but
use the new_compiler and customize_compiler functions provided in this
module.

The compiler system has a registration API: get_default_compiler,
set_compiler, show_compilers.
"""

import os
import sys
import re
import sysconfig

from packaging.util import resolve_name
from packaging.errors import PackagingPlatformError
from packaging import logger

def customize_compiler(compiler):
    """Do any platform-specific customization of a CCompiler instance.

    Mainly needed on Unix, so we can plug in the information that
    varies across Unices and is stored in Python's Makefile.
    """
    if compiler.name == "unix":
        cc, cxx, opt, cflags, ccshared, ldshared, so_ext, ar, ar_flags = (
            sysconfig.get_config_vars('CC', 'CXX', 'OPT', 'CFLAGS',
                                      'CCSHARED', 'LDSHARED', 'SO', 'AR',
                                      'ARFLAGS'))

        if 'CC' in os.environ:
            cc = os.environ['CC']
        if 'CXX' in os.environ:
            cxx = os.environ['CXX']
        if 'LDSHARED' in os.environ:
            ldshared = os.environ['LDSHARED']
        if 'CPP' in os.environ:
            cpp = os.environ['CPP']
        else:
            cpp = cc + " -E"           # not always
        if 'LDFLAGS' in os.environ:
            ldshared = ldshared + ' ' + os.environ['LDFLAGS']
        if 'CFLAGS' in os.environ:
            cflags = opt + ' ' + os.environ['CFLAGS']
            ldshared = ldshared + ' ' + os.environ['CFLAGS']
        if 'CPPFLAGS' in os.environ:
            cpp = cpp + ' ' + os.environ['CPPFLAGS']
            cflags = cflags + ' ' + os.environ['CPPFLAGS']
            ldshared = ldshared + ' ' + os.environ['CPPFLAGS']
        if 'AR' in os.environ:
            ar = os.environ['AR']
        if 'ARFLAGS' in os.environ:
            archiver = ar + ' ' + os.environ['ARFLAGS']
        else:
            if ar_flags is not None:
                archiver = ar + ' ' + ar_flags
            else:
                # see if its the proper default value
                # mmm I don't want to backport the makefile
                archiver = ar + ' rc'

        cc_cmd = cc + ' ' + cflags
        compiler.set_executables(
            preprocessor=cpp,
            compiler=cc_cmd,
            compiler_so=cc_cmd + ' ' + ccshared,
            compiler_cxx=cxx,
            linker_so=ldshared,
            linker_exe=cc,
            archiver=archiver)

        compiler.shared_lib_extension = so_ext


# Map a sys.platform/os.name ('posix', 'nt') to the default compiler
# type for that platform. Keys are interpreted as re match
# patterns. Order is important; platform mappings are preferred over
# OS names.
_default_compilers = (

    # Platform string mappings

    # on a cygwin built python we can use gcc like an ordinary UNIXish
    # compiler
    ('cygwin.*', 'unix'),
    ('os2emx', 'emx'),

    # OS name mappings
    ('posix', 'unix'),
    ('nt', 'msvc'),

    )

def get_default_compiler(osname=None, platform=None):
    """ Determine the default compiler to use for the given platform.

        osname should be one of the standard Python OS names (i.e. the
        ones returned by os.name) and platform the common value
        returned by sys.platform for the platform in question.

        The default values are os.name and sys.platform in case the
        parameters are not given.

    """
    if osname is None:
        osname = os.name
    if platform is None:
        platform = sys.platform
    for pattern, compiler in _default_compilers:
        if re.match(pattern, platform) is not None or \
           re.match(pattern, osname) is not None:
            return compiler
    # Defaults to Unix compiler
    return 'unix'


# compiler mapping
# XXX useful to expose them? (i.e. get_compiler_names)
_COMPILERS = {
    'unix': 'packaging.compiler.unixccompiler.UnixCCompiler',
    'msvc': 'packaging.compiler.msvccompiler.MSVCCompiler',
    'cygwin': 'packaging.compiler.cygwinccompiler.CygwinCCompiler',
    'mingw32': 'packaging.compiler.cygwinccompiler.Mingw32CCompiler',
    'bcpp': 'packaging.compiler.bcppcompiler.BCPPCompiler',
}

def set_compiler(location):
    """Add or change a compiler"""
    cls = resolve_name(location)
    # XXX we want to check the class here
    _COMPILERS[cls.name] = cls


def show_compilers():
    """Print list of available compilers (used by the "--help-compiler"
    options to "build", "build_ext", "build_clib").
    """
    from packaging.fancy_getopt import FancyGetopt
    compilers = []

    for name, cls in _COMPILERS.items():
        if isinstance(cls, str):
            cls = resolve_name(cls)
            _COMPILERS[name] = cls

        compilers.append(("compiler=" + name, None, cls.description))

    compilers.sort()
    pretty_printer = FancyGetopt(compilers)
    pretty_printer.print_help("List of available compilers:")


def new_compiler(plat=None, compiler=None, verbose=0, dry_run=False,
                 force=False):
    """Generate an instance of some CCompiler subclass for the supplied
    platform/compiler combination.  'plat' defaults to 'os.name'
    (eg. 'posix', 'nt'), and 'compiler' defaults to the default compiler
    for that platform.  Currently only 'posix' and 'nt' are supported, and
    the default compilers are "traditional Unix interface" (UnixCCompiler
    class) and Visual C++ (MSVCCompiler class).  Note that it's perfectly
    possible to ask for a Unix compiler object under Windows, and a
    Microsoft compiler object under Unix -- if you supply a value for
    'compiler', 'plat' is ignored.
    """
    if plat is None:
        plat = os.name

    try:
        if compiler is None:
            compiler = get_default_compiler(plat)

        cls = _COMPILERS[compiler]
    except KeyError:
        msg = "don't know how to compile C/C++ code on platform '%s'" % plat
        if compiler is not None:
            msg = msg + " with '%s' compiler" % compiler
        raise PackagingPlatformError(msg)

    if isinstance(cls, str):
        cls = resolve_name(cls)
        _COMPILERS[compiler] = cls


    # XXX The None is necessary to preserve backwards compatibility
    # with classes that expect verbose to be the first positional
    # argument.
    return cls(None, dry_run, force)


def gen_preprocess_options(macros, include_dirs):
    """Generate C pre-processor options (-D, -U, -I) as used by at least
    two types of compilers: the typical Unix compiler and Visual C++.
    'macros' is the usual thing, a list of 1- or 2-tuples, where (name,)
    means undefine (-U) macro 'name', and (name,value) means define (-D)
    macro 'name' to 'value'.  'include_dirs' is just a list of directory
    names to be added to the header file search path (-I).  Returns a list
    of command-line options suitable for either Unix compilers or Visual
    C++.
    """
    # XXX it would be nice (mainly aesthetic, and so we don't generate
    # stupid-looking command lines) to go over 'macros' and eliminate
    # redundant definitions/undefinitions (ie. ensure that only the
    # latest mention of a particular macro winds up on the command
    # line).  I don't think it's essential, though, since most (all?)
    # Unix C compilers only pay attention to the latest -D or -U
    # mention of a macro on their command line.  Similar situation for
    # 'include_dirs'.  I'm punting on both for now.  Anyways, weeding out
    # redundancies like this should probably be the province of
    # CCompiler, since the data structures used are inherited from it
    # and therefore common to all CCompiler classes.

    pp_opts = []
    for macro in macros:

        if not isinstance(macro, tuple) and 1 <= len(macro) <= 2:
            raise TypeError(
                "bad macro definition '%s': each element of 'macros'"
                "list must be a 1- or 2-tuple" % macro)

        if len(macro) == 1:        # undefine this macro
            pp_opts.append("-U%s" % macro[0])
        elif len(macro) == 2:
            if macro[1] is None:    # define with no explicit value
                pp_opts.append("-D%s" % macro[0])
            else:
                # XXX *don't* need to be clever about quoting the
                # macro value here, because we're going to avoid the
                # shell at all costs when we spawn the command!
                pp_opts.append("-D%s=%s" % macro)

    for dir in include_dirs:
        pp_opts.append("-I%s" % dir)

    return pp_opts


def gen_lib_options(compiler, library_dirs, runtime_library_dirs, libraries):
    """Generate linker options for searching library directories and
    linking with specific libraries.

    'libraries' and 'library_dirs' are, respectively, lists of library names
    (not filenames!) and search directories.  Returns a list of command-line
    options suitable for use with some compiler (depending on the two format
    strings passed in).
    """
    lib_opts = []

    for dir in library_dirs:
        lib_opts.append(compiler.library_dir_option(dir))

    for dir in runtime_library_dirs:
        opt = compiler.runtime_library_dir_option(dir)
        if isinstance(opt, list):
            lib_opts.extend(opt)
        else:
            lib_opts.append(opt)

    # XXX it's important that we *not* remove redundant library mentions!
    # sometimes you really do have to say "-lfoo -lbar -lfoo" in order to
    # resolve all symbols.  I just hope we never have to say "-lfoo obj.o
    # -lbar" to get things to work -- that's certainly a possibility, but a
    # pretty nasty way to arrange your C code.

    for lib in libraries:
        lib_dir, lib_name = os.path.split(lib)
        if lib_dir != '':
            lib_file = compiler.find_library_file([lib_dir], lib_name)
            if lib_file is not None:
                lib_opts.append(lib_file)
            else:
                logger.warning("no library file corresponding to "
                              "'%s' found (skipping)" % lib)
        else:
            lib_opts.append(compiler.library_option(lib))

    return lib_opts
