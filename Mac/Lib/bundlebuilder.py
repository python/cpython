#! /usr/bin/env python

"""\
bundlebuilder.py -- Tools to assemble MacOS X (application) bundles.

This module contains three classes to build so called "bundles" for
MacOS X. BundleBuilder is a general tool, AppBuilder is a subclass
specialized in building application bundles. CocoaAppBuilder is a
further specialization of AppBuilder.

[Bundle|App|CocoaApp]Builder objects are instantiated with a bunch
of keyword arguments, and have a build() method that will do all the
work. See the class doc strings for a description of the constructor
arguments.

"""

#
# XXX Todo:
# - a command line interface, also for use with the buildapp() and
#   buildcocoaapp() convenience functions.
# - modulefinder support to build standalone apps
#

__all__ = ["BundleBuilder", "AppBuilder", "CocoaAppBuilder",
		"buildapp", "buildcocoaapp"]


import sys
import os, errno, shutil
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

	def __init__(self, name, plist=None, type="APPL", creator="????",
			resources=None, files=None, builddir="build", platform="MacOS",
			symlink=0, verbosity=1):
		"""See the class doc string for a description of the arguments."""
		self.name, ext = os.path.splitext(name)
		if not ext:
			ext = ".bundle"
		self.bundleextension = ext
		if plist is None:
			plist = Plist()
		self.plist = plist
		self.type = type
		self.creator = creator
		if files is None:
			files = []
		if resources is None:
			resources = []
		self.resources = resources
		self.files = files
		self.builddir = builddir
		self.platform = platform
		self.symlink = symlink
		# misc (derived) attributes
		self.bundlepath = pathjoin(builddir, self.name + self.bundleextension)
		self.execdir = pathjoin("Contents", platform)
		self.resdir = pathjoin("Contents", "Resources")
		self.verbosity = verbosity

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
		plist = plistDefaults.copy()
		plist.CFBundleName = self.name
		plist.CFBundlePackageType = self.type
		plist.CFBundleSignature = self.creator
		plist.update(self.plist)
		infoplist = pathjoin(contents, "Info.plist")
		plist.write(infoplist)

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
			self.message("%s %s to %s" % (msg, src, dst), 2)
			dst = pathjoin(self.bundlepath, dst)
			if self.symlink:
				symlink(src, dst, mkdirs=1)
			else:
				copy(src, dst, mkdirs=1)

	def message(self, msg, level=0):
		if level <= self.verbosity:
			sys.stderr.write(msg + "\n")


mainWrapperTemplate = """\
#!/usr/bin/env python

import os
from sys import argv, executable
resources = os.path.join(os.path.dirname(os.path.dirname(argv[0])),
		"Resources")
mainprogram = os.path.join(resources, "%(mainprogram)s")
assert os.path.exists(mainprogram)
argv.insert(1, mainprogram)
%(executable)s
os.execve(executable, argv, os.environ)
"""

executableTemplate = "executable = os.path.join(resources, \"%s\")"


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
	
	For the other keyword arguments see the BundleBuilder doc string.
	"""

	def __init__(self, name=None, mainprogram=None, executable=None,
			**kwargs):
		"""See the class doc string for a description of the arguments."""
		if mainprogram is None and executable is None:
			raise TypeError, ("must specify either or both of "
					"'executable' and 'mainprogram'")
		if name is not None:
			pass
		elif mainprogram is not None:
			name = os.path.splitext(os.path.basename(mainprogram))[0]
		elif executable is not None:
			name = os.path.splitext(os.path.basename(executable))[0]
		if name[-4:] != ".app":
			name += ".app"

		self.mainprogram = mainprogram
		self.executable = executable

		BundleBuilder.__init__(self, name=name, **kwargs)

	def preProcess(self):
		self.plist.CFBundleExecutable = self.name
		if self.executable is not None:
			if self.mainprogram is None:
				execpath = pathjoin(self.execdir, self.name)
			else:
				execpath = pathjoin(self.resdir, os.path.basename(self.executable))
			self.files.append((self.executable, execpath))
			# For execve wrapper
			executable = executableTemplate % os.path.basename(self.executable)
		else:
			executable = ""  # XXX for locals() call

		if self.mainprogram is not None:
			mainname = os.path.basename(self.mainprogram)
			self.files.append((self.mainprogram, pathjoin(self.resdir, mainname)))
			# Create execve wrapper
			mainprogram = self.mainprogram  # XXX for locals() call
			execdir = pathjoin(self.bundlepath, self.execdir)
			mainwrapperpath = pathjoin(execdir, self.name)
			makedirs(execdir)
			open(mainwrapperpath, "w").write(mainWrapperTemplate % locals())
			os.chmod(mainwrapperpath, 0777)


class CocoaAppBuilder(AppBuilder):

	"""Tiny specialization of AppBuilder. It has an extra constructor
	argument called 'nibname' which defaults to 'MainMenu'. It will
	set the appropriate fields in the plist.
	"""

	def __init__(self, nibname="MainMenu", **kwargs):
		"""See the class doc string for a description of the arguments."""
		self.nibname = nibname
		AppBuilder.__init__(self, **kwargs)
		self.plist.NSMainNibFile = self.nibname
		if not hasattr(self.plist, "NSPrincipalClass"):
			self.plist.NSPrincipalClass = "NSApplication"


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


def buildapp(**kwargs):
	# XXX cmd line argument parsing
	builder = AppBuilder(**kwargs)
	builder.build()


def buildcocoaapp(**kwargs):
	# XXX cmd line argument parsing
	builder = CocoaAppBuilder(**kwargs)
	builder.build()


if __name__ == "__main__":
	# XXX This test is meant to be run in the Examples/TableModel/ folder
	# of the pyobj project... It will go as soon as I've written a proper
	# main program.
	buildcocoaapp(mainprogram="TableModel.py",
		resources=["English.lproj", "nibwrapper.py"], verbosity=4)
