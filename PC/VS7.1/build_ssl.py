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

import os, sys, re

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
    print "Can not find a suitable PERL:"
    if perls:
        print " the following perl interpreters were found:"
        for p in perls:
            print " ", p
        print " None of these versions appear suitable for building OpenSSL"
    else:
        print " NO perl interpreters were found on this machine at all!"
    print " Please install ActivePerl and ensure it appears on your path"
    print "The Python SSL module was not built"
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
        print "Found an SSL directory at '%s'" % (best_name,)
    else:
        print "Could not find an SSL directory in '%s'" % (sources,)
    sys.stdout.flush()
    return best_name

def run_configure(configure, do_script):
    os.system("perl Configure "+configure)
    os.system(do_script)

def main():
    build_all = "-a" in sys.argv
    if sys.argv[1] == "Release":
        arch = "x86"
        debug = False
        configure = "VC-WIN32"
        do_script = "ms\\do_masm"
        makefile = "ms\\nt.mak"
    elif sys.argv[1] == "Debug":
        arch = "x86"
        debug = True
        configure = "VC-WIN32"
        do_script = "ms\\do_masm"
        makefile="ms\\d32.mak"
    elif sys.argv[1] == "ReleaseItanium":
        arch = "ia64"
        debug = False
        configure = "VC-WIN64I"
        do_script = "ms\\do_win64i"
        makefile = "ms\\nt.mak"
        os.environ["VSEXTCOMP_USECL"] = "MS_ITANIUM"
    elif sys.argv[1] == "ReleaseAMD64":
        arch="amd64"
        debug=False
        configure = "VC-WIN64A"
        do_script = "ms\\do_win64a"
        makefile = "ms\\nt.mak"
        os.environ["VSEXTCOMP_USECL"] = "MS_OPTERON"
    make_flags = ""
    if build_all:
        make_flags = "-a"
    # perl should be on the path, but we also look in "\perl" and "c:\\perl"
    # as "well known" locations
    perls = find_all_on_path("perl.exe", ["\\perl\\bin", "C:\\perl\\bin"])
    perl = find_working_perl(perls)
    if perl is None:
        sys.exit(1)

    print "Found a working perl at '%s'" % (perl,)
    sys.stdout.flush()
    # Look for SSL 2 levels up from pcbuild - ie, same place zlib etc all live.
    ssl_dir = find_best_ssl_dir(("..\\..\\..",))
    if ssl_dir is None:
        sys.exit(1)

    old_cd = os.getcwd()
    try:
        os.chdir(ssl_dir)
        # If the ssl makefiles do not exist, we invoke Perl to generate them.
        # Due to a bug in this script, the makefile sometimes ended up empty
        # Force a regeneration if it is.
        if not os.path.isfile(makefile) or os.path.getsize(makefile)==0:
            print "Creating the makefiles..."
            sys.stdout.flush()
            # Put our working Perl at the front of our path
            os.environ["PATH"] = os.path.dirname(perl) + \
                                          os.pathsep + \
                                          os.environ["PATH"]
            run_configure(configure, do_script)
            if arch=="x86" and debug:
                # the do_masm script in openssl doesn't generate a debug
                # build makefile so we generate it here:
                os.system("perl util\mk1mf.pl debug "+configure+" >"+makefile)

        # Now run make.
        makeCommand = "nmake /nologo PERL=\"%s\" -f \"%s\"" %(perl, makefile)
        print "Executing ssl makefiles:", makeCommand
        sys.stdout.flush()
        rc = os.system(makeCommand)
        if rc:
            print "Executing "+makefile+" failed"
            print rc
            sys.exit(rc)
    finally:
        os.chdir(old_cd)
    # And finally, we can build the _ssl module itself for Python.
    defs = "SSL_DIR=\"%s\"" % (ssl_dir,)
    if debug:
        defs = defs + " " + "DEBUG=1"
    if arch in ('amd64', 'ia64'):
        defs = defs + " EXTRA_CFLAGS=/GS- EXTRA_LIBS=bufferoverflowU.lib"
    makeCommand = 'nmake /nologo -f _ssl.mak ' + defs + " " + make_flags
    print "Executing:", makeCommand
    sys.stdout.flush()
    rc = os.system(makeCommand)
    sys.exit(rc)

if __name__=='__main__':
    main()
