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
                    ('skip-build', None,
                     "skip rebuilding everything (for testing/debugging)"),
                    ('install-script=', None,
                     "installation script to be run after installation"
                     " or before deinstallation"),
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
        import base64
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
TVqQAAMAAAAEAAAA//8AALgAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAA+AAAAA4fug4AtAnNIbgBTM0hVGhpcyBwcm9ncmFtIGNhbm5vdCBiZSBydW4gaW4gRE9TIG1v
ZGUuDQ0KJAAAAAAAAAA/1p0ge7fzc3u383N7t/NzAKv/c3m383MUqPlzcLfzc/ir/XN5t/NzFKj3
c3m383N7t/NzdLfzc3u38nPzt/NzGajgc3C383N9lPlzebfzc7yx9XN6t/NzUmljaHu383MAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAABQRQAATAEDAIRxcjwAAAAAAAAAAOAADwELAQYAAFAAAAAQAAAA
oAAA0PMAAACwAAAAAAEAAABAAAAQAAAAAgAABAAAAAAAAAAEAAAAAAAAAAAQAQAABAAAAAAAAAIA
AAAAABAAABAAAAAAEAAAEAAAAAAAABAAAAAAAAAAAAAAADABAQCgAQAAAAABADABAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFVQWDAAAAAAAKAAAAAQAAAA
AAAAAAQAAAAAAAAAAAAAAAAAAIAAAOBVUFgxAAAAAABQAAAAsAAAAEYAAAAEAAAAAAAAAAAAAAAA
AABAAADgLnJzcmMAAAAAEAAAAAABAAAEAAAASgAAAAAAAAAAAAAAAAAAQAAAwAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgAkSW5mbzogVGhpcyBmaWxlIGlz
IHBhY2tlZCB3aXRoIHRoZSBVUFggZXhlY3V0YWJsZSBwYWNrZXIgaHR0cDovL3VweC50c3gub3Jn
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
2MwAAAQCAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJBAAA+AAAAODOAABaAgAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAEACQQAACABAABA0QAAFAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAIBANAB
AQAAAAAAAAAAAAAAAAAdAgEA4AEBAAAAAAAAAAAAAAAAACoCAQDoAQEAAAAAAAAAAAAAAAAANwIB
APABAQAAAAAAAAAAAAAAAABBAgEA+AEBAAAAAAAAAAAAAAAAAEwCAQAAAgEAAAAAAAAAAAAAAAAA
VgIBAAgCAQAAAAAAAAAAAAAAAAAAAAAAAAAAAGACAQBuAgEAfgIBAAAAAACMAgEAAAAAAJoCAQAA
AAAAqgIBAAAAAAC0AgEAAAAAALoCAQAAAAAAyAIBAAAAAABLRVJORUwzMi5ETEwAQURWQVBJMzIu
ZGxsAENPTUNUTDMyLmRsbABHREkzMi5kbGwATVNWQ1JULmRsbABvbGUzMi5kbGwAVVNFUjMyLmRs
bAAATG9hZExpYnJhcnlBAABHZXRQcm9jQWRkcmVzcwAARXhpdFByb2Nlc3MAAABSZWdDbG9zZUtl
eQAAAFByb3BlcnR5U2hlZXRBAABUZXh0T3V0QQAAZXhpdAAAQ29Jbml0aWFsaXplAABHZXREQwAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
"""

# --- EOF ---
