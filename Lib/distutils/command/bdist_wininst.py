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

    user_options = [('bdist-dir=', 'd',
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
ZGUuDQ0KJAAAAAAAAABwr7eMNM7Z3zTO2d80ztnfT9LV3zXO2d+30tffNs7Z39zR3d82ztnfVtHK
3zzO2d80ztjfVM7Z3zTO2d85ztnf3NHT3znO2d+MyN/fNc7Z31JpY2g0ztnfAAAAAAAAAABQRQAA
TAEDAG/hkDoAAAAAAAAAAOAADwELAQYAAEAAAAAQAAAAkAAAwNUAAACgAAAA4AAAAABAAAAQAAAA
AgAABAAAAAAAAAAEAAAAAAAAAADwAAAABAAAAAAAAAIAAAAAABAAABAAAAAAEAAAEAAAAAAAABAA
AAAAAAAAAAAAADDhAABsAQAAAOAAADABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAFVQWDAAAAAAAJAAAAAQAAAAAAAAAAQAAAAAAAAAAAAAAAAAAIAAAOBV
UFgxAAAAAABAAAAAoAAAADgAAAAEAAAAAAAAAAAAAAAAAABAAADgLnJzcmMAAAAAEAAAAOAAAAAE
AAAAPAAAAAAAAAAAAAAAAAAAQAAAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgAkSW5mbzogVGhpcyBmaWxlIGlz
IHBhY2tlZCB3aXRoIHRoZSBVUFggZXhlY3V0YWJsZSBwYWNrZXIgaHR0cDovL3VweC50c3gub3Jn
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
AAAAAAAAAAAAAAAAAAABAAkEAACoAAAAOKsAAKABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJ
BAAA0AAAANisAABiAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAAPgAAABArgAAWgIAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAAAgAQAAoLAAAFwBAAAAAAAAAAAAAAAAAAAAAAAAAAAA
APThAAC84QAAAAAAAAAAAAAAAAAAAeIAAMzhAAAAAAAAAAAAAAAAAAAO4gAA1OEAAAAAAAAAAAAA
AAAAABviAADc4QAAAAAAAAAAAAAAAAAAJeIAAOThAAAAAAAAAAAAAAAAAAAw4gAA7OEAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAOuIAAEjiAABY4gAAAAAAAGbiAAAAAAAAdOIAAAAAAACE4gAAAAAAAI7i
AAAAAAAAlOIAAAAAAABLRVJORUwzMi5ETEwAQURWQVBJMzIuZGxsAENPTUNUTDMyLmRsbABHREkz
Mi5kbGwATVNWQ1JULmRsbABVU0VSMzIuZGxsAABMb2FkTGlicmFyeUEAAEdldFByb2NBZGRyZXNz
AABFeGl0UHJvY2VzcwAAAFJlZ0Nsb3NlS2V5AAAAUHJvcGVydHlTaGVldEEAAFRleHRPdXRBAABl
eGl0AABHZXREQwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==
"""

# --- EOF ---
