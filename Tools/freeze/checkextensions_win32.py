"""Extension management for Windows.

Under Windows it is unlikely the .obj files are of use, as special compiler options
are needed (primarily to toggle the behaviour of "public" symbols.

I dont consider it worth parsing the MSVC makefiles for compiler options.  Even if
we get it just right, a specific freeze application may have specific compiler
options anyway (eg, to enable or disable specific functionality)

So my basic stragtegy is:

* Have a Windows INI file which "describes" an extension module.
* This description can include:
  - The MSVC .dsp file for the extension.  The .c source file names
    are extraced from there.
  - Specific compiler options
  - Flag to indicate if Unicode compilation is expected.

At the moment the name and location of this INI file is hardcoded,
but an obvious enhancement would be to provide command line options.
"""

import os, string, sys
try:
	import win32api
except ImportError:
	win32api = None # User has already been warned

class CExtension:
	"""An abstraction of an extension implemented in C/C++
	"""
	def __init__(self, name, sourceFiles):
		self.name = name
		# A list of strings defining additional compiler options.
		self.sourceFiles = sourceFiles
		# A list of special compiler options to be applied to
		# all source modules in this extension.
		self.compilerOptions = []
		# A list of .lib files the final .EXE will need.
		self.linkerLibs = []

	def GetSourceFiles(self):
		return self.sourceFiles

	def AddCompilerOption(self, option):
		self.compilerOptions.append(option)
	def GetCompilerOptions(self):
		return self.compilerOptions

	def AddLinkerLib(self, lib):
		self.linkerLibs.append(lib)
	def GetLinkerLibs(self):
		return self.linkerLibs

def checkextensions(unknown, ignored):
        # Create a table of frozen extensions

	mapFileName = os.path.join( os.path.split(sys.argv[0])[0], "extensions_win32.ini")
	ret = []
	for mod in unknown:
		defn = get_extension_defn( mod, mapFileName )
		if defn is not None:
			ret.append( defn )
	return ret

def get_extension_defn(moduleName, mapFileName):
	if win32api is None: return None
	dsp = win32api.GetProfileVal(moduleName, "dsp", "", mapFileName)
	if dsp=="":
		sys.stderr.write("No definition of module %s in map file '%s'\n" % (moduleName, mapFileName))
		return None

	# We allow environment variables in the file name
	dsp = win32api.ExpandEnvironmentStrings(dsp)
	sourceFiles = parse_dsp(dsp)
	if sourceFiles is None:
		return None

	module = CExtension(moduleName, sourceFiles)

	cl_options = win32api.GetProfileVal(moduleName, "cl", "", mapFileName)
	if cl_options:
		module.AddCompilerOption(win32api.ExpandEnvironmentStrings(cl_options))

	exclude = win32api.GetProfileVal(moduleName, "exclude", "", mapFileName)
	exclude = string.split(exclude)

	if win32api.GetProfileVal(moduleName, "Unicode", 0, mapFileName):
		module.AddCompilerOption('/D UNICODE /D _UNICODE')

	libs = string.split(win32api.GetProfileVal(moduleName, "libs", "", mapFileName))
	for lib in libs:
		module.AddLinkerLib(lib)

	for exc in exclude:
		if exc in module.sourceFiles:
			modules.sourceFiles.remove(exc)

	return module	

# Given an MSVC DSP file, locate C source files it uses
# returns a list of source files.
def parse_dsp(dsp):
#	print "Processing", dsp
	# For now, only support 
	ret = []
	dsp_path, dsp_name = os.path.split(dsp)
	try:
		lines = open(dsp, "r").readlines()
	except IOError, msg:
		sys.stderr.write("%s: %s\n" % (dsp, msg))
		return None
	for line in lines:
		fields = string.split(string.strip(line), "=", 2)
		if fields[0]=="SOURCE":
			if string.lower(os.path.splitext(fields[1])[1]) in ['.cpp', '.c']:
				ret.append( win32api.GetFullPathName(os.path.join(dsp_path, fields[1] ) ) )
	return ret

def write_extension_table(fname, modules):
	fp = open(fname, "w")
	try:
		fp.write (ext_src_header)
		# Write fn protos
		for module in modules:
			# bit of a hack for .pyd's as part of packages.
			name = string.split(module.name,'.')[-1]
			fp.write('extern void init%s(void);\n' % (name) )
		# Write the table
		fp.write (ext_tab_header)
		for module in modules:
			name = string.split(module.name,'.')[-1]
			fp.write('\t{"%s", init%s},\n' % (name, name) )

		fp.write (ext_tab_footer)
		fp.write(ext_src_footer)
	finally:
		fp.close()


ext_src_header = """\
#include "Python.h"
"""

ext_tab_header = """\

static struct _inittab extensions[] = {
"""

ext_tab_footer = """\
        /* Sentinel */
        {0, 0}
};
"""

ext_src_footer = """\
extern int PyImport_ExtendInittab(struct _inittab *newtab);

int PyInitFrozenExtensions()
{
	return PyImport_ExtendInittab(extensions);
}

"""


