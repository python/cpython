"""distutils.command.bdist

Implements the Distutils 'bdist' command (create a built [binary]
distribution)."""

# created 2000/03/29, Greg Ward

__revision__ = "$Id$"

import os, string
from types import *
from distutils.core import Command
from distutils.errors import *
from distutils.util import get_platform


def show_formats ():
    """Print list of available formats (arguments to "--format" option).
    """
    from distutils.fancy_getopt import FancyGetopt 
    formats=[]
    for format in bdist.format_commands:
        formats.append(("formats=" + format, None,
                        bdist.format_command[format][1]))
    pretty_printer = FancyGetopt(formats)
    pretty_printer.print_help("List of available distribution formats:")


class bdist (Command):

    description = "create a built (binary) distribution"

    user_options = [('bdist-base=', 'b',
                     "temporary directory for creating built distributions"),
                    ('formats=', None,
                     "formats for distribution (comma-separated list)"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in "
                     "[default: dist]"),
                   ]

    help_options = [
        ('help-formats', None,
         "lists available distribution formats", show_formats),
	]

    # The following commands do not take a format option from bdist
    no_format_option = ('bdist_rpm',)

    # This won't do in reality: will need to distinguish RPM-ish Linux,
    # Debian-ish Linux, Solaris, FreeBSD, ..., Windows, Mac OS.
    default_format = { 'posix': 'gztar',
                       'nt': 'zip', }

    format_command = { 'rpm':   ('bdist_rpm',  "RPM distribution"),
                       'gztar': ('bdist_dumb', "gzip'ed tar file"),
                       'bztar': ('bdist_dumb', "bzip2'ed tar file"),
                       'ztar':  ('bdist_dumb', "compressed tar file"),
                       'tar':   ('bdist_dumb', "tar file"),
                       'zip':   ('bdist_dumb', "ZIP file"),
                       'wininst': ('bdist_wininst',
                                   "Windows executable installer"),
                     }
    # establish the preferred order
    format_commands = ['rpm', 'gztar', 'bztar', 'ztar', 'tar', 'zip']


    def initialize_options (self):
        self.bdist_base = None
        self.formats = None
        self.dist_dir = None

    # initialize_options()


    def finalize_options (self):
        # 'bdist_base' -- parent of per-built-distribution-format
        # temporary directories (eg. we'll probably have
        # "build/bdist.<plat>/dumb", "build/bdist.<plat>/rpm", etc.)
        if self.bdist_base is None:
            build_base = self.get_finalized_command('build').build_base
            plat = get_platform()
            self.bdist_base = os.path.join (build_base, 'bdist.' + plat)

        self.ensure_string_list('formats')
        if self.formats is None:
            try:
                self.formats = [self.default_format[os.name]]
            except KeyError:
                raise DistutilsPlatformError, \
                      "don't know how to create built distributions " + \
                      "on platform %s" % os.name

        if self.dist_dir is None:
            self.dist_dir = "dist"
            
    # finalize_options()


    def run (self):

        for format in self.formats:
            try:
                cmd_name = self.format_command[format][0]
            except KeyError:
                raise DistutilsOptionError, \
                      "invalid format '%s'" % format

            sub_cmd = self.reinitialize_command(cmd_name)
            if cmd_name not in self.no_format_option:
                sub_cmd.format = format
            self.run_command (cmd_name)

    # run()

# class bdist
