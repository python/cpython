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


__all__ = ["BundleBuilder", "BundleBuilderError", "AppBuilder", "buildapp"]


import sys
import os, errno, shutil
import imp, marshal
import re
from copy import deepcopy
import getopt
from plistlib import Plist
from types import FunctionType as function


class BundleBuilderError(Exception): pass


class Defaults:

	"""Class attributes that don't start with an underscore and are
	not functions or classmethods are (deep)copied to self.__dict__.
	This allows for mutable default values.
	"""

	def __init__(self, **kwargs):
		defaults = self._getDefaults()
		defaults.update(kwargs)
		self.__dict__.update(defaults)

	def _getDefaults(cls):
		defaults = {}
		for name, value in cls.__dict__.items():
			if name[0] != "_" and not isinstance(value,
					(function, classmethod)):
				defaults[name] = deepcopy(value)
		for base in cls.__bases__:
			if hasattr(base, "_getDefaults"):
				defaults.update(base._getDefaults())
		return defaults
	_getDefaults = classmethod(_getDefaults)


class BundleBuilder(Defaults):

	"""BundleBuilder is a barebones class for assembling bundles. It
	knows nothing about executables or icons, it only copies files
	and creates the PkgInfo and Info.plist files.
	"""

	# (Note that Defaults.__init__ (deep)copies these values to
	# instance variables. Mutable defaults are therefore safe.)

	# Name of the bundle, with or without extension.
	name = None

	# The property list ("plist")
	plist = Plist(CFBundleDevelopmentRegion = "English",
	              CFBundleInfoDictionaryVersion = "6.0")

	# The type of the bundle.
	type = "APPL"
	# The creator code of the bundle.
	creator = None

	# List of files that have to be copied to <bundle>/Contents/Resources.
	resources = []

	# List of (src, dest) tuples; dest should be a path relative to the bundle
	# (eg. "Contents/Resources/MyStuff/SomeFile.ext).
	files = []

	# Directory where the bundle will be assembled.
	builddir = "build"

	# platform, name of the subfolder of Contents that contains the executable.
	platform = "MacOS"

	# Make symlinks instead copying files. This is handy during debugging, but
	# makes the bundle non-distributable.
	symlink = 0

	# Verbosity level.
	verbosity = 1

	def setup(self):
		# XXX rethink self.name munging, this is brittle.
		self.name, ext = os.path.splitext(self.name)
		if not ext:
			ext = ".bundle"
		bundleextension = ext
		# misc (derived) attributes
		self.bundlepath = pathjoin(self.builddir, self.name + bundleextension)
		self.execdir = pathjoin("Contents", self.platform)

		plist = self.plist
		plist.CFBundleName = self.name
		plist.CFBundlePackageType = self.type
		if self.creator is None:
			if hasattr(plist, "CFBundleSignature"):
				self.creator = plist.CFBundleSignature
			else:
				self.creator = "????"
		plist.CFBundleSignature = self.creator
		if not hasattr(plist, "CFBundleIdentifier"):
			plist.CFBundleIdentifier = self.name

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
		self.message("Done.", 1)

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
		files.sort()
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
		pass


if __debug__:
	PYC_EXT = ".pyc"
else:
	PYC_EXT = ".pyo"

MAGIC = imp.get_magic()
USE_ZIPIMPORT = "zipimport" in sys.builtin_module_names

# For standalone apps, we have our own minimal site.py. We don't need
# all the cruft of the real site.py.
SITE_PY = """\
import sys
del sys.path[1:]  # sys.path[0] is Contents/Resources/
"""

if USE_ZIPIMPORT:
	ZIP_ARCHIVE = "Modules.zip"
	SITE_PY += "sys.path.append(sys.path[0] + '/%s')\n" % ZIP_ARCHIVE
	def getPycData(fullname, code, ispkg):
		if ispkg:
			fullname += ".__init__"
		path = fullname.replace(".", os.sep) + PYC_EXT
		return path, MAGIC + '\0\0\0\0' + marshal.dumps(code)

SITE_CO = compile(SITE_PY, "<-bundlebuilder.py->", "exec")

EXT_LOADER = """\
def __load():
	import imp, sys, os
	for p in sys.path:
		path = os.path.join(p, "%(filename)s")
		if os.path.exists(path):
			break
	else:
		assert 0, "file not found: %(filename)s"
	mod = imp.load_dynamic("%(name)s", path)

__load()
del __load
"""

MAYMISS_MODULES = ['mac', 'os2', 'nt', 'ntpath', 'dos', 'dospath',
	'win32api', 'ce', '_winreg', 'nturl2path', 'sitecustomize',
	'org.python.core', 'riscos', 'riscosenviron', 'riscospath'
]

STRIP_EXEC = "/usr/bin/strip"

BOOTSTRAP_SCRIPT = """\
#!/bin/sh

execdir=$(dirname "${0}")
executable=${execdir}/%(executable)s
resdir=$(dirname "${execdir}")/Resources
main=${resdir}/%(mainprogram)s
PYTHONPATH=$resdir
export PYTHONPATH
exec "${executable}" "${main}" "${1}"
"""


class AppBuilder(BundleBuilder):

	# A Python main program. If this argument is given, the main
	# executable in the bundle will be a small wrapper that invokes
	# the main program. (XXX Discuss why.)
	mainprogram = None

	# The main executable. If a Python main program is specified
	# the executable will be copied to Resources and be invoked
	# by the wrapper program mentioned above. Otherwise it will
	# simply be used as the main executable.
	executable = None

	# The name of the main nib, for Cocoa apps. *Must* be specified
	# when building a Cocoa app.
	nibname = None

	# The name of the icon file to be copied to Resources and used for
	# the Finder icon.
	iconfile = None

	# Symlink the executable instead of copying it.
	symlink_exec = 0

	# If True, build standalone app.
	standalone = 0

	# The following attributes are only used when building a standalone app.

	# Exclude these modules.
	excludeModules = []

	# Include these modules.
	includeModules = []

	# Include these packages.
	includePackages = []

	# Strip binaries.
	strip = 0

	# Found Python modules: [(name, codeobject, ispkg), ...]
	pymodules = []

	# Modules that modulefinder couldn't find:
	missingModules = []
	maybeMissingModules = []

	# List of all binaries (executables or shared libs), for stripping purposes
	binaries = []

	def setup(self):
		if self.standalone and self.mainprogram is None:
			raise BundleBuilderError, ("must specify 'mainprogram' when "
					"building a standalone application.")
		if self.mainprogram is None and self.executable is None:
			raise BundleBuilderError, ("must specify either or both of "
					"'executable' and 'mainprogram'")

		if self.name is not None:
			pass
		elif self.mainprogram is not None:
			self.name = os.path.splitext(os.path.basename(self.mainprogram))[0]
		elif executable is not None:
			self.name = os.path.splitext(os.path.basename(self.executable))[0]
		if self.name[-4:] != ".app":
			self.name += ".app"

		if self.executable is None:
			if not self.standalone:
				self.symlink_exec = 1
			self.executable = sys.executable

		if self.nibname:
			self.plist.NSMainNibFile = self.nibname
			if not hasattr(self.plist, "NSPrincipalClass"):
				self.plist.NSPrincipalClass = "NSApplication"

		BundleBuilder.setup(self)

		self.plist.CFBundleExecutable = self.name

		if self.standalone:
			self.findDependencies()

	def preProcess(self):
		resdir = "Contents/Resources"
		if self.executable is not None:
			if self.mainprogram is None:
				execname = self.name
			else:
				execname = os.path.basename(self.executable)
			execpath = pathjoin(self.execdir, execname)
			if not self.symlink_exec:
				self.files.append((self.executable, execpath))
				self.binaries.append(execpath)
			self.execpath = execpath

		if self.mainprogram is not None:
			mainprogram = os.path.basename(self.mainprogram)
			self.files.append((self.mainprogram, pathjoin(resdir, mainprogram)))
			# Write bootstrap script
			executable = os.path.basename(self.executable)
			execdir = pathjoin(self.bundlepath, self.execdir)
			bootstrappath = pathjoin(execdir, self.name)
			makedirs(execdir)
			open(bootstrappath, "w").write(BOOTSTRAP_SCRIPT % locals())
			os.chmod(bootstrappath, 0775)

		if self.iconfile is not None:
			iconbase = os.path.basename(self.iconfile)
			self.plist.CFBundleIconFile = iconbase
			self.files.append((self.iconfile, pathjoin(resdir, iconbase)))

	def postProcess(self):
		if self.standalone:
			self.addPythonModules()
		if self.strip and not self.symlink:
			self.stripBinaries()

		if self.symlink_exec and self.executable:
			self.message("Symlinking executable %s to %s" % (self.executable,
					self.execpath), 2)
			dst = pathjoin(self.bundlepath, self.execpath)
			makedirs(os.path.dirname(dst))
			os.symlink(os.path.abspath(self.executable), dst)

		if self.missingModules or self.maybeMissingModules:
			self.reportMissing()

	def addPythonModules(self):
		self.message("Adding Python modules", 1)

		if USE_ZIPIMPORT:
			# Create a zip file containing all modules as pyc.
			import zipfile
			relpath = pathjoin("Contents", "Resources", ZIP_ARCHIVE)
			abspath = pathjoin(self.bundlepath, relpath)
			zf = zipfile.ZipFile(abspath, "w", zipfile.ZIP_DEFLATED)
			for name, code, ispkg in self.pymodules:
				self.message("Adding Python module %s" % name, 2)
				path, pyc = getPycData(name, code, ispkg)
				zf.writestr(path, pyc)
			zf.close()
			# add site.pyc
			sitepath = pathjoin(self.bundlepath, "Contents", "Resources",
					"site" + PYC_EXT)
			writePyc(SITE_CO, sitepath)
		else:
			# Create individual .pyc files.
			for name, code, ispkg in self.pymodules:
				if ispkg:
					name += ".__init__"
				path = name.split(".")
				path = pathjoin("Contents", "Resources", *path) + PYC_EXT

				if ispkg:
					self.message("Adding Python package %s" % path, 2)
				else:
					self.message("Adding Python module %s" % path, 2)

				abspath = pathjoin(self.bundlepath, path)
				makedirs(os.path.dirname(abspath))
				writePyc(code, abspath)

	def stripBinaries(self):
		if not os.path.exists(STRIP_EXEC):
			self.message("Error: can't strip binaries: no strip program at "
				"%s" % STRIP_EXEC, 0)
		else:
			self.message("Stripping binaries", 1)
			for relpath in self.binaries:
				self.message("Stripping %s" % relpath, 2)
				abspath = pathjoin(self.bundlepath, relpath)
				assert not os.path.islink(abspath)
				rv = os.system("%s -S \"%s\"" % (STRIP_EXEC, abspath))

	def findDependencies(self):
		self.message("Finding module dependencies", 1)
		import modulefinder
		mf = modulefinder.ModuleFinder(excludes=self.excludeModules)
		if USE_ZIPIMPORT:
			# zipimport imports zlib, must add it manually
			mf.import_hook("zlib")
		# manually add our own site.py
		site = mf.add_module("site")
		site.__code__ = SITE_CO
		mf.scan_code(SITE_CO, site)

		includeModules = self.includeModules[:]
		for name in self.includePackages:
			includeModules.extend(findPackageContents(name).keys())
		for name in includeModules:
			try:
				mf.import_hook(name)
			except ImportError:
				self.missingModules.append(name)

		mf.run_script(self.mainprogram)
		modules = mf.modules.items()
		modules.sort()
		for name, mod in modules:
			if mod.__file__ and mod.__code__ is None:
				# C extension
				path = mod.__file__
				filename = os.path.basename(path)
				if USE_ZIPIMPORT:
					# Python modules are stored in a Zip archive, but put
					# extensions in Contents/Resources/.a and add a tiny "loader"
					# program in the Zip archive. Due to Thomas Heller.
					dstpath = pathjoin("Contents", "Resources", filename)
					source = EXT_LOADER % {"name": name, "filename": filename}
					code = compile(source, "<dynloader for %s>" % name, "exec")
					mod.__code__ = code
				else:
					# just copy the file
					dstpath = name.split(".")[:-1] + [filename]
					dstpath = pathjoin("Contents", "Resources", *dstpath)
				self.files.append((path, dstpath))
				self.binaries.append(dstpath)
			if mod.__code__ is not None:
				ispkg = mod.__path__ is not None
				if not USE_ZIPIMPORT or name != "site":
					# Our site.py is doing the bootstrapping, so we must
					# include a real .pyc file if USE_ZIPIMPORT is True.
					self.pymodules.append((name, mod.__code__, ispkg))

		if hasattr(mf, "any_missing_maybe"):
			missing, maybe = mf.any_missing_maybe()
		else:
			missing = mf.any_missing()
			maybe = []
		self.missingModules.extend(missing)
		self.maybeMissingModules.extend(maybe)

	def reportMissing(self):
		missing = [name for name in self.missingModules
				if name not in MAYMISS_MODULES]
		if self.maybeMissingModules:
			maybe = self.maybeMissingModules
		else:
			maybe = [name for name in missing if "." in name]
			missing = [name for name in missing if "." not in name]
		missing.sort()
		maybe.sort()
		if maybe:
			self.message("Warning: couldn't find the following submodules:", 1)
			self.message("    (Note that these could be false alarms -- "
			             "it's not always", 1)
			self.message("    possible to distinguish between \"from package "
			             "import submodule\" ", 1)
			self.message("    and \"from package import name\")", 1)
			for name in maybe:
				self.message("  ? " + name, 1)
		if missing:
			self.message("Warning: couldn't find the following modules:", 1)
			for name in missing:
				self.message("  ? " + name, 1)

	def report(self):
		# XXX something decent
		import pprint
		pprint.pprint(self.__dict__)
		if self.standalone:
			self.reportMissing()

#
# Utilities.
#

SUFFIXES = [_suf for _suf, _mode, _tp in imp.get_suffixes()]
identifierRE = re.compile(r"[_a-zA-z][_a-zA-Z0-9]*$")

def findPackageContents(name, searchpath=None):
	head = name.split(".")[-1]
	if identifierRE.match(head) is None:
		return {}
	try:
		fp, path, (ext, mode, tp) = imp.find_module(head, searchpath)
	except ImportError:
		return {}
	modules = {name: None}
	if tp == imp.PKG_DIRECTORY and path:
		files = os.listdir(path)
		for sub in files:
			sub, ext = os.path.splitext(sub)
			fullname = name + "." + sub
			if sub != "__init__" and fullname not in modules:
				modules.update(findPackageContents(fullname, [path]))
	return modules

def writePyc(code, path):
	f = open(path, "wb")
	f.write(MAGIC)
	f.write("\0" * 4)  # don't bother about a time stamp
	marshal.dump(code, f)
	f.close()

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
	if not os.path.exists(src):
		raise IOError, "No such file or directory: '%s'" % src
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
      --iconfile=FILE    filename of the icon (an .icns file) to be used
                         as the Finder icon
  -l, --link             symlink files/folder instead of copying them
      --link-exec        symlink the executable instead of copying it
      --standalone       build a standalone application, which is fully
                         independent of a Python installation
  -x, --exclude=MODULE   exclude module (with --standalone)
  -i, --include=MODULE   include module (with --standalone)
      --package=PACKAGE  include a whole package (with --standalone)
      --strip            strip binaries (remove debug info)
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

	shortopts = "b:n:r:e:m:c:p:lx:i:hvq"
	longopts = ("builddir=", "name=", "resource=", "executable=",
		"mainprogram=", "creator=", "nib=", "plist=", "link",
		"link-exec", "help", "verbose", "quiet", "standalone",
		"exclude=", "include=", "package=", "strip", "iconfile=")

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
		elif opt == '--iconfile':
			builder.iconfile = arg
		elif opt == "--nib":
			builder.nibname = arg
		elif opt in ('-p', '--plist'):
			builder.plist = Plist.fromFile(arg)
		elif opt in ('-l', '--link'):
			builder.symlink = 1
		elif opt == '--link-exec':
			builder.symlink_exec = 1
		elif opt in ('-h', '--help'):
			usage()
		elif opt in ('-v', '--verbose'):
			builder.verbosity += 1
		elif opt in ('-q', '--quiet'):
			builder.verbosity -= 1
		elif opt == '--standalone':
			builder.standalone = 1
		elif opt in ('-x', '--exclude'):
			builder.excludeModules.append(arg)
		elif opt in ('-i', '--include'):
			builder.includeModules.append(arg)
		elif opt == '--package':
			builder.includePackages.append(arg)
		elif opt == '--strip':
			builder.strip = 1

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
