"""distutils.command.bdist_wininst

Implements the Distutils 'bdist_wininst' command: create a windows installer
exe-program."""

# created 2000/06/02, Thomas Heller

__revision__ = "$Id$"

import sys, os, string
import base64
from distutils.core import Command
from distutils.util import get_platform
from distutils.dir_util import create_tree, remove_tree
from distutils.errors import *
from distutils import log

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
                    ('skip-build', None,
                     "skip rebuilding everything (for testing/debugging)"),
                    ('install-script=', None,
                     "installation script to be run after installation"
                     " or before deinstallation"),
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
        self.skip_build = 0
        self.install_script = None

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

        if self.install_script and \
               self.install_script not in self.distribution.scripts:
            raise DistutilsOptionError, \
                  "install_script '%s' not found in scripts" % self.install_script

    # finalize_options()


    def run (self):
        if (sys.platform != "win32" and
            (self.distribution.has_ext_modules() or
             self.distribution.has_c_libraries())):
            raise DistutilsPlatformError \
                  ("distribution contains extensions and/or C libraries; "
                   "must be compiled on a Windows 32 platform")

        if not self.skip_build:
            self.run_command('build')

        install = self.reinitialize_command('install')
        install.root = self.bdist_dir
        install.skip_build = self.skip_build
        install.warn_dir = 0

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

        log.info("installing to %s", self.bdist_dir)
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
        log.debug("removing temporary file '%s'", arcname)
        os.remove(arcname)

        if not self.keep_temp:
            remove_tree(self.bdist_dir, dry_run=self.dry_run)

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
        if self.install_script:
            lines.append("install_script=%s" % self.install_script)
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
        return base64.decodestring(EXEDATA)
# class bdist_wininst

if __name__ == '__main__':
    # recreate EXEDATA from wininst.exe by rewriting this file

    # If you want to do this at home, you should:
    # - checkout the *distutils* source code
    #   (see also http://sourceforge.net/cvs/?group_id=5470)
    #   by doing:
    #     cvs -d:pserver:anonymous@cvs.python.sourceforge.net:/cvsroot/python login
    #   and
    #     cvs -z3 -d:pserver:anonymous@cvs.python.sourceforge.net:/cvsroot/python co distutils
    # - Built wininst.exe from the MSVC project file distutils/misc/wininst.dsw
    # - Execute this file (distutils/distutils/command/bdist_wininst.py)

    import re
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
A3INweAIigZGg/D/dHSJxQHbdQeLHoPu/BHbEckB23UHix6D7vwR2xHJdSBBAdt1B4seg+78EdsR
yQHbc+91CYseg+78Edtz5IPBAoH9APP//4PRAY0UL4P9/HYPigJCiAdHSXX36WP///+QiwKDwgSJ
B4PHBIPpBHfxAc/pTP///16J97nQAAAAigdHLOg8AXf3gD8GdfKLB4pfBGbB6AjBwBCGxCn4gOvo
AfCJB4PHBYnY4tmNvgDgAACLBwnAdDyLXwSNhDAwAQEAAfNQg8cI/5bkAQEAlYoHRwjAdNyJ+VdI
8q5V/5boAQEACcB0B4kDg8ME6+H/luwBAQBh6UJf//8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgACAAAAIAAAgAUAAABgAACAAAAAAAAA
AAAAAAAAAAABAG4AAAA4AACAAAAAAAAAAAAAAAAAAAABAAAAAABQAAAAMNEAAAgKAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAABABrAAAAkAAAgGwAAAC4AACAbQAAAOAAAIBuAAAACAEAgAAAAAAAAAAA
AAAAAAAAAQAJBAAAqAAAADjbAACgAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAANAAAADY
3AAABAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAAD4AAAA4N4AAFoCAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAQAJBAAAIAEAAEDhAAAUAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAsEgEA5BEB
AAAAAAAAAAAAAAAAADkSAQD0EQEAAAAAAAAAAAAAAAAARhIBAPwRAQAAAAAAAAAAAAAAAABTEgEA
BBIBAAAAAAAAAAAAAAAAAF0SAQAMEgEAAAAAAAAAAAAAAAAAaBIBABQSAQAAAAAAAAAAAAAAAABy
EgEAHBIBAAAAAAAAAAAAAAAAAH4SAQAkEgEAAAAAAAAAAAAAAAAAAAAAAAAAAACIEgEAlhIBAKYS
AQAAAAAAtBIBAAAAAADCEgEAAAAAANISAQAAAAAA3BIBAAAAAADiEgEAAAAAAPASAQAAAAAAChMB
AAAAAABLRVJORUwzMi5ETEwAQURWQVBJMzIuZGxsAENPTUNUTDMyLmRsbABHREkzMi5kbGwATVNW
Q1JULmRsbABvbGUzMi5kbGwAU0hFTEwzMi5kbGwAVVNFUjMyLmRsbAAATG9hZExpYnJhcnlBAABH
ZXRQcm9jQWRkcmVzcwAARXhpdFByb2Nlc3MAAABSZWdDbG9zZUtleQAAAFByb3BlcnR5U2hlZXRB
AABUZXh0T3V0QQAAZnJlZQAAQ29Jbml0aWFsaXplAABTSEdldFNwZWNpYWxGb2xkZXJQYXRoQQAA
AEdldERDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAA=
"""

# --- EOF ---
