# Script for building the _ssl and _hashlib modules for Windows.
# Uses Perl to setup the OpenSSL environment correctly
# and build OpenSSL, then invokes a simple nmake session
# for the actual _ssl.pyd and _hashlib.pyd DLLs.

# THEORETICALLY, you can:
# * Unpack the latest SSL release one level above your main Python source
#   directory.  It is likely you will already find the zlib library and
#   any other external packages there.
# * Install ActivePerl and ensure it is somewhere on your path.
# * Run this script from the PCBuild directory.
#
# it should configure and build SSL, then build the _ssl and _hashlib
# Python extensions without intervention.

# Modified by Christian Heimes
# Now this script supports pre-generated makefiles and assembly files.
# Developers don't need an installation of Perl anymore to build Python. A svn
# checkout from our svn repository is enough.
#
# In Order to create the files in the case of an update you still need Perl.
# Run build_ssl in this order:
# python.exe build_ssl.py Release x64
# python.exe build_ssl.py Release Win32

from __future__ import with_statement, print_function
import os
import re
import sys
import time
import subprocess
from shutil import copy
from distutils import log
from distutils.spawn import find_executable
from distutils.file_util import copy_file
from distutils.sysconfig import parse_makefile, expand_makefile_vars

# The mk1mf.pl output filename template
# !!! This must match what is used in prepare_ssl.py
MK1MF_FMT = 'ms\\nt{}.mak'

# The header files output directory name template
# !!! This must match what is used in prepare_ssl.py
INCLUDE_FMT = 'include{}'

# Fetch all the directory definitions from VC properties
def get_project_properties(propfile):
    macro_pattern = r'<UserMacro\s+Name="([^"]+?)"\s+Value="([^"]*?)"\s*/>'
    with open(propfile) as fin:
        items = re.findall(macro_pattern, fin.read(), re.MULTILINE)
    props = dict(items)
    for name, value in items:
        try:
            props[name] = expand_makefile_vars(value, props)
        except TypeError:
            # value contains undefined variable reference, drop it
            del props[name]
    return props


_variable_rx = re.compile(r"([a-zA-Z][a-zA-Z0-9_]+)\s*=\s*(.*)")
def fix_makefile(makefile, platform_makefile, suffix):
    """Fix some stuff in all makefiles
    """
    subs = {
        'PERL': 'rem',  # just in case
        'CP': 'copy',
        'MKDIR': 'mkdir',
        'OUT_D': 'out' + suffix,
        'TMP_D': 'tmp' + suffix,
        'INC_D': INCLUDE_FMT.format(suffix),
        'INCO_D': '$(INC_D)\\openssl',
        }
    with open(platform_makefile) as fin, open(makefile, 'w') as fout:
        for line in fin:
            m = _variable_rx.match(line)
            if m:
                name = m.group(1)
                if name in subs:
                    line = '%s=%s\n' % (name, subs[name])
            fout.write(line)


_copy_rx = re.compile(r'\t\$\(PERL\) '
                      r'\$\(SRC_D\)\\util\\copy-if-different.pl '
                      r'"([^"]+)"\s+"([^"]+)"')
def copy_files(makefile, makevars):
    # Create the destination directories (see 'init' rule in nt.dll)
    for varname in ('TMP_D', 'LIB_D', 'INC_D', 'INCO_D'):
        dirname = makevars[varname]
        if not os.path.isdir(dirname):
            os.mkdir(dirname)
    # Process the just local library headers (HEADER) as installed headers
    # (EXHEADER) are handled by prepare_ssl.py (see 'headers' rule in nt.dll)
    headers = set(makevars['HEADER'].split())
    with open(makefile) as fin:
        for line in fin:
            m = _copy_rx.match(line)
            if m:
                src, dst = m.groups()
                src = expand_makefile_vars(src, makevars)
                dst = expand_makefile_vars(dst, makevars)
                if dst in headers:
                    copy_file(src, dst, preserve_times=False, update=True)


# Update buildinf.h for the build platform.
def fix_buildinf(makevars):
    platform_cpp_symbol = 'MK1MF_PLATFORM_'
    platform_cpp_symbol += makevars['PLATFORM'].replace('-', '_')
    fn = expand_makefile_vars('$(INCL_D)\\buildinf.h', makevars)
    with open(fn, 'w') as f:
        # sanity check
        f.write(('#ifndef {}\n'
                 '  #error "Windows build (PLATFORM={PLATFORM}) only"\n'
                 '#endif\n').format(platform_cpp_symbol, **makevars))
        buildinf = (
            '#define CFLAGS "compiler: cl {CFLAG}"\n'
            '#define PLATFORM "{PLATFORM}"\n'
            '#define DATE "{}"\n'
            ).format(time.asctime(time.gmtime()),
                     **makevars)
        f.write(buildinf)
    print('Updating buildinf:')
    print(buildinf)
    sys.stdout.flush()


def main():
    if sys.argv[1] == "Debug":
        print("OpenSSL debug builds aren't supported.")
    elif sys.argv[1] != "Release":
        raise ValueError('Unrecognized configuration: %s' % sys.argv[1])

    if sys.argv[2] == "Win32":
        platform = "VC-WIN32"
        suffix = '32'
    elif sys.argv[2] == "x64":
        platform = "VC-WIN64A"
        suffix = '64'
    else:
        raise ValueError('Unrecognized platform: %s' % sys.argv[2])

    # Have the distutils functions display information output
    log.set_verbosity(1)

    # Use the same properties that are used in the VS projects
    solution_dir = os.path.dirname(__file__)
    propfile = os.path.join(solution_dir, 'pyproject.vsprops')
    props = get_project_properties(propfile)

    # Ensure we have the necessary external depenedencies
    ssl_dir = os.path.join(solution_dir, props['opensslDir'])
    if not os.path.isdir(ssl_dir):
        print("Could not find the OpenSSL sources, try running "
              "'build.bat -e'")
        sys.exit(1)

    # Ensure the executables used herein are available.
    if not find_executable('nmake.exe'):
        print('Could not find nmake.exe, try running env.bat')
        sys.exit(1)

    # add our copy of NASM to PATH.  It will be on the same level as openssl
    externals_dir = os.path.join(solution_dir, props['externalsDir'])
    for dir in os.listdir(externals_dir):
        if dir.startswith('nasm'):
            nasm_dir = os.path.join(externals_dir, dir)
            nasm_dir = os.path.abspath(nasm_dir)
            old_path = os.environ['PATH']
            os.environ['PATH'] = os.pathsep.join([nasm_dir, old_path])
            break
    else:
        if not find_executable('nasm.exe'):
            print('Could not find nasm.exe, please add to PATH')
            sys.exit(1)

    # If the ssl makefiles do not exist, we invoke PCbuild/prepare_ssl.py
    # to generate them.
    platform_makefile = MK1MF_FMT.format(suffix)
    if not os.path.isfile(os.path.join(ssl_dir, platform_makefile)):
        pcbuild_dir = os.path.join(os.path.dirname(externals_dir), 'PCbuild')
        prepare_ssl = os.path.join(pcbuild_dir, 'prepare_ssl.py')
        rc = subprocess.call([sys.executable, prepare_ssl, ssl_dir])
        if rc:
            print('Executing', prepare_ssl, 'failed (error %d)' % rc)
            sys.exit(rc)

    old_cd = os.getcwd()
    try:
        os.chdir(ssl_dir)

        # Get the variables defined in the current makefile, if it exists.
        makefile = MK1MF_FMT.format('')
        try:
            makevars = parse_makefile(makefile)
        except EnvironmentError:
            makevars = {'PLATFORM': None}

        # Rebuild the makefile when building for different a platform than
        # the last run.
        if makevars['PLATFORM'] != platform:
            print("Updating the makefile...")
            sys.stdout.flush()
            # Firstly, apply the changes for the platform makefile into
            # a temporary file to prevent any errors from this script
            # causing false positives on subsequent runs.
            new_makefile = makefile + '.new'
            fix_makefile(new_makefile, platform_makefile, suffix)
            makevars = parse_makefile(new_makefile)

            # Secondly, perform the make recipes that use Perl
            copy_files(new_makefile, makevars)

            # Set our build information in buildinf.h.
            # XXX: This isn't needed for a properly "prepared" SSL, but
            # it fixes the current checked-in external (as of 2017-05).
            fix_buildinf(makevars)

            # Finally, move the temporary file to its real destination.
            if os.path.exists(makefile):
                os.remove(makefile)
            os.rename(new_makefile, makefile)

        # Now run make.
        makeCommand = "nmake /nologo /f \"%s\" lib" % makefile
        print("Executing ssl makefiles:", makeCommand)
        sys.stdout.flush()
        rc = os.system(makeCommand)
        if rc:
            print("Executing", makefile, "failed (error %d)" % rc)
            sys.exit(rc)
    finally:
        os.chdir(old_cd)
    sys.exit(rc)

if __name__=='__main__':
    main()
