#! /usr/bin/env python3
# Script for preparing OpenSSL for building on Windows.
# Uses Perl to create nmake makefiles and otherwise prepare the way
# for building on 32 or 64 bit platforms.

# Script originally authored by Mark Hammond.
# Major revisions by:
#   Martin v. Löwis
#   Christian Heimes
#   Zachary Ware

# THEORETICALLY, you can:
# * Unpack the latest OpenSSL release where $(opensslDir) in
#   PCbuild\pyproject.props expects it to be.
# * Install ActivePerl and ensure it is somewhere on your path.
# * Run this script with the OpenSSL source dir as the only argument.
#
# it should configure OpenSSL such that it is ready to be built by
# ssl.vcxproj on 32 or 64 bit platforms.

from __future__ import print_function

import os
import re
import sys
import subprocess
from shutil import copy

# Find all "foo.exe" files on the PATH.
def find_all_on_path(filename, extras=None):
    entries = os.environ["PATH"].split(os.pathsep)
    ret = []
    for p in entries:
        fname = os.path.abspath(os.path.join(p, filename))
        if os.path.isfile(fname) and fname not in ret:
            ret.append(fname)
    if extras:
        for p in extras:
            fname = os.path.abspath(os.path.join(p, filename))
            if os.path.isfile(fname) and fname not in ret:
                ret.append(fname)
    return ret


# Find a suitable Perl installation for OpenSSL.
# cygwin perl does *not* work.  ActivePerl does.
# Being a Perl dummy, the simplest way I can check is if the "Win32" package
# is available.
def find_working_perl(perls):
    for perl in perls:
        try:
            subprocess.check_output([perl, "-e", "use Win32;"])
        except subprocess.CalledProcessError:
            continue
        else:
            return perl

    if perls:
        print("The following perl interpreters were found:")
        for p in perls:
            print(" ", p)
        print(" None of these versions appear suitable for building OpenSSL")
    else:
        print("NO perl interpreters were found on this machine at all!")
    print(" Please install ActivePerl and ensure it appears on your path")


def copy_includes(makefile, suffix):
    dir = 'inc'+suffix+'\\openssl'
    try:
        os.makedirs(dir)
    except OSError:
        pass
    copy_if_different = r'$(PERL) $(SRC_D)\util\copy-if-different.pl'
    with open(makefile) as fin:
        for line in fin:
            if copy_if_different in line:
                perl, script, src, dest = line.split()
                if not '$(INCO_D)' in dest:
                    continue
                # We're in the root of the source tree
                src = src.replace('$(SRC_D)', '.').strip('"')
                dest = dest.strip('"').replace('$(INCO_D)', dir)
                print('copying', src, 'to', dest)
                copy(src, dest)


def run_configure(configure, do_script):
    print("perl Configure "+configure+" no-idea no-mdc2")
    os.system("perl Configure "+configure+" no-idea no-mdc2")
    print(do_script)
    os.system(do_script)

def fix_uplink():
    # uplink.c tries to find the OPENSSL_Applink function exported from the current
    # executable. However, we export it from _ssl[_d].pyd instead. So we update the
    # module name here before building.
    with open('ms\\uplink.c', 'r', encoding='utf-8') as f1:
        code = list(f1)
    os.replace('ms\\uplink.c', 'ms\\uplink.c.orig')
    already_patched = False
    with open('ms\\uplink.c', 'w', encoding='utf-8') as f2:
        for line in code:
            if not already_patched:
                if re.search('MODIFIED FOR CPYTHON _ssl MODULE', line):
                    already_patched = True
                elif re.match(r'^\s+if\s*\(\(h\s*=\s*GetModuleHandle[AW]?\(NULL\)\)\s*==\s*NULL\)', line):
                    f2.write("/* MODIFIED FOR CPYTHON _ssl MODULE */\n")
                    f2.write('if ((h = GetModuleHandleW(L"_ssl.pyd")) == NULL) if ((h = GetModuleHandleW(L"_ssl_d.pyd")) == NULL)\n')
                    already_patched = True
            f2.write(line)
    if not already_patched:
        print("WARN: failed to patch ms\\uplink.c")

def prep(arch):
    makefile_template = "ms\\ntdll{}.mak"
    generated_makefile = makefile_template.format('')
    if arch == "x86":
        configure = "VC-WIN32"
        do_script = "ms\\do_nasm"
        suffix = "32"
    elif arch == "amd64":
        configure = "VC-WIN64A"
        do_script = "ms\\do_win64a"
        suffix = "64"
    else:
        raise ValueError('Unrecognized platform: %s' % arch)

    print("Creating the makefiles...")
    sys.stdout.flush()
    # run configure, copy includes, patch files
    run_configure(configure, do_script)
    makefile = makefile_template.format(suffix)
    try:
        os.unlink(makefile)
    except FileNotFoundError:
        pass
    os.rename(generated_makefile, makefile)
    copy_includes(makefile, suffix)

    print('patching ms\\uplink.c...')
    fix_uplink()

def main():
    if len(sys.argv) == 1:
        print("Not enough arguments: directory containing OpenSSL",
              "sources must be supplied")
        sys.exit(1)

    if len(sys.argv) == 3 and sys.argv[2] not in ('x86', 'amd64'):
        print("Second argument must be x86 or amd64")
        sys.exit(1)

    if len(sys.argv) > 3:
        print("Too many arguments supplied, all we need is the directory",
              "containing OpenSSL sources and optionally the architecture")
        sys.exit(1)

    ssl_dir = sys.argv[1]
    arch = sys.argv[2] if len(sys.argv) >= 3 else None

    if not os.path.isdir(ssl_dir):
        print(ssl_dir, "is not an existing directory!")
        sys.exit(1)

    # perl should be on the path, but we also look in "\perl" and "c:\\perl"
    # as "well known" locations
    perls = find_all_on_path("perl.exe", [r"\perl\bin",
                                          r"C:\perl\bin",
                                          r"\perl64\bin",
                                          r"C:\perl64\bin",
                                         ])
    perl = find_working_perl(perls)
    if perl:
        print("Found a working perl at '%s'" % (perl,))
    else:
        sys.exit(1)
    if not find_all_on_path('nmake.exe'):
        print('Could not find nmake.exe, try running env.bat')
        sys.exit(1)
    if not find_all_on_path('nasm.exe'):
        print('Could not find nasm.exe, please add to PATH')
        sys.exit(1)
    sys.stdout.flush()

    # Put our working Perl at the front of our path
    os.environ["PATH"] = os.path.dirname(perl) + \
                                os.pathsep + \
                                os.environ["PATH"]

    old_cwd = os.getcwd()
    try:
        os.chdir(ssl_dir)
        if arch:
            prep(arch)
        else:
            for arch in ['amd64', 'x86']:
                prep(arch)
    finally:
        os.chdir(old_cwd)

if __name__=='__main__':
    main()
