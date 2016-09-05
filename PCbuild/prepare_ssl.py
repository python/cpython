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


def create_asms(makefile, tmp_d):
    #create a custom makefile out of the provided one
    asm_makefile = os.path.splitext(makefile)[0] + '.asm.mak'
    with open(makefile) as fin, open(asm_makefile, 'w') as fout:
        for line in fin:
            # Keep everything up to the install target (it's convenient)
            if line.startswith('install: all'):
                break
            fout.write(line)
        asms = []
        for line in fin:
            if '.asm' in line and line.strip().endswith('.pl'):
                asms.append(line.split(':')[0])
                while line.strip():
                    fout.write(line)
                    line = next(fin)
                fout.write('\n')

        fout.write('asms: $(TMP_D) ')
        fout.write(' '.join(asms))
        fout.write('\n')
    os.system('nmake /f {} PERL=perl TMP_D={} asms'.format(asm_makefile, tmp_d))


def copy_includes(makefile, suffix):
    dir = 'include'+suffix+'\\openssl'
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


def prep(arch):
    makefile_template = "ms\\nt{}.mak"
    generated_makefile = makefile_template.format('')
    if arch == "x86":
        configure = "VC-WIN32"
        do_script = "ms\\do_nasm"
        suffix = "32"
    elif arch == "amd64":
        configure = "VC-WIN64A"
        do_script = "ms\\do_win64a"
        suffix = "64"
        #os.environ["VSEXTCOMP_USECL"] = "MS_OPTERON"
    else:
        raise ValueError('Unrecognized platform: %s' % arch)

    print("Creating the makefiles...")
    sys.stdout.flush()
    # run configure, copy includes, create asms
    run_configure(configure, do_script)
    makefile = makefile_template.format(suffix)
    try:
        os.unlink(makefile)
    except FileNotFoundError:
        pass
    os.rename(generated_makefile, makefile)
    copy_includes(makefile, suffix)

    print('creating asms...')
    create_asms(makefile, 'tmp'+suffix)


def main():
    if len(sys.argv) == 1:
        print("Not enough arguments: directory containing OpenSSL",
              "sources must be supplied")
        sys.exit(1)

    if len(sys.argv) > 2:
        print("Too many arguments supplied, all we need is the directory",
              "containing OpenSSL sources")
        sys.exit(1)

    ssl_dir = sys.argv[1]

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
        for arch in ['amd64', 'x86']:
            prep(arch)
    finally:
        os.chdir(old_cwd)

if __name__=='__main__':
    main()
