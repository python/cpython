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

import os, sys, re, shutil

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
        fh = os.popen(perl + ' -e "use Win32;"')
        fh.read()
        rc = fh.close()
        if rc:
            continue
        return perl
    print("Can not find a suitable PERL:")
    if perls:
        print(" the following perl interpreters were found:")
        for p in perls:
            print(" ", p)
        print(" None of these versions appear suitable for building OpenSSL")
    else:
        print(" NO perl interpreters were found on this machine at all!")
    print(" Please install ActivePerl and ensure it appears on your path")
    return None

# Locate the best SSL directory given a few roots to look into.
def find_best_ssl_dir(sources):
    candidates = []
    for s in sources:
        try:
            # note: do not abspath s; the build will fail if any
            # higher up directory name has spaces in it.
            fnames = os.listdir(s)
        except os.error:
            fnames = []
        for fname in fnames:
            fqn = os.path.join(s, fname)
            if os.path.isdir(fqn) and fname.startswith("openssl-"):
                candidates.append(fqn)
    # Now we have all the candidates, locate the best.
    best_parts = []
    best_name = None
    for c in candidates:
        parts = re.split("[.-]", os.path.basename(c))[1:]
        # eg - openssl-0.9.7-beta1 - ignore all "beta" or any other qualifiers
        if len(parts) >= 4:
            continue
        if parts > best_parts:
            best_parts = parts
            best_name = c
    if best_name is not None:
        print("Found an SSL directory at '%s'" % (best_name,))
    else:
        print("Could not find an SSL directory in '%s'" % (sources,))
    sys.stdout.flush()
    return best_name

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

def fix_makefile(makefile):
    """Fix some stuff in all makefiles
    """
    if not os.path.isfile(makefile):
        return
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
            fout.write(line)

def run_configure(configure, do_script):
    print("perl Configure "+configure)
    os.system("perl Configure "+configure)
    print(do_script)
    os.system(do_script)

def main():
    build_all = "-a" in sys.argv
    if sys.argv[1] == "Release":
        debug = False
    elif sys.argv[1] == "Debug":
        debug = True
    else:
        raise ValueError(str(sys.argv))

    if sys.argv[2] == "Win32":
        arch = "x86"
        configure = "VC-WIN32"
        do_script = "ms\\do_nasm"
        makefile="ms\\nt.mak"
        m32 = makefile
    elif sys.argv[2] == "x64":
        arch="amd64"
        configure = "VC-WIN64A"
        do_script = "ms\\do_win64a"
        makefile = "ms\\nt64.mak"
        m32 = makefile.replace('64', '')
        #os.environ["VSEXTCOMP_USECL"] = "MS_OPTERON"
    else:
        raise ValueError(str(sys.argv))

    make_flags = ""
    if build_all:
        make_flags = "-a"
    # perl should be on the path, but we also look in "\perl" and "c:\\perl"
    # as "well known" locations
    perls = find_all_on_path("perl.exe", ["\\perl\\bin", "C:\\perl\\bin"])
    perl = find_working_perl(perls)
    if perl is None:
        print("No Perl installation was found. Existing Makefiles are used.")

    print("Found a working perl at '%s'" % (perl,))
    sys.stdout.flush()
    # Look for SSL 2 levels up from pcbuild - ie, same place zlib etc all live.
    ssl_dir = find_best_ssl_dir(("..\\..",))
    if ssl_dir is None:
        sys.exit(1)

    old_cd = os.getcwd()
    try:
        os.chdir(ssl_dir)
        # rebuild makefile when we do the role over from 32 to 64 build
        if arch == "amd64" and os.path.isfile(m32) and not os.path.isfile(makefile):
            os.unlink(m32)

        # If the ssl makefiles do not exist, we invoke Perl to generate them.
        # Due to a bug in this script, the makefile sometimes ended up empty
        # Force a regeneration if it is.
        if not os.path.isfile(makefile) or os.path.getsize(makefile)==0:
            if perl is None:
                print("Perl is required to build the makefiles!")
                sys.exit(1)

            print("Creating the makefiles...")
            sys.stdout.flush()
            # Put our working Perl at the front of our path
            os.environ["PATH"] = os.path.dirname(perl) + \
                                          os.pathsep + \
                                          os.environ["PATH"]
            run_configure(configure, do_script)
            if debug:
                print("OpenSSL debug builds aren't supported.")
            #if arch=="x86" and debug:
            #    # the do_masm script in openssl doesn't generate a debug
            #    # build makefile so we generate it here:
            #    os.system("perl util\mk1mf.pl debug "+configure+" >"+makefile)

            if arch == "amd64":
                create_makefile64(makefile, m32)
            fix_makefile(makefile)
            shutil.copy(r"crypto\buildinf.h", r"crypto\buildinf_%s.h" % arch)
            shutil.copy(r"crypto\opensslconf.h", r"crypto\opensslconf_%s.h" % arch)

        # Now run make.
        if arch == "amd64":
            rc = os.system(r"ml64 -c -Foms\uptable.obj ms\uptable.asm")
            if rc:
                print("ml64 assembler has failed.")
                sys.exit(rc)

        shutil.copy(r"crypto\buildinf_%s.h" % arch, r"crypto\buildinf.h")
        shutil.copy(r"crypto\opensslconf_%s.h" % arch, r"crypto\opensslconf.h")

        #makeCommand = "nmake /nologo PERL=\"%s\" -f \"%s\"" %(perl, makefile)
        makeCommand = "nmake /nologo -f \"%s\"" % makefile
        print("Executing ssl makefiles:", makeCommand)
        sys.stdout.flush()
        rc = os.system(makeCommand)
        if rc:
            print("Executing "+makefile+" failed")
            print(rc)
            sys.exit(rc)
    finally:
        os.chdir(old_cd)
    sys.exit(rc)

if __name__=='__main__':
    main()
