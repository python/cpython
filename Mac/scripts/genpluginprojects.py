import mkcwproject
import sys
import os
import string

PYTHONDIR = sys.prefix
PROJECTDIR = os.path.join(PYTHONDIR, ":Mac:Build")
MODULEDIRS = [	# Relative to projectdirs
	"::Modules:%s",
	"::Modules",
	":::Modules",
]

# Global variable to control forced rebuild (otherwise the project is only rebuilt
# when it is changed)
FORCEREBUILD=0

def relpath(base, path):
	"""Turn abs path into path relative to another. Only works for 2 abs paths
	both pointing to folders"""
	if not os.path.isabs(base) or not os.path.isabs(path):
		raise 'Absolute paths only'
	if base[-1] == ':':
		base = base[:-1]
	basefields = string.split(base, os.sep)
	pathfields = string.split(path, os.sep)
	commonfields = len(os.path.commonprefix((basefields, pathfields)))
	basefields = basefields[commonfields:]
	pathfields = pathfields[commonfields:]
	pathfields = ['']*(len(basefields)+1) + pathfields
	rv = string.join(pathfields, os.sep)
	return rv

def genpluginproject(architecture, module,
		project=None, projectdir=None,
		sources=[], sourcedirs=[],
		libraries=[], extradirs=[],
		extraexportsymbols=[], outputdir=":::Lib:lib-dynload",
		libraryflags=None, stdlibraryflags=None, prefixname=None):
	if architecture == "all":
		# For the time being we generate two project files. Not as nice as
		# a single multitarget project, but easier to implement for now.
		genpluginproject("ppc", module, project, projectdir, sources, sourcedirs,
				libraries, extradirs, extraexportsymbols, outputdir)
		genpluginproject("carbon", module, project, projectdir, sources, sourcedirs,
				libraries, extradirs, extraexportsymbols, outputdir)
		return
	templatename = "template-%s" % architecture
	targetname = "%s.%s" % (module, architecture)
	dllname = "%s.%s.slb" % (module, architecture)
	if not project:
		if architecture != "ppc":
			project = "%s.%s.mcp"%(module, architecture)
		else:
			project = "%s.mcp"%module
	if not projectdir:
		projectdir = PROJECTDIR
	if not sources:
		sources = [module + 'module.c']
	if not sourcedirs:
		for moduledir in MODULEDIRS:
			if '%' in moduledir:
				# For historical reasons an initial _ in the modulename
				# is not reflected in the folder name
				if module[0] == '_':
					modulewithout_ = module[1:]
				else:
					modulewithout_ = module
				moduledir = moduledir % modulewithout_
			fn = os.path.join(projectdir, os.path.join(moduledir, sources[0]))
			if os.path.exists(fn):
				moduledir, sourcefile = os.path.split(fn)
				sourcedirs = [relpath(projectdir, moduledir)]
				sources[0] = sourcefile
				break
		else:
			print "Warning: %s: sourcefile not found: %s"%(module, sources[0])
			sourcedirs = []
	if prefixname:
		pass
	elif architecture == "carbon":
		prefixname = "mwerks_carbonplugin_config.h"
	else:
		prefixname = "mwerks_plugin_config.h"
	dict = {
		"sysprefix" : relpath(projectdir, sys.prefix),
		"sources" : sources,
		"extrasearchdirs" : sourcedirs + extradirs,
		"libraries": libraries,
		"mac_outputdir" : outputdir,
		"extraexportsymbols" : extraexportsymbols,
		"mac_targetname" : targetname,
		"mac_dllname" : dllname,
		"prefixname" : prefixname,
	}
	if libraryflags:
		dict['libraryflags'] = libraryflags
	if stdlibraryflags:
		dict['stdlibraryflags'] = stdlibraryflags
	mkcwproject.mkproject(os.path.join(projectdir, project), module, dict, 
			force=FORCEREBUILD, templatename=templatename)

def	genallprojects(force=0):
	global FORCEREBUILD
	FORCEREBUILD = force
	# Standard Python modules
	genpluginproject("ppc", "pyexpat", 
		sources=["pyexpat.c", "xmlparse.c", "xmlrole.c", "xmltok.c"],
		extradirs=[":::Modules:expat"],
		prefixname="mwerks_pyexpat_config.h"
		)
	genpluginproject("carbon", "pyexpat", 
		sources=["pyexpat.c", "xmlparse.c", "xmlrole.c", "xmltok.c"],
		extradirs=[":::Modules:expat"],
		prefixname="mwerks_carbonpyexpat_config.h"
		)
	genpluginproject("all", "zlib", 
		libraries=["zlib.ppc.Lib"], 
		extradirs=["::::imglibs:zlib:mac", "::::imglibs:zlib"])
	genpluginproject("all", "gdbm", 
		libraries=["gdbm.ppc.gusi.lib"], 
		extradirs=["::::gdbm:mac", "::::gdbm"])
	genpluginproject("all", "_weakref", sources=["_weakref.c"])
	genpluginproject("all", "_symtable", sources=["symtablemodule.c"])
	# Example/test modules
	genpluginproject("all", "_testcapi")
	genpluginproject("all", "xx")
	genpluginproject("all", "xxsubtype", sources=["xxsubtype.c"])
	genpluginproject("all", "_hotshot", sources=["_hotshot.c"])
	
	# bgen-generated Toolbox modules
	genpluginproject("carbon", "_AE", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_AE", libraries=["ObjectSupportLib"], outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_App", libraries=["CarbonAccessors.o", "AppearanceLib"],
			libraryflags="Debug, WeakImport", outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_App", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Cm", libraries=["QuickTimeLib"], outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Cm", outputdir="::Lib:Carbon")
	# XXX can't work properly because we need to set a custom fragment initializer
	#genpluginproject("carbon", "_CG", 
	#		sources=["_CGModule.c", "CFMLateImport.c"],
	#		libraries=["CGStubLib"],
	#		outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Ctl", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Ctl", libraries=["CarbonAccessors.o", "ControlsLib", "AppearanceLib"], 
			libraryflags="Debug, WeakImport", outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Dlg", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Dlg", libraries=["CarbonAccessors.o", "DialogsLib", "AppearanceLib"],
			libraryflags="Debug, WeakImport", outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Drag", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Drag", libraries=["DragLib"], outputdir="::Lib:Carbon")
	genpluginproject("all", "_Evt", outputdir="::Lib:Carbon")
	genpluginproject("all", "_Fm", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Help", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Icn", libraries=["IconServicesLib"], outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Icn", outputdir="::Lib:Carbon")
	genpluginproject("all", "_List", outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Menu", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Menu", libraries=["CarbonAccessors.o", "MenusLib", "ContextualMenu", "AppearanceLib"],
			libraryflags="Debug, WeakImport", outputdir="::Lib:Carbon")
	genpluginproject("all", "_Qd", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Qt", libraries=["QuickTimeLib"], outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Qt", outputdir="::Lib:Carbon")
	genpluginproject("all", "_Qdoffs", outputdir="::Lib:Carbon")
	genpluginproject("all", "_Res", outputdir="::Lib:Carbon")
	genpluginproject("all", "_Scrap", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Snd", libraries=["CarbonAccessors.o", "SoundLib"], outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Snd", outputdir="::Lib:Carbon")
	genpluginproject("all", "_Sndihooks", sources=[":snd:_Sndihooks.c"], outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_TE", libraries=["CarbonAccessors.o", "DragLib"], outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_TE", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Mlte", libraries=["Textension"], outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Mlte", outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_Win", outputdir="::Lib:Carbon")
	genpluginproject("ppc", "_Win", libraries=["CarbonAccessors.o", "WindowsLib", "AppearanceLib"],
			libraryflags="Debug, WeakImport", outputdir="::Lib:Carbon")
	# Carbon Only?
	genpluginproject("carbon", "_CF", outputdir="::Lib:Carbon")
	genpluginproject("carbon", "_CarbonEvt", outputdir="::Lib:Carbon")
	genpluginproject("carbon", "hfsplus")
	
	# Other Mac modules
	genpluginproject("all", "calldll", sources=["calldll.c"])
	genpluginproject("all", "ColorPicker")
	genpluginproject("ppc", "Printing")
##	genpluginproject("ppc", "waste",
##		sources=[
##			"wastemodule.c",
##			'WEAccessors.c', 'WEBirthDeath.c', 'WEDebug.c',
##			'WEDrawing.c', 'WEFontTables.c', 'WEHighLevelEditing.c',
##			'WEICGlue.c', 'WEInlineInput.c', 'WELineLayout.c', 'WELongCoords.c',
##			'WELowLevelEditing.c', 'WEMouse.c', 'WEObjects.c', 'WEScraps.c',
##			'WESelecting.c', 'WESelectors.c', 'WEUserSelectors.c', 'WEUtilities.c',
##			'WEObjectHandlers.c',
##			'WETabs.c',
##			'WETabHooks.c'],
##		libraries=['DragLib'],
##		extradirs=[
##			'::::Waste 1.3 Distribution:*',
##			'::::ICProgKit1.4:APIs']
##		)
	# This is a hack, combining parts of Waste 2.0 with parts of 1.3
	genpluginproject("ppc", "waste",
		sources=[
			"wastemodule.c",
			"WEObjectHandlers.c",
			"WETabs.c", "WETabHooks.c"],
		libraries=[
			"WASTE.PPC.lib",
			"TextCommon",
			"UnicodeConverter",
			"DragLib",
			],
		extradirs=[
			'{Compiler}:MacOS Support:(Third Party Support):Waste 2.0 Distribution:C_C++ Headers',
			'{Compiler}:MacOS Support:(Third Party Support):Waste 2.0 Distribution:Static Libraries',
			'::wastemods',
			]
		)
	genpluginproject("carbon", "waste",
		sources=[
			"wastemodule.c",
			"WEObjectHandlers.c",
			"WETabs.c", "WETabHooks.c"],
		libraries=["WASTE.Carbon.lib"],
		extradirs=[
			'{Compiler}:MacOS Support:(Third Party Support):Waste 2.0 Distribution:C_C++ Headers',
			'{Compiler}:MacOS Support:(Third Party Support):Waste 2.0 Distribution:Static Libraries',
			'::wastemods',
			]
		)
##			'::::Waste 1.3 Distribution:Extras:Sample Object Handlers',
##			'::::Waste 1.3 Distribution:Extras:Waste Tabs 1.3.2']
	genpluginproject("ppc", "ctb")
	genpluginproject("ppc", "icglue", sources=["icgluemodule.c"], 
		libraries=["InternetConfigLib"])
	genpluginproject("carbon", "icglue", sources=["icgluemodule.c"])
	genpluginproject("ppc", "macspeech", libraries=["SpeechLib"])

if __name__ == '__main__':
	genallprojects()
	
	
