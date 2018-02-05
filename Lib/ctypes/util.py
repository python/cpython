import os
import subprocess
import sys

# find_library(name) returns the pathname of a library, or None.
if os.name == "nt":

    def _get_build_version():
        """Return the version of MSVC that was used to build Python.

        For Python 2.3 and up, the version number is included in
        sys.version.  For earlier versions, assume the compiler is MSVC 6.
        """
        # This function was copied from Lib/distutils/msvccompiler.py
        prefix = "MSC v."
        i = sys.version.find(prefix)
        if i == -1:
            return 6
        i = i + len(prefix)
        s, rest = sys.version[i:].split(" ", 1)
        majorVersion = int(s[:-2]) - 6
        minorVersion = int(s[2:3]) / 10.0
        # I don't think paths are affected by minor version in version 6
        if majorVersion == 6:
            minorVersion = 0
        if majorVersion >= 6:
            return majorVersion + minorVersion
        # else we don't know what version of the compiler this is
        return None

    def find_msvcrt():
        """Return the name of the VC runtime dll"""
        version = _get_build_version()
        if version is None:
            # better be safe than sorry
            return None
        if version <= 6:
            clibname = 'msvcrt'
        else:
            clibname = 'msvcr%d' % (version * 10)

        # If python was built with in debug mode
        import imp
        if imp.get_suffixes()[0][0] == '_d.pyd':
            clibname += 'd'
        return clibname+'.dll'

    def find_library(name):
        if name in ('c', 'm'):
            return find_msvcrt()
        # See MSDN for the REAL search order.
        for directory in os.environ['PATH'].split(os.pathsep):
            fname = os.path.join(directory, name)
            if os.path.isfile(fname):
                return fname
            if fname.lower().endswith(".dll"):
                continue
            fname = fname + ".dll"
            if os.path.isfile(fname):
                return fname
        return None

if os.name == "ce":
    # search path according to MSDN:
    # - absolute path specified by filename
    # - The .exe launch directory
    # - the Windows directory
    # - ROM dll files (where are they?)
    # - OEM specified search path: HKLM\Loader\SystemPath
    def find_library(name):
        return name

if os.name == "posix" and sys.platform == "darwin":
    from ctypes.macholib.dyld import dyld_find as _dyld_find
    def find_library(name):
        possible = ['lib%s.dylib' % name,
                    '%s.dylib' % name,
                    '%s.framework/%s' % (name, name)]
        for name in possible:
            try:
                return _dyld_find(name)
            except ValueError:
                continue
        return None

elif sys.platform.startswith("aix"):
    # AIX has two styles of storing shared libraries
    # GNU auto_tools refer to these as svr4 and aix
    # svr4 (System V Release 4) is a regular file, often with .so as suffix
    # AIX style uses an archive (suffix .a) with members (e.g., shr.o, libssl.so)
    # see issue#26439 and _aix.py for more details

    from ctypes._aix import find_library

elif os.name == "posix":
    # Andreas Degert's find functions, using gcc, /sbin/ldconfig, objdump
    import re, tempfile, errno

    def _findLib_gcc(name):
        # Run GCC's linker with the -t (aka --trace) option and examine the
        # library name it prints out. The GCC command will fail because we
        # haven't supplied a proper program with main(), but that does not
        # matter.
        expr = r'[^\(\)\s]*lib%s\.[^\(\)\s]*' % re.escape(name)
        cmd = 'if type gcc >/dev/null 2>&1; then CC=gcc; elif type cc >/dev/null 2>&1; then CC=cc;else exit; fi;' \
              'LANG=C LC_ALL=C $CC -Wl,-t -o "$2" 2>&1 -l"$1"'

        temp = tempfile.NamedTemporaryFile()
        try:
            proc = subprocess.Popen((cmd, '_findLib_gcc', name, temp.name),
                                    shell=True,
                                    stdout=subprocess.PIPE)
            [trace, _] = proc.communicate()
        finally:
            try:
                temp.close()
            except OSError, e:
                # ENOENT is raised if the file was already removed, which is
                # the normal behaviour of GCC if linking fails
                if e.errno != errno.ENOENT:
                    raise
        res = re.search(expr, trace)
        if not res:
            return None
        return res.group(0)


    if sys.platform == "sunos5":
        # use /usr/ccs/bin/dump on solaris
        def _get_soname(f):
            if not f:
                return None

            null = open(os.devnull, "wb")
            try:
                with null:
                    proc = subprocess.Popen(("/usr/ccs/bin/dump", "-Lpv", f),
                                            stdout=subprocess.PIPE,
                                            stderr=null)
            except OSError:  # E.g. command not found
                return None
            [data, _] = proc.communicate()
            res = re.search(br'\[.*\]\sSONAME\s+([^\s]+)', data)
            if not res:
                return None
            return res.group(1)
    else:
        def _get_soname(f):
            # assuming GNU binutils / ELF
            if not f:
                return None
            cmd = 'if ! type objdump >/dev/null 2>&1; then exit; fi;' \
                  'objdump -p -j .dynamic 2>/dev/null "$1"'
            proc = subprocess.Popen((cmd, '_get_soname', f), shell=True,
                                    stdout=subprocess.PIPE)
            [dump, _] = proc.communicate()
            res = re.search(br'\sSONAME\s+([^\s]+)', dump)
            if not res:
                return None
            return res.group(1)

    if (sys.platform.startswith("freebsd")
        or sys.platform.startswith("openbsd")
        or sys.platform.startswith("dragonfly")):

        def _num_version(libname):
            # "libxyz.so.MAJOR.MINOR" => [ MAJOR, MINOR ]
            parts = libname.split(b".")
            nums = []
            try:
                while parts:
                    nums.insert(0, int(parts.pop()))
            except ValueError:
                pass
            return nums or [sys.maxint]

        def find_library(name):
            ename = re.escape(name)
            expr = r':-l%s\.\S+ => \S*/(lib%s\.\S+)' % (ename, ename)

            null = open(os.devnull, 'wb')
            try:
                with null:
                    proc = subprocess.Popen(('/sbin/ldconfig', '-r'),
                                            stdout=subprocess.PIPE,
                                            stderr=null)
            except OSError:  # E.g. command not found
                data = b''
            else:
                [data, _] = proc.communicate()

            res = re.findall(expr, data)
            if not res:
                return _get_soname(_findLib_gcc(name))
            res.sort(key=_num_version)
            return res[-1]

    elif sys.platform == "sunos5":

        def _findLib_crle(name, is64):
            if not os.path.exists('/usr/bin/crle'):
                return None

            env = dict(os.environ)
            env['LC_ALL'] = 'C'

            if is64:
                args = ('/usr/bin/crle', '-64')
            else:
                args = ('/usr/bin/crle',)

            paths = None
            null = open(os.devnull, 'wb')
            try:
                with null:
                    proc = subprocess.Popen(args,
                                            stdout=subprocess.PIPE,
                                            stderr=null,
                                            env=env)
            except OSError:  # E.g. bad executable
                return None
            try:
                for line in proc.stdout:
                    line = line.strip()
                    if line.startswith(b'Default Library Path (ELF):'):
                        paths = line.split()[4]
            finally:
                proc.stdout.close()
                proc.wait()

            if not paths:
                return None

            for dir in paths.split(":"):
                libfile = os.path.join(dir, "lib%s.so" % name)
                if os.path.exists(libfile):
                    return libfile

            return None

        def find_library(name, is64 = False):
            return _get_soname(_findLib_crle(name, is64) or _findLib_gcc(name))

    else:

        def _findSoname_ldconfig(name):
            import struct
            if struct.calcsize('l') == 4:
                machine = os.uname()[4] + '-32'
            else:
                machine = os.uname()[4] + '-64'
            mach_map = {
                'x86_64-64': 'libc6,x86-64',
                'ppc64-64': 'libc6,64bit',
                'sparc64-64': 'libc6,64bit',
                's390x-64': 'libc6,64bit',
                'ia64-64': 'libc6,IA-64',
                }
            abi_type = mach_map.get(machine, 'libc6')

            # XXX assuming GLIBC's ldconfig (with option -p)
            expr = r'\s+(lib%s\.[^\s]+)\s+\(%s' % (re.escape(name), abi_type)

            env = dict(os.environ)
            env['LC_ALL'] = 'C'
            env['LANG'] = 'C'
            null = open(os.devnull, 'wb')
            try:
                with null:
                    p = subprocess.Popen(['/sbin/ldconfig', '-p'],
                                          stderr=null,
                                          stdout=subprocess.PIPE,
                                          env=env)
            except OSError:  # E.g. command not found
                return None
            [data, _] = p.communicate()
            res = re.search(expr, data)
            if not res:
                return None
            return res.group(1)

        def find_library(name):
            return _findSoname_ldconfig(name) or _get_soname(_findLib_gcc(name))

################################################################
# test code

def test():
    from ctypes import cdll
    if os.name == "nt":
        print cdll.msvcrt
        print cdll.load("msvcrt")
        print find_library("msvcrt")

    if os.name == "posix":
        # find and load_version
        print find_library("m")
        print find_library("c")
        print find_library("bz2")

        # getattr
##        print cdll.m
##        print cdll.bz2

        # load
        if sys.platform == "darwin":
            print(cdll.LoadLibrary("libm.dylib"))
            print(cdll.LoadLibrary("libcrypto.dylib"))
            print(cdll.LoadLibrary("libSystem.dylib"))
            print(cdll.LoadLibrary("System.framework/System"))
        # issue-26439 - fix broken test call for AIX
        elif sys.platform.startswith("aix"):
            from ctypes import CDLL
            from _ctypes import RTLD_MEMBER
            if sys.maxsize < 2**32:
                print("Using CDLL(name, RTLD_MEMBER): %s" % CDLL('libc.a(shr.o)', RTLD_MEMBER))
                print("Using cdll.LoadLibrary(): %s" % cdll.LoadLibrary('libc.a(shr.o)'))
                # librpm.so is only available as 32-bit shared library
                print("rpm: %s %s" % (find_library("rpm"), cdll.LoadLibrary("librpm.so")))
            else:
                print("Using CDLL(name, RTLD_MEMBER): %s", CDLL('libc.a(shr_64.o)', RTLD_MEMBER))
                print("Using cdll.LoadLibrary(): %s", cdll.LoadLibrary('libc.a(shr_64.o)'))
            print("crypt :: %s" % find_library('crypt'))
            print("crypt :: %s" % cdll.LoadLibrary(find_library('crypt')))
            print("crypto:: %s" % find_library('crypto'))
            print("crypto:: %s" % cdll.LoadLibrary(find_library('crypto')))
        else:
            print(cdll.LoadLibrary("libm.so"))
            print(cdll.LoadLibrary("libcrypt.so"))
            print(find_library("crypt"))

if __name__ == "__main__":
    test()
