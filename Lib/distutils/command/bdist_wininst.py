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
        fullname = self.distribution.get_fullname()
        archive_basename = os.path.join(self.bdist_dir,
                                        "%s.win32" % fullname)

        arcname = self.make_archive(archive_basename, "zip",
                                    root_dir=self.bdist_dir)
        self.create_exe(arcname, fullname, self.bitmap)

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

        for name in dir(metadata):
            if (name != 'long_description'):
                data = getattr(metadata, name)
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
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAgAAACAAAIAFAAAAYAAAgAAA
AAAAAAAAAAAAAAAAAQBuAAAAOAAAgAAAAAAAAAAAAAAAAAAAAQAAAAAAUAAAADCxAAAICgAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAQAawAAAJAAAIBsAAAAuAAAgG0AAADgAACAbgAAAAgBAIAAAAAA
AAAAAAAAAAAAAAEACQQAAKgAAAA4uwAAoAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAADQ
AAAA2LwAAGIBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJBAAA+AAAAEC+AABaAgAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAEACQQAACABAACgwAAAXAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAA9PEA
ALzxAAAAAAAAAAAAAAAAAAAB8gAAzPEAAAAAAAAAAAAAAAAAAA7yAADU8QAAAAAAAAAAAAAAAAAA
G/IAANzxAAAAAAAAAAAAAAAAAAAl8gAA5PEAAAAAAAAAAAAAAAAAADDyAADs8QAAAAAAAAAAAAAA
AAAAAAAAAAAAAAA68gAASPIAAFjyAAAAAAAAZvIAAAAAAAB08gAAAAAAAITyAAAAAAAAjvIAAAAA
AACU8gAAAAAAAEtFUk5FTDMyLkRMTABBRFZBUEkzMi5kbGwAQ09NQ1RMMzIuZGxsAEdESTMyLmRs
bABNU1ZDUlQuZGxsAFVTRVIzMi5kbGwAAExvYWRMaWJyYXJ5QQAAR2V0UHJvY0FkZHJlc3MAAEV4
aXRQcm9jZXNzAAAAUmVnQ2xvc2VLZXkAAABQcm9wZXJ0eVNoZWV0QQAAVGV4dE91dEEAAGV4aXQA
AEdldERDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAA
"""

# --- EOF ---
