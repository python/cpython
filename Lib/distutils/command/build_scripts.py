"""distutils.command.build_scripts

Implements the Distutils 'build_scripts' command."""

# created 2000/05/23, Bastian Kleineidam

__revision__ = "$Id$"

import sys, os, re
from distutils.core import Command
from distutils.dep_util import newer
from distutils.util import convert_path

# check if Python is called on the first line with this expression.
# This expression will leave lines using /usr/bin/env alone; presumably
# the script author knew what they were doing...)
first_line_re = re.compile(r'^#!(?!\s*/usr/bin/env\b).*python(\s+.*)?')

class build_scripts (Command):

    description = "\"build\" scripts (copy and fixup #! line)"

    user_options = [
        ('build-dir=', 'd', "directory to \"build\" (copy) to"),
        ('force', 'f', "forcibly build everything (ignore file timestamps"),
        ]

    boolean_options = ['force']


    def initialize_options (self):
        self.build_dir = None
        self.scripts = None
        self.force = None
        self.outfiles = None

    def finalize_options (self):
        self.set_undefined_options('build',
                                   ('build_scripts', 'build_dir'),
                                   ('force', 'force'))
        self.scripts = self.distribution.scripts


    def run (self):
        if not self.scripts:
            return
        self.copy_scripts()


    def copy_scripts (self):
        """Copy each script listed in 'self.scripts'; if it's marked as a
        Python script in the Unix way (first line matches 'first_line_re',
        ie. starts with "\#!" and contains "python"), then adjust the first
        line to refer to the current Python interpreter as we copy.
        """
        outfiles = []
        self.mkpath(self.build_dir)
        for script in self.scripts:
            adjust = 0
            script = convert_path(script)
            outfile = os.path.join(self.build_dir, os.path.basename(script))

            if not self.force and not newer(script, outfile):
                self.announce("not copying %s (up-to-date)" % script)
                continue

            # Always open the file, but ignore failures in dry-run mode --
            # that way, we'll get accurate feedback if we can read the
            # script.
            try:
                f = open(script, "r")
            except IOError:
                if not self.dry_run:
                    raise
                f = None
            else:
                first_line = f.readline()
                if not first_line:
                    self.warn("%s is an empty file (skipping)" % script)
                    continue

                match = first_line_re.match(first_line)
                if match:
                    adjust = 1
                    post_interp = match.group(1)

            if adjust:
                self.announce("copying and adjusting %s -> %s" %
                              (script, self.build_dir))
                if not self.dry_run:
                    outf = open(outfile, "w")
                    outf.write("#!%s%s\n" % 
                               (os.path.normpath(sys.executable), post_interp))
                    outf.writelines(f.readlines())
                    outf.close()
                if f:
                    f.close()
            else:
                f.close()
                self.copy_file(script, outfile)

    # copy_scripts ()

# class build_scripts
