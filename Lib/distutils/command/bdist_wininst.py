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
                     "do not compile .py to .pyo (optimized) on the target system"),
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

        # save the path_file and extra_dirs options
        # created by the install command if an extra_path
        # argument has been supplied
        self.extra_dirs = install.extra_dirs
        self.path_file = install.path_file

        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        fullname = self.distribution.get_fullname()
        archive_basename = os.path.join(self.bdist_dir,
                                        "%s.win32" % fullname)

        # Our archive MUST be relative to sys.prefix, which is the
        # same as install_lib in the 'nt' scheme.
        root_dir = os.path.normpath (install.install_lib)

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
        if (self.path_file):
            lines.append ("path_file=%s" % self.path_file)
        if (self.extra_dirs):
            lines.append ("extra_dirs=%s" % self.extra_dirs)

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
TVqQAAMAAAAEAAAA//8AALgAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAA4AAAAA4fug4AtAnNIbgBTM0hVGhpcyBwcm9ncmFtIGNhbm5vdCBiZSBydW4gaW4gRE9TIG1v
ZGUuDQ0KJAAAAAAAAADq05KMrrL8366y/N+usvzf1a7w36+y/N8trvLfrLL831GS+N+ssvzfzK3v
36ay/N+usv3fzrL8366y/N+jsvzfUZL236Oy/N9ptPrfr7L831JpY2iusvzfAAAAAAAAAABQRQAA
TAEDAPimujkAAAAAAAAAAOAADwELAQYAAEAAAAAQAAAAkAAA4NUAAACgAAAA4AAAAABAAAAQAAAA
AgAABAAAAAAAAAAEAAAAAAAAAADwAAAABAAAAAAAAAIAAAAAABAAABAAAAAAEAAAEAAAAAAAABAA
AAAAAAAAAAAAADDhAABwAQAAAOAAADABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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
AABFeGl0UHJvY2VzcwAAAFJlZ0Nsb3NlS2V5AAAAUHJvcGVydHlTaGVldEEAAFRleHRPdXRBAABm
cmVlAABFbmRQYWludAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==
"""

# --- EOF ---
