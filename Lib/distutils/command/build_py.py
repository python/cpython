"""distutils.command.build_py

Implements the Distutils 'build_py' command."""

# created 1999/03/08, Greg Ward

__rcsid__ = "$Id$"

import string, os
from distutils.core import Command
from distutils.errors import *
from distutils.util import mkpath, newer, make_file, copy_file


class BuildPy (Command):

    options = [('dir=', 'd', "directory for platform-shared files"),
              ]


    def set_default_options (self):
        self.dir = None

    def set_final_options (self):
        self.set_undefined_options ('build',
                                    ('libdir', 'dir'))


    def run (self):

        # XXX copy_file by default preserves all stat info -- mode, atime,
        # and mtime.  IMHO this is the right thing to do, but perhaps it
        # should be an option -- in particular, a site administrator might
        # want installed files to reflect the time of installation rather
        # than the last modification time before the installed release.

        # XXX copy_file does *not* preserve MacOS-specific file metadata.
        # If this is a problem for building/installing Python modules, then
        # we'll have to fix copy_file.  (And what about installing scripts,
        # when the time comes for that -- does MacOS use its special
        # metadata to know that a file is meant to be interpreted by
        # Python?)

        self.set_final_options ()

        (modules, package) = \
            self.distribution.get_options ('py_modules', 'package')
        package = package or ''

        infiles = []
        outfiles = []
        missing = []

        # Loop over the list of "pure Python" modules, deriving
        # input and output filenames and checking for missing
        # input files.

        # it's ok not to have *any* py files, right?
        if not modules:
            return
        
        # XXX we should allow for wildcards, so eg. the Distutils setup.py
        # file would just have to say
        #   py_modules = ['distutils.*', 'distutils.command.*']
        # without having to list each one explicitly.
        for m in modules:
            fn = apply (os.path.join, tuple (string.split (m, '.'))) + '.py'
            if not os.path.exists (fn):
                missing.append (fn)
            else:
                infiles.append (fn)
                outfiles.append (os.path.join (self.dir, package, fn))

        # Blow up if any input files were not found.
        if missing:
            raise DistutilsFileError, \
                  "missing files: " + string.join (missing, ' ')

        # Loop over the list of input files, copying them to their
        # temporary (build) destination.
        created = {}
        for i in range (len (infiles)):
            outdir = os.path.split (outfiles[i])[0]
            if not created.get(outdir):
                self.mkpath (outdir)
                created[outdir] = 1

            self.copy_file (infiles[i], outfiles[i])
                       
# end class BuildPy
