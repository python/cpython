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
B4seg+78EdsRyXUgQQHbdQeLHoPu/BHbEckB23PvdQmLHoPu/BHbc+SDwQKB/QDz//+D0QGNFC+D
/fx2D4oCQogHR0l19+lj////kIsCg8IEiQeDxwSD6QR38QHP6Uz///9eife5zgAAAIoHRyzoPAF3
94A/BnXyiweKXwRmwegIwcAQhsQp+IDr6AHwiQeDxwWJ2OLZjb4A4AAAiwcJwHQ8i18EjYQwMAEB
AAHzUIPHCP+W5AEBAJWKB0cIwHTciflXSPKuVf+W6AEBAAnAdAeJA4PDBOvh/5bsAQEAYenoXv//
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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
AABUZXh0T3V0QQAAZXhpdAAAQ29Jbml0aWFsaXplAABTSEdldFNwZWNpYWxGb2xkZXJQYXRoQQAA
AEdldERDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAA=
"""

# --- EOF ---
