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

    user_options = [('bdist-dir=', None,
                     "temporary directory for creating the distribution"),
                    ('keep-temp', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                    ('target-version=', 'v',
                     "require a specific python version" +
                     " on the target system"),
                    ('no-target-compile', 'c',
                     "do not compile .py to .pyc on the target system"),
                    ('no-target-optimize', 'o',
                     "do not compile .py to .pyo (optimized)"
                     "on the target system"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in"),
                    ('bitmap=', 'b',
                     "bitmap to use for the installer instead of python-powered logo"),
                    ('title=', 't',
                     "title to display on the installer background instead of default"),
                   ]

    boolean_options = ['keep-temp']

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_temp = 0
        self.no_target_compile = 0
        self.no_target_optimize = 0
        self.target_version = None
        self.dist_dir = None
        self.bitmap = None
        self.title = None

    # initialize_options()


    def finalize_options (self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'wininst')
        if not self.target_version:
            self.target_version = ""
        if self.distribution.has_ext_modules():
            short_version = sys.version[:3]
            if self.target_version and self.target_version != short_version:
                raise DistutilsOptionError, \
                      "target version can only be" + short_version
            self.target_version = short_version

        self.set_undefined_options('bdist', ('dist_dir', 'dist_dir'))

    # finalize_options()


    def run (self):
        if (sys.platform != "win32" and
            (self.distribution.has_ext_modules() or
             self.distribution.has_c_libraries())):
            raise DistutilsPlatformError \
                  ("distribution contains extensions and/or C libraries; "
                   "must be compiled on a Windows 32 platform")

        self.run_command('build')

        install = self.reinitialize_command('install')
        install.root = self.bdist_dir

        install_lib = self.reinitialize_command('install_lib')
        # we do not want to include pyc or pyo files
        install_lib.compile = 0
        install_lib.optimize = 0

        # Use a custom scheme for the zip-file, because we have to decide
        # at installation time which scheme to use.
        for key in ('purelib', 'platlib', 'headers', 'scripts', 'data'):
            value = string.upper(key)
            if key == 'headers':
                value = value + '/Include/$dist_name'
            setattr(install,
                    'install_' + key,
                    value)

        self.announce("installing to %s" % self.bdist_dir)
        install.ensure_finalized()

        # avoid warning of 'install_lib' about installing
        # into a directory not in sys.path
        sys.path.insert(0, os.path.join(self.bdist_dir, 'PURELIB'))

        install.run()

        del sys.path[0]

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        from tempfile import mktemp
        archive_basename = mktemp()
        fullname = self.distribution.get_fullname()
        arcname = self.make_archive(archive_basename, "zip",
                                    root_dir=self.bdist_dir)
        # create an exe containing the zip-file
        self.create_exe(arcname, fullname, self.bitmap)
        # remove the zip-file again
        self.announce("removing temporary file '%s'" % arcname)
        os.remove(arcname)

        if not self.keep_temp:
            remove_tree(self.bdist_dir, self.verbose, self.dry_run)

    # run()

    def get_inidata (self):
        # Return data describing the installation.

        lines = []
        metadata = self.distribution.metadata

        # Write the [metadata] section.  Values are written with
        # repr()[1:-1], so they do not contain unprintable characters, and
        # are not surrounded by quote chars.
        lines.append("[metadata]")

        # 'info' will be displayed in the installer's dialog box,
        # describing the items to be installed.
        info = (metadata.long_description or '') + '\n'

        for name in ["author", "author_email", "description", "maintainer",
                     "maintainer_email", "name", "url", "version"]:
            data = getattr(metadata, name, "")
            if data:
                info = info + ("\n    %s: %s" % \
                               (string.capitalize(name), data))
                lines.append("%s=%s" % (name, repr(data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        lines.append("\n[Setup]")
        lines.append("info=%s" % repr(info)[1:-1])
        lines.append("target_compile=%d" % (not self.no_target_compile))
        lines.append("target_optimize=%d" % (not self.no_target_optimize))
        if self.target_version:
            lines.append("target_version=%s" % self.target_version)

        title = self.title or self.distribution.get_fullname()
        lines.append("title=%s" % repr(title)[1:-1])
        import time
        import distutils
        build_info = "Build %s with distutils-%s" % \
                     (time.ctime(time.time()), distutils.__version__)
        lines.append("build_info=%s" % build_info)
        return string.join(lines, "\n")

    # get_inidata()

    def create_exe (self, arcname, fullname, bitmap=None):
        import struct

        self.mkpath(self.dist_dir)

        cfgdata = self.get_inidata()

        if self.target_version:
            # if we create an installer for a specific python version,
            # it's better to include this in the name
            installer_name = os.path.join(self.dist_dir,
                                          "%s.win32-py%s.exe" %
                                           (fullname, self.target_version))
        else:
            installer_name = os.path.join(self.dist_dir,
                                          "%s.win32.exe" % fullname)
        self.announce("creating %s" % installer_name)

        if bitmap:
            bitmapdata = open(bitmap, "rb").read()
            bitmaplen = len(bitmapdata)
        else:
            bitmaplen = 0

        file = open(installer_name, "wb")
        file.write(self.get_exe_bytes())
        if bitmap:
            file.write(bitmapdata)

        file.write(cfgdata)
        header = struct.pack("<iii",
                             0x1234567A,       # tag
                             len(cfgdata),     # length
                             bitmaplen,        # number of bytes in bitmap
                             )
        file.write(header)
        file.write(open(arcname, "rb").read())

    # create_exe()

    def get_exe_bytes (self):
        import base64
        return base64.decodestring(EXEDATA)
# class bdist_wininst

if __name__ == '__main__':
    # recreate EXEDATA from wininst.exe by rewriting this file
    import re, base64
    moddata = open("bdist_wininst.py", "r").read()
    exedata = open("../../misc/wininst.exe", "rb").read()
    print "wininst.exe length is %d bytes" % len(exedata)
    print "wininst.exe encoded length is %d bytes" % len(base64.encodestring(exedata))
    exp = re.compile('EXE'+'DATA = """\\\\(\n.*)*\n"""', re.M)
    data = exp.sub('EXE' + 'DATA = """\\\\\n%s"""' %
                    base64.encodestring(exedata), moddata)
    open("bdist_wininst.py", "w").write(data)
    print "bdist_wininst.py recreated"

EXEDATA = """\
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
//9eife5uwAAAIoHRyzoPAF394A/AXXyiweKXwRmwegIwcAQhsQp+IDr6AHwiQeDxwWJ2OLZjb4A
wAAAiwcJwHQ8i18EjYQwMPEAAAHzUIPHCP+WvPEAAJWKB0cIwHTciflXSPKuVf+WwPEAAAnAdAeJ
A4PDBOvh/5bE8QAAYek4bP//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgACAAAAIAAAgAUAAABgAACAAAAA
AAAAAAAAAAAAAAABAG4AAAA4AACAAAAAAAAAAAAAAAAAAAABAAAAAABQAAAAMLEAAAgKAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAABABrAAAAkAAAgGwAAAC4AACAbQAAAOAAAIBuAAAACAEAgAAAAAAA
AAAAAAAAAAAAAQAJBAAAqAAAADi7AACgAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAANAA
AADYvAAABAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAAD4AAAA4L4AAFoCAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAQAJBAAAIAEAAEDBAABcAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAD0AQEA
vAEBAAAAAAAAAAAAAAAAAAECAQDMAQEAAAAAAAAAAAAAAAAADgIBANQBAQAAAAAAAAAAAAAAAAAb
AgEA3AEBAAAAAAAAAAAAAAAAACUCAQDkAQEAAAAAAAAAAAAAAAAAMAIBAOwBAQAAAAAAAAAAAAAA
AAAAAAAAAAAAADoCAQBIAgEAWAIBAAAAAABmAgEAAAAAAHQCAQAAAAAAhAIBAAAAAACOAgEAAAAA
AJQCAQAAAAAAS0VSTkVMMzIuRExMAEFEVkFQSTMyLmRsbABDT01DVEwzMi5kbGwAR0RJMzIuZGxs
AE1TVkNSVC5kbGwAVVNFUjMyLmRsbAAATG9hZExpYnJhcnlBAABHZXRQcm9jQWRkcmVzcwAARXhp
dFByb2Nlc3MAAABSZWdDbG9zZUtleQAAAFByb3BlcnR5U2hlZXRBAABUZXh0T3V0QQAAZXhpdAAA
R2V0REMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAA=
"""

# --- EOF ---
