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
TAEDABAF0joAAAAAAAAAAOAADwELAQYAAEAAAAAQAAAAoAAA8OwAAACwAAAA8AAAAABAAAAQAAAA
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
