"""Create a built (binary) distribution.

If a --formats option was given on the command line, this command will
call the corresponding bdist_* commands; if the option was absent, a
bdist_* command depending on the current platform will be called.
"""

import os

from packaging import util
from packaging.command.cmd import Command
from packaging.errors import PackagingPlatformError, PackagingOptionError


def show_formats():
    """Print list of available formats (arguments to "--format" option).
    """
    from packaging.fancy_getopt import FancyGetopt
    formats = []
    for format in bdist.format_commands:
        formats.append(("formats=" + format, None,
                        bdist.format_command[format][1]))
    pretty_printer = FancyGetopt(formats)
    pretty_printer.print_help("List of available distribution formats:")


class bdist(Command):

    description = "create a built (binary) distribution"

    user_options = [('bdist-base=', 'b',
                     "temporary directory for creating built distributions"),
                    ('plat-name=', 'p',
                     "platform name to embed in generated filenames "
                     "(default: %s)" % util.get_platform()),
                    ('formats=', None,
                     "formats for distribution (comma-separated list)"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in "
                     "[default: dist]"),
                    ('skip-build', None,
                     "skip rebuilding everything (for testing/debugging)"),
                    ('owner=', 'u',
                     "Owner name used when creating a tar file"
                     " [default: current user]"),
                    ('group=', 'g',
                     "Group name used when creating a tar file"
                     " [default: current group]"),
                   ]

    boolean_options = ['skip-build']

    help_options = [
        ('help-formats', None,
         "lists available distribution formats", show_formats),
        ]

    # This is of course very simplistic.  The various UNIX family operating
    # systems have their specific formats, but they are out of scope for us;
    # bdist_dumb is, well, dumb; it's more a building block for other
    # packaging tools than a real end-user binary format.
    default_format = {'posix': 'gztar',
                      'nt': 'zip',
                      'os2': 'zip'}

    # Establish the preferred order (for the --help-formats option).
    format_commands = ['gztar', 'bztar', 'ztar', 'tar',
                       'wininst', 'zip', 'msi']

    # And the real information.
    format_command = {'gztar': ('bdist_dumb', "gzip'ed tar file"),
                      'bztar': ('bdist_dumb', "bzip2'ed tar file"),
                      'ztar':  ('bdist_dumb', "compressed tar file"),
                      'tar':   ('bdist_dumb', "tar file"),
                      'wininst': ('bdist_wininst',
                                  "Windows executable installer"),
                      'zip':   ('bdist_dumb', "ZIP file"),
                      'msi':   ('bdist_msi',  "Microsoft Installer")
                      }


    def initialize_options(self):
        self.bdist_base = None
        self.plat_name = None
        self.formats = None
        self.dist_dir = None
        self.skip_build = False
        self.group = None
        self.owner = None

    def finalize_options(self):
        # have to finalize 'plat_name' before 'bdist_base'
        if self.plat_name is None:
            if self.skip_build:
                self.plat_name = util.get_platform()
            else:
                self.plat_name = self.get_finalized_command('build').plat_name

        # 'bdist_base' -- parent of per-built-distribution-format
        # temporary directories (eg. we'll probably have
        # "build/bdist.<plat>/dumb", etc.)
        if self.bdist_base is None:
            build_base = self.get_finalized_command('build').build_base
            self.bdist_base = os.path.join(build_base,
                                           'bdist.' + self.plat_name)

        self.ensure_string_list('formats')
        if self.formats is None:
            try:
                self.formats = [self.default_format[os.name]]
            except KeyError:
                raise PackagingPlatformError("don't know how to create built distributions " + \
                      "on platform %s" % os.name)

        if self.dist_dir is None:
            self.dist_dir = "dist"

    def run(self):
        # Figure out which sub-commands we need to run.
        commands = []
        for format in self.formats:
            try:
                commands.append(self.format_command[format][0])
            except KeyError:
                raise PackagingOptionError("invalid format '%s'" % format)

        # Reinitialize and run each command.
        for i in range(len(self.formats)):
            cmd_name = commands[i]
            sub_cmd = self.get_reinitialized_command(cmd_name)

            # passing the owner and group names for tar archiving
            if cmd_name == 'bdist_dumb':
                sub_cmd.owner = self.owner
                sub_cmd.group = self.group

            # If we're going to need to run this command again, tell it to
            # keep its temporary files around so subsequent runs go faster.
            if cmd_name in commands[i+1:]:
                sub_cmd.keep_temp = True
            self.run_command(cmd_name)
