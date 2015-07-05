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

import os
import re
import sys
import shutil
import subprocess

# Find all "foo.exe" files on the PATH.
def find_all_on_path(filename, extras = None):
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

def create_makefile64(makefile, m32):
    """Create and fix makefile for 64bit

    Replace 32 with 64bit directories
    """
    if not os.path.isfile(m32):
        return
    with open(m32) as fin:
        with open(makefile, 'w') as fout:
            for line in fin:
                line = line.replace("=tmp32", "=tmp64")
                line = line.replace("=out32", "=out64")
                line = line.replace("=inc32", "=inc64")
                # force 64 bit machine
                line = line.replace("MKLIB=lib", "MKLIB=lib /MACHINE:X64")
                line = line.replace("LFLAGS=", "LFLAGS=/MACHINE:X64 ")
                # don't link against the lib on 64bit systems
                line = line.replace("bufferoverflowu.lib", "")
                fout.write(line)
    os.unlink(m32)

def create_asms(makefile):
    #create a custom makefile out of the provided one
    asm_makefile = os.path.splitext(makefile)[0] + '.asm.mak'
    with open(makefile) as fin:
        with open(asm_makefile, 'w') as fout:
            for line in fin:
                # Keep everything up to the install target (it's convenient)
                if line.startswith('install: all'):
                    break
                else:
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

    os.system('nmake /f {} PERL=perl asms'.format(asm_makefile))
    os.unlink(asm_makefile)



def fix_makefile(makefile):
    """Fix some stuff in all makefiles
    """
    if not os.path.isfile(makefile):
        return
    copy_if_different = r'$(PERL) $(SRC_D)\util\copy-if-different.pl'
    with open(makefile) as fin:
        lines = fin.readlines()
    with open(makefile, 'w') as fout:
        for line in lines:
            if line.startswith("PERL="):
                continue
            if line.startswith("CP="):
                line = "CP=copy\n"
            if line.startswith("MKDIR="):
                line = "MKDIR=mkdir\n"
            if line.startswith("CFLAG="):
                line = line.strip()
                for algo in ("RC5", "MDC2", "IDEA"):
                    noalgo = " -DOPENSSL_NO_%s" % algo
                    if noalgo not in line:
                        line = line + noalgo
                line = line + '\n'
            if copy_if_different in line:
                line = line.replace(copy_if_different, 'copy /Y')
            fout.write(line)

def run_configure(configure, do_script):
    print("perl Configure "+configure+" no-idea no-mdc2")
    os.system("perl Configure "+configure+" no-idea no-mdc2")
    print(do_script)
    os.system(do_script)

def cmp(f1, f2):
    bufsize = 1024 * 8
    with open(f1, 'rb') as fp1, open(f2, 'rb') as fp2:
        while True:
            b1 = fp1.read(bufsize)
            b2 = fp2.read(bufsize)
            if b1 != b2:
                return False
            if not b1:
                return True

def copy(src, dst):
    if os.path.isfile(dst) and cmp(src, dst):
        return
    shutil.copy(src, dst)

def prep(arch):
    if arch == "x86":
        configure = "VC-WIN32"
        do_script = "ms\\do_nasm"
        makefile="ms\\nt.mak"
        m32 = makefile
        dirsuffix = "32"
    elif arch == "amd64":
        configure = "VC-WIN64A"
        do_script = "ms\\do_win64a"
        makefile = "ms\\nt64.mak"
        m32 = makefile.replace('64', '')
        dirsuffix = "64"
        #os.environ["VSEXTCOMP_USECL"] = "MS_OPTERON"
    else:
        raise ValueError('Unrecognized platform: %s' % arch)

    # rebuild makefile when we do the role over from 32 to 64 build
    if arch == "amd64" and os.path.isfile(m32) and not os.path.isfile(makefile):
        os.unlink(m32)

    # If the ssl makefiles do not exist, we invoke Perl to generate them.
    # Due to a bug in this script, the makefile sometimes ended up empty
    # Force a regeneration if it is.
    if not os.path.isfile(makefile) or os.path.getsize(makefile)==0:
        print("Creating the makefiles...")
        sys.stdout.flush()
        run_configure(configure, do_script)

        if arch == "amd64":
            create_makefile64(makefile, m32)
        fix_makefile(makefile)
        copy(r"crypto\buildinf.h", r"crypto\buildinf_%s.h" % arch)
        copy(r"crypto\opensslconf.h", r"crypto\opensslconf_%s.h" % arch)
    else:
        print(makefile, 'already exists!')

    print('creating asms...')
    create_asms(makefile)

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
