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
                     "basename of installation script to be run after"
                     "installation or before deinstallation"),
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

        if self.install_script:
            for script in self.distribution.scripts:
                if self.install_script == os.path.basename(script):
                    break
            else:
                raise DistutilsOptionError, \
                      "install_script '%s' not found in scripts" % \
                      self.install_script
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
6xCQkJCQkJCKBkaIB0cB23UHix6D7vwR23LtuAEAAAAB23UHix6D7vwR2xHAAdtz73UJix6D7vwR
23PkMcmD6ANyDcHgCIoGRoPw/3R0icUB23UHix6D7vwR2xHJAdt1B4seg+78EdsRyXUgQQHbdQeL
HoPu/BHbEckB23PvdQmLHoPu/BHbc+SDwQKB/QDz//+D0QGNFC+D/fx2D4oCQogHR0l19+lj////
kIsCg8IEiQeDxwSD6QR38QHP6Uz///9eife50QAAAIoHRyzoPAF394A/AXXyiweKXwRmwegIwcAQ
hsQp+IDr6AHwiQeDxwWJ2OLZjb4A4AAAiwcJwHQ8i18EjYQwMAEBAAHzUIPHCP+W5AEBAJWKB0cI
wHTciflXSPKuVf+W6AEBAAnAdAeJA4PDBOvh/5bsAQEAYekyX///AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAIAAAAgAACABQAAAGAAAIAAAAAAAAAA
AAAAAAAAAAEAbgAAADgAAIAAAAAAAAAAAAAAAAAAAAEAAAAAAFAAAAAw0QAACAoAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAEAGsAAACQAACAbAAAALgAAIBtAAAA4AAAgG4AAAAIAQCAAAAAAAAAAAAA
AAAAAAABAAkEAACoAAAAONsAAKABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJBAAA0AAAANjc
AAAEAgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAAPgAAADg3gAAWgIAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAABAAkEAAAgAQAAQOEAABQBAAAAAAAAAAAAAAAAAAAAAAAAAAAAACwSAQDkEQEA
AAAAAAAAAAAAAAAAORIBAPQRAQAAAAAAAAAAAAAAAABGEgEA/BEBAAAAAAAAAAAAAAAAAFMSAQAE
EgEAAAAAAAAAAAAAAAAAXRIBAAwSAQAAAAAAAAAAAAAAAABoEgEAFBIBAAAAAAAAAAAAAAAAAHIS
AQAcEgEAAAAAAAAAAAAAAAAAfhIBACQSAQAAAAAAAAAAAAAAAAAAAAAAAAAAAIgSAQCWEgEAphIB
AAAAAAC0EgEAAAAAAMISAQAAAAAA0hIBAAAAAADcEgEAAAAAAOISAQAAAAAA8BIBAAAAAAAKEwEA
AAAAAEtFUk5FTDMyLkRMTABBRFZBUEkzMi5kbGwAQ09NQ1RMMzIuZGxsAEdESTMyLmRsbABNU1ZD
UlQuZGxsAG9sZTMyLmRsbABTSEVMTDMyLmRsbABVU0VSMzIuZGxsAABMb2FkTGlicmFyeUEAAEdl
dFByb2NBZGRyZXNzAABFeGl0UHJvY2VzcwAAAFJlZ0Nsb3NlS2V5AAAAUHJvcGVydHlTaGVldEEA
AFRleHRPdXRBAABmcmVlAABDb0luaXRpYWxpemUAAFNIR2V0U3BlY2lhbEZvbGRlclBhdGhBAAAA
R2V0REMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAA==
"""

# --- EOF ---
