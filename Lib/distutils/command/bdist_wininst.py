"""distutils.command.bdist_wininst

Implements the Distutils 'bdist_wininst' command: create a windows installer
exe-program."""

# created 2000/06/02, Thomas Heller

__revision__ = "$Id$"

import sys, os, string
from distutils.core import Command
from distutils.util import get_platform
from distutils.dir_util import create_tree, remove_tree
from distutils.errors import *

class bdist_wininst (Command):

    description = "create an executable installer for MS Windows"

    user_options = [('bdist-dir=', 'd',
                     "temporary directory for creating the distribution"),
                    ('keep-tree', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                    ('target-version=', 'v',
                     "require a specific python version" +
                     " on the target system"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in"),
                   ]

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_tree = 0
        self.target_compile = 0
        self.target_optimize = 0
        self.target_version = None
        self.dist_dir = None

    # initialize_options()


    def finalize_options (self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'wininst')
        if not self.target_version:
            self.target_version = ""
        else:
            if not self.target_version in ("1.5", "1.6", "2.0"):
                raise DistutilsOptionError (
                    "target version must be 1.5, 1.6, or 2.0")
        if self.distribution.has_ext_modules():
            short_version = sys.version[:3]
            if self.target_version and self.target_version != short_version:
                raise DistutilsOptionError ("target version can only be" +
                                            short_version)
            self.target_version = short_version

        self.set_undefined_options('bdist', ('dist_dir', 'dist_dir'))

    # finalize_options()


    def run (self):
        if (sys.platform != "win32" and
            (self.distribution.has_ext_modules() or
             self.distribution.has_c_libraries())):
            raise DistutilsPlatformError, \
                  ("distribution contains extensions and/or C libraries; "
                   "must be compiled on a Windows 32 platform")

        self.run_command ('build')

        install = self.reinitialize_command('install')
        install.root = self.bdist_dir

        install_lib = self.reinitialize_command('install_lib')
        # we do not want to include pyc or pyo files
        install_lib.compile = 0
        install_lib.optimize = 0

        install_lib.ensure_finalized()

        self.announce ("installing to %s" % self.bdist_dir)
        install.ensure_finalized()
        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        fullname = self.distribution.get_fullname()
        archive_basename = os.path.join(self.bdist_dir,
                                        "%s.win32" % fullname)

        # XXX hack! Our archive MUST be relative to sys.prefix
        # XXX What about .install_data, .install_scripts, ...?
        # [Perhaps require that all installation dirs be under sys.prefix
        # on Windows?  this will be acceptable until we start dealing
        # with Python applications, at which point we should zip up
        # the application directory -- and again everything can be
        # under one dir --GPW]
        root_dir = install.install_lib
        arcname = self.make_archive (archive_basename, "zip",
                                     root_dir=root_dir)
        self.create_exe (arcname, fullname)

        if not self.keep_tree:
            remove_tree (self.bdist_dir, self.verbose, self.dry_run)

    # run()

    def get_inidata (self):
        # Return data describing the installation.

        lines = []
        metadata = self.distribution.metadata

        # Write the [metadata] section.  Values are written with
        # repr()[1:-1], so they do not contain unprintable characters, and
        # are not surrounded by quote chars.
        lines.append ("[metadata]")

        # 'info' will be displayed in the installer's dialog box,
        # describing the items to be installed.
        info = (metadata.long_description or '') + '\n'

        for name in dir (metadata):
            if (name != 'long_description'):
                data = getattr (metadata, name)
                if data:
                   info = info + ("\n    %s: %s" % \
                                  (string.capitalize (name), data))
                   lines.append ("%s=%s" % (name, repr (data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        lines.append ("\n[Setup]")
        lines.append ("info=%s" % repr (info)[1:-1])
        lines.append ("pthname=%s.%s" % (metadata.name, metadata.version))
        if self.target_version:
            lines.append ("target_version=%s" % self.target_version)

        title = self.distribution.get_fullname()
        lines.append ("title=%s" % repr (title)[1:-1])
        return string.join (lines, "\n")

    # get_inidata()

    def create_exe (self, arcname, fullname):
        import struct

        self.mkpath(self.dist_dir)

        cfgdata = self.get_inidata()

        installer_name = os.path.join(self.dist_dir,
                                      "%s.win32.exe" % fullname)
        self.announce ("creating %s" % installer_name)

        file = open (installer_name, "wb")
        file.write (self.get_exe_bytes ())
        file.write (cfgdata)
        header = struct.pack ("<ii",
                              0x12345679,       # tag
                              len (cfgdata))    # length
        file.write (header)
        file.write (open (arcname, "rb").read())

    # create_exe()

    def get_exe_bytes (self):
        import base64
        return base64.decodestring (EXEDATA)

# class bdist_wininst

if __name__ == '__main__':
    import base64
    file = r"..\..\misc\wininst.exe"
    data = open (file, "rb").read()
    bdata = base64.encodestring (data)
    open ("EXEDATA", "w").write (bdata)
    print "%d %d" % (len (data), len (bdata))

EXEDATA = """\
TVqQAAMAAAAEAAAA//8AALgAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAA4AAAAA4fug4AtAnNIbgBTM0hVGhpcyBwcm9ncmFtIGNhbm5vdCBiZSBydW4gaW4gRE9TIG1v
ZGUuDQ0KJAAAAAAAAADq0pWMrrP7366z+9+us/vf1a/336+z+98tr/XfrLP731GT/9+ss/vfzKzo
36az+9+us/rf9rP7366z+9+js/vfUZPx36Oz+99ptf3fr7P731JpY2ius/vfAAAAAAAAAABQRQAA
TAEDAE2JpjkAAAAAAAAAAOAADwELAQYAAEAAAAAQAAAAgAAAQMMAAACQAAAA0AAAAABAAAAQAAAA
AgAABAAAAAAAAAAEAAAAAAAAAADgAAAABAAAAAAAAAIAAAAAABAAABAAAAAAEAAAEAAAAAAAABAA
AAAAAAAAAAAAADDRAABwAQAAANAAADABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAFVQWDAAAAAAAIAAAAAQAAAAAAAAAAQAAAAAAAAAAAAAAAAAAIAAAOBV
UFgxAAAAAABAAAAAkAAAADYAAAAEAAAAAAAAAAAAAAAAAABAAADgLnJzcmMAAAAAEAAAANAAAAAE
AAAAOgAAAAAAAAAAAAAAAAAAQAAAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgAkSW5mbzogVGhpcyBmaWxlIGlz
IHBhY2tlZCB3aXRoIHRoZSBVUFggZXhlY3V0YWJsZSBwYWNrZXIgaHR0cDovL3VweC50c3gub3Jn
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
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgACAAAAIAAAgAUAAABg
AACAAAAAAAAAAAAAAAAAAAABAG4AAAA4AACAAAAAAAAAAAAAAAAAAAABAAAAAABQAAAAMJEAAAgK
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABABrAAAAkAAAgGwAAAC4AACAbQAAAOAAAIBuAAAACAEA
gAAAAAAAAAAAAAAAAAAAAQAJBAAAqAAAADibAAB+AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEA
CQQAANAAAAC4nAAAbgEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAAD4AAAAKJ4AAFoCAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJBAAAIAEAAIigAABcAQAAAAAAAAAAAAAAAAAAAAAAAAAA
AAD00QAAvNEAAAAAAAAAAAAAAAAAAAHSAADM0QAAAAAAAAAAAAAAAAAADtIAANTRAAAAAAAAAAAA
AAAAAAAb0gAA3NEAAAAAAAAAAAAAAAAAACXSAADk0QAAAAAAAAAAAAAAAAAAMNIAAOzRAAAAAAAA
AAAAAAAAAAAAAAAAAAAAADrSAABI0gAAWNIAAAAAAABm0gAAAAAAAHTSAAAAAAAAhNIAAAAAAACO
0gAAAAAAAJTSAAAAAAAAS0VSTkVMMzIuRExMAEFEVkFQSTMyLmRsbABDT01DVEwzMi5kbGwAR0RJ
MzIuZGxsAE1TVkNSVC5kbGwAVVNFUjMyLmRsbAAATG9hZExpYnJhcnlBAABHZXRQcm9jQWRkcmVz
cwAARXhpdFByb2Nlc3MAAABSZWdDbG9zZUtleQAAAFByb3BlcnR5U2hlZXRBAABUZXh0T3V0QQAA
ZXhpdAAARW5kUGFpbnQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=
"""
