"""distutils.command.build_py

Implements the Distutils 'build_py' command."""

# created 1999/03/08, Greg Ward

__rcsid__ = "$Id$"

import string, os
from types import *
from glob import glob

from distutils.core import Command
from distutils.errors import *
from distutils.util import mkpath, copy_file


class BuildPy (Command):

    options = [('build-dir=', 'd', "directory for platform-shared files"),
              ]


    def set_default_options (self):
        self.build_dir = None
        self.modules = None
        self.package = None
        self.package_dir = None

    def set_final_options (self):
        self.set_undefined_options ('build',
                                    ('build_lib', 'build_dir'))

        # Get the distribution options that are aliases for build_py
        # options -- list of packages and list of modules.
        self.packages = self.distribution.packages
        self.modules = self.distribution.py_modules
        self.package_dir = self.distribution.package_dir


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

        infiles = []
        outfiles = []
        missing = []

        # Two options control which modules will be installed: 'packages'
        # and 'modules'.  The former lets us work with whole packages, not
        # specifying individual modules at all; the latter is for
        # specifying modules one-at-a-time.  Currently they are mutually
        # exclusive: you can define one or the other (or neither), but not
        # both.  It remains to be seen how limiting this is.

        # Dispose of the two "unusual" cases first: no pure Python modules
        # at all (no problem, just return silently), and over-specified
        # 'packages' and 'modules' options.

        if not self.modules and not self.packages:
            return
        if self.modules and self.packages:
            raise DistutilsOptionError, \
                  "build_py: supplying both 'packages' and 'modules' " + \
                  "options not allowed"

        # Now we're down to two cases: 'modules' only and 'packages' only.
        if self.modules:
            self.build_modules ()
        else:
            self.build_packages ()


    # run ()
        

    def get_package_dir (self, package):
        """Return the directory, relative to the top of the source
           distribution, where package 'package' should be found
           (at least according to the 'package_dir' option, if any)."""

        if type (package) is StringType:
            path = string.split (package, '.')
        elif type (package) in (TupleType, ListType):
            path = list (path)
        else:
            raise TypeError, "'package' must be a string, list, or tuple"

        if not self.package_dir:
            return apply (os.path.join, path)
        else:
            tail = []
            while path:
                try:
                    pdir = self.package_dir[string.join (path, '.')]
                except KeyError:
                    tail.insert (0, path[-1])
                    del path[-1]
                else:
                    tail.insert (0, pdir)
                    return apply (os.path.join, tail)
            else:
                # arg! everything failed, we might as well have not even
                # looked in package_dir -- oh well
                return apply (os.path.join, tail)

    # get_package_dir ()


    def check_package (self, package, package_dir):

        # Empty dir name means current directory, which we can probably
        # assume exists.  Also, os.path.exists and isdir don't know about
        # my "empty string means current dir" convention, so we have to
        # circumvent them.
        if package_dir != "":
            if not os.path.exists (package_dir):
                raise DistutilsFileError, \
                      "package directory '%s' does not exist" % package_dir
            if not os.path.isdir (package_dir):
                raise DistutilsFileErorr, \
                      ("supposed package directory '%s' exists, " +
                       "but is not a directory") % package_dir

        # Require __init__.py for all but the "root package"
        if package != "":
            init_py = os.path.join (package_dir, "__init__.py")
            if not os.path.isfile (init_py):
                self.warn (("package init file '%s' not found " +
                            "(or not a regular file)") % init_py)
    # check_package ()


    def check_module (self, module, module_file):
        if not os.path.isfile (module_file):
            self.warn ("file %s (for module %s) not found" %
                       module_file, module)
            return 0
        else:
            return 1

    # check_module ()


    def find_package_modules (self, package, package_dir):
        module_files = glob (os.path.join (package_dir, "*.py"))
        module_pairs = []
        for f in module_files:
            module = os.path.splitext (os.path.basename (f))[0]
            module_pairs.append (module, f)
        return module_pairs


    def find_modules (self):
        # Map package names to tuples of useful info about the package:
        #    (package_dir, checked)
        # package_dir - the directory where we'll find source files for
        #   this package
        # checked - true if we have checked that the package directory
        #   is valid (exists, contains __init__.py, ... ?)
        packages = {}

        # List of (module, package, filename) tuples to return
        modules = []

        # We treat modules-in-packages almost the same as toplevel modules,
        # just the "package" for a toplevel is empty (either an empty
        # string or empty list, depending on context).  Differences:
        #   - don't check for __init__.py in directory for empty package

        for module in self.modules:
            path = string.split (module, '.')
            package = tuple (path[0:-1])
            module_base = path[-1]

            try:
                (package_dir, checked) = packages[package]
            except KeyError:
                package_dir = self.get_package_dir (package)
                checked = 0

            if not checked:
                self.check_package (package, package_dir)
                packages[package] = (package_dir, 1)

            # XXX perhaps we should also check for just .pyc files
            # (so greedy closed-source bastards can distribute Python
            # modules too)
            module_file = os.path.join (package_dir, module_base + ".py")
            if not self.check_module (module, module_file):
                continue

            modules.append ((module, package, module_file))

        return modules

    # find_modules ()


    def get_source_files (self):

        if self.modules:
            modules = self.find_modules ()
        else:
            modules = []
            for package in self.packages:
                package_dir = self.get_package_dir (package)
                m = self.find_package_modules (package, package_dir)
                modules.extend (m)

        # Both find_modules() and find_package_modules() return a list of
        # tuples where the last element of each tuple is the filename --
        # what a happy coincidence!
        filenames = []
        for module in modules:
            filenames.append (module[-1])

        return filenames                


    def build_module (self, module, module_file, package):

        if type (package) is StringType:
            package = string.split (package, '.')

        # Now put the module source file into the "build" area -- this is
        # easy, we just copy it somewhere under self.build_dir (the build
        # directory for Python source).
        outfile_path = package
        outfile_path.append (module + ".py")
        outfile_path.insert (0, self.build_dir)
        outfile = apply (os.path.join, outfile_path)

        dir = os.path.dirname (outfile)
        self.mkpath (dir)
        self.copy_file (module_file, outfile)


    def build_modules (self):

        modules = self.find_modules()
        for (module, package, module_file) in modules:

            # Now "build" the module -- ie. copy the source file to
            # self.build_dir (the build directory for Python source).
            # (Actually, it gets copied to the directory for this package
            # under self.build_dir.)
            self.build_module (module, module_file, package)

    # build_modules ()


    def build_packages (self):

        for package in self.packages:
            package_dir = self.get_package_dir (package)
            self.check_package (package, package_dir)

            # Get list of (module, module_file) tuples based on scanning
            # the package directory.  Here, 'module' is the *unqualified*
            # module name (ie. no dots, no package -- we already know its
            # package!), and module_file is the path to the .py file,
            # relative to the current directory (ie. including
            # 'package_dir').
            modules = self.find_package_modules (package, package_dir)

            # Now loop over the modules we found, "building" each one (just
            # copy it to self.build_dir).
            for (module, module_file) in modules:
                self.build_module (module, module_file, package)

    # build_packages ()
                       
# end class BuildPy
