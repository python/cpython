"""Build scripts (copy to build dir and fix up shebang line)."""

import os
import re
import sysconfig

from packaging.command.cmd import Command
from packaging.util import convert_path, newer
from packaging import logger
from packaging.compat import Mixin2to3


# check if Python is called on the first line with this expression
first_line_re = re.compile('^#!.*python[0-9.]*([ \t].*)?$')

class build_scripts(Command, Mixin2to3):

    description = "build scripts (copy and fix up shebang line)"

    user_options = [
        ('build-dir=', 'd', "directory to build (copy) to"),
        ('force', 'f', "forcibly build everything (ignore file timestamps"),
        ('executable=', 'e', "specify final destination interpreter path"),
        ]

    boolean_options = ['force']


    def initialize_options(self):
        self.build_dir = None
        self.scripts = None
        self.force = None
        self.executable = None
        self.outfiles = None
        self.use_2to3 = False
        self.convert_2to3_doctests = None
        self.use_2to3_fixers = None

    def finalize_options(self):
        self.set_undefined_options('build',
                                   ('build_scripts', 'build_dir'),
                                   'use_2to3', 'use_2to3_fixers',
                                   'convert_2to3_doctests', 'force',
                                   'executable')
        self.scripts = self.distribution.scripts

    def get_source_files(self):
        return self.scripts

    def run(self):
        if not self.scripts:
            return
        copied_files = self.copy_scripts()
        if self.use_2to3 and copied_files:
            self._run_2to3(copied_files, fixers=self.use_2to3_fixers)

    def copy_scripts(self):
        """Copy each script listed in 'self.scripts'; if it's marked as a
        Python script in the Unix way (first line matches 'first_line_re',
        ie. starts with "\#!" and contains "python"), then adjust the first
        line to refer to the current Python interpreter as we copy.
        """
        self.mkpath(self.build_dir)
        outfiles = []
        for script in self.scripts:
            adjust = False
            script = convert_path(script)
            outfile = os.path.join(self.build_dir, os.path.basename(script))
            outfiles.append(outfile)

            if not self.force and not newer(script, outfile):
                logger.debug("not copying %s (up-to-date)", script)
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
                    logger.warning('%s: %s is an empty file (skipping)',
                                   self.get_command_name(),  script)
                    continue

                match = first_line_re.match(first_line)
                if match:
                    adjust = True
                    post_interp = match.group(1) or ''

            if adjust:
                logger.info("copying and adjusting %s -> %s", script,
                         self.build_dir)
                if not self.dry_run:
                    outf = open(outfile, "w")
                    if not sysconfig.is_python_build():
                        outf.write("#!%s%s\n" %
                                   (self.executable,
                                    post_interp))
                    else:
                        outf.write("#!%s%s\n" %
                                   (os.path.join(
                            sysconfig.get_config_var("BINDIR"),
                           "python%s%s" % (sysconfig.get_config_var("VERSION"),
                                           sysconfig.get_config_var("EXE"))),
                                    post_interp))
                    outf.writelines(f.readlines())
                    outf.close()
                if f:
                    f.close()
            else:
                if f:
                    f.close()
                self.copy_file(script, outfile)

        if os.name == 'posix':
            for file in outfiles:
                if self.dry_run:
                    logger.info("changing mode of %s", file)
                else:
                    oldmode = os.stat(file).st_mode & 0o7777
                    newmode = (oldmode | 0o555) & 0o7777
                    if newmode != oldmode:
                        logger.info("changing mode of %s from %o to %o",
                                 file, oldmode, newmode)
                        os.chmod(file, newmode)
        return outfiles
