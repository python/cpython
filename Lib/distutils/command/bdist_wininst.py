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
                   ]

    boolean_options = ['keep-temp']

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_temp = 0
        self.no_target_compile = 0
        self.no_target_optimize = 0
        self.target_version = None
        self.dist_dir = None

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
                raise DistutilsOptionError ("target version can only be" +
                                            short_version)
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

        self.run_command ('build')

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

        self.announce ("installing to %s" % self.bdist_dir)
        install.ensure_finalized()
        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        fullname = self.distribution.get_fullname()
        archive_basename = os.path.join(self.bdist_dir,
                                        "%s.win32" % fullname)

        # Our archive MUST be relative to sys.prefix, which is the
        # same as install_purelib in the 'nt' scheme.
        root_dir = os.path.normpath (install.install_purelib)

        # Sanity check: Make sure everything is included
        for key in ('purelib', 'platlib', 'headers', 'scripts', 'data'):
            attrname = 'install_' + key
            install_x = getattr (install, attrname)
            # (Use normpath so that we can string.find to look for
            # subdirectories)
            install_x = os.path.normpath (install_x)
            if string.find (install_x, root_dir) != 0:
                raise DistutilsInternalError \
                      ("'%s' not included in install_lib" % key)
        arcname = self.make_archive (archive_basename, "zip",
                                     root_dir=root_dir)
        self.create_exe (arcname, fullname)

        if not self.keep_temp:
            remove_tree (self.bdist_dir, self.verbose, self.dry_run)

    # run()

    def get_inidata (self):
        # Return data describing the installation.

        lines = []
        metadata = self.distribution.metadata

        # Write the [metadata] section.  Values are written with
        # repr()[1:-1], so they do not contain unprintable characters, and
        # are not surrounded by quote chars.
        lines.append ("[metadata]")

        # 'info' will be displayed in the installer's dialog box,
        # describing the items to be installed.
        info = (metadata.long_description or '') + '\n'

        for name in dir (metadata):
            if (name != 'long_description'):
                data = getattr (metadata, name)
                if data:
                   info = info + ("\n    %s: %s" % \
                                  (string.capitalize (name), data))
                   lines.append ("%s=%s" % (name, repr (data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        lines.append ("\n[Setup]")
        lines.append ("info=%s" % repr (info)[1:-1])
        lines.append ("target_compile=%d" % (not self.no_target_compile))
        lines.append ("target_optimize=%d" % (not self.no_target_optimize))
        if self.target_version:
            lines.append ("target_version=%s" % self.target_version)

        title = self.distribution.get_fullname()
        lines.append ("title=%s" % repr (title)[1:-1])
        import time
        import distutils
        build_info = "Build %s with distutils-%s" % \
                     (time.ctime (time.time()), distutils.__version__)
        lines.append ("build_info=%s" % build_info)
        return string.join (lines, "\n")

    # get_inidata()

    def create_exe (self, arcname, fullname):
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
        self.announce ("creating %s" % installer_name)

        file = open (installer_name, "wb")
        file.write (self.get_exe_bytes ())
        file.write (cfgdata)
        header = struct.pack ("<ii",
                              0x12345679,       # tag
                              len (cfgdata))    # length
        file.write (header)
        file.write (open (arcname, "rb").read())

    # create_exe()

    def get_exe_bytes (self):
        import base64
        return base64.decodestring (EXEDATA)

# class bdist_wininst

if __name__ == '__main__':
    # recreate EXEDATA from wininst.exe by rewriting this file
    import re, base64
    moddata = open ("bdist_wininst.py", "r").read()
    exedata = open ("../../misc/wininst.exe", "rb").read()
    print "wininst.exe length is %d bytes" % len (exedata)
    print "wininst.exe encoded length is %d bytes" % len (base64.encodestring (exedata))
    exp = re.compile ('EXE'+'DATA = """\\\\(\n.*)*\n"""', re.M)
    data = exp.sub ('EXE' + 'DATA = """\\\\\n%s"""' %
                    base64.encodestring (exedata), moddata)
    open ("bdist_wininst.py", "w").write (data)
    print "bdist_wininst.py recreated"

EXEDATA = """\
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
YL4AoEAAjb4AcP//V4PN/+sQkJCQkJCQigZGiAdHAdt1B4seg+78Edty7bgBAAAAAdt1B4seg+78
EdsRwAHbc+91CYseg+78Edtz5DHJg+gDcg3B4AiKBkaD8P90dInFAdt1B4seg+78EdsRyQHbdQeL
HoPu/BHbEcl1IEEB23UHix6D7vwR2xHJAdtz73UJix6D7vwR23Pkg8ECgf0A8///g9EBjRQvg/38
dg+KAkKIB0dJdffpY////5CLAoPCBIkHg8cEg+kEd/EBz+lM////Xon3uZAAAACKB0cs6DwBd/eA
PwF18osHil8EZsHoCMHAEIbEKfiA6+gB8IkHg8cFidji2Y2+ALAAAIsHCcB0PItfBI2EMDDRAAAB
81CDxwj/lrzRAACVigdHCMB03In5V0jyrlX/lsDRAAAJwHQHiQODwwTr4f+WxNEAAGHpmHj//wAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAIAAAAgAACABQAAAGAA
AIAAAAAAAAAAAAAAAAAAAAEAbgAAADgAAIAAAAAAAAAAAAAAAAAAAAEAAAAAAFAAAAAwoQAACAoA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAGsAAACQAACAbAAAALgAAIBtAAAA4AAAgG4AAAAIAQCA
AAAAAAAAAAAAAAAAAAABAAkEAACoAAAAOKsAAKABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJ
BAAA0AAAANisAABiAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAAPgAAABArgAAWgIAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAAAgAQAAoLAAAFwBAAAAAAAAAAAAAAAAAAAAAAAAAAAA
APThAAC84QAAAAAAAAAAAAAAAAAAAeIAAMzhAAAAAAAAAAAAAAAAAAAO4gAA1OEAAAAAAAAAAAAA
AAAAABviAADc4QAAAAAAAAAAAAAAAAAAJeIAAOThAAAAAAAAAAAAAAAAAAAw4gAA7OEAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAOuIAAEjiAABY4gAAAAAAAGbiAAAAAAAAdOIAAAAAAACE4gAAAAAAAI7i
AAAAAAAAlOIAAAAAAABLRVJORUwzMi5ETEwAQURWQVBJMzIuZGxsAENPTUNUTDMyLmRsbABHREkz
Mi5kbGwATVNWQ1JULmRsbABVU0VSMzIuZGxsAABMb2FkTGlicmFyeUEAAEdldFByb2NBZGRyZXNz
AABFeGl0UHJvY2VzcwAAAFJlZ0Nsb3NlS2V5AAAAUHJvcGVydHlTaGVldEEAAFRleHRPdXRBAABl
eGl0AABFbmRQYWludAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==
"""

# --- EOF ---
