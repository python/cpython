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


class bdist (Command):

    description = "create a built (binary) distribution"

    user_options = [('bdist-base=', 'b',
                     "temporary directory for creating built distributions"),
                    ('format=', 'f',
                     "format for distribution " +
                     "(tar, ztar, gztar, bztar, zip, ... )"),
                   ]

    # The following commands do not take a format option from bdist
    no_format_option = ('bdist_rpm',)

    # This won't do in reality: will need to distinguish RPM-ish Linux,
    # Debian-ish Linux, Solaris, FreeBSD, ..., Windows, Mac OS.
    default_format = { 'posix': 'gztar',
                       'nt': 'zip', }

    format_command = { 'gztar': 'bdist_dumb',
                       'bztar': 'bdist_dumb',
                       'ztar':  'bdist_dumb',
                       'tar':   'bdist_dumb',
                       'rpm':   'bdist_rpm',
                       'zip':   'bdist_dumb', }


    def initialize_options (self):
        self.bdist_base = None
        self.format = None

    # initialize_options()


    def finalize_options (self):
        # 'bdist_base' -- parent of per-built-distribution-format
        # temporary directories (eg. we'll probably have
        # "build/bdist.<plat>/dumb", "build/bdist.<plat>/rpm", etc.)
        if self.bdist_base is None:
            build_base = self.get_finalized_command('build').build_base
            plat = get_platform()
            self.bdist_base = os.path.join (build_base, 'bdist.' + plat)

        if self.format is None:
            try:
                self.format = self.default_format[os.name]
            except KeyError:
                raise DistutilsPlatformError, \
                      "don't know how to create built distributions " + \
                      "on platform %s" % os.name
        #elif type (self.format) is StringType:
        #    self.format = string.split (self.format, ',')
            
    # finalize_options()


    def run (self):

        try:
            cmd_name = self.format_command[self.format]
        except KeyError:
            raise DistutilsOptionError, \
                  "invalid archive format '%s'" % self.format

        if cmd_name not in self.no_format_option:
            sub_cmd = self.get_finalized_command (cmd_name)
            sub_cmd.format = self.format
        self.run_command (cmd_name)

    # run()

# class bdist
