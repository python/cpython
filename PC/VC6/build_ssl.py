# Script for building the _ssl module for Windows.
# Uses Perl to setup the OpenSSL environment correctly
# and build OpenSSL, then invokes a simple nmake session
# for _ssl.pyd itself.

# THEORETICALLY, you can:
# * Unpack the latest SSL release one level above your main Python source
#   directory.  It is likely you will already find the zlib library and
#   any other external packages there.
# * Install ActivePerl and ensure it is somewhere on your path.
# * Run this script from the PCBuild directory.
#
# it should configure and build SSL, then build the ssl Python extension
# without intervention.

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
            s = os.path.abspath(s)
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
    return best_name

def main():
    debug = "-d" in sys.argv
    build_all = "-a" in sys.argv
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
    # Look for SSL 3 levels up from pcbuild - ie, same place zlib etc all live.
    ssl_dir = find_best_ssl_dir(("../../..",))
    if ssl_dir is None:
        sys.exit(1)

    old_cd = os.getcwd()
    try:
        os.chdir(ssl_dir)
        # If the ssl makefiles do not exist, we invoke Perl to generate them.
        if not os.path.isfile(os.path.join(ssl_dir, "32.mak")) or \
           not os.path.isfile(os.path.join(ssl_dir, "d32.mak")):
            print "Creating the makefiles..."
            # Put our working Perl at the front of our path
            os.environ["PATH"] = os.path.split(perl)[0] + \
                                          os.pathsep + \
                                          os.environ["PATH"]
            # ms\32all.bat will reconfigure OpenSSL and then try to build
            # all outputs (debug/nondebug/dll/lib).  So we filter the file
            # to exclude any "nmake" commands and then execute.
            tempname = "ms\\32all_py.bat"

            in_bat  = open("ms\\32all.bat")
            temp_bat = open(tempname,"w")
            while 1:
                cmd = in_bat.readline()
                print 'cmd', repr(cmd)
                if not cmd: break
                if cmd.strip()[:5].lower() == "nmake":
                    continue
                temp_bat.write(cmd)
            in_bat.close()
            temp_bat.close()
            os.system(tempname)
            try:
                os.remove(tempname)
            except:
                pass

        # Now run make.
        print "Executing nmake over the ssl makefiles..."
        if debug:
            rc = os.system("nmake /nologo -f d32.mak")
            if rc:
                print "Executing d32.mak failed"
                print rc
                sys.exit(rc)
        else:
            rc = os.system("nmake /nologo -f 32.mak")
            if rc:
                print "Executing 32.mak failed"
                print rc
                sys.exit(rc)
    finally:
        os.chdir(old_cd)
    # And finally, we can build the _ssl module itself for Python.
    defs = "SSL_DIR=%s" % (ssl_dir,)
    if debug:
        defs = defs + " " + "DEBUG=1"
    rc = os.system('nmake /nologo -f _ssl.mak ' + defs + " " + make_flags)
    sys.exit(rc)

if __name__=='__main__':
    main()
