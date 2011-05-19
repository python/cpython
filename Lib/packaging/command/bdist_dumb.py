"""Create a "dumb" built distribution.

A dumb distribution is just an archive meant to be unpacked under
sys.prefix or sys.exec_prefix.
"""

import os

from shutil import rmtree
from sysconfig import get_python_version
from packaging.util import get_platform
from packaging.command.cmd import Command
from packaging.errors import PackagingPlatformError
from packaging import logger

class bdist_dumb(Command):

    description = 'create a "dumb" built distribution'

    user_options = [('bdist-dir=', 'd',
                     "temporary directory for creating the distribution"),
                    ('plat-name=', 'p',
                     "platform name to embed in generated filenames "
                     "(default: %s)" % get_platform()),
                    ('format=', 'f',
                     "archive format to create (tar, ztar, gztar, zip)"),
                    ('keep-temp', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in"),
                    ('skip-build', None,
                     "skip rebuilding everything (for testing/debugging)"),
                    ('relative', None,
                     "build the archive using relative paths"
                     "(default: false)"),
                    ('owner=', 'u',
                     "Owner name used when creating a tar file"
                     " [default: current user]"),
                    ('group=', 'g',
                     "Group name used when creating a tar file"
                     " [default: current group]"),
                   ]

    boolean_options = ['keep-temp', 'skip-build', 'relative']

    default_format = { 'posix': 'gztar',
                       'nt': 'zip',
                       'os2': 'zip' }


    def initialize_options(self):
        self.bdist_dir = None
        self.plat_name = None
        self.format = None
        self.keep_temp = False
        self.dist_dir = None
        self.skip_build = False
        self.relative = False
        self.owner = None
        self.group = None

    def finalize_options(self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'dumb')

        if self.format is None:
            try:
                self.format = self.default_format[os.name]
            except KeyError:
                raise PackagingPlatformError(("don't know how to create dumb built distributions " +
                       "on platform %s") % os.name)

        self.set_undefined_options('bdist', 'dist_dir', 'plat_name')

    def run(self):
        if not self.skip_build:
            self.run_command('build')

        install = self.get_reinitialized_command('install_dist',
                                                 reinit_subcommands=True)
        install.root = self.bdist_dir
        install.skip_build = self.skip_build
        install.warn_dir = False

        logger.info("installing to %s", self.bdist_dir)
        self.run_command('install_dist')

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        archive_basename = "%s.%s" % (self.distribution.get_fullname(),
                                      self.plat_name)

        # OS/2 objects to any ":" characters in a filename (such as when
        # a timestamp is used in a version) so change them to hyphens.
        if os.name == "os2":
            archive_basename = archive_basename.replace(":", "-")

        pseudoinstall_root = os.path.join(self.dist_dir, archive_basename)
        if not self.relative:
            archive_root = self.bdist_dir
        else:
            if (self.distribution.has_ext_modules() and
                (install.install_base != install.install_platbase)):
                raise PackagingPlatformError(
                    "can't make a dumb built distribution where base and "
                    "platbase are different (%r, %r)" %
                    (install.install_base, install.install_platbase))
            else:
                archive_root = os.path.join(
                    self.bdist_dir,
                    self._ensure_relative(install.install_base))

        # Make the archive
        filename = self.make_archive(pseudoinstall_root,
                                     self.format, root_dir=archive_root,
                                     owner=self.owner, group=self.group)
        if self.distribution.has_ext_modules():
            pyversion = get_python_version()
        else:
            pyversion = 'any'
        self.distribution.dist_files.append(('bdist_dumb', pyversion,
                                             filename))

        if not self.keep_temp:
            if self.dry_run:
                logger.info('removing %s', self.bdist_dir)
            else:
                rmtree(self.bdist_dir)

    def _ensure_relative(self, path):
        # copied from dir_util, deleted
        drive, path = os.path.splitdrive(path)
        if path[0:1] == os.sep:
            path = drive + path[1:]
        return path
