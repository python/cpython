import mkcwproject
import sys
import os
import string

PROJECTDIR = os.path.join(sys.prefix, ":Mac:Build")
MODULEDIRS = [	# Relative to projectdirs
	"::Modules:%s",
	"::Modules",
	":::Modules",
]

def relpath(base, path):
	"""Turn abs path into path relative to another. Only works for 2 abs paths
	both pointing to folders"""
	if not os.path.isabs(base) or not os.path.isabs(path):
		raise 'Absolute paths only'
	if base[-1] != ':':
		base = base +':'
	if path[-1] != ':':
		path = path + ':'
	basefields = string.split(base, os.sep)
	pathfields = string.split(path, os.sep)
	commonfields = len(os.path.commonprefix((basefields, pathfields)))
	basefields = basefields[commonfields:]
	pathfields = pathfields[commonfields:]
	pathfields = ['']*len(basefields) + pathfields
	return string.join(pathfields, os.sep)

def genpluginproject(module,
		project=None, projectdir=None,
		sources=[], sourcedirs=[],
		libraries=[], extradirs=[],
		extraexportsymbols=[]):
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
				moduledir, sourcefile = os.path.split(fn)
				sourcedirs = [moduledir]
				sources[0] = sourcefile
				break
		else:
			print "Warning: %s: sourcefile not found: %s"%(module, sources[0])
			sourcedirs = []
	dict = {
		"sysprefix" : sys.prefix,
		"sources" : sources,
		"extrasearchdirs" : sourcedirs + extradirs,
		"libraries": libraries,
		"mac_outputdir" : "::Plugins",
		"extraexportsymbols" : extraexportsymbols,
	}
	mkcwproject.mkproject(os.path.join(projectdir, project), module, dict)

def	genallprojects():
	# Standard Python modules
	genpluginproject("ucnhash", sources=["ucnhash.c"])
	genpluginproject("pyexpat", 
		sources=["pyexpat.c"], 
		libraries=["libexpat.ppc.lib"], 
		extradirs=["::::expat:*"])
	genpluginproject("zlib", 
		libraries=["zlib.ppc.Lib"], 
		extradirs=["::::imglibs:zlib:mac", "::::imglibs:zlib"])
	genpluginproject("gdbm", 
		libraries=["gdbm.ppc.gusi.lib"], 
		extradirs=["::::gdbm:mac", "::::gdbm"])
	
	# bgen-generated Toolbox modules
	genpluginproject("App", libraries=["AppearanceLib"])
	genpluginproject("Cm",
		libraries=["QuickTimeLib"],
		extraexportsymbols=[
			"CmpObj_New",
			"CmpObj_Convert",
			"CmpInstObj_New",
			"CmpInstObj_Convert",
		])
	genpluginproject("Fm")
	genpluginproject("Help")
	genpluginproject("Icn", libraries=["IconServicesLib"])
	genpluginproject("List")
	genpluginproject("Qt", libraries=["QuickTimeLib", "Cm.ppc.slb", "Qdoffs.ppc.slb"], extradirs=["::Plugins"])
	genpluginproject("Qdoffs",
		extraexportsymbols=["GWorldObj_New", "GWorldObj_Convert"])
	genpluginproject("Scrap")
	genpluginproject("Snd", libraries=["SoundLib"])
	genpluginproject("Sndihooks", sources=[":snd:Sndihooks.c"])
	genpluginproject("TE", libraries=["DragLib"])
	
	# Other Mac modules
	genpluginproject("calldll", sources=["calldll.c"])
	genpluginproject("ColorPicker")
	genpluginproject("Printing")
	genpluginproject("waste",
		sources=[
			"wastemodule.c",
			'WEAccessors.c', 'WEBirthDeath.c', 'WEDebug.c',
			'WEDrawing.c', 'WEFontTables.c', 'WEHighLevelEditing.c',
			'WEICGlue.c', 'WEInlineInput.c', 'WELineLayout.c', 'WELongCoords.c',
			'WELowLevelEditing.c', 'WEMouse.c', 'WEObjects.c', 'WEScraps.c',
			'WESelecting.c', 'WESelectors.c', 'WEUserSelectors.c', 'WEUtilities.c',
			'WEObjectHandlers.c',
			'WETabs.c',
			'WETabHooks.c'],
		libraries=['DragLib'],
		extradirs=['::::Waste 1.3 Distribution:*']
		)
	genpluginproject("ctb")
	genpluginproject("icglue", sources=["icgluemodule.c"], 
		libraries=["ICGlueCFM-PPC.lib"], 
		extradirs=["::::ICProgKit1.4:APIs"])
	genpluginproject("macspeech", libraries=["SpeechLib"])

if __name__ == '__main__':
	genallprojects()
	
