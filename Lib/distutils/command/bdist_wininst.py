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

    boolean_options = ['keep-temp', 'no-target-compile', 'no-target-optimize',
                       'skip-build']

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
AXf3gD8BdfKLB4pfBGbB6AjBwBCGxCn4gOvoAfCJB4PHBYnY4tmNvgDAAACLBwnAdDyLXwSNhDAw
8QAAAfNQg8cI/5a88QAAlYoHRwjAdNyJ+VdI8q5V/5bA8QAACcB0B4kDg8ME6+H/lsTxAABh6Vhs
//8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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
