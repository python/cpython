"""distutils.command.bdist

Implements the Distutils 'bdist' command (create a built [binary]
distribution)."""

# created 2000/03/29, Greg Ward

__revision__ = "$Id$"

import os, string
from distutils.core import Command


class bdist (Command):

    description = "create a built (binary) distribution"

    user_options = [('formats=', 'f',
                     "formats for distribution (tar, ztar, gztar, zip, ... )"),
                   ]

    # This won't do in reality: will need to distinguish RPM-ish Linux,
    # Debian-ish Linux, Solaris, FreeBSD, ..., Windows, Mac OS.
    default_format = { 'posix': 'gztar',
                       'nt': 'zip', }

    format_command = { 'gztar': 'bdist_dumb',
                       'zip':   'bdist_dumb', }


    def initialize_options (self):
        self.formats = None

    # initialize_options()


    def finalize_options (self):
        if self.formats is None:
            try:
                self.formats = [self.default_format[os.name]]
            except KeyError:
                raise DistutilsPlatformError, \
                      "don't know how to create built distributions " + \
                      "on platform %s" % os.name
        elif type (self.formats) is StringType:
            self.formats = string.split (self.formats, ',')
            

    # finalize_options()


    def run (self):

        for format in self.formats:
            cmd_name = self.format_command[format]
            sub_cmd = self.find_peer (cmd_name)
            sub_cmd.set_option ('format', format)

            # XXX blecchhh!! should formalize this: at least a
            # 'forget_run()' (?)  method, possibly complicate the
            # 'have_run' dictionary to include some command state as well
            # as the command name -- eg. in this case we might want
            # ('bdist_dumb','zip') to be marked "have run", but not
            # ('bdist_dumb','gztar').
            self.distribution.have_run[cmd_name] = 0
            self.run_peer (cmd_name)

    # run()

# class bdist
