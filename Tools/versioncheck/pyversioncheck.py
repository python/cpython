"""pyversioncheck - Module to help with checking versions"""
import types
import rfc822
import urllib
import sys
import string

# Verbose options
VERBOSE_SILENT=0	# Single-line reports per package
VERBOSE_NORMAL=1	# Single-line reports per package, more info if outdated
VERBOSE_EACHFILE=2	# Report on each URL checked
VERBOSE_CHECKALL=3	# Check each URL for each package

# Test directory
## urllib bug: _TESTDIR="ftp://ftp.cwi.nl/pub/jack/python/versiontestdir/"
_TESTDIR="http://www.cwi.nl/~jack/versiontestdir/"

def versioncheck(package, url, version, verbose=0):
    ok, newversion, fp = checkonly(package, url, version, verbose)
    if verbose > VERBOSE_NORMAL:
        return ok
    if ok < 0:
        print '%s: No correctly formatted current version file found'%(package)
    elif ok == 1:
        print '%s: up-to-date (version %s)'%(package, version)
    else:
        print '%s: version %s installed, version %s found:' % \
                        (package, version, newversion)
        if verbose > VERBOSE_SILENT:
            while 1:
                line = fp.readline()
                if not line: break
                sys.stdout.write('\t'+line)
    return ok

def checkonly(package, url, version, verbose=0):
    if verbose >= VERBOSE_EACHFILE:
        print '%s:'%package
    if type(url) == types.StringType:
        ok, newversion, fp = _check1version(package, url, version, verbose)
    else:
        for u in url:
            ok, newversion, fp = _check1version(package, u, version, verbose)
            if ok >= 0 and verbose < VERBOSE_CHECKALL:
                break
    return ok, newversion, fp

def _check1version(package, url, version, verbose=0):
    if verbose >= VERBOSE_EACHFILE:
        print '  Checking %s'%url
    try:
        fp = urllib.urlopen(url)
    except IOError, arg:
        if verbose >= VERBOSE_EACHFILE:
            print '    Cannot open:', arg
        return -1, None, None
    msg = rfc822.Message(fp, seekable=0)
    newversion = msg.getheader('current-version')
    if not newversion:
        if verbose >= VERBOSE_EACHFILE:
            print '    No "Current-Version:" header in URL or URL not found'
        return -1, None, None
    version = string.strip(string.lower(version))
    newversion = string.strip(string.lower(newversion))
    if version == newversion:
        if verbose >= VERBOSE_EACHFILE:
            print '    Version identical (%s)'%newversion
        return 1, version, fp
    else:
        if verbose >= VERBOSE_EACHFILE:
            print '    Versions different (installed: %s, new: %s)'% \
                        (version, newversion)
        return 0, newversion, fp


def _test():
    print '--- TEST VERBOSE=1'
    print '--- Testing existing and identical version file'
    versioncheck('VersionTestPackage', _TESTDIR+'Version10.txt', '1.0', verbose=1)
    print '--- Testing existing package with new version'
    versioncheck('VersionTestPackage', _TESTDIR+'Version11.txt', '1.0', verbose=1)
    print '--- Testing package with non-existing version file'
    versioncheck('VersionTestPackage', _TESTDIR+'nonexistent.txt', '1.0', verbose=1)
    print '--- Test package with 2 locations, first non-existing second ok'
    versfiles = [_TESTDIR+'nonexistent.txt', _TESTDIR+'Version10.txt']
    versioncheck('VersionTestPackage', versfiles, '1.0', verbose=1)
    print '--- TEST VERBOSE=2'
    print '--- Testing existing and identical version file'
    versioncheck('VersionTestPackage', _TESTDIR+'Version10.txt', '1.0', verbose=2)
    print '--- Testing existing package with new version'
    versioncheck('VersionTestPackage', _TESTDIR+'Version11.txt', '1.0', verbose=2)
    print '--- Testing package with non-existing version file'
    versioncheck('VersionTestPackage', _TESTDIR+'nonexistent.txt', '1.0', verbose=2)
    print '--- Test package with 2 locations, first non-existing second ok'
    versfiles = [_TESTDIR+'nonexistent.txt', _TESTDIR+'Version10.txt']
    versioncheck('VersionTestPackage', versfiles, '1.0', verbose=2)

if __name__ == '__main__':
    _test()
	
    
