#!/usr/bin/env python

"""buildpkg.py -- Build OS X packages for Apple's Installer.app.

This is an experimental command-line tool for building packages to be
installed with the Mac OS X Installer.app application.

It is much inspired by Apple's GUI tool called PackageMaker.app, that
seems to be part of the OS X developer tools installed in the folder
/Developer/Applications. But apparently there are other free tools to
do the same thing which are also named PackageMaker like Brian Hill's
one:

  http://personalpages.tds.net/~brian_hill/packagemaker.html

Beware of the multi-package features of Installer.app (which are not
yet supported here) that can potentially screw-up your installation
and are discussed in these articles on Stepwise:

  http://www.stepwise.com/Articles/Technical/Packages/InstallerWoes.html
  http://www.stepwise.com/Articles/Technical/Packages/InstallerOnX.html

Beside using the PackageMaker class directly, by importing it inside
another module, say, there are additional ways of using this module:
the top-level buildPackage() function provides a shortcut to the same
feature and is also called when using this module from the command-
line.

    ****************************************************************
    NOTE: For now you should be able to run this even on a non-OS X
          system and get something similar to a package, but without
          the real archive (needs pax) and bom files (needs mkbom)
          inside! This is only for providing a chance for testing to
          folks without OS X.
    ****************************************************************

TODO:
  - test pre-process and post-process scripts (Python ones?)
  - handle multi-volume packages (?)
  - integrate into distutils (?)

Dinu C. Gherman,
gherman@europemail.com
November 2001

!! USE AT YOUR OWN RISK !!
"""

__version__ = 0.2
__license__ = "FreeBSD"


import os, sys, glob, fnmatch, shutil, string, copy, getopt
from os.path import basename, dirname, join, islink, isdir, isfile

Error = "buildpkg.Error"

PKG_INFO_FIELDS = """\
Title
Version
Description
DefaultLocation
DeleteWarning
NeedsAuthorization
DisableStop
UseUserMask
Application
Relocatable
Required
InstallOnly
RequiresReboot
RootVolumeOnly
LongFilenames
LibrarySubdirectory
AllowBackRev
OverwritePermissions
InstallFat\
"""

######################################################################
# Helpers
######################################################################

# Convenience class, as suggested by /F.

class GlobDirectoryWalker:
    "A forward iterator that traverses files in a directory tree."

    def __init__(self, directory, pattern="*"):
        self.stack = [directory]
        self.pattern = pattern
        self.files = []
        self.index = 0


    def __getitem__(self, index):
        while 1:
            try:
                file = self.files[self.index]
                self.index = self.index + 1
            except IndexError:
                # pop next directory from stack
                self.directory = self.stack.pop()
                self.files = os.listdir(self.directory)
                self.index = 0
            else:
                # got a filename
                fullname = join(self.directory, file)
                if isdir(fullname) and not islink(fullname):
                    self.stack.append(fullname)
                if fnmatch.fnmatch(file, self.pattern):
                    return fullname


######################################################################
# The real thing
######################################################################

class PackageMaker:
    """A class to generate packages for Mac OS X.

    This is intended to create OS X packages (with extension .pkg)
    containing archives of arbitrary files that the Installer.app
    will be able to handle.

    As of now, PackageMaker instances need to be created with the
    title, version and description of the package to be built.
    The package is built after calling the instance method
    build(root, **options). It has the same name as the constructor's
    title argument plus a '.pkg' extension and is located in the same
    parent folder that contains the root folder.

    E.g. this will create a package folder /my/space/distutils.pkg/:

      pm = PackageMaker("distutils", "1.0.2", "Python distutils.")
      pm.build("/my/space/distutils")
    """

    packageInfoDefaults = {
        'Title': None,
        'Version': None,
        'Description': '',
        'DefaultLocation': '/',
        'DeleteWarning': '',
        'NeedsAuthorization': 'NO',
        'DisableStop': 'NO',
        'UseUserMask': 'YES',
        'Application': 'NO',
        'Relocatable': 'YES',
        'Required': 'NO',
        'InstallOnly': 'NO',
        'RequiresReboot': 'NO',
        'RootVolumeOnly' : 'NO',
        'InstallFat': 'NO',
        'LongFilenames': 'YES',
        'LibrarySubdirectory': 'Standard',
        'AllowBackRev': 'YES',
        'OverwritePermissions': 'NO',
        }


    def __init__(self, title, version, desc):
        "Init. with mandatory title/version/description arguments."

        info = {"Title": title, "Version": version, "Description": desc}
        self.packageInfo = copy.deepcopy(self.packageInfoDefaults)
        self.packageInfo.update(info)

        # variables set later
        self.packageRootFolder = None
        self.packageResourceFolder = None
        self.sourceFolder = None
        self.resourceFolder = None


    def build(self, root, resources=None, **options):
        """Create a package for some given root folder.

        With no 'resources' argument set it is assumed to be the same
        as the root directory. Option items replace the default ones
        in the package info.
        """

        # set folder attributes
        self.sourceFolder = root
        if resources == None:
            self.resourceFolder = root
        else:
            self.resourceFolder = resources

        # replace default option settings with user ones if provided
        fields = self. packageInfoDefaults.keys()
        for k, v in options.items():
            if k in fields:
                self.packageInfo[k] = v
            elif not k in ["OutputDir"]:
                raise Error("Unknown package option: %s" % k)

        # Check where we should leave the output. Default is current directory
        outputdir = options.get("OutputDir", os.getcwd())
        packageName = self.packageInfo["Title"]
        self.PackageRootFolder = os.path.join(outputdir, packageName + ".pkg")

        # do what needs to be done
        self._makeFolders()
        self._addInfo()
        self._addBom()
        self._addArchive()
        self._addResources()
        self._addSizes()
        self._addLoc()


    def _makeFolders(self):
        "Create package folder structure."

        # Not sure if the package name should contain the version or not...
        # packageName = "%s-%s" % (self.packageInfo["Title"],
        #                          self.packageInfo["Version"]) # ??

        contFolder = join(self.PackageRootFolder, "Contents")
        self.packageResourceFolder = join(contFolder, "Resources")
        os.mkdir(self.PackageRootFolder)
        os.mkdir(contFolder)
        os.mkdir(self.packageResourceFolder)

    def _addInfo(self):
        "Write .info file containing installing options."

        # Not sure if options in PKG_INFO_FIELDS are complete...

        info = ""
        for f in string.split(PKG_INFO_FIELDS, "\n"):
            if self.packageInfo.has_key(f):
                info = info + "%s %%(%s)s\n" % (f, f)
        info = info % self.packageInfo
        base = self.packageInfo["Title"] + ".info"
        path = join(self.packageResourceFolder, base)
        f = open(path, "w")
        f.write(info)


    def _addBom(self):
        "Write .bom file containing 'Bill of Materials'."

        # Currently ignores if the 'mkbom' tool is not available.

        try:
            base = self.packageInfo["Title"] + ".bom"
            bomPath = join(self.packageResourceFolder, base)
            cmd = "mkbom %s %s" % (self.sourceFolder, bomPath)
            res = os.system(cmd)
        except:
            pass


    def _addArchive(self):
        "Write .pax.gz file, a compressed archive using pax/gzip."

        # Currently ignores if the 'pax' tool is not available.

        cwd = os.getcwd()

        # create archive
        os.chdir(self.sourceFolder)
        base = basename(self.packageInfo["Title"]) + ".pax"
        self.archPath = join(self.packageResourceFolder, base)
        cmd = "pax -w -f %s %s" % (self.archPath, ".")
        res = os.system(cmd)

        # compress archive
        cmd = "gzip %s" % self.archPath
        res = os.system(cmd)
        os.chdir(cwd)


    def _addResources(self):
        "Add Welcome/ReadMe/License files, .lproj folders and scripts."

        # Currently we just copy everything that matches the allowed
        # filenames. So, it's left to Installer.app to deal with the
        # same file available in multiple formats...

        if not self.resourceFolder:
            return

        # find candidate resource files (txt html rtf rtfd/ or lproj/)
        allFiles = []
        for pat in string.split("*.txt *.html *.rtf *.rtfd *.lproj", " "):
            pattern = join(self.resourceFolder, pat)
            allFiles = allFiles + glob.glob(pattern)

        # find pre-process and post-process scripts
        # naming convention: packageName.{pre,post}_{upgrade,install}
        # Alternatively the filenames can be {pre,post}_{upgrade,install}
        # in which case we prepend the package name
        packageName = self.packageInfo["Title"]
        for pat in ("*upgrade", "*install", "*flight"):
            pattern = join(self.resourceFolder, packageName + pat)
            pattern2 = join(self.resourceFolder, pat)
            allFiles = allFiles + glob.glob(pattern)
            allFiles = allFiles + glob.glob(pattern2)

        # check name patterns
        files = []
        for f in allFiles:
            for s in ("Welcome", "License", "ReadMe"):
                if string.find(basename(f), s) == 0:
                    files.append((f, f))
            if f[-6:] == ".lproj":
                files.append((f, f))
            elif basename(f) in ["pre_upgrade", "pre_install", "post_upgrade", "post_install"]:
                files.append((f, packageName+"."+basename(f)))
            elif basename(f) in ["preflight", "postflight"]:
                files.append((f, f))
            elif f[-8:] == "_upgrade":
                files.append((f,f))
            elif f[-8:] == "_install":
                files.append((f,f))

        # copy files
        for src, dst in files:
            src = basename(src)
            dst = basename(dst)
            f = join(self.resourceFolder, src)
            if isfile(f):
                shutil.copy(f, os.path.join(self.packageResourceFolder, dst))
            elif isdir(f):
                # special case for .rtfd and .lproj folders...
                d = join(self.packageResourceFolder, dst)
                os.mkdir(d)
                files = GlobDirectoryWalker(f)
                for file in files:
                    shutil.copy(file, d)


    def _addSizes(self):
        "Write .sizes file with info about number and size of files."

        # Not sure if this is correct, but 'installedSize' and
        # 'zippedSize' are now in Bytes. Maybe blocks are needed?
        # Well, Installer.app doesn't seem to care anyway, saying
        # the installation needs 100+ MB...

        numFiles = 0
        installedSize = 0
        zippedSize = 0

        files = GlobDirectoryWalker(self.sourceFolder)
        for f in files:
            numFiles = numFiles + 1
            installedSize = installedSize + os.lstat(f)[6]

        try:
            zippedSize = os.stat(self.archPath+ ".gz")[6]
        except OSError: # ignore error
            pass
        base = self.packageInfo["Title"] + ".sizes"
        f = open(join(self.packageResourceFolder, base), "w")
        format = "NumFiles %d\nInstalledSize %d\nCompressedSize %d\n"
        f.write(format % (numFiles, installedSize, zippedSize))

    def _addLoc(self):
        "Write .loc file."
        base = self.packageInfo["Title"] + ".loc"
        f = open(join(self.packageResourceFolder, base), "w")
        f.write('/')

# Shortcut function interface

def buildPackage(*args, **options):
    "A Shortcut function for building a package."

    o = options
    title, version, desc = o["Title"], o["Version"], o["Description"]
    pm = PackageMaker(title, version, desc)
    pm.build(*args, **options)


######################################################################
# Tests
######################################################################

def test0():
    "Vanilla test for the distutils distribution."

    pm = PackageMaker("distutils2", "1.0.2", "Python distutils package.")
    pm.build("/Users/dinu/Desktop/distutils2")


def test1():
    "Test for the reportlab distribution with modified options."

    pm = PackageMaker("reportlab", "1.10",
                      "ReportLab's Open Source PDF toolkit.")
    pm.build(root="/Users/dinu/Desktop/reportlab",
             DefaultLocation="/Applications/ReportLab",
             Relocatable="YES")

def test2():
    "Shortcut test for the reportlab distribution with modified options."

    buildPackage(
        "/Users/dinu/Desktop/reportlab",
        Title="reportlab",
        Version="1.10",
        Description="ReportLab's Open Source PDF toolkit.",
        DefaultLocation="/Applications/ReportLab",
        Relocatable="YES")


######################################################################
# Command-line interface
######################################################################

def printUsage():
    "Print usage message."

    format = "Usage: %s <opts1> [<opts2>] <root> [<resources>]"
    print(format % basename(sys.argv[0]))
    print()
    print("       with arguments:")
    print("           (mandatory) root:         the package root folder")
    print("           (optional)  resources:    the package resources folder")
    print()
    print("       and options:")
    print("           (mandatory) opts1:")
    mandatoryKeys = string.split("Title Version Description", " ")
    for k in mandatoryKeys:
        print("               --%s" % k)
    print("           (optional) opts2: (with default values)")

    pmDefaults = PackageMaker.packageInfoDefaults
    optionalKeys = pmDefaults.keys()
    for k in mandatoryKeys:
        optionalKeys.remove(k)
    optionalKeys.sort()
    maxKeyLen = max(map(len, optionalKeys))
    for k in optionalKeys:
        format = "               --%%s:%s %%s"
        format = format % (" " * (maxKeyLen-len(k)))
        print(format % (k, repr(pmDefaults[k])))


def main():
    "Command-line interface."

    shortOpts = ""
    keys = PackageMaker.packageInfoDefaults.keys()
    longOpts = map(lambda k: k+"=", keys)

    try:
        opts, args = getopt.getopt(sys.argv[1:], shortOpts, longOpts)
    except getopt.GetoptError as details:
        print(details)
        printUsage()
        return

    optsDict = {}
    for k, v in opts:
        optsDict[k[2:]] = v

    ok = optsDict.keys()
    if not (1 <= len(args) <= 2):
        print("No argument given!")
    elif not ("Title" in ok and \
              "Version" in ok and \
              "Description" in ok):
        print("Missing mandatory option!")
    else:
        buildPackage(*args, **optsDict)
        return

    printUsage()

    # sample use:
    # buildpkg.py --Title=distutils \
    #             --Version=1.0.2 \
    #             --Description="Python distutils package." \
    #             /Users/dinu/Desktop/distutils


if __name__ == "__main__":
    main()
