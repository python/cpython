"""Package Install Manager for Python.

This is currently a MacOSX-only strawman implementation.
Despite other rumours the name stands for "Packman IMPlementation".

Tools to allow easy installation of packages. The idea is that there is
an online XML database per (platform, python-version) containing packages
known to work with that combination. This module contains tools for getting
and parsing the database, testing whether packages are installed, computing
dependencies and installing packages.

There is a minimal main program that works as a command line tool, but the
intention is that the end user will use this through a GUI.
"""
import sys
import os
import popen2
import urllib
import urllib2
import urlparse
import plistlib
import distutils.util
import distutils.sysconfig
import md5
import tarfile
import tempfile
import shutil
import time

__all__ = ["PimpPreferences", "PimpDatabase", "PimpPackage", "main",
    "getDefaultDatabase", "PIMP_VERSION", "main"]

_scriptExc_NotInstalled = "pimp._scriptExc_NotInstalled"
_scriptExc_OldInstalled = "pimp._scriptExc_OldInstalled"
_scriptExc_BadInstalled = "pimp._scriptExc_BadInstalled"

NO_EXECUTE=0

PIMP_VERSION="0.5"

# Flavors:
# source: setup-based package
# binary: tar (or other) archive created with setup.py bdist.
# installer: something that can be opened
DEFAULT_FLAVORORDER=['source', 'binary', 'installer']
DEFAULT_DOWNLOADDIR='/tmp'
DEFAULT_BUILDDIR='/tmp'
DEFAULT_INSTALLDIR=distutils.sysconfig.get_python_lib()
DEFAULT_PIMPDATABASE_FMT="http://www.python.org/packman/version-%s/%s-%s-%s-%s-%s.plist"

def getDefaultDatabase(experimental=False):
    if experimental:
        status = "exp"
    else:
        status = "prod"

    major, minor, micro, state, extra = sys.version_info
    pyvers = '%d.%d' % (major, minor)
    if micro == 0 and state != 'final':
        pyvers = pyvers + '%s%d' % (state, extra)

    longplatform = distutils.util.get_platform()
    osname, release, machine = longplatform.split('-')
    # For some platforms we may want to differentiate between
    # installation types
    if osname == 'darwin':
        if sys.prefix.startswith('/System/Library/Frameworks/Python.framework'):
            osname = 'darwin_apple'
        elif sys.prefix.startswith('/Library/Frameworks/Python.framework'):
            osname = 'darwin_macpython'
        # Otherwise we don't know...
    # Now we try various URLs by playing with the release string.
    # We remove numbers off the end until we find a match.
    rel = release
    while True:
        url = DEFAULT_PIMPDATABASE_FMT % (PIMP_VERSION, status, pyvers, osname, rel, machine)
        try:
            urllib2.urlopen(url)
        except urllib2.HTTPError, arg:
            pass
        else:
            break
        if not rel:
            # We're out of version numbers to try. Use the
            # full release number, this will give a reasonable
            # error message later
            url = DEFAULT_PIMPDATABASE_FMT % (PIMP_VERSION, status, pyvers, osname, release, machine)
            break
        idx = rel.rfind('.')
        if idx < 0:
            rel = ''
        else:
            rel = rel[:idx]
    return url

def _cmd(output, dir, *cmditems):
    """Internal routine to run a shell command in a given directory."""

    cmd = ("cd \"%s\"; " % dir) + " ".join(cmditems)
    if output:
        output.write("+ %s\n" % cmd)
    if NO_EXECUTE:
        return 0
    child = popen2.Popen4(cmd)
    child.tochild.close()
    while 1:
        line = child.fromchild.readline()
        if not line:
            break
        if output:
            output.write(line)
    return child.wait()

class PimpDownloader:
    """Abstract base class - Downloader for archives"""

    def __init__(self, argument,
            dir="",
            watcher=None):
        self.argument = argument
        self._dir = dir
        self._watcher = watcher

    def download(self, url, filename, output=None):
        return None

    def update(self, str):
        if self._watcher:
            return self._watcher.update(str)
        return True

class PimpCurlDownloader(PimpDownloader):

    def download(self, url, filename, output=None):
        self.update("Downloading %s..." % url)
        exitstatus = _cmd(output, self._dir,
                    "curl",
                    "--output", filename,
                    url)
        self.update("Downloading %s: finished" % url)
        return (not exitstatus)

class PimpUrllibDownloader(PimpDownloader):

    def download(self, url, filename, output=None):
        output = open(filename, 'wb')
        self.update("Downloading %s: opening connection" % url)
        keepgoing = True
        download = urllib2.urlopen(url)
        if download.headers.has_key("content-length"):
            length = long(download.headers['content-length'])
        else:
            length = -1

        data = download.read(4096) #read 4K at a time
        dlsize = 0
        lasttime = 0
        while keepgoing:
            dlsize = dlsize + len(data)
            if len(data) == 0:
                #this is our exit condition
                break
            output.write(data)
            if int(time.time()) != lasttime:
                # Update at most once per second
                lasttime = int(time.time())
                if length == -1:
                    keepgoing = self.update("Downloading %s: %d bytes..." % (url, dlsize))
                else:
                    keepgoing = self.update("Downloading %s: %d%% (%d bytes)..." % (url, int(100.0*dlsize/length), dlsize))
            data = download.read(4096)
        if keepgoing:
            self.update("Downloading %s: finished" % url)
        return keepgoing

class PimpUnpacker:
    """Abstract base class - Unpacker for archives"""

    _can_rename = False

    def __init__(self, argument,
            dir="",
            renames=[],
            watcher=None):
        self.argument = argument
        if renames and not self._can_rename:
            raise RuntimeError, "This unpacker cannot rename files"
        self._dir = dir
        self._renames = renames
        self._watcher = watcher

    def unpack(self, archive, output=None, package=None):
        return None

    def update(self, str):
        if self._watcher:
            return self._watcher.update(str)
        return True

class PimpCommandUnpacker(PimpUnpacker):
    """Unpack archives by calling a Unix utility"""

    _can_rename = False

    def unpack(self, archive, output=None, package=None):
        cmd = self.argument % archive
        if _cmd(output, self._dir, cmd):
            return "unpack command failed"

class PimpTarUnpacker(PimpUnpacker):
    """Unpack tarfiles using the builtin tarfile module"""

    _can_rename = True

    def unpack(self, archive, output=None, package=None):
        tf = tarfile.open(archive, "r")
        members = tf.getmembers()
        skip = []
        if self._renames:
            for member in members:
                for oldprefix, newprefix in self._renames:
                    if oldprefix[:len(self._dir)] == self._dir:
                        oldprefix2 = oldprefix[len(self._dir):]
                    else:
                        oldprefix2 = None
                    if member.name[:len(oldprefix)] == oldprefix:
                        if newprefix is None:
                            skip.append(member)
                            #print 'SKIP', member.name
                        else:
                            member.name = newprefix + member.name[len(oldprefix):]
                            print '    ', member.name
                        break
                    elif oldprefix2 and member.name[:len(oldprefix2)] == oldprefix2:
                        if newprefix is None:
                            skip.append(member)
                            #print 'SKIP', member.name
                        else:
                            member.name = newprefix + member.name[len(oldprefix2):]
                            #print '    ', member.name
                        break
                else:
                    skip.append(member)
                    #print '????', member.name
        for member in members:
            if member in skip:
                self.update("Skipping %s" % member.name)
                continue
            self.update("Extracting %s" % member.name)
            tf.extract(member, self._dir)
        if skip:
            names = [member.name for member in skip if member.name[-1] != '/']
            if package:
                names = package.filterExpectedSkips(names)
            if names:
                return "Not all files were unpacked: %s" % " ".join(names)

ARCHIVE_FORMATS = [
    (".tar.Z", PimpTarUnpacker, None),
    (".taz", PimpTarUnpacker, None),
    (".tar.gz", PimpTarUnpacker, None),
    (".tgz", PimpTarUnpacker, None),
    (".tar.bz", PimpTarUnpacker, None),
    (".zip", PimpCommandUnpacker, "unzip \"%s\""),
]

class PimpPreferences:
    """Container for per-user preferences, such as the database to use
    and where to install packages."""

    def __init__(self,
            flavorOrder=None,
            downloadDir=None,
            buildDir=None,
            installDir=None,
            pimpDatabase=None):
        if not flavorOrder:
            flavorOrder = DEFAULT_FLAVORORDER
        if not downloadDir:
            downloadDir = DEFAULT_DOWNLOADDIR
        if not buildDir:
            buildDir = DEFAULT_BUILDDIR
        if not pimpDatabase:
            pimpDatabase = getDefaultDatabase()
        self.setInstallDir(installDir)
        self.flavorOrder = flavorOrder
        self.downloadDir = downloadDir
        self.buildDir = buildDir
        self.pimpDatabase = pimpDatabase
        self.watcher = None

    def setWatcher(self, watcher):
        self.watcher = watcher

    def setInstallDir(self, installDir=None):
        if installDir:
            # Installing to non-standard location.
            self.installLocations = [
                ('--install-lib', installDir),
                ('--install-headers', None),
                ('--install-scripts', None),
                ('--install-data', None)]
        else:
            installDir = DEFAULT_INSTALLDIR
            self.installLocations = []
        self.installDir = installDir

    def isUserInstall(self):
        return self.installDir != DEFAULT_INSTALLDIR

    def check(self):
        """Check that the preferences make sense: directories exist and are
        writable, the install directory is on sys.path, etc."""

        rv = ""
        RWX_OK = os.R_OK|os.W_OK|os.X_OK
        if not os.path.exists(self.downloadDir):
            rv += "Warning: Download directory \"%s\" does not exist\n" % self.downloadDir
        elif not os.access(self.downloadDir, RWX_OK):
            rv += "Warning: Download directory \"%s\" is not writable or not readable\n" % self.downloadDir
        if not os.path.exists(self.buildDir):
            rv += "Warning: Build directory \"%s\" does not exist\n" % self.buildDir
        elif not os.access(self.buildDir, RWX_OK):
            rv += "Warning: Build directory \"%s\" is not writable or not readable\n" % self.buildDir
        if not os.path.exists(self.installDir):
            rv += "Warning: Install directory \"%s\" does not exist\n" % self.installDir
        elif not os.access(self.installDir, RWX_OK):
            rv += "Warning: Install directory \"%s\" is not writable or not readable\n" % self.installDir
        else:
            installDir = os.path.realpath(self.installDir)
            for p in sys.path:
                try:
                    realpath = os.path.realpath(p)
                except:
                    pass
                if installDir == realpath:
                    break
            else:
                rv += "Warning: Install directory \"%s\" is not on sys.path\n" % self.installDir
        return rv

    def compareFlavors(self, left, right):
        """Compare two flavor strings. This is part of your preferences
        because whether the user prefers installing from source or binary is."""
        if left in self.flavorOrder:
            if right in self.flavorOrder:
                return cmp(self.flavorOrder.index(left), self.flavorOrder.index(right))
            return -1
        if right in self.flavorOrder:
            return 1
        return cmp(left, right)

class PimpDatabase:
    """Class representing a pimp database. It can actually contain
    information from multiple databases through inclusion, but the
    toplevel database is considered the master, as its maintainer is
    "responsible" for the contents."""

    def __init__(self, prefs):
        self._packages = []
        self.preferences = prefs
        self._url = ""
        self._urllist = []
        self._version = ""
        self._maintainer = ""
        self._description = ""

    # Accessor functions
    def url(self): return self._url
    def version(self): return self._version
    def maintainer(self): return self._maintainer
    def description(self): return self._description

    def close(self):
        """Clean up"""
        self._packages = []
        self.preferences = None

    def appendURL(self, url, included=0):
        """Append packages from the database with the given URL.
        Only the first database should specify included=0, so the
        global information (maintainer, description) get stored."""

        if url in self._urllist:
            return
        self._urllist.append(url)
        fp = urllib2.urlopen(url).fp
        plistdata = plistlib.Plist.fromFile(fp)
        # Test here for Pimp version, etc
        if included:
            version = plistdata.get('Version')
            if version and version > self._version:
                sys.stderr.write("Warning: included database %s is for pimp version %s\n" %
                    (url, version))
        else:
            self._version = plistdata.get('Version')
            if not self._version:
                sys.stderr.write("Warning: database has no Version information\n")
            elif self._version > PIMP_VERSION:
                sys.stderr.write("Warning: database version %s newer than pimp version %s\n"
                    % (self._version, PIMP_VERSION))
            self._maintainer = plistdata.get('Maintainer', '')
            self._description = plistdata.get('Description', '').strip()
            self._url = url
        self._appendPackages(plistdata['Packages'], url)
        others = plistdata.get('Include', [])
        for o in others:
            o = urllib.basejoin(url, o)
            self.appendURL(o, included=1)

    def _appendPackages(self, packages, url):
        """Given a list of dictionaries containing package
        descriptions create the PimpPackage objects and append them
        to our internal storage."""

        for p in packages:
            p = dict(p)
            if p.has_key('Download-URL'):
                p['Download-URL'] = urllib.basejoin(url, p['Download-URL'])
            flavor = p.get('Flavor')
            if flavor == 'source':
                pkg = PimpPackage_source(self, p)
            elif flavor == 'binary':
                pkg = PimpPackage_binary(self, p)
            elif flavor == 'installer':
                pkg = PimpPackage_installer(self, p)
            elif flavor == 'hidden':
                pkg = PimpPackage_installer(self, p)
            else:
                pkg = PimpPackage(self, dict(p))
            self._packages.append(pkg)

    def list(self):
        """Return a list of all PimpPackage objects in the database."""

        return self._packages

    def listnames(self):
        """Return a list of names of all packages in the database."""

        rv = []
        for pkg in self._packages:
            rv.append(pkg.fullname())
        rv.sort()
        return rv

    def dump(self, pathOrFile):
        """Dump the contents of the database to an XML .plist file.

        The file can be passed as either a file object or a pathname.
        All data, including included databases, is dumped."""

        packages = []
        for pkg in self._packages:
            packages.append(pkg.dump())
        plistdata = {
            'Version': self._version,
            'Maintainer': self._maintainer,
            'Description': self._description,
            'Packages': packages
            }
        plist = plistlib.Plist(**plistdata)
        plist.write(pathOrFile)

    def find(self, ident):
        """Find a package. The package can be specified by name
        or as a dictionary with name, version and flavor entries.

        Only name is obligatory. If there are multiple matches the
        best one (higher version number, flavors ordered according to
        users' preference) is returned."""

        if type(ident) == str:
            # Remove ( and ) for pseudo-packages
            if ident[0] == '(' and ident[-1] == ')':
                ident = ident[1:-1]
            # Split into name-version-flavor
            fields = ident.split('-')
            if len(fields) < 1 or len(fields) > 3:
                return None
            name = fields[0]
            if len(fields) > 1:
                version = fields[1]
            else:
                version = None
            if len(fields) > 2:
                flavor = fields[2]
            else:
                flavor = None
        else:
            name = ident['Name']
            version = ident.get('Version')
            flavor = ident.get('Flavor')
        found = None
        for p in self._packages:
            if name == p.name() and \
                    (not version or version == p.version()) and \
                    (not flavor or flavor == p.flavor()):
                if not found or found < p:
                    found = p
        return found

ALLOWED_KEYS = [
    "Name",
    "Version",
    "Flavor",
    "Description",
    "Home-page",
    "Download-URL",
    "Install-test",
    "Install-command",
    "Pre-install-command",
    "Post-install-command",
    "Prerequisites",
    "MD5Sum",
    "User-install-skips",
    "Systemwide-only",
]

class PimpPackage:
    """Class representing a single package."""

    def __init__(self, db, plistdata):
        self._db = db
        name = plistdata["Name"]
        for k in plistdata.keys():
            if not k in ALLOWED_KEYS:
                sys.stderr.write("Warning: %s: unknown key %s\n" % (name, k))
        self._dict = plistdata

    def __getitem__(self, key):
        return self._dict[key]

    def name(self): return self._dict['Name']
    def version(self): return self._dict.get('Version')
    def flavor(self): return self._dict.get('Flavor')
    def description(self): return self._dict['Description'].strip()
    def shortdescription(self): return self.description().splitlines()[0]
    def homepage(self): return self._dict.get('Home-page')
    def downloadURL(self): return self._dict.get('Download-URL')
    def systemwideOnly(self): return self._dict.get('Systemwide-only')

    def fullname(self):
        """Return the full name "name-version-flavor" of a package.

        If the package is a pseudo-package, something that cannot be
        installed through pimp, return the name in (parentheses)."""

        rv = self._dict['Name']
        if self._dict.has_key('Version'):
            rv = rv + '-%s' % self._dict['Version']
        if self._dict.has_key('Flavor'):
            rv = rv + '-%s' % self._dict['Flavor']
        if self._dict.get('Flavor') == 'hidden':
            # Pseudo-package, show in parentheses
            rv = '(%s)' % rv
        return rv

    def dump(self):
        """Return a dict object containing the information on the package."""
        return self._dict

    def __cmp__(self, other):
        """Compare two packages, where the "better" package sorts lower."""

        if not isinstance(other, PimpPackage):
            return cmp(id(self), id(other))
        if self.name() != other.name():
            return cmp(self.name(), other.name())
        if self.version() != other.version():
            return -cmp(self.version(), other.version())
        return self._db.preferences.compareFlavors(self.flavor(), other.flavor())

    def installed(self):
        """Test wheter the package is installed.

        Returns two values: a status indicator which is one of
        "yes", "no", "old" (an older version is installed) or "bad"
        (something went wrong during the install test) and a human
        readable string which may contain more details."""

        namespace = {
            "NotInstalled": _scriptExc_NotInstalled,
            "OldInstalled": _scriptExc_OldInstalled,
            "BadInstalled": _scriptExc_BadInstalled,
            "os": os,
            "sys": sys,
            }
        installTest = self._dict['Install-test'].strip() + '\n'
        try:
            exec installTest in namespace
        except ImportError, arg:
            return "no", str(arg)
        except _scriptExc_NotInstalled, arg:
            return "no", str(arg)
        except _scriptExc_OldInstalled, arg:
            return "old", str(arg)
        except _scriptExc_BadInstalled, arg:
            return "bad", str(arg)
        except:
            sys.stderr.write("-------------------------------------\n")
            sys.stderr.write("---- %s: install test got exception\n" % self.fullname())
            sys.stderr.write("---- source:\n")
            sys.stderr.write(installTest)
            sys.stderr.write("---- exception:\n")
            import traceback
            traceback.print_exc(file=sys.stderr)
            if self._db._maintainer:
                sys.stderr.write("---- Please copy this and mail to %s\n" % self._db._maintainer)
            sys.stderr.write("-------------------------------------\n")
            return "bad", "Package install test got exception"
        return "yes", ""

    def prerequisites(self):
        """Return a list of prerequisites for this package.

        The list contains 2-tuples, of which the first item is either
        a PimpPackage object or None, and the second is a descriptive
        string. The first item can be None if this package depends on
        something that isn't pimp-installable, in which case the descriptive
        string should tell the user what to do."""

        rv = []
        if not self._dict.get('Download-URL'):
            # For pseudo-packages that are already installed we don't
            # return an error message
            status, _  = self.installed()
            if status == "yes":
                return []
            return [(None,
                "Package %s cannot be installed automatically, see the description" %
                    self.fullname())]
        if self.systemwideOnly() and self._db.preferences.isUserInstall():
            return [(None,
                "Package %s can only be installed system-wide" %
                    self.fullname())]
        if not self._dict.get('Prerequisites'):
            return []
        for item in self._dict['Prerequisites']:
            if type(item) == str:
                pkg = None
                descr = str(item)
            else:
                name = item['Name']
                if item.has_key('Version'):
                    name = name + '-' + item['Version']
                if item.has_key('Flavor'):
                    name = name + '-' + item['Flavor']
                pkg = self._db.find(name)
                if not pkg:
                    descr = "Requires unknown %s"%name
                else:
                    descr = pkg.shortdescription()
            rv.append((pkg, descr))
        return rv


    def downloadPackageOnly(self, output=None):
        """Download a single package, if needed.

        An MD5 signature is used to determine whether download is needed,
        and to test that we actually downloaded what we expected.
        If output is given it is a file-like object that will receive a log
        of what happens.

        If anything unforeseen happened the method returns an error message
        string.
        """

        scheme, loc, path, query, frag = urlparse.urlsplit(self._dict['Download-URL'])
        path = urllib.url2pathname(path)
        filename = os.path.split(path)[1]
        self.archiveFilename = os.path.join(self._db.preferences.downloadDir, filename)
        if not self._archiveOK():
            if scheme == 'manual':
                return "Please download package manually and save as %s" % self.archiveFilename
            downloader = PimpUrllibDownloader(None, self._db.preferences.downloadDir,
                watcher=self._db.preferences.watcher)
            if not downloader.download(self._dict['Download-URL'],
                    self.archiveFilename, output):
                return "download command failed"
        if not os.path.exists(self.archiveFilename) and not NO_EXECUTE:
            return "archive not found after download"
        if not self._archiveOK():
            return "archive does not have correct MD5 checksum"

    def _archiveOK(self):
        """Test an archive. It should exist and the MD5 checksum should be correct."""

        if not os.path.exists(self.archiveFilename):
            return 0
        if not self._dict.get('MD5Sum'):
            sys.stderr.write("Warning: no MD5Sum for %s\n" % self.fullname())
            return 1
        data = open(self.archiveFilename, 'rb').read()
        checksum = md5.new(data).hexdigest()
        return checksum == self._dict['MD5Sum']

    def unpackPackageOnly(self, output=None):
        """Unpack a downloaded package archive."""

        filename = os.path.split(self.archiveFilename)[1]
        for ext, unpackerClass, arg in ARCHIVE_FORMATS:
            if filename[-len(ext):] == ext:
                break
        else:
            return "unknown extension for archive file: %s" % filename
        self.basename = filename[:-len(ext)]
        unpacker = unpackerClass(arg, dir=self._db.preferences.buildDir,
                watcher=self._db.preferences.watcher)
        rv = unpacker.unpack(self.archiveFilename, output=output)
        if rv:
            return rv

    def installPackageOnly(self, output=None):
        """Default install method, to be overridden by subclasses"""
        return "%s: This package needs to be installed manually (no support for flavor=\"%s\")" \
            % (self.fullname(), self._dict.get(flavor, ""))

    def installSinglePackage(self, output=None):
        """Download, unpack and install a single package.

        If output is given it should be a file-like object and it
        will receive a log of what happened."""

        if not self._dict.get('Download-URL'):
            return "%s: This package needs to be installed manually (no Download-URL field)" % self.fullname()
        msg = self.downloadPackageOnly(output)
        if msg:
            return "%s: download: %s" % (self.fullname(), msg)

        msg = self.unpackPackageOnly(output)
        if msg:
            return "%s: unpack: %s" % (self.fullname(), msg)

        return self.installPackageOnly(output)

    def beforeInstall(self):
        """Bookkeeping before installation: remember what we have in site-packages"""
        self._old_contents = os.listdir(self._db.preferences.installDir)

    def afterInstall(self):
        """Bookkeeping after installation: interpret any new .pth files that have
        appeared"""

        new_contents = os.listdir(self._db.preferences.installDir)
        for fn in new_contents:
            if fn in self._old_contents:
                continue
            if fn[-4:] != '.pth':
                continue
            fullname = os.path.join(self._db.preferences.installDir, fn)
            f = open(fullname)
            for line in f.readlines():
                if not line:
                    continue
                if line[0] == '#':
                    continue
                if line[:6] == 'import':
                    exec line
                    continue
                if line[-1] == '\n':
                    line = line[:-1]
                if not os.path.isabs(line):
                    line = os.path.join(self._db.preferences.installDir, line)
                line = os.path.realpath(line)
                if not line in sys.path:
                    sys.path.append(line)

    def filterExpectedSkips(self, names):
        """Return a list that contains only unpexpected skips"""
        if not self._db.preferences.isUserInstall():
            return names
        expected_skips = self._dict.get('User-install-skips')
        if not expected_skips:
            return names
        newnames = []
        for name in names:
            for skip in expected_skips:
                if name[:len(skip)] == skip:
                    break
            else:
                newnames.append(name)
        return newnames

class PimpPackage_binary(PimpPackage):

    def unpackPackageOnly(self, output=None):
        """We don't unpack binary packages until installing"""
        pass

    def installPackageOnly(self, output=None):
        """Install a single source package.

        If output is given it should be a file-like object and it
        will receive a log of what happened."""

        if self._dict.has_key('Install-command'):
            return "%s: Binary package cannot have Install-command" % self.fullname()

        if self._dict.has_key('Pre-install-command'):
            if _cmd(output, '/tmp', self._dict['Pre-install-command']):
                return "pre-install %s: running \"%s\" failed" % \
                    (self.fullname(), self._dict['Pre-install-command'])

        self.beforeInstall()

        # Install by unpacking
        filename = os.path.split(self.archiveFilename)[1]
        for ext, unpackerClass, arg in ARCHIVE_FORMATS:
            if filename[-len(ext):] == ext:
                break
        else:
            return "%s: unknown extension for archive file: %s" % (self.fullname(), filename)
        self.basename = filename[:-len(ext)]

        install_renames = []
        for k, newloc in self._db.preferences.installLocations:
            if not newloc:
                continue
            if k == "--install-lib":
                oldloc = DEFAULT_INSTALLDIR
            else:
                return "%s: Don't know installLocation %s" % (self.fullname(), k)
            install_renames.append((oldloc, newloc))

        unpacker = unpackerClass(arg, dir="/", renames=install_renames)
        rv = unpacker.unpack(self.archiveFilename, output=output, package=self)
        if rv:
            return rv

        self.afterInstall()

        if self._dict.has_key('Post-install-command'):
            if _cmd(output, '/tmp', self._dict['Post-install-command']):
                return "%s: post-install: running \"%s\" failed" % \
                    (self.fullname(), self._dict['Post-install-command'])

        return None


class PimpPackage_source(PimpPackage):

    def unpackPackageOnly(self, output=None):
        """Unpack a source package and check that setup.py exists"""
        PimpPackage.unpackPackageOnly(self, output)
        # Test that a setup script has been create
        self._buildDirname = os.path.join(self._db.preferences.buildDir, self.basename)
        setupname = os.path.join(self._buildDirname, "setup.py")
        if not os.path.exists(setupname) and not NO_EXECUTE:
            return "no setup.py found after unpack of archive"

    def installPackageOnly(self, output=None):
        """Install a single source package.

        If output is given it should be a file-like object and it
        will receive a log of what happened."""

        if self._dict.has_key('Pre-install-command'):
            if _cmd(output, self._buildDirname, self._dict['Pre-install-command']):
                return "pre-install %s: running \"%s\" failed" % \
                    (self.fullname(), self._dict['Pre-install-command'])

        self.beforeInstall()
        installcmd = self._dict.get('Install-command')
        if installcmd and self._install_renames:
            return "Package has install-command and can only be installed to standard location"
        # This is the "bit-bucket" for installations: everything we don't
        # want. After installation we check that it is actually empty
        unwanted_install_dir = None
        if not installcmd:
            extra_args = ""
            for k, v in self._db.preferences.installLocations:
                if not v:
                    # We don't want these files installed. Send them
                    # to the bit-bucket.
                    if not unwanted_install_dir:
                        unwanted_install_dir = tempfile.mkdtemp()
                    v = unwanted_install_dir
                extra_args = extra_args + " %s \"%s\"" % (k, v)
            installcmd = '"%s" setup.py install %s' % (sys.executable, extra_args)
        if _cmd(output, self._buildDirname, installcmd):
            return "install %s: running \"%s\" failed" % \
                (self.fullname(), installcmd)
        if unwanted_install_dir and os.path.exists(unwanted_install_dir):
            unwanted_files = os.listdir(unwanted_install_dir)
            if unwanted_files:
                rv = "Warning: some files were not installed: %s" % " ".join(unwanted_files)
            else:
                rv = None
            shutil.rmtree(unwanted_install_dir)
            return rv

        self.afterInstall()

        if self._dict.has_key('Post-install-command'):
            if _cmd(output, self._buildDirname, self._dict['Post-install-command']):
                return "post-install %s: running \"%s\" failed" % \
                    (self.fullname(), self._dict['Post-install-command'])
        return None

class PimpPackage_installer(PimpPackage):

    def unpackPackageOnly(self, output=None):
        """We don't unpack dmg packages until installing"""
        pass

    def installPackageOnly(self, output=None):
        """Install a single source package.
        
        If output is given it should be a file-like object and it
        will receive a log of what happened."""
                    
        if self._dict.has_key('Post-install-command'):
            return "%s: Installer package cannot have Post-install-command" % self.fullname()

        if self._dict.has_key('Pre-install-command'):
            if _cmd(output, '/tmp', self._dict['Pre-install-command']):
                return "pre-install %s: running \"%s\" failed" % \
                    (self.fullname(), self._dict['Pre-install-command'])
                    
        self.beforeInstall()

        installcmd = self._dict.get('Install-command')
        if installcmd:
            if '%' in installcmd:
                installcmd = installcmd % self.archiveFilename
        else:
                installcmd = 'open \"%s\"' % self.archiveFilename
        if _cmd(output, "/tmp", installcmd):
            return '%s: install command failed (use verbose for details)' % self.fullname()
        return '%s: downloaded and opened. Install manually and restart Package Manager' % self.archiveFilename

class PimpInstaller:
    """Installer engine: computes dependencies and installs
    packages in the right order."""

    def __init__(self, db):
        self._todo = []
        self._db = db
        self._curtodo = []
        self._curmessages = []

    def __contains__(self, package):
        return package in self._todo

    def _addPackages(self, packages):
        for package in packages:
            if not package in self._todo:
                self._todo.append(package)

    def _prepareInstall(self, package, force=0, recursive=1):
        """Internal routine, recursive engine for prepareInstall.

        Test whether the package is installed and (if not installed
        or if force==1) prepend it to the temporary todo list and
        call ourselves recursively on all prerequisites."""

        if not force:
            status, message = package.installed()
            if status == "yes":
                return
        if package in self._todo or package in self._curtodo:
            return
        self._curtodo.insert(0, package)
        if not recursive:
            return
        prereqs = package.prerequisites()
        for pkg, descr in prereqs:
            if pkg:
                self._prepareInstall(pkg, False, recursive)
            else:
                self._curmessages.append("Problem with dependency: %s" % descr)

    def prepareInstall(self, package, force=0, recursive=1):
        """Prepare installation of a package.

        If the package is already installed and force is false nothing
        is done. If recursive is true prerequisites are installed first.

        Returns a list of packages (to be passed to install) and a list
        of messages of any problems encountered.
        """

        self._curtodo = []
        self._curmessages = []
        self._prepareInstall(package, force, recursive)
        rv = self._curtodo, self._curmessages
        self._curtodo = []
        self._curmessages = []
        return rv

    def install(self, packages, output):
        """Install a list of packages."""

        self._addPackages(packages)
        status = []
        for pkg in self._todo:
            msg = pkg.installSinglePackage(output)
            if msg:
                status.append(msg)
        return status



def _run(mode, verbose, force, args, prefargs, watcher):
    """Engine for the main program"""

    prefs = PimpPreferences(**prefargs)
    if watcher:
        prefs.setWatcher(watcher)
    rv = prefs.check()
    if rv:
        sys.stdout.write(rv)
    db = PimpDatabase(prefs)
    db.appendURL(prefs.pimpDatabase)

    if mode == 'dump':
        db.dump(sys.stdout)
    elif mode =='list':
        if not args:
            args = db.listnames()
        print "%-20.20s\t%s" % ("Package", "Description")
        print
        for pkgname in args:
            pkg = db.find(pkgname)
            if pkg:
                description = pkg.shortdescription()
                pkgname = pkg.fullname()
            else:
                description = 'Error: no such package'
            print "%-20.20s\t%s" % (pkgname, description)
            if verbose:
                print "\tHome page:\t", pkg.homepage()
                try:
                    print "\tDownload URL:\t", pkg.downloadURL()
                except KeyError:
                    pass
                description = pkg.description()
                description = '\n\t\t\t\t\t'.join(description.splitlines())
                print "\tDescription:\t%s" % description
    elif mode =='status':
        if not args:
            args = db.listnames()
            print "%-20.20s\t%s\t%s" % ("Package", "Installed", "Message")
            print
        for pkgname in args:
            pkg = db.find(pkgname)
            if pkg:
                status, msg = pkg.installed()
                pkgname = pkg.fullname()
            else:
                status = 'error'
                msg = 'No such package'
            print "%-20.20s\t%-9.9s\t%s" % (pkgname, status, msg)
            if verbose and status == "no":
                prereq = pkg.prerequisites()
                for pkg, msg in prereq:
                    if not pkg:
                        pkg = ''
                    else:
                        pkg = pkg.fullname()
                    print "%-20.20s\tRequirement: %s %s" % ("", pkg, msg)
    elif mode == 'install':
        if not args:
            print 'Please specify packages to install'
            sys.exit(1)
        inst = PimpInstaller(db)
        for pkgname in args:
            pkg = db.find(pkgname)
            if not pkg:
                print '%s: No such package' % pkgname
                continue
            list, messages = inst.prepareInstall(pkg, force)
            if messages and not force:
                print "%s: Not installed:" % pkgname
                for m in messages:
                    print "\t", m
            else:
                if verbose:
                    output = sys.stdout
                else:
                    output = None
                messages = inst.install(list, output)
                if messages:
                    print "%s: Not installed:" % pkgname
                    for m in messages:
                        print "\t", m

def main():
    """Minimal commandline tool to drive pimp."""

    import getopt
    def _help():
        print "Usage: pimp [options] -s [package ...]  List installed status"
        print "       pimp [options] -l [package ...]  Show package information"
        print "       pimp [options] -i package ...    Install packages"
        print "       pimp -d                          Dump database to stdout"
        print "       pimp -V                          Print version number"
        print "Options:"
        print "       -v     Verbose"
        print "       -f     Force installation"
        print "       -D dir Set destination directory"
        print "              (default: %s)" % DEFAULT_INSTALLDIR
        print "       -u url URL for database"
        sys.exit(1)

    class _Watcher:
        def update(self, msg):
            sys.stderr.write(msg + '\r')
            return 1

    try:
        opts, args = getopt.getopt(sys.argv[1:], "slifvdD:Vu:")
    except getopt.GetoptError:
        _help()
    if not opts and not args:
        _help()
    mode = None
    force = 0
    verbose = 0
    prefargs = {}
    watcher = None
    for o, a in opts:
        if o == '-s':
            if mode:
                _help()
            mode = 'status'
        if o == '-l':
            if mode:
                _help()
            mode = 'list'
        if o == '-d':
            if mode:
                _help()
            mode = 'dump'
        if o == '-V':
            if mode:
                _help()
            mode = 'version'
        if o == '-i':
            mode = 'install'
        if o == '-f':
            force = 1
        if o == '-v':
            verbose = 1
            watcher = _Watcher()
        if o == '-D':
            prefargs['installDir'] = a
        if o == '-u':
            prefargs['pimpDatabase'] = a
    if not mode:
        _help()
    if mode == 'version':
        print 'Pimp version %s; module name is %s' % (PIMP_VERSION, __name__)
    else:
        _run(mode, verbose, force, args, prefargs, watcher)

# Finally, try to update ourselves to a newer version.
# If the end-user updates pimp through pimp the new version
# will be called pimp_update and live in site-packages
# or somewhere similar
if __name__ != 'pimp_update':
    try:
        import pimp_update
    except ImportError:
        pass
    else:
        if pimp_update.PIMP_VERSION <= PIMP_VERSION:
            import warnings
            warnings.warn("pimp_update is version %s, not newer than pimp version %s" %
                (pimp_update.PIMP_VERSION, PIMP_VERSION))
        else:
            from pimp_update import *

if __name__ == '__main__':
    main()
