"""distutils.command.build_scripts

Implements the Distutils 'build_scripts' command."""

# created 2000/05/23, Bastian Kleineidam

__revision__ = "$Id$"

import sys,os,re
from distutils.core import Command

# check if Python is called on the first line with this expression
first_line_re = re.compile(r"^#!.+python(\s-\w+)*")

class build_scripts (Command):

    description = "\"build\" scripts"

    user_options = [
        ('build-dir=', 'd', "directory to \"build\" (copy) to"),
        ('force', 'f', "forcibly build everything (ignore file timestamps"),
        ]


    def initialize_options (self):
        self.build_dir = None
        self.scripts = None
        self.force = None
        self.outfiles = None

    def finalize_options (self):
        self.set_undefined_options ('build',
                                    ('build_scripts', 'build_dir'),
                                    ('force', 'force'))
        self.scripts = self.distribution.scripts


    def run (self):
        if not self.scripts:
            return
        self._copy_files()
        self._adjust_files()
        
    def _copy_files(self):
        """Copy all the scripts to the build dir"""
        self.outfiles = []
        self.mkpath(self.build_dir)
        for f in self.scripts:
            print self.build_dir
            if self.copy_file(f, self.build_dir):
                self.outfiles.append(os.path.join(self.build_dir, f))
            
    def _adjust_files(self):
        """If the first line begins with #! and ends with python
	   replace it with the current python interpreter"""
        for f in self.outfiles:
            if not self.dry_run:
                data = open(f, "r").readlines()
                if not data:
                    self.warn("%s is an empty file!" % f)
                    continue
                mo = first_line_re.match(data[0])
                if mo:
                    self.announce("Adjusting first line of file %s" % f)
                    data[0] = "#!"+sys.executable
                    # add optional command line options
                    if mo.group(1):
                        data[0] = data[0] + mo.group(1)
                    else:
                        data[0] = data[0] + "\n"
                    open(f, "w").writelines(data)
