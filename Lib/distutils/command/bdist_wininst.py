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
        from tempfile import NamedTemporaryFile
        arc = NamedTemporaryFile(".zip")
        archive_basename = arc.name[:-4]
        fullname = self.distribution.get_fullname()
        arcname = self.make_archive(archive_basename, "zip",
                                    root_dir=self.bdist_dir)
        # create an exe containing the zip-file
        self.create_exe(arcname, fullname, self.bitmap)
        # remove the zip-file again
        log.debug("removing temporary file '%s'", arcname)
        arc.close()

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
B4kDg8ME6+H/luwBAQBh6Whe//8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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
