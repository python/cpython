import mkcwproject
import sys
import os

PROJECTDIR = os.path.join(sys.prefix, ":Mac:Build")
MODULEDIRS = [	# Relative to projectdirs
	"::Modules:%s",
	"::Modules",
	":::Modules",
]

def genpluginproject(module,
		project=None, projectdir=None,
		sources=[], sourcedirs=[],
		libraries=[], extradirs=[]):
	if not project:
		project = module + '.mcp'
	if not projectdir:
		projectdir = PROJECTDIR
	if not sources:
		sources = [module + 'module.c']
	if not sourcedirs:
		for moduledir in MODULEDIRS:
			if '%' in moduledir:
				moduledir = moduledir % module
			fn = os.path.join(projectdir, os.path.join(moduledir, sources[0]))
			if os.path.exists(fn):
				sourcedirs = [moduledir]
				break
		else:
			print "Warning: %s: sourcefile not found: %s"%(module, sources[0])
			sourcedirs = []
	dict = {
		"sysprefix" : sys.prefix,
		"sources" : sources,
		"extrasearchdirs" : sourcedirs + extradirs,
		"libraries": libraries,
	}
	mkcwproject.mkproject(os.path.join(projectdir, project), module, dict)
	
genpluginproject("Cm", libraries=["QuickTimeLib"])
genpluginproject("calldll", sources=["calldll.c"])
genpluginproject("zlib", libraries=["zlib.ppc.Lib"], extradirs=["::::imglibs:zlib"])
