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
        if os.name != 'nt':
            # Must force install to use the 'nt' scheme; we set the
            # prefix too just because it looks silly to put stuff
            # in (eg.) ".../usr/Scripts", "usr/Include", etc.
            install.prefix = "Python"
            install.select_scheme('nt')

        install_lib = self.reinitialize_command('install_lib')
        # we do not want to include pyc or pyo files
        install_lib.compile = 0
        install_lib.optimize = 0

        install_lib.ensure_finalized()

        self.announce("installing to %s" % self.bdist_dir)
        install.ensure_finalized()
        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        fullname = self.distribution.get_fullname()
        archive_basename = os.path.join(self.bdist_dir,
                                        "%s.win32" % fullname)

        # Our archive MUST be relative to sys.prefix, which is the
        # same as install_purelib in the 'nt' scheme.
        root_dir = os.path.normpath(install.install_purelib)

        # Sanity check: Make sure everything is included
        for key in ('purelib', 'platlib', 'headers', 'scripts', 'data'):
            attrname = 'install_' + key
            install_x = getattr(install, attrname)
            # (Use normpath so that we can string.find to look for
            # subdirectories)
            install_x = os.path.normpath(install_x)
            if string.find(install_x, root_dir) != 0:
                raise DistutilsInternalError \
                      ("'%s' not included in install_lib" % key)
        arcname = self.make_archive(archive_basename, "zip",
                                    root_dir=root_dir)
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
TVqQAAMAAAAEAAAA//8AALgAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAA4AAAAA4fug4AtAnNIbgBTM0hVGhpcyBwcm9ncmFtIGNhbm5vdCBiZSBydW4gaW4gRE9TIG1v
ZGUuDQ0KJAAAAAAAAABwv7aMNN7Y3zTe2N803tjfT8LU3zXe2N+3wtbfNt7Y39zB3N823tjfVsHL
3zze2N803tnfSN7Y3zTe2N853tjf3MHS3zne2N+M2N7fNd7Y31JpY2g03tjfAAAAAAAAAABQRQAA
TAEDAE55sjoAAAAAAAAAAOAADwELAQYAAEAAAAAQAAAAoAAAsOwAAACwAAAA8AAAAABAAAAQAAAA
AgAABAAAAAAAAAAEAAAAAAAAAAAAAQAABAAAAAAAAAIAAAAAABAAABAAAAAAEAAAEAAAAAAAABAA
AAAAAAAAAAAAADDxAABsAQAAAPAAADABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAFVQWDAAAAAAAKAAAAAQAAAAAAAAAAQAAAAAAAAAAAAAAAAAAIAAAOBV
UFgxAAAAAABAAAAAsAAAAEAAAAAEAAAAAAAAAAAAAAAAAABAAADgLnJzcmMAAAAAEAAAAPAAAAAE
AAAARAAAAAAAAAAAAAAAAAAAQAAAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgAkSW5mbzogVGhpcyBmaWxlIGlz
IHBhY2tlZCB3aXRoIHRoZSBVUFggZXhlY3V0YWJsZSBwYWNrZXIgaHR0cDovL3VweC50c3gub3Jn
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
