#! /usr/bin/env python

"""\
bundlebuilder.py -- Tools to assemble MacOS X (application) bundles.

This module contains two classes to build so called "bundles" for
MacOS X. BundleBuilder is a general tool, AppBuilder is a subclass
specialized in building application bundles.

[Bundle|App]Builder objects are instantiated with a bunch of keyword
arguments, and have a build() method that will do all the work. See
the class doc strings for a description of the constructor arguments.

The module contains a main program that can be used in two ways:

  % python bundlebuilder.py [options] build
  % python buildapp.py [options] build

Where "buildapp.py" is a user-supplied setup.py-like script following
this model:

  from bundlebuilder import buildapp
  buildapp(<lots-of-keyword-args>)

"""

#
# XXX Todo:
# - modulefinder support to build standalone apps
# - consider turning this into a distutils extension
#

__all__ = ["BundleBuilder", "AppBuilder", "buildapp"]


import sys
import os, errno, shutil
import getopt
from plistlib import Plist


plistDefaults = Plist(
	CFBundleDevelopmentRegion = "English",
	CFBundleInfoDictionaryVersion = "6.0",
)


class BundleBuilder:

	"""BundleBuilder is a barebones class for assembling bundles. It
	knows nothing about executables or icons, it only copies files
	and creates the PkgInfo and Info.plist files.

	Constructor arguments:

		name: Name of the bundle, with or without extension.
		plist: A plistlib.Plist object.
		type: The type of the bundle. Defaults to "APPL".
		creator: The creator code of the bundle. Defaults to "????".
		resources: List of files that have to be copied to
			<bundle>/Contents/Resources. Defaults to an empty list.
		files: List of (src, dest) tuples; dest should be a path relative
			to the bundle (eg. "Contents/Resources/MyStuff/SomeFile.ext.
			Defaults to an empty list.
		builddir: Directory where the bundle will be assembled. Defaults
			to "build" (in the current directory).
		symlink: Make symlinks instead copying files. This is handy during
			debugging, but makes the bundle non-distributable. Defaults to
			False.
		verbosity: verbosity level, defaults to 1
	"""

	def __init__(self, name=None, plist=None, type="APPL", creator="????",
			resources=None, files=None, builddir="build", platform="MacOS",
			symlink=0, verbosity=1):
		"""See the class doc string for a description of the arguments."""
		if plist is None:
			plist = Plist()
		if resources is None:
			resources = []
		if files is None:
			files = []
		self.name = name
		self.plist = plist
		self.type = type
		self.creator = creator
		self.resources = resources
		self.files = files
		self.builddir = builddir
		self.platform = platform
		self.symlink = symlink
		self.verbosity = verbosity

	def setup(self):
		self.name, ext = os.path.splitext(self.name)
		if not ext:
			ext = ".bundle"
		self.bundleextension = ext
		# misc (derived) attributes
		self.bundlepath = pathjoin(self.builddir, self.name + self.bundleextension)
		self.execdir = pathjoin("Contents", self.platform)

		plist = plistDefaults.copy()
		plist.CFBundleName = self.name
		plist.CFBundlePackageType = self.type
		plist.CFBundleSignature = self.creator
		plist.update(self.plist)
		self.plist = plist

	def build(self):
		"""Build the bundle."""
		builddir = self.builddir
		if builddir and not os.path.exists(builddir):
			os.mkdir(builddir)
		self.message("Building %s" % repr(self.bundlepath), 1)
		if os.path.exists(self.bundlepath):
			shutil.rmtree(self.bundlepath)
		os.mkdir(self.bundlepath)
		self.preProcess()
		self._copyFiles()
		self._addMetaFiles()
		self.postProcess()

	def preProcess(self):
		"""Hook for subclasses."""
		pass
	def postProcess(self):
		"""Hook for subclasses."""
		pass

	def _addMetaFiles(self):
		contents = pathjoin(self.bundlepath, "Contents")
		makedirs(contents)
		#
		# Write Contents/PkgInfo
		assert len(self.type) == len(self.creator) == 4, \
				"type and creator must be 4-byte strings."
		pkginfo = pathjoin(contents, "PkgInfo")
		f = open(pkginfo, "wb")
		f.write(self.type + self.creator)
		f.close()
		#
		# Write Contents/Info.plist
		infoplist = pathjoin(contents, "Info.plist")
		self.plist.write(infoplist)

	def _copyFiles(self):
		files = self.files[:]
		for path in self.resources:
			files.append((path, pathjoin("Contents", "Resources",
				os.path.basename(path))))
		if self.symlink:
			self.message("Making symbolic links", 1)
			msg = "Making symlink from"
		else:
			self.message("Copying files", 1)
			msg = "Copying"
		for src, dst in files:
			if os.path.isdir(src):
				self.message("%s %s/ to %s/" % (msg, src, dst), 2)
			else:
				self.message("%s %s to %s" % (msg, src, dst), 2)
			dst = pathjoin(self.bundlepath, dst)
			if self.symlink:
				symlink(src, dst, mkdirs=1)
			else:
				copy(src, dst, mkdirs=1)

	def message(self, msg, level=0):
		if level <= self.verbosity:
			indent = ""
			if level > 1:
				indent = (level - 1) * "  "
			sys.stderr.write(indent + msg + "\n")

	def report(self):
		# XXX something decent
		import pprint
		pprint.pprint(self.__dict__)


mainWrapperTemplate = """\
#!/usr/bin/env python

import os
from sys import argv, executable
resources = os.path.join(os.path.dirname(os.path.dirname(argv[0])),
		"Resources")
mainprogram = os.path.join(resources, "%(mainprogram)s")
assert os.path.exists(mainprogram)
argv.insert(1, mainprogram)
os.environ["PYTHONPATH"] = resources
%(setpythonhome)s
%(setexecutable)s
os.execve(executable, argv, os.environ)
"""

setExecutableTemplate = """executable = os.path.join(resources, "%s")"""
pythonhomeSnippet = """os.environ["home"] = resources"""

class AppBuilder(BundleBuilder):

	"""This class extends the BundleBuilder constructor with these
	arguments:

		mainprogram: A Python main program. If this argument is given,
			the main executable in the bundle will be a small wrapper
			that invokes the main program. (XXX Discuss why.)
		executable: The main executable. If a Python main program is
			specified the executable will be copied to Resources and
			be invoked by the wrapper program mentioned above. Else
			it will simply be used as the main executable.
		nibname: The name of the main nib, for Cocoa apps. Defaults
			to None, but must be specified when building a Cocoa app.

	For the other keyword arguments see the BundleBuilder doc string.
	"""

	def __init__(self, name=None, mainprogram=None, executable=None,
			nibname=None, **kwargs):
		"""See the class doc string for a description of the arguments."""
		self.mainprogram = mainprogram
		self.executable = executable
		self.nibname = nibname
		BundleBuilder.__init__(self, name=name, **kwargs)

	def setup(self):
		if self.mainprogram is None and self.executable is None:
			raise TypeError, ("must specify either or both of "
					"'executable' and 'mainprogram'")

		if self.name is not None:
			pass
		elif self.mainprogram is not None:
			self.name = os.path.splitext(os.path.basename(self.mainprogram))[0]
		elif executable is not None:
			self.name = os.path.splitext(os.path.basename(self.executable))[0]
		if self.name[-4:] != ".app":
			self.name += ".app"

		if self.nibname:
			self.plist.NSMainNibFile = self.nibname
			if not hasattr(self.plist, "NSPrincipalClass"):
				self.plist.NSPrincipalClass = "NSApplication"

		BundleBuilder.setup(self)

		self.plist.CFBundleExecutable = self.name

	def preProcess(self):
		resdir = pathjoin("Contents", "Resources")
		if self.executable is not None:
			if self.mainprogram is None:
				execpath = pathjoin(self.execdir, self.name)
			else:
				execpath = pathjoin(resdir, os.path.basename(self.executable))
			self.files.append((self.executable, execpath))
			# For execve wrapper
			setexecutable = setExecutableTemplate % os.path.basename(self.executable)
		else:
			setexecutable = ""  # XXX for locals() call

		if self.mainprogram is not None:
			setpythonhome = ""  # pythonhomeSnippet if we're making a standalone app
			mainname = os.path.basename(self.mainprogram)
			self.files.append((self.mainprogram, pathjoin(resdir, mainname)))
			# Create execve wrapper
			mainprogram = self.mainprogram  # XXX for locals() call
			execdir = pathjoin(self.bundlepath, self.execdir)
			mainwrapperpath = pathjoin(execdir, self.name)
			makedirs(execdir)
			open(mainwrapperpath, "w").write(mainWrapperTemplate % locals())
			os.chmod(mainwrapperpath, 0777)


def copy(src, dst, mkdirs=0):
	"""Copy a file or a directory."""
	if mkdirs:
		makedirs(os.path.dirname(dst))
	if os.path.isdir(src):
		shutil.copytree(src, dst)
	else:
		shutil.copy2(src, dst)

def copytodir(src, dstdir):
	"""Copy a file or a directory to an existing directory."""
	dst = pathjoin(dstdir, os.path.basename(src))
	copy(src, dst)

def makedirs(dir):
	"""Make all directories leading up to 'dir' including the leaf
	directory. Don't moan if any path element already exists."""
	try:
		os.makedirs(dir)
	except OSError, why:
		if why.errno != errno.EEXIST:
			raise

def symlink(src, dst, mkdirs=0):
	"""Copy a file or a directory."""
	if mkdirs:
		makedirs(os.path.dirname(dst))
	os.symlink(os.path.abspath(src), dst)

def pathjoin(*args):
	"""Safe wrapper for os.path.join: asserts that all but the first
	argument are relative paths."""
	for seg in args[1:]:
		assert seg[0] != "/"
	return os.path.join(*args)


cmdline_doc = """\
Usage:
  python bundlebuilder.py [options] command
  python mybuildscript.py [options] command

Commands:
  build      build the application
  report     print a report

Options:
  -b, --builddir=DIR     the build directory; defaults to "build"
  -n, --name=NAME        application name
  -r, --resource=FILE    extra file or folder to be copied to Resources
  -e, --executable=FILE  the executable to be used
  -m, --mainprogram=FILE the Python main program
  -p, --plist=FILE       .plist file (default: generate one)
      --nib=NAME         main nib name
  -c, --creator=CCCC     4-char creator code (default: '????')
  -l, --link             symlink files/folder instead of copying them
  -v, --verbose          increase verbosity level
  -q, --quiet            decrease verbosity level
  -h, --help             print this message
"""

def usage(msg=None):
	if msg:
		print msg
	print cmdline_doc
	sys.exit(1)

def main(builder=None):
	if builder is None:
		builder = AppBuilder(verbosity=1)

	shortopts = "b:n:r:e:m:c:plhvq"
	longopts = ("builddir=", "name=", "resource=", "executable=",
		"mainprogram=", "creator=", "nib=", "plist=", "link", "help",
		"verbose", "quiet")

	try:
		options, args = getopt.getopt(sys.argv[1:], shortopts, longopts)
	except getopt.error:
		usage()

	for opt, arg in options:
		if opt in ('-b', '--builddir'):
			builder.builddir = arg
		elif opt in ('-n', '--name'):
			builder.name = arg
		elif opt in ('-r', '--resource'):
			builder.resources.append(arg)
		elif opt in ('-e', '--executable'):
			builder.executable = arg
		elif opt in ('-m', '--mainprogram'):
			builder.mainprogram = arg
		elif opt in ('-c', '--creator'):
			builder.creator = arg
		elif opt == "--nib":
			builder.nibname = arg
		elif opt in ('-p', '--plist'):
			builder.plist = Plist.fromFile(arg)
		elif opt in ('-l', '--link'):
			builder.symlink = 1
		elif opt in ('-h', '--help'):
			usage()
		elif opt in ('-v', '--verbose'):
			builder.verbosity += 1
		elif opt in ('-q', '--quiet'):
			builder.verbosity -= 1

	if len(args) != 1:
		usage("Must specify one command ('build', 'report' or 'help')")
	command = args[0]

	if command == "build":
		builder.setup()
		builder.build()
	elif command == "report":
		builder.setup()
		builder.report()
	elif command == "help":
		usage()
	else:
		usage("Unknown command '%s'" % command)


def buildapp(**kwargs):
	builder = AppBuilder(**kwargs)
	main(builder)


if __name__ == "__main__":
	main()
